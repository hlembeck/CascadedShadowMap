#include "DualContour.h"

//DCNode::DCNode(BoundingBox bb, XMFLOAT4 camPos) {
//	static const float sqrt2 = 1.41421356f;
//	bounds = bb;
//	camPos.x -= bounds.Center.x;
//	camPos.y -= bounds.Center.y;
//	camPos.z -= bounds.Center.z;
//	float dist = sqrt(camPos.x*camPos.x + camPos.y*camPos.y + camPos.z*camPos.z);
//	fov = atan(sqrt2*bounds.Extents.x/dist); //Assumes Extents.x = Extents.y = Extents.z i.e bb is a cube
//}

void DualContour::CreateVertexBuffer(UINT resolution) {
	ComPtr<ID3D12Resource> uploadBuffer;
	D3D12_RESOURCE_DESC desc = {
		D3D12_RESOURCE_DIMENSION_BUFFER,
		0,
		16 * resolution * resolution * resolution,
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
	ThrowIfFailed(m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COPY_DEST, NULL, IID_PPV_ARGS(&m_vertexBuffer)));

	XMFLOAT4* vertexData = new XMFLOAT4[resolution * resolution * resolution];
	for (UINT i = 0; i < resolution; i++) {
		for (UINT j = 0; j < resolution; j++) {
			for (UINT k = 0; k < resolution; k++) {
				vertexData[resolution * (i + j * resolution) + k] = { (float)i,(float)j,(float)k,1.0f };
			}
		}
	}

	BYTE* pData;
	D3D12_RANGE readRange;
	uploadBuffer->Map(0, &readRange, (void**)&pData);
	memcpy(pData, vertexData, 16 * resolution * resolution * resolution);
	uploadBuffer->Unmap(0, nullptr);

	delete[] vertexData;

	m_commandAllocator->Reset();
	m_commandList->Reset(m_commandAllocator.Get(), NULL);
	m_commandList->CopyResource(m_vertexBuffer.Get(), uploadBuffer.Get());
	CD3DX12_RESOURCE_BARRIER resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_vertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	m_commandList->ResourceBarrier(1, &resourceBarrier);
	m_commandList->Close();
	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	WaitForList();

	m_vertexBufferView = {
		m_vertexBuffer->GetGPUVirtualAddress(),
		16 * resolution * resolution * resolution,
		16
	};
}

void DualContour::WaitForList() {
	ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), 1));
	if (m_fence->GetCompletedValue() < 1) {
		ThrowIfFailed(m_fence->SetEventOnCompletion(1, m_fenceEvent));
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}
	ThrowIfFailed(m_commandAllocator->Reset());
	m_fence->Signal(0);
}

void DualContour::Init() {
	HeapTileManager::Init();
	VirtualTextureManager::Init();
	TerrainTileManager::Init(64);

	UINT numTilesForResource;
	D3D12_PACKED_MIP_INFO packedMipInfo;
	D3D12_TILE_SHAPE standardTileShapeNonPackedMips;
	UINT numSubresourceTilings = 1;
	D3D12_SUBRESOURCE_TILING subresourceTilingNonPackedMips;
	m_device->GetResourceTiling(
		VirtualTextureManager::m_virtualTexture.Get(),
		&numTilesForResource,
		&packedMipInfo,
		&standardTileShapeNonPackedMips,
		&numSubresourceTilings,
		0,
		&subresourceTilingNonPackedMips
	);

	TileGenerator::Init(standardTileShapeNonPackedMips.WidthInTexels);
	CreateVertexBuffer(standardTileShapeNonPackedMips.WidthInTexels - 2);
	//32x32x32 tiles.
}

void DualContour::Render(ID3D12GraphicsCommandList* commandList) {
	//commandList->SetGraphicsRootDescriptorTable(3, m_srvHandle);
	commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	//commandList->SetGraphicsRootDescriptorTable(1, m_cbvHandle);
	//std::vector<TileParams> tileParams = GetTilesToRender(viewParams);

	//printf("%d\n", m_vertexBufferView.SizeInBytes / m_vertexBufferView.StrideInBytes);
	commandList->DrawInstanced(m_vertexBufferView.SizeInBytes / m_vertexBufferView.StrideInBytes, 1, 0, 0);

	//UINT n;
	//D3D12_RANGE range = {};
	//BYTE* pData;
	//for (UINT k = 0; k < tileParams.size(); k += 819) {
	//	n = std::min(tileParams.size() - k, (UINT64)819);

	//	m_constantBuffer->Map(0, &range, (void**)&pData);
	//	memcpy(pData, tileParams.data() + k, n * sizeof(TileParams));
	//	m_constantBuffer->Unmap(0, nullptr);
	//	commandList->DrawInstanced(23064, n, 0, 0);
	//}
}