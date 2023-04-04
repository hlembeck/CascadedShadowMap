#include "ImprovedTerrain.h"
#include "DescriptorHeaps.h"

Tile::Tile() : m_isResident(0) {}

Tile::Tile(BoundingBox bounds, XMMATRIX worldMatrix, XMUINT2 texCoords, INT isResident) : m_bounds(bounds), m_worldMatrix(worldMatrix), m_texCoords(texCoords), m_isResident(isResident) {}

void Tile::SetTexCoords(XMUINT2 coords, INT isResident) {
	m_texCoords = coords;
	m_isResident = isResident;
}

BOOL Tile::IsVisible(BoundingFrustum frustum, XMMATRIX viewMatrix) {
	BoundingBox temp;
	m_bounds.Transform(temp, viewMatrix);
	return temp.Intersects(frustum) * m_isResident;
}

BOOL Tile::NeedsUpdate(BoundingFrustum frustum, XMMATRIX viewMatrix) {
	BoundingBox temp;
	m_bounds.Transform(temp, viewMatrix);
	return temp.Intersects(frustum) * (~m_isResident);
}

XMMATRIX Tile::Create(float x, float y, UINT level, XMUINT2 texCoords) {
	float halfWidth = 15.5f * (float)(2 << (NTERRAINLEVELS - level - 1));
	m_bounds = BoundingBox(
		{x+halfWidth,0.0f,y+halfWidth},
		{halfWidth,TERRAINHEIGHTMAX,halfWidth}
	);
	m_worldMatrix = XMMatrixTranslation(x, 0.0f, y);
	m_texCoords = texCoords;
	m_isResident = 1;
	return XMMatrixTranspose(XMMatrixTranslation(x, y, 0.0f));
}

TileParams Tile::GetTileParams() {
	return { XMMatrixTranspose(m_worldMatrix), m_texCoords };
}

void ImprovedChunk::Create(float x, float y) {
	m_bounds = BoundingBox(
		{ x+ (float)pow(2.0f,NTERRAINLEVELS)*31.0f, 0.0f, y + (float)pow(2.0f,NTERRAINLEVELS) * 31.0f },
		{ (float)pow(2.0f,NTERRAINLEVELS) * 31.0f, TERRAINHEIGHTMAX, (float)pow(2.0f,NTERRAINLEVELS) * 31.0f }
	);
}

ImprovedTerrain63::ImprovedTerrain63() {}

ImprovedTerrain63::~ImprovedTerrain63() {
	delete[] m_roots;
}

void ImprovedTerrain63::CreateVertexTexture() {
	ComPtr<ID3D12Resource> uploadBuffer;
	D3D12_RESOURCE_DESC desc = {
		D3D12_RESOURCE_DIMENSION_BUFFER,
		0,
		184512, //62x62 squares, with 6 vertices per square, and each vertex uses 8 bytes.
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

	XMFLOAT2* vertexData = new XMFLOAT2[23064];
	for (UINT i = 0; i < 62; i++) {
		for (UINT j = 0; j < 62; j++) {
			vertexData[6 * (i * 62 + j)] = { (float)i,(float)j };
			vertexData[6 * (i * 62 + j) + 1] = { (float)i,(float)j + 1 };
			vertexData[6 * (i * 62 + j) + 2] = { (float)i + (float)1,(float)j + 1 };

			vertexData[6 * (i * 62 + j) + 3] = { (float)i,(float)j };
			vertexData[6 * (i * 62 + j) + 4] = { (float)i + 1,(float)j + 1 };
			vertexData[6 * (i * 62 + j) + 5] = { (float)i + 1,(float)j };
		}
	}

	BYTE* pData;
	D3D12_RANGE readRange;
	uploadBuffer->Map(0, &readRange, (void**)&pData);
	memcpy(pData, vertexData, 184512);
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
		184512,
		8
	};
}

