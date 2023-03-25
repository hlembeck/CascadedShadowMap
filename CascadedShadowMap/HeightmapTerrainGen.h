#pragma once
#include "DXBase.h"

//Generates terrain from the standard Perlin Noise method, and cannot allow things like caves due to the heightmap property.
//Can use to aid generation of more complex terrains.
class HeightmapTerrain :
	public RenderableObject,
	private virtual RandomGenerator
{
public:
	HeightmapTerrain(UINT quality = 2);
	void Load(ID3D12Device* device) override;
	void Update(XMFLOAT4 position);
private:
	void LoadTexture(ID3D12Device* device);
	void LoadVertices(ID3D12Device* device);
	void Wait();
	void ExecuteCommandList();
	UINT m_quality;
	ComPtr<ID3D12Resource> m_randomTexture;

	//Fence
	UINT64 m_fenceValue;
	HANDLE m_fenceEvent;
	ComPtr<ID3D12Fence> m_fence;

	XMFLOAT2 m_pos;
};