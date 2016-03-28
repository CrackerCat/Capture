#ifndef VIDEOENGINECAPTUREHUB_H
#define VIDEOENGINECAPTUREHUB_H

#include "CaptureTexture.h"

#include <string>
#include <list>

namespace h3d {

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


	//捕获的目标信息 [这个结构体需要保证x86/x64一样的大]
	//native_handle 目标窗体句柄 [NULL = 桌面抓取]
	//output_width x output_height 输出分辨率 [-1 = 按源输出]
	//output_fps 输出帧率[ -1 = 按源输出]

	//这是一个非常重要的结构体！
	struct H3D_API CaptureInfo
	{
		unsigned __int64 sNative;

		SDst oWidth;
		SDst oHeight;

		SDst oFps;

		CaptureInfo(void* native_handle = 0, SDst output_width = -1, SDst output_height = -1, SDst output_fps = -1)
			:sNative(reinterpret_cast<unsigned __int64>(native_handle)),oWidth(output_width),oHeight(output_height),oFps(output_fps)
		{}

		unsigned int Reserved1;//format
		unsigned int Reserved2;//mapid
		unsigned int Reserved3;//mapsize
		unsigned int Reserved4;//pitch
		unsigned int Flip;//fuck opengl

		unsigned int sPID;
	};

	//完成抓取的回调函数签名
	typedef void (__stdcall* CaptureCallBack)(CaptureTexture*);

	class H3D_API CaptureHUB {
	public:
		CaptureHUB() {}

		virtual ~CaptureHUB() {}

		virtual CaptureTexture* Capture() = 0;

		virtual void Stop() = 0;

		virtual bool Flip() {
			return false;
		}
	};


	struct MemoryInfo {
		unsigned int Reserved1;//texture1offset
		unsigned int Reserved2;//texture2offset
		unsigned int Reserved3;//lastrendered;
		unsigned int Reserved4;
	};

	bool H3D_API LoadPlugin();

	void H3D_API UnLoadPlugin();

#pragma warning(push)
#pragma warning(disable:4251)
	struct H3D_API CameraInfo
	{
		std::wstring Name;
		unsigned int Index;
		std::wstring Id;
	};
#pragma warning(pop)

	H3D_API std::list<CameraInfo> GetCameraInfos();
}

#define INFO_MEMORY             L"Local\\H3DInfoMemory"
#define TEXTURE_MEMORY          L"Local\\H3DTextureMemory"

#define TEXTURE_FIRST_MUTEX     L"H3DTextureFirstMutex"
#define TEXTURE_SECOND_MUTEX     L"H3DTextureSecondMutex"
#define EVNET_CAPTURE_READY		L"H3DCaptureReady"
#define EVENT_OBS_STOP			L"H3DObserverStop"
#define EVENT_OBS_BEGIN			L"H3DObserverBegin"

#define OBS_KEEP_ALIVE		L"H3DObserverKeepAlive"

#endif
