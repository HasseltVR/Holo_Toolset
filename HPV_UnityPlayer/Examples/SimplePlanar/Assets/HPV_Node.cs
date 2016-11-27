/**********************************************************
* Holo_ToolSet
* http://github.com/HasseltVR/Holo_ToolSet
* http://www.uhasselt.be/edm
*
* Distributed under LGPL v2.1 Licence
* http ://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
**********************************************************/
using UnityEngine;
#if UNITY_EDITOR
using UnityEditor;
#endif
using System.Runtime.InteropServices;
using System;

/*
*   HPV_Node: One node streams one HPV file and controls its playback. 
*   The Node is furthermore responsible for loading the correct shader for the file
*   and allocating the Unity GPU resource. 
*
*   When in editor mode, a handy UI menu appears in the inspector controlling
*   all playback parameters.
*/
public class HPV_Node : MonoBehaviour
{
    /* reference to the HPV Manager */
    private HPV_Manager m_manager = null;

    /* Public player specific parameters */
    public string filename = "";
    public Material m_texture_target = null;
    public HPVEventDelegate onHPVEventDelegate;

    /* Private player specific parameters */
    private byte m_node_id = 0;
    private int width = 0;
    private int height = 0;
    private HPV_Unity_Bridge.HPVCompressionType hpv_type = 0;
    private bool b_needs_init = false;
    private EventProcessor m_event_processor = null;

    /* Called from manager when new HPV event occurs */
    void onHPVEvent(HPV_Unity_Bridge.HPVEventType eventID)
    {
        m_event_processor.QueueEvent(() =>
        {
            if (HPV_Unity_Bridge.HPVEventType.HPV_EVENT_PLAY == eventID)
            {
                Debug.Log("Node [" + m_node_id + "]: PLAY");
            }
            else if (HPV_Unity_Bridge.HPVEventType.HPV_EVENT_PAUSE == eventID)
            {
                Debug.Log("Node [" + m_node_id + "]: PAUSE");
            }
            else if (HPV_Unity_Bridge.HPVEventType.HPV_EVENT_STOP == eventID)
            {
                Debug.Log("Node [" + m_node_id + "]: STOP");
            }
            else if (HPV_Unity_Bridge.HPVEventType.HPV_EVENT_RESUME == eventID)
            {
                Debug.Log("Node [" + m_node_id + "]: RESUME");
            }
            else if (HPV_Unity_Bridge.HPVEventType.HPV_EVENT_LOOP == eventID)
            {
                Debug.Log("Node [" + m_node_id + "]: LOOP");
            }
            else
            {
                Debug.Log("Node [" + m_node_id + "]: unhandled HPV Event: " + eventID);
            }
        });
    }

    /* Returns the HPV Player ID of this invocated node */
    public byte getID()
    {
        return m_node_id;
    }

    /* Startup the node, opens the file */
    void Start()
    {
        m_event_processor = SingletonService<EventProcessor>.GetSingleton();
        m_manager = SingletonService<HPV_Manager>.GetSingleton();

        if (!m_event_processor)
        {
            Debug.LogError("Make sure there is exactly ONE EventProcessor.cs script in the scene.");
            return;
        }

        if (!m_manager)
        {
            Debug.LogError("Make sure there is exactly ONE HPV_Manager.cs script present in the scene.");
            return;
        }

        // init the player
        int node_id = m_manager.initPlayer(new HPVEventDelegate(onHPVEvent));
     
        if (node_id >= 0)
        {
            m_node_id = (byte)node_id;
            string filepath = Application.streamingAssetsPath + "/../../" + filename;
            int ret = m_manager.openVideo(m_node_id, filepath);
     
            if (ret == 1)
            {
                // store parameters of the file
                int fps = m_manager.getFrameRate(m_node_id);
                width = m_manager.getWidth(m_node_id);
                height = m_manager.getHeight(m_node_id);
                int number_of_frames = m_manager.getNumberOfFrames(m_node_id);
                hpv_type = m_manager.getCompressionType(m_node_id);
            
                ret = m_manager.startVideo(m_node_id);
                ret = m_manager.setLoopState(m_node_id, 1);

                Debug.Log("Opened video " + filename + " with total of " + number_of_frames + " of frames and dimensions " + width + "x" + height + " @ " + fps + "fps [TYPE: " + HPV_Unity_Bridge.HPVCompressionTypeStrings[(int)hpv_type] + "]");
                b_needs_init = true;
            }                 
        }
        else
        {
            Debug.LogError("Couldn't create new HPV_Video node!");
        }      
    }

