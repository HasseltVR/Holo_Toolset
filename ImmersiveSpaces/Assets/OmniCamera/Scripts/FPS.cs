/**********************************************************
* Holo_ToolSet
* http://github.com/HasseltVR/Holo_ToolSet
* http://www.uhasselt.be/edm
*
* Distributed under LGPL v2.1 Licence
* http ://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
**********************************************************/
using UnityEngine;

public class FPS : ScriptableObject {

	private int frameCount = 0;
	private float fps = 0;
	private float timeLeft = 0.5f;
	private float timePassed = 0f;
	private float updateInterval = 0.5f;
	
	// Update is called once per frame
	public void Update () 
	{
		frameCount += 1;
		timeLeft -= Time.deltaTime;
		timePassed += Time.timeScale / Time.deltaTime;

		if (timeLeft <= 0.0f)
		{
			fps = timePassed / frameCount;
			timeLeft = updateInterval;
			timePassed = 0f;
			frameCount = 0;
		}
	}

    public float GetFPS()
    {
        return fps;
    }
}
