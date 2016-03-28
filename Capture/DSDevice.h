#ifndef DIRECTSHOW_DEVICE_H
#define DIRECTSHOW_DEVICE_H
#include <dshow.h>

namespace h3d {

	class DeviceFilter;

	struct DShowCapture;

	class DevicePin : public IPin, public IMemInputPin
	{
		friend class DeviceEnumMediaTypes;

		DShowCapture* device;

		long refCount;

		GUID expectedMajorType;
		GUID expectedMediaType;
		DeviceFilter *filter;
		AM_MEDIA_TYPE connectedMediaType;
		IPin *connectedPin;

		bool IsValidMediaType(const AM_MEDIA_TYPE *pmt) const;

	public:
		DevicePin(DeviceFilter *filter, DShowCapture *capture, const GUID &expectedMediaType, const GUID &expectedMajorType = MEDIATYPE_Video);
		virtual ~DevicePin();

		// IUnknown methods
		STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
		STDMETHODIMP_(ULONG) AddRef();
		STDMETHODIMP_(ULONG) Release();

		// IPin methods
		STDMETHODIMP Connect(IPin *pReceivePin, const AM_MEDIA_TYPE *pmt);
		STDMETHODIMP ReceiveConnection(IPin *connector, const AM_MEDIA_TYPE *pmt);
		STDMETHODIMP Disconnect();
		STDMETHODIMP ConnectedTo(IPin **pPin);
		STDMETHODIMP ConnectionMediaType(AM_MEDIA_TYPE *pmt);
		STDMETHODIMP QueryPinInfo(PIN_INFO *pInfo);
		STDMETHODIMP QueryDirection(PIN_DIRECTION *pPinDir);
		STDMETHODIMP QueryId(LPWSTR *lpId);
		STDMETHODIMP QueryAccept(const AM_MEDIA_TYPE *pmt);
		STDMETHODIMP EnumMediaTypes(IEnumMediaTypes **ppEnum);
		STDMETHODIMP QueryInternalConnections(IPin* *apPin, ULONG *nPin);
		STDMETHODIMP EndOfStream();

		STDMETHODIMP BeginFlush();
		STDMETHODIMP EndFlush();
		STDMETHODIMP NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);

		// IMemInputPin methods
		STDMETHODIMP GetAllocator(IMemAllocator **ppAllocator);
		STDMETHODIMP NotifyAllocator(IMemAllocator *pAllocator, BOOL bReadOnly);
		STDMETHODIMP GetAllocatorRequirements(ALLOCATOR_PROPERTIES *pProps);
		STDMETHODIMP Receive(IMediaSample *pSample);
		STDMETHODIMP ReceiveMultiple(IMediaSample **pSamples, long nSamples, long *nSamplesProcessed);
		STDMETHODIMP ReceiveCanBlock();
	};

	class DeviceFilter : public IBaseFilter
	{
		friend class DevicePin;

		long refCount;

		FILTER_STATE state;
		IFilterGraph *graph;
		DevicePin *pin;
		IAMFilterMiscFlags *flags;

	public:
		DeviceFilter(DShowCapture *source, const GUID &expectedMediaType, const GUID &expectedMajorType = MEDIATYPE_Video);
		virtual ~DeviceFilter();

		// IUnknown methods
		STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
		STDMETHODIMP_(ULONG) AddRef();
		STDMETHODIMP_(ULONG) Release();

		// IPersist method
		STDMETHODIMP GetClassID(CLSID *pClsID);

		// IMediaFilter methods
		STDMETHODIMP GetState(DWORD dwMSecs, FILTER_STATE *State);
		STDMETHODIMP SetSyncSource(IReferenceClock *pClock);
		STDMETHODIMP GetSyncSource(IReferenceClock **pClock);
		STDMETHODIMP Stop();
		STDMETHODIMP Pause();
		STDMETHODIMP Run(REFERENCE_TIME tStart);

		// IBaseFilter methods
		STDMETHODIMP EnumPins(IEnumPins ** ppEnum);
		STDMETHODIMP FindPin(LPCWSTR Id, IPin **ppPin);
		STDMETHODIMP QueryFilterInfo(FILTER_INFO *pInfo);
		STDMETHODIMP JoinFilterGraph(IFilterGraph *pGraph, LPCWSTR pName);
		STDMETHODIMP QueryVendorInfo(LPWSTR *pVendorInfo);

		inline DevicePin* GetDevicePin() const { return pin; }
	};

	class DeviceEnumPins : public IEnumPins
	{
		long refCount;

		DeviceFilter *filter;
		UINT curPin;

	public:
		DeviceEnumPins(DeviceFilter *filterIn, DeviceEnumPins *pEnum);
		virtual ~DeviceEnumPins();

		// IUnknown
		STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
		STDMETHODIMP_(ULONG) AddRef();
		STDMETHODIMP_(ULONG) Release();

		// IEnumPins
		STDMETHODIMP Next(ULONG cPins, IPin **ppPins, ULONG *pcFetched);
		STDMETHODIMP Skip(ULONG cPins);
		STDMETHODIMP Reset();
		STDMETHODIMP Clone(IEnumPins **ppEnum);
	};

	class DeviceEnumMediaTypes : public IEnumMediaTypes
	{
		long refCount;

		DevicePin *pin;

	public:
		DeviceEnumMediaTypes(DevicePin *pinIn, DeviceEnumMediaTypes *pEnum);
		virtual ~DeviceEnumMediaTypes();

		// IUnknown
		STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
		STDMETHODIMP_(ULONG) AddRef();
		STDMETHODIMP_(ULONG) Release();

		// IEnumMediaTypes
		STDMETHODIMP Next(ULONG cMediaTypes, AM_MEDIA_TYPE **ppMediaTypes, ULONG * pcFetched);
		STDMETHODIMP Skip(ULONG cMediaTypes);
		STDMETHODIMP Reset();
		STDMETHODIMP Clone(IEnumMediaTypes **ppEnum);
	};

	inline static HRESULT GetUnconnectedPin(
		IBaseFilter *pFilter,   // Pointer to the filter.
		PIN_DIRECTION PinDir,   // Direction of the pin to find.
		int index,
		IPin **ppPin)           // Receives a pointer to the pin.
	{
		*ppPin = NULL;
		IEnumPins *pEnum = 0;
		IPin *pPin = 0;
		HRESULT hr = pFilter->EnumPins(&pEnum);
		if (FAILED(hr))
		{
			return hr;
		}
		while (pEnum->Next(1, &pPin, NULL) == S_OK)
		{
			PIN_DIRECTION ThisPinDir;
			pPin->QueryDirection(&ThisPinDir);
			if (ThisPinDir == PinDir)
			{
				IPin *pTmp = 0;
				hr = pPin->ConnectedTo(&pTmp);
				if (SUCCEEDED(hr))  // Already connected, not the pin we want.
				{
					pTmp->Release();
				}
				else  // Unconnected, this is the pin we want.
				{
					if (index == 0)
					{
						pEnum->Release();
						*ppPin = pPin;
						return S_OK;
					}
				}
				index--;
			}
			pPin->Release();
		}
		pEnum->Release();
		return E_FAIL;
	}
}

#endif
