#pragma once
#include "TextureManagement.h"
#include "PipelineObjects.h"
#include "TileGenerator.h"


class DualContour :
	public virtual VirtualTextureManager,
	public virtual HeapTileManager,
	public TerrainTileManager,
	public virtual PipelineObjects,
	public virtual TileGenerator
{

	//Vertex Buffer
	ComPtr<ID3D12Resource> m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

	ComPtr<ID3D12Resource> m_blockmapTexture; //Texture of one texel per smallest size block (octree node) to be rendered. Texel maps into virtual texture.
	D3D12_GPU_DESCRIPTOR_HANDLE m_blockmapSRV;

	void CreateVertexBuffer(UINT resolution);
	void WaitForList();
public:
	virtual void Init();

	void Render(ID3D12GraphicsCommandList* commandList);
};