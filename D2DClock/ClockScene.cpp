#include "ClockScene.h"
#include <d2d1helper.h>			// Direct2D
#include <wrl.h>                // Microsoft::WRL::ComPtr<>
#include <wincodec.h>			// WIC
#include <wincodecsdk.h>		// WIC


template <class T> void SafeRelease(T **ppT)
{
    if (*ppT)
    {
        (*ppT)->Release();
        *ppT = NULL;
    }
}

ClockScene::ClockScene()
    : m_pStroke(NULL),
    m_pFactory(NULL),
    m_pRenderTarget(NULL),
    m_pBitmapClockDial(NULL),
    m_pBitmapClockHour(NULL),
    m_pBitmapClockMinute(NULL),
    m_pImagingFactory(NULL)
{
    m_pBitmapMap.insert(std::pair<std::wstring, ID2D1Bitmap *>(GlobalConst::PNG_FILE[0], m_pBitmapClockDial));
    m_pBitmapMap.insert(std::pair<std::wstring, ID2D1Bitmap *>(GlobalConst::PNG_FILE[1], m_pBitmapClockHour));
    m_pBitmapMap.insert(std::pair<std::wstring, ID2D1Bitmap *>(GlobalConst::PNG_FILE[2], m_pBitmapClockMinute));
}

ClockScene::~ClockScene()
{
    CleanUp();
}

void ClockScene::CleanUp()
{
    DiscardDeviceDependentResources();

    // Discard device-independent resources.
    SafeRelease(&m_pFactory);
    SafeRelease(&m_pImagingFactory);
    SafeRelease(&m_pBitmapClockDial);
    SafeRelease(&m_pBitmapClockHour);
    SafeRelease(&m_pBitmapClockMinute);
}

void ClockScene::DiscardDeviceDependentResources()
{
    SafeRelease(&m_pStroke);
    SafeRelease(&m_pRenderTarget);
    SafeRelease(&m_pImagingFactory);
    SafeRelease(&m_pBitmapClockDial);
    SafeRelease(&m_pBitmapClockHour);
    SafeRelease(&m_pBitmapClockMinute);
}

HRESULT ClockScene::Initialize()
{
    return D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
        &m_pFactory);
}

void ClockScene::Render(HWND hwnd)
{
    HRESULT hr = CreateGraphicsResources(hwnd);
    if (FAILED(hr))
    {
        return;
    }

    assert(m_pRenderTarget != NULL);
    m_pRenderTarget->BeginDraw();

    RenderScene();

    hr = m_pRenderTarget->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET)
    {
        DiscardDeviceDependentResources();
    }
}

HRESULT ClockScene::CreateGraphicsResources(HWND hwnd)
{
    HRESULT hr = S_OK;
    if (m_pRenderTarget == NULL)
    {
        hr = CoCreateInstance(
            CLSID_WICImagingFactory,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_IWICImagingFactory,
            reinterpret_cast<void **>(&m_pImagingFactory));
        if (FAILED(hr))
        {
            MessageBox(NULL, L"CoCreateInstance() failed.", NULL, MB_OK);
            return E_FAIL;
        }

        HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pFactory);
        if (FAILED(hr))
        {
            MessageBox(NULL, L"D2D1CreateFactory() failed.", NULL, MB_OK);
            return E_FAIL;
        }
        RECT rc;
        GetClientRect(hwnd, &rc);
        D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);
 
        // Create a Direct2D render target.
        hr = m_pFactory->CreateHwndRenderTarget(
            // Creates a D2D1_RENDER_TARGET_PROPERTIES structure.
            D2D1::RenderTargetProperties(),
            // Creates a D2D1_HWND_RENDER_TARGET_PROPERTIES structure.
            D2D1::HwndRenderTargetProperties(hwnd, size),
            &m_pRenderTarget
            );
        if (FAILED(hr))
        {
            MessageBox(NULL, L"CreateHwndRenderTarget() failed.", NULL, MB_OK);
            return E_FAIL;
        }

        for (auto iter = m_pBitmapMap.begin(); iter != m_pBitmapMap.end(); ++iter)
        {
            // Read PNG file
            Microsoft::WRL::ComPtr<IWICBitmapDecoder> dec;
            hr = m_pImagingFactory->CreateDecoderFromFilename(
                iter->first.c_str(),               // file name
                NULL,   // GUID for vendor
                GENERIC_READ,    // GENERIC_READ / GENERIC_WRITE
                WICDecodeMetadataCacheOnLoad,   // OnDemand / OnLoad
                &dec);
            if (FAILED(hr))
            {
                MessageBox(NULL, L"CreateDecoderFromFilename() failed.", NULL, MB_OK);
                return E_FAIL;
            }
            Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
            // Retrieves the specified frame of the image.
            hr = dec->GetFrame(0, &frame);    // arg0: The particular frame to retrieve.
            if (FAILED(hr))
            {
                MessageBox(NULL, L"GetFrame() failed.", NULL, MB_OK);
                return E_FAIL;
            }
            Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
            // Creates a new instance of the IWICFormatConverter class
            hr = m_pImagingFactory->CreateFormatConverter(&converter);
            if (FAILED(hr))
            {
                MessageBox(NULL,
                    L"CreateFormatConverter() failed.",
                    NULL,
                    MB_OK);
                return E_FAIL;
            }
            hr = converter->Initialize(
                frame.Get(),   // ComPtr IWICBitmapSource
                GUID_WICPixelFormat32bppPBGRA,  // REFWICPixelFormatGUID 
                WICBitmapDitherTypeNone,    // WICBitmapDitherType 
                NULL,    // IWICPalette
                0.f,     // alphaThresholdPercent
                WICBitmapPaletteTypeMedianCut    // paletteTranslate
                );
                // An optimal palette generated using a median
                // - cut algorithm.Derived from the colors in an image.
            if (FAILED(hr))
            {
                MessageBox(NULL,
                    L"Initialize() failed.",
                    NULL,
                    MB_OK);
                return E_FAIL;
            }
            // Creates an ID2D1Bitmap by copying the specified WIC bitmap
            hr = m_pRenderTarget->CreateBitmapFromWicBitmap(
                converter.Get(),    // IWICBitmapSource 
                NULL,               // optional
                &(iter->second)     // ComPtr ID2D1Bitmap 
                );
            if (FAILED(hr))
            {
                MessageBox(NULL,
                    L"CreateBitmapFromWicBitmap() failed.",
                    NULL,
                    MB_OK);
                return E_FAIL;
            }
        }

        hr = CreateDeviceDependentResources();
        if (FAILED(hr))
        {
            MessageBox(NULL, L"CreateDeviceDependentResources() failed", NULL, MB_OK);
            return E_FAIL;
        }
        CalculateLayout();
    }
    return hr;
}

