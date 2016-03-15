#include "CaptureHook.h"
#include "Extern.h"
#include <ddraw.h>

using h3d::APIHook;
using h3d::CaptureInfo;
using h3d::MemoryInfo;

APIHook ddrawSurfaceCreate;
APIHook ddrawSurfaceFlip;
APIHook ddrawSurfaceRelease;
APIHook ddrawSurfaceRestore;
APIHook ddrawSurfaceSetPalette;
APIHook ddrawSurfaceBlt;
APIHook ddrawSurfaceBltFast;
APIHook ddrawSurfaceLock;
APIHook ddrawSurfaceUnlock;
APIHook ddrawPaletteSetEntries;

CaptureInfo ddraw_captureinfo;

#define NUM_BUFFERS 3
#define ZERO_ARRAY {0, 0, 0}

namespace {
	CRITICAL_SECTION ddraw_mutex;
	LPBYTE           tex_address[2];
	MemoryInfo   *copy_info;
	DWORD            curCapture;
	HANDLE           copy_thread;
	HANDLE           copy_event;
	BOOL             has_texture;
	bool             bKillThread;
	DWORD            curCPUTexture;
	DWORD            copy_wait;
	LONGLONG         lastTime;

	bool bGotSurfaceDesc = false;
	bool bLockingSurface = false;

	DWORD g_dwSize = 0; // bytesize of buffers
	DWORD g_dwCaptureSize = 0; // bytesize of memory capture buffer

	LPDIRECTDRAWSURFACE7    ddCaptures[NUM_BUFFERS] = ZERO_ARRAY;
	HANDLE                  ddUnlockFctMutex = 0;
	LPDIRECTDRAWSURFACE7    g_frontSurface = NULL;
	LPDDSURFACEDESC2        g_surfaceDesc;
	LPDIRECTDRAW7           g_ddInterface = NULL;
	LPCTSTR                 mutexName = TEXT("Global\\ddUnlockFunctionMutex");

	BOOL use_flip = false;
	BOOL g_bUse32bitCapture = false;
	BOOL g_bConvert16to32 = false;
	BOOL g_bUsePalette = false;

}
HRESULT STDMETHODCALLTYPE Blt(LPDIRECTDRAWSURFACE7, LPRECT, LPDIRECTDRAWSURFACE7, LPRECT, DWORD, LPDDBLTFX);
ULONG STDMETHODCALLTYPE Release(LPDIRECTDRAWSURFACE7 surface);
void HookAll();
void UnDoAll();

unsigned int getCodeFromHRESULT(HRESULT hr)
{
	return (unsigned int)(((unsigned long)0xFFFF) & hr);
}

