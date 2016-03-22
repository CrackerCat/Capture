#ifndef VIDEOENGINECAPTURECAMERA_H
#define VIDEOENGINECAPTURECAMERA_H

#include "MemoryCapture.h"
#include <string>
#include <list>

struct IMFMediaSource;
struct IMFSourceReader;

namespace h3d {
#pragma warning(push)
#pragma warning(disable:4251)
	struct H3D_API CameraInfo
	{
		std::wstring Name;
		unsigned int Index;
	};
#pragma warning(pop)

	H3D_API std::list<CameraInfo> GetCameraInfos();

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
