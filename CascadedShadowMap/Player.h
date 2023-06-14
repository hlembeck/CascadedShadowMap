#pragma once
#include "Camera.h"
#include "BasicShadow.h"
#include "PipelineObjects.h"

class Player :
	public Camera,
	public virtual BasicShadow,
	public virtual PipelineObjects
{
	float m_linearScale;
	XMFLOAT3 m_linearVelocity;

	ComPtr<ID3D12Resource> m_intersectionFlag; //For testing intersection with objects. Used as RWStructuredBuffer<uint> in compute shaders, and is set nonzero whenever an intersection occurs. If nonzero on move update, then don't move, else move.
	ComPtr<ID3D12Resource> m_intersectionReadback; //readback buffer for m_intersectionFlag. Allows CPU to read data.
	D3D12_GPU_DESCRIPTOR_HANDLE m_intersectionFlagHandle;

	void CreateIntersectionFlag();
protected:
	void ApplyObjIntersectionsToPosition(D3D12_GPU_VIRTUAL_ADDRESS intersectionParamsAddress);
public:
	Player();
	~Player();

	void OnInit(float fovY, float aspectRatio, float nearZ, float farZ);
	CameraShaderConstants GetCameraConstants();
	void OnKeyDown(WPARAM wParam);
	void OnKeyUp(WPARAM wParam);

	void Move(float elapsedTime);
	void UpdateLinearVelocity(float v);
};