#include <string>
#include <sstream>
void printDDrawError(HRESULT err, const char* callerName = "")
{
	if (err == DD_OK)
		return;

	std::string s;
	switch (err)
	{
	case DDERR_CANTCREATEDC: s = "can't create dc"; break;
	case DDERR_CANTLOCKSURFACE: s = "can't lock surface"; break;
	case DDERR_CLIPPERISUSINGHWND: s = "clipper is using hwnd"; break;
	case DDERR_COLORKEYNOTSET: s = "color key not set"; break;
	case DDERR_CURRENTLYNOTAVAIL: s = "currently not available"; break;
	case DDERR_DCALREADYCREATED: s = "dc already created"; break;
	case DDERR_DEVICEDOESNTOWNSURFACE: s = "device does not own surface"; break;
	case DDERR_DIRECTDRAWALREADYCREATED: s = "direct draw already created"; break;
	case DDERR_EXCLUSIVEMODEALREADYSET: s = "exclusive mode already set"; break;
	case DDERR_GENERIC: s = "undefined error"; break;
	case DDERR_HEIGHTALIGN: s = "height alignment is not a multiple of required alignment"; break;
	case DDERR_IMPLICITLYCREATED: s = "cannot restore surface, implicitly created"; break;
	case DDERR_INCOMPATIBLEPRIMARY: s = "incompatible primary"; break;
	case DDERR_INVALIDCAPS: s = "invalid caps"; break;
	case DDERR_INVALIDCLIPLIST: s = "invalid cliplist"; break;
	case DDERR_INVALIDMODE: s = "invalid mode"; break;
	case DDERR_INVALIDOBJECT: s = "invalid object"; break;
	case DDERR_INVALIDPARAMS: s = "invalid params"; break;
	case DDERR_INVALIDPIXELFORMAT: s = "invalid pixel format"; break;
	case DDERR_INVALIDPOSITION: s = "invalid position"; break;
	case DDERR_INVALIDRECT: s = "invalid rect"; break;
	case DDERR_LOCKEDSURFACES: s = "surface(s) locked"; break;
	case DDERR_MOREDATA: s = "data size exceeds buffer size"; break;
	case DDERR_NOALPHAHW: s = "no alpha acceleration hw available"; break;
	case DDERR_NOBLTHW: s = "no hw blit available"; break;
	case DDERR_NOCLIPLIST: s = "no cliplist available"; break;
	case DDERR_NOCLIPPERATTACHED: s = "no clipper attached"; break;
	case DDERR_NOCOLORCONVHW: s = "no color conversion hw available"; break;
	case DDERR_NOCOLORKEYHW: s = "no hw support for dest color key"; break;
	case DDERR_NOCOOPERATIVELEVELSET: s = "no cooperative level set"; break;
	case DDERR_NODC: s = "no dc available"; break;
	case DDERR_NOEXCLUSIVEMODE: s = "application does not have exclusive mode"; break;
	case DDERR_NOFLIPHW: s = "no hw flip available"; break;
	case DDERR_NOOVERLAYDEST: s = "GetOverlayPosition() is called but UpdateOverlay() has not been called"; break;
	case DDERR_NOOVERLAYHW: s = "no hw overlay available"; break;
	case DDERR_NOPALETTEATTACHED: s = "no palette attached"; break;
	case DDERR_NORASTEROPHW: s = "no raster operation hw available"; break;
	case DDERR_NOSTRETCHHW: s = "no hw support for stretching available"; break;
	case DDERR_NOTAOVERLAYSURFACE: s = "not an overlay component"; break;
	case DDERR_NOTFLIPPABLE: s = "surface not flippable"; break;
	case DDERR_NOTFOUND: s = "item not found"; break;
	case DDERR_NOTLOCKED: s = "surface not locked"; break;
	case DDERR_NOTPALETTIZED: s = "surface is not palette-based"; break;
	case DDERR_NOVSYNCHW: s = "no vsync hw available"; break;
	case DDERR_NOZOVERLAYHW: s = "not supported (NOZOVERLAYHW)"; break;
	case DDERR_OUTOFCAPS: s = "hw needed has already been allocated"; break;
	case DDERR_OUTOFMEMORY: s = "out of memory"; break;
	case DDERR_OUTOFVIDEOMEMORY: s = "out of video memory"; break;
	case DDERR_OVERLAPPINGRECTS: s = "overlapping rects on surface"; break;
	case DDERR_OVERLAYNOTVISIBLE: s = "GetOverlayPosition() is called on a hidden overlay"; break;
	case DDERR_PALETTEBUSY: s = "palette is locked by another thread"; break;
	case DDERR_PRIMARYSURFACEALREADYEXISTS: s = "primary surface already exists"; break;
	case DDERR_REGIONTOOSMALL: s = "region too small"; break;
	case DDERR_SURFACEBUSY: s = "surface is locked by another thread"; break;
	case DDERR_SURFACELOST: s = "surface lost"; break;
	case DDERR_TOOBIGHEIGHT: s = "requested height too large"; break;
	case DDERR_TOOBIGSIZE: s = "requested size too large"; break;
	case DDERR_TOOBIGWIDTH: s = "requested width too large"; break;
	case DDERR_UNSUPPORTED: s = "operation unsupported"; break;
	case DDERR_UNSUPPORTEDFORMAT: s = "unsupported pixel format"; break;
	case DDERR_VERTICALBLANKINPROGRESS: s = "vertical blank in progress"; break;
	case DDERR_VIDEONOTACTIVE: s = "video port not active"; break;
	case DDERR_WASSTILLDRAWING: s = "blit operation incomplete"; break;
	case DDERR_WRONGMODE: s = "surface cannot be restored because it was created in a different mode"; break;
	default:  logstream << "unknown error: " << getCodeFromHRESULT(err) << std::endl; return;
	}

	std::stringstream msg;
	msg << callerName << " DDraw Error: " << s << std::endl;
	logstream << msg.str().c_str();
}

class Palette
{
	DWORD numEntries;

	void destroy()
	{
		if (entries)
			delete[] entries;

		if (primary_surface_palette_ref)
			primary_surface_palette_ref->Release();

		entries = NULL;
		numEntries = 0;
		bInitialized = false;
		primary_surface_palette_ref = NULL;
	}

public:
	LPDIRECTDRAWPALETTE primary_surface_palette_ref;
	LPPALETTEENTRY entries;
	BOOL bInitialized;

	Palette()
		: numEntries(0), entries(NULL), primary_surface_palette_ref(NULL)
	{
	}

	~Palette()
	{
		destroy();
	}

	void Reallocate(DWORD size)
	{
		Free();

		numEntries = size;
		entries = new PALETTEENTRY[size];
		bInitialized = true;
	}

	void Free()
	{
		bool bWait = true;

		// try locking mutexes so we don't accidently free memory used by copythread
		HANDLE mutexes[2];
		for (int i = 0; i < 2; i++)
		{
			if (!DuplicateHandle(GetCurrentProcess(), texture_mutex[i], GetCurrentProcess(), &mutexes[i], NULL, FALSE, DUPLICATE_SAME_ACCESS))
			{
				logstream << "Palette::Free(): could not duplicate textureMutex handles, assuming OBS was closed and copy thread finished" << std::endl;
				bWait = false;
				break;
			}
		}

		// wait 2 seconds before deleting memory
		DWORD ret = WaitForMultipleObjects(2, mutexes, TRUE, 2000);

		destroy();

		if (bWait)
		{
			if (ret == WAIT_OBJECT_0)
			{
				ReleaseMutex(mutexes[0]);
				ReleaseMutex(mutexes[1]);
			}

			CloseHandle(mutexes[0]);
			CloseHandle(mutexes[1]);
		}
	}

