#include "BasicDirectionalLight.hlsl"

cbuffer CameraConstants : register(b0) {
	matrix viewProj;
	float4 viewDirection;
	float4 viewPosition;
};

cbuffer PlanetParams : register(b1) {
	matrix worldMatrix;
	matrix orientation;
};

Texture3D<float4> RandTexture : register(t1);
SamplerState gSampler : register(s0);

struct VSOutput {
	float4 wPos : POSITION;
	float4 pos : SV_Position;
	float4 n : NORMAL;
	float height : HEIGHT;
};

static const float invSqrt2 = 1.0f/sqrt(2.0f);

static const float4x4 cubeFaces[6] = {
	{ //back face
		2.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 2.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 2.0f, 0.0f,
		-1.0f, -1.0f, -1.0f, 1.0f
	},
	{ //front face
		2.0f, 0.0f, 0.0f, 0.0f,
		0.0f, -2.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 2.0f, 0.0f,
		-1.0f, 1.0f, 1.0f, 1.0f
	},
	{ //left face
		0.0f, 0.0f, -2.0f, 0.0f,
		0.0f, 2.0f, 0.0f, 0.0f,
		2.0f, 0.0f, 0.0f, 0.0f,
		-1.0f, -1.0f, 1.0f, 1.0f
	},
	{ //right face
		0.0f, 0.0f, 2.0f, 0.0f,
		0.0f, 2.0f, 0.0f, 0.0f,
		-2.0f, 0.0f, 0.0f, 0.0f,
		1.0f, -1.0f, -1.0f, 1.0f
	},
	{ //bottom face
		2.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, -2.0f, 0.0f,
		0.0f, 2.0f, 0.0f, 0.0f,
		-1.0f, -1.0f, 1.0f, 1.0f
	},
	{ //top face
		2.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 2.0f, 0.0f,
		0.0f, -2.0f, 0.0f, 0.0f,
		-1.0f, 1.0f, -1.0f, 1.0f
	}
};

//static const float4x4 backFace = {
//	1.0f, 0.0f, 0.0f, 0.0f,
//	0.0f, 0.0f, -1.0f, 0.0f,
//	0.0f, 1.0f, 0.0f, 0.0f,
//	0.0f, 0.0f, -1.0f, 1.0f
//};
//
//static const float4x4 frontFace = {
//	1.0f, 0.0f, 0.0f, 0.0f,
//	0.0f, 0.0f, 1.0f, 0.0f,
//	0.0f, -1.0f, 0.0f, 0.0f,
//	0.0f, 0.0f, 1.0f, 1.0f
//};
//
//static const float4x4 leftFace = {
//	0.0f, 1.0f, 0.0f, 0.0f,
//	-1.0f, 0.0f, 0.0f, 0.0f,
//	0.0f, 0.0f, 1.0f, 0.0f,
//	-1.0f, 0.0f, 0.0f, 1.0f
//};
//
//static const float4x4 rightFace = {
//	0.0f, -1.0f, 0.0f, 0.0f,
//	1.0f, 0.0f, 0.0f, 0.0f,
//	0.0f, 0.0f, 1.0f, 0.0f,
//	1.0f, 0.0f, 0.0f, 1.0f
//};
//
//static const float4x4 bottomFace = {
//	1.0f, 0.0f, 0.0f, 0.0f,
//	0.0f, -1.0f, 0.0f, 0.0f,
//	0.0f, 0.0f, -1.0f, 0.0f,
//	0.0f, -1.0f, 0.0f, 1.0f
//};
//
//static const float4x4 topFace = {
//	1.0f, 0.0f, 0.0f, 0.0f,
//	0.0f, 1.0f, 0.0f, 0.0f,
//	0.0f, 0.0f, 1.0f, 0.0f,
//	0.0f, 1.0f, 0.0f, 1.0f
//};

