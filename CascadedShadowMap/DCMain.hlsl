#include "Constants.hlsl"

struct WorldTile {
	matrix worldMatrix;
	int4 tileCoords;
};

cbuffer CameraConstants : register(b0) {
	matrix viewProj;
	float4 direction;
	float4 position;
};

cbuffer TileCoords : register(b2) {
	WorldTile worldTiles[64];
};

Texture3D<float4> Samples : register(t0);

struct VSOutput {
	int4 tPos : TPOS;
	uint instance : INSTANCE;
};

struct GSOutput {
	float4 pos : SV_POSITION;
	float4 wPos : WPOS;
	float tilePos : TILEPOS;
};

VSOutput VS(float4 input : POSITION, uint instance : SV_InstanceID) {
	VSOutput ret = (VSOutput)0;
	ret.tPos = input;
	ret.instance = instance;
	return ret;
}

//Sphere test
float Sphere(float3 pos) {
	static const float r = 9.0f;
	return pos.x * pos.x + pos.y * pos.y + pos.z * pos.z - r;
}

/*
Lower sign -> {0,1,2} (neg,zero,pos)
Upper sign -> {0,3,6}
edgeSign = map(Lower sign) + map(Upper sign)
*/
static const int2 signMaps[9] = {
	{0,0}, //Both negative maps to degenerate triangle (duplicate vertices)
	{0,0}, //For TESTING, zero lower and negative upper maps to degenerate (lower lies on surface, should not generate triangle on the edge)
	{1,0}, //lower positive, upper negative, generate downward-facing quad
	{0,0},{0,0},{0,0}, //lower=0
	{0,1}, //upper positive, lower negative, gen upward
	{0,0},
	{0,0}
};


