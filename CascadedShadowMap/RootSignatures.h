#pragma once
#include "DXBase.h"

//Inherited by Game class, so that each instance of Game will have its own RootSignatures. However, it is preferrable to have implemented as static instance, so that root signatures are maintained for the entire app, regardless of whether multiple instances of Game are generated.
class RootSignatures : public virtual DXBase {
	void LoadComputeRS();
	void LoadGraphicsRS();
protected:
	ComPtr<ID3D12RootSignature> m_gRootSignature;
	ComPtr<ID3D12RootSignature> m_cRootSignature;
	virtual void OnInit();
};