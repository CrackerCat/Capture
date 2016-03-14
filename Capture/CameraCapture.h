#ifndef VIDEOENGINECAPTURECAMERA_H
#define VIDEOENGINECAPTURECAMERA_H

#include "MemoryCapture.h"
#include <string>
#include <list>
namespace h3d {
	struct H3D_API CameraInfo
	{
		std::wstring Name;
		unsigned int Index;
	};

	H3D_API std::list<CameraInfo> GetCameraInfos();

	class H3D_API CameraCapture : public CaptureHUB {
		CaptureCallBack opt_callbak;

		CaptureInfo capture_info;

		CameraCapture* pImpl;

		MemoryTexture* pTex;
	public:
		CameraCapture(CaptureInfo & info,unsigned int Index,CaptureCallBack callback = 0);

		~CameraCapture();

		CaptureTexture* Capture() override;

		void Stop() override;
	};
}

#endif
