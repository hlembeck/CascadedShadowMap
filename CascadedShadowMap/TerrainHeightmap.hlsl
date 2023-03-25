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


float4 VS( float4 pos : POSITION ) : SV_POSITION
{
	return pos;
}