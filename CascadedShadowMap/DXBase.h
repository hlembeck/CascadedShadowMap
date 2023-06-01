#pragma once
#include "Shared.h"

constexpr UINT NUMFRAMES = 2;

class DXBase : public RandomGenerator {
public:
	DXBase();
	~DXBase();
protected:
	void GetAdapter(IDXGIFactory1* pFactory, IDXGIAdapter1** ppAdapter);
	void CreateDevice(IDXGIFactory4* factory);
	ComPtr<ID3D12Device> m_device;
	ComPtr<ID3D12CommandQueue> m_commandQueue;
};

class WindowInterface : public virtual AspectRatio {
public:
	WindowInterface(UINT width, UINT height);
	~WindowInterface();
	virtual void OnInit() = 0;
	virtual void OnUpdate() = 0;
	virtual void OnRender() = 0;
	virtual void OnKeyDown(WPARAM wParam) = 0;
	virtual void OnKeyUp(WPARAM wParam) = 0;
	virtual void OnScroll(WPARAM wParam) = 0;
	virtual void OnDestroy() = 0;
	virtual void SetCursorPoint(POINT p, HCURSOR hCursor) = 0;
	virtual void OnResize(const UINT width, const UINT height) = 0;
protected:
	UINT m_width;
	UINT m_height;
};

class DXWindowBase :
	public virtual DXBase,
	public WindowInterface
{
public:
	DXWindowBase(UINT width, UINT height);
	virtual ~DXWindowBase();
	virtual void OnInit();
protected:
	void CreateRenderTargets();
	void CreateDepthStencil();
	void CreateSwapChain();
	void CreateFence();
	HRESULT Present();
	void WaitForPreviousFrame();
	ComPtr<ID3D12Resource> m_renderTargets[NUMFRAMES];
	D3D12_CPU_DESCRIPTOR_HANDLE m_rtvHandles[NUMFRAMES];
	UINT m_frameIndex;
	CD3DX12_RECT m_scissorRect;
	CD3DX12_VIEWPORT m_viewport;

	ComPtr<ID3D12Resource> m_depthBuffer;
	D3D12_CPU_DESCRIPTOR_HANDLE m_dsvHandle;

private:
	ComPtr<IDXGISwapChain3> m_swapChain;
	//Fence for presenting the frame.
	UINT64 m_fenceValue;
	HANDLE m_fenceEvent;
	ComPtr<ID3D12Fence> m_fence;
};

class CommandListAllocatorPair :public virtual DXBase {
	void CreateCommandList();
protected:
	ComPtr<ID3D12GraphicsCommandList> m_commandList;
	ComPtr<ID3D12CommandAllocator> m_commandAllocator;
	ComPtr<ID3D12Fence> m_fence;
	HANDLE m_fenceEvent;
	void WaitForQueue();
public:
	void Init();
};