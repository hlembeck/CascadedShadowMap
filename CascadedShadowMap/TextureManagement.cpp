#include "TextureManagement.h"
HeapTileManager::~HeapTileManager() { delete[] m_freeTiles; }

INT HeapTileManager::Pop() {
	if (m_end == 0)
		return -1;
	return m_freeTiles[--m_end];
}

void HeapTileManager::Push(INT tile) {
	m_freeTiles[m_end++] = tile;
}

void HeapTileManager::Init(UINT numTiles) {
	D3D12_HEAP_DESC heapDesc = {
		numTiles * 65536,
		CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		0,
		D3D12_HEAP_FLAG_DENY_BUFFERS | D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES
	};
	ThrowIfFailed(m_device->CreateHeap(&heapDesc, IID_PPV_ARGS(&m_heap)));

	m_size = numTiles / 4;
	m_freeTiles = new INT[m_size];
	for (UINT i = 0; i < m_size; i++) {
		m_freeTiles[i] = 4 * i;
	}
	m_end = m_size;
}

VirtualTextureManager::~VirtualTextureManager() { delete[] m_freeTiles; }

BOOL VirtualTextureManager::Pop(D3D12_TILED_RESOURCE_COORDINATE* tile) {
	if (m_end == 0)
		return FALSE;
	*tile = m_freeTiles[--m_end];
	return TRUE;
}

void VirtualTextureManager::Push(D3D12_TILED_RESOURCE_COORDINATE tile) {
	m_freeTiles[m_end++] = tile;
}

void VirtualTextureManager::Init() {
	D3D12_RESOURCE_DESC desc = {
		D3D12_RESOURCE_DIMENSION_TEXTURE3D,
		0,
		2048,
		2048,
		2048,
		1,
		DXGI_FORMAT_R16_FLOAT,
		{1,0},
		D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE,
	};

	ThrowIfFailed(m_device->CreateReservedResource(&desc, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, NULL, IID_PPV_ARGS(&m_virtualTexture)));
	m_freeTiles = new D3D12_TILED_RESOURCE_COORDINATE[16353]; // Tile is 64x64, so there are 2^8=256 per dimension, giving 256*64=16384 groups of four tiles. 31 of these are used for coarse chunks, leaving 16353 tiles remaining.

	for (UINT i = 31; i < 64; i++) {
		m_freeTiles[i - 31] = { 4 * i,0 };
	}

	for (UINT i = 1; i < 256; i++) {
		for (UINT j = 0; j < 64; j++) {
			m_freeTiles[33 + (i - 1) * 64 + j] = { 4 * j,i };
		}
	}
	m_end = 16353;
}

TerrainTileManager::TerrainTileManager() {}
TerrainTileManager::~TerrainTileManager() {
	delete[] m_currInstances;
	delete[] m_freeInstances;
	delete[] m_tileRegionSizes;
	delete[] m_heapRegionSizes;
}

INT TerrainTileManager::Push(UINT heapIndex, D3D12_TILED_RESOURCE_COORDINATE tile) {
	//printf("%d %d\n", tile.X,tile.Y);
	if (m_back == 0)
		return -1;
	UINT index = m_freeInstances[--m_back];
	m_currInstances[index] = { heapIndex,tile };
	return index;
}

TileInstance TerrainTileManager::Remove(UINT index) {
	m_freeInstances[m_back++] = index;
	TileInstance ret = m_currInstances[index];
	m_currInstances[index].heapIndex = -1;
	return ret;
}

void TerrainTileManager::Init(UINT n) {
	m_numInstances = n;
	m_currInstances = new TileInstance[n];
	m_freeInstances = new UINT[n];

	//Constants
	m_tileRegionSizes = new D3D12_TILE_REGION_SIZE[n];
	m_heapRegionSizes = new UINT[n];
	D3D12_TILE_REGION_SIZE constSize = {
		4,
		FALSE
	};
	for (UINT i = 0; i < n; i++) {
		m_tileRegionSizes[i] = constSize;
		m_heapRegionSizes[i] = 4;
		m_freeInstances[i] = i;
		m_currInstances[i].heapIndex = -1;
	}
	m_back = n;
}

BOOL TerrainTileManager::HasCapacity() {
	return m_back;
}

void TerrainTileManager::UpdateTileMappings(ID3D12Resource* virtualTexture, ID3D12Heap* heap) {
	/*for (UINT i = 0; i < m_back; i++) {
		printf("Tile: (%d, %d, %d, %d)\n", m_currResourceTiles[i].X, m_currResourceTiles[i].Y, m_currResourceTiles[i].Z, m_currResourceTiles[i].Subresource);
	}*/

	UINT size = m_numInstances - m_back;
	if (size == 0)
		return;

	D3D12_TILED_RESOURCE_COORDINATE* pResourceRegionStartCoords = new D3D12_TILED_RESOURCE_COORDINATE[size];
	UINT* pHeapRangeStartOffsets = new UINT[size];

	UINT j = 0;
	TileInstance curr;
	for (UINT i = 0; i < m_numInstances; i++) {
		curr = m_currInstances[i];
		if (curr.heapIndex != -1) {
			pResourceRegionStartCoords[j] = curr.tile;
			pHeapRangeStartOffsets[j++] = curr.heapIndex;
		}
	}

	m_commandQueue->UpdateTileMappings(
		virtualTexture,
		size,
		pResourceRegionStartCoords,
		m_tileRegionSizes,
		heap,
		size,
		nullptr,
		pHeapRangeStartOffsets,
		m_heapRegionSizes,
		D3D12_TILE_MAPPING_FLAG_NONE
	);

	delete[] pResourceRegionStartCoords;
	delete[] pHeapRangeStartOffsets;
}
