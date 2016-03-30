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

	//格式方式为DXGI书写格式[内存序]
	enum SWAPFORMAT {
		SWAPFORMAT_UNKNOWN,
		//D3D9 Support Format
		R10G10B10A2,//it not supported
		B10G10R10A2,
		BGRA8,
		BGRX8,
		B5G6R5A1,
		B5G6R5X1,
		B5G6R5,

		//D3D11 New Format
		RGBA8,
		RGBA16,
		R10G10B10XRA2,

		//Camera Support Format
		//http://www.fourcc.org/yuv.php#UYVY
		HDYC,//UYUV,HD Space
		UYVY,
		YUY2,
		YVYU,
		YV12,
		I420,
	};

	class H3D_API CaptureTexture {
	public:
		struct MappedData {
			byte* pData;
			unsigned long RowPitch;
		};

		enum TextureType {
			Memory_Texture,
			Device_Texture
		};

		CaptureTexture(TextureType tt)
			:type(tt)
		{}

		virtual ~CaptureTexture() {}

		virtual SDst GetWidth() const = 0;

		virtual SDst GetHeight() const = 0;

		virtual MappedData Map() = 0;
		virtual void UnMap() = 0;
	public:
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
