#pragma once
#include "Shared.h"

class Mouse {
protected:
	POINT m_cursorPos;
	float m_mouseScaleFactor; //Sets angular velocity, will use fov to do this later
	HCURSOR m_hCursor;
public:
	void SetCursorPoint(POINT p, HCURSOR hCursor);
};

class Keyboard {
	XMFLOAT3 m_linearVelocity;
protected:
	virtual void OnKeyDown(WPARAM wParam);
	virtual void OnKeyUp(WPARAM wParam);
};

class UserInput :
	public Keyboard,
	public Mouse,
	public virtual Time
{
protected:
	BOOL m_inputCaptured;
public:
	void OnKeyDown(WPARAM wParam);
	void OnKeyUp(WPARAM wParam);
};