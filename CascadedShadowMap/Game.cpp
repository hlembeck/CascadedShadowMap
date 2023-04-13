#include "Game.h"

constexpr float FARZ = 200.0f;

Game::Game(UINT width, UINT height) : DXWindowBase(width,height) {}

Game::~Game() {}

void Game::OnInit() {
    DXWindowBase::OnInit();
    PipelineObjects::OnInit();
    DescriptorHeaps::OnInit(m_device.Get());
    DXWindowBase::CreateRenderTargets();
    DXWindowBase::CreateDepthStencil();
    DXWindowBase::CreateFence();


    Camera::LoadConstantBuffer(m_device.Get());
    Player::OnInit(FOVY, m_aspectRatio, 0.1f, FARZ);
    BasicShadow::Load(2048);
    CreateCommandList();

    Terrain::Init(*this);

    m_commandList->Reset(m_commandAllocator.Get(), NULL);
    Scene::Load(m_commandList.Get());
    m_commandList->Close();
    Wait();
    m_time = std::chrono::high_resolution_clock::now();
}

void Game::OnUpdate() {
    std::chrono::time_point<std::chrono::high_resolution_clock> curr = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> diff = curr - m_time;
    float elapsedTime = diff.count();
    m_time = curr;
    Player::Move(elapsedTime*2.0f);
    if (m_inputCaptured) {
        POINT cursorPos = {};
        GetCursorPos(&cursorPos);
        Player::Pitch(((cursorPos.y - m_cursorPos.y) / m_mouseScaleFactor) * elapsedTime);
        Player::RotateWorldY(((cursorPos.x - m_cursorPos.x) / m_mouseScaleFactor) * elapsedTime);
        SetCursorPos(m_cursorPos.x, m_cursorPos.y);
    }
    Terrain::UpdateRoots(GetTerrainLODViewParams());
}

void Game::DrawFinal() {
    {
        CD3DX12_RESOURCE_BARRIER resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        m_commandList->ResourceBarrier(1, &resourceBarrier);

        m_commandList->OMSetRenderTargets(1, &DXWindowBase::m_rtvHandles[m_frameIndex], FALSE, &(DXWindowBase::m_dsvHandle));
        const float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
        m_commandList->ClearRenderTargetView(DXWindowBase::m_rtvHandles[m_frameIndex], clearColor, 0, nullptr);
        m_commandList->ClearDepthStencilView(DXWindowBase::m_dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);


        //Draw terrain.
        m_commandList->SetPipelineState(PipelineObjects::m_chunkTerrainRender.Get());
        m_commandList->SetGraphicsRootDescriptorTable(2, BasicShadow::m_cbvHandle);
        Terrain::RenderTiles(GetTerrainLODViewParams(), m_commandList.Get());

        //m_commandList->SetGraphicsRootDescriptorTable(3, TerrainClipmap::m_srvHandle);
        
        //Draw normal
        m_commandList->SetPipelineState(m_simplePSO.Get());
        Scene::Draw(m_commandList.Get());

        //Setup for presenting
        resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        m_commandList->ResourceBarrier(1, &resourceBarrier);
    }
}

void Game::CreateCommandList() {
    ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));
    ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), NULL, IID_PPV_ARGS(&m_commandList)));
    ThrowIfFailed(m_commandList->Close());

    ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
    m_fenceValue = 0;

    // Create an event handle to use for frame synchronization.
    m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (m_fenceEvent == nullptr) {
        ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
    }

    Wait();
}

void Game::Wait() {
    const UINT64 fence = m_fenceValue;
    ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), fence));
    m_fenceValue++;
    if (m_fence->GetCompletedValue() < fence) {
        ThrowIfFailed(m_fence->SetEventOnCompletion(fence, m_fenceEvent));
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }
}

void Game::ResetCommandList(ID3D12PipelineState* pso) {
    ThrowIfFailed(m_commandAllocator->Reset());
    ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), pso));
}

void Game::OnRender() {
    Camera::UpdateConstants();
    BasicShadow::UpdateProjections();

    ResetCommandList(PipelineObjects::m_shadowPSO.Get());
    m_commandList->SetGraphicsRootSignature(m_gRootSignature.Get());
    ID3D12DescriptorHeap* pHeaps[] = { DescriptorHeaps::GetCBVHeap() };
    m_commandList->SetDescriptorHeaps(1, pHeaps);
    BasicShadow::DrawShadow(m_commandList.Get(), this);

    m_commandList->RSSetViewports(1, &(DXWindowBase::m_viewport));
    m_commandList->RSSetScissorRects(1, &(DXWindowBase::m_scissorRect));
    m_commandList->SetGraphicsRootConstantBufferView(0, Camera::GetConstantsAddress());
    DrawFinal();
    ThrowIfFailed(m_commandList->Close());
    // Execute the command list.
    ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
    DXWindowBase::m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
    Wait();
    // Present the frame.
    Present();
    WaitForPreviousFrame();
}

void Game::OnKeyDown(WPARAM wParam) {
    if (wParam == VK_ESCAPE) {
        if (m_inputCaptured) {
            m_inputCaptured = FALSE;
            SetCursor(m_hCursor);
        }
        else {
            m_inputCaptured = TRUE;
            SetCursor(NULL);
        }
        return;
    }
    if (m_inputCaptured) {
        Player::OnKeyDown(wParam);
    }
}

void Game::OnKeyUp(WPARAM wParam) {
    if (m_inputCaptured) {
        Player::OnKeyUp(wParam);
    }
}

void Game::OnScroll(WPARAM wParam) {
    float v = (SHORT)(HIWORD(wParam)) / 600.0f;
    Player::UpdateLinearVelocity(v);
}

void Game::SetCursorPoint(POINT p, HCURSOR hCursor) {
    m_cursorPos = p;
    m_mouseScaleFactor = 20.0f;
    m_hCursor = hCursor;
    SetCursor(m_hCursor);
}

void Game::OnDestroy() {}

void Game::OnResize(const UINT width, const UINT height) {
    m_width = width;
    m_height = height;
    m_aspectRatio = (float)width / height;
    Camera::SetLens(FOVY, m_aspectRatio, 0.1f, FARZ);
    BasicShadow::Resize();
}

TerrainLODViewParams Game::GetTerrainLODViewParams() {
    BoundingFrustum frustum;
    BoundingFrustum::CreateFromMatrix(frustum, Camera::GetProjectionMatrix());
    return {
        Camera::GetPosition(),
        Camera::GetTanFOVH(),
        (float)m_width,
        frustum,
        m_viewMatrix
    };
}