	void ReloadPrimarySurfacePaletteEntries()
	{
		if (!primary_surface_palette_ref)
			return;

		HRESULT err;
		ddrawPaletteSetEntries.UnDo();
		if (FAILED(err = primary_surface_palette_ref->SetEntries(0, 0, numEntries, entries)))
		{
			logstream << "ReloadPrimarySurfacePaletteEntries(): could not set entires" << std::endl;
			printDDrawError(err);
		}
		ddrawPaletteSetEntries.ReDo();
	}

} g_CurrentPalette;

bool SetupPalette(LPDIRECTDRAWPALETTE lpDDPalette)
{
	if (!lpDDPalette)
	{
		//logstream << "Detaching palette" << std::endl;
		g_CurrentPalette.Free();
		g_bUsePalette = false;
		return false;
	}

	logstream << "initializing palette" << std::endl;

	DWORD caps;
	HRESULT hr;
	if (FAILED(hr = lpDDPalette->GetCaps(&caps)))
	{
		logstream << "Failed to get palette caps" << std::endl;
		printDDrawError(hr, "SetupPalette");
		return false;
	}

	if (caps & DDPCAPS_8BITENTRIES)
	{
		logstream << "8-bit index-palette used (lookup color is an index to another lookup table), not implemented" << std::endl;
		return false;
	}

	DWORD numEntries = 0;
	if (caps & DDPCAPS_8BIT)
		numEntries = 0x100;
	else if (caps & DDPCAPS_4BIT)
		numEntries = 0x10;
	else if (caps & DDPCAPS_2BIT)
		numEntries = 0x04;
	else if (caps & DDPCAPS_1BIT)
		numEntries = 0x02;
	else
	{
		logstream << "Unrecognized palette format" << std::endl;
		return false;
	}

	//logstream << "Trying to retrieve " << numEntries << " palette entries" << std::endl;

	g_CurrentPalette.Reallocate(numEntries);
	hr = lpDDPalette->GetEntries(0, 0, numEntries, g_CurrentPalette.entries);
	if (FAILED(hr))
	{
		logstream << "Failed to retrieve palette entries" << std::endl;
		printDDrawError(hr, "SetupPalette");
		g_CurrentPalette.Free();
		return false;
	}

	g_CurrentPalette.primary_surface_palette_ref = lpDDPalette;
	g_CurrentPalette.bInitialized = true;
	g_bUsePalette = true;

	logstream << "Initialized palette with " << numEntries << " color entries" << std::endl;

	return true;
}

void CleanUpDDraw()
{
	logstream << "Cleaning up" << std::endl;


	if (copy_thread)
	{
		bKillThread = true;
		SetEvent(copy_event);
		if (WaitForSingleObject(copy_thread, 500) != WAIT_OBJECT_0)
			TerminateThread(copy_thread, -1);

		CloseHandle(copy_thread);
		CloseHandle(copy_event);

		copy_thread = NULL;
		copy_event = NULL;
	}

	ddrawSurfaceRelease.UnDo();
	for (int i = 0; i < NUM_BUFFERS; i++)
	{
		if (ddCaptures[i])
		{
			ddCaptures[i]->Release();
			ddCaptures[i] = NULL;
		}
	}
	ddrawSurfaceRelease.ReDo();

	h3d::DestroySharedMem();

	has_texture = false;
	curCapture = 0;
	curCPUTexture = 0;
	copy_wait = 0;
	lastTime = 0;
	g_frontSurface = NULL;
	use_flip = false;
	target_acquired = false;
	g_dwSize = 0;
	g_bUse32bitCapture = false;
	g_bConvert16to32 = false;
	g_bUsePalette = false;
	g_dwCaptureSize = 0;

	if (g_ddInterface)
	{
		g_ddInterface->Release();
		g_ddInterface = NULL;
	}

	g_CurrentPalette.Free();

	if (g_surfaceDesc)
		delete g_surfaceDesc;
	g_surfaceDesc = NULL;

	if (ddUnlockFctMutex)
	{
		CloseHandle(ddUnlockFctMutex);
		ddUnlockFctMutex = 0;
	}


	logstream << "---------------------- Cleared DirectDraw Capture ----------------------" << std::endl;
}

