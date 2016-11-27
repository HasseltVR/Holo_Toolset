/**********************************************************
* Holo_ToolSet
* http://github.com/HasseltVR/Holo_ToolSet
* http://www.uhasselt.be/edm
*
* Distributed under LGPL v2.1 Licence
* http ://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
**********************************************************/
using UnityEngine;

public class LensModel : ScriptableObject
{
	#region public
	public int imwidth, imheight;
	public float pixwidth, pixheight;

	public enum IMAGEORIGIN
	{
		TOPLEFT = 0,
		BOTTOMLEFT = 1,
		NRIMAGEORIGINS = 2
	}
	public IMAGEORIGIN imageOrigin;

	string[] imageOriginName = new string[] 
	{
		"TOPLEFT",
		"BOTTOMLEFT"
	};

	public enum LENSMODEL
	{
		RECTILINEAR = 0,		// r = f * tan(theta)   - rectilinear lens. theta = angle w.r.t. principal axis
		ORTHOGRAPHIC = 1,       // r = f * sin(theta)   - orthographic projection of a sphere
		EQUIDISTANT = 2,		// r = f * theta        - equidistant fish eye lens, e.g. fujinon FE series
		STEREOGRAPHIC = 3,		// r = 2*f*tan(theta/2) - less marginal object distortion, ideal for printing, aka "little planets" projection
		EQUISOLIDANGLE = 4,     // r = 2*f*sin(theta/2) - equisolid angle fish eye lens / mirror image on a ball 
		EQUIRECTANGULAR = 5,    // u proportional longitude, v proportional latitude (-hfov/2 ... +hfov/2, -vfov/2 ... +vfov/2 range within image)
		CYLINDRICAL = 6,        // u proportional longitude, v proportional tan(latitude) (-hfov/2 ... +hfov/2, -vfov/2 ... +vfov/2 range within image) 
		NRLENSMODELS = 7,
	}
	public LENSMODEL lensModel;

	static public string[] lensModelNames = new string[]
	{
		"RECTILINEAR", 
		"ORTHOGRAPHIC", 
		"EQUIDISTANT",
		"STEREOGRAPHIC",
		"EQUISOLIDANGLE", 
		"EQUIRECTANGULAR", 
		"CYLINDRICAL" 
	};

	Vector3[] eyeFlip = new Vector3[]
	{
		new Vector3(1, -1, -1),
		new Vector3(1, 1, -1)
	};

	// image circle: radius of a circle with center equal to the principal point,
	// within which the lens produces a useable image. In milimeters as measured
	// on the focal plane of the lens.
	public float imageCircleRadius;	// 0.0 means infinite radius (no delimited circle).
	
	// Intrinsic camera parameters are the parameters in above lens model formulae.
	// Use setIntrinsic() to modify these parameters. setIntrinsic() keeps the intrinsic
	// matrix K (see below) and depending variables in sync with the parameters.

	public float fl;	// focal length in mm: different meaning for different lens 
	// model (see above). Note: equirectangular and cylinderical
	// projection need hfov and vfov instead of fl.
	
	public float hfov, vfov;       // horizontal and vertical field of view angles used instead
	// of focal length for equirectangular and cylindrical
	// projection (see above) - in degrees
	
	public Vector2 pc;                // principal point (image coordinates). Corresponds to the
	// principal axis: the ray pointing straight into the 
	// view direction of the camera.
	// Ray angles theta and image distance r in the lens model 
	// formulae above are measured w.r.t. this ray and point.
	
	public float skew;             // effective skew angle, counter-clockwise, in degrees.
	// Nonzero values usually are an artifact of the calibration,
	// and only sometimes due to the lens focal plane not being
	// coincident with the camera sensor plane.
	
	public float aspect;           // effective aspect ratio (dimensionless), beyond the
	// effect of skew (division by cos(skew angle)). This is 
	// allowed to differ from pixel width / pixel height as an
	// artifact of calibration procedures.
	// 

	public void init(LENSMODEL _lensmodel, int _imwidth, int _imheight, float _pixwidth, float _pixheight)
	{
		this.imwidth = _imwidth;
		this.imheight = _imheight;
		this.pixwidth = _pixwidth;
		this.pixheight = _pixheight;

		pc = new Vector2(imwidth*0.5f, imheight*0.5f); 
		imageOrigin = IMAGEORIGIN.TOPLEFT;
		setProjection(_lensmodel);
		skew = 0;
		if (imheight > 0)
		{
			aspect = imwidth / imheight;
		}
		else
		{
			aspect = 1.0f;
		}
		//aspect = pixwidth / pixheight;
		fl = 0.5f*imwidth*pixwidth*0.001f;    /* 90 degrees horizontal field of view with rectilinear lens */
		hfov = 360;
		vfov = hfov*(imheight*pixheight) / (imwidth*pixwidth);
		if (vfov>180) vfov = 180;
		set();
	}

