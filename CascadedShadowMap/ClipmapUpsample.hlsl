#include "Constants.hlsl"

Texture2DArray<float> gClipmaps : register(t0);
SamplerState gSampler : register(s0);

struct VSInput {
	float4 pos : POSITION;
	float2 tPos : TPOS;
};

struct VSOutput {
	float4 pos : SV_POSITION;
	float2 tPos : TPOS;
};

VSOutput VS(VSInput input)
{
	return (VSOutput)input;
}

float PS(VSOutput input) : SV_TARGET
{
	float height = gClipmaps.SampleLevel(gSampler, float3(input.tPos,0.0f), 0.0f);
	return height;
}