void handleBufferConversion(LPDWORD dst, LPBYTE src, LONG pitch)
{
	//logstream << "trying to download buffer" << std::endl;

	DWORD advance = 0;

	if (g_bUsePalette)
	{
		if (!g_CurrentPalette.bInitialized)
		{
			logstream << "current palette not initialized" << std::endl;
			return;
		}

		//logstream << "Using palette" << std::endl;

		advance = pitch - g_surfaceDesc->dwWidth;

		for (UINT y = 0; y < ddraw_captureinfo.oHeight; y++)
		{
			for (UINT x = 0; x < ddraw_captureinfo.oWidth; x++)
			{
				// use color lookup table
				const PALETTEENTRY& entry = g_CurrentPalette.entries[*src];
				*dst = 0xFF000000 + (entry.peRed << 16) + (entry.peGreen << 8) + entry.peBlue;

				src++;
				dst++;
			}
			src += advance;
		}
	}
	else if (g_bConvert16to32)
	{
		//logstream << "Converting 16bit R5G6B5 to 32bit ARGB" << std::endl;

		advance = pitch / 2 - g_surfaceDesc->dwWidth;

		LPWORD buf = (LPWORD)src;
		for (UINT y = 0; y < ddraw_captureinfo.oHeight; y++)
		{
			for (UINT x = 0; x < ddraw_captureinfo.oWidth; x++)
			{
				// R5G6B5 format
				WORD pxl = *buf;
				*dst = 0xFF000000 + ((0xF800 & pxl) << 8) + ((0x07E0 & pxl) << 5) + ((0x001F & pxl) << 3);

				buf++;
				dst++;
			}
			buf += advance;
		}
	}
	else if (g_bUse32bitCapture)
	{
		// no conversion needed
		UINT width = 4 * g_surfaceDesc->dwWidth;
		for (UINT y = 0; y < ddraw_captureinfo.oHeight; y++)
		{
			memcpy(dst, src, width);
			dst += g_surfaceDesc->dwWidth;
			src += g_surfaceDesc->lPitch;
		}
	}
}

inline HRESULT STDMETHODCALLTYPE UnlockSurface(LPDIRECTDRAWSURFACE7 surface, LPRECT data)
{
	//logstream << "Called UnlockSurface" << std::endl;

	// standard handler
	if (!target_acquired)
	{
		ddrawSurfaceUnlock.UnDo();
		HRESULT hr = surface->Unlock(data);
		ddrawSurfaceUnlock.ReDo();
		return hr;
	}

	// use mutex lock to prevent memory corruption when calling UnDo/ReDo from multiple threads
	HANDLE mutex = OpenMutex(SYNCHRONIZE, FALSE, mutexName);
	if (!mutex)
	{
		logstream << "Could not open mutex - Error(" << GetLastError() << ')' << std::endl;
		return DDERR_GENERIC;
	}

	DWORD ret = WaitForSingleObject(mutex, INFINITE);
	if (ret == WAIT_OBJECT_0)
	{
		ddrawSurfaceUnlock.UnDo();
		HRESULT hr = surface->Unlock(data);
		ddrawSurfaceUnlock.ReDo();

		ReleaseMutex(mutex);
		CloseHandle(mutex);
		return hr;
	}
	else
	{
		logstream << "error while waiting for unlock-mutex to get signaled" << std::endl;
		logstream << "GetLastError: " << GetLastError() << std::endl;
		CloseHandle(mutex);
		return DDERR_GENERIC;
	}
}

#include "DXGI_D3D10_D3D11.h"
void CaptureDDraw()
{
	//RUNEVERYRESET logstream << "called CaptureDDraw()" << std::endl;

	h3d::EventProcess();

	if (!capture_run) {
		CleanUpDDraw();
	}

	KeepAliveProcess(CleanUpDDraw())

		if (has_texture && capture_run)
		{
			//todo mantian fps

			HRESULT hr;
			ddrawSurfaceBlt.UnDo();
			hr = ddCaptures[curCapture]->Blt(NULL, g_frontSurface, NULL, DDBLT_ASYNC, NULL);
			ddrawSurfaceBlt.ReDo();
			if (SUCCEEDED(hr))
			{
				DWORD nextCapture = (curCapture == NUM_BUFFERS - 1) ? 0 : (curCapture + 1);
				curCPUTexture = curCapture;
				curCapture = nextCapture;

				SetEvent(copy_event);
			}
			else
			{
				printDDrawError(hr, "CaptureDDraw");
			}

		}
}

DWORD CopyDDrawTextureThread(LPVOID lpUseless)
{
	int sharedMemID = 0;

	HANDLE hEvent = NULL;
	if (!DuplicateHandle(GetCurrentProcess(), copy_event, GetCurrentProcess(), &hEvent, NULL, FALSE, DUPLICATE_SAME_ACCESS))
	{
		logstream << "CopyDDrawTextureThread: couldn't duplicate event handle - " << GetLastError() << std::endl;
		return 0;
	}

	logstream <<"CopyDDrawTextureThread: waiting for copyEvents"<<std::endl;
	while (WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0)
	{
		//logstream << "CopyDDrawTextureThread: received copyEvent" << std::endl;
		if (bKillThread)
			break;

		int nextSharedMemID = sharedMemID == 0 ? 1 : 0;

		DWORD copyTex = curCPUTexture;
		if (copyTex < NUM_BUFFERS)
		{
			//logstream << "CopyDDrawTextureThread: processing data" << std::endl;
			int lastRendered = -1;

			//copy to whichever is available
			if (WaitForSingleObject(texture_mutex[sharedMemID], 0) == WAIT_OBJECT_0)
				lastRendered = (int)sharedMemID;
			else if (WaitForSingleObject(texture_mutex[nextSharedMemID], 0) == WAIT_OBJECT_0)
				lastRendered = (int)nextSharedMemID;

			if (lastRendered != -1)
			{
				//logstream << "CopyDDrawTextureThread: copying to memfile" << std::endl;
				HRESULT err;
				DDSURFACEDESC2 desc;
				desc.dwSize = sizeof(desc);
				DWORD flags = DDLOCK_WAIT | DDLOCK_READONLY | DDLOCK_NOSYSLOCK;
				//logstream << "CopyDDrawTextureThread: locking buffer" << std::endl;
				if (SUCCEEDED(err = ddCaptures[copyTex]->Lock(NULL, &desc, flags, NULL)))
				{
					handleBufferConversion((LPDWORD)tex_address[lastRendered], (LPBYTE)desc.lpSurface, desc.lPitch);
					if (FAILED(err = ddCaptures[copyTex]->Unlock(NULL)))
					{
						printDDrawError(err, "CopyDDrawTextureThread");
					}
					
				}
				else
				{
					printDDrawError(err, "CopyDDrawTextureThread");
				}
				ReleaseMutex(texture_mutex[lastRendered]);
				copy_info->Reserved3 = (UINT)lastRendered;
			}
		}

		sharedMemID = nextSharedMemID;
	}

	logstream << "CopyDDrawTextureThread: killed"<<std::endl;

	CloseHandle(hEvent);
	return 0;
}

