#ifndef VIDEOENGINETEXTURECAPTURE_H
#define VIDEOENGINETEXTURECAPTURE_H

//!don't support xp

#include "CaptureHUB.h"
#include "D3D11RenderSystem.hpp"

#pragma warning(disable:4800)
namespace h3d {
	class H3D_API TextureCapture : public CaptureHUB {
		CaptureCallBack opt_callbak;

		CaptureInfo capture_info;

		D3D11Texture* capture_tex;
		D3D11Texture* shared_tex;

		HANDLE hMemoryMap;
		LPVOID mMemory;
	public:
		TextureCapture(CaptureInfo & info, CaptureCallBack callback = 0);

		~TextureCapture();

		D3D11Texture* Capture() override;

		void Stop() override;

		bool Flip() override {
			return  capture_info.Flip;
		}
	};
}

#endif
