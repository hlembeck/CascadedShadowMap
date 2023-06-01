#include "TileGenerator.h"
#include "DescriptorHeaps.h"

void TileGenerator::Init(UINT tileWidth, UINT maxTilesPerRender) {
	CreateCommandList();
	m_blockWidth = tileWidth-1;
	D3D12_RESOURCE_DESC desc = {
			D3D12_RESOURCE_DIMENSION_TEXTURE3D,
			0,
			m_blockWidth,
			m_blockWidth,
			m_blockWidth * maxTilesPerRender,
			1,
			DXGI_FORMAT_R32G32B32A32_FLOAT,
			{1,0},
			D3D12_TEXTURE_LAYOUT_UNKNOWN,
			D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
	};
	D3D12_CLEAR_VALUE clearVal = { DXGI_FORMAT_R32G32B32A32_FLOAT };
	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
	ThrowIfFailed(m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COPY_SOURCE, &clearVal, IID_PPV_ARGS(&m_srcTexture)));
	m_scissorRect = { 0,0,(INT)m_blockWidth,(INT)m_blockWidth };
	m_viewport = { 0.0f,0.0f,(FLOAT)m_blockWidth,(FLOAT)m_blockWidth,0.0f,1.0f };

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {
		DXGI_FORMAT_R32G32B32A32_FLOAT,
		D3D12_RTV_DIMENSION_TEXTURE3D
	};
	rtvDesc.Texture3D = {
		0,
		0,
		m_blockWidth * maxTilesPerRender
	};
	m_srcRTV = DescriptorHeaps::BatchHandles(D3D12_DESCRIPTOR_HEAP_TYPE_RTV).cpuHandle;
	m_device->CreateRenderTargetView(m_srcTexture.Get(), &rtvDesc, m_srcRTV);

	CreateVertexBuffer();
	CreateConstantBuffer();
}

void TileGenerator::FillTile(XMMATRIX worldMatrix, XMUINT3 texCoords, ID3D12Resource* destResource, D3D12_GPU_DESCRIPTOR_HANDLE randSRVHandle) {
	BYTE* pData;
	D3D12_RANGE readRange = {};
	CD3DX12_RESOURCE_BARRIER resourceBarrier;
	const float clearColor[] = { 0.0f,0.0f,0.0f,0.0f };
	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_cbuffer->Map(0, &readRange, (void**)&pData);
	memcpy(pData, &worldMatrix, sizeof(XMMATRIX));
	m_cbuffer->Unmap(0, nullptr);

	ThrowIfFailed(m_commandAllocator->Reset());
	ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), PipelineObjects::m_dcSampleGenPSO.Get()));

	resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_srcTexture.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_commandList->ResourceBarrier(1, &resourceBarrier);

	m_commandList->SetGraphicsRootSignature(RootSignatures::m_gRootSignature.Get());
	m_commandList->RSSetViewports(1, &m_viewport);
	m_commandList->RSSetScissorRects(1, &m_scissorRect);
	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	ID3D12DescriptorHeap* pHeaps[] = { DescriptorHeaps::GetCBVHeap() };
	m_commandList->SetDescriptorHeaps(1, pHeaps);
	m_commandList->SetGraphicsRootDescriptorTable(3, randSRVHandle);
	m_commandList->SetGraphicsRootConstantBufferView(0, m_cbuffer->GetGPUVirtualAddress());
	m_commandList->SetGraphicsRoot32BitConstant(4, m_blockWidth, 0);

	m_commandList->ClearRenderTargetView(m_srcRTV, clearColor, 0, nullptr);
	m_commandList->OMSetRenderTargets(1, &m_srcRTV, FALSE, NULL);
	//Draw
	m_commandList->IASetVertexBuffers(0, 1, &m_vertexView);
	m_commandList->DrawInstanced(6, m_blockWidth, 0, 0);

	resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_srcTexture.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
	m_commandList->ResourceBarrier(1, &resourceBarrier);

	resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(destResource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
	m_commandList->ResourceBarrier(1, &resourceBarrier);


	D3D12_TEXTURE_COPY_LOCATION srcLocation;
	D3D12_TEXTURE_COPY_LOCATION destLocation;
	//Copy tiles
	{
		srcLocation = {
			m_srcTexture.Get(),
			D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX
		};
		destLocation = {
			destResource,
			D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX
		};
		destLocation.SubresourceIndex = 0;

		srcLocation.SubresourceIndex = 0;
		m_commandList->CopyTextureRegion(&destLocation, texCoords.x, texCoords.y, texCoords.z, &srcLocation, NULL);
	}


	resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(destResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	m_commandList->ResourceBarrier(1, &resourceBarrier);

	m_commandList->Close();
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	WaitForQueue();
}

void TileGenerator::CreateCommandList() {
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

void TileGenerator::CreateVertexBuffer() {
	float resolution = (float)m_blockWidth;
	float vertices[24] = {
		-1.0f,-1.0f, 0.0f,0.0f,
		-1.0f,1.0f, 0.0f,resolution,
		1.0f,1.0f, resolution,resolution,

		-1.0f,-1.0f, 0.0f,0.0f,
		1.0f,1.0f, resolution,resolution,
		1.0f,-1.0f, resolution,0.0f
	};

	ComPtr<ID3D12Resource> uploadBuffer;
	D3D12_RESOURCE_DESC desc = {
		D3D12_RESOURCE_DIMENSION_BUFFER,
		0,
		96,
		1,
		1,
		1,
		DXGI_FORMAT_UNKNOWN,
		{1,0},
		D3D12_TEXTURE_LAYOUT_ROW_MAJOR
	};

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
	ThrowIfFailed(m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&uploadBuffer)));
	heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	ThrowIfFailed(m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COPY_DEST, NULL, IID_PPV_ARGS(&m_vertices)));


	BYTE* pData;
	D3D12_RANGE readRange;
	uploadBuffer->Map(0, &readRange, (void**)&pData);
	memcpy(pData, vertices, 48);
	uploadBuffer->Unmap(0, nullptr);

	m_commandAllocator->Reset();
	m_commandList->Reset(m_commandAllocator.Get(), NULL);
	m_commandList->CopyResource(m_vertices.Get(), uploadBuffer.Get());
	CD3DX12_RESOURCE_BARRIER resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_vertices.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	m_commandList->ResourceBarrier(1, &resourceBarrier);
	m_commandList->Close();
	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	WaitForQueue();

	m_vertexView = {
		m_vertices->GetGPUVirtualAddress(),
		96,
		16
	};
}

