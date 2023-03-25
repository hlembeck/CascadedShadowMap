#include "DXBase.h"
#include "Application.h"
#include "DescriptorHeaps.h"

DXBase::DXBase() {}

DXBase::~DXBase() {}

void DXBase::CreateDevice(IDXGIFactory4* factory) {
    ComPtr<IDXGIAdapter1> adapter;
    GetAdapter(factory, &adapter);

    ThrowIfFailed(D3D12CreateDevice(
        adapter.Get(),
        D3D_FEATURE_LEVEL_12_0,
        IID_PPV_ARGS(&m_device)
    ));
}

void DXBase::GetAdapter(IDXGIFactory1* pFactory, IDXGIAdapter1** ppAdapter) {
    UINT i = 0;
    ComPtr<IDXGIAdapter1> adapter;
    while (pFactory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND) {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);
        //Don't use the basic render driver software adapter.
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
            continue;
        }
        *ppAdapter = adapter.Detach();
        return;
    }
    *ppAdapter = NULL;
}

WindowInterface::WindowInterface(UINT width, UINT height) : m_width(width), m_height(height) {
    m_aspectRatio = static_cast<float>(width) / static_cast<float>(height);
}

WindowInterface::~WindowInterface() {};

DXWindowBase::DXWindowBase(UINT width, UINT height) : WindowInterface(width, height) {}

DXWindowBase::~DXWindowBase() {

}

void DXWindowBase::OnInit() {
    CreateSwapChain();
}

void DXWindowBase::CreateRenderTargets() {

    m_viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(WindowInterface::m_width), static_cast<float>(WindowInterface::m_height));
    m_scissorRect = CD3DX12_RECT(0, 0, static_cast<LONG>(WindowInterface::m_width), static_cast<LONG>(WindowInterface::m_height));

    // Create a RTV for each frame.
    HandlePair handles;
    for (UINT i = 0; i < NUMFRAMES; i++)
    {
        ThrowIfFailed(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i])));
        handles = DescriptorHeaps::BatchHandles(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        m_rtvHandles[i] = handles.cpuHandle;
        m_device->CreateRenderTargetView(m_renderTargets[i].Get(), nullptr, handles.cpuHandle);
        m_renderTargets[i]->SetName(L"Render Target");
    }
}

void DXWindowBase::CreateDepthStencil() {
    D3D12_RESOURCE_DESC resourceDesc = {
        D3D12_RESOURCE_DIMENSION_TEXTURE2D,
        0,
        m_width,
        m_height,
        1,
        1,
        DXGI_FORMAT_D32_FLOAT,
        {1,0},
        D3D12_TEXTURE_LAYOUT_UNKNOWN,
        D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
    };
    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

    D3D12_CLEAR_VALUE optClear;
    optClear.Format = DXGI_FORMAT_D32_FLOAT;
    optClear.DepthStencil.Depth = 1.0f;
    m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &optClear, IID_PPV_ARGS(&m_depthBuffer));

    D3D12_DEPTH_STENCIL_VIEW_DESC desc = {
        DXGI_FORMAT_D32_FLOAT,
        D3D12_DSV_DIMENSION_TEXTURE2D
    };

    m_dsvHandle = DescriptorHeaps::BatchHandles(D3D12_DESCRIPTOR_HEAP_TYPE_DSV).cpuHandle;
    m_device->CreateDepthStencilView(m_depthBuffer.Get(), &desc, m_dsvHandle);
}

void DXWindowBase::CreateSwapChain() {
    HWND hWnd = Application::GetHWND();
    //Create Device
    ComPtr<IDXGIFactory4> factory;
    UINT dxgiFactoryFlags = 0;
    ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));
    DXBase::CreateDevice(factory.Get());

    D3D12_COMMAND_QUEUE_DESC queueDesc = { D3D12_COMMAND_LIST_TYPE_DIRECT , D3D12_COMMAND_QUEUE_FLAG_NONE };
    ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

    // Describe and create the swap chain.
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {
        m_width,
        m_height,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        FALSE,
        {1,0}, //sample desc
        DXGI_USAGE_RENDER_TARGET_OUTPUT,
        NUMFRAMES,
        DXGI_SCALING_STRETCH, //Scaling stretch
        DXGI_SWAP_EFFECT_FLIP_DISCARD,

    };

    ComPtr<IDXGISwapChain1> swapChain;
    //IDXGIFactory2 necessary for CreateSwapChainForHwnd
    ThrowIfFailed(factory->CreateSwapChainForHwnd(
        m_commandQueue.Get(),
        hWnd,
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain
    ));

    //DXGI won't respond to alt-enter - WndProc is responsible.
    ThrowIfFailed(factory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER));

    ThrowIfFailed(swapChain.As(&m_swapChain)); //Original swap chain interface now represented by m_swapChain
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}

void DXWindowBase::CreateFence() {
    ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
    m_fenceValue = 0;

    // Create an event handle to use for frame synchronization.
    m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (m_fenceEvent == nullptr) {
        ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
    }

    WaitForPreviousFrame();
}

HRESULT DXWindowBase::Present() {
    return m_swapChain->Present(1, 0);
}

void DXWindowBase::WaitForPreviousFrame() {
    m_fenceValue++;
    ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), m_fenceValue));
    if (m_fence->GetCompletedValue() < m_fenceValue) {
        ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValue, m_fenceEvent));
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}