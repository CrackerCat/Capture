#include "DSDevice.h"
#include "CameraCapture.h"
#include <dvdmedia.h>

#define FILTER_NAME     L"DirectShowDevice Filter"
#define VIDEO_PIN_NAME  L"Video Capture"
#define AUDIO_PIN_NAME  L"Audio Capture"


using namespace h3d;

#define SafeRelease(var) if(var) {var->Release(); var = NULL;}
static void WINAPI FreeMediaType(AM_MEDIA_TYPE& mt)
{
	if (mt.cbFormat != 0)
	{
		CoTaskMemFree((LPVOID)mt.pbFormat);
		mt.cbFormat = 0;
		mt.pbFormat = NULL;
	}

	SafeRelease(mt.pUnk);
}

static HRESULT WINAPI CopyMediaType(AM_MEDIA_TYPE *pmtTarget, const AM_MEDIA_TYPE *pmtSource)
{
	if (!pmtSource || !pmtTarget) return S_FALSE;

	*pmtTarget = *pmtSource;

	if (pmtSource->cbFormat && pmtSource->pbFormat)
	{
		pmtTarget->pbFormat = (PBYTE)CoTaskMemAlloc(pmtSource->cbFormat);
		if (pmtTarget->pbFormat == NULL)
		{
			pmtTarget->cbFormat = 0;
			return E_OUTOFMEMORY;
		}
		else
			memcpy(pmtTarget->pbFormat, pmtSource->pbFormat, pmtTarget->cbFormat);
	}

	if (pmtTarget->pUnk != NULL)
		pmtTarget->pUnk->AddRef();

	return S_OK;
}

static BITMAPINFOHEADER* GetVideoBMIHeader(const AM_MEDIA_TYPE *pMT)
{
	return (pMT->formattype == FORMAT_VideoInfo) ?
		&reinterpret_cast<VIDEOINFOHEADER*>(pMT->pbFormat)->bmiHeader :
		&reinterpret_cast<VIDEOINFOHEADER2*>(pMT->pbFormat)->bmiHeader;
}

h3d::DevicePin::DevicePin(DeviceFilter * _filter, DShowCapture * _device, const GUID & expectedMediaType, const GUID & expectedMajorType)
	:filter(_filter), device(_device), refCount(1)
{
	connectedMediaType.majortype = expectedMajorType;
	connectedMediaType.subtype = GUID_NULL;
	connectedMediaType.pbFormat = NULL;
	connectedMediaType.cbFormat = 0;
	connectedMediaType.pUnk = NULL;
	this->expectedMediaType = expectedMediaType;
	this->expectedMajorType = expectedMajorType;
}


DevicePin::~DevicePin()
{
	FreeMediaType(connectedMediaType);
}

STDMETHODIMP DevicePin::QueryInterface(REFIID riid, void **ppv)
{
	if (riid == IID_IUnknown || riid == IID_IPin)
	{
		AddRef();
		*ppv = (IPin*)this;
		return NOERROR;
	}
	else if (riid == IID_IMemInputPin)
	{
		AddRef();
		*ppv = (IMemInputPin*)this;
		return NOERROR;
	}
	else
	{
		*ppv = NULL;
		return E_NOINTERFACE;
	}
}

STDMETHODIMP_(ULONG) DevicePin::AddRef() { return ++refCount; }
STDMETHODIMP_(ULONG) DevicePin::Release() { if (!InterlockedDecrement(&refCount)) { delete this; return 0; } return refCount; }

// IPin methods
STDMETHODIMP DevicePin::Connect(IPin *pReceivePin, const AM_MEDIA_TYPE *pmt)
{
	if (filter->state == State_Running)
		return VFW_E_NOT_STOPPED;
	if (connectedPin)
		return VFW_E_ALREADY_CONNECTED;
	if (!pmt)
		return S_OK;
	if (pmt->majortype != GUID_NULL && pmt->majortype != expectedMajorType)
		return S_FALSE;
	if (pmt->majortype == expectedMajorType && !IsValidMediaType(pmt))
		return S_FALSE;

	return S_OK;
}