void TileGenerator::CreateConstantBuffer() {
	D3D12_RESOURCE_DESC desc = {
		D3D12_RESOURCE_DIMENSION_BUFFER,
		0,
		sizeof(XMMATRIX) * 128,
		1,
		1,
		1,
		DXGI_FORMAT_UNKNOWN,
		{1,0},
		D3D12_TEXTURE_LAYOUT_ROW_MAJOR
	};

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
	ThrowIfFailed(m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&m_cbuffer)));
	//printf("Create Constant Buffer\n");
}

void TileGenerator::WaitForQueue() {
	ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), 1));
	if (m_fence->GetCompletedValue() < 1) {
		ThrowIfFailed(m_fence->SetEventOnCompletion(1, m_fenceEvent));
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}
	ThrowIfFailed(m_commandAllocator->Reset());
	m_fence->Signal(0);
}

BOOL TileGenerator::FillTiles(XMMATRIX* worldMatrices, XMUINT3* tileCoords, UINT n, ID3D12Resource* destResource, D3D12_GPU_DESCRIPTOR_HANDLE randSRVHandle) {
	if (n > 64) //max tiles per render = 64
		return FALSE;
	BYTE* pData;
	D3D12_RANGE readRange = {};
	CD3DX12_RESOURCE_BARRIER resourceBarrier;
	const float clearColor[] = { 0.0f,0.0f,0.0f,0.0f };
	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_cbuffer->Map(0, &readRange, (void**)&pData);
	memcpy(pData, &worldMatrices, sizeof(XMMATRIX)*n);
	m_cbuffer->Unmap(0, nullptr);

	ThrowIfFailed(m_commandAllocator->Reset());
	ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), PipelineObjects::m_dcSampleGenPSO.Get()));

	resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_srcTexture.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_commandList->ResourceBarrier(1, &resourceBarrier);

	m_commandList->SetGraphicsRootSignature(RootSignatures::m_gRootSignature.Get());
	m_commandList->RSSetViewports(1, &m_viewport);
	m_commandList->RSSetScissorRects(1, &m_scissorRect);
	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	ID3D12DescriptorHeap* pHeaps[] = { DescriptorHeaps::GetCBVHeap() };
	m_commandList->SetDescriptorHeaps(1, pHeaps);
	if(randSRVHandle.ptr)
		m_commandList->SetGraphicsRootDescriptorTable(3, randSRVHandle);
	m_commandList->SetGraphicsRootConstantBufferView(0, m_cbuffer->GetGPUVirtualAddress());
	m_commandList->SetGraphicsRoot32BitConstant(4, m_blockWidth, 0);

	m_commandList->ClearRenderTargetView(m_srcRTV, clearColor, 0, nullptr);
	m_commandList->OMSetRenderTargets(1, &m_srcRTV, FALSE, NULL);
	//Draw
	m_commandList->IASetVertexBuffers(0, 1, &m_vertexView);
	m_commandList->DrawInstanced(6, m_blockWidth, 0, 0);

	resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_srcTexture.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
	m_commandList->ResourceBarrier(1, &resourceBarrier);

	resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(destResource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
	m_commandList->ResourceBarrier(1, &resourceBarrier);

	//Copy
	D3D12_TEXTURE_COPY_LOCATION srcLocation = {
			m_srcTexture.Get(),
			D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX
	};
	D3D12_TEXTURE_COPY_LOCATION destLocation = {
			destResource,
			D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX
	};
	for (UINT i = 0; i < n; i++) {
		//m_commandList->CopyTextureRegion(
	}

	resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(destResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	m_commandList->ResourceBarrier(1, &resourceBarrier);

	m_commandList->Close();
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	WaitForQueue();
	return TRUE;
}