#include "HeightmapTerrainGen.h"

HeightmapTerrain::HeightmapTerrain(UINT quality) : m_quality(std::max((UINT)2, quality)), m_pos({ 0.0f,0.0f }) {}

void HeightmapTerrain::Load(ID3D12Device* device) {

    ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
    m_fenceValue = 1;

    // Create an event handle to use for frame synchronization.
    m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (m_fenceEvent == nullptr) {
        ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
    }

    Wait();

    m_nVertices = 6 * (m_quality - 1) * (m_quality - 1) * 1024;
    LoadTexture();
    LoadVertices();
}

void HeightmapTerrain::LoadTexture(ID3D12Device* device) {
    D3D12_RESOURCE_DESC resourceDesc = {
        D3D12_RESOURCE_DIMENSION_TEXTURE2D,
        0,
        256,
        256,
        1,
        1,
        DXGI_FORMAT_R32G32_FLOAT,
        {1,0},
        D3D12_TEXTURE_LAYOUT_UNKNOWN,
        D3D12_RESOURCE_FLAG_NONE
    };

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

    ThrowIfFailed(device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        NULL,
        IID_PPV_ARGS(&m_randomTexture)
    ));

    { //Copy Texture

        D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
        UINT numRows;
        UINT64 rowSize, totalSize;
        device->GetCopyableFootprints(&resourceDesc, 0, 1, 0, &footprint, &numRows, &rowSize, &totalSize);

        ComPtr<ID3D12Resource> srcBuffer;
        resourceDesc = {
            D3D12_RESOURCE_DIMENSION_BUFFER,
            0,
            65536 * sizeof(XMFLOAT2),
            1,
            1,
            1,
            DXGI_FORMAT_UNKNOWN,
            {1,0},
            D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
            D3D12_RESOURCE_FLAG_NONE
        };

        heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

        ThrowIfFailed(device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            NULL,
            IID_PPV_ARGS(&srcBuffer)
        ));

        { //Copy to source buffer
            XMFLOAT2* randomVals = new XMFLOAT2[65536];
            for (UINT i = 0; i < 65536; i++) {
                randomVals[i] = { Get(),Get() };
            }

            D3D12_RANGE readRange = { 0,0 };
            char* pData;
            ThrowIfFailed(srcBuffer->Map(0, &readRange, (void**)&pData));
            memcpy(pData, randomVals, sizeof(XMFLOAT2) * 65536);
            srcBuffer->Unmap(0, nullptr);
            delete[] randomVals;
        }

        {
            D3D12_TEXTURE_COPY_LOCATION destLocation = {
            m_randomTexture.Get(),
            D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX
            };
            destLocation.SubresourceIndex = 0;

            D3D12_TEXTURE_COPY_LOCATION srcLocation = {
                srcBuffer.Get(),
                D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT
            };
            srcLocation.PlacedFootprint = footprint;

            ThrowIfFailed(m_commandAllocator->Reset());
            ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), NULL));

            m_commandList->CopyTextureRegion(&destLocation, 0, 0, 0, &srcLocation, NULL);

            CD3DX12_RESOURCE_BARRIER resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_randomTexture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            m_commandList->ResourceBarrier(1, &resourceBarrier);
            m_commandList->Close();
        }
        ExecuteCommandList();
        Wait();
    }

    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {
            DXGI_FORMAT_R32G32_FLOAT,
            D3D12_SRV_DIMENSION_TEXTURE2D,
            D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING

        };
        srvDesc.Texture2D = {
            0,
            1,
            0,
            0.0f
        };
        device->CreateShaderResourceView(m_randomTexture.Get(), &srvDesc, m_srvHeap->GetCPUDescriptorHandleForHeapStart());
    }
}

