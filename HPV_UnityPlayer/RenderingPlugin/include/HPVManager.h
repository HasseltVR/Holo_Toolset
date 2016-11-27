/**********************************************************
* Holo_ToolSet
* http://github.com/HasseltVR/Holo_ToolSet
* http://www.uhasselt.be/edm
*
* Distributed under LGPL v2.1 Licence
* http ://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
**********************************************************/
#pragma once

#include <string>
#include <map>
#include <vector>
#include <stdint.h>
#include <stdio.h>

#include "Log.h"
#include "HPVHeader.h"
#include "HPVPlayer.h"
#include "HPVUnityRenderBridge.h"
#include "ThreadSafeQueue.h"
#include "HPVEvent.h"

namespace HPV {

    const uint8_t MAX_NUMBER_OF_PLAYERS = 6;
    
    /*
     *  The HPVManager class is the global manager for all HPV resources.
     *  It takes care of adding and deleting new players on/from the HPV stack and updating their CPU resources.
     *  It furthermore processes all HPV related events and posts them to the provided listeners, if any.
     */
    class HPVManager
    {
    public:
        HPVManager();
        ~HPVManager();
        
        uint8_t		                initPlayer();
        HPVPlayerRef                getPlayer(uint8_t node_id);
        std::size_t                 getNumPlayers() { return m_players.size(); }
        std::vector<bool>           update();
        void                        closeAll();
        void                        processEvents();
		bool                        isValidNodeId(uint8_t node_id) { return node_id >= 0 && node_id < m_players.size(); }
        
        std::vector<HPVEventCallback> m_event_listeners;

    private:
        std::map<uint8_t, HPVPlayerRef>   m_players;
        ThreadSafe_Queue<HPVEvent>  m_event_queue;
        uint8_t                     m_num_players;
        int8_t                      addPlayer();
    };
    
    /*
     * HPVManager singleton instance and helpers
     */
    HPVManager *    ManagerSingleton();
    void            InitHPVEngine(bool log_to_file=false);
    void            DestroyHPVEngine();
    //uint8_t			NewPlayer();
    void            Update();
    
    /*
     *  Templated method to add listeners to the HPV system.
     */
    template<typename T, typename args, class ListenerClass>
    void AddEventListener(T* owner, void (ListenerClass::*listenerMethod)(args))
    {
        using namespace std::placeholders;
        HPVEventCallback cb;
        ManagerSingleton()->m_event_listeners.push_back(cb);
        ManagerSingleton()->m_event_listeners.back() = std::bind(listenerMethod, owner, _1);
    }

	/*
	*  Templated method to add listeners to the HPV system.
	*/
	template<typename args>
	void AddEventListener(void (*listenerMethod)(args))
	{
		using namespace std::placeholders;
		HPVEventCallback cb;
		ManagerSingleton()->m_event_listeners.push_back(cb);
		ManagerSingleton()->m_event_listeners.back() = std::bind(listenerMethod, _1);
	}
}

/* The HPV Manager Singleton */
extern HPV::HPVManager m_HPVManager;
