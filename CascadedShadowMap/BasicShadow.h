#include "DXBase.h"
#include "Scene.h"

class BasicShadow : 
	public virtual DXBase,
	public virtual AspectRatio,
	private virtual CameraView 
{
private:
	float m_right; //Right of near plane
	float m_top;
	XMMATRIX m_shadowViewMatrix;

	XMMATRIX m_projections[NFRUSTA];
	ComPtr<ID3D12Resource> m_projectionsBuffer;
	ComPtr<ID3D12Resource> m_depthMap;

	void LoadProjections();
protected:
	D3D12_VIEWPORT m_shadowViewport;
	D3D12_RECT m_shadowScissorRect;
	D3D12_CPU_DESCRIPTOR_HANDLE m_dsvHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_cbvHandle;
	void Load(UINT resolution = 1024);
	void DrawShadow(ID3D12GraphicsCommandList* commandList, Scene* scene);
	void UpdateProjections();
	void Resize(); //Onresize. View volume doesn't need near and far Z, as those are in CameraView
};