#pragma once
#include "../eterBase/Stl.h"
#include <unordered_set>

class CMSWindow
{
public:
    CMSWindow();
    virtual ~CMSWindow() = default;

    void Destroy();
    bool Create(const wchar_t* c_szName, int brush = BLACK_BRUSH, uint32_t cs = 0, uint32_t ws = WS_OVERLAPPEDWINDOW, HICON hIcon = NULL, int iCursorResource = 32512);

    void Show();
    void Hide();

    void SetVisibleMode(bool isVisible);

    void SetPosition(int x, int y);
    void SetCenterPositionz();

    void SetTitle(const char* title);

    void AdjustSize(int width, int height);
    void SetSize(int width, int height);

    bool IsVisible();
    bool IsActive();

    void GetMousePosition(POINT* ppt);
    void GetClientRect(RECT* prc);
    void GetWindowRect(RECT* prc);

    int GetScreenWidth();
    int GetScreenHeight();

    HWND GetWindowHandle();
    HINSTANCE GetHInstance();

    virtual LRESULT WindowProcedure(HWND hWnd, UINT uiMsg, WPARAM wParam, LPARAM lParam);
    virtual void OnSize(WPARAM wParam, LPARAM lParam);

protected:
    typedef std::unordered_set<std::wstring> TWindowClassSet;

    const wchar_t* RegisterWindowClass(uint32_t style, int brush, WNDPROC pfnWndProc, HICON hIcon = NULL, int iCursorResource = 32512);

    HWND m_hWnd;
    RECT m_rect;
    bool m_isActive;
    bool m_isVisible;

    static TWindowClassSet ms_classNames;
    static HINSTANCE ms_hInstance;
};