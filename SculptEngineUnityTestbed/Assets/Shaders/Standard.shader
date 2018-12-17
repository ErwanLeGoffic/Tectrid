// Upgrade NOTE: replaced 'mul(UNITY_MATRIX_MVP,*)' with 'UnityObjectToClipPos(*)'

Shader "MyOwn/Standard"
{
	Properties
	{
		_Color("Main Color", Color) = (1,1,1,0.5)
		_SpecColor("Spec Color", Color) = (1,1,1,1)
		_Emission("Emmisive Color", Color) = (0,0,0,0)
		_Shininess("Shininess", Range(0.01, 1000)) = 0.7
	}
	SubShader
	{
		Tags{ "RenderType" = "Opaque" "Queue" = "Geometry"  "IgnoreProjector" = "True" }
		LOD 100

		ZWrite On

		Pass
		{
			Tags{ LightMode = Vertex }
			CGPROGRAM
			#pragma vertex vert  
			#pragma fragment frag

			#include "UnityCG.cginc"

			#define ADD_SPECULAR

			fixed4 _Color;
			fixed4 _SpecColor;
			fixed4 _Emission;

			half _Shininess;

			struct v2f
			{
				float4 pos : SV_POSITION;
				fixed3 diff : COLOR;
				#ifdef ADD_SPECULAR
					fixed3 spec : TEXCOORD1;
				#endif
			};

			v2f vert(appdata_full v)
			{
				v2f o;
				o.pos = UnityObjectToClipPos(v.vertex);

				float3 viewpos = mul(UNITY_MATRIX_MV, v.vertex).xyz;

				o.diff = UNITY_LIGHTMODEL_AMBIENT.xyz;

				#ifdef ADD_SPECULAR
						o.spec = 0;
						fixed3 viewDirObj = normalize(ObjSpaceViewDir(v.vertex));
				#endif

				//All calculations are in object space
				for(int i = 0; i < 1; i++)
				{
					half3 toLight = unity_LightPosition[i].xyz - viewpos.xyz * unity_LightPosition[i].w;
					half lengthSq = dot(toLight, toLight);
					half atten = 1.0 / (1.0 + lengthSq * unity_LightAtten[i].z);

					fixed3 lightDirObj = mul((float3x3)UNITY_MATRIX_T_MV, toLight);	//View => model

					lightDirObj = normalize(lightDirObj);

					fixed diff = max(0, dot(v.normal, lightDirObj));
					o.diff += unity_LightColor[i].rgb * (diff * atten);

					#ifdef ADD_SPECULAR
						fixed3 h = normalize(viewDirObj + lightDirObj);
						fixed nh = max(0, dot(v.normal, h));

						fixed spec = pow(nh, _Shininess * 128.0) * 0.5;
						o.spec += spec * unity_LightColor[i].rgb * atten;
					#endif
				}

				o.diff = (o.diff * _Color + _Emission.rgb);
				#ifdef ADD_SPECULAR
					o.spec *= _SpecColor;
				#endif
				return o;
			}

			fixed4 frag(v2f i) : COLOR
			{
				fixed4 c;

				#ifdef ADD_SPECULAR
					c.rgb = i.diff + i.spec;
				#else
					c.rgb = i.diff;
				#endif

				c.a = 1;

				return c;
			}

			ENDCG
		}
	}	
}