bool SetupDDraw()
{
	logstream << "called SetupDDraw()" << std::endl;

	if (!g_ddInterface)
	{
		logstream << "SetupDDraw: DirectDraw interface not set, returning" << std::endl;
		return false;
	}

	bool bSuccess = true;

	bKillThread = false;

	if (copy_thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CopyDDrawTextureThread, NULL, 0, NULL))
	{
		if (!(copy_event = CreateEvent(NULL, FALSE, FALSE, NULL)))
		{
			logstream << "SetupDDraw: CreateEvent failed, GetLastError = " << GetLastError() << std::endl;
			bSuccess = false;
		}
	}
	else
	{
		logstream << "SetupDDraw: CreateThread failed, GetLastError = " << GetLastError() << std::endl;
		bSuccess = false;
	}

	if (bSuccess)
	{
		if (!ddUnlockFctMutex)
		{
			ddUnlockFctMutex = CreateMutex(NULL, FALSE, mutexName);
			if (!ddUnlockFctMutex)
			{
				bSuccess = false;
			}
		}
	}

	if (bSuccess && !g_frontSurface)
	{
		CleanUpDDraw();
		return false;
	}
	else if (bSuccess)
	{
		LPDIRECTDRAWPALETTE palette = NULL;
		HRESULT err;
		if (SUCCEEDED(err = g_frontSurface->GetPalette(&palette)))
		{
			if (palette)
				SetupPalette(palette);
		}
		else if (err == DDERR_NOPALETTEATTACHED)
		{
			//logstream << "No palette attached to primary surface" << std::endl;
		}
		else
		{
			logstream << "Error retrieving palette" << std::endl;
			printDDrawError(err, "getFrontSurface");
		}
	}

	if (bSuccess && !g_surfaceDesc)
	{
		logstream << "SetupDDraw: no surface descriptor found, creating a new one (not an error)" << std::endl;

		g_surfaceDesc = new DDSURFACEDESC2;
		g_surfaceDesc->dwSize = sizeof(DDSURFACEDESC);

		HRESULT hr;
		if (FAILED(hr = ((LPDIRECTDRAWSURFACE)g_frontSurface)->GetSurfaceDesc((LPDDSURFACEDESC)g_surfaceDesc)))
		{
			g_surfaceDesc->dwSize = sizeof(DDSURFACEDESC2);
			if (FAILED(g_frontSurface->GetSurfaceDesc(g_surfaceDesc)))
			{
				logstream << "SetupDDraw: error getting surface descriptor" << std::endl;
				printDDrawError(hr, "SetupDDraw");
				bSuccess = false;
			}
		}
	}

	if (bSuccess && g_surfaceDesc)
	{
		const DDPIXELFORMAT& pf = g_surfaceDesc->ddpfPixelFormat;
		if (pf.dwFlags & DDPF_RGB)
		{
			if (pf.dwRGBBitCount == 16)
			{
				logstream << "SetupDDraw: found 16bit format (using R5G6B5 conversion)" << std::endl;
				g_bConvert16to32 = true;
			}
			else if (pf.dwRGBBitCount == 32)
			{
				logstream << "SetupDDraw: found 32bit format (using plain copy)" << std::endl;
				g_bUse32bitCapture = true;
			}
		}
		else if (pf.dwFlags & (DDPF_PALETTEINDEXED8 | DDPF_PALETTEINDEXED4 | DDPF_PALETTEINDEXED2 | DDPF_PALETTEINDEXED1))
		{
			logstream << "SetupDDraw: front surface uses palette indices" << std::endl;
		}
	}

	if (bSuccess)
	{
		logstream << "SetupDDraw: primary surface width = " << g_surfaceDesc->dwWidth << ", height = " << g_surfaceDesc->dwHeight << std::endl;
		g_dwSize = g_surfaceDesc->lPitch*g_surfaceDesc->dwHeight;
		ddraw_captureinfo.oWidth = g_surfaceDesc->dwWidth;
		ddraw_captureinfo.oHeight = g_surfaceDesc->dwHeight;
		ddraw_captureinfo.Reserved4 = 4 * ddraw_captureinfo.oWidth;
		ddraw_captureinfo.Reserved1 = h3d::BGRA8;
		DWORD g_dwCaptureSize = ddraw_captureinfo.Reserved4*ddraw_captureinfo.oHeight;
		ddraw_captureinfo.Reserved2 = h3d::CtorSharedMemCPUCapture(g_dwCaptureSize,ddraw_captureinfo.Reserved3,copy_info, tex_address);

		memcpy(ptr_capture_info, &ddraw_captureinfo, sizeof(CaptureInfo));

		DDSURFACEDESC2 captureDesc;
		ZeroMemory(&captureDesc, sizeof(captureDesc));
		captureDesc.dwSize = sizeof(DDSURFACEDESC2);

		captureDesc.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_PIXELFORMAT | DDSD_PITCH;
		captureDesc.dwWidth = g_surfaceDesc->dwWidth;
		captureDesc.dwHeight = g_surfaceDesc->dwHeight;
		captureDesc.lPitch = g_surfaceDesc->lPitch;
		captureDesc.ddpfPixelFormat = g_surfaceDesc->ddpfPixelFormat;
		captureDesc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;

		HRESULT err;

		ddrawSurfaceCreate.UnDo();
		for (int i = 0; i < NUM_BUFFERS && bSuccess; i++)
		{
			if (FAILED(err = g_ddInterface->CreateSurface(&captureDesc, &ddCaptures[i], NULL)))
			{
				logstream << "SetupDDraw: Could not create offscreen capture" << std::endl;
				printDDrawError(err, "SetupDDraw");
				bSuccess = false;
				break;
			}
		}
		ddrawSurfaceCreate.ReDo();

		if (bSuccess)
		{
			has_texture = true;

			SetEvent(hReadyEvent);

		}
	}

	if (bSuccess)
	{
		logstream << "SetupDDraw successfull" << std::endl;
		HookAll();
		return true;
	}
	else
	{
		logstream << "SetupDDraw failed" << std::endl;
		CleanUpDDraw();
		return false;
	}
}

