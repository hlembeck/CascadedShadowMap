struct SimpleVertex {
	float4 pos;
	float4 normal;
};

struct Constants {
	float2 position;
	float d;
};

ConstantBuffer<Constants> rootConstants : register(b0);

Texture2D<float2> gTexture : register(t0);

RWStructuredBuffer<SimpleVertex> Vertices : register(u0);

float SmoothStep(float x) {
	return saturate(6 * x * x * x * x * x - 15 * x * x * x * x + 10 * x * x * x);
}

float Interpolate(float x, float y, float t) {
	return (y - x) * SmoothStep(t) + x;
}

float DotGrid(float2 pos, int2 tex) {
	float2 offset = pos - tex;
	return dot(offset, gTexture.Load(float3(tex, 0)));
}

float GetHeight(float2 pos) {
	int2 texCoords = floor(pos);
	pos -= texCoords;
	texCoords.x &= 255;
	texCoords.y &= 255;

	float d1 = dot(pos, gTexture.Load(float3(texCoords, 0)));
	float d2 = dot(float2(pos.x - 1.0f, pos.y), gTexture.Load(float3(texCoords.x + 1, texCoords.y, 0)));
	float d3 = dot(float2(pos.x, pos.y - 1.0f), gTexture.Load(float3(texCoords.x, texCoords.y + 1, 0)));
	float d4 = dot(float2(pos.x - 1.0f, pos.y - 1.0f), gTexture.Load(float3(texCoords.x + 1, texCoords.y + 1, 0)));

	return Interpolate(Interpolate(d1, d2, pos.x), Interpolate(d3, d4, pos.x), pos.y);
}

[numthreads(512, 1, 1)]
void main(int3 dispatchThreadID : SV_DispatchThreadID) // Thread ID
{
	Vertices[dispatchThreadID.x].pos.xz += rootConstants.position;
	Vertices[dispatchThreadID.x].pos.y = GetHeight(Vertices[dispatchThreadID.x].pos.xz);

	float3 pos = Vertices[dispatchThreadID.x].pos.xyz;

	float dx = rootConstants.d * 0.25f;

	float2 p1 = { Vertices[dispatchThreadID.x].pos.x, Vertices[dispatchThreadID.x].pos.z + dx };
	float2 p2 = { Vertices[dispatchThreadID.x].pos.x + dx, Vertices[dispatchThreadID.x].pos.z };

	float3 v1 = { p1.x,GetHeight(p1),p1.y };
	float3 v2 = { p2.x,GetHeight(p2),p2.y };
	v1 -= pos;
	v2 -= pos;
	Vertices[dispatchThreadID.x].normal = float4(normalize(cross(v1, v2)), 0.0f);
}