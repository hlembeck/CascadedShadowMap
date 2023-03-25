#include "Constants.hlsl"

cbuffer CSMCameras : register(b0) {
	matrix cameras[NFRUSTA];
}

cbuffer ObjectInstances : register(b1) {
	matrix instances[MAXINSTANCES];
}

struct VSInput {
	float4 pos : POSITION;
	float4 n : NORMAL;
	uint instance : SV_InstanceID;
};

struct GSOutput {
	float4 pos : SV_POSITION;
	uint slice : SV_RenderTargetArrayIndex;
};

float4 VS(VSInput input) : SV_POSITION{
	return mul(input.pos,instances[input.instance]);
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