#ifndef VIDEOENGINECAPTUREHUB_H
#define VIDEOENGINECAPTUREHUB_H

#include "CaptureTexture.h"

namespace h3d {

	enum SWAPFORMAT {
		SWAPFORMAT_UNKNOWN,
		RGB8,
		RGBA8,
		BGR8,
		BGRA8,
		BGRX8,
		RGBA16,
		RGBA32,
		B5G6R5,
		R10G10B10A2,
		HDYC,
	};


	//捕获的目标信息 [这个结构体需要保证x86/x64一样的大]
	//native_handle 目标窗体句柄 [NULL = 桌面抓取]
	//output_width x output_height 输出分辨率 [-1 = 按源输出]
	//output_fps 输出帧率[ -1 = 按源输出]
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
	};


	struct MemoryInfo {
		unsigned int Reserved1;//texture1offset
		unsigned int Reserved2;//texture2offset
		unsigned int Reserved3;//lastrendered;
		unsigned int Reserved4;
	};

	bool H3D_API LoadPlugin();

	void H3D_API UnLoadPlugin();
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
