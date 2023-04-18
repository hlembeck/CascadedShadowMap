#include "TerrainHeightmap.h"
#include "DescriptorHeaps.h"

void HeightmapGen::GenerateInitialTiles(XMMATRIX* worldMatrices, D3D12_TILED_RESOURCE_COORDINATE* texCoords, UINT nMatrices, ID3D12Resource* destResource, float chunkSpacing) {
	D3D12_TILED_RESOURCE_COORDINATE tiledCoordinate = {0,0,0};

	D3D12_TILE_RANGE_FLAGS flag = D3D12_TILE_RANGE_FLAG_NONE;

	UINT heapRangeStartOffset = 0;

	m_commandQueue->UpdateTileMappings(
		destResource,
		nMatrices,
		texCoords,
		NULL,
		m_heap.Get(),
		1,
		&flag,
		&heapRangeStartOffset,
		NULL,
		D3D12_TILE_MAPPING_FLAG_NONE
	);
	WaitForQueue();

	BYTE* pData;
	D3D12_RANGE readRange = {};
	m_cbuffer->Map(0, &readRange, (void**)&pData);
	memcpy(pData, worldMatrices, sizeof(XMMATRIX)*nMatrices);
	m_cbuffer->Unmap(0, nullptr);

	ThrowIfFailed(m_commandAllocator->Reset());
	ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), PipelineObjects::m_blockTerrainGen.Get()));

	CD3DX12_RESOURCE_BARRIER resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_coarseTerrainTexture.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_commandList->ResourceBarrier(1, &resourceBarrier);

	m_commandList->SetGraphicsRootSignature(RootSignatures::m_gRootSignature.Get());
	m_commandList->RSSetViewports(1, &m_coarseTerrainViewport);
	m_commandList->RSSetScissorRects(1, &m_coarseTerrainScissorRect);
	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	ID3D12DescriptorHeap* pHeaps[] = { DescriptorHeaps::GetCBVHeap() };
	m_commandList->SetDescriptorHeaps(1, pHeaps);
	m_commandList->SetGraphicsRootDescriptorTable(3, m_randomTextureSRV);
	m_commandList->SetGraphicsRootConstantBufferView(0, m_cbuffer->GetGPUVirtualAddress());
	m_commandList->SetGraphicsRoot32BitConstants(4, 1, &chunkSpacing, 0);

	const float clearColor[] = { 0.0f,0.0f,0.0f,0.0f };
	m_commandList->ClearRenderTargetView(m_coarseTerrainRTV, clearColor, 0, nullptr);
	m_commandList->OMSetRenderTargets(1, &m_coarseTerrainRTV, FALSE, NULL);
	//Draw
	m_commandList->IASetVertexBuffers(0, 1, &m_quadVertexView);
	m_commandList->DrawInstanced(6, nMatrices, 0, 0);

	resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_coarseTerrainTexture.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
	m_commandList->ResourceBarrier(1, &resourceBarrier);

	resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(destResource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
	m_commandList->ResourceBarrier(1, &resourceBarrier);


	D3D12_TEXTURE_COPY_LOCATION srcLocation;
	D3D12_TEXTURE_COPY_LOCATION destLocation;
	//Copy tiles
	{
		srcLocation = {
			m_coarseTerrainTexture.Get(),
			D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX
		};
		destLocation = {
			destResource,
			D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX
		};
		destLocation.SubresourceIndex = 0;


		for (UINT i = 0; i < nMatrices; i++) {
			srcLocation.SubresourceIndex = i;
			m_commandList->CopyTextureRegion(&destLocation, 64*texCoords[i].X,64*texCoords[i].Y, 0, &srcLocation, NULL);
		}
	}


	resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(destResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	m_commandList->ResourceBarrier(1, &resourceBarrier);

	m_commandList->Close();
	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	WaitForQueue();
}

void HeightmapGen::UpdateChunks(XMMATRIX* worldMatrices, XMUINT2* texCoords, UINT nMatrices, ID3D12Resource* destResource, float chunkSpacing) {
	BYTE* pData;
	D3D12_RANGE readRange = {};
	m_cbuffer->Map(0, &readRange, (void**)&pData);
	memcpy(pData, worldMatrices, sizeof(XMMATRIX) * nMatrices);
	m_cbuffer->Unmap(0, nullptr);

	ThrowIfFailed(m_commandAllocator->Reset());
	ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), PipelineObjects::m_blockTerrainGen.Get()));

	CD3DX12_RESOURCE_BARRIER resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_coarseTerrainTexture.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_commandList->ResourceBarrier(1, &resourceBarrier);

	m_commandList->SetGraphicsRootSignature(RootSignatures::m_gRootSignature.Get());
	m_commandList->RSSetViewports(1, &m_coarseTerrainViewport);
	m_commandList->RSSetScissorRects(1, &m_coarseTerrainScissorRect);
	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	ID3D12DescriptorHeap* pHeaps[] = { DescriptorHeaps::GetCBVHeap() };
	m_commandList->SetDescriptorHeaps(1, pHeaps);
	m_commandList->SetGraphicsRootDescriptorTable(3, m_randomTextureSRV);
	m_commandList->SetGraphicsRootConstantBufferView(0, m_cbuffer->GetGPUVirtualAddress());
	m_commandList->SetGraphicsRoot32BitConstants(4, 1, &chunkSpacing, 0);

	const float clearColor[] = { 0.0f,0.0f,0.0f,0.0f };
	m_commandList->ClearRenderTargetView(m_coarseTerrainRTV, clearColor, 0, nullptr);
	m_commandList->OMSetRenderTargets(1, &m_coarseTerrainRTV, FALSE, NULL);
	//Draw
	m_commandList->IASetVertexBuffers(0, 1, &m_quadVertexView);
	m_commandList->DrawInstanced(6, nMatrices, 0, 0);

	resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_coarseTerrainTexture.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
	m_commandList->ResourceBarrier(1, &resourceBarrier);

	resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(destResource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
	m_commandList->ResourceBarrier(1, &resourceBarrier);


	D3D12_TEXTURE_COPY_LOCATION srcLocation;
	D3D12_TEXTURE_COPY_LOCATION destLocation;
	//Copy tiles
	{
		srcLocation = {
			m_coarseTerrainTexture.Get(),
			D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX
		};
		destLocation = {
			destResource,
			D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX
		};
		destLocation.SubresourceIndex = 0;

		for (UINT i = 0; i < nMatrices; i++) {
			srcLocation.SubresourceIndex = i;
			m_commandList->CopyTextureRegion(&destLocation, texCoords[i].x, texCoords[i].y, 0, &srcLocation, NULL);
		}
	}


	resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(destResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	m_commandList->ResourceBarrier(1, &resourceBarrier);

	m_commandList->Close();
	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	WaitForQueue();
}

void HeightmapGen::CreateResources(UINT16 nRoots) {
	{ //Heap for virtual texture
		D3D12_HEAP_DESC heapDesc = {
		1073741824, //1 Gigabyte
		CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		0,
		D3D12_HEAP_FLAG_DENY_BUFFERS | D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES
		};

		ThrowIfFailed(m_device->CreateHeap(&heapDesc, IID_PPV_ARGS(&m_heap)));
	}

	D3D12_RESOURCE_DESC desc;
	//Random texture
	{
		desc = {
			D3D12_RESOURCE_DIMENSION_TEXTURE2D,
			0,
			m_randomResolution,
			m_randomResolution,
			1,
			1,
			DXGI_FORMAT_R32G32_FLOAT,
			{1,0},
			D3D12_TEXTURE_LAYOUT_UNKNOWN
		};

		CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
		ThrowIfFailed(m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COPY_DEST, NULL, IID_PPV_ARGS(&m_randomTexture)));

		HandlePair handles = DescriptorHeaps::BatchHandles(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_randomTextureSRV = handles.gpuHandle;
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {
			DXGI_FORMAT_R32G32_FLOAT,
			D3D12_SRV_DIMENSION_TEXTURE2D,
			D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING
		};
		srvDesc.Texture2D = {
			0,
			1,
			0,
			0.0f
		};
		m_device->CreateShaderResourceView(m_randomTexture.Get(), &srvDesc, handles.cpuHandle);

	}
	//Upload buffer for random texture
	{
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT placedFootprint;
		UINT numRows;
		UINT64 rowSize;
		UINT64 totalSize;
		m_device->GetCopyableFootprints(
			&desc,
			0,
			1,
			0,
			&placedFootprint,
			&numRows,
			&rowSize,
			&totalSize
		);

		desc = {
				D3D12_RESOURCE_DIMENSION_BUFFER,
				0,
				Pad256(rowSize) * m_randomResolution,
				1,
				1,
				1,
				DXGI_FORMAT_UNKNOWN,
				{1,0},
				D3D12_TEXTURE_LAYOUT_ROW_MAJOR
		};

		CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
		ThrowIfFailed(m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&m_randomUploadBuffer)));
	}
	//Render target for coarse terrain block
	{
		desc = {
			D3D12_RESOURCE_DIMENSION_TEXTURE2D,
			0,
			63,
			63,
			nRoots,
			1,
			DXGI_FORMAT_R32G32B32A32_FLOAT,
			{1,0},
			D3D12_TEXTURE_LAYOUT_UNKNOWN,
			D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
		};
		D3D12_CLEAR_VALUE clearVal = { DXGI_FORMAT_R32G32B32A32_FLOAT };
		CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
		ThrowIfFailed(m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COPY_SOURCE, &clearVal, IID_PPV_ARGS(&m_coarseTerrainTexture)));
		m_coarseTerrainScissorRect = { 0,0,63,63 };
		m_coarseTerrainViewport = {0.0f,0.0f,63.0f,63.0f,0.0f,1.0f};
	}
	//Quad for rendering the coarse terrain
	{
		desc = {
			D3D12_RESOURCE_DIMENSION_BUFFER,
			0,
			48,
			1,
			1,
			1,
			DXGI_FORMAT_UNKNOWN,
			{1,0},
			D3D12_TEXTURE_LAYOUT_ROW_MAJOR
		};

		CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
		ThrowIfFailed(m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&m_coarseTerrainQuad)));

		m_quadVertexView = {
			m_coarseTerrainQuad->GetGPUVirtualAddress(),
			48,
			8
		};
	}
}

