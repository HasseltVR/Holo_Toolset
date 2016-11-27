/**********************************************************
* Holo_ToolSet
* http://github.com/HasseltVR/Holo_ToolSet
* http://www.uhasselt.be/edm
*
* Distributed under LGPL v2.1 Licence
* http ://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
**********************************************************/
using UnityEngine;
using System;
using System.Runtime.InteropServices;
using System.Collections;
using System.Collections.Generic;

/*
*   HPV_Unity_Bridge: This class links all unmanaged functionality, exposed through the
*   HPV_Unity_Bridge.dll file, to the managed Unity C# domain, and makes them publicly 
*   available through a set of static funtions. The HPV_Manager then calls into these
*   static funtions. The class also exposed the necessary HPV enums and typenames.
*/
public class HPV_Unity_Bridge
{
    //////////////////////////////////////////////////////////////////////////////////////////
    // HPV Global specific functions 													    //
    //////////////////////////////////////////////////////////////////////////////////////////
    [DllImport("HPV_Unity_Bridge", CallingConvention = CallingConvention.Cdecl)]
    public static extern void SetTextureFromUnity(System.IntPtr texture);

    [DllImport("HPV_Unity_Bridge", CallingConvention   = CallingConvention.Cdecl)]
    public static extern void SetUnityStreamingAssetsPath([MarshalAs(UnmanagedType.LPStr)] string path);

    [DllImport("HPV_Unity_Bridge", CallingConvention = CallingConvention.Cdecl)]
    public static extern IntPtr GetRenderEventFunc();

    //////////////////////////////////////////////////////////////////////////////////////////
    // HPV Node specific functions 													    	//
    //////////////////////////////////////////////////////////////////////////////////////////
    /// <summary>
    /// Tries to open a file with given path as DXT-compressed video file.
    /// </summary>	
    [DllImport("HPV_Unity_Bridge", CallingConvention = CallingConvention.Cdecl)]
    public static extern int InitPlayer();

    /// <summary>
    /// Tries to open a file with given path as DXT-compressed video file.
    /// </summary>	
    [DllImport("HPV_Unity_Bridge", CallingConvention = CallingConvention.Cdecl)]
    public static extern int PlayerHasResources(byte node_id);

    /// <summary>
    /// Tries to open a file with given path as DXT-compressed video file.
    /// </summary>	
    [DllImport("HPV_Unity_Bridge", CallingConvention = CallingConvention.Cdecl)]
    public static extern int OpenVideo(byte hpv_node_id, [MarshalAs(UnmanagedType.LPStr)] string path);

    /// <summary>
    /// Close the video file and clean up resources.
    /// </summary>	
    [DllImport("HPV_Unity_Bridge", CallingConvention = CallingConvention.Cdecl)]
    public static extern int CloseVideo(byte hpv_node_id);

    /// <summary>
    /// Start video with a certain frame-rate. 
    /// Framerate can be queried using GetFrameRate()
    /// </summary>	
    [DllImport("HPV_Unity_Bridge", CallingConvention = CallingConvention.Cdecl)]
    public static extern int StartVideo(byte hpv_node_id);

    /// <summary>
    /// Stop video with a certain frame-rate. 
    /// </summary>	
    [DllImport("HPV_Unity_Bridge", CallingConvention = CallingConvention.Cdecl)]
    public static extern int StopVideo(byte hpv_node_id);

    /// <summary>
    /// Set the loop state: 1 = looping | 0 = play once
    /// </summary>	
    [DllImport("HPV_Unity_Bridge", CallingConvention = CallingConvention.Cdecl)]
    public static extern int SetLoopState(byte hpv_node_id, int loopState);

    /// <summary>
    /// Set the loopin point
    /// </summary>	
    [DllImport("HPV_Unity_Bridge", CallingConvention = CallingConvention.Cdecl)]
    public static extern int SetLoopIn(byte hpv_node_id, int loop_in);

    /// <summary>
    /// Set the loopout point
    /// </summary>	
    [DllImport("HPV_Unity_Bridge", CallingConvention = CallingConvention.Cdecl)]
    public static extern int SetLoopOut(byte hpv_node_id, int loop_out);

    /// <summary>
    /// Set play direction: 0: normal, 1: reverse
    /// </summary>	
    [DllImport("HPV_Unity_Bridge", CallingConvention = CallingConvention.Cdecl)]
    public static extern int SetPlayDirection(byte hpv_node_id, bool direction);

    /// <summary>
    /// Returns the width of the video frame
    /// </summary>	
    [DllImport("HPV_Unity_Bridge", CallingConvention = CallingConvention.Cdecl)]
    public static extern int GetVideoWidth(byte hpv_node_id);

    /// <summary>
    /// Returns the height of the video frame
    /// </summary>	
    [DllImport("HPV_Unity_Bridge")]
    public static extern int GetVideoHeight(byte hpv_node_id);