// set frontSurface to lpDDSurface if lpDDSurface is primary
// returns true if frontSurface is set
// returns false if frontSurface is NULL and lpDDSurface is not primary
bool getFrontSurface(LPDIRECTDRAWSURFACE7 lpDDSurface)
{
	//logstream << "called getFrontSurface" << std::endl;

	if (!lpDDSurface)
	{
		//logstream << "lpDDSurface null" << std::endl;
		return false;
	}

	if (!g_ddInterface)
	{
		LPDIRECTDRAWSURFACE7 dummy;
		if (lpDDSurface->QueryInterface(IID_IDirectDrawSurface7, (LPVOID*)&dummy) == S_OK)
		{
			IUnknown* Unknown;
			HRESULT err;
			if (FAILED(err = dummy->GetDDInterface((LPVOID*)&Unknown)))
			{
				logstream << "getFrontSurface: could not get DirectDraw interface" << std::endl;
				printDDrawError(err, "getFrontSurface");
			}
			else
			{
				if (Unknown->QueryInterface(IID_IDirectDraw7, (LPVOID*)&g_ddInterface) == S_OK)
				{
					logstream << "Got DirectDraw interface pointer" << std::endl;
				}
				else
				{
					logstream << "Query of DirectDraw interface failed" << std::endl;
				}
			}
			ddrawSurfaceRelease.UnDo();
			dummy->Release();
			ddrawSurfaceRelease.ReDo();
		}
	}

	if (!target_acquired)
	{
		DDSCAPS2 caps;
		if (SUCCEEDED(lpDDSurface->GetCaps(&caps)))
		{
			//logstream << "checking if surface is primary" << std::endl;
			if (caps.dwCaps & DDSCAPS_PRIMARYSURFACE)
			{
				logstream << "found primary surface" << std::endl;
				g_frontSurface = lpDDSurface;
				if (!SetupDDraw())
				{
					return false;
				}
				else
				{
					target_acquired = true;
				}
			}
		}
		else
		{
			logstream << "could not retrieve caps" << std::endl;
		}
	}

	return lpDDSurface == g_frontSurface;
}

HRESULT STDMETHODCALLTYPE Unlock(LPDIRECTDRAWSURFACE7 surface, LPRECT data)
{
	HRESULT hr;

	//logstream << "Hooked Unlock()" << std::endl;

	EnterCriticalSection(&ddraw_mutex);

	//UnlockSurface handles the actual unlocking and un-/ReDos the method (as well as thread synchronisation)
	hr = UnlockSurface(surface, data);

	if (getFrontSurface(surface))
	{
		// palette fix, should be tested on several machines
		if (g_bUsePalette && g_CurrentPalette.bInitialized)
			//g_CurrentPalette.ReloadPrimarySurfacePaletteEntries();
			if (!use_flip)
				CaptureDDraw();
	}

	LeaveCriticalSection(&ddraw_mutex);

	return hr;
}

