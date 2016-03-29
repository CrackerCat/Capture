#ifndef VIDEOENGINEMEMORYCAPTURE_H
#define VIDEOENGINEMEMORYCAPTURE_H

#include "CaptureHUB.h"
#include <Windows.h>

#pragma warning(disable:4800)
namespace h3d {
	class MemoryCapture : public CaptureHUB {
		CaptureCallBack opt_callbak;

		CaptureInfo capture_info;

		HANDLE hMemoryMap;
		LPVOID mMemory;

		MemoryInfo* pInfo;
		LPBYTE TextureAddress[2];

		CaptureTexture* texture;

		void* sws_context;//use ffmepg convert format
		bool ffmpeg_support;
		unsigned int R_MASK;
		unsigned int G_MASK;
		unsigned int B_MASK;
	public:
		MemoryCapture(CaptureInfo & info, CaptureCallBack callback = 0);

		~MemoryCapture();

		CaptureTexture* Capture() override;

		void Stop() override;

		bool Flip() override {
			return  capture_info.Flip;
		}
	public:
		static HANDLE texture_mutexs[2];
	};
}


#endif
