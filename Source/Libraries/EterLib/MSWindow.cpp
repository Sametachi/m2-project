#include "StdAfx.h"
#include "MSWindow.h"
#include <Shobjidl.h>

#include <boost/locale/encoding_utf.hpp>
#include <windowsx.h>
#include <fmt/format.h>
#include <fmt/xchar.h>
#include <dwmapi.h>

CMSWindow::TWindowClassSet CMSWindow::ms_classNames;
HINSTANCE CMSWindow::ms_hInstance = NULL;

LRESULT CALLBACK MSWindowProcedure(HWND hWnd, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    CMSWindow* pWnd = (CMSWindow*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    if (pWnd)
        return pWnd->WindowProcedure(hWnd, uiMsg, wParam, lParam);

    return DefWindowProcW(hWnd, uiMsg, wParam, lParam);
}

LRESULT CMSWindow::WindowProcedure(HWND hWnd, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uiMsg)
    {
    case WM_SIZE:
        OnSize(wParam, lParam);
        break;

    case WM_ACTIVATEAPP:
        m_isActive = (wParam == WA_ACTIVE) || (wParam == WA_CLICKACTIVE);
        break;
    }

    return DefWindowProcW(hWnd, uiMsg, wParam, lParam);
}

void CMSWindow::OnSize(WPARAM wParam, LPARAM /*lParam*/)
{
    if (wParam == SIZE_MINIMIZED)
    {
        InvalidateRect(m_hWnd, NULL, true);
        m_isActive = false;
        m_isVisible = false;
    }
    else
    {
        m_isActive = true;
        m_isVisible = true;
    }
}

void CMSWindow::Destroy()
{
    if (!m_hWnd)
        return;

    if (IsWindow(m_hWnd))
        DestroyWindow(m_hWnd);

    m_hWnd = NULL;
    m_isVisible = false;
}

bool CMSWindow::Create(const wchar_t* c_szName, int brush, uint32_t cs, uint32_t ws, HICON hIcon, int iCursorResource)
{
    Destroy();

    const wchar_t* c_szClassName = RegisterWindowClass(cs, brush, MSWindowProcedure, hIcon, iCursorResource);

    m_hWnd = CreateWindowW(
        c_szClassName,
        c_szName,
        ws,
        0, 0, 0, 0,
        NULL,
        NULL,
        ms_hInstance,
        NULL);

    if (!m_hWnd)
        return false;

    WCHAR myAppID[128];
    swprintf(myAppID, sizeof(myAppID) / sizeof(myAppID[0]), L"YitsoraID%u", GetCurrentProcessId());
    HRESULT hr = SetCurrentProcessExplicitAppUserModelID(myAppID);
    if (!SUCCEEDED(hr))
        return false;

    SetWindowLongPtr(m_hWnd, GWLP_USERDATA, (LONG_PTR)this);

    return true;
}

void CMSWindow::SetVisibleMode(bool isVisible)
{
    m_isVisible = isVisible;

    if (m_isVisible)
    {
        ShowWindow(m_hWnd, SW_SHOW);
    }
    else
    {
        ShowWindow(m_hWnd, SW_HIDE);
    }
}

void CMSWindow::Show()
{
    m_isVisible = true;
    ShowWindow(m_hWnd, SW_SHOW);
}

void CMSWindow::Hide()
{
    m_isVisible = false;
    ShowWindow(m_hWnd, SW_HIDE);
}

bool CMSWindow::IsVisible()
{
    return m_isVisible;
}

bool CMSWindow::IsActive()
{
    return m_isActive;
}

HINSTANCE CMSWindow::GetHInstance()
{
    return ms_hInstance;
}

HWND CMSWindow::GetWindowHandle()
{
    return m_hWnd;
}

int CMSWindow::GetScreenWidth()
{
    return GetSystemMetrics(SM_CXSCREEN);
}

int CMSWindow::GetScreenHeight()
{
    return GetSystemMetrics(SM_CYSCREEN);
}

void CMSWindow::GetWindowRect(RECT* prc)
{
    ::GetWindowRect(m_hWnd, prc);
}

void CMSWindow::GetClientRect(RECT* prc)
{
    ::GetClientRect(m_hWnd, prc);
}

void CMSWindow::GetMousePosition(POINT* ppt)
{
    GetCursorPos(ppt);
    ScreenToClient(m_hWnd, ppt);
}

void CMSWindow::SetPosition(int x, int y)
{
    SetWindowPos(m_hWnd, NULL, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
}

void CMSWindow::SetCenterPositionz()
{
    RECT rc;

    GetClientRect(&rc);

    int windowWidth = rc.right - rc.left;
    int windowHeight = rc.bottom - rc.top;

    SetPosition((GetScreenWidth() - windowWidth) / 2, (GetScreenHeight() - windowHeight) / 2);
}

void CMSWindow::AdjustSize(int width, int height)
{
    SetRect(&m_rect, 0, 0, width, height);

    AdjustWindowRectEx(&m_rect,
        GetWindowStyle(m_hWnd),
        GetMenu(m_hWnd) != NULL,
        GetWindowExStyle(m_hWnd));

    MoveWindow(m_hWnd,
        0, 0,
        m_rect.right - m_rect.left,
        m_rect.bottom - m_rect.top,
        false);
}

void CMSWindow::SetTitle(const char* title)
{
    using boost::locale::conv::utf_to_utf;
    SetWindowTextW(m_hWnd, utf_to_utf<wchar_t>(title).c_str());
}

void CMSWindow::SetSize(int width, int height)
{
    SetWindowPos(m_hWnd, NULL, 0, 0, width, height, SWP_NOZORDER | SWP_NOMOVE);
}

const wchar_t* CMSWindow::RegisterWindowClass(uint32_t style, int brush, WNDPROC pfnWndProc, HICON hIcon, int iCursorResource)
{
    std::wstring className = fmt::format(L"yit - s{}:b{}:p:{}", style, brush, (void*)(pfnWndProc));

    auto f = ms_classNames.find(className);
    if (f != ms_classNames.end())
        return (*f).c_str();

    auto r = ms_classNames.emplace(std::move(className));
    assert(r.second && "Race condition - impossible");

    WNDCLASSW wc{};

    wc.style = style;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.lpfnWndProc = pfnWndProc;
    wc.hCursor = LoadCursor(ms_hInstance, MAKEINTRESOURCE(iCursorResource));
    wc.hIcon = hIcon ? hIcon : LoadIcon(ms_hInstance, IDI_APPLICATION);
    wc.hbrBackground = (HBRUSH)GetStockObject(brush);
    wc.hInstance = ms_hInstance;
    wc.lpszClassName = (*r.first).c_str();
    wc.lpszMenuName = L"";

    if (!RegisterClassW(&wc))
        return L"";

    return (*r.first).c_str();
}

CMSWindow::CMSWindow()
{
    m_hWnd = NULL;
    m_isVisible = false;
    m_isActive = false;
}