HRESULT STDMETHODCALLTYPE SetPalette(LPDIRECTDRAWSURFACE7 surface, LPDIRECTDRAWPALETTE lpDDPalette)
{
	//logstream << "Hooked SetPalette()" << std::endl;

	ddrawSurfaceSetPalette.UnDo();
	HRESULT hr = surface->SetPalette(lpDDPalette);
	ddrawSurfaceSetPalette.ReDo();

	if (getFrontSurface(surface))
	{
		if (lpDDPalette)
			lpDDPalette->AddRef();
		SetupPalette(lpDDPalette);
	}

	return hr;
}

HRESULT STDMETHODCALLTYPE PaletteSetEntries(LPDIRECTDRAWPALETTE palette, DWORD dwFlags, DWORD dwStartingEntry, DWORD dwCount, LPPALETTEENTRY lpEntries)
{
	//logstream << "Hooked SetEntries()" << std::endl;

	ddrawPaletteSetEntries.UnDo();
	HRESULT hr = palette->SetEntries(dwFlags, dwStartingEntry, dwCount, lpEntries);
	ddrawPaletteSetEntries.ReDo();

	// update buffer palette
	if (SUCCEEDED(hr))
	{
		if (g_CurrentPalette.bInitialized)
		{
			memcpy(g_CurrentPalette.entries + dwStartingEntry, lpEntries, 4 * dwCount); // each entry is 4 bytes if DDCAPS_8BITENTRIES flag is not set
		}
	}

	return hr;
}

