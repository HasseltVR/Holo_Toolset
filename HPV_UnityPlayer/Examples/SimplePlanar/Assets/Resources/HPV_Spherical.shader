Shader "Custom/HPV/Spherical" {
    Properties {
        _MainTex ("Diffuse (RGB) Transparency (A)", 2D) = "transparent" {}
    }
 
    SubShader{
        Tags { "RenderType" = "Transparent" "Queue" = "Transparent"}
        Pass {
            Cull Front
            ZWrite Off
            Blend SrcAlpha OneMinusSrcAlpha
 
            CGPROGRAM
                #pragma vertex vert
                #pragma fragment frag
				#pragma multi_compile CT_RGB CT_CoCg_Y
                #include "UnityCG.cginc"
				#define PI 3.141592653589793

				static const float4 offsets = float4(0.50196078431373, 0.50196078431373, 0.0, 0.0);
				static const float scale_factor = 255.0 / 8.0;
				sampler2D _MainTex;
 
                struct appdata {
                   float4 vertex : POSITION;
                   float3 normal : NORMAL;
                };
 
                struct v2f
                {
                    float4    pos : SV_POSITION;
                    float3    normal : TEXCOORD0;
                };
 
                v2f vert (appdata v)
                {
                    v2f o;
                    o.pos = mul(UNITY_MATRIX_MVP, v.vertex);
                    o.normal = v.normal;
                    return o;
                }
 
                inline float2 RadialCoords(float3 a_coords)
                {
                    float3 a_coords_n = normalize(a_coords);
                    float lon = atan2(a_coords_n.z, a_coords_n.x);
                    float lat = acos(a_coords_n.y);
                    float2 sphereCoords = float2(-lon, lat) / PI;
                    // adding offset to get frontal view in Unity
                    return float2(sphereCoords.x * 0.5 - 0.25, sphereCoords.y);
                }
 
                float4 frag(v2f IN) : COLOR
                {
                	// get our 
                    float2 equiUV = RadialCoords(IN.normal);
					
					#if CT_RGB
					return tex2D(_MainTex, equiUV);
					#endif

					#if CT_CoCg_Y
					float4 rgba = tex2D(_MainTex, equiUV);

					rgba -= offsets;

					float Y = rgba.a;
					float scale = rgba.b * scale_factor + 1;
					float Co = rgba.r / scale;
					float Cg = rgba.g / scale;

					float R = Y + Co - Cg;
					float G = Y + Cg;
					float B = Y - Co - Cg;

					return fixed4(R, G, B, 1);
					#endif
                }
            ENDCG
        }
    }
    FallBack "VertexLit"
}