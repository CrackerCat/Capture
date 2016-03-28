#ifndef VIDEOENGINECAPTURECAMERA_H
#define VIDEOENGINECAPTURECAMERA_H

#include "CaptureHUB.h"
#include "DSDevice.h"

struct SwsContext;

#ifndef DS_CAPTURE
struct IMFMediaSource;
struct IMFSourceReader;

#else
struct IBaseFilter;
struct IFilterGraph;
struct ISampleGrabber;
#include <list>
#endif
#pragma warning(disable:4251)
namespace h3d {

#ifdef DS_CAPTURE
	struct DShowSample {
		LPBYTE data;
		size_t size;

		LONGLONG timestamp;

		volatile long refs;

		DShowSample(LPBYTE pData, DWORD dwSize) {
			data = new BYTE[dwSize];
			size = dwSize;
			memcpy(data, pData, dwSize);
			refs = 1;
		}

		~DShowSample() {
			delete[] data;
		}

		void AddRef() { ++refs;};
		void Release() {
			if (!InterlockedDecrement(&refs))
				delete this;
		}
	};


	struct DShowCapture {
		virtual void FlushSamples() = 0;
		virtual void ReceiveMediaSample(IMediaSample *pSample, bool video) = 0;
	};
#endif

	class CameraCapture : public CaptureHUB
#ifdef DS_CAPTURE
		,public DShowCapture
#endif
	{
		CaptureCallBack opt_callbak;

		CaptureInfo capture_info;

		CameraCapture* pImpl;

		CaptureTexture* capture_tex;

		SwsContext* sws_context;

#ifndef DS_CAPTURE
		IMFMediaSource* pSource;
		IMFSourceReader* pReader;

#else
		IGraphBuilder* pGraph;
		ICaptureGraphBuilder2* pCapture;
		IMediaControl* pControl;

		IBaseFilter* pSource;
		DeviceFilter* pDevice;

		std::list<DShowSample*> samples;
		DShowSample* last_sample;
		CRITICAL_SECTION sample_mutex;

		void FlushSamples();
		void ReceiveMediaSample(IMediaSample *pSample, bool video);

		HANDLE stop_event;
		HANDLE sample_thread;
		DWORD static WINAPI SampleThread(CameraCapture* capture);

		CaptureTexture* prev_tex;//暂时不用，多线程加速!,交换链
#endif
	public:
		CameraCapture(const CaptureInfo & info,unsigned int Index,CaptureCallBack callback = 0);

		~CameraCapture();

		CaptureTexture* Capture() override;

		void Stop() override;
	private:
		friend std::list<CameraInfo> H3D_API GetCameraInfos();
		static std::list<CameraInfo> cameras;
	};
}

#endif
