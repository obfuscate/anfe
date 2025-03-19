#include "common.h"

struct VSInput
{
	float3 pos : POSITION;
	float2 uv : TEXCOORD0;
};

struct PSInput
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD0;
};

Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);

PSInput vs_main(VSInput i)
{
	PSInput o;

	o.pos = float4(i.pos, 1.0f) + g_offset;
	o.uv = i.uv;
	
	return o;
}

float4 ps_main(PSInput i) : SV_TARGET0
{
	return g_texture.Sample(g_sampler, i.uv);
}
