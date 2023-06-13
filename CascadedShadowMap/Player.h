#pragma once
#include "Camera.h"
#include "BasicShadow.h"

class Player :
	public Camera,
	public virtual BasicShadow
{
	float m_linearScale;
	XMFLOAT3 m_position;
	XMFLOAT3 m_linearVelocity;

	ComPtr<ID3D12Resource> m_intersectionFlag; //For testing intersection with objects. Used as RWStructuredBuffer<uint> in compute shaders, and is set nonzero whenever an intersection occurs. If nonzero on move update, then don't move, else move.
	ComPtr<ID3D12Resource> m_intersectionReadback; //readback buffer for m_intersectionFlag. Allows CPU to read data.

	void CreateIntersectionFlag();
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