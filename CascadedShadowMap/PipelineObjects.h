#pragma once
#include "DXBase.h"
#include "RootSignatures.h"

class PipelineObjects : public virtual RootSignatures {
	//void LoadSimplePSO(const D3D12_INPUT_LAYOUT_DESC& layoutDesc);
	//void LoadShadowPSO(const D3D12_INPUT_LAYOUT_DESC& layoutDesc);
	//void LoadShadowTerrainPSO();
	//void LoadBlockTerrainGenPSO();
	//void LoadChunkTerrainRenderPSO();
	//void LoadDCMainPSO();
	//void LoadDCSampleGenPSO();
	void LoadPlanetRenderPSO();
	void LoadPlanetTilerPSO();
	void LoadIntersectionPSO();
protected:
	//ComPtr<ID3D12PipelineState> m_simplePSO; //No textures, no shadow maps, one const static light in shader. Testing local illumination.

	//ComPtr<ID3D12PipelineState> m_shadowPSO;
	//ComPtr<ID3D12PipelineState> m_shadowTerrainPSO;
	//ComPtr<ID3D12PipelineState> m_blockTerrainGen;
	//ComPtr<ID3D12PipelineState> m_chunkTerrainRender;
	//ComPtr<ID3D12PipelineState> m_dcMainPSO;
	//ComPtr<ID3D12PipelineState> m_dcSampleGenPSO;
	ComPtr<ID3D12PipelineState> m_planetRenderPSO;
	ComPtr<ID3D12PipelineState> m_planetTilerPSO;
	ComPtr<ID3D12PipelineState> m_intersectionPSO;
	void OnInit();
};