STDMETHODIMP DevicePin::ReceiveConnection(IPin *pConnector, const AM_MEDIA_TYPE *pmt)
{
	if (filter->state != State_Stopped)
		return VFW_E_NOT_STOPPED;
	if (!pConnector || !pmt)
		return E_POINTER;
	if (connectedPin)
		return VFW_E_ALREADY_CONNECTED;

	if (QueryAccept(pmt) != S_OK)
		return VFW_E_TYPE_NOT_ACCEPTED;

	connectedPin = pConnector;
	connectedPin->AddRef();

	FreeMediaType(connectedMediaType);
	return CopyMediaType(&connectedMediaType, pmt);
}

STDMETHODIMP DevicePin::Disconnect()
{
	if (!connectedPin)
		return S_FALSE;

	connectedPin->Release();
	connectedPin = NULL;
	return S_OK;
}


STDMETHODIMP DevicePin::ConnectedTo(IPin **pPin)
{
	if (!connectedPin)
		return VFW_E_NOT_CONNECTED;

	connectedPin->AddRef();
	*pPin = connectedPin;
	return S_OK;
}

STDMETHODIMP DevicePin::ConnectionMediaType(AM_MEDIA_TYPE *pmt)
{
	if (!connectedPin)
		return VFW_E_NOT_CONNECTED;

	return CopyMediaType(pmt, &connectedMediaType);
}

STDMETHODIMP DevicePin::QueryPinInfo(PIN_INFO *pInfo)
{
	pInfo->pFilter = filter;
	if (filter) filter->AddRef();

	if (expectedMajorType == MEDIATYPE_Video)
		memcpy(pInfo->achName, VIDEO_PIN_NAME, sizeof(VIDEO_PIN_NAME));
	else
		memcpy(pInfo->achName, AUDIO_PIN_NAME, sizeof(AUDIO_PIN_NAME));

	pInfo->dir = PINDIR_INPUT;

	return NOERROR;
}

STDMETHODIMP DevicePin::QueryDirection(PIN_DIRECTION *pPinDir) { *pPinDir = PINDIR_INPUT; return NOERROR; }
STDMETHODIMP DevicePin::QueryId(LPWSTR *lpId) { *lpId = L"Capture Pin"; return S_OK; }
STDMETHODIMP DevicePin::QueryAccept(const AM_MEDIA_TYPE *pmt)
{
	if (pmt->majortype != expectedMajorType)
		return S_FALSE;
	if (!IsValidMediaType(pmt))
		return S_FALSE;

	if (connectedPin)
	{
		FreeMediaType(connectedMediaType);
		CopyMediaType(&connectedMediaType, pmt);
	}

	return S_OK;
}

STDMETHODIMP DevicePin::EnumMediaTypes(IEnumMediaTypes **ppEnum)
{
	*ppEnum = new DeviceEnumMediaTypes(this, NULL);
	if (!*ppEnum)
		return E_OUTOFMEMORY;

	return NOERROR;
}

STDMETHODIMP DevicePin::QueryInternalConnections(IPin **apPin, ULONG *nPin) { return E_NOTIMPL; }
STDMETHODIMP DevicePin::EndOfStream() { return S_OK; }
STDMETHODIMP DevicePin::BeginFlush()
{
	device->FlushSamples();
	return S_OK;
}

STDMETHODIMP DevicePin::EndFlush()
{
	return S_OK;
}

STDMETHODIMP DevicePin::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate) { return S_OK; }

// IMemInputPin methods
STDMETHODIMP DevicePin::GetAllocator(IMemAllocator **ppAllocator) { return VFW_E_NO_ALLOCATOR; }
STDMETHODIMP DevicePin::NotifyAllocator(IMemAllocator *pAllocator, BOOL bReadOnly) { return S_OK; }
STDMETHODIMP DevicePin::GetAllocatorRequirements(ALLOCATOR_PROPERTIES *pProps) { return E_NOTIMPL; }

STDMETHODIMP DevicePin::Receive(IMediaSample *pSample)
{
	if (pSample)
	{
		if (expectedMajorType == MEDIATYPE_Video)
			device->ReceiveMediaSample(pSample, true);
		else if (expectedMajorType == MEDIATYPE_Audio)
			device->ReceiveMediaSample(pSample, false);
	}
	return S_OK;
}

STDMETHODIMP DevicePin::ReceiveMultiple(IMediaSample **pSamples, long nSamples, long *nSamplesProcessed)
{
	for (long i = 0; i<nSamples; i++)
		Receive(pSamples[i]);
	*nSamplesProcessed = nSamples;
	return S_OK;
}

STDMETHODIMP DevicePin::ReceiveCanBlock() { return S_OK; }

