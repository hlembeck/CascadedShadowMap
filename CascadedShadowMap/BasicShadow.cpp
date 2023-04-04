#include "BasicShadow.h"
#include "DescriptorHeaps.h"

void BasicShadow::LoadProjections() {
	UINT width = Pad256(sizeof(XMMATRIX) * NFRUSTA);
	D3D12_RESOURCE_DESC desc = {
		D3D12_RESOURCE_DIMENSION_BUFFER,
		0,
		width,
		1,
		1,
		1,
		DXGI_FORMAT_UNKNOWN,
		{1,0},
		D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
		D3D12_RESOURCE_FLAG_NONE
	};
	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
	m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&m_projectionsBuffer));

	
	m_shadowViewMatrix = XMMatrixLookToLH({},LIGHTDIR,{0.0f,1.0f,0.0f,0.0f}/*XMVector3Orthogonal(LIGHTDIR)*/);

	BYTE* pData;
	D3D12_RANGE readRange = {};
	m_projectionsBuffer->Map(0, &readRange, (void**)&pData);
	memcpy(pData, m_projections, sizeof(XMMATRIX) * NFRUSTA);
	m_projectionsBuffer->Unmap(0, nullptr);


	//Create CBV
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {
		m_projectionsBuffer->GetGPUVirtualAddress(),
		width
	};
	HandlePair handles = DescriptorHeaps::BatchHandles(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_device->CreateConstantBufferView(&cbvDesc, handles.cpuHandle);
	m_cbvHandle = handles.gpuHandle;
}

void BasicShadow::Load(UINT resolution) {
	LoadProjections();
	m_shadowScissorRect = {0,0,(INT)resolution,(INT)resolution };
	m_shadowViewport = { 0.0f,0.0f,(float)resolution,(float)resolution,0.0f,1.0f};

	D3D12_RESOURCE_DESC resourceDesc = {
		D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		0,
		resolution,
		resolution,
		NFRUSTA,
		1,
		DXGI_FORMAT_D32_FLOAT,
		{1,0},
		D3D12_TEXTURE_LAYOUT_UNKNOWN,
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
	};
	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

	D3D12_CLEAR_VALUE optClear;
	optClear.Format = DXGI_FORMAT_D32_FLOAT;
	optClear.DepthStencil.Depth = 1.0f;
	m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &optClear, IID_PPV_ARGS(&m_depthMap));

	m_depthMap->SetName(L"Depth Map");

	D3D12_DEPTH_STENCIL_VIEW_DESC desc = {
		DXGI_FORMAT_D32_FLOAT,
		D3D12_DSV_DIMENSION_TEXTURE2DARRAY
	};

	desc.Texture2DArray = {
		0,
		0,
		NFRUSTA
	};

	m_dsvHandle = DescriptorHeaps::BatchHandles(D3D12_DESCRIPTOR_HEAP_TYPE_DSV).cpuHandle;
	m_device->CreateDepthStencilView(m_depthMap.Get(), &desc, m_dsvHandle);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{
		DXGI_FORMAT_R32_FLOAT,
		D3D12_SRV_DIMENSION_TEXTURE2DARRAY,
		D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING
	};

	srvDesc.Texture2DArray = {
		0,
		1,
		0,
		NFRUSTA,
		0,
		0.0f
	};

	HandlePair handles = DescriptorHeaps::BatchHandles(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_device->CreateShaderResourceView(m_depthMap.Get(), &srvDesc, handles.cpuHandle);
}

