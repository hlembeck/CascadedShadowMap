#pragma once
#include "DescriptorHeaps.h"
#include "PipelineObjects.h"
#include "Scene.h"
#include "Player.h"
#include "CelestialManager.h"

class Game :
	public DXWindowBase,
	private Scene,
	private Player,
	private Time, //Disconnected from Game to allow other classes access to it.
	private CelestialManager
{

	//Command list & allocator  --  may detach from this class in the future.
	ComPtr<ID3D12CommandAllocator> m_commandAllocator;
	ComPtr<ID3D12GraphicsCommandList> m_commandList;

	//Fence for DrawFinal. May not be necessary.
	ComPtr<ID3D12Fence> m_fence;
	UINT64 m_fenceValue;
	HANDLE m_fenceEvent;
	//Methods for cmdlist
	void CreateCommandList();
	void Wait();
	void ResetCommandList(ID3D12PipelineState* pso);

	BOOL m_inputCaptured;
	POINT m_cursorPos;
	float m_mouseScaleFactor; //Sets angular velocity, will use fov to do this later
	HCURSOR m_hCursor;

	void DrawFinal(); //Draw final image, to back buffer.
	TerrainLODViewParams GetTerrainLODViewParams();
public:
	Game(UINT width, UINT height);
	~Game();
	void OnInit() final;
	void OnUpdate() final;
	void OnRender() final;
	void OnDestroy() final;
	void OnKeyDown(WPARAM wParam) final;
	void OnKeyUp(WPARAM wParam) final;
	void OnScroll(WPARAM wParam) final;
	void SetCursorPoint(POINT p, HCURSOR hCursor) final;
	void OnResize(const UINT width, const UINT height) final;
};