//6 vertices per quad, max 3 quads (one per outward edge from vertex)
[maxvertexcount(18)]
void GS(point VSOutput input[1], inout TriangleStream<GSOutput> OutputStream) {
	static const int4 end = { 16,16,16,0 };
	static const int4 e1 = { 1,0,0,0 };
	static const int4 e2 = { 0,1,0,0 };
	static const int4 e3 = { 0,0,1,0 };

	WorldTile worldTile = worldTiles[input[0].instance];
	const int4 tile = worldTile.tileCoords;
	worldTile.tileCoords.xyz += input[0].tPos.xyz;

	GSOutput output = (GSOutput)0;
	output.tilePos = worldTile.worldMatrix._14/5.0f;

	int4 currSample;

	//int edgeSign = (1 + sign(Sphere(mul(input[0].tPos, worldTile.worldMatrix).xyz))) + (1 + sign(Sphere(mul(input[0].tPos + e1, worldTile.worldMatrix).xyz))) * 3;

	//right-edge
	if (Sphere(mul(input[0].tPos, worldTile.worldMatrix).xyz) < 0.0f && Sphere(mul(input[0].tPos + e1, worldTile.worldMatrix).xyz) > 0.0f) {
		currSample = max(tile, worldTile.tileCoords - e2 - e3);
		//currSample = clamp(worldTile.tileCoords - e2 - e3, tile, tile + end);
		output.wPos = Samples.Load(currSample);
		output.pos = mul(output.wPos, viewProj);
		OutputStream.Append(output);

		currSample = clamp(worldTile.tileCoords - e3, tile, tile + end);
		output.wPos = Samples.Load(currSample);
		output.pos = mul(output.wPos, viewProj);
		OutputStream.Append(output);

		currSample = clamp(worldTile.tileCoords, tile, tile + end);
		output.wPos = Samples.Load(currSample);
		output.pos = mul(output.wPos, viewProj);
		OutputStream.Append(output);

		OutputStream.RestartStrip();

		currSample = max(tile, worldTile.tileCoords - e2 - e3);
		//currSample = clamp(worldTile.tileCoords - e2 - e3, tile, tile + end);
		output.wPos = Samples.Load(currSample);
		output.pos = mul(output.wPos, viewProj);
		OutputStream.Append(output);

		currSample = worldTile.tileCoords;
		//currSample = clamp(worldTile.tileCoords, tile, tile + end);
		output.wPos = Samples.Load(currSample);
		output.pos = mul(output.wPos, viewProj);
		OutputStream.Append(output);

		currSample = max(tile, worldTile.tileCoords - e2);
		//currSample = clamp(worldTile.tileCoords - e2, tile, tile + end);
		output.wPos = Samples.Load(currSample);
		output.pos = mul(output.wPos, viewProj);
		OutputStream.Append(output);


		OutputStream.RestartStrip();
	}
	if (Sphere(mul(input[0].tPos, worldTile.worldMatrix).xyz) > 0.0f && Sphere(mul(input[0].tPos + e1, worldTile.worldMatrix).xyz) < 0.0f) {
		currSample = max(tile, worldTile.tileCoords - e2 - e3);
		//currSample = clamp(worldTile.tileCoords - e2 - e3, tile, tile + end);
		output.wPos = Samples.Load(currSample);
		output.pos = mul(output.wPos, viewProj);
		OutputStream.Append(output);

		currSample = worldTile.tileCoords;
		//currSample = clamp(worldTile.tileCoords, tile, tile + end);
		output.wPos = Samples.Load(currSample);
		output.pos = mul(output.wPos, viewProj);
		OutputStream.Append(output);

		currSample = max(tile, worldTile.tileCoords - e3);
		//currSample = clamp(worldTile.tileCoords - e3, tile, tile + end);
		output.wPos = Samples.Load(currSample);
		output.pos = mul(output.wPos, viewProj);
		OutputStream.Append(output);

		OutputStream.RestartStrip();

		currSample = max(tile, worldTile.tileCoords - e2 - e3);
		//currSample = clamp(worldTile.tileCoords - e2 - e3, tile, tile + end);
		output.wPos = Samples.Load(currSample);
		output.pos = mul(output.wPos, viewProj);
		OutputStream.Append(output);

		currSample = max(tile, worldTile.tileCoords - e2);
		//currSample = clamp(worldTile.tileCoords - e2, tile, tile + end);
		output.wPos = Samples.Load(currSample);
		output.pos = mul(output.wPos, viewProj);
		OutputStream.Append(output);

		currSample = worldTile.tileCoords;
		//currSample = clamp(worldTile.tileCoords, tile, tile + end);
		output.wPos = Samples.Load(currSample);
		output.pos = mul(output.wPos, viewProj);
		OutputStream.Append(output);


		OutputStream.RestartStrip();
	}


	//up-endge
	if (Sphere(mul(input[0].tPos,worldTile.worldMatrix).xyz) < 0.0f && Sphere(mul(input[0].tPos+e2, worldTile.worldMatrix).xyz) > 0.0f) {
		currSample = max(tile, worldTile.tileCoords - e1 - e3);
		//currSample = clamp(worldTile.tileCoords - e1 - e3, tile, tile+end);
		output.wPos = Samples.Load(currSample);
		output.pos = mul(output.wPos, viewProj);
		OutputStream.Append(output);

		currSample = max(tile, worldTile.tileCoords - e1);
		//currSample = clamp(worldTile.tileCoords - e1, tile, tile+end);
		output.wPos = Samples.Load(currSample);
		output.pos = mul(output.wPos, viewProj);
		OutputStream.Append(output);

		currSample = worldTile.tileCoords;
		//currSample = clamp(worldTile.tileCoords, tile, tile + end);
		output.wPos = Samples.Load(currSample);
		output.pos = mul(output.wPos, viewProj);
		OutputStream.Append(output);

		OutputStream.RestartStrip();

		currSample = max(tile, worldTile.tileCoords - e1 - e3);
		//currSample = clamp(worldTile.tileCoords - e1 - e3, tile, tile + end);
		output.wPos = Samples.Load(currSample);
		output.pos = mul(output.wPos, viewProj);
		OutputStream.Append(output);

		currSample = worldTile.tileCoords;
		//currSample = clamp(worldTile.tileCoords, tile, tile + end);
		output.wPos = Samples.Load(currSample);
		output.pos = mul(output.wPos, viewProj);
		OutputStream.Append(output);

		currSample = max(tile, worldTile.tileCoords - e3);
		//currSample = clamp(worldTile.tileCoords - e3, tile, tile + end);
		output.wPos = Samples.Load(currSample);
		output.pos = mul(output.wPos, viewProj);
		OutputStream.Append(output);


		OutputStream.RestartStrip();
	}
	if (Sphere(mul(input[0].tPos, worldTile.worldMatrix).xyz) > 0.0f && Sphere(mul(input[0].tPos + e2, worldTile.worldMatrix).xyz) < 0.0f) {
		currSample = max(tile, worldTile.tileCoords - e1 - e3);
		//currSample = clamp(worldTile.tileCoords - e1 - e3, tile, tile + end);
		output.wPos = Samples.Load(currSample);
		output.pos = mul(output.wPos, viewProj);
		OutputStream.Append(output);
		
		currSample = worldTile.tileCoords;
		//currSample = clamp(worldTile.tileCoords, tile, tile + end);
		output.wPos = Samples.Load(currSample);
		output.pos = mul(output.wPos, viewProj);
		OutputStream.Append(output);

		currSample = max(tile, worldTile.tileCoords - e1);
		//currSample = clamp(worldTile.tileCoords - e1, tile, tile + end);
		output.wPos = Samples.Load(currSample);
		output.pos = mul(output.wPos, viewProj);
		OutputStream.Append(output);

		OutputStream.RestartStrip();

		currSample = max(tile, worldTile.tileCoords - e1 - e3);
		//currSample = clamp(worldTile.tileCoords - e1 - e3, tile, tile + end);
		output.wPos = Samples.Load(currSample);
		output.pos = mul(output.wPos, viewProj);
		OutputStream.Append(output);

		currSample = max(tile, worldTile.tileCoords - e3);
		//currSample = clamp(worldTile.tileCoords - e3, tile, tile + end);
		output.wPos = Samples.Load(currSample);
		output.pos = mul(output.wPos, viewProj);
		OutputStream.Append(output);

		currSample = worldTile.tileCoords;
		//currSample = clamp(worldTile.tileCoords, tile, tile + end);
		output.wPos = Samples.Load(currSample);
		output.pos = mul(output.wPos, viewProj);
		OutputStream.Append(output);


		OutputStream.RestartStrip();
	}
	//forward-edge
	if (Sphere(mul(input[0].tPos, worldTile.worldMatrix).xyz) < 0.0f && Sphere(mul(input[0].tPos + e3, worldTile.worldMatrix).xyz) > 0.0f) {
		currSample = max(tile, worldTile.tileCoords - e1 - e2);
		//currSample = clamp(worldTile.tileCoords - e1 - e2, tile, tile + end);
		output.wPos = Samples.Load(currSample);
		output.pos = mul(output.wPos, viewProj);
		OutputStream.Append(output);

		currSample = worldTile.tileCoords;
		//currSample = clamp(worldTile.tileCoords, tile, tile + end);
		output.wPos = Samples.Load(currSample);
		output.pos = mul(output.wPos, viewProj);
		OutputStream.Append(output);

		currSample = max(tile, worldTile.tileCoords - e1);
		//currSample = clamp(worldTile.tileCoords - e1, tile, tile + end);
		output.wPos = Samples.Load(currSample);
		output.pos = mul(output.wPos, viewProj);
		OutputStream.Append(output);

		OutputStream.RestartStrip();

		currSample = max(tile, worldTile.tileCoords - e1 - e2);
		//currSample = clamp(worldTile.tileCoords - e1 - e2, tile, tile + end);
		output.wPos = Samples.Load(currSample);
		output.pos = mul(output.wPos, viewProj);
		OutputStream.Append(output);

		currSample = max(tile, worldTile.tileCoords - e2);
		//currSample = clamp(worldTile.tileCoords - e2, tile, tile + end);
		output.wPos = Samples.Load(currSample);
		output.pos = mul(output.wPos, viewProj);
		OutputStream.Append(output);

		currSample = worldTile.tileCoords;
		//currSample = clamp(worldTile.tileCoords, tile, tile + end);
		output.wPos = Samples.Load(currSample);
		output.pos = mul(output.wPos, viewProj);
		OutputStream.Append(output);


		OutputStream.RestartStrip();
	}
	if (Sphere(mul(input[0].tPos, worldTile.worldMatrix).xyz) > 0.0f && Sphere(mul(input[0].tPos + e3, worldTile.worldMatrix).xyz) < 0.0f) {
		currSample = max(tile, worldTile.tileCoords - e1 - e2);
		//currSample = clamp(worldTile.tileCoords - e1 - e2, tile, tile + end);
		output.wPos = Samples.Load(currSample);
		output.pos = mul(output.wPos, viewProj);
		OutputStream.Append(output);

		currSample = max(tile, worldTile.tileCoords - e1);
		//currSample = clamp(worldTile.tileCoords - e1, tile, tile + end);
		output.wPos = Samples.Load(currSample);
		output.pos = mul(output.wPos, viewProj);
		OutputStream.Append(output);

		currSample = worldTile.tileCoords;
		//currSample = clamp(worldTile.tileCoords, tile, tile + end);
		output.wPos = Samples.Load(currSample);
		output.pos = mul(output.wPos, viewProj);
		OutputStream.Append(output);

		OutputStream.RestartStrip();

		currSample = max(tile, worldTile.tileCoords - e1 - e2);
		//currSample = clamp(worldTile.tileCoords - e1 - e2, tile, tile + end);
		output.wPos = Samples.Load(currSample);
		output.pos = mul(output.wPos, viewProj);
		OutputStream.Append(output);

		currSample = worldTile.tileCoords;
		//currSample = clamp(worldTile.tileCoords, tile, tile + end);
		output.wPos = Samples.Load(currSample);
		output.pos = mul(output.wPos, viewProj);
		OutputStream.Append(output);

		currSample = max(tile, worldTile.tileCoords - e2);
		//currSample = clamp(worldTile.tileCoords - e2, tile, tile + end);
		output.wPos = Samples.Load(currSample);
		output.pos = mul(output.wPos, viewProj);
		OutputStream.Append(output);


		OutputStream.RestartStrip();
	}
}

