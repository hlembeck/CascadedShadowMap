#pragma once
#include "CelestialBody.h"

//Planets have assumed hydrostatic equilibrium by gravity, therefore they are unlikely to have very large overhangs and such as created by a 3D noise and dual-contours. Instead, a standard heightmap is put on the planet.


/*
A tile can be completely described by the following:
	- UINT side index, representing which side of the cube the tile lies on
	- UINT negative exponent, k, such that tile has side lengths 1/2^k
	- UINT2 offset (i,j) from (-1,-1) that begins the tile.

Tile matrix maps [-1,1]x[-1,1] to the portion of the standard square, [-1,1]x[-1,1] in worldspace that the tile covers. Thus it maps 0 -> ( -1 + (i+0.5)/2^k , -1 + (j+0.5)/2^k ).
*/ 
struct PlanetTile {
	BoundingBox aabb;
	float weight;
	UINT heapTile;
	UINT side;
	XMMATRIX GetTileMatrix(); //Transform [-1,1]x[-1,1] -> portion of face of cube that the tile represents

	PlanetTile(UINT sideIndex, UINT negExponent, XMUINT2 offset);
	~PlanetTile();
};

struct PlanetParams {
	XMMATRIX worldMatrix;
	XMMATRIX orientation;
};

/*
For heightmap noise, it is tempting to attempt generation of the noise reatime with rendering (in the same shader that renders the planet). However, the noise functions may be too computationally intensive to do so for as many tiles as may be necessary for rendering a given frame.
*/
class Planet : public CelestialBody {
	std::vector<PlanetTile*> m_tiles;

	float m_radius;

	XMFLOAT3 m_position;

	PlanetParams m_planetParams;
	PlanetIntersectionParams m_intersectionParams;
	ComPtr<ID3D12Resource> m_planetParamsBuffer;
	D3D12_GPU_DESCRIPTOR_HANDLE m_planetParamsHandle;

	ComPtr<ID3D12Resource> m_tileMap; //Should be a virtual texture, but is committed resource here for testing simplicity
	D3D12_GPU_DESCRIPTOR_HANDLE m_tileMapSRV;
	D3D12_GPU_DESCRIPTOR_HANDLE m_tileMapUAV;


	void CreateVertexBuffer(CommandListAllocatorPair* pCmdListAllocPair); //Should be in CelestialManager, since all planets should share the same vertex buffer, just different draw calls.
	void CreateRandTexture(CommandListAllocatorPair* pCmdListAllocPair);

	void CreatePlanetParams(ID3D12Device* device);
	void UpdatePlanetParams(XMVECTOR camPos);

	void CreateTileMap(ID3D12Device* device);
public:
	Planet(CommandListAllocatorPair* pCmdListAllocPair, float radius, XMFLOAT3 initialPos, BoundingFrustum viewFrustum);
	~Planet();

	void FillRenderCmdList(XMVECTOR camPos, ID3D12GraphicsCommandList* commandList); //Assumes IA toplogy is triangelist, and that pipeline state is set to m_planetRenderPSO (see PipelineObjects object)
};