HRESULT ClockScene::CreateDeviceDependentResources()
{
    return m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0, 0, 0), &m_pStroke);
}

void ClockScene::DrawClockHand(
    const float fHandLength,
    const float fAngle,
    const float fStrokeWidth) const
{
     m_pRenderTarget->SetTransform(
        D2D1::Matrix3x2F::Rotation(fAngle, m_ellipse.point)
        );

    // endPoint defines one end of the hand.
    D2D_POINT_2F endPoint = D2D1::Point2F(
        m_ellipse.point.x,
        m_ellipse.point.y - (m_ellipse.radiusY * fHandLength)
        );

    // Draw a line from the center of the ellipse to endPoint.
    m_pRenderTarget->DrawLine(
        m_ellipse.point, endPoint, m_pStroke, fStrokeWidth);
}

void ClockScene::DrawClockHandPict(const std::wstring key, const float fAngle)
{
    m_pRenderTarget->SetTransform(
        D2D1::Matrix3x2F::Rotation(fAngle, m_ellipse.point)
        );
    D2D1_SIZE_F fSize = m_pRenderTarget->GetSize();
    D2D1_SIZE_F fSizeClock = m_pBitmapMap[key]->GetSize();
    m_pRenderTarget->DrawBitmap(m_pBitmapMap[key],
        D2D1::Rect<float>(
            (fSize.width - fSizeClock.width) / 2.0f ,
            0,
            (fSize.width + fSizeClock.width) / 2.0f,
            fSize.height
            )
        );
}

void ClockScene::RenderScene()
{
    m_pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));

    // Draw Dial
    D2D1_SIZE_F fSize = m_pBitmapMap[GlobalConst::PNG_FILE[0]]->GetSize();

    m_pRenderTarget->DrawBitmap(
        m_pBitmapMap[GlobalConst::PNG_FILE[0]],
        D2D1::Rect<float>(0, 0, fSize.width, fSize.height));

    // Draw hands
    SYSTEMTIME time;
    GetLocalTime(&time);

    // 60 minutes = 30 degrees, 1 minute = 0.5 degree
    const float fHourAngle = (360.0f / 12) * (time.wHour) + (time.wMinute * 0.5f);
    const float fMinuteAngle = (360.0f / 60) * (time.wMinute);
    const float fSecondAngle =
        (360.0f / 60) * (time.wSecond) + (360.0f / 60000) * (time.wMilliseconds);

    DrawClockHandPict(GlobalConst::PNG_FILE[1], fHourAngle);
    DrawClockHandPict(GlobalConst::PNG_FILE[2], fMinuteAngle);
    DrawClockHand(0.75f, fSecondAngle, 1);

    // Restore the identity transformation.
    m_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
}

void ClockScene::CalculateLayout()
{
    m_ellipse = D2D1::Ellipse(D2D1::Point2F(GlobalConst::RADIUS, GlobalConst::RADIUS),
        GlobalConst::RADIUS, GlobalConst::RADIUS);
}


