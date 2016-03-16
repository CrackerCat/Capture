
#include "CaptureHookCommon.h"

#include "D3D9CaptureHook.h"
DXGI_FORMAT d3d11_format = DXGI_FORMAT_UNKNOWN;

#include <d3d11.h>
#include "d3d9_magic.h"

namespace {
	//GPU COPY STUFF
	ID3D11Device* d3d11_device = NULL;
	ID3D11DeviceContext* d3d11_context = NULL;

	ID3D11Texture2D* d3d11_texture = NULL;

	IDirect3DSurface9* d3d9_surface = NULL;

	unsigned __int64* pTexHandle = NULL;

}


typedef HRESULT(WINAPI*DXGICREATEPROC)(REFIID riid, void ** ppFactory);

#define SR(var) if(var) {var->Release();var = NULL;}

bool GPUCaptureCheck(IDirect3DDevice9 * device)
{
#ifdef _USING_V110_SDK71_
	return false;
#endif

	bool result = false;

	IDirect3D9* d3d = NULL;
	if (SUCCEEDED(device->GetDirect3D(&d3d))) {
		IDirect3D9Ex* d3d9ex = NULL;
		if (result = SUCCEEDED(d3d->QueryInterface(__uuidof(IDirect3D9Ex), reinterpret_cast<void**>(&d3d9ex))))
			d3d9ex->Release();
		d3d->Release();
	}

	//只有不是D3D9EX的时候才去做D3D9Patch！[win7以上系统]
	if (!result) {
		result = h3d::D3D9PatchTypeSupportSharedTexture(hD3D9Dll);
	}
	return result;
}

void CreateGPUCapture(IDirect3DDevice9* device) {
	HMODULE hDXGIDll = LoadLibraryW(L"dxgi.dll");
	if (!hDXGIDll)
		return;

	DXGICREATEPROC CreateDXGI = (DXGICREATEPROC)GetProcAddress(hDXGIDll, "CreateDXGIFactory1");

	IDXGIFactory1* pFactory = NULL;
	if (FAILED(CreateDXGI(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&pFactory))))
		return;

	//总是使用第一个显卡
	IDXGIAdapter1* pAdapter = NULL;
	if (FAILED(pFactory->EnumAdapters1(0, &pAdapter)))
		goto d3d11_clear;

	HMODULE hD3D11Dll = LoadLibraryW(L"d3d11.dll");
	if (!hD3D11Dll)
		goto d3d11_clear;

	D3D_FEATURE_LEVEL desired_levels[] = { D3D_FEATURE_LEVEL_11_0,D3D_FEATURE_LEVEL_10_1,D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3, D3D_FEATURE_LEVEL_9_2 ,D3D_FEATURE_LEVEL_9_1 };
	D3D_FEATURE_LEVEL support_level;
	D3D_DRIVER_TYPE driver_type = D3D_DRIVER_TYPE_UNKNOWN;

	UINT Flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
	Flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	PFN_D3D11_CREATE_DEVICE d3d11_create = (PFN_D3D11_CREATE_DEVICE)GetProcAddress(hD3D11Dll, "D3D11CreateDevice");
	if (FAILED(d3d11_create(pAdapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, Flags, desired_levels, 6, D3D11_SDK_VERSION, &d3d11_device, &support_level, &d3d11_context)))
		goto d3d11_clear;

	logstream << "D3D9Capture By D3D11 Create Device Succ" << std::endl;
d3d11_clear:
	SR(pAdapter);
	SR(pFactory);

	if (!d3d11_device)
		return;

	CD3D11_TEXTURE2D_DESC copyTexDesc(d3d11_format,
		d3d9_captureinfo.oWidth, d3d9_captureinfo.oHeight,
		1, 1,
		D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);
	copyTexDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;

	if (FAILED(d3d11_device->CreateTexture2D(&copyTexDesc, NULL, &d3d11_texture)))
		return;

	HANDLE sharedHandle = NULL;
	IDXGIResource* dxgi_res = NULL;
	if (FAILED(d3d11_texture->QueryInterface(&dxgi_res)))
		return;

	if (FAILED(dxgi_res->GetSharedHandle(&sharedHandle)))
		return;

	dxgi_res->Release();

	IDirect3DTexture9* d3d9_texture = NULL;
	{
		//!!!用D3D9设备从D3D11的共享句柄创建贴图!!!
		h3d::BeginD3D9Patch(hD3D9Dll);

		if (FAILED(device->CreateTexture(copyTexDesc.Width, copyTexDesc.Height, 1, D3DUSAGE_RENDERTARGET, (D3DFORMAT)d3d9_format, D3DPOOL_DEFAULT, &d3d9_texture, &sharedHandle))) {
			logstream << "Warning: CreateTexture->OpenShared Failed" << std::endl;
			return;
		}

		h3d::EndD3D9Patch(hD3D9Dll);
	}
	if (!d3d9_texture)
		return;

	if (FAILED(d3d9_texture->GetSurfaceLevel(0, &d3d9_surface)))
		goto d3d9_clear;

d3d9_clear:
	SR(d3d9_texture);

	if (!d3d9_surface)
		return;

	d3d9_captureinfo.Reserved2 = h3d::CtorSharedMemGPUCapture(&pTexHandle);
	if (!d3d9_captureinfo.Reserved2)
		goto clear_all;

	d3d9_captureinfo.Reserved3 = -1;
	has_textured = true;
	*pTexHandle = reinterpret_cast<unsigned __int64>(sharedHandle);
	memcpy(ptr_capture_info, &d3d9_captureinfo, sizeof(h3d::CaptureInfo));
	SetEvent(hReadyEvent);

	logstream << "Allthing has ready, HOOK Success[Sent Event to CaptureApp]" << std::endl;
	return;
clear_all:
	FlushGPU();
}

void D3D9CaptureGPU(IDirect3DDevice9 * device)
{
	//todo check HOTD_NG.exe; GetRenderTarget??
	IDirect3DSurface9* back_buffer = NULL;
	if (FAILED(device->GetRenderTarget(0, &back_buffer)))
		device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &back_buffer);
	if (back_buffer) {
		if (FAILED(device->StretchRect(back_buffer, NULL, d3d9_surface, NULL, D3DTEXF_NONE)))
			logstream << "StretchRect Failed" << std::endl;
		back_buffer->Release();
	}
}

void FlushGPU()
{
	//GPU
	SR(d3d11_device);
	SR(d3d11_context);
	SR(d3d11_texture);
	SR(d3d9_surface);
}
