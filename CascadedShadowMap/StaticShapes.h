#pragma once
#include "Shared.h"

//Holds geometric data (positions, normals) of a cube of lenth 2, centered at the origin. It is NOT a unit cube, which has unit length
class StaticCube : 
    public virtual RenderableObject
{
    D3D12_CPU_DESCRIPTOR_HANDLE m_cbvHandle; //Handle to CBV for the instance data.
public:
    virtual ~StaticCube();
    void Load(ID3D12Device* device);
};