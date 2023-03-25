#include "StaticShapes.h"

StaticCube::~StaticCube() {}

//void StaticCube::CreateResources(ID3D12Device* device) {
//    D3D12_RESOURCE_DESC resourceDesc = {
//        D3D12_RESOURCE_DIMENSION_BUFFER,
//        0,
//        sizeof(XMMATRIX)*MAXINSTANCES,
//        1,
//        1,
//        1,
//        DXGI_FORMAT_UNKNOWN,
//        {1,0},
//        D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
//        D3D12_RESOURCE_FLAG_NONE
//    };
//
//    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
//
//    device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_instanceData));
//}

void StaticCube::Load(ID3D12Device* device) {

    XMFLOAT2 texCoords[36] = {
        //Z+
        {1.0f,1.0f},
        {0.0f,0.0f},
        {1.0f,0.0f},
        {1.0f,1.0f},
        {0.0f,1.0f},
        {0.0f,0.0f},
        //Z-
        {0.0f,0.0f},
        {0.0f,1.0f},
        {1.0f,1.0f},
        {0.0f,0.0f},
        {1.0f,1.0f},
        {1.0f,0.0f},
        //X+
        {0.0f,1.0f},
        {1.0f,1.0f},
        {0.0f,0.0f},
        {0.0f,1.0f},
        {1.0f,0.0f},
        {1.0f,1.0f},
        //X-
        {0.0f,0.0f},
        {1.0f,0.0f},
        {0.0f,1.0f},
        {0.0f,0.0f},
        {1.0f,1.0f},
        {1.0f,0.0f},
        //Y+
        {0.0f,1.0f},
        {1.0f,0.0f},
        {0.0f,0.0f},
        {0.0f,1.0f},
        {0.0f,0.0f},
        {1.0f,1.0f},
        //Y-
        {0.0f,0.0f},
        {0.0f,1.0f},
        {1.0f,1.0f},
        {0.0f,0.0f},
        {1.0f,0.0f},
        {0.0f,1.0f},
    };


    XMFLOAT4 vertices[72] = {
        //Z+
        {-1.0f,-1.0f,1.0f,1.0f}, {0.0f,0.0f,1.0f,0.0f},
        {1.0f,1.0f,1.0f,1.0f}, {0.0f,0.0f,1.0f,0.0f},
        {-1.0f,1.0f,1.0f,1.0f}, {0.0f,0.0f,1.0f,0.0f},
        {-1.0f,-1.0f,1.0f,1.0f}, {0.0f,0.0f,1.0f,0.0f},
        {1.0f,-1.0f,1.0f,1.0f}, {0.0f,0.0f,1.0f,0.0f},
        {1.0f,1.0f,1.0f,1.0f}, {0.0f,0.0f,1.0f,0.0f},
        //Z-
        {-1.0f,-1.0f,-1.0f,1.0f}, {0.0f,0.0f,-1.0f,0.0f},
        {-1.0f,1.0f,-1.0f,1.0f}, {0.0f,0.0f,-1.0f,0.0f},
        {1.0f,1.0f,-1.0f,1.0f}, {0.0f,0.0f,-1.0f,0.0f},
        {-1.0f,-1.0f,-1.0f,1.0f}, {0.0f,0.0f,-1.0f,0.0f},
        {1.0f,1.0f,-1.0f,1.0f}, {0.0f,0.0f,-1.0f,0.0f},
        {1.0f,-1.0f,-1.0f,1.0f}, {0.0f,0.0f,-1.0f,0.0f},
        //X+
        {1.0f,-1.0f,1.0f,1.0f},{1.0f,0.0f,0.0f,0.0f},
        {1.0f,1.0f,-1.0f,1.0f},{1.0f,0.0f,0.0f,0.0f},
        {1.0f,1.0f,1.0f,1.0f},{1.0f,0.0f,0.0f,0.0f},
        {1.0f,-1.0f,1.0f,1.0f},{1.0f,0.0f,0.0f,0.0f},
        {1.0f,-1.0f,-1.0f,1.0f},{1.0f,0.0f,0.0f,0.0f},
        {1.0f,1.0f,-1.0f,1.0f},{1.0f,0.0f,0.0f,0.0f},
        //X-
        {-1.0f,-1.0f,-1.0f,1.0f},{-1.0f,0.0f,0.0f,0.0f},
        {-1.0f,1.0f,1.0f,1.0f},{-1.0f,0.0f,0.0f,0.0f},
        {-1.0f,1.0f,-1.0f,1.0f},{-1.0f,0.0f,0.0f,0.0f},
        {-1.0f,-1.0f,-1.0f,1.0f},{-1.0f,0.0f,0.0f,0.0f},
        {-1.0f,-1.0f,1.0f,1.0f},{-1.0f,0.0f,0.0f,0.0f},
        {-1.0f,1.0f,1.0f,1.0f},{-1.0f,0.0f,0.0f,0.0f},
        //Y+
        {-1.0f,1.0f,-1.0f,1.0f},{0.0f,1.0f,0.0f,0.0f},
        {-1.0f,1.0f,1.0f,1.0f},{0.0f,1.0f,0.0f,0.0f},
        {1.0f,1.0f,1.0f,1.0f},{0.0f,1.0f,0.0f,0.0f},
        {-1.0f,1.0f,-1.0f,1.0f},{0.0f,1.0f,0.0f,0.0f},
        {1.0f,1.0f,1.0f,1.0f},{0.0f,1.0f,0.0f,0.0f},
        {1.0f,1.0f,-1.0f,1.0f},{0.0f,1.0f,0.0f,0.0f},
        //Y-
        {-1.0f,-1.0f,-1.0f,1.0f},{0.0f,-1.0f,0.0f,0.0f},
        {1.0f,-1.0f,1.0f,1.0f},{0.0f,-1.0f,0.0f,0.0f},
        {-1.0f,-1.0f,1.0f,1.0f},{0.0f,-1.0f,0.0f,0.0f},
        {-1.0f,-1.0f,-1.0f,1.0f},{0.0f,-1.0f,0.0f,0.0f},
        {1.0f,-1.0f,-1.0f,1.0f},{0.0f,-1.0f,0.0f,0.0f},
        {1.0f,-1.0f,1.0f,1.0f},{0.0f,-1.0f,0.0f,0.0f}
    };

    m_vertexView.nVertices = 36;

    D3D12_RESOURCE_DESC resourceDesc = {
        D3D12_RESOURCE_DIMENSION_BUFFER,
        0,
        1152,
        1,
        1,
        1,
        DXGI_FORMAT_UNKNOWN,
        {1,0},
        D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
        D3D12_RESOURCE_FLAG_NONE
    };

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);

    device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_vertices));
    
    resourceDesc.Width = 288;
    device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_texCoords));

    BYTE* pData;
    D3D12_RANGE readRange = { 0,0 };
    m_vertices->Map(0, &readRange, (void**)&pData);
    memcpy(pData, vertices, 1152);
    m_vertices->Unmap(0, nullptr);

    m_vertexView.views[0] = {
        m_vertices->GetGPUVirtualAddress(),
        1152,
        32
    };

    m_texCoords->Map(0, &readRange, (void**)&pData);
    memcpy(pData, texCoords, 288);
    m_texCoords->Unmap(0, nullptr);

    m_textured = TRUE;
    m_vertexView.views[1] = {
        m_texCoords->GetGPUVirtualAddress(),
        288,
        8
    };

    //CreateResources(device);
}

