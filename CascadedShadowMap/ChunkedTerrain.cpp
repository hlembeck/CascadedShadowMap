#include "ChunkedTerrain.h"
#include "DescriptorHeaps.h"

TerrainManager::TerrainManager(const TCHAR heightmapFilename[], const TCHAR lookupFilename[]) : m_currBlocks(new BlockLookup[9]) {
	m_hHeightmapFile = CreateFile(heightmapFilename, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	m_hLookupFile = CreateFile(lookupFilename, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
}

const BOOL TerrainManager::IsOpen() { 
	if (m_hHeightmapFile == INVALID_HANDLE_VALUE || m_hLookupFile == INVALID_HANDLE_VALUE)
		return FALSE;
	return TRUE;
}

void TerrainManager::ReplaceBlock(XMINT2 oldCoords, XMINT2 newCoords) {

}

BOOL TerrainManager::SearchLookupFile(XMINT2 pos, UINT64& offset) {
	return FALSE;
}

void TerrainManager::CalcBlocks(CameraView view) {
	//XMINT2 block = {(INT)position.x/4096-1,(INT)position.y/4096-1}; //position lies in some 128x128 chunk, and in some 32*128,32*128 block.
	//BlockLookup* newBlocks = new BlockLookup[9];
	//UINT64 offset;
	//DWORD bytesRead = 0;
	//BOOL isGenerated = SearchLookupFile(block, offset);
	//LONG highOffset = HIWORD(offset);
	//SetFilePointer(m_hLookupFile, LOWORD(offset), &highOffset, FILE_BEGIN);
	//ReadFile(m_hLookupFile, newBlocks[0].lookupTable, 4096, &bytesRead, NULL);

	//memcpy(m_currBlocks, newBlocks, sizeof(BlockLookup) * 9);
	//delete[] newBlocks;
}

ChunkGenParams Chunk::Create(float x, float y, ID3D12Device* device) {
	m_root.m_worldMatrix = XMMatrixTranspose(XMMatrixTranslation(x,0.0f,y));
	//m_root.bounds = BoundingBox({ x + 63.0f,0.0f,y + 63.0f }, { 63.0f,TERRAINHEIGHTMAX,63.0f }); //Hopefully correct, must test before using in practice.
	XMFLOAT4 src = {x,-TERRAINHEIGHTMAX,y,1.0f};
	XMVECTOR p1 = XMLoadFloat4(&src);
	src = { x + 126.0f,TERRAINHEIGHTMAX,y + 126.0f,1.0f };
	XMVECTOR p2 = XMLoadFloat4(&src);
	BoundingBox::CreateFromPoints(m_root.bounds, p1, p2);

	//Create only first level of the tree for testing.
	D3D12_RESOURCE_DESC resourceDesc = {
		D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		0,
		127,
		127,
		1,
		1,
		DXGI_FORMAT_R32G32B32A32_FLOAT,
		{1,0},
		D3D12_TEXTURE_LAYOUT_UNKNOWN,
		D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
	};
	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_CLEAR_VALUE clearVal = { DXGI_FORMAT_R32G32B32A32_FLOAT };
	ThrowIfFailed(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, &clearVal, IID_PPV_ARGS(&m_heightmapTreeLevels[0])));

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

	HandlePair handles = DescriptorHeaps::BatchHandles(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	device->CreateShaderResourceView(m_heightmapTreeLevels[0].Get(), &srvDesc, handles.cpuHandle);
	m_srvHandles[0] = handles.gpuHandle;
	m_rtvHandles[0] = DescriptorHeaps::BatchHandles(D3D12_DESCRIPTOR_HEAP_TYPE_RTV).cpuHandle;
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {
		DXGI_FORMAT_R32G32B32A32_FLOAT,
		D3D12_RTV_DIMENSION_TEXTURE2D
	};
	rtvDesc.Texture2D = {
		0,0
	};
	device->CreateRenderTargetView(m_heightmapTreeLevels[0].Get(),&rtvDesc,m_rtvHandles[0]);

	return { XMMatrixTranspose(XMMatrixTranslation(x, y, 0.0f)), m_rtvHandles[0], m_heightmapTreeLevels[0].Get() };
}

D3D12_GPU_DESCRIPTOR_HANDLE Chunk::GetSRVHandle() { return m_srvHandles[0]; }

XMMATRIX* Chunk::GetWorldMatrix() { return &m_root.m_worldMatrix; };

BOOL Chunk::IsVisible(FrustumRays rays) {
	return m_root.bounds.Intersects(rays.o,rays.v1,rays.d) || m_root.bounds.Intersects(rays.o, rays.v2, rays.d) || m_root.bounds.Intersects(rays.o, rays.v3, rays.d) || m_root.bounds.Intersects(rays.o, rays.v4, rays.d);
}

BOOL Chunk::IsVisible(BoundingFrustum frustum, XMMATRIX viewMatrix) {
	BoundingBox temp;
	m_root.bounds.Transform(temp,viewMatrix);
	return temp.Intersects(frustum);
}

void ChunkedTerrain::LoadVertexBuffer() {
	D3D12_RESOURCE_DESC desc = {
		D3D12_RESOURCE_DIMENSION_BUFFER,
		0,
		762048, //Each square contains six vertices, and each vertex uses 8 bytes (float2 format)
		1,
		1,
		1,
		DXGI_FORMAT_UNKNOWN,
		{1,0},
		D3D12_TEXTURE_LAYOUT_ROW_MAJOR
	};

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);

	ThrowIfFailed(m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&m_vertexBuffer)));

	XMFLOAT2* vertices = new XMFLOAT2[95256];
	for (INT i = 0; i < 126; i++) {
		for (INT j = 0; j < 126; j++) {
			vertices[6 * (i * 126 + j)] = { (float)i,(float)j };
			vertices[6 * (i * 126 + j) + 1] = { (float)i,(float)j + 1 };
			vertices[6 * (i * 126 + j) + 2] = { (float)i + (float)1,(float)j + 1 };

			vertices[6 * (i * 126 + j) + 3] = { (float)i,(float)j };
			vertices[6 * (i * 126 + j) + 4] = { (float)i + 1,(float)j + 1 };
			vertices[6 * (i * 126 + j) + 5] = { (float)i + 1,(float)j };
		}
	}
	BYTE* pData;
	D3D12_RANGE readRange = {};
	m_vertexBuffer->Map(0, &readRange, (void**)&pData);
	memcpy(pData, vertices, 762048);
	m_vertexBuffer->Unmap(0, nullptr);

	delete[] vertices;

	m_vertexBufferView = {
		m_vertexBuffer->GetGPUVirtualAddress(),
		762048,
		8
	};
}

