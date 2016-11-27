/**********************************************************
* Holo_ToolSet
* http://github.com/HasseltVR/Holo_ToolSet
* http://www.uhasselt.be/edm
*
* Distributed under LGPL v2.1 Licence
* http ://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
**********************************************************
* This class takes care of running functions on the main thread
* http://stackoverflow.com/questions/22513881/unity3d-how-to-process-events-in-the-correct-thread 
/**********************************************************/
using UnityEngine;
using System.Collections.Generic;
using System;

public class EventProcessor : MonoBehaviour
{
    void OnEnable()
    {
        SingletonService<EventProcessor>.RegisterSingletonInstance(this);
    }

    void OnDisable()
    {
        SingletonService<EventProcessor>.UnregisterSingletonInstance(this);
    }

    public void QueueEvent(Action action)
    {
        lock (m_queueLock)
        {
            m_queuedEvents.Add(action);
        }
    }

    public bool EnsureAllEventsExecuted()
    {
        return (m_executingEvents.Count > 0) ? false :true;
    }

    void Update()
    {
        MoveQueuedEventsToExecuting();

        while (m_executingEvents.Count > 0)
        {
            Action e = m_executingEvents[0];
            m_executingEvents.RemoveAt(0);
            e();
        }
    }

    private void MoveQueuedEventsToExecuting()
    {
        lock (m_queueLock)
        {
            while (m_queuedEvents.Count > 0)
            {
                Action e = m_queuedEvents[0];
                m_executingEvents.Add(e);
                m_queuedEvents.RemoveAt(0);
            }
        }
    }

    private System.Object m_queueLock = new System.Object();
    private List<Action> m_queuedEvents = new List<Action>();
    private List<Action> m_executingEvents = new List<Action>();
}
