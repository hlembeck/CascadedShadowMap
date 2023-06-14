#include "Planet.h"
#include "DescriptorHeaps.h"


XMMATRIX PlanetTile::GetTileMatrix() {
	return XMMatrixIdentity();
}

PlanetTile::PlanetTile(UINT sideIndex, UINT negExponent, XMUINT2 offset) {

}

PlanetTile::~PlanetTile() {}

Planet::Planet(CommandListAllocatorPair* pCmdListAllocPair, float radius, XMFLOAT3 initialPos, BoundingFrustum viewFrustum) : m_radius(radius), m_position(initialPos) {
	CreateVertexBuffer(pCmdListAllocPair);
	CreatePlanetParams(pCmdListAllocPair->GetDevice());
	CreateTileMap(pCmdListAllocPair->GetDevice());
	CreateRandTexture(pCmdListAllocPair);
	m_innerSphere = BoundingSphere(initialPos, radius - 1.0f);
	m_outerSphere = BoundingSphere(initialPos, radius);
	if (viewFrustum.Intersects(m_outerSphere)) {
		//May as well generate lowest-LOD tiles for each side (generate the entire cube, at lowest LOD)
		m_tiles.push_back(new PlanetTile(0, 0, {}));
		m_tiles.push_back(new PlanetTile(1, 0, {}));
		m_tiles.push_back(new PlanetTile(2, 0, {}));
		m_tiles.push_back(new PlanetTile(3, 0, {}));
		m_tiles.push_back(new PlanetTile(4, 0, {}));
		m_tiles.push_back(new PlanetTile(5, 0, {}));
	}
	/*m_planetParams.faceMatrices[0] = XMMatrixTranspose(XMMatrixMultiply(XMMatrixRotationX(-XM_PIDIV2), XMMatrixTranslation(0.0f, 0.0f, -1.0f)));
	m_planetParams.faceMatrices[1] = XMMatrixTranspose(XMMatrixMultiply(XMMatrixRotationX(XM_PIDIV2), XMMatrixTranslation(0.0f, 0.0f, 1.0f)));
	m_planetParams.faceMatrices[2] = XMMatrixTranspose(XMMatrixMultiply(XMMatrixRotationZ(XM_PIDIV2), XMMatrixTranslation(-1.0f, 0.0f, 0.0f)));
	m_planetParams.faceMatrices[3] = XMMatrixTranspose(XMMatrixMultiply(XMMatrixRotationZ(-XM_PIDIV2), XMMatrixTranslation(1.0f, 0.0f, 0.0f)));
	m_planetParams.faceMatrices[4] = XMMatrixTranspose(XMMatrixMultiply(XMMatrixRotationX(XM_PI), XMMatrixTranslation(0.0f, -1.0f, 0.0f)));
	m_planetParams.faceMatrices[5] = XMMatrixTranspose(XMMatrixTranslation(0.0f, 1.0f, 0.0f));*/
}

Planet::~Planet() {
	for (auto i = m_tiles.begin(); i != m_tiles.end(); i++) {
		delete *i;
	}
}

