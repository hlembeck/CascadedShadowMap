#pragma once
#include "DXBase.h"
#include "TerrainHeightmap.h"

struct Tile;

//Sorted children
struct TileCluster {
	Tile* t1;
	Tile* t2;
	Tile* t3;
	Tile* t4;
};

struct Tile {
	BoundingBox bounds; //World-space bounds of the Tile.
	TileParams tileParams;
	XMFLOAT2 position;

	TileCluster children; //If screen-space error of this tile is too high, then all four children are created, since at the moment there is no system to render a quadrant at a higher resolution than the other quadrants. This means that m_children is NULL iff this tile has screen-space error low enough.

	float error;
	INT instance;

	Tile();
	Tile(float x, float y, D3D12_TILED_RESOURCE_COORDINATE texCoords, float width, TerrainLODViewParams viewParams);
	~Tile();
	void Init(float x, float y, D3D12_TILED_RESOURCE_COORDINATE texCoords, float width, TerrainLODViewParams viewParams);
	BOOL IsVisible(BoundingFrustum frustum, XMMATRIX matrix);
	void MakeResident(INT inst, D3D12_TILED_RESOURCE_COORDINATE tileCoords); //INT inst is the index into TerrainTileManager::m_currInstances
	TileCluster CreateChildren(TerrainLODViewParams viewParams, D3D12_TILED_RESOURCE_COORDINATE tile, INT inst);
	XMMATRIX GetCreationMatrix();
};

class TileCmp {
public:
	BOOL operator() (Tile* a, Tile* b) {
		return a->error < b->error ? TRUE : FALSE;
	};

};

class Chunk : public Tile {
public:
	XMMATRIX Create(float x, float y, D3D12_TILED_RESOURCE_COORDINATE texCoords, float width, TerrainLODViewParams viewParams);
};

struct TileInstance {
	UINT heapIndex;
	D3D12_TILED_RESOURCE_COORDINATE tile;
};

class HeapTileManager : public virtual DXBase {
	INT* m_freeTiles; //INT type as to avoid narrowing conversion (which should not matter as a heap incurring an invalid conversion would be terabytes in size)
	UINT m_size;
	UINT m_end;
protected:
	ComPtr<ID3D12Heap> m_heap;
public:
	~HeapTileManager();
	INT Pop();
	//Push only used on tiles popped from this class instance. Thus no need to check size.
	void Push(INT tile);
	void Init(UINT numTiles = 8192); //512MB default
};

class VirtualTextureManager : public virtual DXBase {
	D3D12_TILED_RESOURCE_COORDINATE* m_freeTiles;
	UINT m_end;
protected:
	ComPtr<ID3D12Resource> m_virtualTexture;
public:
	~VirtualTextureManager();
	BOOL Pop(D3D12_TILED_RESOURCE_COORDINATE* tile);
	void Push(D3D12_TILED_RESOURCE_COORDINATE tile);
	virtual void Init();
};

class TerrainTileManager : public virtual DXBase {
	UINT m_numInstances;
	TileInstance* m_currInstances;
	UINT* m_freeInstances;

	D3D12_TILE_REGION_SIZE* m_tileRegionSizes; //never changed, API doesn't allow for copying of multiple region sizes for multiple regions, unless each region size is 1 tile.
	UINT* m_heapRegionSizes; //never changed; same as above but for heap tiles
	UINT m_back;
public:
	TerrainTileManager();
	~TerrainTileManager();
	INT Push(UINT heapIndex, D3D12_TILED_RESOURCE_COORDINATE tile);
	TileInstance Remove(UINT index);
	void Init(UINT n);
	BOOL HasCapacity();

	void UpdateTileMappings(ID3D12Resource* virtualTexture, ID3D12Heap* heap);
};

class LODTerrain :
	public virtual DXBase,
	public virtual HeightmapGen,
	public virtual VirtualTextureManager,
	public virtual HeapTileManager,
	public TerrainTileManager
{
	float m_chunkWidth;

	XMFLOAT2 m_chunkPosition; //Chunk that camera lies in.

	//Needed to map tiles for virtual texturing.
	ComPtr<ID3D12CommandAllocator> m_commandAllocator;
	ComPtr<ID3D12GraphicsCommandList> m_commandList;
	ComPtr<ID3D12Fence> m_fence;
	HANDLE m_fenceEvent;

	ComPtr<ID3D12Resource> m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

	//Testing virtual texture
	//ComPtr<ID3D12Resource> m_virtualTexture;
	D3D12_GPU_DESCRIPTOR_HANDLE m_srvHandle; //SRV for virtual texture, used in RenderTile

	//Quadtree forest on CPU to be traversed each upon camera update (change in position or direction)
	Chunk m_chunks[121];
	ComPtr<ID3D12Heap> m_coarseHeap; //Heap for coarse terrain

	//Constant buffer for the world matrices and UV coordinate starts for tiles.
	ComPtr<ID3D12Resource> m_constantBuffer;
	D3D12_GPU_DESCRIPTOR_HANDLE m_cbvHandle;


	void CreateVertexTexture();
	void CreateVirtualTexture();
	void CreateCommandList();
	void WaitForList();

	//Testing
	void CreateChunks(CameraView view, TerrainLODViewParams viewParams);
	void CreateConstantBuffer();

	std::vector<TileParams> GetTilesToRender(TerrainLODViewParams viewParams);
public:
	LODTerrain();
	~LODTerrain();
	void Init(CameraView view, TerrainLODViewParams viewParams);

	//Tiles
	void RenderChunks(TerrainLODViewParams viewParams, ID3D12GraphicsCommandList* commandList);
	void Render(TerrainLODViewParams viewParams, ID3D12GraphicsCommandList* commandList);

	//void UpdateRoots(TerrainLODViewParams viewParams);
};