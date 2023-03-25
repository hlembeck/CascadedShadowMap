#include "Scene.h"
#include "StaticShapes.h"
#include "DescriptorHeaps.h"

Scene::Scene() : m_materials{ {SIMPLE} } {}

Scene::~Scene() {
	for (auto i = m_objects.begin();i!=m_objects.end(); i++) {
		delete *i;
	}
}

void LoadRandomInstances(ID3D12Device* device, ObjectInstances* pObject, UINT numInstances, RandomGenerator* pRand, float radius) {
    { //Load resource
        D3D12_RESOURCE_DESC resourceDesc = {
            D3D12_RESOURCE_DIMENSION_BUFFER,
            0,
            sizeof(XMMATRIX) * MAXINSTANCES,
            1,
            1,
            1,
            DXGI_FORMAT_UNKNOWN,
            {1,0},
            D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
            D3D12_RESOURCE_FLAG_NONE
        };

        CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
        ThrowIfFailed(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&pObject->instanceData)));
    }

	HandlePair cbvHandles;
	pObject->nInstances = std::min(numInstances,MAXINSTANCES);
    XMMATRIX* matrices = new XMMATRIX[pObject->nInstances];
    XMMATRIX tMat;
    XMVECTOR tVec;
    for (UINT i = 0; i < pObject->nInstances; i++) {
        tVec = {pRand->Get(),pRand->Get(),pRand->Get()};
        tMat = XMMatrixRotationAxis(tVec,XM_PI*(pRand->Get()));
        matrices[i] = XMMatrixTranspose(XMMatrixMultiply(tMat,XMMatrixTranslation(pRand->Get()*radius, pRand->Get() * radius, pRand->Get() * radius)));
        //matrices[i] = XMMatrixIdentity();
    }

    BYTE* pData;
    D3D12_RANGE readRange = { 0,0 };
    pObject->instanceData->Map(0, &readRange, (void**)&pData);
    memcpy(pData, matrices, sizeof(XMMATRIX)*(pObject->nInstances));
    pObject->instanceData->Unmap(0, nullptr);

	cbvHandles = DescriptorHeaps::BatchHandles(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = { pObject->instanceData->GetGPUVirtualAddress(),Pad256(sizeof(XMMATRIX) * pObject->nInstances)};

    device->CreateConstantBufferView(&cbvDesc, cbvHandles.cpuHandle);
    pObject->m_cbvHandle = cbvHandles.gpuHandle;


    delete[] matrices;
}

void Scene::Load(ID3D12GraphicsCommandList* commandList) {
	ObjectInstances objInstances;

	m_objects.push_back(new StaticCube());
	m_objects[0]->Load(m_device.Get());
	objInstances.pObject = m_objects[0];
	objInstances.nInstances = 1;
	LoadRandomInstances(m_device.Get(), &objInstances, 50, (RandomGenerator*)this, 10.0f);

	m_materials[0].objects.push_back(objInstances);
}

void Scene::Draw(ID3D12GraphicsCommandList* commandList) {
	VertexView currView;
	for (UINT i = 0; i < NMATERIALS; i++) {
		for (auto j = m_materials[i].objects.begin(); j != m_materials[i].objects.end(); j++) {
			currView = j->pObject->m_vertexView;
			commandList->SetGraphicsRootDescriptorTable(1,j->m_cbvHandle);
            //commandList->SetGraphicsRootDescriptorTable(3, m_srvHandles[0]);
			commandList->IASetVertexBuffers(0, 2, currView.views);
			commandList->DrawInstanced(currView.nVertices, j->nInstances, 0, 0);
		}
	}
}

void Scene::DrawBarebones(ID3D12GraphicsCommandList* commandList) {
    VertexView currView;
    for (UINT i = 0; i < NMATERIALS; i++) {
        for (auto j = m_materials[i].objects.begin(); j != m_materials[i].objects.end(); j++) {
            currView = j->pObject->m_vertexView;
            commandList->SetGraphicsRootDescriptorTable(1, j->m_cbvHandle);
            commandList->IASetVertexBuffers(0, 1, currView.views);
            commandList->DrawInstanced(currView.nVertices, j->nInstances, 0, 0);
        }
    }
}