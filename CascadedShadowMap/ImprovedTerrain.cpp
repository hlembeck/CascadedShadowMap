#include "ImprovedTerrain.h"
#include "DescriptorHeaps.h"

Tile::~Tile() {}

XMMATRIX Tile::Create(float x, float y, D3D12_TILED_RESOURCE_COORDINATE texCoords, float width, TerrainLODViewParams viewParams) {

	float halfWidth = width * 31.0f;

	m_position = { x,y };

	m_bounds = BoundingBox(
		{ x + halfWidth,0.0f,y + halfWidth },
		{ halfWidth,TERRAINHEIGHTMAX,halfWidth }
	);

	m_tileParams = { XMMatrixTranspose(XMMatrixMultiply(XMMatrixScaling(width,1.0f,width),XMMatrixTranslation(x, 0.0f, y))),{64 * texCoords.X,64 * texCoords.Y} };

	return XMMatrixTranspose(XMMatrixMultiply(XMMatrixScaling(width, width, 1.0f), XMMatrixTranslation(x, y, 0.0f)));
}

XMMATRIX Tile::Update(float x, float y, float width) {
	float halfWidth = width * 31.0f;

	m_position = { x, y };

	m_bounds = BoundingBox(
		{ m_position.x + halfWidth,0.0f,m_position.y + halfWidth },
		{ halfWidth,TERRAINHEIGHTMAX,halfWidth }
	);

	m_tileParams.worldMatrix = XMMatrixTranspose(XMMatrixMultiply(XMMatrixScaling(width, 1.0f, width), XMMatrixTranslation(m_position.x, 0.0f, m_position.y)));

	return XMMatrixTranspose(XMMatrixMultiply(XMMatrixScaling(width, width, 1.0f), XMMatrixTranslation(m_position.x, m_position.y, 0.0f)));
}

BOOL Tile::IsVisible(BoundingFrustum frustum, XMMATRIX viewMatrix) {
	BoundingBox temp;
	m_bounds.Transform(temp, viewMatrix);
	return temp.Intersects(frustum);
}

TileParams Tile::GetTileParams() {
	return m_tileParams;
}

XMFLOAT2 Tile::GetPosition() {
	return m_position;
}

XMUINT2 Tile::GetTexCoords() {
	return m_tileParams.texCoords;
}

//Adapted from Real-Time Rendering, Ch.19
//tanFOVH = tan(fovH/2) = tan(0.5f*view.m_fovY)*view.m_aspectRatio
void Tile::SetError(TerrainLODViewParams viewParams) {
	float halfWidth = m_bounds.Extents.x;
	float radius = sqrt(halfWidth * halfWidth * 2.0f + TERRAINHEIGHTMAX * TERRAINHEIGHTMAX);

	XMFLOAT4 circumSphere = { m_bounds.Center.x,0.0f,m_bounds.Center.z,radius };

	circumSphere = { circumSphere.x - viewParams.position.x,circumSphere.y - viewParams.position.y,circumSphere.z - viewParams.position.z,circumSphere.w };
	float d = sqrt(circumSphere.x * circumSphere.x + circumSphere.y * circumSphere.y + circumSphere.z * circumSphere.z) - circumSphere.w;
	m_error = 0.5f * halfWidth * viewParams.screenWidth;
	m_error /= std::max(d, 0.0001f) * viewParams.tanFOVH;
}

float Tile::GetError() {
	return m_error;
}

Terrain::Terrain() : m_TilePosition({ 0.0f,0.0f }) {}

Terrain::~Terrain() { delete[] m_texTiles; }

void Terrain::CreateVertexTexture() {
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

void Terrain::CreateVirtualTexture() {
	D3D12_RESOURCE_DESC desc = {
		D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		0,
		16384/*704*/, //Uses 63x63 for corners of the grid.
		16384,
		1,
		1,
		DXGI_FORMAT_R32G32B32A32_FLOAT,
		{1,0},
		D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE,
	};

	ThrowIfFailed(m_device->CreateReservedResource(&desc, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, NULL, IID_PPV_ARGS(&m_virtualTexture)));

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

void Terrain::CreateCommandList() {
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

void Terrain::WaitForList() {
	ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), 1));
	if (m_fence->GetCompletedValue() < 1) {
		ThrowIfFailed(m_fence->SetEventOnCompletion(1, m_fenceEvent));
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}
	ThrowIfFailed(m_commandAllocator->Reset());
	m_fence->Signal(0);
}