//void StaticCube::LoadRandomInstances(ID3D12Device* device, UINT numInstances, RandomGenerator* randGen, float radius) {
//    m_nInstances = std::min(numInstances,MAXINSTANCES);
//    XMMATRIX* matrices = new XMMATRIX[m_nInstances];
//    XMMATRIX tMat;
//    XMVECTOR tVec;
//    for (UINT i = 0; i < m_nInstances; i++) {
//        tVec = {randGen->Get(),randGen->Get(),randGen->Get()};
//        tMat = XMMatrixRotationAxis(tVec,XM_PI*(randGen->Get()));
//        matrices[i] = XMMatrixMultiply(tMat,XMMatrixTranslation(randGen->Get()*radius, randGen->Get() * radius, randGen->Get() * radius));
//    }
//
//    BYTE* pData;
//    D3D12_RANGE readRange = { 0,0 };
//    m_instanceData->Map(0, &readRange, (void**)&pData);
//    memcpy(pData, matrices, sizeof(XMMATRIX)*m_nInstances);
//    m_instanceData->Unmap(0, nullptr);
//
//    delete[] matrices;
//}

//void GetRenderableObject(RenderableObject& obj) {
//    BYTE* pData;
//    WORD bytesRead;
//    HANDLE hFile = CreateFile(geomFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
//
//}