// Upgrade NOTE: replaced '_Projector' with 'unity_Projector'

Shader "Cg projector shader for omnidirectional projector" {
   Properties {
      _InputTex ("Projected Equirectangular Image", 2D) = "white" {}
	  _FOV ("Field of view", Vector) = (0, 0, 0)
	  _Rot ("Rotation", Vector) = (0, 0, 0)
   }
   SubShader {
      Pass {      
         Blend One Zero
         ZWrite Off // don't change depths
         Offset -1, -1 // avoid depth fighting
         
         CGPROGRAM
 
         #pragma vertex vert  
         #pragma fragment frag 
 
         // User-specified properties
         uniform sampler2D _InputTex; 
		 float3 _FOV;
		 float3 _Rot;
 
         // Projector-specific uniforms
         uniform float4x4 unity_Projector; 
 
          struct vertexInput {
            float4 vertex : POSITION;
            float3 normal : NORMAL;
         };
         struct vertexOutput {
            float4 pos : SV_POSITION;
            float4 posProj : TEXCOORD0; // position in projector space
         };
 
         vertexOutput vert(vertexInput input) 
         {
            vertexOutput output;
 
            output.posProj = mul(unity_Projector, input.vertex);
            output.pos = mul(UNITY_MATRIX_MVP, input.vertex);
            return output;
         }

		 #define RAD45 (3.1415926535f/4.0f)
		 #define RAD90 (3.1415926535f/2.0f)
		 #define RAD180 (3.1415926535f)
		 #define RAD360 (3.1415926535f*2.0f)
 
 
         float4 frag(vertexOutput input) : COLOR
         {
            if (input.posProj.w > 0.0) // in front of projector?
            {
				float2 coords = input.posProj.xy / input.posProj.w;
				
				//if(coords.x < 0.5) return float4(1.0, 0.0, 0.0, 1.0);
				//return float4(coords.x, coords.x, coords.x, 1.0f);

				if(coords.x < 0.0f || coords.x > 1.0f || coords.y < 0.0f || coords.y > 1.0f)
					discard;
					//return float4(0.0f, 1.0f, 0.0f, 0.0f);
				
				coords = coords * 2.0f - 1.0f; //[-1, 1]
				
				float3 pointonplanepr = normalize(float3(tan(_FOV * RAD180 / 180.0f / 2) * coords, 1.0f));
				float3 pointonplane = float3(
					1 * pointonplanepr.x + 0 * pointonplanepr.y + 0 * pointonplanepr.z,
					0 * pointonplanepr.x + cos(_Rot.x * RAD180 / 180.0f) * pointonplanepr.y - sin(_Rot.x * RAD180 / 180.0f) * pointonplanepr.z,
					0 * pointonplanepr.x + sin(_Rot.x * RAD180 / 180.0f) * pointonplanepr.y + cos(_Rot.x * RAD180 / 180.0f) * pointonplanepr.z
				 );
				pointonplane = float3(
					cos(_Rot.y * RAD180 / 180.0f) * pointonplane.x + 0 * pointonplane.y + sin(_Rot.y * RAD180 / 180.0f) * pointonplane.z,
					0 * pointonplane.x + 1 * pointonplane.y + 0 * pointonplane.z,
					-sin(_Rot.y * RAD180 / 180.0f) * pointonplane.x + 0 * pointonplane.y + cos(_Rot.y * RAD180 / 180.0f) * pointonplane.z
				 );
				pointonplane = normalize(pointonplane);
				float d = sqrt(pointonplane.x * pointonplane.x + pointonplane.z * pointonplane.z);
				float x = 2.0f * (acos(pointonplane.z / d));
				
				
				if(pointonplane.x < 0)
					x = -x;

				//x += (_Rot.x * 2.0f * RAD180 / 180.0f);
				
				/*	
				x += RAD360 + RAD360;
				if(x > RAD180) x -= RAD360;
				if(x > RAD180) x -= RAD360;
				if(x > RAD180) x -= RAD360;
				if(x > RAD180) x -= RAD360;
				if(x > RAD180) x -= RAD360;
				*/
				x /= RAD360;
				//return float4((x < -1 ? 1.0f : 0.0f), (x > 1 ? 1.0f : 0.0f), (x + 1.0f) / 2.0f, 1.0f);
				float y = (asin(pointonplane.y) /* + (_Rot.y * RAD180 / 180.0f) */ );
				if(y > RAD90)
					return float4(1.0f, 0.0f, 0.0f, 0.0f);
				if(y < -RAD90)
					return float4(0.0f, 1.0f, 0.0f, 0.0f);
				y /= RAD90;
				float2 coordsER = float2(x / 2.0f + 0.5f, y / 2.0f + 0.5f);
			//	if(coordsER.x > 1) coordsER.x -= 1;

			//	return float4(frac(1.0f * coordsER.x), frac(2.0f * coordsER.y), frac(2.0f * coordsER.y), 1.0f);
				//return float4(coordsER.x, coordsER.y, coordsER.y, 1.0f);

				/*
				float2 focal = 1.0f / tan(_FOV / 2.0f * RAD180 / 180.0f); 
				float2 angles = atan(coords / focal);
				
				angles = angles + (_Rot.xy * RAD180 / 180.0f);
				angles.x += RAD360 + RAD360;
				if(angles.y > RAD90 || angles.y < -RAD90) {
					angles.x += RAD180;
					angles.y = -angles.y;
				}
				while(angles.x > RAD360) angles.x -= RAD360;
				//while(angles.y > RAD90) angles.y -= RAD180;
				//while(angles.y < -RAD90) angles.y += RAD180;
				float2 coordsER = float2(angles.x / RAD360, (angles.y + RAD90) / RAD180);

				//return float4(frac(10.0f * angles.y), frac(10.0f * angles.y), frac(10.0f * angles.y), 1.0f);
				//return float4(frac((angles.y + RAD180) / RAD360), frac((angles.y + RAD180) / RAD360), frac((angles.y + RAD180) / RAD360), 1.0f);
				
				*/
				//return float4(frac(10.0f * x), frac(10.0f * x), 1.0f, 1.0f);
				
				return tex2D(_InputTex , coordsER); 
            }
            else // behind projector
            {
				discard;
               return float4(1.0, 0.0, 0.0, 1.0);
            }
         }
 
         ENDCG
      }
   }  
   Fallback "Projector/Light"
}