    /// <summary>
    /// Returns the frame rate that was included as parameter at compression time.
    /// </summary>	
    [DllImport("HPV_Unity_Bridge", CallingConvention = CallingConvention.Cdecl)]
    public static extern int GetFrameRate(byte hpv_node_id);

    /// <summary>
    /// Returns the compression type of the file. 
    /// 0 = file has no alpha, 24bits RGB texture is used
    /// 1 = file has alpha, 32bits RGBA texture is used
    /// </summary>	
    [DllImport("HPV_Unity_Bridge", CallingConvention = CallingConvention.Cdecl)]
    public static extern int GetCompressionType(byte hpv_node_id);

    /// <summary>
    /// Pauzes the video. Call ResumeVideo() to resume playback.
    /// </summary>	
    [DllImport("HPV_Unity_Bridge", CallingConvention = CallingConvention.Cdecl)]
    public static extern int PauseVideo(byte hpv_node_id);

    /// <summary>
    /// Resumes video playback after PauseVideo() has been called.
    /// </summary>	
    [DllImport("HPV_Unity_Bridge")]
    public static extern int ResumeVideo(byte hpv_node_id);

    /// <summary>
    /// Reports the current frame number of the loaded videofile.
    /// </summary>	
    [DllImport("HPV_Unity_Bridge", CallingConvention = CallingConvention.Cdecl)]
    public static extern int GetCurrentFrameNum(byte hpv_node_id);

    /// <summary>
    /// Reports the total number of frames in the videofile
    /// </summary>	
    [DllImport("HPV_Unity_Bridge", CallingConvention = CallingConvention.Cdecl)]
    public static extern int GetNumberOfFrames(byte hpv_node_id);

    /// <summary>
    /// Get the native texture handle
    /// </summary>	
    [DllImport("HPV_Unity_Bridge", CallingConvention = CallingConvention.Cdecl)]
    public static extern IntPtr GetTexPtr(byte hpv_node_id);

    /// <summary>
    /// Get the actual playhead (normalized)
    /// </summary>	
    [DllImport("HPV_Unity_Bridge")]
    public static extern float GetPosition(byte hpv_node_id);

    /// <summary>
    /// Set the playback speed
    /// </summary>	
    [DllImport("HPV_Unity_Bridge", CallingConvention = CallingConvention.Cdecl)]
    public static extern int SetSpeed(byte hpv_node_id, double speed);

    /// <summary>
    /// Seek to normalized position.
    /// </summary>	
    [DllImport("HPV_Unity_Bridge", CallingConvention = CallingConvention.Cdecl)]
    public static extern int SeekToPos(byte hpv_node_id, double pos);

    /// <summary>
    /// Enable the gathering of decode stats
    /// </summary>	
    [DllImport("HPV_Unity_Bridge", CallingConvention = CallingConvention.Cdecl)]
    public static extern int EnableStats(byte hpv_node_id, bool enable);

    /// <summary>
    /// Get decode stats.
    /// </summary>	
    [DllImport("HPV_Unity_Bridge", CallingConvention = CallingConvention.Cdecl)]
    public static extern IntPtr PassDecodeStats(byte hpv_node_id);

    /// <summary>
    /// Get decode stats.
    /// </summary>	
    [DllImport("HPV_Unity_Bridge", CallingConvention = CallingConvention.Cdecl)]
    public static extern bool HasEvents();

    /// <summary>
    /// Get decode stats.
    /// </summary>	
    [DllImport("HPV_Unity_Bridge", CallingConvention = CallingConvention.Cdecl)]
    public static extern int GetLastEvent();

    /// <summary>
    /// When Unity engine stops, this function must be called to clean resources
    /// </summary>	
    [DllImport("HPV_Unity_Bridge", CallingConvention = CallingConvention.Cdecl)]
    public static extern void OnStop();

    public enum HPVCompressionType
    {
        HPV_TYPE_DXT1_NO_ALPHA = 0,
        HPV_TYPE_DXT5_ALPHA,
        HPV_TYPE_SCALED_DXT5_CoCg_Y,
        HPv_NUM_TYPES = 3
    };

    public static string[] HPVCompressionTypeStrings = new string[]
    {
        "DXT1_NO_ALPHA",
        "DXT5_ALPHA",
        "SCALED_DXT5_CoCg_Y"
    };

    public enum HPVEventType
    {
        HPV_EVENT_PLAY = 0,
        HPV_EVENT_PAUSE,
        HPV_EVENT_STOP,
        HPV_EVENT_RESUME,
        HPV_EVENT_LOOP,
        HPV_EVENT_NUM_TYPES = 5
    };

    public struct HPVDecodeStats
    {
        public ulong hdd_read_time;
        public ulong l4z_decompress_time;
        public ulong gpu_upload_time;
    }
}

/*
*   Event delegate interface for receiving HPV events
*/
public delegate void HPVEventDelegate(HPV_Unity_Bridge.HPVEventType eventID);

