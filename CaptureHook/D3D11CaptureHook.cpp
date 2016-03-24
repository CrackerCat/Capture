#include "DXGI_D3D10_D3D11.h"
#include "CaptureHookCommon.h"


namespace {
	bool has_textured = false;
	unsigned __int64* pTextureHandle = NULL;

	HANDLE shared_handle = NULL;;

	bool open_msaa = false;
	ID3D11Texture2D* copy_resource = NULL;
	DXGI_FORMAT d3d11_format = DXGI_FORMAT_UNKNOWN;
}

bool D3D11ResourceCreate(ID3D11Device*);

h3d::CaptureInfo d3d11_captureinfo;

void h3d::D3D11Capture(IDXGISwapChain *pSwapChain) {
	ID3D11Device* device = NULL;
	if (FAILED(pSwapChain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&device))))
		return;

	EventProcess();

	if (capture_run && !has_textured) {
		if (!d3d11_format || !D3D11ResourceCreate(device)) 
			return;

		d3d11_captureinfo.Reserved2 = CtorSharedMemGPUCapture(&pTextureHandle);
		if (!d3d11_captureinfo.Reserved2) {
			D3D11Flush();
			return;
		}

		has_textured = true;
		d3d11_captureinfo.Reserved3 = -1;
		d3d11_captureinfo.Flip = 0;
		*pTextureHandle =reinterpret_cast<unsigned __int64>(shared_handle);
		logstream << "D3D11 SharedTexture Handle = 0X" << shared_handle << std::endl;

		memcpy(ptr_capture_info, &d3d11_captureinfo, sizeof(CaptureInfo));

		SetEvent(hReadyEvent);
		logstream << "Allthing has ready,HOOK Success[Sent Event to CaptureApp]" << std::endl;
	}

	KeepAliveProcess(D3D11Flush())

	if (has_textured && capture_run && pTextureHandle) {
		ID3D11DeviceContext* context;
		device->GetImmediateContext(&context);

		//todo fps maintain
		ID3D11Resource* back_buffer;
		if (SUCCEEDED(pSwapChain->GetBuffer(0, __uuidof(ID3D11Resource), reinterpret_cast<void**>(&back_buffer)))) {
			if (open_msaa)
				context->ResolveSubresource(copy_resource, 0, back_buffer, 0, d3d11_format);
			else
				context->CopyResource(copy_resource, back_buffer);
			back_buffer->Release();
		}
		else {
			logstream << "Maybe the Resource Is ID3D10Resourec..." << std::endl;
		}

		context->Release();
	}

	device->Release();
}

#define SR(var) if(var) {var->Release();var = NULL;}

#pragma warning(disable:4244)
bool D3D11ResourceCreate(ID3D11Device* pDevice) {
	CD3D11_TEXTURE2D_DESC copyTexDesc(d3d11_format, 
		d3d11_captureinfo.oWidth, d3d11_captureinfo.oHeight, 
		1, 1, 
		D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);
	copyTexDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;

	if (SUCCEEDED(pDevice->CreateTexture2D(&copyTexDesc,NULL,&copy_resource))) {
		IDXGIResource* dxgi_resource = NULL;
		if (SUCCEEDED(copy_resource->QueryInterface(&dxgi_resource))) {
			dxgi_resource->GetSharedHandle(&shared_handle);
			dxgi_resource->Release();

			ID3D11Resource* test_res;
			if (FAILED(pDevice->OpenSharedResource(shared_handle, __uuidof(ID3D11Resource), reinterpret_cast<void**>(&test_res))))
				logstream << "OpenSharedResource Test Failed";

			SR(test_res);

			return true;
		}
		copy_resource->Release();
	}
	logstream << "Capture Resource Create Failed" << std::endl;
	return false;
}

void h3d::D3D11Flush() {
	has_textured = false;
	pTextureHandle = NULL;
	shared_handle = NULL;

	SR(copy_resource);

	DestroySharedMem();

	logstream << "Flush D3D11 Capture" << std::endl;
}

void h3d::D3D11CaptureSetup(IDXGISwapChain* pSwapChain) {
	logstream << "D3D11CaptureSetup ..." << std::endl;
	D3D11Flush();
	CapturSetup(d3d11_captureinfo, d3d11_format, open_msaa, pSwapChain);
}