//float GetNoise(float3 pos) {
//	const float3 dX = { 1.0f / 16.0f, 0.0f, 0.0f }; //Inverse of number of samples.
//	const float3 dY = { 0.0f, 1.0f / 16.0f, 0.0f };
//	const float3 dZ = { 0.0f, 0.0f, 1.0f / 16.0f };
//	float3 localCoords = fmod(pos, dX + dY + dZ);
//	pos -= localCoords;
//
//	float4 v1 = RandTexture.SampleLevel(gSampler, pos, 0.0f);
//	float4 v2 = RandTexture.SampleLevel(gSampler, pos + dX, 0.0f);
//	float4 v3 = RandTexture.SampleLevel(gSampler, pos + dY, 0.0f);
//	float4 v4 = RandTexture.SampleLevel(gSampler, pos + dZ, 0.0f);
//	float4 v5 = RandTexture.SampleLevel(gSampler, pos + dX + dY, 0.0f);
//	float4 v6 = RandTexture.SampleLevel(gSampler, pos + dX + dZ, 0.0f);
//	float4 v7 = RandTexture.SampleLevel(gSampler, pos + dY + dZ, 0.0f);
//	float4 v8 = RandTexture.SampleLevel(gSampler, pos + dX + dY + dZ, 0.0f);
//
//	float p1 = dot(localCoords, v1);
//	float p2 = dot(localCoords - (pos + dX), v2);
//	float p3 = dot(localCoords - (pos + dY), v3);
//	float p4 = dot(localCoords - (pos + dZ), v4);
//	float p5 = dot(localCoords - (pos + dX + dY), v5);
//	float p6 = dot(localCoords - (pos + dX + dZ), v6);
//	float p7 = dot(localCoords - (pos + dY + dZ), v7);
//	float p8 = dot(localCoords - (pos + dX + dY + dZ), v8);
//
//	
//	float i1 = p1 + (p2 - p1) * smoothstep(0.0f, 1.0f / 16.0f, localCoords.x);
//	float i2 = p3 + (p5 - p3) * smoothstep(0.0f, 1.0f / 16.0f, localCoords.x);
//	float ret = i1 + (i2 - i1) * smoothstep(0.0f, 1.0f / 16.0f, localCoords.y);
//
//	i1 = p4 + (p6 - p4) * smoothstep(0.0f, 1.0f / 16.0f, localCoords.x);
//	i2 = p7 + (p8 - p7) * smoothstep(0.0f, 1.0f / 16.0f, localCoords.x);
//	i1 += (i2 - i1) * smoothstep(0.0f, 1.0f / 16.0f, localCoords.y);
//
//	ret += (i1 - ret) * smoothstep(0.0f, 1.0f / 16.0f, localCoords.z);
//
//	return ret;
//}

//float GetNoise(float3 pos) {
//	pos += float3(1.0f, 1.0f, 1.0f);
//	pos *= 0.5f;
//	const float3 dX = { 1.0f / 16.0f, 0.0f, 0.0f }; //Inverse of number of samples.
//	const float3 dY = { 0.0f, 1.0f / 16.0f, 0.0f };
//	const float3 dZ = { 0.0f, 0.0f, 1.0f / 16.0f };
//	float3 localCoords = fmod(pos, dX + dY + dZ);
//	pos -= localCoords;
//
//	float4 v1 = RandTexture.SampleLevel(gSampler, pos, 0.0f);
//	float4 v2 = RandTexture.SampleLevel(gSampler, pos + dX, 0.0f);
//	float4 v3 = RandTexture.SampleLevel(gSampler, pos + dY, 0.0f);
//	float4 v4 = RandTexture.SampleLevel(gSampler, pos + dZ, 0.0f);
//	float4 v5 = RandTexture.SampleLevel(gSampler, pos + dX + dY, 0.0f);
//	float4 v6 = RandTexture.SampleLevel(gSampler, pos + dX + dZ, 0.0f);
//	float4 v7 = RandTexture.SampleLevel(gSampler, pos + dY + dZ, 0.0f);
//	float4 v8 = RandTexture.SampleLevel(gSampler, pos + dX + dY + dZ, 0.0f);
//
//	float p1 = dot(localCoords, v1);
//	float p2 = dot(localCoords - (pos + dX), v2);
//	float p3 = dot(localCoords - (pos + dY), v3);
//	float p4 = dot(localCoords - (pos + dZ), v4);
//	float p5 = dot(localCoords - (pos + dX + dY), v5);
//	float p6 = dot(localCoords - (pos + dX + dZ), v6);
//	float p7 = dot(localCoords - (pos + dY + dZ), v7);
//	float p8 = dot(localCoords - (pos + dX + dY + dZ), v8);
//
//
//	float i1 = p1 + (p2 - p1) * smoothstep(0.0f, 1.0f / 16.0f, localCoords.x);
//	float i2 = p3 + (p5 - p3) * smoothstep(0.0f, 1.0f / 16.0f, localCoords.x);
//	float ret = i1 + (i2 - i1) * smoothstep(0.0f, 1.0f / 16.0f, localCoords.y);
//
//	i1 = p4 + (p6 - p4) * smoothstep(0.0f, 1.0f / 16.0f, localCoords.x);
//	i2 = p7 + (p8 - p7) * smoothstep(0.0f, 1.0f / 16.0f, localCoords.x);
//	i1 += (i2 - i1) * smoothstep(0.0f, 1.0f / 16.0f, localCoords.y);
//
//	ret += (i1 - ret) * smoothstep(0.0f, 1.0f / 16.0f, localCoords.z);
//
//	return ret;
//}

