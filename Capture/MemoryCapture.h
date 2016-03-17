#ifndef VIDEOENGINEMEMORYCAPTURE_H
#define VIDEOENGINEMEMORYCAPTURE_H

#include "CaptureHUB.h"
#include <Windows.h>

namespace h3d {
	class MemoryTexture : public CaptureTexture {
		SDst cx;
		SDst cy;

		byte* native;
		UINT src_format;

		void* sws_context;//use ffmepg convert format

		bool ffmpeg_support;
		unsigned int R_MASK;
		unsigned int G_MASK;
		unsigned int B_MASK;
	public:
		MemoryTexture(UINT format, SDst width, SDst height);
		~MemoryTexture();

		SDst GetWidth() const override {
			return cx;
		}
		SDst GetHeight() const override {
			return cy;
		}

		MappedData Map() override;
		void UnMap() override {
		}

		void ReSize(SDst width, SDst height);

		void WriteData(LPBYTE pData, int pitch);
	};


	class MemoryCapture : public CaptureHUB {
		CaptureCallBack opt_callbak;

		CaptureInfo capture_info;

		HANDLE hMemoryMap;
		LPVOID mMemory;

		MemoryInfo* pInfo;
		LPBYTE TextureAddress[2];

		MemoryTexture* texture;

	public:
		MemoryCapture(CaptureInfo & info, CaptureCallBack callback = 0);

		~MemoryCapture();

		CaptureTexture* Capture() override;

		void Stop() override;

	public:
		static HANDLE texture_mutexs[2];
	};
}


#endif
