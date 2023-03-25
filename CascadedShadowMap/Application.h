#pragma once
#include "DXBase.h"

/*
This class contains the window procedures.
*/
class Application {
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static HWND m_hWnd;
public:
	static int Run(HINSTANCE hInstance, int nCmdShow, UINT width, UINT height, DXWindowBase* pInstance);
	static HWND GetHWND();
};