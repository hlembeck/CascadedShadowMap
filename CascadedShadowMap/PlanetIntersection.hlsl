#include "Noise.hlsl"

cbuffer CamParams : register(b0) {
	float4 PlayerPos;
};

cbuffer PlanetParams : register(b1) {
	matrix worldMatrix;
	matrix invWorldMatrix;
};

RWStructuredBuffer<float4> IntersectFlag : register(u0);

[numthreads(1, 1, 1)]
void CS(uint3 gID : SV_GroupID) {
	float4 pos = mul(PlayerPos, invWorldMatrix);

	float3 surface = normalize(pos.xyz);
	float height = GetHeight(surface);
	pos.xyz = surface * max(height, length(pos.xyz));
	IntersectFlag[0] = mul(pos,worldMatrix);
}