
#include "CaptureHookCommon.h"

#include "D3D9CaptureHook.h"
DXGI_FORMAT d3d11_format = DXGI_FORMAT_UNKNOWN;

#include <d3d11.h>
#include "d3d9_magic.h"


#define SR(var) if(var) {var->Release();var = NULL;}

bool GPUCaptureCheck(IDirect3DDevice9 * device)
{
#ifdef _USING_V110_SDK71_
	return false;
#endif

	if (d3d11_format == DXGI_FORMAT_UNKNOWN)
		return false;

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

#include "D3D9_OPENGL_GPU.inl"

namespace {
	IDirect3DSurface9* d3d9_surface = NULL;
}

void CreateD3D9GPUCapture(IDirect3DDevice9* device) {
	
	HANDLE sharedHandle = h3d::CreateSharedTexture(d3d11_texture, d3d11_device, d3d11_context, d3d11_format, d3d9_captureinfo.oWidth, d3d9_captureinfo.oHeight);
	if (!sharedHandle)
		goto clear_all;
	IDirect3DTexture9* d3d9_texture = NULL;
	//todo only d3d9 do this check D3D9EX
	{
		//!!!用D3D9设备从D3D11的共享句柄创建贴图!!!
		//D3DERR_INVALIDCALL
		h3d::BeginD3D9Patch(hD3D9Dll);
		if (FAILED(device->CreateTexture(d3d9_captureinfo.oWidth, d3d9_captureinfo.oHeight, 1, D3DUSAGE_RENDERTARGET, (D3DFORMAT)d3d9_format, D3DPOOL_DEFAULT, &d3d9_texture, &sharedHandle))) {
			logstream << "Warning: IDirect3DDevice9->CreateTexture Failed" << std::endl;
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

	logstream << "Allthing has ready,D3D9 GPU HOOK Success[Sent Event to CaptureApp]" << std::endl;
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
