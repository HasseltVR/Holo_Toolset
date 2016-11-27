#include <stdio.h>
#include <vector>
#include <string>

#include "HPVEvent.h"
#include "HPVManager.h"
#include "HPVUnityRenderBridge.h"

using namespace HPV;

/* Shorthand version for dll exported functions */
#define HPV_FNC_EXPORT_VOID extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
#define HPV_FNC_EXPORT_INT extern "C" int UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
#define HPV_FNC_EXPORT_FLOAT extern "C" float UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
#define HPV_FNC_EXPORT_PTR extern "C" intptr_t UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API

static ThreadSafe_Queue<HPVEvent> m_events;
static std::string s_UnityStreamingAssetsPath;

////////////////////////////////////////////////////////////////////////////////////////
/* HPV Events */
////////////////////////////////////////////////////////////////////////////////////////

void onHPVEvent(const HPV::HPVEvent& event)
{
	m_events.push(event);
}

////////////////////////////////////////////////////////////////////////////////////////
/* HPV Unity plugin specific */
////////////////////////////////////////////////////////////////////////////////////////
extern "C" void	UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginLoad(IUnityInterfaces* unityInterfaces)
{
	HPV::InitHPVEngine(true);
	HPV::AddEventListener(&onHPVEvent);
	RendererSingleton()->load(unityInterfaces);
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginUnload()
{
	// doesn't get called??
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API SetUnityStreamingAssetsPath(const char* path)
{
	s_UnityStreamingAssetsPath = std::string(path);
}

////////////////////////////////////////////////////////////////////////////////////////
/* HPV Methods */
////////////////////////////////////////////////////////////////////////////////////////
HPV_FNC_EXPORT_VOID OnStop()
{
	ManagerSingleton()->closeAll();
	RendererSingleton()->deleteGPUResources();
}

HPV_FNC_EXPORT_INT InitPlayer()
{
	uint8_t node_id = ManagerSingleton()->initPlayer();
	RendererSingleton()->initPlayer(node_id);
	return node_id;
}

HPV_FNC_EXPORT_INT PlayerHasResources(uint8_t node_id)
{
	return RendererSingleton()->nodeHasResources(node_id);
}

HPV_FNC_EXPORT_INT OpenVideo(uint8_t node_id, const char* path)
{
	int ret = HPV_RET_ERROR;

	if (ManagerSingleton()->isValidNodeId(node_id))
	{
		ret = ManagerSingleton()->getPlayer(node_id)->open(std::string(path));
	}

	if (ret)
	{
		RendererSingleton()->scheduleCreateGPUResources(node_id);
	}

	return ret;
}

HPV_FNC_EXPORT_INT CloseVideo(uint8_t node_id)
{
	if (ManagerSingleton()->isValidNodeId(node_id))
	{
		return ManagerSingleton()->getPlayer(node_id)->close();
	}
	else
	{
		return HPV_RET_ERROR;
	}
}

HPV_FNC_EXPORT_INT StartVideo(uint8_t node_id)
{
	if (ManagerSingleton()->isValidNodeId(node_id))
	{
		return ManagerSingleton()->getPlayer(node_id)->play();
	}
	else
	{
		return HPV_RET_ERROR;
	}
}

HPV_FNC_EXPORT_INT PauseVideo(uint8_t node_id)
{
	if (ManagerSingleton()->isValidNodeId(node_id))
	{
		return ManagerSingleton()->getPlayer(node_id)->pause();
	}
	else
	{
		return HPV_RET_ERROR;
	}
}

HPV_FNC_EXPORT_INT ResumeVideo(uint8_t node_id)
{
	if (ManagerSingleton()->isValidNodeId(node_id))
	{
		return ManagerSingleton()->getPlayer(node_id)->resume();
	}
	else
	{
		return HPV_RET_ERROR;
	}
}

HPV_FNC_EXPORT_INT StopVideo(uint8_t node_id)
{
	if (ManagerSingleton()->isValidNodeId(node_id))
	{
		return ManagerSingleton()->getPlayer(node_id)->stop();
	}
	else
	{
		return HPV_RET_ERROR;
	}
}

HPV_FNC_EXPORT_INT SetLoopIn(uint8_t node_id, uint64_t loopIn)
{
	if (ManagerSingleton()->isValidNodeId(node_id))
	{
		return ManagerSingleton()->getPlayer(node_id)->setLoopInPoint(loopIn);
	}
	else
	{
		return HPV_RET_ERROR;
	}
}

HPV_FNC_EXPORT_INT SetLoopOut(uint8_t node_id, uint64_t loopOut)
{
	if (ManagerSingleton()->isValidNodeId(node_id))
	{
		return ManagerSingleton()->getPlayer(node_id)->setLoopOutPoint(loopOut);
	}
	else
	{
		return HPV_RET_ERROR;
	}
}

HPV_FNC_EXPORT_INT SetLoopState(uint8_t node_id, int loopState)
{
	if (ManagerSingleton()->isValidNodeId(node_id))
	{
		return ManagerSingleton()->getPlayer(node_id)->setLoopMode(loopState);
	}
	else
	{
		return HPV_RET_ERROR;
	}
}

HPV_FNC_EXPORT_INT GetVideoWidth(uint8_t node_id)
{
	if (ManagerSingleton()->isValidNodeId(node_id))
	{
		return ManagerSingleton()->getPlayer(node_id)->getWidth();
	}
	else
	{
		return HPV_RET_ERROR;
	}
}

HPV_FNC_EXPORT_INT GetVideoHeight(uint8_t node_id)
{
	if (ManagerSingleton()->isValidNodeId(node_id))
	{
		return ManagerSingleton()->getPlayer(node_id)->getHeight();
	}
	else
	{
		return HPV_RET_ERROR;
	}
}

HPV_FNC_EXPORT_INT GetFrameRate(uint8_t node_id)
{
	if (ManagerSingleton()->isValidNodeId(node_id))
	{
		return ManagerSingleton()->getPlayer(node_id)->getFrameRate();
	}
	else
	{
		return HPV_RET_ERROR;
	}
}

HPV_FNC_EXPORT_INT GetCompressionType(uint8_t node_id)
{
	if (ManagerSingleton()->isValidNodeId(node_id))
	{
		return static_cast<int>(ManagerSingleton()->getPlayer(node_id)->getCompressionType());
	}
	else
	{
		return HPV_RET_ERROR;
	}
}

HPV_FNC_EXPORT_INT GetCurrentFrameNum(uint8_t node_id)
{
	if (ManagerSingleton()->isValidNodeId(node_id))
	{
		return static_cast<int>(ManagerSingleton()->getPlayer(node_id)->getCurrentFrameNumber());
	}
	else
	{
		return HPV_RET_ERROR;
	}
}

HPV_FNC_EXPORT_INT GetNumberOfFrames(uint8_t node_id)
{
	if (ManagerSingleton()->isValidNodeId(node_id))
	{
		return static_cast<int>(ManagerSingleton()->getPlayer(node_id)->getNumberOfFrames());
	}
	else
	{
		return HPV_RET_ERROR;
	}
}

HPV_FNC_EXPORT_FLOAT GetPosition(uint8_t node_id)
{
	if (ManagerSingleton()->isValidNodeId(node_id))
	{
		return ManagerSingleton()->getPlayer(node_id)->getPosition();
	}
	else
	{
		return HPV_RET_ERROR;
	}
}

HPV_FNC_EXPORT_PTR GetTexPtr(uint8_t node_id)
{
	if (ManagerSingleton()->isValidNodeId(node_id))
	{
		return RendererSingleton()->getTexturePtr(node_id);
	}
	else
	{
		return HPV_RET_ERROR;
	}
}

HPV_FNC_EXPORT_INT SetSpeed(uint8_t node_id, double speed)
{
	if (ManagerSingleton()->isValidNodeId(node_id))
	{
		return ManagerSingleton()->getPlayer(node_id)->setSpeed(speed);
	}
	else
	{
		return HPV_RET_ERROR;
	}
}

HPV_FNC_EXPORT_INT SetPlayDirection(uint8_t node_id, bool direction)
{
	if (ManagerSingleton()->isValidNodeId(node_id))
	{
		return ManagerSingleton()->getPlayer(node_id)->setPlayDirection(direction);
	}
	else
	{
		return HPV_RET_ERROR;
	}
}

HPV_FNC_EXPORT_INT SeekToPos(uint8_t node_id, double pos)
{
	if (ManagerSingleton()->isValidNodeId(node_id))
	{
		return ManagerSingleton()->getPlayer(node_id)->seek(pos);
	}
	else
	{
		return HPV_RET_ERROR;
	}
}

HPV_FNC_EXPORT_INT SeekToFrame(uint8_t node_id, int64_t frame)
{
	if (ManagerSingleton()->isValidNodeId(node_id))
	{
		return ManagerSingleton()->getPlayer(node_id)->seek(frame);
	}
	else
	{
		return HPV_RET_ERROR;
	}
}

HPV_FNC_EXPORT_INT EnableStats(uint8_t node_id, bool enable)
{
	if (ManagerSingleton()->isValidNodeId(node_id))
	{
		return ManagerSingleton()->getPlayer(node_id)->enableStats(enable);
	}
	else
	{
		return HPV_RET_ERROR;
	}
}

HPV_FNC_EXPORT_PTR PassDecodeStats(uint8_t node_id)
{
	if (ManagerSingleton()->isValidNodeId(node_id))
	{
		return reinterpret_cast<intptr_t>(&ManagerSingleton()->getPlayer(node_id)->_decode_stats);
	}
	else
	{
		return HPV_RET_ERROR;
	}
}

HPV_FNC_EXPORT_INT HasEvents()
{
	return static_cast<int>(m_events.size());
}

HPV_FNC_EXPORT_INT GetLastEvent()
{
	HPVEvent last_event(HPVEventType::HPV_EVENT_NUM_TYPES, nullptr);
	if (m_events.try_pop(last_event))
	{
		int pack = ((last_event.player->_id << 16) | (((int)last_event.type) & 0xffff));
		return pack;
	}
	else
	{
		return HPV_RET_ERROR;
	}
}

////////////////////////////////////////////////////////////////////////////////////////
/* HPV Rendering */
////////////////////////////////////////////////////////////////////////////////////////
static void UNITY_INTERFACE_API OnRenderEvent(int eventID);

extern "C" UnityRenderingEvent UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetRenderEventFunc()
{
	return OnRenderEvent;
}

static void UNITY_INTERFACE_API OnRenderEvent(int eventID)
{
	HPV::Update();
}