HRESULT STDMETHODCALLTYPE CreateSurface(IDirectDraw7* ddInterface, LPDDSURFACEDESC2 lpDDSurfaceDesc, LPDIRECTDRAWSURFACE7 *lplpDDSurface, IUnknown *pUnkOuter)
{
	//logstream << "Hooked CreateSurface()" << std::endl;

	if (!g_ddInterface)
	{
		if (ddInterface->QueryInterface(IID_IDirectDraw, (LPVOID*)&g_ddInterface) == S_OK)
		{
			logstream << "IDirectDraw::CreateSurface: got DDInterface pointer" << std::endl;
		}
		else
		{
			logstream << "IDirectDraw::CreateSurface: could not query DirectDraw interface" << std::endl;
		}
	}

	ddrawSurfaceCreate.UnDo();
	HRESULT hRes = ddInterface->CreateSurface(lpDDSurfaceDesc, lplpDDSurface, pUnkOuter);
	ddrawSurfaceCreate.ReDo();

	if (hRes == DD_OK)
	{
		if (lpDDSurfaceDesc->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
		{
			logstream << "IDirectDraw::CreateSurface: Primary surface created at 0x" << *lplpDDSurface << std::endl;
			getFrontSurface(*lplpDDSurface);
		}
	}
	else
		printDDrawError(hRes, "CreateSurface");

	return hRes;
}

ULONG STDMETHODCALLTYPE Release(LPDIRECTDRAWSURFACE7 surface)
{
	//logstream << "Hooked Release()" << std::endl;

	ddrawSurfaceRelease.UnDo();
	/*
	if(surface == g_frontSurface)
	{
	logstream << "Releasing primary surface 0x" << surface << std::endl;
	surface->AddRef();
	ULONG refCount = surface->Release();

	if(refCount == 1)
	{
	logstream << "Freeing primary surface" << std::endl;
	g_frontSurface = NULL;
	target_acquired = false;
	CleanUpDDraw();
	}
	}
	*/

	ULONG refCount = surface->Release();
	ddrawSurfaceRelease.ReDo();

	if (surface == g_frontSurface)
	{
		if (refCount == 0)
		{
			logstream << "Freeing primary surface" << std::endl;
			CleanUpDDraw();
		}
	}

	return refCount;
}

HRESULT STDMETHODCALLTYPE Restore(LPDIRECTDRAWSURFACE7 surface)
{

	ddrawSurfaceRestore.UnDo();
	HRESULT hr = surface->Restore();

	if (has_texture)
	{
		if (surface == g_frontSurface)
		{
			logstream << "SurfaceRestore: restoring offscreen buffers" << std::endl;
			bool success = true;
			for (UINT i = 0; i < NUM_BUFFERS; i++)
			{
				HRESULT err;
				if (FAILED(err = ddCaptures[i]->Restore()))
				{
					logstream << "SurfaceRestore: error restoring offscreen buffer" << std::endl;
					printDDrawError(err, "Restore");
					success = false;
				}
			}
			if (!success)
			{
				CleanUpDDraw();
			}
		}
	}
	ddrawSurfaceRestore.ReDo();

	if (!has_texture)
	{
		getFrontSurface(surface);
	}

	return hr;
}

HRESULT STDMETHODCALLTYPE Flip(LPDIRECTDRAWSURFACE7 surface, LPDIRECTDRAWSURFACE7 lpDDSurface, DWORD flags)
{

	HRESULT hr;

	EnterCriticalSection(&ddraw_mutex);

	ddrawSurfaceFlip.UnDo();
	hr = surface->Flip(lpDDSurface, flags);
	ddrawSurfaceFlip.ReDo();

	use_flip = true;

	if (getFrontSurface(surface))
	{
		CaptureDDraw();
	}

	LeaveCriticalSection(&ddraw_mutex);

	return hr;
}

HRESULT STDMETHODCALLTYPE Blt(LPDIRECTDRAWSURFACE7 surface, LPRECT lpDestRect, LPDIRECTDRAWSURFACE7 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx)
{

	EnterCriticalSection(&ddraw_mutex);

	ddrawSurfaceBlt.UnDo();
	HRESULT hr = surface->Blt(lpDestRect, lpDDSrcSurface, lpSrcRect, dwFlags, lpDDBltFx);
	ddrawSurfaceBlt.ReDo();

	if (SUCCEEDED(hr))
	{
		if (!use_flip)
		{
			if (getFrontSurface(surface))
			{
				CaptureDDraw();
			}
		}
	}

	LeaveCriticalSection(&ddraw_mutex);

	return hr;
}

void UnDoAll()
{
	ddrawSurfaceCreate.UnDo();
	ddrawSurfaceRestore.UnDo();
	ddrawSurfaceRelease.UnDo();
	ddrawSurfaceSetPalette.UnDo();
	ddrawPaletteSetEntries.UnDo();
}

typedef HRESULT(WINAPI *CREATEDIRECTDRAWEXPROC)(GUID*, LPVOID*, REFIID, IUnknown*);

#include "../Capture/WinPlatform.h"

bool InitDDrawCapture()
{
	using h3d::PTR;
	bool versionSupported = false;
	HMODULE hDDrawLib = NULL;
	if (hDDrawLib = GetModuleHandle(TEXT("ddraw.dll")))
	{
		bool isWin7min = h3d::GetOSVersion() >= 7;
		bool isWin8min = h3d::GetOSVersion() >= 8;

		PTR libBaseAddr = PTR(hDDrawLib);
		PTR surfCreateOffset;
		PTR surfUnlockOffset;
		PTR surfReleaseOffset;
		PTR surfRestoreOffset;
		PTR surfBltOffset;
		PTR surfFlipOffset;
		PTR surfSetPaletteOffset;
		PTR palSetEntriesOffset;


		if (isWin7min && !isWin8min)
		{
			surfCreateOffset = 0x617E;
			surfUnlockOffset = 0x4C40;
			surfReleaseOffset = 0x3239;
			surfRestoreOffset = 0x3E9CB;
			surfBltOffset = surfCreateOffset + 0x44F63;
			surfFlipOffset = surfCreateOffset + 0x37789;
			surfSetPaletteOffset = surfCreateOffset + 0x4D2D3;
			palSetEntriesOffset = surfCreateOffset + 0x4CE68;
			versionSupported = true;
		}
		else if (isWin8min)
		{
			surfCreateOffset = 0x9530 + 0xC00;
			surfUnlockOffset = surfCreateOffset + 0x2A1D0;
			surfReleaseOffset = surfCreateOffset - 0x1A80;
			surfRestoreOffset = surfCreateOffset + 0x36000;
			surfBltOffset = surfCreateOffset + 0x438DC;
			surfFlipOffset = surfCreateOffset + 0x33EF3;
			surfSetPaletteOffset = surfCreateOffset + 0x4D3B8;
			palSetEntriesOffset = surfCreateOffset + 0x4CF4C;
			versionSupported = true;	// some crash bugs remaining
		}


		if (versionSupported)
		{
			ddrawSurfaceCreate.Do((FARPROC)(libBaseAddr + surfCreateOffset), (FARPROC)CreateSurface);
			ddrawSurfaceRestore.Do((FARPROC)(libBaseAddr + surfRestoreOffset), (FARPROC)Restore);
			ddrawSurfaceRelease.Do((FARPROC)(libBaseAddr + surfReleaseOffset), (FARPROC)Release);
			ddrawSurfaceSetPalette.Do((FARPROC)(libBaseAddr + surfSetPaletteOffset), (FARPROC)SetPalette);
			ddrawPaletteSetEntries.Do((FARPROC)(libBaseAddr + palSetEntriesOffset), (FARPROC)PaletteSetEntries);

			ddrawSurfaceCreate.UnDo();
			ddrawSurfaceRestore.UnDo();
			ddrawSurfaceRelease.UnDo();
			ddrawSurfaceSetPalette.UnDo();
			ddrawPaletteSetEntries.UnDo();

			ddrawSurfaceUnlock.Do((FARPROC)(libBaseAddr + surfUnlockOffset), (FARPROC)Unlock);
			ddrawSurfaceBlt.Do((FARPROC)(libBaseAddr + surfBltOffset), (FARPROC)Blt);
			ddrawSurfaceFlip.Do((FARPROC)(libBaseAddr + surfFlipOffset), (FARPROC)Flip);
		}
	}

	return versionSupported;
}

void HookAll()
{
	ddrawSurfaceCreate.ReDo();
	ddrawSurfaceRestore.ReDo();
	ddrawSurfaceRelease.ReDo();
	ddrawSurfaceSetPalette.ReDo();
	ddrawPaletteSetEntries.ReDo();
}

void CheckDDrawCapture()
{
	EnterCriticalSection(&ddraw_mutex);
	ddrawSurfaceUnlock.ReDo(true);
	ddrawSurfaceFlip.ReDo(true);
	ddrawSurfaceBlt.ReDo(true);
	LeaveCriticalSection(&ddraw_mutex);
}