void Terrain::Init(CameraView view, TerrainLODViewParams viewParams) {
	CreateCommandList();
	CreateVertexTexture();

	m_texTiles = new D3D12_TILED_RESOURCE_COORDINATE[4096];
	m_begin = 0;
	m_end = 4096;
	for (UINT i = 0; i < 64; i++) {
		for (UINT j = 0; j < 64; j++) {
			m_texTiles[i * 64 + j] = { i,j };
		}
	}

	std::pair<XMMATRIX*, D3D12_TILED_RESOURCE_COORDINATE*> tiles = CreateRoots(view, viewParams);
	CreateVirtualTexture();
	HeightmapGen::Init(121);
	HeightmapGen::GenerateInitialTiles(tiles.first, tiles.second, 121, m_virtualTexture.Get(), m_TileWidth / 62.0f);
	CreateConstantBuffer(); //Needs m_nRoots
	delete[] tiles.first;
	delete[] tiles.second;
}

void Terrain::RenderTiles(TerrainLODViewParams viewParams, ID3D12GraphicsCommandList* commandList) {
	commandList->SetGraphicsRootDescriptorTable(3, m_srvHandle);
	commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	commandList->SetGraphicsRootDescriptorTable(1, m_cbvHandle);
	std::vector<TileParams> tileParams;
	for (UINT i = 0; i < 121; i++) {
		if (m_Tiles[i].IsVisible(viewParams.frustum, viewParams.viewMatrix)) {
			tileParams.push_back(m_Tiles[i].GetTileParams());
		}
	}

	D3D12_RANGE range = {};
	BYTE* pData;
	m_constantBuffer->Map(0, &range, (void**)&pData);
	memcpy(pData, tileParams.data(), sizeof(TileParams) * tileParams.size());
	m_constantBuffer->Unmap(0, nullptr);
	for (UINT i = 0; i < tileParams.size(); i++) {
		commandList->SetGraphicsRoot32BitConstant(4, i, 0);
		commandList->DrawInstanced(23064, 1, 0, 0);
	}
}

std::pair<XMMATRIX*, D3D12_TILED_RESOURCE_COORDINATE*> Terrain::CreateRoots(CameraView view, TerrainLODViewParams viewParams) {
	/*
	The length-edges of the view frustum are used to calculate the box of Tiles handled by the game. This allows the camera to rotate without necessarily updating the Tiles (rotation can be extremely fast depending on mouse settings).

	The Tiles used form a square, and ideally the camera lies in the center (this doesn't occur often, as the Tiles are world-bound to a grid of spacing 62*2^(NTERRAINLEVELS). Thus we initially take the square, centered at the origin, of side-lengths equal to the 62*2^(NTERRAINLEVELS)-padded view frustum's length-edges.
	*/

	struct TileCmp {
		BOOL Cmp(Tile* a, Tile* b) {
			return a->GetError() > b->GetError() ? TRUE : FALSE;
		};
	};

	std::priority_queue<Tile*, std::vector<Tile*>, TileCmp> heap;

	float y = tan(view.m_fovY / 2);

	float lengthEdge = view.m_farZ * sqrt(1.0f + y * y * (view.m_aspectRatio * view.m_aspectRatio + 1.0f));

	//Find optimal power of 2 grid spacing, to cover frustum.
	y = lengthEdge / 310.0f;
	y = ceil(log2(y));

	m_TileWidth = 62.0f * (float)pow(2.0f, y);

	HeightmapGen::CreateConstantBuffer(121);

	XMMATRIX* worldMatrices = new XMMATRIX[121];
	D3D12_TILED_RESOURCE_COORDINATE* tiles = new D3D12_TILED_RESOURCE_COORDINATE[121];
	y = -m_TileWidth * 5.0f;
	//Loop through the roots.
	for (UINT i = 0; i < 11; i++) {
		for (UINT j = 0; j < 11; j++) {
			BatchTexTile(tiles + i * 11 + j);
			worldMatrices[i * 11 + j] = m_Tiles[i * 11 + j].Create(y + i * m_TileWidth, y + j * m_TileWidth, tiles[i * 11 + j], m_TileWidth / 62.0f, viewParams);
		}
	}

	//for (float i = 0; i < 2.0f * m_TileWidth; i += 1.0f) {
	//	for (float j = 0; j < 2.0f * TileWidth; j += 1.0f) {
	//		ind = (int)i * 2 * TileWidth + (int)j;
	//		m_Tiles[ind] = new Tile[(pow(4, NTERRAINLEVELS) - 1) / 3];
	//		//worldMatrices[ind] = curr->Create((-TileWidth + i) * 62.0f, (-TileWidth + j) * 62.0f, 0, { (UINT)i*64,(UINT)j*64 }, 1);
	//		worldMatrices[ind] = CreateTileTree(ind, (-TileWidth + i) * 62.0f, (-TileWidth + j) * 62.0f, { (UINT)i * 64,(UINT)j * 64 });
	//	}
	//}
	return { worldMatrices,tiles };
}

