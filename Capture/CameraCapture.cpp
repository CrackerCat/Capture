#include "CameraCapture.h"
#include "MemoryCapture.h"
using namespace h3d;
#ifndef _USING_V110_SDK71_

#include <mfapi.h>
#include <mfidl.h>
#include <Mfreadwrite.h>
#include "D3D11RenderSystem.hpp"

#include <assert.h>

extern "C" {
	//ffmmpeg不能支持所有的格式，可能需要考虑写一个转换
	//todo memory capture support R10G10B10A2
#include <libswscale/swscale.h>
}


std::list<CameraInfo> h3d::GetCameraInfos()
{
	std::list<CameraInfo> camera_list;

	IMFAttributes* pConfig = NULL;
	if (FAILED(MFCreateAttributes(&pConfig, 1)))
		return camera_list;

	pConfig->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);

	UINT32 count = 0;
	IMFActivate** ppDevices = NULL;
	if (FAILED(MFEnumDeviceSources(pConfig,&ppDevices,&count)))
		return camera_list;

	for (UINT i = 0; i != count; ++i) {
		wchar_t* szFriendlyName = NULL;
		UINT32 cchName;
		ppDevices[i]->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, &szFriendlyName, &cchName);
		camera_list.push_back({ szFriendlyName,i });
		CoTaskMemFree(szFriendlyName);
		ppDevices[i]->Release();
	}
	CoTaskMemFree(ppDevices);
	pConfig->Release();
	return camera_list;
}


h3d::CameraCapture::CameraCapture(const CaptureInfo & info, unsigned int Index, CaptureCallBack callback)
	:capture_info(info),opt_callbak(callback), capture_tex(NULL)
{
	IMFAttributes* pConfig = NULL;
	MFCreateAttributes(&pConfig, 1);
	assert(pConfig);
	pConfig->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
	UINT32 count = 0;
	IMFActivate** ppDevices = NULL;
	MFEnumDeviceSources(pConfig, &ppDevices, &count);
	assert(Index < count);

	ppDevices[Index]->ActivateObject(IID_PPV_ARGS(&pSource));
	for (UINT i = 0; i != count; ++i)
		ppDevices[i]->Release();
	CoTaskMemFree(ppDevices);
	pConfig->Release();


	IMFPresentationDescriptor* pPD = NULL;

	pSource->CreatePresentationDescriptor(&pPD);
	assert(pPD);

	IMFStreamDescriptor* pSD = NULL;
	BOOL fSelected;
	pPD->GetStreamDescriptorByIndex(0, &fSelected, &pSD);
	assert(pSD);

	IMFMediaTypeHandler* pHandler = NULL;
	pSD->GetMediaTypeHandler(&pHandler);
	assert(pHandler);

	DWORD cTypes = 0;
	pHandler->GetMediaTypeCount(&cTypes);

	{
		IMFMediaType* pType = NULL;
		//Set Video Capture Format
		for (DWORD i = 0; i != cTypes; ++i) {
			pHandler->GetMediaTypeByIndex(i, &pType);
			if (true)
				break;
			pType->Release();
		}

		if (pType)
		{
			UINT32 MaxNumerator,MaxDenominator;
			MFGetAttributeRatio(pType, MF_MT_FRAME_RATE_RANGE_MAX, &MaxNumerator, &MaxDenominator);
			UINT32 MinNumerator, MinDenominator;
			MFGetAttributeRatio(pType, MF_MT_FRAME_RATE_RANGE_MIN, &MinNumerator, &MinDenominator);

			UINT32 Width, Height;
			MFGetAttributeSize(pType, MF_MT_FRAME_SIZE, &Width, &Height);
			capture_info.oHeight = Height;
			
			sws_context = sws_getContext(Width, Height, AV_PIX_FMT_UYVY422, Width, Height, AV_PIX_FMT_BGRA, SWS_LANCZOS, NULL, NULL, NULL);
			if (GetEngine().GetLevel() >= D3D_FEATURE_LEVEL_9_1)
				capture_tex = GetEngine().GetFactory().CreateTexture(Width, Height, BGRA8, EA_CPU_WRITE | EA_GPU_READ);
			else
				capture_tex = new MemoryTexture(Width, Height);
		}

		if (pType)
			pHandler->SetCurrentMediaType(pType);

		if (pType)
			pType->Release();
	}

	pHandler->Release();
	pSD->Release();
	pPD->Release();

	IMFAttributes* pReaderConfig = NULL;
	MFCreateAttributes(&pReaderConfig, 1);
	pReaderConfig->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING,TRUE);

	MFCreateSourceReaderFromMediaSource(pSource, pReaderConfig, &pReader);
	pReaderConfig->Release();
	assert(pReader);
}

h3d::CameraCapture::~CameraCapture()
{
	pSource->Release();
	pReader->Release();
}

CaptureTexture * h3d::CameraCapture::Capture()
{
	DWORD streamIndex, flags;
	LONGLONG llTimeStamp;
	IMFSample* pSample = NULL;
	pReader->ReadSample(MF_SOURCE_READER_ANY_STREAM, 0, &streamIndex, &flags, &llTimeStamp, &pSample);
	if (pSample) {
		IMFMediaBuffer* pMediaBuffer = NULL;
		pSample->GetBufferByIndex(0, &pMediaBuffer);
		BYTE* pBuffer = NULL;
		DWORD cbCurrentLength = 0;
		pMediaBuffer->Lock(&pBuffer, NULL, &cbCurrentLength);

		int src_pitch = cbCurrentLength / capture_info.oHeight;

		CaptureTexture::MappedData mapped = capture_tex->Map();
		int dst_pitch = static_cast<int>(mapped.RowPitch);
		sws_scale(sws_context, &pBuffer, &src_pitch, 0, capture_info.oHeight, &mapped.pData, &dst_pitch);
		capture_tex->UnMap();

		pMediaBuffer->Unlock();

		pMediaBuffer->Release();
		pSample->Release();
	}
	return capture_tex;
}



#endif

#ifdef _USING_V110_SDK71_

std::list<CameraInfo> h3d::GetCameraInfos() {
	return std::list<CameraInfo>();
}

CameraCapture::CameraCapture(CaptureInfo & info, unsigned int Index, CaptureCallBack callback)
	:capture_info(info), opt_callbak(callback) {
}

CameraCapture::~CameraCapture()
{
}

CaptureTexture * h3d::CameraCapture::Capture()
{
	return nullptr;
}

#endif

void h3d::CameraCapture::Stop()
{
}