void ChunkedTerrain::Init(CameraView view) {
	if (TerrainManager::IsOpen()) {
		printf("File error\n");
		throw std::exception();
	}
	LoadVertexBuffer();
	//CreateRoots();
	HeightmapGen::Init();
	//TerrainManager::GenerateBlock(m_rootWorldMatrix, &m_root, { 0,0 });
	CreateChunks(view);
	CreateConstantBuffer(); //Must be called after m_nChunks is set.
	MapWorldMatrices();
}

ChunkedTerrain::ChunkedTerrain() {}
ChunkedTerrain::~ChunkedTerrain() {
	if(m_chunks)
		delete[] m_chunks;
}

void ChunkedTerrain::CreateChunks(CameraView view) {

	/*
	
	The length-edges of the view frustum are used to calculate the box of chunks handled by the game. This allows the camera to rotate without necessarily updating the chunks.

	The chunks used form a square, and ideally the camera lies in the center (this doesn't occur often, as the chunks are world-bound to a grid of spacing 126. Thus we initially take the square, centered at the origin, of side-lengths equal to the 126-padded view frustum's length-edges. 

	*/

	float opp = tan(view.m_fovY / 2) * view.m_farZ;

	float lengthEdge = opp*sqrt(1.0f+view.m_aspectRatio*view.m_aspectRatio);

	INT chunkWidth = ceil(lengthEdge/126.0f); //Number of chunks necessary to span the length-edge
	m_nChunks = 4 * pow(chunkWidth, 2);
	m_chunks = new Chunk[m_nChunks];

	Chunk* curr;
	for (float i = 0; i < 2.0f*chunkWidth; i+=1.0f) {
		for (float j = 0; j < 2.0f*chunkWidth; j+=1.0f) {
			curr = &m_chunks[(int)i * 2 * chunkWidth + (int)j];
			HeightmapGen::GenerateChunk(curr->Create(( - chunkWidth + i) * 126.0f, (-chunkWidth + j) * 126.0f, m_device.Get()));
		}
	}
}

void ChunkedTerrain::MapWorldMatrices() {
	BYTE* pData;
	D3D12_RANGE readRange = {};
	m_worldCoordsCBuffer->Map(0, &readRange, (void**)&pData);
	for (UINT i = 0; i < m_nChunks; i++) {
		memcpy(pData + i*sizeof(XMMATRIX), m_chunks[i].GetWorldMatrix(), sizeof(XMMATRIX));
	}
	m_worldCoordsCBuffer->Unmap(0, nullptr);
}

void ChunkedTerrain::CreateConstantBuffer() {
	UINT paddedSize = Pad256(sizeof(XMMATRIX) * m_nChunks);
	//World Coords Constant Buffer
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
	ThrowIfFailed(m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_worldCoordsCBuffer)));
	
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {
		m_worldCoordsCBuffer->GetGPUVirtualAddress(),
		paddedSize
	};

	HandlePair handles = DescriptorHeaps::BatchHandles(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	m_cbvHandle = handles.gpuHandle;
	m_device->CreateConstantBufferView(&cbvDesc, handles.cpuHandle);
}

void ChunkedTerrain::RenderTerrain(BoundingFrustum frustum, XMMATRIX viewMatrix, ID3D12GraphicsCommandList* commandList) {
	commandList->SetGraphicsRootDescriptorTable(1, m_cbvHandle);
	for (UINT i = 0; i < m_nChunks; i++) {
		if (m_chunks[i].IsVisible(frustum, viewMatrix)) {
			commandList->SetGraphicsRoot32BitConstant(4, i, 0);
			commandList->SetGraphicsRootDescriptorTable(3, m_chunks[i].GetSRVHandle());
			commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
			commandList->DrawInstanced(95256, 1, 0, 0);
		}
	}
}

void ChunkedTerrain::DrawBarebones(ID3D12GraphicsCommandList* commandList) {
	commandList->SetPipelineState(PipelineObjects::m_shadowTerrainPSO.Get());
	commandList->SetGraphicsRootDescriptorTable(1, m_cbvHandle);
	for (UINT i = 0; i < m_nChunks; i++) {
		commandList->SetGraphicsRoot32BitConstant(4, i, 0);
		commandList->SetGraphicsRootDescriptorTable(3, m_chunks[i].GetSRVHandle());
		commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
		commandList->DrawInstanced(95256, 1, 0, 0);
	}
}