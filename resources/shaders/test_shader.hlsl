#include "common.h"

struct PSInput
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
};

PSInput vs_main(float4 position : POSITION, float4 color : COLOR)
{
	PSInput result;

	result.position = position + g_offset;
	result.color = color;

	return result;
}

float4 ps_main(PSInput input) : SV_TARGET0
{
	return input.color;
}
