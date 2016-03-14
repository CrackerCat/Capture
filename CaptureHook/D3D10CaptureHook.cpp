#include "DXGI_D3D10_D3D11.h"
#include "CaptureHook.h"

//这些代码和D3D11的十分相像，但是并没有什么好办法,可以使用宏的方式
//不过会变得难以阅读 EventProcess,KeepAliveProcess就已经让人迷糊了

namespace {
	bool has_textured = false;
	unsigned __int64* pTextureHandle = NULL;

	HANDLE shared_handle = NULL;;

	bool open_msaa = false;
	ID3D10Texture2D* copy_resource = NULL;
	DXGI_FORMAT d3d10_format = DXGI_FORMAT_UNKNOWN;
}

bool D3D10ResourceCreate(ID3D10Device*);

h3d::CaptureInfo d3d10_captureinfo;

void h3d::D3D10Capture(IDXGISwapChain* pSwapChain) {
	ID3D10Device* device = NULL;
	if (FAILED(pSwapChain->GetDevice(__uuidof(ID3D10Device), reinterpret_cast<void**>(&device))))
		return;

	EventProcess();

	if (capture_run && !has_textured) {
		if (!d3d10_format || !D3D10ResourceCreate(device))
			return;

		d3d10_captureinfo.Reserved2 = CtorSharedMemGPUCapture(&pTextureHandle);
		if (!d3d10_captureinfo.Reserved2) {
			D3D10Flush();
			return;
		}

		has_textured = true;
		d3d10_captureinfo.Reserved3 = -1;
		*pTextureHandle = reinterpret_cast<unsigned __int64>(shared_handle);
		logstream << "D3D10 SharedTexture Handle = 0X" << shared_handle << std::endl;

		memcpy(ptr_capture_info, &d3d10_captureinfo, sizeof(CaptureInfo));

		SetEvent(hReadyEvent);
		logstream << "Allthing has ready,HOOK Success[Sent Event to CaptureApp]" << std::endl;
	}

	KeepAliveProcess(D3D10Flush())

	if (has_textured && capture_run && pTextureHandle) {
			//todo fps maintain
			ID3D10Resource* back_buffer;
			if (SUCCEEDED(pSwapChain->GetBuffer(0, __uuidof(ID3D10Resource), reinterpret_cast<void**>(&back_buffer)))) {
				if (open_msaa)
					device->ResolveSubresource(copy_resource, 0, back_buffer, 0, d3d10_format);
				else
					device->CopyResource(copy_resource, back_buffer);
				back_buffer->Release();
			}
	}

	device->Release();
}

#define SR(var) if(var) {var->Release();var = NULL;}

#pragma warning(disable:4244)
bool D3D10ResourceCreate(ID3D10Device* pDevice) {
	CD3D10_TEXTURE2D_DESC copyTexDesc(d3d10_format,
		d3d10_captureinfo.oWidth, d3d10_captureinfo.oHeight,
		1, 1,
		D3D10_BIND_RENDER_TARGET | D3D10_BIND_SHADER_RESOURCE);
	copyTexDesc.MiscFlags = D3D10_RESOURCE_MISC_SHARED;

	if (SUCCEEDED(pDevice->CreateTexture2D(&copyTexDesc, NULL, &copy_resource))) {
		IDXGIResource* dxgi_resource = NULL;
		if (SUCCEEDED(copy_resource->QueryInterface(&dxgi_resource))) {
			dxgi_resource->GetSharedHandle(&shared_handle);
			dxgi_resource->Release();

			ID3D10Resource* test_res;
			if (FAILED(pDevice->OpenSharedResource(shared_handle, __uuidof(ID3D10Resource), reinterpret_cast<void**>(&test_res))))
				logstream << "OpenSharedResource Test Failed";

			SR(test_res);

			return true;
		}
		copy_resource->Release();
	}
	logstream << "Capture Resource Create Failed" << std::endl;
	return false;
}

void h3d::D3D10Flush() {
	has_textured = false;
	pTextureHandle = NULL;
	shared_handle = NULL;

	SR(copy_resource);

	DestroySharedMem();

	logstream << "Flush D3D11 Capture" << std::endl;
}

void h3d::D3D10CaptureSetup(IDXGISwapChain* pSwapChain) {
	logstream << "D3D10CaptureSetup ..." << std::endl;
	D3D10Flush();
	CapturSetup(d3d10_captureinfo, d3d10_format, open_msaa, pSwapChain);
}