/*
*   HPV_Manager: this class handles all Unity HPV functionality and bridges into the native plugin
*   through the HPV_Unity_Bridge class
*/
public class HPV_Manager : MonoBehaviour
{
    /* Amount of active HPV Nodes */
    private int m_active_nodes = 0;

    /* The update loop */
    private IEnumerator HPV_update_loop;

    /* Map of all current HPV nodes registered in the system */
    public Dictionary<byte, HPVEventDelegate> hpv_node_delegates = new Dictionary<byte, HPVEventDelegate>();

    void OnEnable()
    {
        SingletonService<HPV_Manager>.RegisterSingletonInstance(this);
    }

    void OnDisable()
    {
        hpv_node_delegates.Clear();

        StopCoroutine(HPV_update_loop);
        SingletonService<HPV_Manager>.UnregisterSingletonInstance(this);
        HPV_Unity_Bridge.OnStop();
    }

    void Start()
    {
        HPV_update_loop = HPV_Update();
        StartCoroutine(HPV_update_loop);
    }

    private IEnumerator HPV_Update()
    {
        while (true)
        {
            // Wait until all Unity rendering is done
            yield return new WaitForEndOfFrame();

            // Issue the texture refresh in the video plugin
            // This also handles all internal events and makes then ready to
            // be read by the managed plugin, using GetLastEvent()
            GL.IssuePluginEvent(HPV_Unity_Bridge.GetRenderEventFunc(), 1);

            // Process HPV events and dispatch to node
            if (HPV_Unity_Bridge.HasEvents())
            {
                int ev_id = HPV_Unity_Bridge.GetLastEvent();
                byte node_id = (byte)(ushort)(ev_id >> 16);
                int event_id = (ev_id & 0xffff);

                if (hpv_node_delegates.ContainsKey(node_id))
                {
                    hpv_node_delegates[node_id]((HPV_Unity_Bridge.HPVEventType)event_id);
                }
            }
        }
    }

    public int initPlayer(HPVEventDelegate node_delegate)
    {
        int node_id = HPV_Unity_Bridge.InitPlayer();
        hpv_node_delegates.Add((byte)node_id, node_delegate);
        return node_id;
    }

    public int hasResources(byte node_id)
    {
        return HPV_Unity_Bridge.PlayerHasResources(node_id);
    }

    public int openVideo(byte hpv_node_id, string filepath)
    {
        return HPV_Unity_Bridge.OpenVideo(hpv_node_id, filepath);
    }

    public int getWidth(byte node_id)
    {
        return HPV_Unity_Bridge.GetVideoWidth(node_id);
    }

    public int getHeight(byte node_id)
    {
        return HPV_Unity_Bridge.GetVideoHeight(node_id);
    }

    public int getFrameRate(byte node_id)
    {
        return HPV_Unity_Bridge.GetFrameRate(node_id);
    }

    public int startVideo(byte node_id)
    {
        return HPV_Unity_Bridge.StartVideo(node_id);
    }

    public int stopVideo(byte node_id)
    {
        return HPV_Unity_Bridge.StopVideo(node_id);
    }
    
    public int pauseVideo(byte node_id)
    {
        return HPV_Unity_Bridge.PauseVideo(node_id);
    }

    public int resumeVideo(byte node_id)
    {
        return HPV_Unity_Bridge.ResumeVideo(node_id);
    }

    public int setDirection(byte node_id, bool direction)
    {
        return HPV_Unity_Bridge.SetPlayDirection(node_id, direction);
    }

    public int setSpeed(byte node_id, double speed)
    {
        return HPV_Unity_Bridge.SetSpeed(node_id, speed);
    }

    public int setLoopIn(byte node_id, int loop_in)
    {
        return HPV_Unity_Bridge.SetLoopIn(node_id, loop_in);
    }

    public int setLoopOut(byte node_id, int loop_out)
    {
        return HPV_Unity_Bridge.SetLoopOut(node_id, loop_out);
    }

    public int setLoopState(byte node_id, int loop_state)
    {
        return HPV_Unity_Bridge.SetLoopState(node_id, loop_state);
    }

    public int seekToPos(byte node_id, double pos)
    {
        return HPV_Unity_Bridge.SeekToPos(node_id, pos);
    }

    public int getNumberOfFrames(byte node_id)
    {
        return HPV_Unity_Bridge.GetNumberOfFrames(node_id);
    }

    public HPV_Unity_Bridge.HPVCompressionType getCompressionType(byte node_id)
    {
        return (HPV_Unity_Bridge.HPVCompressionType)HPV_Unity_Bridge.GetCompressionType(node_id);
    }

    public IntPtr getTexturePtr(byte node_id)
    {
        return HPV_Unity_Bridge.GetTexPtr(node_id);
    }

    public int enableStats(byte node_id, bool enable)
    {
        return HPV_Unity_Bridge.EnableStats(node_id, enable);
    }

    public IntPtr getDecodeStatsPtr(byte node_id)
    {
        return HPV_Unity_Bridge.PassDecodeStats(node_id);
    }
}
