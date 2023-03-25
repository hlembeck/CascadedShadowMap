#include "UserInput.h"

void Mouse::SetCursorPoint(POINT p, HCURSOR hCursor) {
    m_cursorPos = p;
    m_mouseScaleFactor = 20.0f;
    m_hCursor = hCursor;
    SetCursor(m_hCursor);
}

void Keyboard::OnKeyDown(WPARAM wParam) {
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

void Keyboard::OnKeyUp(WPARAM wParam) {
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

void UserInput::OnKeyDown(WPARAM wParam) {
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
        Keyboard::OnKeyDown(wParam);
    }
}

void UserInput::OnKeyUp(WPARAM wParam) {
    if (m_inputCaptured) {
        Keyboard::OnKeyUp(wParam);
    }
}