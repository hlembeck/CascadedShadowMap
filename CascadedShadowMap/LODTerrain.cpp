#include "LODTerrain.h"
#include "DescriptorHeaps.h"

Tile::Tile() : instance(-1), children({ nullptr,nullptr,nullptr,nullptr }) {}

Tile::Tile(float x, float y, D3D12_TILED_RESOURCE_COORDINATE texCoords, float width, TerrainLODViewParams viewParams) : instance(-1), children({nullptr,nullptr,nullptr,nullptr}) {
	Init(x, y, texCoords, width, viewParams);
}

Tile::~Tile() {
	//printf("(%g, %g) %g\n", bounds.Center.x, bounds.Center.z, bounds.Extents.x);
	delete children.t1;
	delete children.t2;
	delete children.t3;
	delete children.t4;
}

void Tile::Init(float x, float y, D3D12_TILED_RESOURCE_COORDINATE texCoords, float width, TerrainLODViewParams viewParams) {
	//printf("%g\n", width);
	float halfWidth = width * 31.0f;
	position = { x,y };
	bounds = BoundingBox(
		{ x + halfWidth,0.0f,y + halfWidth },
		{ halfWidth,TERRAINHEIGHTMAX,halfWidth }
	);
	tileParams = { XMMatrixTranspose(XMMatrixMultiply(XMMatrixScaling(width,1.0f,width),XMMatrixTranslation(x, 0.0f, y))),{64 * texCoords.X,64 * texCoords.Y} };

	float radius = sqrt(halfWidth * halfWidth * 2.0f + TERRAINHEIGHTMAX * TERRAINHEIGHTMAX);
	XMFLOAT4 circumSphere = { bounds.Center.x,0.0f,bounds.Center.z,radius };
	circumSphere = { circumSphere.x - viewParams.position.x,circumSphere.y - viewParams.position.y,circumSphere.z - viewParams.position.z,circumSphere.w };
	float d = sqrt(circumSphere.x * circumSphere.x + circumSphere.y * circumSphere.y + circumSphere.z * circumSphere.z) - circumSphere.w;
	error = 0.5f * halfWidth * viewParams.screenWidth;
	error /= std::max(d, 1.0f) * viewParams.tanFOVH;
	error = std::min(abs(error), 1000000.0f);
}

BOOL Tile::IsVisible(BoundingFrustum frustum, XMMATRIX viewMatrix) {
	BoundingBox temp;
	bounds.Transform(temp, viewMatrix);

	if (bounds.Extents.x == 31.0f) {
		printf("%d\n", temp.Intersects(frustum));
	}
	return temp.Intersects(frustum);
}

void Tile::MakeResident(INT inst, D3D12_TILED_RESOURCE_COORDINATE tileCoords) {
	instance = inst;
	tileParams.texCoords = {tileCoords.X * 64, tileCoords.Y * 64};
}

TileCluster Tile::CreateChildren(TerrainLODViewParams viewParams, D3D12_TILED_RESOURCE_COORDINATE tile, INT inst) {
	instance = inst;
	float width = bounds.Extents.x / 62.0f;
	float x = bounds.Center.x;
	float y = bounds.Center.z;
	children.t1 = new Tile(x, y, tile, width, viewParams);
	children.t2 = new Tile(x - bounds.Extents.x, y, {tile.X + 1,tile.Y}, width, viewParams);
	children.t3 = new Tile(x, y- bounds.Extents.x, { tile.X + 2,tile.Y }, width, viewParams);
	children.t4 = new Tile(x- bounds.Extents.x, y- bounds.Extents.x,{ tile.X + 3,tile.Y }, width, viewParams);

	return children;
}

XMMATRIX Tile::GetCreationMatrix() {
	static const float scaleFactor = 63.0f / 62.0f;
	float halfWidth = bounds.Extents.x;
	return XMMatrixTranspose(XMMatrixMultiply(XMMatrixScaling(halfWidth*scaleFactor, halfWidth*scaleFactor, 1.0f), XMMatrixTranslation(bounds.Center.x - halfWidth, bounds.Center.z - halfWidth, 0.0f)));
}