    /* Updating the texture happens in the native plugin. 
     * The Unity texture however gets initiated when the native plugin
     * is done with allocating resources. This happens here.
     */
    void Update()
    {
        if (b_needs_init)
        {
            if (m_manager.hasResources(m_node_id) == 1)
            {
                TextureFormat fmt = 0;

                // Create a texture depending on HPV compression type of file
                if (HPV_Unity_Bridge.HPVCompressionType.HPV_TYPE_DXT1_NO_ALPHA == hpv_type)
                {
                    fmt = TextureFormat.DXT1;
                    if (m_texture_target)
                    {
                        m_texture_target.DisableKeyword("CT_CoCg_Y");
                        m_texture_target.EnableKeyword("CT_RGB");
                    }
                }
                else if (HPV_Unity_Bridge.HPVCompressionType.HPV_TYPE_DXT5_ALPHA == hpv_type)
                {
                    fmt = TextureFormat.DXT5;
                    {
                        m_texture_target.DisableKeyword("CT_CoCg_Y");
                        m_texture_target.EnableKeyword("CT_RGB");
                    }
                }
                else if (HPV_Unity_Bridge.HPVCompressionType.HPV_TYPE_SCALED_DXT5_CoCg_Y == hpv_type)
                {
                    fmt = TextureFormat.DXT5;
                    {
                        m_texture_target.DisableKeyword("CT_RGB");
                        m_texture_target.EnableKeyword("CT_CoCg_Y");
                    }
                }
                
                // get texture pointer from plugin
                IntPtr ptr = m_manager.getTexturePtr(m_node_id);
                Texture2D video_tex = Texture2D.CreateExternalTexture(width, height, fmt, false, true, ptr);
                video_tex.filterMode = FilterMode.Bilinear;

                // set texture onto our material
                if (m_texture_target)
                    m_texture_target.mainTexture = video_tex;
                // ... or if no material is present, set to texture of this GameObject
                else
                    GetComponent<Renderer>().material.mainTexture = video_tex;
            
                b_needs_init = false;

                Debug.Log("Done Creating unity texture");
            }
        }
    }

    void OnDisable()
    { 
        if (m_texture_target)
            m_texture_target.mainTexture = null;
        else
            GetComponent<Renderer>().material.mainTexture = null;
    }
}

#if UNITY_EDITOR
[CustomEditor(typeof(HPV_Node))]
public class HPV_Node_Editor : Editor
{
    private bool playState = true;
    private string playState_str = "Stop";

    private bool pauzeState = false;
    private string pauzeState_str = "Pauze";

    private float rangeMin = 0.0f;
    private float rangeMax = 1.0f;
    private int loop_in = 0;
    private int loop_out = 0;

    private string[] loop_options = new string[] { "Single Play", "Loop", "Palindrome Loop" };
    private int loop_index = 1;
    private int prev_loop_index = 1;

    private float speed = 1.0f;
    private float prev_speed = 1.0f;

    private float seek = 0;
    private float prev_seek = 0;

    private bool isNearlyEqual(float x, float y)
    {
        const float epsilon = 1e-5F;
        return Mathf.Abs(x - y) <= epsilon * Mathf.Abs(x);
    }

    private bool bShowStats = false;
    private bool prevShowStats = false;

