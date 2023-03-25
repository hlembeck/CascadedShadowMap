#include "TerrainHeightmap.h"
#include "DescriptorHeaps.h"

void HeightmapGen::GenerateBlock(XMINT2 pos, ComPtr<ID3D12Resource> destResource) {
	//Draw to m_coarseTerrainTexture
	{
		ThrowIfFailed(m_commandAllocator->Reset());
		ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), PipelineObjects::m_blockTerrainGen.Get()));

		m_commandList->SetGraphicsRootSignature(RootSignatures::m_gRootSignature.Get());
		m_commandList->RSSetViewports(1, &m_coarseTerrainViewport);
		m_commandList->RSSetScissorRects(1, &m_coarseTerrainScissorRect);
		m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		CD3DX12_RESOURCE_BARRIER resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_coarseTerrainTexture.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
		m_commandList->ResourceBarrier(1, &resourceBarrier);

		m_commandList->OMSetRenderTargets(0, &m_coarseTerrainRTV, FALSE, NULL);

		//Draw
		m_commandList->IASetVertexBuffers(0, 1, &m_quadVertexView);
		m_commandList->DrawInstanced(6, 1, 0, 0);

		resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_coarseTerrainTexture.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		m_commandList->ResourceBarrier(1, &resourceBarrier);
	}
}

void HeightmapGen::CreateResources() {
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
			127,
			127,
			1,
			1,
			DXGI_FORMAT_R32_FLOAT,
			{1,0},
			D3D12_TEXTURE_LAYOUT_UNKNOWN,
			D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
		};

		CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
		ThrowIfFailed(m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COPY_DEST, NULL, IID_PPV_ARGS(&m_coarseTerrainTexture)));
		m_coarseTerrainScissorRect = { 0,0,127,127 };
		m_coarseTerrainViewport = {0.0f,0.0f,127.0f,127.0f,0.0f,1.0f};
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
	WaitForList();
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
		WaitForList();
	}
}

void HeightmapGen::CreateCoarseTerrainRTV() {
	D3D12_RENDER_TARGET_VIEW_DESC desc = {
		DXGI_FORMAT_R32_FLOAT,
		D3D12_RTV_DIMENSION_TEXTURE2D
	};
	desc.Texture2D = {
		0,
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
	WaitForList();*/

}

void HeightmapGen::GenerateChunk(ChunkGenParams params) {
	BYTE* pData;
	D3D12_RANGE readRange = {};
	m_cbuffer->Map(0, &readRange, (void**)&pData);
	memcpy(pData, &params.worldMatrix, sizeof(XMMATRIX));
	m_cbuffer->Unmap(0, nullptr);

	ThrowIfFailed(m_commandAllocator->Reset());
	ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), PipelineObjects::m_blockTerrainGen.Get()));

	m_commandList->SetGraphicsRootSignature(RootSignatures::m_gRootSignature.Get());
	m_commandList->RSSetViewports(1, &m_chunkViewport);
	m_commandList->RSSetScissorRects(1, &m_chunkScissorRect);
	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	ID3D12DescriptorHeap* pHeaps[] = { DescriptorHeaps::GetCBVHeap() };
	m_commandList->SetDescriptorHeaps(1, pHeaps);
	m_commandList->SetGraphicsRootDescriptorTable(3, m_randomTextureSRV);
	m_commandList->SetGraphicsRootConstantBufferView(0, m_cbuffer->GetGPUVirtualAddress());

	CD3DX12_RESOURCE_BARRIER resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(params.renderTarget, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_commandList->ResourceBarrier(1, &resourceBarrier);

	const float clearColor[] = { 0.0f,0.0f,0.0f,0.0f };
	m_commandList->ClearRenderTargetView(params.rtvHandle, clearColor, 0, nullptr);
	m_commandList->OMSetRenderTargets(1, &params.rtvHandle, FALSE, NULL);

	//Draw
	m_commandList->IASetVertexBuffers(0, 1, &m_quadVertexView);
	m_commandList->DrawInstanced(6, 1, 0, 0);

	resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(params.renderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	m_commandList->ResourceBarrier(1, &resourceBarrier);

	m_commandList->Close();
	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	WaitForList();
}

void HeightmapGen::WaitForList() {
	ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), 1));
	if (m_fence->GetCompletedValue() < 1) {
		ThrowIfFailed(m_fence->SetEventOnCompletion(1, m_fenceEvent));
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}
	ThrowIfFailed(m_commandAllocator->Reset());
	m_fence->Signal(0);
}

void HeightmapGen::Init() {
	CreateCommandList();
	CreateResources();
	CreateCoarseTerrainRTV();
	FillCoarseVertexBuffer();
	RefreshTexture();

	m_chunkScissorRect = { 0,0,127,127 };
	m_chunkViewport = { 0.0f,0.0f,127.0f,127.0f,0.0f,1.0f };
	D3D12_RESOURCE_DESC desc = {
		D3D12_RESOURCE_DIMENSION_BUFFER,
		0,
		256, //4 matrices
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