/**********************************************************
* Holo_ToolSet
* http://github.com/HasseltVR/Holo_ToolSet
* http://www.uhasselt.be/edm
*
* Distributed under LGPL v2.1 Licence
* http ://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
**********************************************************/
using UnityEngine;
using System.Collections;

#if UNITY_EDITOR
using UnityEditor;
[InitializeOnLoad]
[ExecuteInEditMode]
[RequireComponent(typeof(Camera))]
#endif
public class ProjectionManager : MonoBehaviour {

    /* Public parameters */
    public int cubemapSize = 2048;
    public bool createEquirectangularMap = true;
    public int equirectangularSize = 2048;
    public Material cubemapToProjectionMaterial = null;
    public RenderTexture rtex_equi = null;
    public bool renderLast = true;
    public bool showModelOverlay = false;
    public float minFov = 30.0f;
    public float maxFov = 180.0f;
    public float fovSensitivity = 20.0f;
    public float fov = 90.0f;

    /* Private parameters */
    private ArcBall mArcball = null;
    private FPS mFps = null;
    private Camera cam = null;
    private RenderTexture rtex = null;
    private LensModel mLensmodel = null;

    private string realFolder = "";
    private bool bCaptureInit = false;
	
	/* SAVE TO DISK */
	public string folder = "ScreenshotMovieOutput";
	public int frameRate = 25;
	public int sizeMultiplier = 1;
	public bool saveToDisk = false;

    /* GUI */
    public GUISkin guiSkin = null;
    private int selectionGridInt = 0;
	private int prevSelectionGridInt = 0;
	private string[] LensModelNames = 
    {
		"RECTILINEAR", 
		"ORTHOGRAPHIC", 
		"EQUIDISTANT",
		"STEREOGRAPHIC",
		"EQUISOLIDANGLE", 
		"EQUIRECTANGULAR", 
		"CYLINDRICAL" 
	};

	private string selectedModel = "";
    private LensModel.LENSMODEL selectedModelID; 
	private int lastWidth;
	private int lastHeight;
	private bool bCheckForResize = true;
	private bool bGuiVisible = true;
	
	void Awake()
	{
		// start coroutine that checks for screen resize
		StartCoroutine( checkForResize() );
		
		// only one gui draw fase
		useGUILayout = false;
		
		// setup our lensmodel
		mLensmodel = ScriptableObject.CreateInstance<LensModel>();
		mLensmodel.init(LensModel.LENSMODEL.EQUIRECTANGULAR, Screen.width, Screen.height, 1.0f, 1.0f);
		
		// setup arcball
        mArcball = ScriptableObject.CreateInstance<ArcBall>();
        mArcball.init(Screen.width, Screen.height, transform.rotation);
		
		// setup FPS counter
		mFps = ScriptableObject.CreateInstance<FPS>();
	}
	
	void Start()
	{
		UpdateCubemap (63);
		
		selectionGridInt = 0;
		
		for (int i=0; i<LensModelNames.Length; ++i)
        {
			// at start select equirectangular mode
			// all others must be disabled
			if (i == (int)LensModel.LENSMODEL.EQUIRECTANGULAR)
            {
				mLensmodel.setProjection(LensModel.LENSMODEL.EQUIRECTANGULAR);
				selectedModel = LensModelNames [(int)LensModel.LENSMODEL.EQUIRECTANGULAR];
                selectedModelID = LensModel.LENSMODEL.EQUIRECTANGULAR;

                // set FOV to 45deg and corresponding focal_length in shader
                fov = 45;

				// update camera
				Camera c = this.GetComponent<Camera> ();
				c.fieldOfView = fov;

				// update fov
				float fl = mLensmodel.fov2fl (fov);
				
				if (cubemapToProjectionMaterial)
                {
                    cubemapToProjectionMaterial.EnableKeyword ("LM_EQUIRECTANGULAR");
                    cubemapToProjectionMaterial.SetFloat ("focal_length", fl);
				}
				continue;
			}
            cubemapToProjectionMaterial.DisableKeyword ("LM_" + LensModel.lensModelNames [i]);
		}
	}
	
