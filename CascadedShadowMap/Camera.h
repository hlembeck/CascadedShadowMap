#pragma once
#include "Shared.h"

/*
Implementation based on Camera class from Frank Luna's "Introduction to 3D Game Programming with DirectX 12"
*/

class Camera :
	public virtual CameraView
{
	XMVECTOR m_position;
	XMVECTOR m_right;
	XMVECTOR m_up;

	//Matrices used per render
	XMMATRIX m_projectionMatrix;

	//Constant buffer for constants.
	ComPtr<ID3D12Resource> m_cBuffer;
	D3D12_GPU_DESCRIPTOR_HANDLE m_cBufferHandle;
public:
	Camera();
	~Camera();

	virtual void OnInit(float fovY, float aspectRatio, float nearZ, float farZ);
	void UpdateConstants();

	void LoadConstantBuffer(ID3D12Device* device);
	D3D12_GPU_DESCRIPTOR_HANDLE GetConstantsHandle();

	D3D12_GPU_VIRTUAL_ADDRESS GetConstantsAddress() const;

	void SetLens(float fovY, float ar, float zn, float zf);
	void Move(XMVECTOR dist);
	void SetPosition(XMVECTOR pos);

	void Pitch(float angle);
	void RotateWorldY(float angle);

	void UpdateView();
	XMMATRIX GetViewMatrix();
	XMMATRIX GetViewProjectionMatrix();
	XMMATRIX GetProjectionMatrix();

	XMFLOAT4 GetPosition(); //For shader, thus XMFLOAT4 necessary instead of the opaque XMVECTOR
	XMFLOAT4 GetDirection();
	//Returns direction vector, whose length is the length of the frustum (assuming nearZ=0).
	XMVECTOR GetScaledDirection();
	XMVECTOR GetPositionVECTOR();

	BoundingFrustum GetFrustum();
	XMMATRIX GetOrientation(); //Returns change-of-basis matrix from <e1,e2,e3> -> <m_right,m_up,m_direction>

	//Rays bounding the frustum
	FrustumRays GetRays();
	float GetTanFOVH();
};