void ImprovedTerrain63::CreateVirtualTexture() {
	D3D12_RESOURCE_DESC desc = {
		D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		0,
		256, //Uses 63x63 for corners of the grid.
		256,
		1,
		1,
		DXGI_FORMAT_R32G32B32A32_FLOAT,
		{1,0},
		D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE,
	};

	ThrowIfFailed(m_device->CreateReservedResource(&desc, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, NULL, IID_PPV_ARGS(&m_virtualTexture)));

	D3D12_HEAP_DESC heapDesc = {
		1048576,
		CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		0,
		D3D12_HEAP_FLAG_DENY_BUFFERS | D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES
	};

	ThrowIfFailed(m_device->CreateHeap(&heapDesc, IID_PPV_ARGS(&m_heap)));

	HandlePair handles = DescriptorHeaps::BatchHandles(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {
		DXGI_FORMAT_R32G32B32A32_FLOAT,
		D3D12_SRV_DIMENSION_TEXTURE2D,
		D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING
	};
	srvDesc.Texture2D = {
		0,
		1,
		0,
		0.0f
	};
	m_device->CreateShaderResourceView(m_virtualTexture.Get(), &srvDesc, handles.cpuHandle);
	m_srvHandle = handles.gpuHandle;


	//TESTING
	/*UINT totalTiles = 0;
	D3D12_PACKED_MIP_INFO packedMipInfo = {};
	D3D12_TILE_SHAPE tileShapeNonPacked = {};
	UINT numSubresourceTilings = 1;
	D3D12_SUBRESOURCE_TILING subresourceTilingNonPacked = {};
	m_device->GetResourceTiling(m_virtualTexture.Get(), &totalTiles, &packedMipInfo, &tileShapeNonPacked, &numSubresourceTilings, 0, &subresourceTilingNonPacked);

	printf("Resource Tiling:\n\n");
	printf("Total Tiles: %d\n\n", totalTiles);
	printf("Packed Mip Info:\n");
	printf("Num Standard Mips: %d\n", packedMipInfo.NumStandardMips);
	printf("Num Packed Mips: %d\n", packedMipInfo.NumPackedMips);
	printf("Num Tiles for Packed Mips: %d\n", packedMipInfo.NumTilesForPackedMips);
	printf("Start Tile Index in Resource: %d\n\n", packedMipInfo.StartTileIndexInOverallResource);
	printf("Standard Tile Shape for Non-packed Mips, in Texels:\n");
	printf("Width: %d\n", tileShapeNonPacked.WidthInTexels);
	printf("Height: %d\n", tileShapeNonPacked.HeightInTexels);
	printf("Depth: %d\n\n", tileShapeNonPacked.DepthInTexels);
	printf("Num Subresource Tilings: %d\n\n", numSubresourceTilings);
	printf("Subresource Tilings for Non-packed Mips:\n");
	printf("Width (in Tiles): %d\n", subresourceTilingNonPacked.WidthInTiles);
	printf("Height (in Tiles): %d\n", subresourceTilingNonPacked.HeightInTiles);
	printf("Depth (in Tiles): %d\n", subresourceTilingNonPacked.DepthInTiles);
	printf("Start Tile Index in Resource: %d\n\n", subresourceTilingNonPacked.StartTileIndexInOverallResource);*/
}

void ImprovedTerrain63::CreateCommandList() {
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

void ImprovedTerrain63::WaitForList() {
	ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), 1));
	if (m_fence->GetCompletedValue() < 1) {
		ThrowIfFailed(m_fence->SetEventOnCompletion(1, m_fenceEvent));
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}
	ThrowIfFailed(m_commandAllocator->Reset());
	m_fence->Signal(0);
}

void ImprovedTerrain63::Init(CameraView view) {
	D3D12_RESOURCE_DESC desc = {
		D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		0,
		63,
		63,
		1,
		1,
		DXGI_FORMAT_R32_FLOAT,
		{1,0},
		D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE
	};

	CreateCommandList();
	CreateVertexTexture();
	CreateVirtualTexture();
	HeightmapGen::Init();
	CreateRoots(view);
	CreateConstantBuffer(); //Needs m_nRoots
}

void ImprovedTerrain63::CreateChunk() {
	m_chunk.Create(0.0f, 0.0f);
	HeightmapGen::GenerateBlock(XMMatrixIdentity(),m_virtualTexture.Get(),m_heap.Get());
}

void ImprovedTerrain63::RenderChunk(ID3D12GraphicsCommandList* commandList) {
	commandList->SetGraphicsRootDescriptorTable(3, m_srvHandle);
	commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	commandList->DrawInstanced(23064, 1, 0, 0);
}

