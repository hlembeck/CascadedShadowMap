#include "CelestialManager.h"
#include "DescriptorHeaps.h"

TileManager::TileManager() : m_nodes(new TreeNode*[2048]) {}

TileManager::~TileManager() {
	delete[] m_freeTiles;

	std::vector<TreeNode*> deletedNodes;

	TreeNode* temp;
	while (m_heapSize) {
		/*printf("\n\nHeap:\n");
		for (UINT i = 0; i < m_heapSize; i++) {
			printf(" %p\n", m_nodes[i]);
		}
		printf("Deleted:\n");
		for (UINT i = 0; i < deletedNodes.size(); i++) {
			printf(" %p\n", deletedNodes[i]);
		}*/

		temp = m_nodes[--m_heapSize];
		m_nodes[m_heapSize] = nullptr;
		if (temp==nullptr)
			continue;
		{
			//printf("Considering %p\n", temp);
			UINT i = 0;
			UINT j = 0;
			for (UINT k = 0; k < deletedNodes.size(); k++) {
				if (deletedNodes[i] != temp) {
					//printf("test\n");
					i++;
				}
				if (deletedNodes[j] != temp->parent)
					j++;
			}

			if (i == deletedNodes.size()) {
				if (temp->parent && j == deletedNodes.size()) {
					//printf("Pushing %p\n", temp->parent);
					m_nodes[m_heapSize++] = temp->parent;
				}
				deletedNodes.push_back(temp);
				//printf("Deleting %p\n", temp);
				delete temp;
			}
		}
	}
	delete[] m_nodes;
}

void TileManager::Swap(UINT i, UINT j) {
	TreeNode* temp = m_nodes[i];
	m_nodes[i] = m_nodes[j];
	m_nodes[j] = temp;
}

//No need to check whether m_nodes has space, since checking for space in the heap for the >=1 tiles present in the node is done beforehand. Since the size of m_nodes is not greater than the size of m_freeHeapTiles (at least one tile per node), it follows that any node that can be put into the heap can be pushed to the m_nodes queue.
void TileManager::Push(TreeNode* node) {
	for (UINT i = 0; i < node->n; i++) {
		node->tiles[i].tileCoords = PopTile();
	}
	m_nodes[m_heapSize] = node;
	float weight = m_nodes[m_heapSize]->tiles[0].weight;
	UINT i = m_heapSize++;
	UINT j = (i-1)>>1;
	while (i && m_nodes[j]->tiles[0].weight < weight) { //Nodes in the heap portion of m_nodes are guaranteed to have at least one tile occupied.
		Swap(i, j);
		i = j;
		j = (j - 1) >> 2;
	}
}

//Does not delete the tile because the octree structure is preserved and the tile is deleted in the destructor of this class
void TileManager::Pop() {
	m_nodes[0] = m_nodes[--m_heapSize];

	UINT temp = 0;
	UINT max;
	while (2 * temp + 2 < m_heapSize) {
		max = m_nodes[2 * temp + 1]->tiles[0].weight > m_nodes[2 * temp + 2]->tiles[0].weight ? 2*temp+1 : 2*temp+2;
		if (m_nodes[temp]->tiles[0].weight < m_nodes[max]->tiles[0].weight) {
			Swap(temp, max);
			temp = max;
		}
		else
			return;
	}
}

XMUINT4 TileManager::PopTile() {
	return m_freeTiles[--m_freeTilesEnd];
}

void TileManager::PushTile(XMUINT4 tile) {
	m_freeTiles[m_freeTilesEnd++] = tile;
}