// hash based 3d value noise
// Found on https://stackoverflow.com/questions/15628039/simplex-noise-shader
// function taken from https://www.shadertoy.com/view/XslGRr
// Created by inigo quilez - iq/2013
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
// ported from GLSL to HLSL
//START SOURCE
float hash(float n)
{
	return frac(sin(n) * 43758.5453);
}

float noise(float3 x)
{
	// The noise function returns a value in the range -1.0f -> 1.0f

	float3 p = floor(x);
	float3 f = frac(x);

	f = f * f * (3.0 - 2.0 * f);
	float n = p.x + p.y * 57.0 + 113.0 * p.z;

	return lerp(lerp(lerp(hash(n + 0.0), hash(n + 1.0), f.x),
		lerp(hash(n + 57.0), hash(n + 58.0), f.x), f.y),
		lerp(lerp(hash(n + 113.0), hash(n + 114.0), f.x),
			lerp(hash(n + 170.0), hash(n + 171.0), f.x), f.y), f.z);
}
//END SOURCE


float2 GetCoordinates(uint instance) {
	return float2(instance % 8, instance / 8);
}

float3 GetNormal(float3 pos) {
	float e = 0.0001f;



	return pos;
}

VSOutput VS(float4 input : POSITION, uint instance : SV_InstanceID) {
	const float d = 1.0f / 8.0f;
	input.xy = d * (input.xy + GetCoordinates(instance / 6));
	const float4 unit = { 1.0f,1.0f,1.0f,0.0f };
	VSOutput ret = (VSOutput)0;
	ret.pos = input;
	ret.pos = mul(ret.pos, cubeFaces[instance % 6]);
	ret.pos.xyz = normalize(ret.pos.xyz);
	ret.pos = mul(ret.pos, orientation);
	ret.n = float4(ret.pos.xyz, 0.0f);



	float height = noise(10.0f*unit + ret.pos.xyz * 5.0f);
	ret.pos.xyz *= 1.0f + 0.25f * height;
	ret.height = height;

	//float3 texCoords = (ret.pos.xyz+unit)*0.5f;

	ret.pos = mul(ret.pos, worldMatrix);
	//ret.n = normalize(mul(ret.n, worldMatrix)); //Not necessary here, but in general should be done.
	ret.wPos = ret.pos;
	ret.pos = mul(ret.pos,viewProj);
	return ret;
}

float4 PS(VSOutput input) : SV_TARGET{
	const float4 unit = {1.0f,0.0f,1.0f,0.0f};
	return unit * input.height;
	//return GetDirectionalLightFactor(input.wPos, -LIGHTDIR, input.n, viewDirection, viewPosition);
}