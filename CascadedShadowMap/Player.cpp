#include "Player.h"

Player::Player() : m_linearScale(2.0f) {}

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
    ThrowIfFailed(m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COPY_DEST, NULL, IID_PPV_ARGS(&m_intersectionReadback))); //Cnanot be changed from COPY_DEST state
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