void HeightmapGen::CreateConstantBuffer(UINT nRoots) {
	nRoots = Pad256(nRoots * 64);
	D3D12_RESOURCE_DESC desc = {
		D3D12_RESOURCE_DIMENSION_BUFFER,
		0,
		nRoots,
		1,
		1,
		1,
		DXGI_FORMAT_UNKNOWN,
		{1,0},
		D3D12_TEXTURE_LAYOUT_ROW_MAJOR
	};

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
	ThrowIfFailed(m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&m_cbuffer)));
}

void HeightmapGen::CreateCommandList() {
	ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));
	ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), NULL, IID_PPV_ARGS(&m_commandList)));
	ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
	ThrowIfFailed(m_commandList->Close());
	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (m_fenceEvent == nullptr) {
		ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
	}
	WaitForQueue();
}

void HeightmapGen::RefreshTexture() {

	D3D12_RESOURCE_DESC resourceDesc = m_randomTexture->GetDesc();
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT placedFootprint;
	UINT numRows;
	UINT64 rowSize;
	UINT64 totalSize;
	m_device->GetCopyableFootprints(
		&resourceDesc,
		0,
		1,
		0,
		&placedFootprint,
		&numRows,
		&rowSize,
		&totalSize
	);

	UINT paddedRes = Pad256(rowSize);

	XMFLOAT2* randomVals = new XMFLOAT2[m_randomResolution];
	D3D12_RANGE readRange = {};
	BYTE* pData;
	XMFLOAT2 temp;
	float m;
	m_randomUploadBuffer->Map(0, &readRange, (void**)&pData);
	for (UINT i = 0; i < m_randomResolution; i++) {
		for (UINT j = 0; j < m_randomResolution; j++) {
			temp = { RandomGenerator::Get(),RandomGenerator::Get() };
			m = sqrt(temp.x * temp.x + temp.y * temp.y);
			randomVals[j] = { temp.x / m, temp.y/m };
		}
		memcpy(pData+i*paddedRes, randomVals, 8 * m_randomResolution);
	}
	m_randomUploadBuffer->Unmap(0, nullptr);
	delete[] randomVals;

	{ //Copy to the resource
		ThrowIfFailed(m_commandAllocator->Reset());
		ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), NULL));

		{
			CD3DX12_RESOURCE_BARRIER resourceBarrier;

			D3D12_TEXTURE_COPY_LOCATION destLocation = {
				m_randomTexture.Get(),
				D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX
			};
			destLocation.SubresourceIndex = 0;

			D3D12_TEXTURE_COPY_LOCATION srcLocation = {
				m_randomUploadBuffer.Get(),
				D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT
			};
			srcLocation.PlacedFootprint = placedFootprint;

			m_commandList->CopyTextureRegion(&destLocation, 0, 0, 0, &srcLocation, NULL);

			resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_randomTexture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, 0);
			m_commandList->ResourceBarrier(1, &resourceBarrier);
		}

		m_commandList->Close();
		ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
		m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
		WaitForQueue();
	}
}