void Planet::CreateVertexBuffer(CommandListAllocatorPair* pCmdListAllocPair) {
	XMFLOAT4* vertices = new XMFLOAT4[1536];

	float scale = 0.0625f;

	UINT index = 0;
	for (UINT i = 0; i < 16; i++) {
		for (UINT j = 0; j < 16; j++) {
			vertices[index++] = { scale * (float)i, scale * (float)j, 0.0f, 1.0f };
			vertices[index++] = { scale * (float)i, scale * ((float)j + 1.0f), 0.0f, 1.0f };
			vertices[index++] = { scale * ((float)i + 1.0f), scale * ((float)j + 1.0f), 0.0f, 1.0f };

			vertices[index++] = { scale * (float)i, scale * (float)j, 0.0f,  1.0f };
			vertices[index++] = { scale * ((float)i + 1.0f), scale * ((float)j + 1.0f), 0.0f, 1.0f };
			vertices[index++] = { scale * ((float)i + 1.0f), scale * (float)j, 0.0f, 1.0f };
		}
	}

	ComPtr<ID3D12Resource> uploadBuffer;
	D3D12_RESOURCE_DESC desc = {
		D3D12_RESOURCE_DIMENSION_BUFFER,
		0,
		24576,
		1,
		1,
		1,
		DXGI_FORMAT_UNKNOWN,
		{1,0},
		D3D12_TEXTURE_LAYOUT_ROW_MAJOR
	};

	ID3D12Device* device(pCmdListAllocPair->GetDevice());

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
	ThrowIfFailed(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&uploadBuffer)));
	heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	ThrowIfFailed(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COPY_DEST, NULL, IID_PPV_ARGS(&m_vertexBuffer)));


	BYTE* pData;
	D3D12_RANGE readRange;
	uploadBuffer->Map(0, &readRange, (void**)&pData);
	memcpy(pData, vertices, 24576);
	uploadBuffer->Unmap(0, nullptr);
	pCmdListAllocPair->ResetPair();
	ID3D12GraphicsCommandList* pCmdList = pCmdListAllocPair->GetCommandList();
	pCmdList->CopyResource(m_vertexBuffer.Get(), uploadBuffer.Get());
	CD3DX12_RESOURCE_BARRIER resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_vertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	pCmdList->ResourceBarrier(1, &resourceBarrier);
	pCmdListAllocPair->ExecuteAndWait();

	m_vertexBufferView = {
		m_vertexBuffer->GetGPUVirtualAddress(),
		24576,
		16
	};

	delete[] vertices;
}

void Planet::CreateRandTexture(CommandListAllocatorPair* pCmdListAllocPair) {
	ID3D12Device* device = pCmdListAllocPair->GetDevice();

	D3D12_RESOURCE_DESC desc = {
			D3D12_RESOURCE_DIMENSION_TEXTURE3D,
			0,
			16,
			16,
			16,
			1,
			DXGI_FORMAT_R32G32B32A32_FLOAT,
			{1,0},
			D3D12_TEXTURE_LAYOUT_UNKNOWN
	};
	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
	ThrowIfFailed(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COPY_DEST, NULL, IID_PPV_ARGS(&m_randTexture)));
	HandlePair handles = DescriptorHeaps::BatchHandles(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_randTextureSRV = handles.gpuHandle;
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
	device->CreateShaderResourceView(m_randTexture.Get(), &srvDesc, handles.cpuHandle);

	ComPtr<ID3D12Resource> uploadBuffer;
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT placedFootprint;
	UINT numRows;
	UINT64 rowSize;
	UINT64 totalSize;
	device->GetCopyableFootprints(
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
			65536,
			1,
			1,
			1,
			DXGI_FORMAT_UNKNOWN,
			{1,0},
			D3D12_TEXTURE_LAYOUT_ROW_MAJOR
	};

	heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	ThrowIfFailed(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&uploadBuffer)));

	XMFLOAT4* randVals = new XMFLOAT4[4096];
	float m;
	D3D12_RANGE readRange = {};
	BYTE* pData;
	UINT index;
	uploadBuffer->Map(0, &readRange, (void**)&pData);
	for (UINT i = 0; i < 16; i++) {
		for (UINT j = 0; j < 16; j++) {
			for (UINT k = 0; k < 16; k++) {
				index = (i * 16 + j) * 16 + k;
				randVals[index] = { pCmdListAllocPair->Get(),pCmdListAllocPair->Get(),pCmdListAllocPair->Get(),0.0f };
				m = sqrt(randVals[index].x * randVals[index].x + randVals[index].y * randVals[index].y + randVals[index].z * randVals[index].z);
				randVals[index].x /= m;
				randVals[index].y /= m;
				randVals[index].z /= m;
			}
		}
	}
	memcpy(pData, randVals, sizeof(XMFLOAT4) * 4096);
	uploadBuffer->Unmap(0, nullptr);
	delete[] randVals;

	D3D12_TEXTURE_COPY_LOCATION dst = {
		m_randTexture.Get(),
		D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX
	};
	dst.SubresourceIndex = 0;

	D3D12_TEXTURE_COPY_LOCATION src = {
		uploadBuffer.Get(),
		D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT
	};
	src.PlacedFootprint = placedFootprint;

	pCmdListAllocPair->ResetPair();
	ID3D12GraphicsCommandList* pCmdList = pCmdListAllocPair->GetCommandList();
	pCmdList->CopyTextureRegion(&dst,0,0,0,&src,NULL);
	CD3DX12_RESOURCE_BARRIER resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_randTexture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	pCmdList->ResourceBarrier(1, &resourceBarrier);
	pCmdListAllocPair->ExecuteAndWait();

}

