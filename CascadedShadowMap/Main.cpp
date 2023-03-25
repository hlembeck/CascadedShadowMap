#include "Main.h"

constexpr UINT initialWidth = 1920;
constexpr UINT initialHeight = 1080;

HWND Application::m_hWnd = 0;
UINT DescriptorHeaps::m_usedSize[4] = {};
UINT DescriptorHeaps::m_incrementSize[4] = {};
ComPtr<ID3D12DescriptorHeap> DescriptorHeaps::m_cbvHeap = 0;
ComPtr<ID3D12DescriptorHeap> DescriptorHeaps::m_samplerHeap = 0;
ComPtr<ID3D12DescriptorHeap> DescriptorHeaps::m_rtvHeap = 0;
ComPtr<ID3D12DescriptorHeap> DescriptorHeaps::m_dsvHeap = 0;

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {

    //Console for debugging
    AllocConsole();
    FILE* fDummy;
    freopen_s(&fDummy, "CONIN$", "r", stdin);
    freopen_s(&fDummy, "CONOUT$", "w", stderr);
    freopen_s(&fDummy, "CONOUT$", "w", stdout);

    Game game(initialWidth, initialHeight);
    Application::Run(hInstance, nCmdShow, initialWidth, initialHeight, &game);
    return 0;
}