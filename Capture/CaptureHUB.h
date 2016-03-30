#ifndef VIDEOENGINECAPTUREHUB_H
#define VIDEOENGINECAPTUREHUB_H

#include "CaptureTexture.h"

#include <string>
#include <list>

namespace h3d {

	


	//�����Ŀ����Ϣ [����ṹ����Ҫ��֤x86/x64һ���Ĵ�]
	//native_handle Ŀ�괰���� [NULL = ����ץȡ]
	//output_width x output_height ����ֱ��� [-1 = ��Դ���]
	//output_fps ���֡��[ -1 = ��Դ���]

	//����һ���ǳ���Ҫ�Ľṹ�壡
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

	//���ץȡ�Ļص�����ǩ��
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
