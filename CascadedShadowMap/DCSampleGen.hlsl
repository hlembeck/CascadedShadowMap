#include "Constants.hlsl"

struct WorldTile {
	matrix worldMatrix;
	uint4 tileCoords;
};

cbuffer TileCoords : register(b0) {
	WorldTile worldTiles[64];
};

RWTexture3D<float4> TextureOut : register(u0);

//Sphere test
float Sphere(float4 pos) {
	static const float r = 9.0f;
	return pos.x * pos.x + pos.y * pos.y + pos.z * pos.z - r;
}

float GetZero(float a, float b) {
	return a / (a - b);
}

[numthreads(16, 8, 8)]
void CS(uint3 gtID : SV_GroupThreadID, uint3 gID : SV_GroupID) {
	float4 coords = { 0.5f,0.5f,0.5f,0.0f };
	WorldTile worldTile = worldTiles[gID.x];

	uint3 localCoords = { gtID.x, gtID.y + (gID.y << 3), gtID.z + (gID.z << 3) };

	//coords.x = GetZero(Sphere(mul(float4(localCoords, 1.0f), worldTile.worldMatrix)), Sphere(mul(float4(localCoords.x+1., 1.0f), worldTile.worldMatrix)));
	
	worldTile.tileCoords.xyz += localCoords;
	TextureOut[worldTile.tileCoords.xyz] = mul(float4(localCoords,1.0f)+coords,worldTile.worldMatrix);
}