void ImprovedTerrain63::RenderTiles(BoundingFrustum frustum, XMMATRIX viewMatrix, ID3D12GraphicsCommandList* commandList) {
	
	commandList->SetGraphicsRootDescriptorTable(3, m_srvHandle);
	commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	commandList->SetGraphicsRootDescriptorTable(1, m_cbvHandle);
	D3D12_RANGE range = {};
	BYTE* pData;
	TileParams temp;
	m_constantBuffer->Map(0, &range, (void**)&pData);
	for (UINT i = 0; i < m_nRoots; i++) {
		temp = m_roots[i].GetTileParams();
		memcpy(pData + sizeof(TileParams) * i, &temp, sizeof(TileParams));
	}
	m_constantBuffer->Unmap(0, nullptr);
	for (UINT i = 0; i < m_nRoots; i++) {
		commandList->SetGraphicsRoot32BitConstant(4, i, 0);
		//if (m_roots[i].IsVisible(frustum, viewMatrix)) {
			commandList->DrawInstanced(23064, 1, 0, 0);
		//}
	}
	//frameIndex++;
}

void ImprovedTerrain63::CreateRoots(CameraView view) {
	/*
	The length-edges of the view frustum are used to calculate the box of chunks handled by the game. This allows the camera to rotate without necessarily updating the chunks (rotation can be extremely fast depending on mouse settings).

	The chunks used form a square, and ideally the camera lies in the center (this doesn't occur often, as the chunks are world-bound to a grid of spacing 62*2^(NTERRAINLEVELS). Thus we initially take the square, centered at the origin, of side-lengths equal to the 62*2^(NTERRAINLEVELS)-padded view frustum's length-edges.
	*/

	float opp = tan(view.m_fovY / 2) * view.m_farZ;

	float lengthEdge = opp * sqrt(1.0f + view.m_aspectRatio * view.m_aspectRatio);

	float rootSpacing = 62.0f*pow(2.0f, NTERRAINLEVELS-1);

	INT chunkWidth = ceil(lengthEdge / rootSpacing); //Number of chunks necessary to span the length-edge
	m_nRoots = 4 * pow(chunkWidth, 2);
	m_roots = new Tile[m_nRoots];
	HeightmapGen::CreateConstantBuffer(m_nRoots);
	
	Tile* curr;
	XMMATRIX* worldMatrices = new XMMATRIX[m_nRoots];
	UINT ind;
	for (float i = 0; i < 2.0f * chunkWidth; i += 1.0f) {
		for (float j = 0; j < 2.0f * chunkWidth; j += 1.0f) {
			ind = (int)i * 2 * chunkWidth + (int)j;
			curr = &m_roots[ind];
			worldMatrices[ind] = curr->Create((-chunkWidth + i) * 62.0f, (-chunkWidth + j) * 62.0f, 0, { (UINT)i*64,(UINT)j*64 });
		}
	}
	HeightmapGen::GenerateInitialTiles(worldMatrices, m_nRoots, m_virtualTexture.Get(), m_heap.Get());
}

void ImprovedTerrain63::CreateConstantBuffer() {
	UINT paddedSize = Pad256(sizeof(TileParams) * m_nRoots);
	D3D12_RESOURCE_DESC resourceDesc = {
		D3D12_RESOURCE_DIMENSION_BUFFER,
		0,
		paddedSize,
		1,
		1,
		1,
		DXGI_FORMAT_UNKNOWN,
		{1,0},
		D3D12_TEXTURE_LAYOUT_ROW_MAJOR
	};
	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
	m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&m_constantBuffer));

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {
		m_constantBuffer->GetGPUVirtualAddress(),
		paddedSize
	};
	HandlePair handles = DescriptorHeaps::BatchHandles(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_device->CreateConstantBufferView(&cbvDesc, handles.cpuHandle);
	m_cbvHandle = handles.gpuHandle;
}

void ImprovedTerrain63::UpdateRoots(ID3D12GraphicsCommandList* commandList, BoundingFrustum frustum, XMMATRIX viewMatrix) {
	for (UINT i = 0; i < m_nRoots; i++) {
		if (m_roots[i].NeedsUpdate(frustum, viewMatrix)) {

		}
	}
}