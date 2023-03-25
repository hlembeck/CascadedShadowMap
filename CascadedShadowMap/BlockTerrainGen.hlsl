#include "Constants.hlsl"

Texture2D<float2> gTexture : register(t1);
SamplerState gSampler : register(s0);

cbuffer WorldMatrix : register(b0) {
	matrix worldMat;
};

struct VSOutput {
	float4 pos : SV_POSITION;
	float4 wPos : WPOS;
};

VSOutput VS(float2 pos : POSITION) {
	VSOutput ret = (VSOutput)0;
	ret.pos = float4(pos, 0.0f, 1.0f);
	ret.wPos = ret.pos;
	ret.wPos.xy += 1.0f;
	ret.wPos.xy *= 127.0f*0.5f; //Scale to [0,127]x[0,127]
	ret.wPos = mul(ret.wPos,worldMat);
	return ret;
}

float SmoothStep(float x) {
	return saturate(6 * x * x * x * x * x - 15 * x * x * x * x + 10 * x * x * x);
}

float Interpolate(float x, float y, float t) {
	return (y - x) * SmoothStep(t) + x;
}

float DotGrid(float2 pos, int2 tex) {
	float2 offset = pos - tex;
	return dot(offset, gTexture.Load(int3(tex, 0)));
}

float GetNoise(float2 pos, int granularity, float invGranularity) {
	static const float invRes = 1.0f / 16.0f;
	int2 texCoords = pos / granularity;
	pos -= texCoords * granularity;
	pos *= invGranularity;
	pos *= sign(pos);
	texCoords *= sign(texCoords);

	float2 tPos = texCoords;
	tPos *= invRes;

	float d1 = dot(pos, gTexture.SampleLevel(gSampler, tPos, 0.0f));
	float d2 = dot(float2(pos.x - 1.0f, pos.y), gTexture.SampleLevel(gSampler, float2(tPos.x+invRes,tPos.y), 0.0f));
	float d3 = dot(float2(pos.x, pos.y - 1.0f), gTexture.SampleLevel(gSampler, float2(tPos.x,tPos.y+invRes), 0.0f));
	float d4 = dot(float2(pos.x - 1.0f, pos.y - 1.0f), gTexture.SampleLevel(gSampler, float2(tPos.x+invRes,tPos.y+invRes), 0.0f));

	return Interpolate(Interpolate(d1, d2, pos.x), Interpolate(d3, d4, pos.x), pos.y);
}

//float GetHeightTest(float2 pos, int granularity, float invGranularity) {
//	int2 texCoords = pos / granularity;
//	pos -= texCoords * granularity;
//	pos *= invGranularity;
//	pos *= sign(pos);
//	texCoords *= sign(texCoords);
//	texCoords.x &= 15;
//	texCoords.y &= 15;
//
//	float d1 = dot(pos, gTexture.Load(float3(texCoords, 0)));
//	float d2 = dot(float2(pos.x - 1.0f, pos.y), gTexture.Load(float3(texCoords.x + 1, texCoords.y, 0)));
//	float d3 = dot(float2(pos.x, pos.y - 1.0f), gTexture.Load(float3(texCoords.x, texCoords.y + 1, 0)));
//	float d4 = dot(float2(pos.x - 1.0f, pos.y - 1.0f), gTexture.Load(float3(texCoords.x + 1, texCoords.y + 1, 0)));
//
//	//return texCoords.y/16.0f;
//	return Interpolate(Interpolate(d1, d2, pos.x), Interpolate(d3, d4, pos.x), pos.y);
//}

float GetHeight(float2 pos) {
	static const int g0 = 5;
	static const float invG0 = 0.2f;
	static const int g1 = 10;
	static const float invG1 = 0.1f;
	static const int g2 = 20;
	static const float invG2 = 1.0f / 20.0f;
	static const int g3 = 100;
	static const float invG3 = 0.01f;

	pos.xy += float2(3.0f, 6.0f);
	float ret = 3.0f * GetNoise(pos.xy, g1, invG1) * (1.0f+GetNoise(pos.xy, g0, invG0));

	pos.xy += float2(-17.0f, 31.0f);
	ret += 6.0f * GetNoise(pos.xy, g2, invG2);

	pos.xy += float2(37.0f, -107.0f);
	ret += 40.0f * GetNoise(pos.xy, g3, invG3);

	return float4(ret, 0.0f, 0.0f, 0.0f);
}

float4 PS(VSOutput input) : SV_TARGET
{
	float height = GetHeight(input.wPos.xy);
	float3 v1 = float3(1.0f,GetHeight(float2(input.wPos.x+1.0f,input.wPos.y))-height,0.0f);
	float3 v2 = float3(0.0f,GetHeight(float2(input.wPos.x, input.wPos.y+1.0f))-height,1.0f);
	return float4(height,normalize(cross(v2,v1)));
}