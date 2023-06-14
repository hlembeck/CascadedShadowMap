#include "Camera.h"
#include "DescriptorHeaps.h"

Camera::Camera() {
	m_position = { 0.0f,0.0f,-1.0f }; //Set again in OnInit
	m_right = { 1.0f,0.0f,0.0f };
	m_up = { 0.0f,1.0f,0.0f };
	m_direction = { 0.0f,0.0f,1.0f };
	UpdateView();
	SetLens(FOVY, 1.0f, 0.1f, 1.0f);
}

Camera::~Camera() {

}

void Camera::OnInit(float fovY, float aspectRatio, float nearZ, float farZ) {
	m_position = { 0.0f,0.0f,-5.0f };
	m_right = { 1.0f,0.0f,0.0f };
	m_up = { 0.0f,1.0f,0.0f };
	m_direction = { 0.0f,0.0f,1.0f };
	SetLens(fovY, aspectRatio, nearZ, farZ);
	UpdateView();
}

void Camera::LoadConstantBuffer(ID3D12Device* device) {
	HandlePair handles = DescriptorHeaps::BatchHandles(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_cBufferHandle = handles.gpuHandle;

	D3D12_RESOURCE_DESC resourceDesc = {
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
	device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&m_cBuffer));

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {
		m_cBuffer->GetGPUVirtualAddress(),
		256
	};
	device->CreateConstantBufferView(&cbvDesc, handles.cpuHandle);
}

void Camera::UpdateConstants() {
	CameraShaderConstants constants = {XMMatrixTranspose(GetViewProjectionMatrix()),GetDirection(),GetPosition()};
	BYTE* pData;
	D3D12_RANGE range(0, 0);
	m_cBuffer->Map(0, &range, (void**)&pData);
	memcpy(pData, &constants, sizeof(CameraShaderConstants));
	m_cBuffer->Unmap(0, nullptr);
}

D3D12_GPU_DESCRIPTOR_HANDLE Camera::GetConstantsHandle() { return m_cBufferHandle; }

D3D12_GPU_VIRTUAL_ADDRESS Camera::GetConstantsAddress() const {
	return m_cBuffer->GetGPUVirtualAddress();
}

void Camera::SetLens(float fovY, float aspectRatio, float nearZ, float farZ) {
	m_fovY = fovY;
	m_nearZ = nearZ;
	m_farZ = farZ;

	/*m_nearWindowHeight = 2.0f * m_nearZ * tanf(0.5f * m_fovY);
	m_farWindowHeight = 2.0f * m_farZ * tanf(0.5f * m_fovY);*/

	m_projectionMatrix = XMMatrixPerspectiveFovLH(m_fovY, aspectRatio, m_nearZ, m_farZ); //Refers to the correct matrix in Luna's book (p189)

	//Cascaded Shadow Map
	//UpdateDepth(nearZ, farZ);
	//UpdateMaps(XMMatrixInverse(nullptr, m_viewMatrix), fovY, aspectRatio);
}

void Camera::Move(XMVECTOR dist) { //Movement on y-axis is "wrong": pressing space moves in "up" direction of camera, not of world
	XMMATRIX mat(m_right, m_up, m_direction, {0.0f,0.0f,0.0f,0.0f});
	m_position += XMVector3Transform(dist, mat);
	UpdateView();
}

void Camera::SetPosition(XMVECTOR pos) { //Movement on y-axis is "wrong": pressing space moves in "up" direction of camera, not of world
	m_position = pos;
	UpdateView();
}


void Camera::Pitch(float angle) {
	XMMATRIX rotationMatrix = XMMatrixRotationAxis(m_right, angle);
	m_up = XMVector3TransformNormal(m_up, rotationMatrix);
	m_direction = XMVector3TransformNormal(m_direction, rotationMatrix);
}

void Camera::RotateWorldY(float angle) {
	XMMATRIX rotationMatrix = XMMatrixRotationY(angle);
	m_right = XMVector3TransformNormal(m_right, rotationMatrix);
	m_direction = XMVector3TransformNormal(m_direction, rotationMatrix);
}

void Camera::UpdateView() {
	m_direction = XMVector3Normalize(m_direction);
	m_up = XMVector3Normalize(XMVector3Cross(m_direction, m_right));
	m_right = XMVector3Cross(m_up, m_direction);
	m_viewMatrix = XMMatrixLookToLH(m_position, m_direction, m_up);

	//CSMDirectional::UpdateMaps(XMMatrixInverse(nullptr, m_viewMatrix));
}

XMMATRIX Camera::GetViewMatrix() {
	return m_viewMatrix;
}

XMMATRIX Camera::GetViewProjectionMatrix() {
	return XMMatrixMultiply(m_viewMatrix, m_projectionMatrix);
}

XMMATRIX Camera::GetProjectionMatrix() {
	return m_projectionMatrix;
}

XMFLOAT4 Camera::GetPosition() {
	XMFLOAT4 ret;
	XMStoreFloat4(&ret, m_position);
	ret.w = 1.0f;
	return ret;
}

XMFLOAT4 Camera::GetDirection() {
	XMFLOAT4 ret;
	XMStoreFloat4(&ret, m_direction);
	ret.w = 0.0f;
	return ret;
}

XMVECTOR Camera::GetPositionVECTOR() {
	return m_position;
}

XMVECTOR Camera::GetScaledDirection() {
	return m_direction * m_farZ;
}

BoundingFrustum Camera::GetFrustum() {
	BoundingFrustum ret(m_projectionMatrix);
	ret.Transform(ret, m_viewMatrix);
	return ret;
}

FrustumRays Camera::GetRays() {
	FrustumRays ret;
	ret.d = m_farZ;
	ret.o = m_position;
	float l = tan(m_fovY / 2);

	XMVECTOR d1 = l * m_up + l * m_aspectRatio * m_right;
	XMVECTOR d2 = l * m_up - l * m_aspectRatio * m_right;

	ret.v1 = XMVector3Normalize(m_direction + d1);
	ret.v2 = XMVector3Normalize(m_direction - d1);
	//l *= m_aspectRatio;
	ret.v3 = XMVector3Normalize(m_direction + d2);
	ret.v4 = XMVector3Normalize(m_direction - d2);
	return ret;
}

float Camera::GetTanFOVH() {
	return tan(0.5f * m_fovY) * m_aspectRatio;
}

XMMATRIX Camera::GetOrientation() {
	const XMVECTOR e4({0.0f,0.0f,0.0f,1.0f});
	XMMATRIX ret = XMMATRIX(m_right, m_up, m_direction, e4);
	//return XMMatrixInverse(NULL,ret);
	return XMMatrixIdentity();
}