void Terrain::CreateConstantBuffer() {
	UINT paddedSize = Pad256(sizeof(TileParams) * 121);
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

void Terrain::UpdateRoots(TerrainLODViewParams viewParams) {

	BOOL stationary = TRUE;
	{
		if (viewParams.position.x - m_TilePosition.x > m_TileWidth) {
			m_TilePosition.x += m_TileWidth;
			stationary = FALSE;
		}
		else if (m_TilePosition.x - viewParams.position.x > m_TileWidth) {
			m_TilePosition.x -= m_TileWidth;
			stationary = FALSE;
		}
		if (viewParams.position.z - m_TilePosition.y > m_TileWidth) {
			m_TilePosition.y += m_TileWidth;
			stationary = FALSE;
		}
		else if (m_TilePosition.y - viewParams.position.z > m_TileWidth) {
			m_TilePosition.y -= m_TileWidth;
			stationary = FALSE;
		}
		if (stationary)
			return;
	}

	XMFLOAT2 temp;
	std::vector<XMMATRIX> matrices;
	std::vector<XMUINT2> texCoords;
	for (UINT i = 0; i < 121; i++) {
		stationary = TRUE;
		temp = m_Tiles[i].GetPosition();

		if (temp.x > m_TilePosition.x + 5.5f * m_TileWidth) {
			temp.x = m_TilePosition.x - 5.0f * m_TileWidth;
			stationary = FALSE;
		}
		else if (temp.x < m_TilePosition.x - 5.5f * m_TileWidth) {
			temp.x = m_TilePosition.x + 5.0f * m_TileWidth;
			stationary = FALSE;
		}
		if (temp.y > m_TilePosition.y + 5.5f * m_TileWidth) {
			temp.y = m_TilePosition.y - 5.0f * m_TileWidth;
			stationary = FALSE;
		}
		else if (temp.y < m_TilePosition.y - 5.5f * m_TileWidth) {
			temp.y = m_TilePosition.y + 5.0f * m_TileWidth;
			stationary = FALSE;
		}
		if (stationary == FALSE) {
			matrices.push_back(m_Tiles[i].Update(temp.x, temp.y, m_TileWidth / 62.0f));
			texCoords.push_back(m_Tiles[i].GetTexCoords());
		}
	}
	if (matrices.size() > 0) {
		HeightmapGen::UpdateChunks(matrices.data(), texCoords.data(), matrices.size(), m_virtualTexture.Get(), m_TileWidth / 62.0f);
	}
}

BOOL Terrain::BatchTexTile(D3D12_TILED_RESOURCE_COORDINATE* pTexCoords) {
	if (m_begin > m_end)
		return FALSE;
	*pTexCoords = m_texTiles[(m_begin++) % 4096];
	return TRUE;
}

void Terrain::PushTexTile(D3D12_TILED_RESOURCE_COORDINATE texCoords) {
	m_texTiles[(++m_end) % 16384] = texCoords;
}