bool DevicePin::IsValidMediaType(const AM_MEDIA_TYPE *pmt) const
{
	if (pmt->pbFormat)
	{
		if (pmt->subtype != expectedMediaType || pmt->majortype != expectedMajorType)
			return false;

		if (expectedMajorType == MEDIATYPE_Video)
		{
			BITMAPINFOHEADER *bih = GetVideoBMIHeader(pmt);

			if (bih->biHeight == 0 || bih->biWidth == 0)
				return false;
		}
	}

	return true;
}


//========================================================================================================

class CaptureFlags : public IAMFilterMiscFlags {
	long refCount = 1;
public:
	inline CaptureFlags() {}
	STDMETHODIMP_(ULONG) AddRef() { return ++refCount; }
	STDMETHODIMP_(ULONG) Release() { if (!--refCount) { delete this; return 0; } return refCount; }
	STDMETHODIMP         QueryInterface(REFIID riid, void **ppv)
	{
		if (riid == IID_IUnknown)
		{
			AddRef();
			*ppv = (void*)this;
			return NOERROR;
		}

		*ppv = nullptr;
		return E_NOINTERFACE;
	}

	STDMETHODIMP_(ULONG) GetMiscFlags() { return AM_FILTER_MISC_FLAGS_IS_RENDERER; }
};

DeviceFilter::DeviceFilter(DShowCapture *source, const GUID &expectedMediaType, const GUID &expectedMajorType)
	: state(State_Stopped), refCount(1)
{
	pin = new DevicePin(this, source, expectedMediaType, expectedMajorType);
	flags = new CaptureFlags();
}

DeviceFilter::~DeviceFilter()
{
	pin->Release();
	flags->Release();
}

// IUnknown methods
STDMETHODIMP DeviceFilter::QueryInterface(REFIID riid, void **ppv)
{
	if (riid == IID_IUnknown)
	{
		AddRef();
		*ppv = (IUnknown*)this;
		return NOERROR;
	}
	else if (riid == IID_IPersist)
	{
		AddRef();
		*ppv = (IPersist*)this;
		return NOERROR;
	}
	else if (riid == IID_IMediaFilter)
	{
		AddRef();
		*ppv = (IMediaFilter*)this;
		return NOERROR;
	}
	else if (riid == IID_IBaseFilter)
	{
		AddRef();
		*ppv = (IBaseFilter*)this;
		return NOERROR;
	}
	else if (riid == IID_IAMFilterMiscFlags)
	{
		flags->AddRef();
		*ppv = flags;
		return NOERROR;
	}
	else
	{
		*ppv = NULL;
		return E_NOINTERFACE;
	}
}

STDMETHODIMP_(ULONG) DeviceFilter::AddRef() { return ++refCount; }
STDMETHODIMP_(ULONG) DeviceFilter::Release() { if (!InterlockedDecrement(&refCount)) { delete this; return 0; } return refCount; }

// IPersist method
STDMETHODIMP DeviceFilter::GetClassID(CLSID *pClsID) { return E_NOTIMPL; }

// IMediaFilter methods
STDMETHODIMP DeviceFilter::GetState(DWORD dwMSecs, FILTER_STATE *State) { *State = state; return S_OK; }
STDMETHODIMP DeviceFilter::SetSyncSource(IReferenceClock *pClock) { return S_OK; }
STDMETHODIMP DeviceFilter::GetSyncSource(IReferenceClock **pClock) { *pClock = NULL; return NOERROR; }
STDMETHODIMP DeviceFilter::Stop() { pin->EndFlush(); state = State_Stopped; return S_OK; }
STDMETHODIMP DeviceFilter::Pause() { state = State_Paused;  return S_OK; }
STDMETHODIMP DeviceFilter::Run(REFERENCE_TIME tStart) { state = State_Running; return S_OK; }

// IBaseFilter methods
STDMETHODIMP DeviceFilter::EnumPins(IEnumPins **ppEnum)
{
	*ppEnum = new DeviceEnumPins(this, NULL);
	return (*ppEnum == NULL) ? E_OUTOFMEMORY : NOERROR;
}

STDMETHODIMP DeviceFilter::FindPin(LPCWSTR Id, IPin **ppPin) { return E_NOTIMPL; }
STDMETHODIMP DeviceFilter::QueryFilterInfo(FILTER_INFO *pInfo)
{
	memcpy(pInfo->achName, FILTER_NAME, sizeof(FILTER_NAME));

	pInfo->pGraph = graph;
	if (graph) graph->AddRef();
	return NOERROR;
}

