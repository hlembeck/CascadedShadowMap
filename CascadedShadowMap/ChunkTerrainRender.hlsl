#include "BasicDirectionalLight.hlsl"

Texture2DArray<float> gCSM : register(t0); //Cascaded Shadow Map
Texture2D<float4> gTerrain : register(t1);
SamplerState gSampler : register(s0);
SamplerComparisonState gDepthSampler : register(s1);

cbuffer CameraConstants : register(b0) {
	matrix viewProj;
	float4 direction;
	float4 position;
};

cbuffer WorldMatrices : register(b1) {
	matrix matrices[64];
};

cbuffer CSMCameras : register(b2) {
	matrix cameras[NFRUSTA];
}

cbuffer Index : register(b3) {
	uint i;
};

struct VSOutput {
	float4 pos : SV_POSITION;
	float4 wPos : WPOS;
	float4 n : NORMAL;
};

VSOutput VS(float2 pos : POSITION) {
	static const float granularity = 15.0f;
	static const float d = 1.0f / 126.0f;
	VSOutput ret = (VSOutput)0;
	float2 wPos = pos;
	wPos.y = 126.0f - wPos.y;
	//ret.wPos = fmod(pos,float2(granularity,granularity));
	//ret.wPos = pos - ret.wPos;
	//ret.wPos *= d;
	float4 terrain = gTerrain.Load(int3(wPos, 0));
	ret.pos = mul(float4(pos.x, terrain.r, pos.y, 1.0f), matrices[i]);
	ret.wPos = ret.pos;
	ret.pos = mul(ret.pos, viewProj);
	ret.n = float4(terrain.gba,0.0f);
	return ret;
}

float GetShadowFactor(float4 wPos) {
	float3 tPos = mul(wPos, cameras[0]).xyz;
	tPos.xy += 1.0f;
	tPos.xy *= 0.5f;
	tPos.y = 1.0f - tPos.y;
	float ret = gCSM.SampleCmpLevelZero(gDepthSampler, float3(tPos.xy, 0.0f), tPos.z - 0.0001f);
	tPos.x += DSHADOW;
	ret += gCSM.SampleCmpLevelZero(gDepthSampler, float3(tPos.xy, 0.0f), tPos.z - 0.0001f);
	tPos.y += DSHADOW;
	ret += gCSM.SampleCmpLevelZero(gDepthSampler, float3(tPos.xy, 0.0f), tPos.z - 0.0001f);
	tPos.x -= DSHADOW;
	ret += gCSM.SampleCmpLevelZero(gDepthSampler, float3(tPos.xy, 0.0f), tPos.z - 0.0001f);

	for (uint i = 1; i < NFRUSTA; i++) {
		tPos = mul(wPos, cameras[i]).xyz;
		tPos.xy += 1.0f;
		tPos.xy *= 0.5f;
		tPos.y = 1.0f - tPos.y;
		ret += gCSM.SampleCmpLevelZero(gDepthSampler, float3(tPos.xy, i), tPos.z - 0.0001f);
		tPos.x += DSHADOW;
		ret += gCSM.SampleCmpLevelZero(gDepthSampler, float3(tPos.xy, i), tPos.z - 0.0001f);
		tPos.y += DSHADOW;
		ret += gCSM.SampleCmpLevelZero(gDepthSampler, float3(tPos.xy, i), tPos.z - 0.0001f);
		tPos.x -= DSHADOW;
		ret += gCSM.SampleCmpLevelZero(gDepthSampler, float3(tPos.xy, i), tPos.z - 0.0001f);
	}
	ret *= 0.25f * NFRUSTA;
	return saturate(ret);
}


float4 PS(VSOutput input) : SV_TARGET{
	input.n = normalize(input.n);
	return GetShadowFactor(input.wPos) * GetDirectionalLightFactor(input.wPos, -LIGHTDIR, input.n, direction, position) * float4(0.1f,0.9f,0.2f,1.0f);
}