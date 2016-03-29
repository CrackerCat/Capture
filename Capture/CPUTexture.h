#ifndef CPUTEXTURE_H
#define CPUTEXTURE_H

#include "CaptureTexture.h"
#include <Windows.h>

namespace h3d {
	class GDITexture : public CaptureTexture, public IGDI {
		SDst cx;
		SDst cy;

		byte* native;

		HDC compatible_dc;
		HBITMAP target_bitmap;
		BITMAPINFOHEADER BitMapInfoHeader;

		//i thick if os >= 7 create D3D texture
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

		HDC GetDC() override {
			return compatible_dc;
		}

		void ReleaseDC() override {
		}

#ifdef _DEBUG
		//void DumpBMP(const wchar_t* bmpname);
#endif

		//void ReSize(SDst width, SDst height);
	};

	class MemoryTexture : public CaptureTexture {
		SDst cx;
		SDst cy;

		byte* native;
	public:
		MemoryTexture(SDst width, SDst height);
		virtual ~MemoryTexture();

		SDst GetWidth() const override {
			return cx;
		}
		SDst GetHeight() const override {
			return cy;
		}

		MappedData Map() override;
		void UnMap() override {
		}
	};
}


#endif
