#include "common.h"

struct VSInput
{
	float3 pos : POSITION;
	float4 color : COLOR0;
	float2 uv : TEXCOORD0;
};

struct PSInput
{
	float4 pos : SV_POSITION;
	float4 color : COLOR0;
	float2 uv : TEXCOORD0;
};

Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);

PSInput vs_main(VSInput i)
{
	PSInput o;

	o.pos = mul(float4(i.pos, 1.0f), g_world);
	o.pos = mul(o.pos, g_view);
	o.pos = mul(o.pos, g_proj);
	o.color = i.color;
	o.uv = i.uv;

	return o;
}

float4 ps_main(PSInput i) : SV_TARGET0
{
	return i.color;
}
