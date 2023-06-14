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

float GetHeight(float3 pos) {
	float height = 0.7 * noise(4.0f * pos.xyz) + 0.15f * noise(8.0f * pos.yzx) + 0.15f * noise(8.0f * pos.zxy);
	height *= noise(height * pos.xyz);
	height *= 1.0f - noise(height * pos.yzx);
	height = noise(height * pos.zxy);
	return 1.0f + 0.25f * height;
}

float2 ToSpherical(float3 dir) {
	return float2(acos(dir.z), atan2(dir.y, dir.x));
}

float3 ToCartesian(float2 pos) {
	return float3(sin(pos.x) * cos(pos.y), sin(pos.x) * sin(pos.y), cos(pos.x));
}

float3 GetNormal(float3 pos) {
	const float e = 0.0001f;
	const float2 e1 = { e,0.0f };
	const float2 e2 = { 0.0f,e };

	float3 p1 = ToCartesian(ToSpherical(pos) + e1);
	float3 p2 = ToCartesian(ToSpherical(pos) + e2);

	p1 *= GetHeight(p1);
	p2 *= GetHeight(p2);
	pos *= GetHeight(pos);

	p1 -= pos;
	p2 -= pos;

	p1 = normalize(cross(p1, p2));
	return sign(dot(pos, p1)) * p1;
}