void HeightmapTerrain::Update(XMFLOAT4 position) {
    {
        ThrowIfFailed(m_commandAllocator->Reset());
        ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), m_pipelineState.Get()));
        m_commandList->SetComputeRootSignature(m_rootSignature.Get());

        ID3D12DescriptorHeap* pHeaps[] = { m_srvHeap.Get() };
        m_commandList->SetDescriptorHeaps(1, pHeaps);

        float d = 1.0f / (m_quality - 1);
        position.x = std::floor(position.x);
        position.z = std::floor(position.z);
        XMFLOAT2 pos = { position.x - m_pos.x,position.z - m_pos.y };
        m_commandList->SetComputeRoot32BitConstants(0, 2, &pos, 0);
        m_pos.x = position.x;
        m_pos.y = position.z;
        m_commandList->SetComputeRoot32BitConstants(0, 1, &d, 2);
        m_commandList->SetComputeRootDescriptorTable(1, m_srvHeap->GetGPUDescriptorHandleForHeapStart());
        m_commandList->SetComputeRootUnorderedAccessView(2, m_vertices->GetGPUVirtualAddress());

        m_commandList->Dispatch(nVertices >> 9, 1, 1);

        ThrowIfFailed(m_commandList->Close());
    }

    ExecuteCommandList();
    Wait();
}

void LoadVertexSquare(float i, float j, SimpleVertex* vertices, UINT& index, UINT quality = 2) {
    float d = 1.0f / (quality - 1);
    for (UINT k = 0; k < quality - 1; k++) {
        for (UINT l = 0; l < quality - 1; l++) {
            float x = i + k * d;
            float y = j + l * d;
            vertices[index++].position = { x, 0.0f, y, 1.0f };
            vertices[index++].position = { x, 0.0f, y + d, 1.0f };
            vertices[index++].position = { x + d, 0, y + d, 1.0f };

            vertices[index++].position = { x, 0.0f, y, 1.0f };
            vertices[index++].position = { x + d, 0.0f, y + d, 1.0f };
            vertices[index++].position = { x + d, 0.0f, y, 1.0f };
        }
    }
}

void HeightmapTerrain::LoadVertices(ID3D12Device* device) {
    D3D12_RESOURCE_DESC resourceDesc = {
        D3D12_RESOURCE_DIMENSION_BUFFER,
        0,
        m_nVertices * sizeof(SimpleVertex),
        1,
        1,
        1,
        DXGI_FORMAT_UNKNOWN,
        {1,0},
        D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
        D3D12_RESOURCE_FLAG_NONE
    };

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);

    device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        NULL,
        IID_PPV_ARGS(&m_vertices)
    );


    SimpleVertex* vertices = new SimpleVertex[m_nVertices];
    memset(vertices, 0, sizeof(SimpleVertex) * m_nVertices);
    float d = 1.0f / (m_quality - 1);
    UINT index = 0;
    for (float i = -16.0f; i < 16.0f; i++) {
        for (float j = -16.0f; j < 16.0f; j++) {
            LoadVertexSquare(i, j, vertices, index, m_quality);
        }
    }

    D3D12_RANGE readRange = { 0,0 };
    char* pData;
    ThrowIfFailed(m_vertices->Map(0, &readRange, (void**)&pData));
    memcpy(pData, vertices, sizeof(SimpleVertex) * m_nVertices);
    m_vertices->Unmap(0, nullptr);

    delete[] vertices;
    m_vertexView = { m_vertices->GetGPUVirtualAddress(), m_nVertices * (UINT)sizeof(SimpleVertex), sizeof(SimpleVertex) };
}

void HeightmapTerrain::Wait() {
    const UINT64 fence = m_fenceValue;
    ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), fence));
    m_fenceValue++;
    if (m_fence->GetCompletedValue() < fence) {
        ThrowIfFailed(m_fence->SetEventOnCompletion(fence, m_fenceEvent));
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }
}

void HeightmapTerrain::ExecuteCommandList() {
    ID3D12CommandList* ppCommandList[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(1, ppCommandList);
    Wait();
}