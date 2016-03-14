#include "TextureCapture.h"
#include <sstream>

h3d::TextureCapture::TextureCapture(CaptureInfo & info, CaptureCallBack callback)
	:capture_info(info),opt_callbak(callback)
{
	std::wstringstream wss;
	wss << TEXTURE_MEMORY << info.Reserved2;

	hMemoryMap = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, wss.str().c_str());
	if (!hMemoryMap) {
		throw std::runtime_error("MemoryCapture Can't Find TEXTURE_MEMORY Map");
	}
	mMemory = MapViewOfFile(hMemoryMap, FILE_MAP_ALL_ACCESS, 0, 0, info.Reserved3);
	if (!mMemory) {
		throw std::runtime_error("MemoryCapture CaptureInfo Param Error");
	}

	unsigned __int64 far_handle = *reinterpret_cast<unsigned __int64*>(mMemory);
	shared_tex = GetEngine().GetFactory().CreateTexture(info.oWidth, info.oHeight, far_handle);
	capture_tex = GetEngine().GetFactory().CreateTexture(info.oWidth, info.oHeight,(SWAPFORMAT)info.Reserved1);
}

h3d::TextureCapture::~TextureCapture()
{
	if (capture_tex)
		delete capture_tex;
	capture_tex = NULL;

	if (shared_tex)
		delete shared_tex;
	shared_tex = NULL;

	if (mMemory)
		UnmapViewOfFile(mMemory);

	if (hMemoryMap)
		CloseHandle(hMemoryMap);
}

h3d::D3D11Texture * h3d::TextureCapture::Capture()
{
	capture_tex->Assign(shared_tex);
	return capture_tex;
}

#pragma warning(disable:4390)
void h3d::TextureCapture::Stop()
{
	std::wstringstream wss;
	wss << EVENT_OBS_STOP << capture_info.sPID;
	HANDLE hStopEvent = OpenEventW(EVENT_ALL_ACCESS, FALSE, wss.str().c_str());
	if (!hStopEvent)
		;//Target Process Exited

	if (hStopEvent)
		SetEvent(hStopEvent);
}
