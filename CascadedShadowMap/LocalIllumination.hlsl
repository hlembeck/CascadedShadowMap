#include "BasicDirectionalLight.hlsl"

cbuffer CameraConstants : register(b0) {
	matrix viewProj;
	float4 direction;
	float4 position;
};

cbuffer ObjectInstances : register(b1) {
	matrix instances[MAXINSTANCES];
}

cbuffer CSMCameras : register(b2) {
	matrix cameras[NFRUSTA];
}

Texture2DArray<float> gCSM : register(t0); //Cascaded Shadow Map
SamplerState gSampler : register(s0);
SamplerComparisonState gDepthSampler : register(s1);

struct VSInput {
	float4 pos : POSITION;
	float4 n : NORMAL;
	float2 texCoords : TEXCOORDS;
	uint instance : SV_InstanceID;
};

struct VSOutput {
	float4 pos : SV_POSITION;
	float4 wPos : WPOS;
	float4 n : NORMAL;
	float3 tPos : TPOS;
	float2 texCoords : TEXCOORDS;
};

VSOutput VS(VSInput input) {
	VSOutput output = (VSOutput)0;
	output.wPos = mul(input.pos,instances[input.instance]);
	output.pos = mul(output.wPos,viewProj);
	output.n = mul(input.n, instances[input.instance]);
	output.texCoords = input.texCoords;
	return output;
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

	for (uint i = 1; i < NFRUSTA;i++) {
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
	ret *= 0.25f*NFRUSTA;
	return saturate(ret);
}

float4 PS(VSOutput input) : SV_TARGET{
	input.n = normalize(input.n);
	//return float4(gTerrain.SampleLevel(gSampler, float3(input.texCoords,1.0f), 0.0f), 0.0f, 0.0f, 1.0f);
	return GetShadowFactor(input.wPos) * GetDirectionalLightFactor(input.wPos, -LIGHTDIR, input.n, direction, position);
}