#include "MemoryCapture.h"
#include <sstream>

extern "C" {
#include <libswscale/swscale.h>
}

AVPixelFormat GetAVPixelFormat(h3d::SWAPFORMAT format) {
	switch (format)
	{
	case h3d::RGB8:
		return AV_PIX_FMT_RGB24;
	case h3d::RGBA8:
		return AV_PIX_FMT_RGBA;
	case h3d::BGR8:
		return AV_PIX_FMT_BGR24;
	case h3d::BGRA8:
		return AV_PIX_FMT_BGRA;
	case h3d::RGBA16:
		return AV_PIX_FMT_RGBA64;
	case h3d::B5G6R5:
		return AV_PIX_FMT_RGB565;
	case h3d::HDYC:
		return AV_PIX_FMT_UYVY422;
	}
	return AV_PIX_FMT_NONE;
}

HANDLE h3d::MemoryCapture::texture_mutexs[2] = { NULL,NULL };

h3d::MemoryCapture::MemoryCapture(CaptureInfo & info, CaptureCallBack callback)
	:capture_info(info),opt_callbak(callback),pInfo(NULL),mMemory(NULL),hMemoryMap(NULL),texture(NULL)
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

	pInfo =(MemoryInfo*)mMemory;
	TextureAddress[0] =(PBYTE)mMemory + pInfo->Reserved1;
	TextureAddress[1] = (PBYTE)mMemory + pInfo->Reserved2;

	//Create Texture
	texture = new MemoryTexture(info.Reserved1, info.oWidth, info.oHeight);
}

h3d::MemoryCapture::~MemoryCapture()
{
	pInfo = NULL;
	TextureAddress[0] = NULL;
	TextureAddress[1] = NULL;

	if (mMemory)
		UnmapViewOfFile(mMemory);

	if (hMemoryMap)
		CloseHandle(hMemoryMap);

	if (texture)
		delete texture;
}

#pragma warning(disable:4800)
#pragma warning(disable:4804)

h3d::CaptureTexture * h3d::MemoryCapture::Capture()
{
	bool last_rendered = pInfo->Reserved3;
	if (last_rendered < 2) {
		bool next_rendered = !last_rendered;

		HANDLE hMutex = NULL;
		if (WaitForSingleObject(texture_mutexs[last_rendered], 0) == WAIT_OBJECT_0)
			hMutex = texture_mutexs[last_rendered];
		else  if (WaitForSingleObject(texture_mutexs[next_rendered], 0) == WAIT_OBJECT_0) {
			hMutex = texture_mutexs[next_rendered];
			last_rendered = next_rendered;
		}

		if (hMutex) {
			texture->WriteData(TextureAddress[last_rendered], capture_info.Reserved4);

			ReleaseMutex(hMutex);
		}
	}

	return texture;
}

void h3d::MemoryCapture::Stop()
{
	std::wstringstream wss;
	wss << EVENT_OBS_STOP << capture_info.sPID;
	HANDLE hStopEvent = OpenEventW(EVENT_ALL_ACCESS, FALSE, wss.str().c_str());
	if (!hStopEvent)
		return;//Target Process Exited

	if (hStopEvent)
		SetEvent(hStopEvent);
}

#pragma warning(disable:4244)
h3d::MemoryTexture::MemoryTexture(UINT format, SDst width, SDst height)
	:src_format(format),cx(width),cy(height)
{
	sws_context = sws_getContext(cx, cy, GetAVPixelFormat((SWAPFORMAT)format), cx, cy, AV_PIX_FMT_BGRA, SWS_LANCZOS, NULL, NULL, NULL);
	if (!sws_context)
		throw std::runtime_error("format unsupport");

	native = new byte[width*height * 4];
}

h3d::MemoryTexture::~MemoryTexture()
{
	sws_freeContext((SwsContext*)sws_context);
	delete[] native;
}

h3d::MemoryTexture::MappedData h3d::MemoryTexture::Map()
{
	return{native,cx*4};
}

void h3d::MemoryTexture::ReSize(SDst width, SDst height)
{
	cx = width;
	cy = height;
	delete[] native;
	native = new byte[width*height * 4];
}

void h3d::MemoryTexture::WriteData(LPBYTE pData, int pitch)
{
	int linear = cx * 4;
	sws_scale((SwsContext*)sws_context, &pData, &pitch, 0, cy, &native, &linear);
}

#include <mfapi.h>

#include <fstream>
extern std::ofstream logstream;
std::ofstream logstream;

bool h3d::LoadPlugin() {
	MemoryCapture::texture_mutexs[0] = CreateMutex(NULL, NULL, TEXTURE_FIRST_MUTEX);
	if (!MemoryCapture::texture_mutexs[0])
		return false;
	MemoryCapture::texture_mutexs[1] = CreateMutex(NULL, NULL, TEXTURE_SECOND_MUTEX);
	if (!MemoryCapture::texture_mutexs[1])
		return false;
	MFStartup(MF_VERSION);

	logstream.open("Capture.log", std::ios_base::in | std::ios_base::out | std::ios_base::trunc, 0X40);

	return true;
}

void h3d::UnLoadPlugin() {
	CloseHandle(MemoryCapture::texture_mutexs[0]);
	CloseHandle(MemoryCapture::texture_mutexs[1]);

	if (logstream.is_open())
		logstream.close();

	MFShutdown();
}
