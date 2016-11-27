/**********************************************************
* Holo_ToolSet
* http://github.com/HasseltVR/Holo_ToolSet
* http://www.uhasselt.be/edm
*
* Distributed under LGPL v2.1 Licence
* http ://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
**********************************************************/
Shader "Custom/Cubemap2Projection" {
  Properties {
		_MainTex ("Cubemap (RGB)", CUBE) = "" {}
	}

	Subshader {
		Pass {
			ZTest Always Cull Off ZWrite Off
			Fog { Mode off }      

			CGPROGRAM
				#pragma vertex vert
				#pragma fragment frag
				#pragma fragmentoption ARB_precision_hint_fastest
				#pragma multi_compile LM_RECTILINEAR LM_ORTHOGRAPHIC LM_EQUIDISTANT LM_STEREOGRAPHIC LM_EQUISOLIDANGLE LM_EQUIRECTANGULAR LM_CYLINDRICAL
				#include "UnityCG.cginc"
				
				struct v2f {
					float4 pos : POSITION;
					float2 uv : TEXCOORD0;
				};
				
				// helper functions for creating rotation matrices
				float3 rotateVectorAboutX(float angle, float3 vec)
				{ 
  					angle = radians(angle);
  					float3x3 rotationMatrix ={float3(1.0,0.0,0.0),
                    						  float3(0.0,cos(angle),-sin(angle)),
                    						  float3(0.0,sin(angle),cos(angle))};
  					return mul(vec, rotationMatrix);
				}

				float3 rotateVectorAboutY(float angle, float3 vec)
				{ 
 					angle = radians(angle);
  					float3x3 rotationMatrix ={float3(cos(angle),0.0,sin(angle)),
                       						  float3(0.0,1.0,0.0),
                            				  float3(-sin(angle),0.0,cos(angle))};
  					return mul(vec, rotationMatrix);
				}

				float3 rotateVectorAboutZ(float angle, float3 vec)
				{
  					angle = radians(angle);
  					float3x3 rotationMatrix ={float3(cos(angle),-sin(angle),0.0),
                            				  float3(sin(angle),cos(angle),0.0),
                           					  float3(0.0,0.0,1.0)};
  					return mul(vec, rotationMatrix);
				}
		
				samplerCUBE _MainTex;

				#define PI 3.141592653589793
				#define HALFPI 1.57079632679
				
				v2f vert( appdata_img v )
				{
					v2f o;
					o.pos = mul(UNITY_MATRIX_MVP, v.vertex);
					// norm -> snorm
					float2 uv = v.texcoord.xy * 2 - 1;
					o.uv = uv;
					return o;
				}
				
				// the transformation matrix for the lookup direction
				// coming from the arcball
				float4x4 _arcballMat;
				float focal_length = 1.0;
				float hradpermm = 1.0;
		
				float4 frag(v2f i) : COLOR 
				{
					float cosy = cos(i.uv.y);
					float3 dir = float3(0,0,0);
					//float focal_length = 2.;

					#if LM_RECTILINEAR
					dir.x = i.uv.x;
					dir.y = i.uv.y;
					dir.z = focal_length;	
					#endif	
					
					#if LM_ORTHOGRAPHIC
					float r = sqrt(i.uv.x*i.uv.x + i.uv.y*i.uv.y);
					if (r > 1.0) return float4(0.0, 0.0, 0.0, 1.0);	// circular mask
					dir.x = i.uv.x / focal_length;
					dir.y = i.uv.y / focal_length;
					dir.z = sqrt(1. - dir.x*dir.x - dir.y*dir.y);
					#endif
					
					#if LM_EQUIDISTANT		
					float r = sqrt(i.uv.x*i.uv.x+i.uv.y*i.uv.y);
					if (r > 1.0) return float4(0.0,0.0,0.0,1.0);	// circular mask
					float t = r / focal_length;
					float c = cos(t);
					float s = sin(t);
					dir.x = s*i.uv.x/r;
					dir.y = s*i.uv.y/r;
					dir.z = c;
					#endif 
	
					#if LM_STEREOGRAPHIC
					float f = 4 * focal_length;
					dir.x = i.uv.x * f;
					dir.y = i.uv.y * f;
					dir.z = f*focal_length - i.uv.x*i.uv.x - i.uv.y*i.uv.y;	
					#endif
					
					#if LM_EQUISOLIDANGLE
					float r = sqrt(i.uv.x*i.uv.x + i.uv.y*i.uv.y);
					if (r > 1.0) return float4(0.0, 0.0, 0.0, 1.0);	// circular mask
					float f = 2 * focal_length;
					dir.x = i.uv.x / focal_length;
					dir.y = i.uv.y / focal_length;
					float z2 = 1 - dir.x*dir.x - dir.y*dir.y;
					dir.z = sqrt(z2);

					dir.x *= 2*dir.z;
					dir.y *= 2*dir.z;
					dir.z  = 2*z2-1.;
					#endif
					
					#if LM_EQUIRECTANGULAR
					float _x = (i.uv.x - 0.5) * - PI;
					float _y = (i.uv.y + 1.) * HALFPI;
					dir.x =  sin(_y ) * cos(_x);
					dir.y = -cos(_y );			
					dir.z =  sin(_y ) * sin(_x);
					#endif
										
					#if LM_CYLINDRICAL
					float theta = i.uv.x * hradpermm/3.0;
					float phi = atan(i.uv.y);///**vpermm*/);
					dir.x = cos(phi)*sin(theta);
					dir.y = sin(phi);
					dir.z = cos(phi)*cos(theta);
					#endif

					// apply our arcball camera rotation
					dir = mul((float3x3)_arcballMat, dir);
					
					return texCUBE(_MainTex, dir);
				}
			ENDCG
		}
	}
	Fallback Off
}