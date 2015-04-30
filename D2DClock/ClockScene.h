#pragma once
#include <D2d1.h>
#include <assert.h>
#include <unordered_map>
//#include <map>
#include "GlobalConst.h"

struct ID2D1Bitmap;
struct IWICImagingFactory;

class ClockScene
{
    ID2D1Factory                *m_pFactory;
    ID2D1HwndRenderTarget       *m_pRenderTarget;

    ID2D1SolidColorBrush        *m_pStroke;
    D2D1_ELLIPSE                 m_ellipse;

    HRESULT CreateGraphicsResources(HWND hwnd);
    HRESULT CreateDeviceDependentResources();
    void    DiscardDeviceDependentResources();
    void    CalculateLayout();
    void    RenderScene();

    void    DrawClockHand(
        const float fHandLength,
        const float fAngle,
        const float fStrokeWidth) const;

    ID2D1Bitmap                 *m_pBitmapClockDial;
    ID2D1Bitmap                 *m_pBitmapClockHour;
    ID2D1Bitmap	                *m_pBitmapClockMinute;
    //std::map<const std::wstring, ID2D1Bitmap *> m_pBitmapMap;
    // ÉLÅ[Ç™const std::wstringÇæÇ∆ÇæÇﬂ(unordered_map)
    std::unordered_map<std::wstring, ID2D1Bitmap *> m_pBitmapMap;
    void    DrawClockHandPict(const std::wstring key, float fAngle);

    IWICImagingFactory	        *m_pImagingFactory;

public:

    ClockScene();
    ~ClockScene();
    HRESULT Initialize();
    void Render(HWND hwnd);
    void CleanUp();
};

