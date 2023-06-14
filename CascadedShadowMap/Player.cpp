#include "Player.h"
#include "DescriptorHeaps.h"

Player::Player() : m_linearScale(2.0f), m_linearVelocity() {}

Player::~Player() {}

void Player::CreateIntersectionFlag() {
    D3D12_RESOURCE_DESC resourceDesc = {
        D3D12_RESOURCE_DIMENSION_BUFFER,
        0,
        256,
        1,
        1,
        1,
        DXGI_FORMAT_UNKNOWN,
        {1,0},
        D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
    };

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
    ThrowIfFailed(m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, NULL, IID_PPV_ARGS(&m_intersectionFlag)));

    heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    ThrowIfFailed(m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COPY_DEST, NULL, IID_PPV_ARGS(&m_intersectionReadback))); //Cannot be changed from COPY_DEST state

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {
        DXGI_FORMAT_UNKNOWN,
        D3D12_UAV_DIMENSION_BUFFER
    };
    uavDesc.Buffer = {
        0,
        1,
        16
    };
    HandlePair handles = DescriptorHeaps::BatchHandles(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_device->CreateUnorderedAccessView(m_intersectionFlag.Get(), NULL, &uavDesc, handles.cpuHandle);
    m_intersectionFlagHandle = handles.gpuHandle;
}

void Player::ApplyObjIntersectionsToPosition(D3D12_GPU_VIRTUAL_ADDRESS intersectionParamsAddress) {
    CommandListAllocatorPair::ResetPair(PipelineObjects::m_intersectionPSO.Get());
    ID3D12GraphicsCommandList* cmdList = CommandListAllocatorPair::m_commandList.Get();
    //Execute compute shader to fill m_intersectionFlag with actual position

    XMFLOAT4 pos = Camera::GetPosition();
    cmdList->SetComputeRootSignature(RootSignatures::m_cRootSignature.Get());
    //printf("pos: %g %g %g\n", pos.x, pos.y, pos.z);
    ID3D12DescriptorHeap* pHeaps[] = { DescriptorHeaps::GetCBVHeap() };
    cmdList->SetDescriptorHeaps(1, pHeaps);
    cmdList->SetComputeRoot32BitConstants(0, 4, &pos, 0);
    cmdList->SetComputeRootConstantBufferView(1, intersectionParamsAddress);
    cmdList->SetComputeRootDescriptorTable(3, m_intersectionFlagHandle);

    cmdList->Dispatch(1, 1, 1);

    CD3DX12_RESOURCE_BARRIER resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_intersectionFlag.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
    cmdList->ResourceBarrier(1, &resourceBarrier);
    cmdList->CopyResource(m_intersectionReadback.Get(), m_intersectionFlag.Get());
    resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_intersectionFlag.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    cmdList->ResourceBarrier(1, &resourceBarrier);
    CommandListAllocatorPair::ExecuteAndWait();

    BYTE* pData;
    D3D12_RANGE readRange = { 0,sizeof(XMFLOAT4) };
    m_intersectionReadback->Map(0, &readRange, (void**)&pData);
    memcpy(&pos, pData, sizeof(XMFLOAT4));
    m_intersectionReadback->Unmap(0, nullptr);
    //printf("pos: %g %g %g\n\n", pos.x, pos.y, pos.z);
    Camera::SetPosition(XMLoadFloat4(&pos));
}

void Player::OnInit(float fovY, float aspectRatio, float nearZ, float farZ) {
    Camera::OnInit(fovY, aspectRatio, nearZ, farZ);
    //CSMDirectional::Load();
    CreateIntersectionFlag();
}

CameraShaderConstants Player::GetCameraConstants() {
    return { Camera::GetViewProjectionMatrix(), Camera::GetDirection(), Camera::GetPosition() };
}

void Player::OnKeyDown(WPARAM wParam) {
    switch (wParam) {
    case 0x57: //'w' key
        m_linearVelocity.z = 5.0f;
        return;
    case 0x41: //'a' key
        m_linearVelocity.x = -5.0f;
        return;
    case 0x53: //'s' key
        m_linearVelocity.z = -5.0f;
        return;
    case 0x44: //'d' key
        m_linearVelocity.x = 5.0f;
        return;
    case VK_SPACE:
        m_linearVelocity.y = 5.0f;
        return;
    case VK_SHIFT:
        m_linearVelocity.y = -5.0f;
        return;
    }
}

void Player::OnKeyUp(WPARAM wParam) {
    switch (wParam) {
    case 0x57: //'w' key
        if (m_linearVelocity.z > 0)
            m_linearVelocity.z = 0.0f;
        return;
    case 0x41: //'a' key
        if (m_linearVelocity.x < 0)
            m_linearVelocity.x = 0.0f;
        return;
    case 0x53: //'s' key
        if (m_linearVelocity.z < 0)
            m_linearVelocity.z = 0.0f;
        return;
    case 0x44: //'d' key
        if (m_linearVelocity.x > 0)
            m_linearVelocity.x = 0.0f;
        return;
    case VK_SPACE:
        if (m_linearVelocity.y > 0)
            m_linearVelocity.y = 0.0f;
        return;
    case VK_SHIFT:
        if (m_linearVelocity.y < 0)
            m_linearVelocity.y = 0.0f;
        return;
    }
}

void Player::Move(float elapsedTime) {
    elapsedTime *= m_linearScale;
    Camera::Move({ m_linearVelocity.x * elapsedTime, m_linearVelocity.y * elapsedTime, m_linearVelocity.z * elapsedTime });
}

void Player::UpdateLinearVelocity(float v) {
    m_linearScale += v;
}