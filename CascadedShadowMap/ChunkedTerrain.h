#pragma once
#include "DXBase.h"
#include "TerrainHeightmap.h"

struct BlockLookup {
	XMINT2 worldCoords;
	UINT lookupTable[1024]; //Pointer to beginning of lookup table of size 32*32=1024. A pair (i,j) maps to i+j*32, and is input to the lookup table, where the offset into the terrain file is returned.
};

/*
Lookup file contains lookup tables for blocks of chunks. Finding a specific lookup table in this file requires a search.

Format of lookup file entries (no padding between entries, since the size of each entry is fixed):

[INT2] worldCoords
[UINT[1024]] lookupTable.

Size = 2*4 + 1024*4 = 2056

*/

struct TreeRoot;

//TerrainManager manages writing and reading the terrain from file.
class TerrainManager : public virtual HeightmapGen {
	HANDLE m_hHeightmapFile;
	HANDLE m_hLookupFile;
	BlockLookup* m_currBlocks; //9 blocks loaded, where center is where player is located.

	BOOL SearchLookupFile(XMINT2 pos, UINT64& offset);
protected:
	void ReplaceBlock(XMINT2 oldCoords, XMINT2 newCoords);
	void CalcBlocks(CameraView view);
public:
	const BOOL IsOpen();
	TerrainManager(const TCHAR heightmapFilename[], const TCHAR lookupFilename[]);
	TerrainManager() {};
};


struct TreeNode;

struct TreeNode {
	//Child nodes
	TreeNode* topLeft;
	TreeNode* topRight;
	TreeNode* bottomLeft;
	TreeNode* botomRight;

	//Spacial bounds and geometrical error
	BoundingBox bounds;
	float geomError;

	//Resource info
	BOOL isResident;
	
	XMMATRIX m_worldMatrix; //Transform from [-1,1] to correct coordinates
};

class Chunk {
	TreeNode m_root;
	ComPtr<ID3D12Resource> m_heightmapTreeLevels[NTERRAINLEVELS]; //One resource per level;
	D3D12_GPU_DESCRIPTOR_HANDLE m_srvHandles[NTERRAINLEVELS];
	D3D12_CPU_DESCRIPTOR_HANDLE m_rtvHandles[NTERRAINLEVELS]; //For testing : used by HeightmapGen to render coarse terrain onto the resource
public:
	BOOL IsVisible(FrustumRays rays);
	BOOL IsVisible(BoundingFrustum frustum, XMMATRIX viewMatrix);
	ChunkGenParams Create(float x, float y, ID3D12Device* device);
	D3D12_GPU_DESCRIPTOR_HANDLE GetSRVHandle();
	XMMATRIX* GetWorldMatrix();
};

/*
Basic Idea: Use instancing to reuse a single vertex buffer, whose size ideally is consistent with the "chunk" resolution (=cunkres-1). Each chunk of heightmap data must be fully resident on GPU whenever used.

Suppose we use a single DX12 resource to store all heightmap data. We'll create it as reserved/tiled, and of the maximum resolution possible (16k x 16k). With heightmap format of R32_FLOAT, a tile in the resource is 128x128; thus we use 127x127 as our chunk resolution, giving 126x126 squares in the vertex buffer (chunk should be cleanly subdivided into quarters for the quadtree).

If we attach texture coordinates to our vertices, then they will be such that the entire vertex buffer covers exactly the UV coordinate range. When rendering, instance ID is used to transform these coordinates so that they access the correct tile of the reserved texture.
*/

constexpr float TERRAINHEIGHTMAX = 40.0f;

class ChunkedTerrain : public virtual TerrainManager {
	ComPtr<ID3D12Resource> m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	ComPtr<ID3D12Resource> m_heightmap;
	
	Chunk* m_chunks;
	UINT m_nChunks;
	ComPtr<ID3D12Resource> m_worldCoordsCBuffer; //CBuffer for storing the world matrices for the roots
	D3D12_GPU_DESCRIPTOR_HANDLE m_cbvHandle; //Handle for above cbuffer

	ComPtr<ID3D12Heap> m_heap;

	void LoadVertexBuffer();
	void CreateChunks(CameraView view);
	void CreateConstantBuffer();
	void MapWorldMatrices();
public:
	ChunkedTerrain();
	~ChunkedTerrain();
	void Init(CameraView view);
	void RenderTerrain(BoundingFrustum frustum, XMMATRIX viewMatrix, ID3D12GraphicsCommandList* commandList);
	void DrawBarebones(ID3D12GraphicsCommandList* commandList);
};