XMMATRIX Chunk::Create(float x, float y, D3D12_TILED_RESOURCE_COORDINATE texCoords, float width, TerrainLODViewParams viewParams) {
	Init(x, y, texCoords, width, viewParams);

	return XMMatrixTranspose(XMMatrixMultiply(XMMatrixScaling(width, width, 1.0f), XMMatrixTranslation(x, y, 0.0f)));
}

HeapTileManager::~HeapTileManager() { delete[] m_freeTiles; }

INT HeapTileManager::Pop() {
	if (m_end==0)
		return -1;
	return m_freeTiles[--m_end];
}

void HeapTileManager::Push(INT tile) {
	m_freeTiles[m_end++] = tile;
}

void HeapTileManager::Init(UINT numTiles) {
	D3D12_HEAP_DESC heapDesc = {
		numTiles * 65536,
		CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		0,
		D3D12_HEAP_FLAG_DENY_BUFFERS | D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES
	};
	ThrowIfFailed(m_device->CreateHeap(&heapDesc, IID_PPV_ARGS(&m_heap)));

	m_size = numTiles / 4;
	m_freeTiles = new INT[m_size];
	for (UINT i = 0; i < m_size; i++) {
		m_freeTiles[i] = 4 * i;
	}
	m_end = m_size;
}

VirtualTextureManager::~VirtualTextureManager() { delete[] m_freeTiles; }

BOOL VirtualTextureManager::Pop(D3D12_TILED_RESOURCE_COORDINATE* tile) {
	if (m_end==0)
		return FALSE;
	*tile = m_freeTiles[--m_end];
	return TRUE;
}

void VirtualTextureManager::Push(D3D12_TILED_RESOURCE_COORDINATE tile) {
	m_freeTiles[m_end++] = tile;
}

void VirtualTextureManager::Init() {
	D3D12_RESOURCE_DESC desc = {
		D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		0,
		16384,
		16384,
		1,
		1,
		DXGI_FORMAT_R32G32B32A32_FLOAT,
		{1,0},
		D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE,
	};

	ThrowIfFailed(m_device->CreateReservedResource(&desc, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, NULL, IID_PPV_ARGS(&m_virtualTexture)));
	m_freeTiles = new D3D12_TILED_RESOURCE_COORDINATE[16353]; // Tile is 64x64, so there are 2^8=256 per dimension, giving 256*64=16384 groups of four tiles. 31 of these are used for coarse chunks, leaving 16353 tiles remaining.

	for (UINT i = 31; i < 64; i++) {
		m_freeTiles[i-31] = { 4 * i,0 };
	}

	for (UINT i = 1; i < 256; i++) {
		for (UINT j = 0; j < 64; j++) {
			m_freeTiles[33+(i-1)*64+j] = {4*j,i};
		}
	}
	m_end = 16353;
}

TerrainTileManager::TerrainTileManager() {}
TerrainTileManager::~TerrainTileManager() { 
	delete[] m_currInstances;
	delete[] m_freeInstances;
	delete[] m_tileRegionSizes;
	delete[] m_heapRegionSizes;
}

INT TerrainTileManager::Push(UINT heapIndex, D3D12_TILED_RESOURCE_COORDINATE tile) {
	//printf("%d %d\n", tile.X,tile.Y);
	if (m_back == 0)
		return -1;
	UINT index = m_freeInstances[--m_back];
	m_currInstances[index] = { heapIndex,tile };
	return index;
}

TileInstance TerrainTileManager::Remove(UINT index) {
	m_freeInstances[m_back++] = index;
	TileInstance ret = m_currInstances[index];
	m_currInstances[index].heapIndex = -1;
	return ret;
}

void TerrainTileManager::Init(UINT n) {
	m_numInstances = n;
	m_currInstances = new TileInstance[n];
	m_freeInstances = new UINT[n];

	//Constants
	m_tileRegionSizes = new D3D12_TILE_REGION_SIZE[n];
	m_heapRegionSizes = new UINT[n];
	D3D12_TILE_REGION_SIZE constSize = {
		4,
		FALSE
	};
	for (UINT i = 0; i < n; i++) {
		m_tileRegionSizes[i] = constSize;
		m_heapRegionSizes[i] = 4;
		m_freeInstances[i] = i;
		m_currInstances[i].heapIndex = -1;
	}
	m_back = n;
}