float4 PS(GSOutput input) : SV_TARGET {
	return float4(1.0f,0.0f,1.0f,1.0f);
}





/*
if (Sphere(mul(input[0].tPos, worldTile.worldMatrix).xyz) < 0.0f && Sphere(mul(input[0].tPos + e1, worldTile.worldMatrix).xyz) > 0.0f) {
		currSample = clamp(worldTile.tileCoords - e2 - e3, tile, tile + end);
		output.wPos = Samples.Load(currSample);
		output.pos = mul(output.wPos, viewProj);
		OutputStream.Append(output);

		currSample = clamp(worldTile.tileCoords - e3, tile, tile + end);
		output.wPos = Samples.Load(currSample);
		output.pos = mul(output.wPos, viewProj);
		OutputStream.Append(output);

		currSample = clamp(worldTile.tileCoords, tile, tile + end);
		output.wPos = Samples.Load(currSample);
		output.pos = mul(output.wPos, viewProj);
		OutputStream.Append(output);

		OutputStream.RestartStrip();

		currSample = clamp(worldTile.tileCoords - e2 - e3, tile, tile + end);
		output.wPos = Samples.Load(currSample);
		output.pos = mul(output.wPos, viewProj);
		OutputStream.Append(output);

		currSample = clamp(worldTile.tileCoords, tile, tile + end);
		output.wPos = Samples.Load(currSample);
		output.pos = mul(output.wPos, viewProj);
		OutputStream.Append(output);

		currSample = clamp(worldTile.tileCoords - e2, tile, tile + end);
		output.wPos = Samples.Load(currSample);
		output.pos = mul(output.wPos, viewProj);
		OutputStream.Append(output);


		OutputStream.RestartStrip();
	}
*/ 