void TileManager::CreateTextureAndCBuffer() {
	CommandListAllocatorPair::Init();

	D3D12_RESOURCE_DESC desc = {
			D3D12_RESOURCE_DIMENSION_TEXTURE3D,
			0,
			256,
			256,
			256,
			1,
			DXGI_FORMAT_R32G32B32A32_FLOAT,
			{1,0},
			D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE,
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
	};

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
	ThrowIfFailed(m_device->CreateCommittedResource(&heapProps,D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, NULL, IID_PPV_ARGS(&m_tiles)));

	//CBUFFER
	desc = {
		D3D12_RESOURCE_DIMENSION_BUFFER,
		0,
		64 * sizeof(WorldTile),
		1,
		1,
		1,
		DXGI_FORMAT_UNKNOWN,
		{1,0},
		D3D12_TEXTURE_LAYOUT_ROW_MAJOR
	};
	heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	ThrowIfFailed(m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&m_cbuffer)));

	//UAV for compute shader generation
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {
		DXGI_FORMAT_R32G32B32A32_FLOAT,
		D3D12_UAV_DIMENSION_TEXTURE3D
	};
	uavDesc.Texture3D = {
		0,
		0,
		(UINT)-1
	};
	HandlePair handles = DescriptorHeaps::BatchHandles(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_uavHandle = handles.gpuHandle;
	m_device->CreateUnorderedAccessView(m_tiles.Get(), nullptr, &uavDesc, handles.cpuHandle);

	//CBV for WorldTiles
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {
		m_cbuffer->GetGPUVirtualAddress(),
		64 * (sizeof(XMMATRIX) + sizeof(XMUINT4))
	};
	handles = DescriptorHeaps::BatchHandles(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_cbvHandle = handles.gpuHandle;
	m_device->CreateConstantBufferView(&cbvDesc, handles.cpuHandle);

	//SRV for rendering. Note that created after CBV, as is needed for binding both in one table in root parameter 2 of gRootSignature
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {
		DXGI_FORMAT_R32G32B32A32_FLOAT,
		D3D12_SRV_DIMENSION_TEXTURE3D,
		D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING
	};
	srvDesc.Texture3D = {
		0,
		1,
		0.0f
	};
	handles = DescriptorHeaps::BatchHandles(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_srvHandle = handles.gpuHandle;
	m_device->CreateShaderResourceView(m_tiles.Get(), &srvDesc, handles.cpuHandle);
	

	//Create free tile arrays
	m_freeTiles = new XMUINT4[4096];
	for (UINT i = 0; i < 16; i++) {
		for (UINT j = 0; j < 16; j++) {
			for (UINT k = 0; k < 16; k++) {
				m_freeTiles[(i * 16 + j) * 16 + k] = { 16*i,16*j,16*k,0 }; //Tiles positioned in initial segment w.r.t lexicographic order
			}
		}
	}

}

void TileManager::Update(XMFLOAT3 camPos) {

}

//Possibly suboptimal algorithm for filling tiles. Returns when there is not enough room to store children of (first of) the most visible nodes. Could instead continue to the first tile that can be split into children, or until there is no room for any tile's children.
void TileManager::InitTiles(std::vector<WorldTile>& worldTiles) {
	TreeNode* temp = m_nodes[0];
	//printf("Root: %p\n", temp);
	TreeNode* newNode;

	UINT test = 0;

	while (m_freeTilesEnd>0 && test < 1) {
		newNode = new TreeNode(temp->tiles[0].aabb, temp->body, temp, temp->tiles[0].weight);
		if (newNode->n <= m_freeTilesEnd) {
			temp->DecreaseCount();
			if (temp->n == 0) {
				Pop();
			}
			Push(newNode);
			test++;
			//printf("Ptr: %p, Parent: %p\n", newNode, newNode->parent);
		}
		else {
			delete newNode;
			goto next;
		}
		temp = m_nodes[0];
	}
	//printf("\n\n");

	next:
	for (UINT i = 0; i < m_heapSize; i++) {
		for (UINT j = 0; j < m_nodes[i]->n; j++) {
			worldTiles.push_back({ m_nodes[i]->tiles[j].GetWorldMatrix(), m_nodes[i]->tiles[j].tileCoords });
		}
	}

}

void TileManager::PrintWeights() {
	for (UINT i = 0; i < m_heapSize; i++) {
		printf("Node %d:\n Pointer: %p\n Parent: %p\n %d Children: ", i, m_nodes[i], m_nodes[i]->parent, m_nodes[i]->n);
		for (UINT j = 0; j < m_nodes[i]->n; j++) {
			printf(" %g,", m_nodes[i]->tiles[j].weight);
		}
		printf("\n\n");
	}
}

void TileManager::PrintTiles() {
	for (UINT i = 0; i < m_heapSize; i++) {
		printf("Node %d:\n Pointer: %p\n Parent: %p\n %d Children: ", i, m_nodes[i], m_nodes[i]->parent, m_nodes[i]->n);
		for (UINT j = 0; j < m_nodes[i]->n; j++) {
			printf(" (%d, %d, %d),", m_nodes[i]->tiles[j].tileCoords.x, m_nodes[i]->tiles[j].tileCoords.y, m_nodes[i]->tiles[j].tileCoords.z);
		}
		printf("\n\n");
	}
}

void TileManager::GenerateTiles(const std::vector<WorldTile>& worldTiles) {
	BYTE* pData;
	D3D12_RANGE readRange;
	m_cbuffer->Map(0, &readRange, (void**)&pData);
	//printf("Num Tiles: %d\n", worldTiles.size());
	memcpy(pData, worldTiles.data(), std::min(64,(INT)worldTiles.size()) * sizeof(WorldTile));
	m_cbuffer->Unmap(0, nullptr);

	m_commandList->Reset(m_commandAllocator.Get(), PipelineObjects::m_dcSampleGenPSO.Get());
	m_commandList->SetComputeRootSignature(RootSignatures::m_cRootSignature.Get());
	ID3D12DescriptorHeap* pHeaps[] = { DescriptorHeaps::GetCBVHeap() };
	m_commandList->SetComputeRootConstantBufferView(0, m_cbuffer->GetGPUVirtualAddress());
	m_commandList->SetDescriptorHeaps(1, pHeaps);
	m_commandList->SetComputeRootDescriptorTable(2, m_uavHandle);
	//printf("%d\n", worldTiles.size());
	m_commandList->Dispatch(worldTiles.size(),2,2);

	m_commandList->Close();
	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(1, ppCommandLists);
	WaitForQueue();
}

CelestialManager::CelestialManager(UINT numBodies) : m_maxBodies(numBodies), m_celestialBodies(new CelestialBody*[numBodies]) {}

CelestialManager::~CelestialManager() {
	for (UINT i = 0; i < m_numBodies; i++) {
		delete m_celestialBodies[i];
	}
	delete[] m_celestialBodies;
}

void CelestialManager::CreateUploadBuffer() {
	D3D12_RESOURCE_DESC desc = {
			D3D12_RESOURCE_DIMENSION_BUFFER,
			0,
			384,
			1,
			1,
			1,
			DXGI_FORMAT_UNKNOWN,
			{1,0},
			D3D12_TEXTURE_LAYOUT_ROW_MAJOR
	};

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
	ThrowIfFailed(m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&m_uploadBuffer)));
}