BOOL TerrainTileManager::HasCapacity() {
	return m_back;
}

void TerrainTileManager::UpdateTileMappings(ID3D12Resource* virtualTexture, ID3D12Heap* heap) {
	/*for (UINT i = 0; i < m_back; i++) {
		printf("Tile: (%d, %d, %d, %d)\n", m_currResourceTiles[i].X, m_currResourceTiles[i].Y, m_currResourceTiles[i].Z, m_currResourceTiles[i].Subresource);
	}*/

	UINT size = m_numInstances - m_back;
	if (size == 0)
		return;

	D3D12_TILED_RESOURCE_COORDINATE* pResourceRegionStartCoords = new D3D12_TILED_RESOURCE_COORDINATE[size];
	UINT* pHeapRangeStartOffsets = new UINT[size];
	
	UINT j = 0;
	TileInstance curr;
	for (UINT i = 0; i < m_numInstances; i++) {
		curr = m_currInstances[i];
		if (curr.heapIndex != -1) {
			pResourceRegionStartCoords[j] = curr.tile;
			pHeapRangeStartOffsets[j++] = curr.heapIndex;
		}
	}

	m_commandQueue->UpdateTileMappings(
		virtualTexture,
		size,
		pResourceRegionStartCoords,
		m_tileRegionSizes,
		heap,
		size,
		nullptr,
		pHeapRangeStartOffsets,
		m_heapRegionSizes,
		D3D12_TILE_MAPPING_FLAG_NONE
	);

	delete[] pResourceRegionStartCoords;
	delete[] pHeapRangeStartOffsets;
}

LODTerrain::LODTerrain() : m_chunkPosition({ 0.0f,0.0f }) {}

LODTerrain::~LODTerrain() {}

void LODTerrain::CreateVertexTexture() {
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

void LODTerrain::CreateVirtualTexture() {
	//Heap for chunks
	D3D12_HEAP_DESC heapDesc = {
		7929856,
		CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		0,
		D3D12_HEAP_FLAG_DENY_BUFFERS | D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES
	};

	ThrowIfFailed(m_device->CreateHeap(&heapDesc, IID_PPV_ARGS(&m_coarseHeap)));

	//D3D12_RESOURCE_DESC desc = {
	//	D3D12_RESOURCE_DIMENSION_TEXTURE2D,
	//	0,
	//	16384/*704*/, //Uses 63x63 for corners of the grid.
	//	16384,
	//	1,
	//	1,
	//	DXGI_FORMAT_R32G32B32A32_FLOAT,
	//	{1,0},
	//	D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE,
	//};

	//ThrowIfFailed(m_device->CreateReservedResource(&desc, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, NULL, IID_PPV_ARGS(&m_virtualTexture)));

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
}

void LODTerrain::CreateCommandList() {
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

void LODTerrain::WaitForList() {
	ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), 1));
	if (m_fence->GetCompletedValue() < 1) {
		ThrowIfFailed(m_fence->SetEventOnCompletion(1, m_fenceEvent));
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}
	ThrowIfFailed(m_commandAllocator->Reset());
	m_fence->Signal(0);
}

void LODTerrain::Init(CameraView view, TerrainLODViewParams viewParams) {
	HeapTileManager::Init();
	VirtualTextureManager::Init();
	TerrainTileManager::Init(64);
	CreateCommandList();
	CreateVertexTexture();

	CreateVirtualTexture();
	HeightmapGen::Init(121);
	CreateChunks(view, viewParams);
	CreateConstantBuffer();
}