	public void clear()
	{
		pixwidth = pixheight = 0.0f;
		imwidth = imheight = 0;
		imageOrigin = IMAGEORIGIN.TOPLEFT;
		lensModel = LENSMODEL.RECTILINEAR;
		pc = Vector2.zero;
		fl = aspect = skew = 0.0f;
		hfov = 360; vfov = 180;
		imageCircleRadius = 0.0f;
		set();
	}

	public void defaults()
	{
		pc = new Vector2(imwidth*0.5f, imheight*0.5f);
		skew = 0;
		aspect = pixwidth / pixheight;
		fl = 0.5f*imwidth*pixwidth*0.001f;    /* 90 degrees horizontal field of view with rectilinear lens */
		hfov = 360;
		vfov = hfov*(imheight*pixheight) / (imwidth*pixwidth);
		if (vfov>180) vfov = 180;
		set();
	}

	public void set()
	{
		setIntrinsic(fl, aspect, skew, pc);
	}

	public void setIntrinsic(float _fl, float _aspect, float _skew, Vector2 _pc)
	{
		this.fl = _fl;
		this.aspect = _aspect;
		this.skew = _skew;
		this.pc = _pc;
		//precalc();     // precompute constants for toRay/fromRay
	}

	public void setProjection(LENSMODEL _model)
	{
		this.lensModel = _model;
		//precalc();
	}

	public void setImageSize(int _width, int _height)
	{
		imwidth = _width;
		imheight = _height;
	}

	// Convert field of view in degrees to corresponding focal length, taking
	// into account pixel pitch, image size and lens model. Does not take into
	// account lens distortion. Only makes sense for RECTILINEAR,
	// ORTHOGRAPHIC, EQUIDISTANCE, STEREOGRAPHIC and EQUISOLIDANGLE lens model.
	// Return 0.0 if lensModel is anything else.
	public float hfov2fl(float hfov)	// horizontal field of view
	{
		float r = 0.5f*imwidth*pixwidth*1e-3f;           // half sensor width in mm
		float t = 0.5f*hfov*Mathf.Deg2Rad;               // half horizontal field of view
		return rt2fl(r, t);
	}

	public float vfov2fl(float vfov)  // vertical field of view
	{
		float r = 0.5f*imheight*pixheight*1e-3f;         // half sensor height in mm
		float t = 0.5f*vfov*Mathf.Deg2Rad;               // half vertical field of view
		return rt2fl(r, t);
	}

	public float fov2fl(float fov)
	{
		// horizontal if wide image, vertical if high image
		return (imwidth*pixwidth >= imheight*pixheight) ? hfov2fl(fov) : vfov2fl(fov);
	}
	
	// convert distance in mm on sensor plane and corresponding angle in radians
	// w.r.t. the optical axis of the lens, to focal length in mm.
	float rt2fl(float r, float t)
	{
		switch (lensModel)
        {
		    case LENSMODEL.RECTILINEAR: return r / Mathf.Tan(t);
		    case LENSMODEL.ORTHOGRAPHIC: return 1.0f; //r / Mathf.Sin(t);
		    case LENSMODEL.EQUIDISTANT: return r / t;
		    case LENSMODEL.STEREOGRAPHIC: return r / (2 * Mathf.Tan(t / 2));
		    case LENSMODEL.EQUISOLIDANGLE: return 1.0f; //r / (2 * Mathf.Sin(t / 2));
            default: return 0.0f;
		}
	}
	
	Vector2 getImSize() { return new Vector2(imwidth, imheight); }
	Vector3 getEyeFlip(IMAGEORIGIN io) { return eyeFlip[(int)io]; }
	public double getHradPerMm() { return hradpermm; }
	public double getVperMm() { return vpermm; }
	#endregion

	#region private
	private double hradpermm, vradpermm, hmmperrad, vmmperrad, vpermm, mmperv;
	private void precalc()
	{
		if (lensModel<0 || lensModel >= LENSMODEL.NRLENSMODELS)
			Debug.LogError("precalc()::unrecognized lens model " + lensModel.ToString());
		if (imageOrigin<0 || imageOrigin >= IMAGEORIGIN.NRIMAGEORIGINS)
			Debug.LogError("CameraCalibration: unrecognized image origin id " + imageOrigin.ToString());
		
		if (hfov == 0.0) hfov = 360;
		if (vfov == 0.0) vfov = 180;
		
		// for equirectangular/cylindrical to/from ray mapping:
		hradpermm = (hfov*Mathf.Deg2Rad) / (imwidth*pixwidth*1e-3);
		vradpermm = (vfov*Mathf.Deg2Rad) / (imheight*pixheight*1e-3);
		hmmperrad = (imwidth*pixwidth*1e-3) / (hfov*Mathf.Deg2Rad);
		vmmperrad = (imheight*pixheight*1e-3) / (vfov*Mathf.Deg2Rad);
		vpermm 	  = 2.0*Mathf.Tan(vfov*0.5f*Mathf.Deg2Rad) / (imheight*pixheight*1e-3);
		mmperv    = (imheight*pixheight*1e-3) / (2.0*Mathf.Tan(vfov*0.5f*Mathf.Deg2Rad));
	}
    #endregion
}