STDMETHODIMP DeviceFilter::JoinFilterGraph(IFilterGraph *pGraph, LPCWSTR pName) { graph = pGraph; return NOERROR; }
STDMETHODIMP DeviceFilter::QueryVendorInfo(LPWSTR *pVendorInfo) { return E_NOTIMPL; }


//========================================================================================================

DeviceEnumPins::DeviceEnumPins(DeviceFilter *filterIn, DeviceEnumPins *pEnum)
	: filter(filterIn), refCount(1)
{
	filter->AddRef();
	curPin = (pEnum != NULL) ? pEnum->curPin : 0;
}

DeviceEnumPins::~DeviceEnumPins()
{
	filter->Release();
}

// IUnknown
STDMETHODIMP DeviceEnumPins::QueryInterface(REFIID riid, void **ppv)
{
	if (riid == IID_IUnknown || riid == IID_IEnumPins)
	{
		AddRef();
		*ppv = (IEnumPins*)this;
		return NOERROR;
	}
	else
	{
		*ppv = NULL;
		return E_NOINTERFACE;
	}
}

STDMETHODIMP_(ULONG) DeviceEnumPins::AddRef() { return ++refCount; }
STDMETHODIMP_(ULONG) DeviceEnumPins::Release() { if (!InterlockedDecrement(&refCount)) { delete this; return 0; } return refCount; }

// IEnumPins
STDMETHODIMP DeviceEnumPins::Next(ULONG cPins, IPin **ppPins, ULONG *pcFetched)
{
	UINT nFetched = 0;

	if (curPin == 0 && cPins > 0)
	{
		IPin *pPin = filter->GetDevicePin();
		*ppPins = pPin;
		pPin->AddRef();
		nFetched = 1;
		curPin++;
	}

	if (pcFetched) *pcFetched = nFetched;

	return (nFetched == cPins) ? S_OK : S_FALSE;
}

STDMETHODIMP DeviceEnumPins::Skip(ULONG cPins) { return (++curPin > 1) ? S_FALSE : S_OK; }
STDMETHODIMP DeviceEnumPins::Reset() { curPin = 0; return S_OK; }
STDMETHODIMP DeviceEnumPins::Clone(IEnumPins **ppEnum)
{
	*ppEnum = new DeviceEnumPins(filter, this);
	return (*ppEnum == NULL) ? E_OUTOFMEMORY : NOERROR;
}


//========================================================================================================

DeviceEnumMediaTypes::DeviceEnumMediaTypes(DevicePin *pinIn, DeviceEnumMediaTypes *pEnum)
	: pin(pinIn), refCount(1)
{
	pin->AddRef();
}

DeviceEnumMediaTypes::~DeviceEnumMediaTypes()
{
	pin->Release();
}

STDMETHODIMP DeviceEnumMediaTypes::QueryInterface(REFIID riid, void **ppv)
{
	if (riid == IID_IUnknown || riid == IID_IEnumMediaTypes)
	{
		AddRef();
		*ppv = (IEnumMediaTypes*)this;
		return NOERROR;
	}
	else
	{
		*ppv = NULL;
		return E_NOINTERFACE;
	}
}

STDMETHODIMP_(ULONG) DeviceEnumMediaTypes::AddRef() { return ++refCount; }
STDMETHODIMP_(ULONG) DeviceEnumMediaTypes::Release() { if (!InterlockedDecrement(&refCount)) { delete this; return 0; } return refCount; }

// IEnumMediaTypes
STDMETHODIMP DeviceEnumMediaTypes::Next(ULONG cMediaTypes, AM_MEDIA_TYPE **ppMediaTypes, ULONG *pcFetched) { return S_FALSE; }
STDMETHODIMP DeviceEnumMediaTypes::Skip(ULONG cMediaTypes) { return S_FALSE; }
STDMETHODIMP DeviceEnumMediaTypes::Reset() { return S_OK; }
STDMETHODIMP DeviceEnumMediaTypes::Clone(IEnumMediaTypes **ppEnum)
{
	*ppEnum = new DeviceEnumMediaTypes(pin, this);
	return (*ppEnum == NULL) ? E_OUTOFMEMORY : NOERROR;
}
