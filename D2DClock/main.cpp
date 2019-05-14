#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include <windows.h>
#include <locale>
#include "BaseWindow.h"
#include "ClockScene.h"
#include "resource.h"

namespace {
    // Constants 
    const WCHAR WINDOW_NAME[] = L"Analog Clock";
    const int OFFSET = 12;    // area offset
    const PCWSTR CLSS_NM = L"Clock Window Class";
}

HANDLE g_hTimer = NULL;
BOOL InitializeTimer();

class MainWindow : public BaseWindow<MainWindow>
{
private:
    ClockScene   m_scene;
    HMENU        m_menu;
    HMENU        m_submenu;
    HRGN         m_hRegion;
public:
    PCWSTR  ClassName() const override { return CLSS_NM; }
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
};

LRESULT MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    POINT pos;

    switch (uMsg)
    {
    case WM_CREATE:
        SetLayeredWindowAttributes(m_hwnd, 0, 180, LWA_ALPHA);    // alpha channel
        if (FAILED(m_scene.Initialize()))
        {
            return -1;
        }
        m_hRegion = CreateEllipticRgn(OFFSET, OFFSET,
            static_cast<int>(GlobalConst::CLIENT_AREA_SIZE) - OFFSET,
            static_cast<int>(GlobalConst::CLIENT_AREA_SIZE) - OFFSET);
        SetWindowRgn(m_hwnd, m_hRegion, TRUE);

        // Load menu resource
        m_menu = LoadMenu(NULL, MAKEINTRESOURCE(IDR_MENU1));
        // Get handle of the submenu
        m_submenu = GetSubMenu(m_menu, 0);
        return 0;

    case WM_DESTROY:
        m_scene.CleanUp();
        PostQuitMessage(0);
        return 0;

    case WM_PAINT:
    case WM_DISPLAYCHANGE:
        {
            PAINTSTRUCT ps;
            BeginPaint(m_hwnd, &ps);
            m_scene.Render(m_hwnd);
            EndPaint(m_hwnd, &ps);
        }
        return 0;

    // Delegate to Direct2D
    case WM_ERASEBKGND:
        return 1;

    case WM_COMMAND:        // When the item selected
        switch (LOWORD(wParam))
        {
        case ID_EXIT:       // Exit
            SendMessage(m_hwnd, WM_CLOSE, 0, 0);
            break;
        }
        return 0;

    case WM_RBUTTONUP:      // When up right mouse button
        // Get the cursor position
        pos.x = LOWORD(lParam);
        pos.y = HIWORD(lParam);

        // Convert to screen position
        ClientToScreen(m_hwnd, &pos);

        // Show popup menu
        TrackPopupMenu(m_submenu, TPM_LEFTALIGN, pos.x, pos.y, 0, m_hwnd, NULL);
        return 0;

    case WM_CLOSE:          // When the window closed
        DestroyMenu(m_menu);
        m_menu = NULL;
        // Delegate to DefWindowProc()
        return DefWindowProc(m_hwnd, uMsg, wParam, lParam);

    case WM_LBUTTONDOWN:     // When up left mouse button
        // Treat as cliking on the title bar
        PostMessage(m_hwnd, WM_NCLBUTTONDOWN, HTCAPTION, lParam);
        return 0;

    default:
        return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
    }
}


BOOL InitializeTimer()
{
    g_hTimer = CreateWaitableTimer(NULL, FALSE, NULL);
    if (g_hTimer == NULL)
    {
        return FALSE;
    }

    LARGE_INTEGER li = {0};
    // 1/60 Sec
    if (!SetWaitableTimer(g_hTimer, &li, (1000/60), NULL, NULL,FALSE))
    {
        CloseHandle(g_hTimer);
        g_hTimer = NULL;
        return FALSE;
    }

    return TRUE;
}

INT WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, INT nCmdShow)
{
    std::locale::global(std::locale(""));
#ifdef _DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
    if (FAILED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE)))
    {
        return 0;
    }

    if (!InitializeTimer())
    {
        CoUninitialize();
        return 0;
    }

    MainWindow win;

    //if (!win.Create(WINDOW_NAME, WS_OVERLAPPEDWINDOW ^ (WS_THICKFRAME | WS_MAXIMIZEBOX)))
    if (!win.Create(WINDOW_NAME, WS_POPUP))
    {
        return 0;
    }

    ShowWindow(win.Window(), nCmdShow);

    // Run the message loop.
    MSG msg = {};
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }
        // Wait until the timer expires or any message is posted.
        if (MsgWaitForMultipleObjects(1, &g_hTimer, FALSE, INFINITE, QS_ALLINPUT)
            == WAIT_OBJECT_0)
        {
            InvalidateRect(win.Window(), NULL, FALSE);
        }
    }

    CloseHandle(g_hTimer);
    CoUninitialize();

    return 0;
}