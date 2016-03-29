#include "CameraCapture.h"
#include "CPUTexture.h"
#include "D3D11RenderSystem.hpp"

#include <fstream>

extern "C" {
	//ffmmpeg不能支持所有的格式，可能需要考虑写一个转换
	//todo memory capture support R10G10B10A2
#include <libswscale/swscale.h>
}


extern std::ofstream logstream;
using namespace h3d;
#ifndef DS_CAPTURE

#include <mfapi.h>
#include <mfidl.h>
#include <Mfreadwrite.h>
#pragma comment(lib,"Mfreadwrite.lib")
#pragma comment(lib,"Mf.lib")
#pragma comment(lib,"Mfplat.lib")
#pragma comment(lib,"mfuuid.lib")


#include <assert.h>




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
			
			GUID format;
			HRESULT hr=  pType->GetGUID(MF_MT_SUBTYPE, &format);
			AVPixelFormat  av_format;
			if (format == MFVideoFormat_YUY2)
				av_format = AV_PIX_FMT_YUYV422;
			else
				av_format = AV_PIX_FMT_UYVY422;
			sws_context = sws_getContext(Width, Height, av_format, Width, Height, AV_PIX_FMT_BGRA, SWS_LANCZOS, NULL, NULL, NULL);
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

#ifdef DS_CAPTURE
#include <dshow.h>

#include <atlbase.h>
#include <atlcom.h>

#include "IVideoCaptureFilter.h"
#pragma comment(lib,"Strmiids.lib")


std::list<CameraInfo> CameraCapture::cameras;

std::list<CameraInfo> h3d::GetCameraInfos() {
	HRESULT hr;
	ATL::CComPtr<ICreateDevEnum> create_dev_enum = NULL;
	hr = create_dev_enum.CoCreateInstance(CLSID_SystemDeviceEnum);
	if (FAILED(hr) || NULL == create_dev_enum)
	{
		return CameraCapture::cameras;
	}

	ATL::CComPtr<IEnumMoniker> enum_moniker = NULL;
	hr = create_dev_enum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &enum_moniker, 0);
	if (FAILED(hr) || NULL == enum_moniker)
	{
		return CameraCapture::cameras;
	}
	enum_moniker->Reset();

	CameraCapture::cameras.clear();
	while (true)
	{
		ATL::CComPtr<IMoniker> moniker = NULL;
		ULONG ulFetched = 0;
		hr = enum_moniker->Next(1, &moniker, &ulFetched);
		if (FAILED(hr) || NULL == moniker)
		{
			break;
		}

		ATL::CComPtr<IPropertyBag> pBag = NULL;
		hr = moniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pBag);
		if (FAILED(hr) || NULL == pBag)
		{
			continue;
		}

		ATL::CComVariant var;
		var.vt = VT_BSTR;
		pBag->Read(L"FriendlyName", &var, NULL);

		LPOLESTR wszDeviceId;
		moniker->GetDisplayName(0, NULL, &wszDeviceId);
		CameraCapture::cameras.push_back({ var.bstrVal,CameraCapture::cameras.size(),wszDeviceId});
		CoTaskMemFree(wszDeviceId);
	}

	return CameraCapture::cameras;
}

#define SR(var) if(var) {var->Release(); var = NULL;}

static void DeleteMediaType(AM_MEDIA_TYPE *pmt)
{
	if (pmt != NULL)
	{
		if (pmt->cbFormat != 0)
		{
			CoTaskMemFree((LPVOID)pmt->pbFormat);
			pmt->cbFormat = 0;
			pmt->pbFormat = NULL;
		}

		SR(pmt->pUnk);
		CoTaskMemFree(pmt);
	}
}

const GUID MEDIASUBTYPE_I420 = { 0x30323449, 0x0000, 0x0010,{ 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 } };

AVPixelFormat GetAVPixelFormat(h3d::SWAPFORMAT format);

