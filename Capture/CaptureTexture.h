#ifndef VIDEOENGINECAPTURETEXTURE_H
#define VIDEOENGINECAPTURETEXTURE_H

#include "CapturePlugin.h"

//R << 8 | G << 16 || B << 24 || A
//R = [2] G = [1] B[0] = 2 A[3] = not use
//AV_PIX_FMT_BGRA
#define CAPTURETEXTURE_FORMAT "BGRA8 little-order"


struct HDC__;

namespace h3d
{
	enum TextureType {
		Memory_Texture,
		Device_Texture
	};


	class H3D_API CaptureTexture {
	public:
		CaptureTexture(TextureType tt)
		:type(tt)
		{}

		struct MappedData {
			byte* pData;
			unsigned long RowPitch;
		};

		virtual ~CaptureTexture() {}

		virtual SDst GetWidth() const = 0;

		virtual SDst GetHeight() const = 0;

		virtual MappedData Map() = 0;
		virtual void UnMap() = 0;

		TextureType GetType() const {
			return type;
		}
	private:
		TextureType type;
	};

	struct IGDI { 
		virtual HDC__ * GetDC() = 0;
		virtual void ReleaseDC() = 0;
	};
}



#endif