void Planet::CreatePlanetParams(ID3D12Device* device) {
	m_planetParams.worldMatrix = XMMatrixTranspose(XMMatrixMultiply(XMMatrixScaling(m_radius,m_radius,m_radius),XMMatrixTranslation(m_position.x,m_position.y,m_position.z)));
	m_planetParams.orientation = XMMatrixIdentity();

	m_intersectionParams.worldMatrix = m_planetParams.worldMatrix;
	m_intersectionParams.invWorldMatrix = XMMatrixTranspose(XMMatrixInverse(NULL,XMMatrixMultiply(XMMatrixScaling(m_radius, m_radius, m_radius), XMMatrixTranslation(m_position.x, m_position.y, m_position.z))));

	D3D12_RESOURCE_DESC desc = {
		D3D12_RESOURCE_DIMENSION_BUFFER,
		0,
		Pad256(sizeof(PlanetParams)),
		1,
		1,
		1,
		DXGI_FORMAT_UNKNOWN,
		{1,0},
		D3D12_TEXTURE_LAYOUT_ROW_MAJOR
	};
	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
	ThrowIfFailed(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&m_planetParamsBuffer)));

	desc.Width = Pad256(sizeof(PlanetIntersectionParams));
	ThrowIfFailed(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&m_intersectionParamsBuffer)));

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {
		m_planetParamsBuffer->GetGPUVirtualAddress(),
		Pad256(sizeof(PlanetParams))
	};
	HandlePair handles = DescriptorHeaps::BatchHandles(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_planetParamsHandle = handles.gpuHandle;
	device->CreateConstantBufferView(&cbvDesc, handles.cpuHandle);

	BYTE* pData;
	D3D12_RANGE readRange;
	m_planetParamsBuffer->Map(0, &readRange, (void**)&pData);
	memcpy(pData, &m_planetParams, sizeof(PlanetParams));
	m_planetParamsBuffer->Unmap(0, nullptr);

	m_intersectionParamsBuffer->Map(0, &readRange, (void**)&pData);
	memcpy(pData, &m_intersectionParams, sizeof(PlanetIntersectionParams));
	m_intersectionParamsBuffer->Unmap(0, nullptr);
}