void HeightmapGen::CreateCoarseTerrainRTV(UINT16 nRoots) {
	D3D12_RENDER_TARGET_VIEW_DESC desc = {
		DXGI_FORMAT_R32G32B32A32_FLOAT,
		D3D12_RTV_DIMENSION_TEXTURE2DARRAY
	};
	desc.Texture2DArray = {
		0,
		0,
		nRoots,
		0
	};

	m_coarseTerrainRTV = DescriptorHeaps::BatchHandles(D3D12_DESCRIPTOR_HEAP_TYPE_RTV).cpuHandle;
	m_device->CreateRenderTargetView(m_coarseTerrainTexture.Get(), &desc, m_coarseTerrainRTV);
}

void HeightmapGen::FillCoarseVertexBuffer() {
	/*ComPtr<ID3D12Resource> uploadBuffer;
	D3D12_RESOURCE_DESC desc = {
			D3D12_RESOURCE_DIMENSION_BUFFER,
			0,
			48,
			1,
			1,
			1,
			DXGI_FORMAT_UNKNOWN,
			{1,0},
			D3D12_TEXTURE_LAYOUT_ROW_MAJOR
	};

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
	m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&uploadBuffer));*/

	float vertices[12] = {
		-1.0f,-1.0f,
		-1.0f,1.0f,
		1.0f,1.0f,

		-1.0f,-1.0f,
		1.0f,1.0f,
		1.0f,-1.0f
	};

	D3D12_RANGE readRange = {};
	BYTE* pData;
	m_coarseTerrainQuad->Map(0, &readRange, (void**)&pData);
	memcpy(pData, vertices, 48);
	m_coarseTerrainQuad->Unmap(0, nullptr);


	/*ThrowIfFailed(m_commandAllocator->Reset());
	ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), NULL));

	m_commandList->CopyResource(m_coarseTerrainQuad.Get(), uploadBuffer.Get());

	m_commandList->Close();
	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	WaitForQueue();*/

}

void HeightmapGen::WaitForQueue() {
	ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), 1));
	if (m_fence->GetCompletedValue() < 1) {
		ThrowIfFailed(m_fence->SetEventOnCompletion(1, m_fenceEvent));
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}
	ThrowIfFailed(m_commandAllocator->Reset());
	m_fence->Signal(0);
}

void HeightmapGen::Init(UINT nRoots) {
	CreateCommandList();
	//TerrainMipmap::Init(m_commandAllocator.Get(), m_commandList.Get());
	WaitForQueue();
	CreateResources(nRoots);
	CreateCoarseTerrainRTV(nRoots);
	FillCoarseVertexBuffer();
	RefreshTexture();

	m_chunkScissorRect = { 0,0,127,127 };
	m_chunkViewport = { 0.0f,0.0f,127.0f,127.0f,0.0f,1.0f };
}