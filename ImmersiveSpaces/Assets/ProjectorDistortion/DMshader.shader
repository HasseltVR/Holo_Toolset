Shader "Unlit/DMshader"
{
	Properties
	{
		_MainTex ("Texture", 2D) = "white" {}
		_AlphaTex ("Texture", 2D) = "white" {}
		_Gamma("Gamma Projector", Range(0,2.0)) = 1.0
	}
	SubShader
	{
		Tags { "RenderType"="Opaque" }
		LOD 100

		Pass
		{
			CGPROGRAM
			#pragma vertex vert
			#pragma fragment frag
			// make fog work
			#pragma multi_compile_fog
			
			#include "UnityCG.cginc"

			struct appdata
			{
				float4 vertex : POSITION;
				float2 uv : TEXCOORD0;
				float4 color : COLOR;
			};

			struct v2f
			{
				float2 uv : TEXCOORD0;
				UNITY_FOG_COORDS(1)
				float4 vertex : SV_POSITION;
				float4 vertexColor : COLOR;
			};

			sampler2D _MainTex;
			float4 _MainTex_ST;
			sampler2D _AlphaTex;
			float _Gamma;
			
			v2f vert (appdata v)
			{
				v2f o;
				o.vertex = mul(UNITY_MATRIX_MVP, v.vertex);
				o.uv = TRANSFORM_TEX(v.uv, _MainTex);
				o.vertexColor = v.color;
				
				return o;
			}
			
			fixed4 frag (v2f i) : SV_Target
			{
				// sample the texture
				fixed4 col = tex2D(_MainTex, i.uv);
				fixed4 alpha = tex2D(_AlphaTex, i.uv);
				
				col = col * pow(alpha, _Gamma);
			
			//	col *= i.vertexColor;
				
				return col;
			}
			ENDCG
		}
	}
}