void Planet::CreateTileMap(ID3D12Device* device) {
	D3D12_RESOURCE_DESC resourceDesc = {
		D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		0,
		512,
		512,
		6,
		1,
		DXGI_FORMAT_R32G32B32A32_FLOAT,
		{1,0},
		D3D12_TEXTURE_LAYOUT_UNKNOWN,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
	};

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
	ThrowIfFailed(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, NULL, IID_PPV_ARGS(&m_tileMap)));

	HandlePair handles = DescriptorHeaps::BatchHandles(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_tileMapUAV = handles.gpuHandle;
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {
		DXGI_FORMAT_R32G32B32A32_FLOAT,
		D3D12_UAV_DIMENSION_TEXTURE2DARRAY
	};
	uavDesc.Texture2DArray = {0,0,6,0};
	device->CreateUnorderedAccessView(m_tileMap.Get(), NULL, &uavDesc, handles.cpuHandle);
	handles = DescriptorHeaps::BatchHandles(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_tileMapSRV = handles.gpuHandle;
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {
		DXGI_FORMAT_R32G32B32A32_FLOAT,
		D3D12_SRV_DIMENSION_TEXTURE2DARRAY,
		D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING
	};
	srvDesc.Texture2DArray= {
		0,
		1,
		0,
		6,
		0,
		0.0f
	};
	device->CreateShaderResourceView(m_tileMap.Get(), &srvDesc, handles.cpuHandle);
}

//float SphereIntersection(BoundingSphere sphere, XMVECTOR o, XMVECTOR v) {
//	o = o - XMLoadFloat3(&sphere.Center);
//	XMVECTOR b = 2 * XMVector3Dot(v, o);
//	float r = sphere.Radius * sphere.Radius;
//	XMVECTOR c = XMVector3Dot(o, o) - XMVECTOR({ r,r,r });
//
//	XMVECTOR t = (-b + XMVectorSqrt(b * b - 4 * c)) * 0.5f;
//	//XMVECTOR t = c;
//	XMFLOAT3 s;
//	XMStoreFloat3(&s, t);
//	return s.x;
//}

void Planet::UpdatePlanetParams(XMVECTOR camPos) {
	/*XMFLOAT4 test;
	XMStoreFloat4(&test, camPos);
	printf("(%g, %g, %g, %g)\n", test.x, test.y, test.z, test.w);*/

	const XMVECTOR e1({ 1.0f,0.0f,0.0f,0.0f });
	const XMVECTOR e2({ 0.0f,1.0f,0.0f,0.0f });
	const XMVECTOR e3({ 0.0f,0.0f,1.0f,0.0f });
	const XMVECTOR e4({ 0.0f,0.0f,0.0f,1.0f });
	const XMVECTOR z4({ 1.0f,1.0f,1.0f,0.0f });


	//XMVECTOR r({m_innerSphere.Center.x, m_innerSphere.Center.y, m_innerSphere.Center.z,0.0f});
	XMVECTOR r({});
	camPos = XMVector3Normalize(r-camPos);
	XMVECTOR u = XMVector3Normalize(z4 * XMVector3Orthogonal(camPos));
	r = XMVector3Normalize(XMVector3Cross(u,camPos));
	m_planetParams.orientation = XMMatrixTranspose(XMMATRIX(r, u, camPos, e4));
	//m_planetParams.orientation = XMMatrixLookToLH(e4, camPos, u);
	//orientation = XMMatrixTranspose(orientation);
	//orientation = XMMatrixTranspose(XMMatrixInverse(NULL, orientation));
	//orientation = XMMatrixIdentity();

	/*m_planetParams.faceMatrices[0] = XMMatrixMultiply(orientation, m_planetParams.faceMatrices[0]);
	m_planetParams.faceMatrices[1] = XMMatrixMultiply(orientation, m_planetParams.faceMatrices[1]);
	m_planetParams.faceMatrices[2] = XMMatrixMultiply(orientation, m_planetParams.faceMatrices[2]);
	m_planetParams.faceMatrices[3] = XMMatrixMultiply(orientation, m_planetParams.faceMatrices[3]);
	m_planetParams.faceMatrices[4] = XMMatrixMultiply(orientation, m_planetParams.faceMatrices[4]);
	m_planetParams.faceMatrices[5] = XMMatrixMultiply(orientation, m_planetParams.faceMatrices[5]);*/

	BYTE* pData;
	D3D12_RANGE readRange;
	m_planetParamsBuffer->Map(0, &readRange, (void**)&pData);
	memcpy(pData, &m_planetParams, sizeof(PlanetParams));
	m_planetParamsBuffer->Unmap(0, nullptr);
}

void Planet::FillRenderCmdList(XMVECTOR camPos, ID3D12GraphicsCommandList* commandList) {
	UpdatePlanetParams(camPos);
	commandList->SetGraphicsRootDescriptorTable(3, m_randTextureSRV);
	commandList->SetGraphicsRootDescriptorTable(1, m_planetParamsHandle);
	commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	commandList->DrawInstanced(m_vertexBufferView.SizeInBytes / m_vertexBufferView.StrideInBytes, 6 * 64, 0, 0);
}