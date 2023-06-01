#pragma once
#include "DXBase.h"

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