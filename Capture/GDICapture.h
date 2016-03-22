#ifndef VIDEOENGINEGDICAPTURE_H
#define VIDEOENGINEGDICAPTURE_H

#include "CaptureHUB.h"
#include <Windows.h>


namespace h3d {
	class H3D_API GDICapture : public CaptureHUB {
		CaptureCallBack opt_callbak;
		
		CaptureInfo capture_info;

		CaptureTexture* capture_tex;
	public:
		GDICapture(CaptureInfo & info, CaptureCallBack callback = 0);

		~GDICapture();

		CaptureTexture* Capture() override;

		void Stop() override {}
	};
}


#endif
