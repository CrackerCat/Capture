#ifndef VIDEOENGINEGDICAPTURE_H
#define VIDEOENGINEGDICAPTURE_H

#include "CaptureHUB.h"
#include <Windows.h>


namespace h3d {
	class H3D_API GDITexture : public CaptureTexture {
		SDst cx;
		SDst cy;

		byte* native;

		HDC compatible_dc;
		HBITMAP target_bitmap;
		BITMAPINFOHEADER BitMapInfoHeader;

		void* sws_context;//convert RGBA to BGRA
	public:
		GDITexture(HWND hwnd, SDst width, SDst height);
		~GDITexture();

		SDst GetWidth() const override {
			return cx;
		}
		SDst GetHeight() const override {
			return cy;
		}

		MappedData Map() override;
		void UnMap() override {
		}

		HDC GetDC() {
			return compatible_dc;
		}

#ifdef _DEBUG
		void DumpBMP(const wchar_t* bmpname);
#endif

		void ReSize(SDst width, SDst height);
	};

	class H3D_API GDICapture : public CaptureHUB {
		CaptureCallBack opt_callbak;
		
		CaptureInfo capture_info;

		GDITexture* capture_tex;
	public:
		GDICapture(CaptureInfo & info, CaptureCallBack callback = 0);

		~GDICapture();

		GDITexture* Capture() override;

		void Stop() override {}
	};
}


#endif
