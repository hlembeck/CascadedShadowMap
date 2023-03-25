#include "CSM.h"
#include "DescriptorHeaps.h"

CSMDirectional::CSMDirectional() {}

CSMDirectional::CSMDirectional(const FXMVECTOR v) : m_ratio(std::pow(m_farZ / m_nearZ, 1.0f / NFRUSTA)) {
	//UpdateView(v);
}

void CSMDirectional::LoadShadowMaps(UINT resolution) {
	D3D12_RESOURCE_DESC resourceDesc = {
		D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		0,
		1920,
		1080,
		1,
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
	m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &optClear, IID_PPV_ARGS(&m_shadowMaps));

	D3D12_DEPTH_STENCIL_VIEW_DESC desc = {
		DXGI_FORMAT_D32_FLOAT,
		D3D12_DSV_DIMENSION_TEXTURE2D
	};

	m_dsvHandle = DescriptorHeaps::BatchHandles(D3D12_DESCRIPTOR_HEAP_TYPE_DSV).cpuHandle;
	m_device->CreateDepthStencilView(m_shadowMaps.Get(), &desc, m_dsvHandle);
}

void CSMDirectional::LoadSRV(UINT resolution, D3D12_CPU_DESCRIPTOR_HANDLE srvHandle) {
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {
		DXGI_FORMAT_R32_FLOAT,
		D3D12_SRV_DIMENSION_TEXTURE2D,
		D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING
	};

	srvDesc.Texture2D= {
		0,
		1,
		0,
		0.0f
	};

	m_device->CreateShaderResourceView(m_shadowMaps.Get(), &srvDesc, srvHandle);
}

void CSMDirectional::LoadProjectionsBuffer(D3D12_CPU_DESCRIPTOR_HANDLE handle) {
	D3D12_RESOURCE_DESC desc = {
		D3D12_RESOURCE_DIMENSION_BUFFER,
		0,
		256,
		1,
		1,
		1,
		DXGI_FORMAT_UNKNOWN,
		{1,0},
		D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
		D3D12_RESOURCE_FLAG_NONE
	};
	
	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);

	ThrowIfFailed(m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&m_projectionsBuffer)));

	m_projectionsBuffer->SetName(L"Projections Buffer");

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {
		m_projectionsBuffer->GetGPUVirtualAddress(),
		Pad256(sizeof(XMMATRIX) * NFRUSTA)
	};
	m_device->CreateConstantBufferView(&cbvDesc, handle);
}

void CSMDirectional::Load(const UINT resolution) {
	m_scissorRect = { 0,0,(INT)resolution,(INT)resolution };
	m_viewport = { 0.0f,0.0f,(float)resolution,(float)resolution,0.0f,1.0f };

	HandlePair handles = DescriptorHeaps::BatchHandles(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_srvHandle = handles.gpuHandle;
	LoadShadowMaps(resolution);
	LoadSRV(resolution, handles.cpuHandle);
	handles = DescriptorHeaps::BatchHandles(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_cbvHandle = handles.gpuHandle;
	LoadProjectionsBuffer(handles.cpuHandle);
}

void CSMDirectional::UpdateProjections() {
	m_ratio = std::pow(m_farZ / m_nearZ, 1.0f / NFRUSTA);

	XMMATRIX invCameraView = XMMatrixInverse(NULL, m_viewMatrix);

	//vertices of near plane
	XMVECTOR n1 = { m_viewVol.r,m_viewVol.t,m_viewVol.n };
	XMVECTOR n2 = { -m_viewVol.r,m_viewVol.t,m_viewVol.n };
	XMVECTOR n3 = { -m_viewVol.r,-m_viewVol.t,m_viewVol.n };
	XMVECTOR n4 = { m_viewVol.r,-m_viewVol.t,m_viewVol.n };

	//vectors from near plane corner to corresponding far plane corner
	XMVECTOR d1;
	XMVECTOR d2;
	XMVECTOR d3;
	XMVECTOR d4;

	invCameraView = XMMatrixMultiply(invCameraView, m_shadowViewMatrix); //Transform from camera space to shadow view space

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
			XMVectorScale(n1, m_ratio);
			XMVectorScale(n2, m_ratio);
			XMVectorScale(n3, m_ratio);
			XMVectorScale(n4, m_ratio);
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
		XMStoreFloat3(&fMin, min);
		XMStoreFloat3(&fMax, max);
		printf("%g %g\n", fMin.z, fMax.z);
		//m_projections[i] = XMMatrixTranspose(XMMatrixMultiply(m_shadowViewMatrix,XMMatrixOrthographicOffCenterLH(-40.0f, 40.0f, -40.0f, 40.0f,-40.0f,40.0f)));
		//printf("%g %g %g %g %g %g\n", fMin.x, fMax.x, fMin.y, fMax.y, fMin.z, fMax.z);
	}

	/*BYTE* pData;
	D3D12_RANGE readRange = {};
	m_projectionsBuffer->Map(0, &readRange, (void**)&pData);
	memcpy(pData, m_projections, sizeof(XMMATRIX) * NFRUSTA);
	m_projectionsBuffer->Unmap(0, nullptr);*/
}

void CSMDirectional::UpdateView(const FXMVECTOR v) {
	m_viewVol.r = tan(m_fovY * m_aspectRatio * 0.5f) * m_nearZ;
	m_viewVol.t = tan(m_fovY * 0.5f) * m_nearZ;
	m_shadowViewMatrix = XMMatrixLookToLH({ 0.0f,0.0f,0.0f,0.0f }, v, XMVector3Orthogonal(v));
	m_projections[0] = XMMatrixTranspose(XMMatrixMultiply(m_shadowViewMatrix,XMMatrixOrthographicLH(4.0f,4.0f,-2.0f,2.0f)));

	BYTE* pData;
	D3D12_RANGE readRange = {0,0};
	m_projectionsBuffer->Map(0, &readRange, (void**)&pData);
	memcpy(pData, m_projections, sizeof(XMMATRIX));
	m_projectionsBuffer->Unmap(0, nullptr);
}

void CSMDirectional::FillShadowMaps(ID3D12GraphicsCommandList* commandList, Scene* scene) {
	commandList->RSSetScissorRects(1, &m_scissorRect);
	commandList->RSSetViewports(1, &m_viewport);
	commandList->OMSetRenderTargets(0, NULL, TRUE, &m_dsvHandle); //Unsure if setting render target is necessary

	CD3DX12_RESOURCE_BARRIER resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_shadowMaps.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	commandList->ResourceBarrier(1, &resourceBarrier);

	commandList->ClearDepthStencilView(m_dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	//commandList->SetGraphicsRootDescriptorTable(2, m_srvHandle);
	//commandList->SetGraphicsRootConstantBufferView(0, m_projectionsBuffer->GetGPUVirtualAddress());
	scene->Draw(commandList);

	resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_shadowMaps.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	commandList->ResourceBarrier(1, &resourceBarrier);
}

void CSMDirectional::PrintProjections() {
	XMFLOAT4X4 m;
	for (UINT i = 0; i < NFRUSTA; i++) {
		XMStoreFloat4x4(&m, m_projections[i]);
		printf("%g %g %g %g\n%g %g %g %g\n%g %g %g %g\n%g %g %g %g\n\n", m._11,m._12,m._13,m._14, m._21, m._22, m._23, m._24, m._31, m._32, m._33, m._34, m._41, m._42, m._43, m._44);
	}
}