#ifndef D3D9_OEPNGL_GPU_INL
#define D3D9_OPENGL_GPU_INL

//该文件的用途只是分离相同代码。not a header!

#include <d3d11.h>
#include <dxgi.h>
#include "Extern.h"

namespace {
	//if support_gpu ,don't check dxgi_capture?
	//GPU COPY STUFF
	ID3D11Device* d3d11_device = NULL;
	ID3D11DeviceContext* d3d11_context = NULL;

	ID3D11Texture2D* d3d11_texture = NULL;

	unsigned __int64* pTexHandle = NULL;
}

#ifndef SR
#define SR(var) if(var) {var->Release();var = NULL;}
#endif

namespace h3d {

	typedef HRESULT(WINAPI*DXGICREATEPROC)(REFIID riid, void ** ppFactory);

	static HANDLE CreateSharedTexture(ID3D11Texture2D *& texture, ID3D11Device *& device, ID3D11DeviceContext *& context, DXGI_FORMAT format, UINT width, UINT height)
	{
		HMODULE hDXGIDll = LoadLibraryW(L"dxgi.dll");
		if (!hDXGIDll)
			return NULL;

		DXGICREATEPROC CreateDXGI = (DXGICREATEPROC)GetProcAddress(hDXGIDll, "CreateDXGIFactory1");

		IDXGIFactory1* pFactory = NULL;
		if (FAILED(CreateDXGI(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&pFactory))))
			return NULL;

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
		if (FAILED(d3d11_create(pAdapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, Flags, desired_levels, 6, D3D11_SDK_VERSION, &device, &support_level, &context)))
			goto d3d11_clear;

		logstream << "CreateSharedTexture By D3D11 Success" << std::endl;
	d3d11_clear:
		SR(pAdapter);
		SR(pFactory);

		if (!device)
			return NULL;

		CD3D11_TEXTURE2D_DESC copyTexDesc(format,
			width, height,
			1, 1,
			D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);
		copyTexDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;

		if (FAILED(device->CreateTexture2D(&copyTexDesc, NULL, &texture)))
			return NULL;

		HANDLE sharedHandle = NULL;
		IDXGIResource* dxgi_res = NULL;
		if (FAILED(texture->QueryInterface(&dxgi_res)))
			return NULL;

		if (FAILED(dxgi_res->GetSharedHandle(&sharedHandle)))
			return NULL;

		dxgi_res->Release();
		return sharedHandle;
	}
}

#endif