CameraCapture::CameraCapture(const CaptureInfo & info, unsigned int Index, CaptureCallBack callback)
	:capture_info(info), opt_callbak(callback),
	pGraph(NULL), pCapture(NULL),pSource(NULL),pDevice(NULL),pControl(NULL),
	last_sample(NULL),stop_event(NULL),sample_thread(NULL),
	capture_tex(NULL),prev_tex(NULL),
	sws_context(NULL){
	if (Index >= cameras.size())
		throw std::runtime_error("Error Device Index");

	
	ATL::CComPtr<IBindCtx> bind_ctx;
	if (FAILED(::CreateBindCtx(0, &bind_ctx)))
		throw std::runtime_error("Error CreateBindCtx");
	
	std::list<CameraInfo>::iterator iter = cameras.begin();
	std::advance(iter, Index);

	ULONG ulFetched;
	ATL::CComPtr<IMoniker> moniker;
	if (FAILED(::MkParseDisplayName(bind_ctx, iter->Id.c_str(), &ulFetched, &moniker)))
		throw std::runtime_error("Error MkParseDisplayName");
	
	// ask for the actual filter
	if (FAILED(moniker->BindToObject(0, 0, IID_IBaseFilter,reinterpret_cast<void**>(&pSource))))
		throw std::runtime_error("Error MkParseDisplayName");

	CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, IID_IFilterGraph, reinterpret_cast<void**>(&pGraph));

	CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pCapture));

	pCapture->SetFiltergraph(pGraph);

	ATL::CComPtr<IPin> pin = NULL;
	GetUnconnectedPin(pSource, PINDIR_OUTPUT, 0, &pin);
	
	//todo support elgato
	bool elgato_device = iter->Name.find(L"elgato") != std::wstring::npos;
	ATL::CComPtr<IElgatoVideoCaptureFilter6> elgato_interface = NULL; //EGC v 2.20
	pSource->QueryInterface(&elgato_interface);

	//todo get output type match it
	ATL::CComPtr<IAMStreamConfig> config;
	pin->QueryInterface(&config);
	AM_MEDIA_TYPE* pmt = NULL;
	config->GetFormat(&pmt);

	if (pmt->formattype == FORMAT_VideoInfo) {
		VIDEOINFOHEADER *vih = (VIDEOINFOHEADER *)pmt->pbFormat;
		capture_info.oWidth = vih->bmiHeader.biWidth;
		capture_info.oHeight = vih->bmiHeader.biHeight;
		capture_info.oFps = 10000000 / vih->AvgTimePerFrame;
	}

	//TODO support CPU/GPU convert //窄意摄像头支持的格式
	capture_info.Reserved1 = BGRX8;
	if (pmt->subtype == MEDIASUBTYPE_I420 || pmt->subtype == MEDIASUBTYPE_IYUV)
		capture_info.Reserved1 = I420;
	else if (pmt->subtype == MEDIASUBTYPE_YV12)
		capture_info.Reserved1 = YV12;
	else if (pmt->subtype == MEDIASUBTYPE_YVYU)
		capture_info.Reserved1 = YVYU;
	else if (pmt->subtype == MEDIASUBTYPE_YUY2)
		capture_info.Reserved1 = YUY2;
	else if (pmt->subtype == MEDIASUBTYPE_UYVY)
		capture_info.Reserved1 = UYVY;
	else {
		VIDEOINFOHEADER *vih = (VIDEOINFOHEADER *)pmt->pbFormat;
		switch (vih->bmiHeader.biCompression)
		{
		case 'CYDH':
			capture_info.Reserved1 = HDYC; //709 space
		}
	}

	GUID media_type = pmt->subtype;
	DeleteMediaType(pmt);

	//use some threads

	pDevice = new DeviceFilter(this, media_type);
	HRESULT hr = pGraph->AddFilter(pDevice, NULL);
	hr = pGraph->AddFilter(pSource, NULL);


	//Connect Pins
	if (elgato_device && !elgato_interface) {
		if (FAILED(pGraph->ConnectDirect(pin, pDevice->GetDevicePin(), NULL)))
			logstream << "Error: Failed ConnectDirect" << std::endl;
	}
	else {
		if (FAILED(pCapture->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, pSource, NULL, pDevice))) {
			if (FAILED(pGraph->Connect(pin, pDevice->GetDevicePin())))
				logstream << "Error: Failed RenderStream&&Connect" << std::endl;
		}
	}

	hr = pGraph->QueryInterface(&pControl);

	hr = pControl->Run();

	InitializeCriticalSection(&sample_mutex);

	stop_event = CreateEvent(NULL, FALSE, FALSE, NULL);
	sample_thread = CreateThread(NULL, 0,(LPTHREAD_START_ROUTINE)SampleThread, this, 0, NULL);

	sws_context = sws_getContext(capture_info.oWidth, capture_info.oHeight, GetAVPixelFormat((SWAPFORMAT)capture_info.Reserved1), capture_info.oWidth, capture_info.oHeight, AV_PIX_FMT_BGRA, SWS_LANCZOS, NULL, NULL, NULL);

	//Create Texture
	if (GetEngine().GetLevel() >= D3D_FEATURE_LEVEL_9_1) {
		capture_tex = GetEngine().GetFactory().CreateTexture(capture_info.oWidth, capture_info.oHeight, BGRA8, EA_CPU_WRITE | EA_GPU_READ);
	}
	else {
		capture_tex = new MemoryTexture(capture_info.oWidth, capture_info.oHeight);
	}


	logstream << "CameraCapture Finish" << std::endl;
}

