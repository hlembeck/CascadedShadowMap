#include "Constants.hlsl"

float4 SchlickApprox(float theta, float3 r0) {
	theta = pow(1.0f - cos(theta), 5);
	float3 temp = float3(1.0f, 1.0f, 1.0f) - r0;
	temp.x *= theta;
	temp.y *= theta;
	temp.z *= theta;
	return float4(r0 + temp, 1.0f);
}

float4 GetSpecular(float4 pos, float4 lightDirection, float4 n, float4 viewDirection, float4 viewPosition) {
	float4 ret = (float4)0;
	float4 h = normalize(normalize(viewPosition - pos) - lightDirection);
	float angle = dot(h, n);
	float scale = -dot(n, lightDirection) * dot(viewDirection, normalize(viewPosition - pos));
	ret = SchlickApprox(angle, 1.0f) * (float4(1.0f, 1.0f, 1.0f, 1.0f) - float4(1.0f, 1.0f, 1.0f, 1.0f));
	ret.x *= scale;
	ret.y *= scale;
	ret.z *= scale;
	return ret;
}

float4 GetDiffuse(float4 pos, float4 lightDirection, float4 n, float4 viewDirection, float4 viewPosition) {
	float4 ret = float4(1.0f,1.0f,1.0f,1.0f);
	float scale = dot(n, lightDirection) * dot(viewDirection, normalize(pos - viewPosition));
	ret.x *= scale;
	ret.y *= scale;
	ret.z *= scale;
	return ret;
}
 
float4 GetDirectionalLightFactor(float4 pos, float4 lightDirection, float4 n, float4 viewDirection, float4 viewPosition) {
	return clamp(GetDiffuse(pos, lightDirection, n, viewDirection, viewPosition) + GetSpecular(pos, lightDirection, n, viewDirection, viewPosition),0.1f,1.0f);
}