	void Update()
	{
		// update Arcball on mouse
		if (Input.GetMouseButton (0)) 
		{
			mArcball.update (Input.mousePosition);
			this.transform.rotation = mArcball.getRotation();
		} 
		else if (Input.GetMouseButtonUp (0)) 
		{
			mArcball.stop();
		} 
		else if (Input.GetKeyDown (KeyCode.H)) 
		{
			mArcball.reset();
		} 
		else if (Input.GetKeyDown (KeyCode.G)) 
		{
			bGuiVisible = !bGuiVisible;
		}
        		
		// get scroll amount
		float scroll = Input.GetAxis("Mouse ScrollWheel");
		if (scroll > 0.0f || scroll < 0.0f)
		{
			fov += -Input.GetAxis("Mouse ScrollWheel") * fovSensitivity;

            switch (selectedModelID)
            {
                case LensModel.LENSMODEL.RECTILINEAR:
                    minFov = 1;
                    maxFov = 179;
                    break;
                case LensModel.LENSMODEL.EQUIDISTANT:
                    minFov = 1;
                    maxFov = 220;
                    break;
                case LensModel.LENSMODEL.STEREOGRAPHIC:
                    minFov = 1;
                    maxFov = 359;
                    break;
                case LensModel.LENSMODEL.EQUIRECTANGULAR:
                    minFov = 360;
                    maxFov = 360;
                    break;
                case LensModel.LENSMODEL.CYLINDRICAL:
                    minFov = 1;
                    maxFov = 179;
                    break;
                default:
                    minFov = maxFov = 360;
                    break;
            }

			fov = Mathf.Clamp(fov, minFov, maxFov);

            UpdateFOV();
		}
		
		mFps.Update();

        if (!renderLast)
        {
            UpdateCubemap(63);
        }
	}
	
	// First let the scene update all of its objects, then capture scene
	void LateUpdate () 
	{
        if (renderLast)
        {
            UpdateCubemap(63);
        }
	}

    void UpdateFOV()
    {
        Camera c = this.GetComponent<Camera>();
        c.fieldOfView = fov;

        // get the corresponding focal length for the selected lens model
        if (cubemapToProjectionMaterial != null) cubemapToProjectionMaterial.SetFloat("focal_length", mLensmodel.fov2fl(fov));
    }
	
    // Update our cubemap to send to shader
	void UpdateCubemap(int faceMask)
	{
		if (!cam)
		{
            // create empty gameobject with a camera
            GameObject go = new GameObject( "CubemapCamera"+ Random.seed, typeof(Camera));

			go.hideFlags = HideFlags.HideAndDontSave;

			// place it on the main camera (this object)
			go.GetComponent<Camera>().transform.position = this.transform.position;
			go.GetComponent<Camera>().transform.rotation = this.transform.rotation;

            // copy new camera to our main camera
            go.GetComponent<Camera>().backgroundColor = Color.black;
            //go.GetComponent<Camera>().cullingMask |= (1 << 0);
            go.GetComponent<Camera>().cullingMask = ~(1 << 8);
            go.GetComponent<Camera>().tag = "MainCamera";
            go.GetComponent<Camera>().nearClipPlane = 0.1f;
            go.GetComponent<Camera>().farClipPlane = 4000;

            go.GetComponent<Camera>().enabled = false;

			cam = go.GetComponent<Camera>();
        }
		
		if (!rtex)
		{
			rtex = new RenderTexture(cubemapSize, cubemapSize, 16);
			rtex.isPowerOfTwo = true;
			rtex.isCubemap = true;
			rtex.useMipMap = false;
			rtex.hideFlags = HideFlags.HideAndDontSave;
		}

		// render to our cubemap
		cam.transform.position = transform.position;

		// Construct a rotation matrix and set it for the shader
		var mat = Matrix4x4.TRS (Vector3.zero, transform.rotation, Vector3.one);

		//Quaternion corrected_r = transform.rotation.
		if (cubemapToProjectionMaterial != null) 
		{
            cubemapToProjectionMaterial.SetMatrix ("_arcballMat", mat);
		}
		
        // render to our render texture
		if (rtex != null) cam.RenderToCubemap(rtex, faceMask);
		
		if (!rtex_equi)
		{
			// equirectangular textures are always ratio 2:1
			rtex_equi = new RenderTexture(equirectangularSize*2, equirectangularSize, 16);
			rtex_equi.isPowerOfTwo = true;
			rtex_equi.isCubemap = false;
			rtex_equi.useMipMap = true;
			rtex_equi.generateMips = true;
			rtex_equi.antiAliasing = 8;
			rtex_equi.wrapMode = TextureWrapMode.Clamp;
			rtex_equi.filterMode = FilterMode.Trilinear;
			rtex_equi.hideFlags = HideFlags.HideAndDontSave;
		}
		
		if (createEquirectangularMap)
		{
			if (rtex && rtex_equi && cubemapToProjectionMaterial)
			{
				// blit from cubemap to projection via shader
				Graphics.Blit(rtex, rtex_equi, cubemapToProjectionMaterial);
			}
		}
		
		if (saveToDisk)
		{
			if (!bCaptureInit)
			{
				// Find a folder that doesn't exist yet by appending numbers!
				realFolder = folder;
				int count = 1;
				while (System.IO.Directory.Exists(realFolder))
				{
					realFolder = folder + count;
					count++;
				}
				// Create the folder
				System.IO.Directory.CreateDirectory(realFolder);

				// Set the playback framerate!
				// (real time doesn't influence time anymore)
				Time.captureFramerate = frameRate;

				bCaptureInit = true;
			}

			// set our equi rendertexture as the active rendertexture
			RenderTexture.active = rtex_equi;
			// create new texture for holding pixels
			Texture2D tex = new Texture2D(rtex_equi.width, rtex_equi.height, TextureFormat.RGB24, false);
			// do the readback
			tex.ReadPixels( new Rect(0, 0, tex.width, tex.height), 0, 0);
			//can help avoid errors 
			RenderTexture.active = null; 
			
			byte[] bytes = tex.EncodeToPNG();

			// name is "realFolder/shot 0005.png"
			var name = string.Format("{0}/shot {1:D04}.png", realFolder, Time.frameCount);
			
			// Capture the screenshot
			//Application.CaptureScreenshot(name, sizeMultiplier);
			System.IO.File.WriteAllBytes(name, bytes );
		}
	}
	
