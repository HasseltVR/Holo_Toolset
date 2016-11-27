using UnityEngine;
using System.Collections;

public class setupprojectors : MonoBehaviour {

	// Use this for initialization
	void Start () {
		Projector p = this.GetComponent<Projector>();
		p.material = new Material(p.material);
		Vector2 fov = new Vector2(2.0f * Mathf.Atan(p.aspectRatio * Mathf.Tan(p.fieldOfView / 180.0f * Mathf.PI / 2.0f)) * 180.0f / Mathf.PI, p.fieldOfView);
		p.material.SetVector("_FOV", fov);
		Vector3 rot = new Vector3(this.transform.localEulerAngles.x < 90 ? this.transform.localEulerAngles.x : (180 - this.transform.localEulerAngles.x), this.transform.localEulerAngles.y, this.transform.localEulerAngles.z);
		//Vector3 rot = new Vector3(this.transform.localEulerAngles.y, this.transform.localEulerAngles.x < 90 ? -this.transform.localEulerAngles.x : -(180 - this.transform.localEulerAngles.x), this.transform.localEulerAngles.z);
		p.material.SetVector("_Rot", rot);
		Debug.Log(rot);
	}
	
	// Update is called once per frame
	void Update () {
	
	}
}
