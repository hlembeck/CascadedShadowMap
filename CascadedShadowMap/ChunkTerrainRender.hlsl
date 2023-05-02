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

struct TileParams {
	matrix worldMatrix;
	uint2 texCoordsBase; //Instead of using UV coordinates, direct index into the texture ensures that the texture is accessed at the correct place (still possible with UV coordinates, this is for testing).
};

cbuffer TileInfo : register(b1) {
	TileParams tileParams[819];
};

cbuffer CSMCameras : register(b2) {
	matrix cameras[NFRUSTA];
}

//cbuffer Index : register(b3) {
//	uint i;
//};

struct VSOutput {
	float4 pos : SV_POSITION;
	float4 wPos : WPOS;
	float4 n : NORMAL;
	float width : WIDTH;
};

VSOutput VS(float2 pos : POSITION, uint i : SV_InstanceID) {
	VSOutput ret = (VSOutput)0;
	float2 wPos = pos;
	wPos.y = 62.0f - wPos.y;
	wPos += tileParams[i].texCoordsBase;
	float4 terrain = gTerrain.Load(int3(wPos, 0));
	ret.wPos = mul(float4(pos.x, terrain.r, pos.y, 1.0f),tileParams[i].worldMatrix);
	ret.pos = mul(ret.wPos, viewProj);
	ret.n = float4(terrain.gba,0.0f);
	ret.width = tileParams[i].worldMatrix._11;
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
	int width = log2(floor(input.width));
	float4 lodColor = { width / 3.0f, 0.0f, 0.0f, 1.0f};
	return /*GetShadowFactor(input.wPos) * *//*GetDirectionalLightFactor(input.wPos, -LIGHTDIR, input.n, direction, position) **/ lodColor;
	//return float4(.75f,0.3f,0.2f,0.0f);
}