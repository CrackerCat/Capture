#ifndef VIDEOENGINECAPTURECAMERA_H
#define VIDEOENGINECAPTURECAMERA_H

#include "CaptureHUB.h"

struct IMFMediaSource;
struct IMFSourceReader;

struct SwsContext;
namespace h3d {

	class H3D_API CameraCapture : public CaptureHUB {
		CaptureCallBack opt_callbak;

		CaptureInfo capture_info;

		CameraCapture* pImpl;

		CaptureTexture* capture_tex;

		IMFMediaSource* pSource;
		IMFSourceReader* pReader;

		SwsContext* sws_context;
	public:
		CameraCapture(const CaptureInfo & info,unsigned int Index,CaptureCallBack callback = 0);

		~CameraCapture();

		CaptureTexture* Capture() override;

		void Stop() override;
	};
}

#endif
