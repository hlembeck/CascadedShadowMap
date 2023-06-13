/*
* Fills a 2D texture with information necessary to properly render the planet with LOD. The texture format is R32G32B32_FLOAT, where the R,G channels store the lowest (low-left) corner of the tile, and the G channel stores the scaling multipler for the tile size.
*/
#include "Constants.hlsl"


RWTexture2DArray<float4> TexOut : register(u0);

[numthreads(16, 16, 1)]
void CS(uint3 gtID : SV_GroupThreadID, uint3 gID : SV_GroupID) {
	const float d = 1.0f / 16.0f;
	float4 pos = {gtID.x * d, gtID.y * d, d, 0.0f};
	TexOut[uint3(gtID.xy,gID.z)] = pos;
}