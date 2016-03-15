#ifndef VIDEOENGINECAPTURETEXTURE_H
#define VIDEOENGINECAPTURETEXTURE_H

#include "CapturePlugin.h"

//R << 8 | G << 16 || B << 24 || A
//R = [2] G = [1] B[0] = 2 A[3] = not use
//AV_PIX_FMT_BGRA
#define CAPTURETEXTURE_FORMAT "BGRA8 little-order"

namespace h3d
{
	class H3D_API CaptureTexture {
	public:
		CaptureTexture() {}

		struct MappedData {
			byte* pData;
			unsigned long RowPitch;
		};

		virtual ~CaptureTexture() {}

		virtual SDst GetWidth() const = 0;

		virtual SDst GetHeight() const = 0;

		virtual MappedData Map() = 0;
		virtual void UnMap() = 0;
	private:
	};
}



#endif