	void OnGUI()
	{
		if (guiSkin)
			GUI.skin = guiSkin;
		
		if (!rtex_equi) 
		{
			Debug.LogError("No texture assigned to rtex_equi!");
			return;
		}
		if (showModelOverlay)
		{
			//RenderTexture.active = rtex_equi;
			//GUI.DrawTexture(new Rect(10,10,360,180), rtex_equi, ScaleMode.ScaleToFit, false, 0);
			GUI.DrawTexture(new Rect(0,0,Screen.width, Screen.height), rtex_equi, ScaleMode.StretchToFill);
			
			if (bGuiVisible && cubemapToProjectionMaterial)
			{
				selectionGridInt = GUI.SelectionGrid(new Rect(20,10,200,160), selectionGridInt, LensModelNames,1);

				if (selectionGridInt != prevSelectionGridInt)
				{
					// select our new lens model
					mLensmodel.setProjection((LensModel.LENSMODEL)selectionGridInt);
                    cubemapToProjectionMaterial.DisableKeyword("LM_" + LensModel.lensModelNames[prevSelectionGridInt]);
                    cubemapToProjectionMaterial.EnableKeyword("LM_"  + LensModel.lensModelNames[selectionGridInt]);

                    // if LM_CYLINDRICAL, we have to update hradpermm uniform
                    cubemapToProjectionMaterial.SetFloat("hradpermm", (float)mLensmodel.getHradPerMm());
					
					// save it in system
					selectedModel = LensModel.lensModelNames[selectionGridInt];
                    selectedModelID = (LensModel.LENSMODEL)selectionGridInt;

                    prevSelectionGridInt = selectionGridInt;

                    fov = 90;
                    UpdateFOV();
				}

				if (GUI.Button(new Rect(20,190,100,20), "RESET"))
				{
					Debug.Log ("ArcBall Reset");
					mArcball.reset();
					this.transform.rotation = mArcball.getRotation();
				}

				GUI.Label (new Rect(Screen.width-200, 10, 200, 40), selectedModel + "\nFOV: " + fov.ToString());
				GUI.Label (new Rect(Screen.width-200, 40, 200, 40), "FPS: " + mFps.GetFPS().ToString());
			}
		}
	}
	
	private string OurTempSquareImageLocation()
	{
		string r = Application.persistentDataPath + "/p.png";
		Debug.Log ("Writing png to: " + r);
		return r;
	}
	
	IEnumerator checkForResize(){
		lastWidth = Screen.width;
		lastHeight = Screen.height;
		
		while(bCheckForResize)
        {
			if(lastWidth != Screen.width || lastHeight != Screen.height)
            {
				lastWidth = Screen.width;
				lastHeight = Screen.height;
				mLensmodel.setImageSize(lastWidth, lastHeight);
				mArcball.setScreenSize(lastWidth, lastHeight);
			}
			yield return new WaitForSeconds(0.3f);
		}
	}
	
	void OnDestroy()
	{
        bCheckForResize = false;
	}
	
	void OnDisable()
	{
		DestroyImmediate(cam);
		DestroyImmediate(rtex);
        ScriptableObject.DestroyImmediate (mArcball);
		ScriptableObject.DestroyImmediate (mLensmodel);
	}
}