#include "MemoryCapture.h"
#include "D3D11RenderSystem.hpp"
#include "CPUTexture.h"
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
	case h3d::RGBA8:
		return AV_PIX_FMT_RGBA;
	case h3d::HDYC: case h3d::UYVY:
		return AV_PIX_FMT_UYVY422;
	case h3d::YUY2:
		return AV_PIX_FMT_YUYV422;
	case h3d::YVYU:
		return AV_PIX_FMT_YVYU422;
	case h3d::YV12:
		return AV_PIX_FMT_YUV420P;//It's error! need YVU420P
	case h3d::I420:
		return AV_PIX_FMT_YUV420P;
	}
	return AV_PIX_FMT_NONE;
}

HANDLE h3d::MemoryCapture::texture_mutexs[2] = { NULL,NULL };

h3d::MemoryCapture::MemoryCapture(CaptureInfo & info, CaptureCallBack callback)
	:capture_info(info),opt_callbak(callback),pInfo(NULL),mMemory(NULL),hMemoryMap(NULL),texture(NULL),sws_context(NULL)
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
	if (GetEngine().GetLevel() >= D3D_FEATURE_LEVEL_9_1) {
		texture = GetEngine().GetFactory().CreateTexture(info.oWidth, info.oHeight, (SWAPFORMAT)info.Reserved1, EA_CPU_WRITE | EA_GPU_READ);
	}
	else{
		texture = new MemoryTexture(info.oWidth, info.oHeight);

		ffmpeg_support = info.Reserved1 != R10G10B10A2 && info.Reserved1 != B10G10R10A2;
		if (ffmpeg_support)
			sws_context = sws_getContext(info.oWidth, info.oHeight, GetAVPixelFormat((SWAPFORMAT)info.Reserved1), info.oWidth, info.oHeight, AV_PIX_FMT_BGRA, SWS_LANCZOS, NULL, NULL, NULL);
		if (info.Reserved1 == R10G10B10A2) {
			B_MASK = 0X00000FFC; //B
			G_MASK = 0X003FF000;//G
			R_MASK = 0XFFC00000;//R
		}
		if (info.Reserved1 == B10G10R10A2) {
			R_MASK = 0X00000FFC; //R
			G_MASK = 0X003FF000;//G
			B_MASK = 0XFFC00000;//B
		}
	}

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

	if (sws_context)
		sws_freeContext((SwsContext*)sws_context);
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
			h3d::CaptureTexture::MappedData mapped = texture->Map();
			if (texture->GetType() == CaptureTexture::Memory_Texture) {
				int src_pitch = capture_info.Reserved4;
				int dst_pitch = static_cast<int>(mapped.RowPitch);
				if (ffmpeg_support)
					sws_scale((SwsContext*)sws_context, &TextureAddress[last_rendered], &src_pitch, 0,capture_info.oHeight, &mapped.pData, &dst_pitch);
				else
				{
					for (int y = 0; y !=capture_info.oHeight; ++y) {
						byte* dst_begin = mapped.pData + y*dst_pitch;
						int* src_begin = reinterpret_cast<int*>(TextureAddress[last_rendered] + y*src_pitch);
						for (int x = 0; x != capture_info.oWidth; ++x) {
							int src_pixel = *(src_begin + x);
							dst_begin[x * 4] = src_pixel & B_MASK;  //B
							dst_begin[x * 4 + 1] = src_pixel & G_MASK;//G
							dst_begin[x * 4 + 2] = src_pixel & R_MASK;//R
																	  //ignore a
						}
					}
				}
			}
			else {
				if (capture_info.Reserved4 == mapped.RowPitch)
					memcpy(mapped.pData, TextureAddress[last_rendered], mapped.RowPitch*texture->GetHeight());
				else
					for (int y = 0; y != capture_info.oHeight; ++y)
						memcpy(mapped.pData + y*mapped.RowPitch, 
							TextureAddress[last_rendered] + y*capture_info.Reserved4, 
							mapped.RowPitch < capture_info.Reserved4 ? mapped.RowPitch : capture_info.Reserved4);
			}
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



#ifndef DS_CAPTURE
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

#ifndef DS_CAPTURE
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

#ifndef DS_CAPTURE
	MFShutdown();
#endif
}
