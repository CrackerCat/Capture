#include "GDICapture.h"
#include "CPUTexture.h"

extern "C"
{
#include <libswscale/swscale.h>
}


using namespace h3d;






#include "D3D11RenderSystem.hpp"
GDICapture::GDICapture(CaptureInfo & info, CaptureCallBack callback)
	:capture_info(info),opt_callbak(callback)
{
	if (capture_info.sNative == 0) {
		capture_info.sNative =reinterpret_cast<__int64>(GetDesktopWindow());
	}

	HWND hwnd = (HWND)capture_info.sNative;

	SDst cx = info.oWidth;
	SDst cy = info.oHeight;

	RECT rect = { 0 , 0 , 0, 0 };
	GetWindowRect(hwnd, &rect);

	if(cx == -1)
		cx = rect.right - rect.left;
	if(cy == -1)
		cy = rect.bottom - rect.top;

	if (GetEngine().GetLevel() >= D3D_FEATURE_LEVEL_9_1)
		capture_tex = GetEngine().GetFactory().CreateGDITexture(cx, cy);
	else
		capture_tex = new GDITexture(hwnd, cx, cy);
}

GDICapture::~GDICapture() {
	
	delete capture_tex;
}

CaptureTexture* GDICapture::Capture() {
	HWND hwnd = (HWND)capture_info.sNative;

	if (IsWindow(hwnd) && IsIconic(hwnd))
		return capture_tex;

	RECT rect = { 0 , 0 , 0, 0 };
	GetWindowRect(hwnd, &rect);
	SDst cx = rect.right - rect.left;
	SDst cy = rect.bottom - rect.top;

	//使用GetDC 只抓取客户区
	auto src_hdc = GetWindowDC(hwnd);

	/*if (capture_info.oWidth == -1 || capture_info.oHeight == -1) {
		if (capture_tex->GetWidth() != cx || capture_tex->GetHeight() != cy)
			capture_tex->ReSize(cx, cy);
	}*/
	IGDI* capture_gdi = NULL;
	if (GetEngine().GetLevel() >= D3D_FEATURE_LEVEL_9_1)
		capture_gdi = static_cast<D3D11GDITexture*>(capture_tex);
	else
		capture_gdi = static_cast<GDITexture*>(capture_tex);

	//更改后四个参数可以决定要拷贝的区域
	//不建议使用StretchBlt 缩放过大的区域，效果非常差
	if (!StretchBlt(capture_gdi->GetDC(), 0, 0, capture_tex->GetWidth(),capture_tex->GetHeight(), src_hdc, 0, 0, cx, cy, SRCCOPY)) {
		ReleaseDC(hwnd,src_hdc);
		capture_gdi->ReleaseDC();
		return NULL;
	}
	ReleaseDC(hwnd, src_hdc);
	capture_gdi->ReleaseDC();
	return capture_tex;
}