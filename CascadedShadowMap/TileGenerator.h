#pragma once
#include "DXBase.h"
#include "PipelineObjects.h"


/*
Generate a grid of sample points, such that the grid fits in a single GPU tile. Sample points describe whether the point is inside the surface, by returning a positive, zero, or negative float.
*/ 
class TileGenerator : public virtual DXBase, public virtual PipelineObjects {
	ComPtr<ID3D12Resource> m_srcTexture; //Render to srcTexture, then copy to tile of virtual texture
	D3D12_CPU_DESCRIPTOR_HANDLE m_srcRTV;
	UINT m_blockWidth;

	D3D12_VIEWPORT m_viewport;
	D3D12_RECT m_scissorRect;

	//Cbuffer for matrix. Upload heap, so accessible by CPU
	ComPtr<ID3D12Resource> m_cbuffer;

	//Vertex buffer for quad
	ComPtr<ID3D12Resource> m_vertices;
	D3D12_VERTEX_BUFFER_VIEW m_vertexView;

	void CreateCommandList();
	void CreateVertexBuffer();
	void CreateConstantBuffer();
protected:
	ComPtr<ID3D12GraphicsCommandList> m_commandList;
	ComPtr<ID3D12CommandAllocator> m_commandAllocator;
	ComPtr<ID3D12Fence> m_fence;
	HANDLE m_fenceEvent;
	void WaitForQueue();
public:
	void Init(UINT tileWidth, UINT maxTilesPerRender = 64);
	//Uses member texture as RTV, then copy to destResource
	void FillTile(XMMATRIX worldMatrix, XMUINT3 texCoords, ID3D12Resource* destResource, D3D12_GPU_DESCRIPTOR_HANDLE randSRVHandle);
	BOOL FillTiles(XMMATRIX* worldMatrices, XMUINT3* tileCoords, UINT n, ID3D12Resource* destResource, D3D12_GPU_DESCRIPTOR_HANDLE randSRVHandle = {});
};