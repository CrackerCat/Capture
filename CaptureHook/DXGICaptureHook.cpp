#include "CaptureHook.h"
#include "DXGI_D3D10_D3D11.h"
#include "Extern.h"

#include <Windows.h>
#include <Shlobj.h>

#include "MinHook\MinHook.h"

//Only A little Game Support D3D10，because D3D11 contains D3D10
IDXGISwapChain* CreateDummySwapChain();
IDXGISwapChain* CreateDummySwapChainDecay();

#define DXGIPRESENT_OFFSET 8
h3d::APIHook dxgi_present;
HRESULT __stdcall DXGIPresent(IDXGISwapChain*, UINT SyncInterval,UINT Flags);

#define DXGIRESIZE_OFFSET 13
h3d::APIHook dxgi_resize;
HRESULT __stdcall DXGIResizeBuffers(IDXGISwapChain*, UINT BufferCount,UINT Width,UINT Height,DXGI_FORMAT NewFormat,UINT SwapChainFlags);


bool h3d::BeginDXGICaptureHook() {

	IDXGISwapChain* swap_chain = CreateDummySwapChain();
	if (!swap_chain)
		swap_chain = CreateDummySwapChainDecay();

	if (swap_chain) {
		dxgi_present.Do(GetVirtual(swap_chain, DXGIPRESENT_OFFSET), (WINAPIPROC)DXGIPresent);
		dxgi_resize.Do(GetVirtual(swap_chain,DXGIRESIZE_OFFSET),(WINAPIPROC)DXGIResizeBuffers);

		swap_chain->Release();

		//dxgi_present.ReDo();
		//dxgi_resize.ReDo();

		return true;
	}

	return false;
}

void h3d::EndDXGICaptureHook() {
	dxgi_present.UnDo();
	dxgi_resize.UnDo();

	D3D11Flush();
	D3D10Flush();
}


typedef HRESULT(WINAPI*D3D11PROC)(IDXGIAdapter*, D3D_DRIVER_TYPE, HMODULE, UINT, D3D_FEATURE_LEVEL*, UINT, UINT, DXGI_SWAP_CHAIN_DESC*, IDXGISwapChain**, ID3D11Device** ppDevice, D3D_FEATURE_LEVEL*, ID3D11DeviceContext** ppImmediateContext);

IDXGISwapChain* CreateDummySwapChain() {
	DXGI_SWAP_CHAIN_DESC swap_desc;
	ZeroMemory(&swap_desc, sizeof(swap_desc));

	swap_desc.BufferCount = 2;
	swap_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swap_desc.BufferDesc.Width = 2;
	swap_desc.BufferDesc.Height = 2;
	swap_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swap_desc.OutputWindow = hSender;
	swap_desc.SampleDesc.Count = 1;
	swap_desc.Windowed = TRUE;

	IDXGISwapChain* swap_chain = NULL;
	D3D_DRIVER_TYPE	driver_type = D3D_DRIVER_TYPE_HARDWARE;

	wchar_t dllPath[MAX_PATH];
	SHGetFolderPathW(NULL, CSIDL_SYSTEM, NULL, SHGFP_TYPE_CURRENT, dllPath);
	wcscat_s(dllPath, MAX_PATH, L"\\d3d11.dll");

	HMODULE hD3D11Dll = GetModuleHandleW(dllPath);
	if (hD3D11Dll) {
		D3D11PROC create = (D3D11PROC)GetProcAddress(hD3D11Dll, "D3D11CreateDeviceAndSwapChain");
		if (create) {
			D3D_FEATURE_LEVEL desired_levels[] = { D3D_FEATURE_LEVEL_11_0,D3D_FEATURE_LEVEL_10_1,D3D_FEATURE_LEVEL_10_0,
				D3D_FEATURE_LEVEL_9_3, D3D_FEATURE_LEVEL_9_2 ,D3D_FEATURE_LEVEL_9_1 };
			D3D_FEATURE_LEVEL support_level;

			ID3D11DeviceContext* context;
			ID3D11Device* device;
			
			if (SUCCEEDED(
				(*create)(NULL,driver_type,NULL,0,desired_levels,6,D3D11_SDK_VERSION,&swap_desc,&swap_chain,&device,&support_level,&context)
				)) {
				context->Release();
				device->Release();
				return swap_chain;
			}

			logstream << "Create SwapChain Failed(D3D11)" << std::endl;
		}
	}

	return nullptr;
}

