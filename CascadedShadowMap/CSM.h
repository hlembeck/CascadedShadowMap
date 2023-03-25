#pragma once
#include "DXBase.h"
#include "Scene.h"

class CSMDirectional :
	public virtual DXBase,
	public virtual AspectRatio,
	private virtual CameraView
{
	ViewVolume m_viewVol;
	XMMATRIX m_shadowViewMatrix;

	D3D12_RECT m_scissorRect;
	D3D12_VIEWPORT m_viewport;
	D3D12_CPU_DESCRIPTOR_HANDLE m_dsvHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_cbvHandle;
	float m_ratio; //Const ratio of far/near planes in the camera's view. Each shadow map is based on those near/far planes.
	XMMATRIX m_projections[NFRUSTA];
	ComPtr<ID3D12Resource> m_projectionsBuffer;
	ComPtr<ID3D12Resource> m_shadowMaps; //2D textue array
	D3D12_CLEAR_VALUE m_clearVal;

	void LoadShadowMaps(UINT resolution);
	void LoadSRV(UINT resolution, D3D12_CPU_DESCRIPTOR_HANDLE srvHandle);
	void LoadProjectionsBuffer(D3D12_CPU_DESCRIPTOR_HANDLE handle);
protected:
	D3D12_GPU_DESCRIPTOR_HANDLE m_srvHandle; //Handle to start of descriptor range that is bound as a table to the main drawing pipeline. Contains a SRV descriptor for the shadow maps, as well as a CBV descriptor for the respective transforms.
public:
	CSMDirectional();
	CSMDirectional(const FXMVECTOR v);
	void Load(const UINT resolution = 1024U);
	void UpdateProjections(); //Update projection maps
	void UpdateView(const FXMVECTOR v); //Update view matrix of light. Position and orientation (except for the viewing direction) are not relevant - only the direction is relevant, as we use an orthographic projection

	void FillShadowMaps(ID3D12GraphicsCommandList* commandList, Scene* scene);

	void PrintProjections();
};