void LODTerrain::CreateChunks(CameraView view, TerrainLODViewParams viewParams) {
	float y = tan(view.m_fovY / 2);

	float lengthEdge = view.m_farZ * sqrt(1.0f + y * y * (view.m_aspectRatio * view.m_aspectRatio + 1.0f));

	//Find optimal power of 2 grid spacing, to cover frustum.
	y = lengthEdge / 310.0f;
	y = ceil(log2(y));

	m_chunkWidth = 62.0f * (float)pow(2.0f, y);

	HeightmapGen::CreateConstantBuffer(121);

	D3D12_TILED_RESOURCE_COORDINATE* tiles = new D3D12_TILED_RESOURCE_COORDINATE[121];
	for (UINT i = 0; i < 121; i++) {
		tiles[i] = { i,0 };
	}
	y = -m_chunkWidth * 5.0f;

	//Loop through the roots.
	for (UINT i = 0; i < 11; i++) {
		for (UINT j = 0; j < 11; j++) {
			m_chunks[i * 11 + j].Init(y + i * m_chunkWidth, y + j * m_chunkWidth, tiles[i * 11 + j], m_chunkWidth / 62.0f, viewParams);
			//worldMatrices[i * 11 + j] = m_chunks[i * 11 + j].GetCreationMatrix();
		}
	}

	{ //UpdateTileMappings on coarseHeap
		D3D12_TILED_RESOURCE_COORDINATE tiledCoordinate = { 0,0,0 };

		D3D12_TILE_RANGE_FLAGS flag = D3D12_TILE_RANGE_FLAG_NONE;

		UINT heapRangeStartOffset = 0;

		m_commandQueue->UpdateTileMappings(
			VirtualTextureManager::m_virtualTexture.Get(),
			121,
			tiles,
			NULL,
			m_coarseHeap.Get(),
			1,
			&flag,
			&heapRangeStartOffset,
			NULL,
			D3D12_TILE_MAPPING_FLAG_NONE
		);
		WaitForQueue();
	}
	delete[] tiles;


	std::priority_queue<Tile*, std::vector<Tile*>, TileCmp> queue;
	for (UINT i = 0; i < 121; i++) {
		queue.push(m_chunks + i);
	}


	std::vector<XMMATRIX> tilesToGen;
	std::vector<XMUINT2> texTiles;

	Tile* curr;
	TileCluster temp;
	D3D12_TILED_RESOURCE_COORDINATE tile;
	INT instance;
	//Queue, since container is vector, can have size grow considerably larger than capacity of tile managers.
	/*
	VirtualTextureManager holds more tiles than can fit in either the HeapTileManager or the TerrainTileManager, so if proper
	management of these heaps is done, there is no need to check if VirtualTextureManager is full.

	If TerrainTileManager is initialized with size at most that of HeapTileManager, then since heap tiles are injectively mapped into TerrainTileManager, proper management ensures no need to check HeapTileManager capacity.
	
	That is, we only need check TerrrainTileManager has capacity for the children of a tile, before creating the children.
	*/
	//Tiles in the queue are already mapped to heap and texture tiles (in TerrainTileManager, or are coarse chunks), so calling UpdateTileMappings on the array in TerrainTileManager will push these mappings to the GPU.
	UINT test = 0;
	while (!queue.empty() && queue.size() < 350 && TerrainTileManager::HasCapacity()) {
		curr = queue.top();
		//printf("%g\n", curr->error);
		queue.pop();
		tilesToGen.push_back(curr->GetCreationMatrix());
		texTiles.push_back(curr->tileParams.texCoords);
		//printf("%d %d\n", curr->tileParams.texCoords.x/64, curr->tileParams.texCoords.y/64);
		if (curr->error > 1000.0f && curr->bounds.Extents.x >= 62.0f) {
			test++;
			//printf("test\n");
			VirtualTextureManager::Pop(&tile);
			//printf("%d %d %d\n", tile.X, tile.Y, tile.Subresource);
			instance = TerrainTileManager::Push((UINT)HeapTileManager::Pop(), tile);
			temp = curr->CreateChildren(viewParams,tile,instance);
			queue.push(temp.t1);
			queue.push(temp.t2);
			queue.push(temp.t3);
			queue.push(temp.t4);
		}
	}

	while (!queue.empty()) {
		curr = queue.top();
		//printf("%g\n", curr->error);
		queue.pop();
		tilesToGen.push_back(curr->GetCreationMatrix());
		texTiles.push_back(curr->tileParams.texCoords);
	}

	TerrainTileManager::UpdateTileMappings(m_virtualTexture.Get(), m_heap.Get());

	HeightmapGen::GenerateTiles(tilesToGen.data(), texTiles.data(), tilesToGen.size(), VirtualTextureManager::m_virtualTexture.Get());
	WaitForQueue();

	/*INT i = 0;
	Tile* curr = queue.top();
	std::vector<XMMATRIX> matrices;
	std::vector<D3D12_TILED_RESOURCE_COORDINATE> tileCoords;
	while (i < 4065 && curr) {
		curr->tile = HeightmapGen::PopQueue();
		curr->SetTexCoords(PopQueue());
	}*/

}