typedef HRESULT(WINAPI *D3D10PROC)(IDXGIAdapter*, D3D10_DRIVER_TYPE, HMODULE, UINT, UINT, DXGI_SWAP_CHAIN_DESC*, IDXGISwapChain**, ID3D10Device**);
IDXGISwapChain* CreateDummySwapChainDecay() {
	DXGI_SWAP_CHAIN_DESC swap_desc;
	ZeroMemory(&swap_desc, sizeof(swap_desc));

	swap_desc.BufferCount = 2;
	swap_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swap_desc.BufferDesc.Width = 2;
	swap_desc.BufferDesc.Height = 2;
	swap_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swap_desc.OutputWindow = hSender;
	swap_desc.SampleDesc.Count = 1;
	swap_desc.Windowed = TRUE;

	IDXGISwapChain* swap_chain = NULL;
	D3D10_DRIVER_TYPE driver_type = D3D10_DRIVER_TYPE_NULL;


	wchar_t dllPath[MAX_PATH];
	SHGetFolderPathW(NULL, CSIDL_SYSTEM, NULL, SHGFP_TYPE_CURRENT, dllPath);
	wcscat_s(dllPath, MAX_PATH, L"\\d3d10.dll");

	HMODULE hD3D10Dll = GetModuleHandleW(dllPath);
	if (hD3D10Dll) {
		D3D10PROC create = (D3D10PROC)GetProcAddress(hD3D10Dll, "D3D10CreateDeviceAndSwapChain");
		if (create) {
			ID3D10Device* device = NULL;
			if (SUCCEEDED((*create)(NULL,driver_type,NULL,0,D3D10_SDK_VERSION,&swap_desc,&swap_chain,&device))) {
				device->Release();
				return swap_chain;
			}
		}
	}

	return nullptr;
}

IDXGISwapChain* current_swapchain = NULL;

typedef void(*CAPTUREPROC)(IDXGISwapChain*);
void DispatchCapture(IDXGISwapChain *);

CAPTUREPROC capture_proc = NULL;

HRESULT __stdcall DXGIPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
	if (current_swapchain == NULL && !target_acquired) {
		DispatchCapture(pSwapChain);
	}

	if ((Flags & DXGI_PRESENT_TEST) == 0 && current_swapchain == pSwapChain && capture_proc) {
		(*capture_proc)(pSwapChain);
	}

	dxgi_present.UnDo();
	HRESULT hr = pSwapChain->Present(SyncInterval, Flags);
	dxgi_present.ReDo();

	return hr;
}

typedef void(*ENDPROC)();
ENDPROC end_proc = NULL;


void DispatchCapture(IDXGISwapChain * pSwapChain) {

	IUnknown* device_unknown;
	if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(IUnknown), reinterpret_cast<void**>(&device_unknown)))) {
		bool d3d11_special = false;
		/*Cod 使命召唤 能查出D3D10的device,但是实际是D3D11设备*/
		if (wcscmp(game_name, L"iw6sp64_ship.exe") == 0 ||
			wcscmp(game_name, L"iw6mp64_ship.exe") == 0 ||
			wcscmp(game_name, L"justcause3.exe") == 0)
			d3d11_special = true;


		ID3D11Device* device_d3d11 = NULL;
		ID3D10Device* device_d3d10 = NULL;
		/*一些D3D10程序 能查出D3D11的device,但实际是D3D10设备*/
		if (!d3d11_special && SUCCEEDED(device_unknown->QueryInterface(&device_d3d10))) {
			logstream << "DispatchCapture Goto D3D11" << std::endl;
			h3d::D3D10CaptureSetup(pSwapChain);
			capture_proc = h3d::D3D10Capture;
			end_proc = h3d::D3D10Flush;
			device_d3d10->Release();

			current_swapchain = pSwapChain;
			target_acquired = true;
		}
		else if (SUCCEEDED(device_unknown->QueryInterface(&device_d3d11))) {
			logstream << "DispatchCapture Goto D3D11" << std::endl;
			h3d::D3D11CaptureSetup(pSwapChain);
			capture_proc = h3d::D3D11Capture;
			end_proc = h3d::D3D11Flush;
			device_d3d11->Release();

			current_swapchain = pSwapChain;
			target_acquired = true;
		}
		
		
		device_unknown->Release();
	}
}

HRESULT __stdcall DXGIResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags) {
	if (end_proc)
		(*end_proc)();

	end_proc = NULL;
	capture_proc = NULL;
	current_swapchain = NULL;
	target_acquired = false;

	dxgi_resize.UnDo();
	HRESULT hr = pSwapChain->ResizeBuffers(BufferCount, Width, Height, NewFormat, SwapChainFlags);
	dxgi_resize.ReDo();

	return hr;
}