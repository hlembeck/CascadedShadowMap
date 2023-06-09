#pragma once
#include "DXBase.h"
#include "TerrainHeightmap.h"

class Tile {
	BoundingBox m_bounds; //World-space bounds of the Tile.
	TileParams m_tileParams;
	XMFLOAT2 m_position;

	Tile* m_children; //If screen-space error of this tile is too high, then all four children are created, since at the moment there is no system to render a quadrant at a higher resolution than the other quadrants. This means that m_children is NULL iff this tile has screen-space error low enough.

	float m_error;
public:
	~Tile();
	XMMATRIX Create(float x, float y, D3D12_TILED_RESOURCE_COORDINATE texCoords, float width, TerrainLODViewParams viewParams);
	XMMATRIX Update(float x, float y, float width); //x,y are change in position, not position itself.
	BOOL IsVisible(BoundingFrustum frustum, XMMATRIX viewMatrix);
	TileParams GetTileParams();
	XMFLOAT2 GetPosition();
	XMUINT2 GetTexCoords();
	void SetError(TerrainLODViewParams);
	float GetError();
};

/*
Tiles of high resolution (2^NTERRAINLEVELS*(62,62)) are generated by the HeightmapGen class in a seperate thread; unless NTERRAINLEVELS is large, this can be generated in one pass by HeightmapGen. A tile at level i (i=0,...,NTERRAINLEVELS-1) loads this texture at a step of 2^(NTERRAINLEVELS-i); thus tiles of consecutive levels have steps that differ by a factor of 2. These tiles form a quadtree which can be traversed to determine which tiles to render. The geometrical error from the generated terrain is computed for each level in the ImprovedTile class
*/
class Terrain :
	public virtual DXBase,
	public virtual HeightmapGen
{
	float m_TileWidth;

	XMFLOAT2 m_TilePosition; //Tile that camera lies in.

	//Needed to map tiles for virtual texturing.
	ComPtr<ID3D12CommandAllocator> m_commandAllocator;
	ComPtr<ID3D12GraphicsCommandList> m_commandList;
	ComPtr<ID3D12Fence> m_fence;
	HANDLE m_fenceEvent;

	ComPtr<ID3D12Resource> m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

	//Testing virtual texture
	ComPtr<ID3D12Resource> m_virtualTexture;
	D3D12_GPU_DESCRIPTOR_HANDLE m_srvHandle; //SRV for virtual texture, used in RenderTile

	//Quadtree forest on CPU to be traversed each upon camera update (change in position or direction)
	Tile m_Tiles[121];

	D3D12_TILED_RESOURCE_COORDINATE* m_texTiles;
	UINT m_begin;
	UINT m_end;

	//Constant buffer for the world matrices and UV coordinate starts for tiles.
	ComPtr<ID3D12Resource> m_constantBuffer;
	D3D12_GPU_DESCRIPTOR_HANDLE m_cbvHandle;


	void CreateVertexTexture();
	void CreateVirtualTexture();
	void CreateCommandList();
	void WaitForList();

	//Testing
	std::pair<XMMATRIX*, D3D12_TILED_RESOURCE_COORDINATE*> CreateRoots(CameraView view, TerrainLODViewParams viewParams);
	void CreateConstantBuffer();

	BOOL BatchTexTile(D3D12_TILED_RESOURCE_COORDINATE* pTexCoords);
	void PushTexTile(D3D12_TILED_RESOURCE_COORDINATE texCoords);
public:
	Terrain();
	~Terrain();
	void Init(CameraView view, TerrainLODViewParams viewParams);

	//Tiles
	void RenderTiles(TerrainLODViewParams viewParams, ID3D12GraphicsCommandList* commandList);

	void UpdateRoots(TerrainLODViewParams viewParams);
};