void LODTerrain::CreateConstantBuffer() {
	D3D12_RESOURCE_DESC resourceDesc = {
		D3D12_RESOURCE_DIMENSION_BUFFER,
		0,
		65536,
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
		65536 //Maximum size
	};
	HandlePair handles = DescriptorHeaps::BatchHandles(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_device->CreateConstantBufferView(&cbvDesc, handles.cpuHandle);
	m_cbvHandle = handles.gpuHandle;
}

std::vector<TileParams> LODTerrain::GetTilesToRender(TerrainLODViewParams viewParams) {
	std::vector<TileParams> ret;

	std::priority_queue < Tile*, std::vector<Tile*>, TileCmp> tileQueue;

	for (UINT i = 0; i < 121; i++) {
		tileQueue.push(m_chunks + i);
	}
	Tile* curr;
	UINT test = 0;
	while (!tileQueue.empty() && test < 20) {
		curr = tileQueue.top();
		tileQueue.pop();
		if (curr->IsVisible(viewParams.frustum, viewParams.viewMatrix)) {
			if (curr->error > 1000.0f && curr->instance != -1) {
				test++;
				//printf("%g\n", curr->children.t1->bounds.Extents.x / 31.0f);
				tileQueue.push(curr->children.t1);
				tileQueue.push(curr->children.t2);
				tileQueue.push(curr->children.t3);
				tileQueue.push(curr->children.t4);
				/*printf("(%d %d), ", curr->children.t1->tileParams.texCoords.x, curr->children.t1->tileParams.texCoords.y);
				printf("(%d %d), ", curr->children.t2->tileParams.texCoords.x, curr->children.t2->tileParams.texCoords.y);
				printf("(%d %d), ", curr->children.t3->tileParams.texCoords.x, curr->children.t3->tileParams.texCoords.y);
				printf("(%d %d)\n", curr->children.t4->tileParams.texCoords.x, curr->children.t4->tileParams.texCoords.y);*/
			}
			else
				ret.push_back(curr->tileParams);
		}
	}

	//printf("\n\n\n\n");

	while (!tileQueue.empty()) {
		curr = tileQueue.top();
		tileQueue.pop();
		if (curr->IsVisible(viewParams.frustum, viewParams.viewMatrix)) {
			ret.push_back(curr->tileParams);
		}
	}

	return ret;
}


//(14848 16320), (14912 16320), (14976 16320), (15040 16320)


void LODTerrain::RenderChunks(TerrainLODViewParams viewParams, ID3D12GraphicsCommandList* commandList) {
	commandList->SetGraphicsRootDescriptorTable(3, m_srvHandle);
	commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	commandList->SetGraphicsRootDescriptorTable(1, m_cbvHandle);
	std::vector<TileParams> tileParams = GetTilesToRender(viewParams);

	UINT n;
	D3D12_RANGE range = {};
	BYTE* pData;
	for (UINT k = 0; k < tileParams.size(); k += 819) {
		n = std::min(tileParams.size() - k, (UINT64)819);

		//printf("%d\n", n);

		m_constantBuffer->Map(0, &range, (void**)&pData);
		memcpy(pData, tileParams.data()+k, n * sizeof(TileParams));
		m_constantBuffer->Unmap(0, nullptr);
		commandList->DrawInstanced(23064, n, 0, 0);
	}
	//printf("\n\n\n\n");
}

void LODTerrain::Render(TerrainLODViewParams viewParams, ID3D12GraphicsCommandList* commandList) {
	commandList->SetGraphicsRootDescriptorTable(3, m_srvHandle);
	commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	commandList->SetGraphicsRootDescriptorTable(1, m_cbvHandle);

	std::vector<TileParams> tileParams = GetTilesToRender(viewParams);
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