void BasicShadow::DrawShadow(ID3D12GraphicsCommandList* commandList, Scene* scene) {
	commandList->RSSetViewports(1, &m_shadowViewport);
	commandList->RSSetScissorRects(1, &m_shadowScissorRect);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->SetGraphicsRootConstantBufferView(0, m_projectionsBuffer->GetGPUVirtualAddress());

	CD3DX12_RESOURCE_BARRIER resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_depthMap.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	commandList->ResourceBarrier(1, &resourceBarrier);

	commandList->OMSetRenderTargets(0, NULL, FALSE, &(BasicShadow::m_dsvHandle));
	commandList->ClearDepthStencilView(BasicShadow::m_dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	scene->DrawBarebones(commandList);

	resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_depthMap.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	commandList->ResourceBarrier(1, &resourceBarrier);
}

void BasicShadow::UpdateProjections() {
	const float ratio = std::pow(m_farZ / m_nearZ, 1.0f / NFRUSTA);

	XMMATRIX invCameraView = XMMatrixInverse(NULL, m_viewMatrix);

	//vertices of near plane
	XMVECTOR n1 = { m_right,m_top,m_nearZ,1.0f };
	XMVECTOR n2 = { -m_right,m_top,m_nearZ,1.0f };
	XMVECTOR n3 = { -m_right,-m_top,m_nearZ,1.0f };
	XMVECTOR n4 = { m_right,-m_top,m_nearZ,1.0f };

	//vectors from near plane corner to corresponding far plane corner
	XMVECTOR d1;
	XMVECTOR d2;
	XMVECTOR d3;
	XMVECTOR d4;

	XMVECTOR r = { ratio,ratio,ratio,1.0f };

	invCameraView = XMMatrixMultiply(invCameraView, m_shadowViewMatrix); //Transform from camera view space to shadow view space

	d1 = XMVector4Transform(n1, invCameraView);
	d2 = XMVector4Transform(n2, invCameraView);
	d3 = XMVector4Transform(n3, invCameraView);
	d4 = XMVector4Transform(n4, invCameraView);

	XMVECTOR min, max;
	XMFLOAT3 fMin, fMax;
	for (UINT i = 0; i < NFRUSTA; i++) {
		{ //Get bounds of box
			min = XMVectorMin(d1, d2);
			max = XMVectorMax(d1, d2);
			min = XMVectorMin(min, d3);
			max = XMVectorMax(max, d3);
			min = XMVectorMin(min, d4);
			max = XMVectorMax(max, d4);

			n1 = XMVectorMultiply(r, n1);
			n2 = XMVectorMultiply(r, n2);
			n3 = XMVectorMultiply(r, n3);
			n4 = XMVectorMultiply(r, n4);
			d1 = XMVector4Transform(n1, invCameraView);
			d2 = XMVector4Transform(n2, invCameraView);
			d3 = XMVector4Transform(n3, invCameraView);
			d4 = XMVector4Transform(n4, invCameraView);

			min = XMVectorMin(min, d1);
			max = XMVectorMax(max, d1);
			min = XMVectorMin(min, d2);
			max = XMVectorMax(max, d2);
			min = XMVectorMin(min, d3);
			max = XMVectorMax(max, d3);
			min = XMVectorMin(min, d4);
			max = XMVectorMax(max, d4);
		}
		XMStoreFloat3(&fMax, max);
		XMStoreFloat3(&fMin, min);
		m_projections[i] = XMMatrixTranspose(XMMatrixMultiply(m_shadowViewMatrix,XMMatrixOrthographicOffCenterLH(fMin.x - SHADOWBIAS, fMax.x - SHADOWBIAS, fMin.y - SHADOWBIAS, fMax.y - SHADOWBIAS, -1000.0f, fMax.z + SHADOWBIAS)));
		//printf("%g %g\n", fMin.z, fMax.z);
	}
	BYTE* pData;
	D3D12_RANGE readRange = {};
	m_projectionsBuffer->Map(0, &readRange, (void**)&pData);
	memcpy(pData, m_projections, sizeof(XMMATRIX) * NFRUSTA);
	m_projectionsBuffer->Unmap(0, nullptr);
}

void BasicShadow::Resize() {
	m_right = tan(m_fovY * m_aspectRatio * 0.5f) * m_nearZ;
	m_top = tan(m_fovY * 0.5f) * m_nearZ;
}