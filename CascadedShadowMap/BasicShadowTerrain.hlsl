#include "Constants.hlsl"

Texture2D<float4> gTerrain : register(t1);
SamplerState gSampler : register(s0);

cbuffer CSMCameras : register(b0) {
	matrix cameras[NFRUSTA];
}

cbuffer WorldMatrices : register(b1) {
	matrix matrices[64];
};

cbuffer Index : register(b3) {
	uint i;
};

struct GSOutput {
	float4 pos : SV_POSITION;
	uint slice : SV_RenderTargetArrayIndex;
};

float4 VS(float2 pos : POSITION) : SV_POSITION {
	float2 wPos = pos;
	wPos.y = 126.0f - wPos.y;
	float4 ret = mul(float4(pos.x, gTerrain.Load(int3(wPos, 0)).r, pos.y, 1.0f), matrices[i]);
	return ret;
}

[maxvertexcount(3 * NFRUSTA)]
void GS(triangle float4 input[3] : SV_POSITION, inout TriangleStream<GSOutput> OutStream)
{
	GSOutput output = (GSOutput)0;

	for (uint i = 0; i < NFRUSTA; i++) {
		output.slice = i;
		output.pos = mul(input[0], cameras[i]);
		OutStream.Append(output);
		output.pos = mul(input[1], cameras[i]);
		OutStream.Append(output);
		output.pos = mul(input[2], cameras[i]);
		OutStream.Append(output);

		OutStream.RestartStrip();
	}
}