void CelestialManager::CreateVertexBuffer() {
	XMFLOAT4* vertices = new XMFLOAT4[4096];

	for (UINT i = 0; i < 16; i++) {
		for (UINT j = 0; j < 16; j++) {
			for (UINT k = 0; k < 16; k++) {
				vertices[i + (j + k * 16) * 16] = { (float)i, (float)j, (float)k, 1.0f };
			}
		}
	}

	ComPtr<ID3D12Resource> uploadBuffer;
	D3D12_RESOURCE_DESC desc = {
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
	ThrowIfFailed(m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&uploadBuffer)));
	heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	ThrowIfFailed(m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COPY_DEST, NULL, IID_PPV_ARGS(&m_vertexBuffer)));


	BYTE* pData;
	D3D12_RANGE readRange;
	uploadBuffer->Map(0, &readRange, (void**)&pData);
	memcpy(pData, vertices, 65536);
	uploadBuffer->Unmap(0, nullptr);

	m_commandAllocator->Reset();
	m_commandList->Reset(m_commandAllocator.Get(), NULL);
	m_commandList->CopyResource(m_vertexBuffer.Get(), uploadBuffer.Get());
	CD3DX12_RESOURCE_BARRIER resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_vertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	m_commandList->ResourceBarrier(1, &resourceBarrier);
	m_commandList->Close();
	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	WaitForQueue();

	m_vertexBufferView = {
		m_vertexBuffer->GetGPUVirtualAddress(),
		65536,
		16
	};

	delete[] vertices;
}

void CelestialManager::GenerateInitialCelestialBodies() {
	//test
	m_celestialBodies[m_numBodies++] = new CelestialBody(Star, 3.0f, { 0.0f,0.0f,10.0f }, m_device.Get());
}

void CelestialManager::WaitForList() {
	ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), 1));
	if (m_fence->GetCompletedValue() < 1) {
		ThrowIfFailed(m_fence->SetEventOnCompletion(1, m_fenceEvent));
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}
	ThrowIfFailed(m_commandAllocator->Reset());
	m_fence->Signal(0);
}

void CelestialManager::Init(ID3D12Device* device) {
	TileManager::CreateTextureAndCBuffer();

	//TileGenerator::Init(32);
	//32x32x32 tiles.

	//CreateUploadBuffer();
	CreateVertexBuffer();
	m_celestialBodies[m_numBodies++] = new CelestialBody(Planet, 3.0f, {0.0f,0.0f,0.0f}, m_device.Get());

	TileManager::Push(m_celestialBodies[0]->GetRoot());
	TileManager::InitTiles(m_worldTiles);
	//With tiles generated, can now render vertex position data to them.
	TileManager::GenerateTiles(m_worldTiles);

	//Fill texture using compute shader

	//m_tileQueue.PrintTiles();
}

void CelestialManager::Render(ID3D12GraphicsCommandList* commandList) {
	commandList->SetGraphicsRootDescriptorTable(3, TileManager::m_srvHandle);
	commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	ID3D12DescriptorHeap* pHeaps[] = { DescriptorHeaps::GetCBVHeap() };
	commandList->SetGraphicsRootDescriptorTable(2, TileManager::m_cbvHandle);
	//std::vector<TileParams> tileParams = GetTilesToRender(viewParams);
	//printf("%d\n", m_vertexBufferView.SizeInBytes / m_vertexBufferView.StrideInBytes);
	commandList->DrawInstanced(m_vertexBufferView.SizeInBytes/m_vertexBufferView.StrideInBytes, m_worldTiles.size(), 0, 0);

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