CameraCapture::~CameraCapture()
{

	if (sample_thread) {
		SetEvent(stop_event);
		WaitForSingleObject(sample_thread, INFINITE);
		CloseHandle(sample_thread);
		CloseHandle(stop_event);
	}

	if (pGraph && pSource && pDevice) {
		pGraph->RemoveFilter(pSource);
		pGraph->RemoveFilter(pDevice);
	}

	DeleteCriticalSection(&sample_mutex);

	SR(pGraph);
	SR(pSource);
	SR(pCapture);
	SR(pControl);

	delete pDevice;
}

void h3d::CameraCapture::FlushSamples()
{
	EnterCriticalSection(&sample_mutex);
	std::list<DShowSample*>::iterator iter = samples.begin();
	for (; iter != samples.end(); ++iter)
		delete *iter;
	samples.clear();
	SR(last_sample);
	LeaveCriticalSection(&sample_mutex);
}

void h3d::CameraCapture::ReceiveMediaSample(IMediaSample * pSample, bool video)
{
	if (!pSample)
		return;

	if (video) {
		if (!pSample->GetActualDataLength())
			return;

		BYTE* pointer = NULL;
		if (SUCCEEDED(pSample->GetPointer(&pointer))) {
			DShowSample* sample = new DShowSample(pointer, pSample->GetActualDataLength());
			LONGLONG stop_time;
			pSample->GetTime(&stop_time, &sample->timestamp);
			EnterCriticalSection(&sample_mutex);
			samples.push_back(sample);
			LeaveCriticalSection(&sample_mutex);
		}
	}
}

DWORD WINAPI h3d::CameraCapture::SampleThread(CameraCapture * capture)
{
	//todo accord to timestamp remove sample
	while (WaitForSingleObject(capture->stop_event, 2) == WAIT_TIMEOUT) {
		EnterCriticalSection(&capture->sample_mutex);

		if (capture->samples.size() > 2) {
			SR(capture->last_sample);
			capture->last_sample = capture->samples.front();
			capture->samples.pop_front();
		}

		LeaveCriticalSection(&capture->sample_mutex);
	}

	return 0;
}


CaptureTexture * h3d::CameraCapture::Capture()
{
	DShowSample* curr_sample = NULL;

	EnterCriticalSection(&sample_mutex);
	curr_sample = last_sample;
	if(curr_sample)
		curr_sample->AddRef();
	LeaveCriticalSection(&sample_mutex);

	if (!curr_sample)
		return capture_tex;

	CaptureTexture::MappedData mapped = capture_tex->Map();
	
	int src_pitch = curr_sample->size / capture_info.oHeight;
	int dst_pitch = static_cast<int>(mapped.RowPitch);

	//YV12 I420P error src_pitch should be a array contain 3 elements! data also
	sws_scale(sws_context, &curr_sample->data, &src_pitch, 0, capture_info.oHeight, &mapped.pData, &dst_pitch);
	capture_tex->UnMap();
	SR(curr_sample);

	//注意，这个情况比较复杂
	//Win7 D3D11 有一部分格式可以在GPU转换，有一部分得用CPU 做一次转换先 ，然后问题是。。。这样Capture接口就有问题了，Scene不能决定Capture的Texture的渲染行为
	//XP D3D9 ??
	//现在统一使用ffmpeg，我对其效率表示堪忧

	return capture_tex;
}

void h3d::CameraCapture::Stop()
{
	pControl->Stop();
	FlushSamples();
}

#endif