    public override void OnInspectorGUI()
    {
        DrawDefaultInspector();

        HPV_Manager manager = SingletonService<HPV_Manager>.GetSingleton();
        if (!manager)
            return;

        byte m_node_id = ((HPV_Node)target).getID();
        GUILayoutOption[] options = { GUILayout.Width(250), GUILayout.Height(100) };

        // global vertical
        EditorGUILayout.BeginVertical(options);

        EditorGUILayout.Separator();
        // play controls horizontal
        EditorGUILayout.LabelField("Play Controls");
        EditorGUILayout.BeginHorizontal();
        if (GUILayout.Button(playState_str, GUILayout.MinWidth(75), GUILayout.ExpandWidth(true), GUILayout.MinHeight(40), GUILayout.ExpandHeight(true)))
        {
            playState = !playState;
            if (playState)
            {
                playState_str = "Stop";
                loop_in = 0;
                loop_out = manager.getNumberOfFrames(m_node_id);
                rangeMin = 0.0f;
                rangeMax = 1.0f;
                speed = 1.0f;
                prev_speed = 1.0f;
                seek = 0.0f;
                prev_seek = 0.0f;
                manager.startVideo(m_node_id);
            }
            else
            {
                playState_str = "Play";
                manager.stopVideo(m_node_id);
            }
        }
        if (playState)
        {
            if (GUILayout.Button(pauzeState_str, GUILayout.MinWidth(75), GUILayout.ExpandWidth(true), GUILayout.MinHeight(40), GUILayout.ExpandHeight(true)))
            {
                pauzeState = !pauzeState;
                if (pauzeState)
                {
                    pauzeState_str = "Resume";
                    manager.pauseVideo(m_node_id);
                }
                else
                {
                    pauzeState_str = "Pauze";
                    manager.resumeVideo(m_node_id);
                }
            }
        }

        // end play controls horizontal
        EditorGUILayout.EndHorizontal();

        // start pauze controls horizontal
        EditorGUILayout.BeginHorizontal();
        if (GUILayout.Button("<< BWD", GUILayout.MinWidth(75), GUILayout.ExpandWidth(true), GUILayout.MinHeight(40), GUILayout.ExpandHeight(true)))
        {
            manager.setDirection(m_node_id, false);
        }
        if (GUILayout.Button("FWD >>", GUILayout.MinWidth(75), GUILayout.ExpandWidth(true), GUILayout.MinHeight(40), GUILayout.ExpandHeight(true)))
        {
            manager.setDirection(m_node_id, true);
        }

        // end pauze controls horizontal
        EditorGUILayout.EndHorizontal();

        EditorGUILayout.Separator();
        EditorGUILayout.LabelField("Speed");
        speed = EditorGUILayout.Slider(speed, -10, 10);
        if (!isNearlyEqual(speed, prev_speed))
        {
            prev_speed = speed;
            manager.setSpeed(m_node_id, speed);
        }

        EditorGUILayout.Separator();
        EditorGUILayout.LabelField("Loop");
        loop_index = EditorGUILayout.Popup(loop_index, loop_options);
        if (loop_index != prev_loop_index)
        {
            manager.setLoopState(m_node_id, loop_index);
            prev_loop_index = loop_index;
        }

        EditorGUILayout.IntField(loop_in);
        EditorGUILayout.IntField(loop_out);

        EditorGUILayout.MinMaxSlider(ref rangeMin, ref rangeMax, 0.0f, 1.0f);
        if (GUILayout.Button("Update loop points"))
        {
            loop_in = (int)(rangeMin * (manager.getNumberOfFrames(m_node_id)-1));
            loop_out = (int)(rangeMax * (manager.getNumberOfFrames(m_node_id) - 1));

            manager.setLoopIn(m_node_id, loop_in);
            manager.setLoopOut(m_node_id, loop_out);
        }

        EditorGUILayout.Separator();
        EditorGUILayout.LabelField("Seek");
        seek = EditorGUILayout.Slider(seek, 0.0f, 1.0f);
        if (!isNearlyEqual(seek, prev_seek))
        {
            manager.seekToPos(m_node_id, seek);
            prev_seek = seek;
        }

        bShowStats = EditorGUILayout.Foldout(bShowStats, "Decode stats");
        if (bShowStats)
        {
            if (bShowStats != prevShowStats)
            {
                manager.enableStats(m_node_id, bShowStats);
                prevShowStats = bShowStats;
            }

            IntPtr ptr = manager.getDecodeStatsPtr(m_node_id);
            HPV_Unity_Bridge.HPVDecodeStats decode_stats = (HPV_Unity_Bridge.HPVDecodeStats)Marshal.PtrToStructure(ptr, typeof(HPV_Unity_Bridge.HPVDecodeStats));

            string stats = "HDD: " + (decode_stats.hdd_read_time / (float)1e6).ToString("F") + "ms";
            stats += "\nL4Z: " + (decode_stats.l4z_decompress_time / (float)1e6).ToString("F") + "ms";
            stats += "\nGPU: " + (decode_stats.gpu_upload_time / (float)1e6).ToString("F") + "ms";
            EditorGUILayout.HelpBox(stats, MessageType.Info);
            Repaint();
        }
        else if (!bShowStats && prevShowStats)
        {
            manager.enableStats(m_node_id, bShowStats);
            prevShowStats = bShowStats;
        }

        // end global vertical
        EditorGUILayout.EndVertical();
    }
}
#endif
