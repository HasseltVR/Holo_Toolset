Shader "Custom/HPV/Planar" {
    Properties {
        _MainTex ("Base (RGBA)", 2D) = "black" {}
    }
    SubShader {
        Pass {
            CGPROGRAM
            #pragma vertex vert_img
            #pragma fragment frag
			#pragma multi_compile CT_RGB CT_CoCg_Y

            #include "UnityCG.cginc"
            
            uniform sampler2D _MainTex;


			static const float4 offsets = float4(0.50196078431373, 0.50196078431373, 0.0, 0.0);
			static const float scale_factor = 255.0 / 8.0;

            fixed4 frag(v2f_img i) : SV_Target {
            	float2 tc = float2(1 - i.uv.x, i.uv.y);

				#if CT_RGB
				return tex2D(_MainTex, tc);
				#endif

				#if CT_CoCg_Y
				float4 rgba = tex2D(_MainTex, tc);

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
}