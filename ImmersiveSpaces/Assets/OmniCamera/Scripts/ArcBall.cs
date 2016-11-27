/**********************************************************
* Holo_ToolSet
* http://github.com/HasseltVR/Holo_ToolSet
* http://www.uhasselt.be/edm
*
* Distributed under LGPL v2.1 Licence
* http ://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
**********************************************************/
using UnityEngine;

[ExecuteInEditMode]
public class ArcBall : ScriptableObject
{
	#region variables
	private Vector2 	mScreenSize;
	private Vector3 	mInitialMousePos;
	private Vector2 	mCenter;
	float 				mRadius;

	private Quaternion  mInitialQuat;
	private Quaternion 	mPreviousQuat;
	private Quaternion 	mCurrentQuat;

	bool				bIsDragging;

	// gets refreshed on drag's start
	Vector3 			from;
	#endregion
	
	#region public class methods
	public void init(int _width, int _height, Quaternion _baseRotation)
	{
		setScreenSize (_width, _height);
		mInitialQuat = mPreviousQuat = mCurrentQuat = _baseRotation;
		bIsDragging = false;
	}

	public void update(Vector2 _mousPos)
	{	
		if (!bIsDragging)
		{
			mInitialMousePos = _mousPos;
			// we start at initial mouse position ...
			from = mouseOnSphere(mInitialMousePos);

			mPreviousQuat = mCurrentQuat;
			bIsDragging = true;
		}
		else if (bIsDragging)
		{
			// ... and go to current mouse position
			Vector3 to   = mouseOnSphere(_mousPos);
			// get the rotation
			Quaternion r = Quaternion.FromToRotation(from, to);
			// append our previous rotation
			mCurrentQuat = r * mPreviousQuat;
			// NORMALIZE??
		}
	}
	
	public void stop()
	{
		bIsDragging = false;
	}
	
	// Reset our orientation to face front
	public void reset()
	{
		mPreviousQuat = mCurrentQuat = mInitialQuat;
	}
	
	public Quaternion getRotation()
	{
		return mCurrentQuat;
	}

	// We need the screensize to get a correct radius for our arcball
	// This method takes care of updating all necessary values
	public void setScreenSize(int _w, int _h)
	{
		float halfW = _w / 2.0f;
		float halfH = _h / 2.0f;
		setCenter(new Vector2(halfW, halfH));
		mRadius = Mathf.Min(halfW, halfH);
	}
	#endregion

	#region private class methods
	// get front vector
	private Quaternion getRotateToFront()
	{
		// This really depends on the way we setup the sphere with our texture on
		//return Quaternion.identity;
		return Quaternion.LookRotation (new Vector3 (1, 0, 0), Vector3.up);
	}

	private void setCenter(Vector2 _center)
	{
		mCenter = _center;
	}
	
	// Get the spherical coordinates of our mousepoint
	private Vector3 mouseOnSphere(Vector3 _mousePos)
	{
		Vector3 result;
			 
		result.x = (_mousePos.x - mCenter.x) / (mRadius * 2);
        result.y = (_mousePos.y - mCenter.y) / (mRadius * 2);
        result.z = 0.0f;
		
		float mag = result.magnitude;
		if (mag > 1.0f)
		{
			result.Normalize();
		}
		else
		{
			result.z = Mathf.Sqrt(1.0f - mag);
			result.Normalize();
		}
		
		return result;
	}
	#endregion
}