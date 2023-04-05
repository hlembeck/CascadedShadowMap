#pragma once
#include "DXBase.h"
#include "PipelineObjects.h"

struct ChunkGenParams {
	XMMATRIX worldMatrix;
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle;
	ID3D12Resource* renderTarget;
};

class HeightmapGen :
	public virtual DXBase,
	public virtual PipelineObjects
{
	const UINT m_randomResolution;
	ComPtr<ID3D12GraphicsCommandList> m_commandList;
	ComPtr<ID3D12CommandAllocator> m_commandAllocator;
	ComPtr<ID3D12Fence> m_fence;
	HANDLE m_fenceEvent;

	ComPtr<ID3D12Resource> m_randomTexture;
	D3D12_GPU_DESCRIPTOR_HANDLE m_randomTextureSRV;
	ComPtr<ID3D12Resource> m_randomUploadBuffer;

	ComPtr<ID3D12Resource> m_coarseTerrainQuad; //Quad for rendering the coarse terrain to. Format is FLOAT2, and the coordinates are the (x,y) coordinates of the vertex.
	D3D12_VERTEX_BUFFER_VIEW m_quadVertexView;
	ComPtr<ID3D12Resource> m_coarseTerrainTexture; //Render target used in GenerateBlock. Used to generate a coarse terrain block, which is then upsampled to the highest detail desired.
	D3D12_CPU_DESCRIPTOR_HANDLE m_coarseTerrainRTV;
	D3D12_VIEWPORT m_coarseTerrainViewport;
	D3D12_RECT m_coarseTerrainScissorRect;

	//TESTING
	D3D12_VIEWPORT m_chunkViewport;
	D3D12_RECT m_chunkScissorRect;
	ComPtr<ID3D12Resource> m_cbuffer;
protected:
	//Since it is fast to generate the ground level data for an entire chunk at a time (entire 4096x4096 block fits in single texture, for example), we generate by blocks. The geometric error calculations for each level take more time.
	void GenerateBlock(XMMATRIX worldMatrix, ID3D12Resource* destResource, ID3D12Heap* heap);
	void CreateResources(UINT16 nRoots);
	void CreateConstantBuffer(UINT nRoots);
	void CreateCommandList();
	//Refresh the random texture used for generating noise (m_randomTexture)
	void RefreshTexture();
	void CreateCoarseTerrainRTV(UINT16 nRoots);
	void FillCoarseVertexBuffer();

	//TESTING
	void GenerateChunk(ChunkGenParams params);
	D3D12_GPU_DESCRIPTOR_HANDLE GetTESTHandle() { return m_randomTextureSRV; };

	//Tiles
	void GenerateInitialTiles(XMMATRIX* worldMatrices, UINT nMatrices, ID3D12Resource* destResource, ID3D12Heap* heap);

	//To generate 4032x4032 chunk, which then has mip levels generated.
	void Generate4KChunk(XMMATRIX worldMatrix, ID3D12Resource* destResource, ID3D12Heap* heap);
public:
	HeightmapGen(const UINT randomResolution = 16) : m_randomResolution(randomResolution) {};
	void Init(UINT nRoots);
	void WaitForQueue();
};