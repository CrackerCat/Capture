#include "MemoryCapture.h"
#include <sstream>

extern "C" {
	//ffmmpeg不能支持所有的格式，可能需要考虑写一个转换
	//todo memory capture support R10G10B10A2
#include <libswscale/swscale.h>
}

AVPixelFormat GetAVPixelFormat(h3d::SWAPFORMAT format) {
	switch (format)
	{
	case h3d::BGRA8:
	case h3d::BGRX8:
		return AV_PIX_FMT_BGRA;
	case h3d::B5G6R5:case h3d::B5G6R5A1:case h3d::B5G6R5X1:
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
	ffmpeg_support = src_format != R10G10B10A2 && src_format != B10G10R10A2;
	if(ffmpeg_support)
		sws_context = sws_getContext(cx, cy, GetAVPixelFormat((SWAPFORMAT)format), cx, cy, AV_PIX_FMT_BGRA, SWS_LANCZOS, NULL, NULL, NULL);
	if (src_format == R10G10B10A2) {
		B_MASK = 0X00000FFC; //B
		G_MASK = 0X003FF000;//G
		R_MASK = 0XFFC00000;//R
	}
	if (src_format == B10G10R10A2) {
		R_MASK = 0X00000FFC; //R
		G_MASK = 0X003FF000;//G
		B_MASK = 0XFFC00000;//B
	}

	native = new byte[width*height * 4];
}

h3d::MemoryTexture::~MemoryTexture()
{
	if(sws_context)
		sws_freeContext((SwsContext*)sws_context);
	delete[] native;
}

h3d::MemoryTexture::MappedData h3d::MemoryTexture::Map()
{
	return{native,static_cast<unsigned long>(cx*4)};
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
	if (ffmpeg_support)
		sws_scale((SwsContext*)sws_context, &pData, &pitch, 0, cy, &native, &linear);
	else
	{
		for (int y = 0; y != cy; ++y) {
			byte* dst_begin = native + y*linear;
			int* src_begin =reinterpret_cast<int*>(pData + y*pitch);
			for (int x = 0; x != cx; ++x) {
				int src_pixel = *(src_begin + x);
				dst_begin[x * 4] = src_pixel & B_MASK;  //B
				dst_begin[x * 4 + 1] = src_pixel & G_MASK;//G
				dst_begin[x * 4 + 2] = src_pixel & R_MASK;//R
				//ignore a
			}
		}
	}
}

#ifndef _USING_V110_SDK71_
#include <mfapi.h>
#endif

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

#ifndef _USING_V110_SDK71_
	MFStartup(MF_VERSION);
#endif

	logstream.open("Capture.log",std::ios_base::trunc);

	return true;
}

void h3d::UnLoadPlugin() {
	CloseHandle(MemoryCapture::texture_mutexs[0]);
	CloseHandle(MemoryCapture::texture_mutexs[1]);

	if (logstream.is_open())
		logstream.close();

#ifndef _USING_V110_SDK71_
	MFShutdown();
#endif
}
