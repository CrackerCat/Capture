#ifndef VIDEOENGINEGRAPHICCAPTURE_H
#define VIDEOENGINEGRAPHICCAPTURE_H

#include "CaptureHub.h"

namespace h3d {
	//only for test
	H3D_API CaptureHUB*  GraphicCapture(unsigned long processId);

	H3D_API void EndCapture(CaptureHUB* capture);
}


#endif
