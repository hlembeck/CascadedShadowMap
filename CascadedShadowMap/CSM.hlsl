#include "Constants.hlsl"

//cbuffer Projections : register(b0) {
//	matrix matrices[NFRUSTA];
//};

cbuffer ObjectInstances : register(b1) {
	matrix instances[MAXINSTANCES];
}

struct VSInput {
	float4 pos : POSITION;
	float4 n : NORMAL;
	uint instance : SV_InstanceID;
};

//struct GSOutput {
//	float4 pos : SV_POSITION;
//	uint slice : SV_RenderTargetArrayIndex;
//};

float4 VS(VSInput input) : SV_POSITION{
	float4 ret = mul(input.pos,instances[input.instance]);
	return mul(ret,MATRIX);
}

//[maxvertexcount(3 * NFRUSTA)]
//void GS(triangle float4 input[3] : SV_POSITION, inout TriangleStream<GSOutput> OutStream)
//{
//	GSOutput output = (GSOutput)0;
//
//	for (uint i = 0; i < NFRUSTA; i++) {
//		output.slice = i;
//		output.pos = mul(input[0], matrices[i]);
//		OutStream.Append(output);
//		output.pos = mul(input[1], matrices[i]);
//		OutStream.Append(output);
//		output.pos = mul(input[2], matrices[i]);
//		OutStream.Append(output);
//
//		OutStream.RestartStrip();
//	}
//}