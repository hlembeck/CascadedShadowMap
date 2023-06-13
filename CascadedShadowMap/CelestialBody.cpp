#include "CelestialBody.h"
#include "DescriptorHeaps.h"

float GetWeight(BoundingBox aabb, XMFLOAT3 camPos) {
	static const float sqrt2 = sqrt(2);
	camPos.x -= aabb.Center.x;
	camPos.y -= aabb.Center.y;
	camPos.z -= aabb.Center.z;
	float d = camPos.x * camPos.x + camPos.y * camPos.y + camPos.z * camPos.z;
	d = sqrt(d);
	float r = sqrt2 * aabb.Extents.x;
	return asin(r / d);
}

BOOL NodeCmp(NodeTile a, NodeTile b) {
	return a.weight > b.weight;
}

XMMATRIX NodeTile::GetWorldMatrix() {
	//printf("pos: %g %g %g\n", aabb.Center.x, aabb.Center.y, aabb.Center.z);
	static const float scale = 2.0f / 17.0f; //Maps {0,...,16} -> {0,...,16/17} -> {0,...,32/17}, so that extents are [0,2]. Scale by aabb.Extents then maps to [0,2*aabb.Extents] = [0,aabb.Sidelengths]
	return XMMatrixTranspose(XMMatrixMultiply(XMMatrixScaling(scale * aabb.Extents.x,scale * aabb.Extents.y,scale * aabb.Extents.z), XMMatrixTranslation(aabb.Center.x-aabb.Extents.x, aabb.Center.y-aabb.Extents.y, aabb.Center.z-aabb.Extents.z)));
}

TreeNode::TreeNode(BoundingBox bounds, CelestialBody* pBody, TreeNode* par, float parWeight, XMFLOAT3 camPos) : body(pBody), parentWeight(parWeight), parent(par) {
	float width = bounds.Extents.x * 0.5f;
	//printf("%g\n", width);
	BoundingBox currBounds({bounds.Center.x-width,bounds.Center.y-width,bounds.Center.z-width}, {width,width,width});
	if (pBody->Intersects(currBounds)) {
		tiles[n++] = { currBounds,GetWeight(currBounds, camPos) };
	}
	currBounds = BoundingBox({ bounds.Center.x + width,bounds.Center.y - width,bounds.Center.z - width }, { width,width,width });
	if (pBody->Intersects(currBounds)) {
		tiles[n++] = { currBounds,GetWeight(currBounds, camPos) };
	}
	currBounds = BoundingBox({ bounds.Center.x - width,bounds.Center.y + width,bounds.Center.z - width }, { width,width,width });
	if (pBody->Intersects(currBounds)) {
		tiles[n++] = { currBounds,GetWeight(currBounds, camPos) };
	}
	currBounds = BoundingBox({ bounds.Center.x - width,bounds.Center.y - width,bounds.Center.z + width }, { width,width,width });
	if (pBody->Intersects(currBounds)) {
		tiles[n++] = { currBounds,GetWeight(currBounds, camPos) };
	}
	currBounds = BoundingBox({ bounds.Center.x + width,bounds.Center.y + width,bounds.Center.z - width }, { width,width,width });
	if (pBody->Intersects(currBounds)) {
		tiles[n++] = { currBounds,GetWeight(currBounds, camPos) };
	}
	currBounds = BoundingBox({ bounds.Center.x + width,bounds.Center.y - width,bounds.Center.z + width }, { width,width,width });
	if (pBody->Intersects(currBounds)) {
		tiles[n++] = { currBounds,GetWeight(currBounds, camPos) };
	}
	currBounds = BoundingBox({ bounds.Center.x - width,bounds.Center.y + width,bounds.Center.z + width }, { width,width,width });
	if (pBody->Intersects(currBounds)) {
		tiles[n++] = { currBounds,GetWeight(currBounds, camPos) };
	}
	currBounds = BoundingBox({ bounds.Center.x + width,bounds.Center.y + width,bounds.Center.z + width }, { width,width,width });
	if (pBody->Intersects(currBounds)) {
		tiles[n++] = { currBounds,GetWeight(currBounds, camPos) };
	}
	std::sort(tiles, tiles + n, NodeCmp);
}

TreeNode::TreeNode(BoundingSphere bounds, CelestialBody* pBody, XMFLOAT3 camPos) : body(pBody), n(1) {
	BoundingBox box(bounds.Center, { bounds.Radius + 1.0f,bounds.Radius + 1.0f,bounds.Radius + 1.0f });
	tiles[0] = { box,GetWeight(box,camPos) };
}

void TreeNode::DecreaseCount() {
	tiles[0] = tiles[--n]; //Guaranteed to be called only when n>0.
	std::sort(tiles, tiles + n, NodeCmp);
}

void TreeNode::IncreaseCount(BoundingBox bounds) {
	for (UINT i = n; i < 8; i++) {
		if (tiles[i].aabb.Contains(bounds)) {
			n++;
			std::sort(tiles, tiles + n, NodeCmp);
			return;
		}
	}
}

void CelestialBody::CreateTileMap(ID3D12Device* device) {
	D3D12_RESOURCE_DESC desc = {
			D3D12_RESOURCE_DIMENSION_TEXTURE3D,
			0,
			2048,
			2048,
			2048,
			7,
			DXGI_FORMAT_R32G32B32A32_FLOAT,
			{1,0},
			D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE
	};
	ThrowIfFailed(device->CreateReservedResource(&desc, D3D12_RESOURCE_STATE_COPY_DEST, NULL, IID_PPV_ARGS(&m_tileMap)));
	HandlePair handles = DescriptorHeaps::BatchHandles(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_randTextureSRV = handles.gpuHandle;
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {
		DXGI_FORMAT_R32G32B32_FLOAT,
		D3D12_SRV_DIMENSION_TEXTURE3D,
		D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING
	};
	srvDesc.Texture3D = {
		10,
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
			Pad256(rowSize) * 16,
			1,
			1,
			1,
			DXGI_FORMAT_UNKNOWN,
			{1,0},
			D3D12_TEXTURE_LAYOUT_ROW_MAJOR
	};

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
	ThrowIfFailed(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&uploadBuffer)));
}

void CelestialBody::GetTileMapTiling(ID3D12Device* device) {
	UINT numTilesForResource = 0;
	D3D12_PACKED_MIP_INFO packedMipDesc = {};
	D3D12_TILE_SHAPE standardTileShape = {};
	UINT numSubresourceTilings = 7;
	D3D12_SUBRESOURCE_TILING pSubresourceTilingsForNonPackedMips[7];
	device->GetResourceTiling(m_tileMap.Get(), &numTilesForResource, &packedMipDesc, &standardTileShape, &numSubresourceTilings, 0, pSubresourceTilingsForNonPackedMips);
	printf("Num subresource tilings retreived: %d\n", numSubresourceTilings);
	printf("Standard tile shape: %d %d %d\n", standardTileShape.WidthInTexels, standardTileShape.HeightInTexels, standardTileShape.DepthInTexels);
	printf("Num packed : %d\nNum tiles for packed: %d\n", packedMipDesc.NumPackedMips, packedMipDesc.NumTilesForPackedMips);
}

TreeNode* CelestialBody::GetRoot() {
	return m_root;
}

BOOL CelestialBody::Intersects(BoundingBox aabb) {
	return aabb.Intersects(m_outerSphere) && (m_innerSphere.Contains(aabb)-2);
}