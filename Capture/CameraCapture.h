#ifndef VIDEOENGINECAPTURECAMERA_H
#define VIDEOENGINECAPTURECAMERA_H

#include "CaptureHUB.h"

struct IMFMediaSource;
struct IMFSourceReader;

namespace h3d {
	class MemoryTexture;

	class H3D_API CameraCapture : public CaptureHUB {
		CaptureCallBack opt_callbak;

		CaptureInfo capture_info;

		CameraCapture* pImpl;

		MemoryTexture* pTex;

		IMFMediaSource* pSource;
		IMFSourceReader* pReader;
	public:
		CameraCapture(const CaptureInfo & info,unsigned int Index,CaptureCallBack callback = 0);

		~CameraCapture();

		CaptureTexture* Capture() override;

		void Stop() override;
	};
}

#endif
