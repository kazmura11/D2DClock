#pragma once
#ifdef _DEBUG
#include <sstream>
#endif
#include "GlobalConst.h"

// See: http://msdn.microsoft.com/ja-jp/library/windows/desktop/ff381400%28v=vs.85%29.aspx
template <class DERIVED_TYPE> 
class BaseWindow
{
public:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        DERIVED_TYPE *pThis = NULL;

        if (uMsg == WM_NCCREATE)
        {
            // Get CREATESTRUCT
            CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT *>(lParam);
            pThis = reinterpret_cast<DERIVED_TYPE *>(pCreate->lpCreateParams);
            // Call SetWindowLongPtr, pass the data to the original data
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));

            pThis->m_hwnd = hwnd;
        }
        else
        {
            // Get the Pointer
            pThis = reinterpret_cast<DERIVED_TYPE *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        }
        if (pThis)
        {
            return pThis->HandleMessage(uMsg, wParam, lParam);
        }
        else
        {
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }
    }

    BaseWindow() : m_hwnd(NULL) { }

    BOOL Create(
        PCWSTR lpWindowName,
        DWORD dwStyle,
        DWORD dwExStyle = WS_EX_LAYERED,   // Transparent layer
        int x = CW_USEDEFAULT,
        int y = CW_USEDEFAULT,
        int nClientWidth = GlobalConst::CLIENT_AREA_SIZE,
        int nClientHeight = GlobalConst::CLIENT_AREA_SIZE,
        HWND hWndParent = 0,
        HMENU hMenu = 0
        )
    {
        WNDCLASS wc = { 0 };

        wc.lpfnWndProc   = DERIVED_TYPE::WindowProc;
        wc.hInstance     = GetModuleHandle(NULL);
        wc.lpszClassName = ClassName();
        // Change cursor
        wc.hCursor = static_cast<HCURSOR>(
            LoadImage(NULL,
            MAKEINTRESOURCE(IDC_ARROW),
            IMAGE_CURSOR,
            0,
            0,
            LR_DEFAULTSIZE | LR_SHARED)
            );
        RegisterClass(&wc);

        // Adjust window rect
        RECT rc = { 0, 0, nClientWidth, nClientHeight };
        AdjustWindowRectEx(&rc, dwStyle, FALSE, 0);
        int nWidth = rc.right - rc.left;
        int nHeight = rc.bottom - rc.top;

        if (dwStyle == WS_POPUP && x == CW_USEDEFAULT && y == CW_USEDEFAULT)
        {
            // Get the client rect without task bar
            RECT workerArea;
            SystemParametersInfo(SPI_GETWORKAREA, 0, &workerArea, 0);

            x = workerArea.right - nClientWidth;
            y = workerArea.bottom - nClientHeight;
#ifdef _DEBUG
            std::wstringstream ss1;
            ss1 << L"taskbar" << L"\n"
                << L"top " << workerArea.top << L"\n"
                << L"left " << workerArea.left << L"\n"
                << L"bottom " << workerArea.bottom << L"\n"
                << L"right " << workerArea.right << L"\n";

            MessageBox(NULL, ss1.str().c_str(), L"Debug Message", MB_OK);
            std::wstringstream ss2;
            ss2 << L"x : " << x << L"y : " << y;
            MessageBox(NULL, ss2.str().c_str(), L"Debug Message", MB_OK);
#endif
        }

        m_hwnd = CreateWindowEx(
            dwExStyle, 
            ClassName(),
            lpWindowName,
            dwStyle,
            x, y, nWidth, nHeight,
            hWndParent,
            hMenu,
            GetModuleHandle(NULL),
            this   // Additional application data
            );

        return (m_hwnd ? TRUE : FALSE);
    }

    HWND Window() const { return m_hwnd; }

protected:
    virtual PCWSTR  ClassName() const = 0;
    virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;

    HWND m_hwnd;
};