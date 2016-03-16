#include "CaptureHookCommon.h"

#include "D3D9CaptureHook.h"
bool has_textured = false;
h3d::CaptureInfo d3d9_captureinfo;
HMODULE hD3D9Dll = NULL;
DWORD d3d9_format = D3DFMT_UNKNOWN;


#include <dxgiformat.h>

#include <Windows.h>
#pragma warning(disable:4091)
#include <Shlobj.h>
#include <d3d9.h>

//Support D3D9EX

#pragma warning(disable:4244)

h3d::SWAPFORMAT ConvertFormat(D3DFORMAT);
DXGI_FORMAT ConverDXGIFormat(D3DFORMAT);

typedef LPDIRECT3D9(WINAPI* D3D9CREATEPROC)(UINT);
typedef HRESULT(WINAPI* D3D9CREATEXPROC)(UINT, IDirect3D9Ex**);

typedef HRESULT(WINAPI* ENDSCENE)(IDirect3DDevice9*);

#define D3D9ENDSCENE_OFFSET 42
h3d::APIHook end_scene;
h3d::APIHook end_sceneex;
HRESULT __stdcall EndScne(IDirect3DDevice9*);

bool d3d9ex_support = false;

//I don't think this code work for allthing
bool h3d::BeginD3D9CaptureHook() {

	wchar_t sD3D9Path[MAX_PATH];
	SHGetFolderPathW(NULL, CSIDL_SYSTEM, NULL, SHGFP_TYPE_CURRENT, sD3D9Path);
	wcscat_s(sD3D9Path, MAX_PATH, L"\\d3d9.dll");

	bool d3d9_support = false;
	hD3D9Dll = GetModuleHandle(sD3D9Path);
	if (hD3D9Dll) {
		D3DPRESENT_PARAMETERS d3dpp;
		ZeroMemory(&d3dpp, sizeof(d3dpp));
		d3dpp.Windowed = true;
		d3dpp.SwapEffect = D3DSWAPEFFECT_FLIP;
		d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
		d3dpp.BackBufferCount = 1;
		d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
		d3dpp.hDeviceWindow = hSender;

		D3D9CREATEPROC fD3D9Create = (D3D9CREATEPROC)GetProcAddress(hD3D9Dll, "Direct3DCreate9");
		if (fD3D9Create) {
			LPDIRECT3D9 pD3D = fD3D9Create(D3D_SDK_VERSION);
			if (pD3D) {
				IDirect3DDevice9* device;
				if (SUCCEEDED(pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_NULLREF, hSender, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &device))) {

					end_scene.Do(h3d::GetVirtual(device, D3D9ENDSCENE_OFFSET), (h3d::WINAPIPROC)EndScne);

					device->Release();
					d3d9_support = true;
				}

				pD3D->Release();
			}
		}

		D3D9CREATEXPROC fD3D9CreateEx = (D3D9CREATEXPROC)GetProcAddress(hD3D9Dll, "Direct3DCreate9Ex");
		if (fD3D9CreateEx) {
			IDirect3D9Ex* pD3DEx = NULL;
			if (SUCCEEDED(fD3D9CreateEx(D3D_SDK_VERSION, &pD3DEx))) {
				IDirect3DDevice9Ex* deviceEx = NULL;
				if (SUCCEEDED(pD3DEx->CreateDeviceEx(D3DADAPTER_DEFAULT, D3DDEVTYPE_NULLREF, hSender, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, NULL, &deviceEx))) {
					if (end_scene.GetTarget() != h3d::GetVirtual(deviceEx, D3D9ENDSCENE_OFFSET)) {
						logstream << "IDirect3DDevice9Ex::EndScene != IDirect3DDevice9::EndScene" << std::endl;
						end_sceneex.Do(h3d::GetVirtual(deviceEx, D3D9ENDSCENE_OFFSET), (h3d::WINAPIPROC)EndScne);
					}
					d3d9ex_support = true;
				}

				pD3DEx->Release();
			}
		}
	}

	return d3d9ex_support || d3d9_support;
}

bool gpu_support = false;

IDirect3DDevice9* current_device = NULL;
void D3D9CaptureSetup(IDirect3DDevice9* device);

HRESULT __stdcall EndScne(IDirect3DDevice9* device) {
	//EnterCriticalSection
	end_scene.UnDo();

	if (current_device == NULL && !target_acquired) {
		gpu_support =  GPUCaptureCheck(device);
		current_device = device;
		D3D9CaptureSetup(device);
		target_acquired = true;
	}


	HRESULT hr = device->EndScene();
	end_scene.ReDo();

	//LeaveCriticalSection
	return hr;
}


#define D3D9PRESENT_OFFSET 17
h3d::APIHook present;
HRESULT __stdcall Present(IDirect3DDevice9 *device, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion);

#define D3D9RESET_OFFSET 16
h3d::APIHook reset;
HRESULT __stdcall Reset(IDirect3DDevice9 *device, D3DPRESENT_PARAMETERS* params);

#define D3D9PRESENTEX_OFFSET 121
h3d::APIHook presentex;
HRESULT __stdcall PresentEx(IDirect3DDevice9Ex *device, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion, DWORD dwFlags);

#define D3D9RESETEX_OFFSET 132
h3d::APIHook resetex;
HRESULT __stdcall ResetEx(IDirect3DDevice9Ex *device, D3DPRESENT_PARAMETERS* params, D3DDISPLAYMODEEX* fullScreenData);


#define SWAPCHAINPRESENT_OFFSET 3
h3d::APIHook swap_present;
HRESULT __stdcall SwapPresent(IDirect3DSwapChain9 *swap, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion, DWORD dwFlags);





void D3D9CaptureSetup(IDirect3DDevice9* device) {
	IDirect3DSwapChain9* swap_chain = NULL;
	//通过交换链信息来获取相关信息
	if (SUCCEEDED(device->GetSwapChain(0, &swap_chain))) {
		D3DPRESENT_PARAMETERS d3dpp;
		if (SUCCEEDED(swap_chain->GetPresentParameters(&d3dpp))) {

			if (d3d9_format != d3dpp.BackBufferFormat ||
				d3d9_captureinfo.oWidth != d3dpp.BackBufferWidth || d3d9_captureinfo.oHeight != d3dpp.BackBufferHeight ||
				reinterpret_cast<HWND>(d3d9_captureinfo.sNative) != d3dpp.hDeviceWindow) {
				//todo printf prsents info
				d3d11_format = ConverDXGIFormat(d3dpp.BackBufferFormat);
				d3d9_format = d3dpp.BackBufferFormat;
				d3d9_captureinfo.oWidth = d3dpp.BackBufferWidth;
				d3d9_captureinfo.oHeight = d3dpp.BackBufferHeight;
				d3d9_captureinfo.sNative = reinterpret_cast<unsigned __int64>(d3dpp.hDeviceWindow);
				d3d9_captureinfo.Reserved1 = ConvertFormat(d3dpp.BackBufferFormat);
			}
		}
		else
			logstream << "Don't have enough infomation to hook(failed to get present params)" << std::endl;

		//todo D3D9EX/PresentEx
		IDirect3DDevice9Ex* deviceEx = NULL;
		if (d3d9ex_support && SUCCEEDED(device->QueryInterface(__uuidof(IDirect3DDevice9Ex), reinterpret_cast<void**>(&deviceEx)))) {
			presentex.Do(h3d::GetVirtual(device, D3D9PRESENTEX_OFFSET), (h3d::WINAPIPROC)PresentEx);
			resetex.Do(h3d::GetVirtual(device, D3D9RESETEX_OFFSET), (h3d::WINAPIPROC)ResetEx);
			deviceEx->Release();
		}


		present.Do(h3d::GetVirtual(device, D3D9PRESENT_OFFSET), (h3d::WINAPIPROC)Present);
		reset.Do(h3d::GetVirtual(device, D3D9RESET_OFFSET), (h3d::WINAPIPROC)Reset);
		swap_present.Do(h3d::GetVirtual(swap_chain, SWAPCHAINPRESENT_OFFSET), (h3d::WINAPIPROC)SwapPresent);

		swap_chain->Release();
		logstream << "D3D9Capture Succesfully Set Up D3D9 HOOKS" << std::endl;
	}
	else
		logstream << "D3D9Capture Faild to get SwapCahin to initialize hooks" << std::endl;
}

h3d::SWAPFORMAT ConvertFormat(D3DFORMAT format) {
	//https://msdn.microsoft.com/en-us/library/windows/desktop/bb172558(v=vs.85).aspx#BackBuffer_or_Display_Formats
	switch (format) {
	case D3DFMT_A2R10G10B10: return h3d::R10G10B10A2;
	case D3DFMT_A8R8G8B8: return h3d::BGRA8;
	case D3DFMT_X8R8G8B8: return h3d::BGRX8;//如果是使用FFMPEG，不需要这个格式
	case D3DFMT_A1R5G5B5: return h3d::B5G6R5A1;
	case D3DFMT_X1R5G5B5:return h3d::B5G6R5X1;
	case D3DFMT_R5G6B5: return h3d::B5G6R5;
	}
	return h3d::SWAPFORMAT_UNKNOWN;
}
DXGI_FORMAT ConverDXGIFormat(D3DFORMAT format) {
	switch (format)
	{
	case D3DFMT_A2B10G10R10:    return DXGI_FORMAT_R10G10B10A2_UNORM;
	case D3DFMT_A8R8G8B8:       return DXGI_FORMAT_B8G8R8A8_UNORM;
	case D3DFMT_X8R8G8B8:       return DXGI_FORMAT_B8G8R8X8_UNORM;
	case D3DFMT_A1R5G5B5:  case D3DFMT_X1R5G5B5:     return DXGI_FORMAT_B5G5R5A1_UNORM;
	case D3DFMT_R5G6B5:         return DXGI_FORMAT_B5G6R5_UNORM;
	}

	return DXGI_FORMAT_UNKNOWN;
}


void D3D9Capture(IDirect3DDevice9* device);

HRESULT __stdcall Present(IDirect3DDevice9 *device, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion) {
	present.UnDo();

	D3D9Capture(device);

	HRESULT hr = device->Present(pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);

	present.ReDo();

	return hr;
}

HRESULT __stdcall PresentEx(IDirect3DDevice9Ex *device, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion, DWORD dwFlags) {
	present.UnDo();

#ifdef _DEBUG
	//logstream << "D3D9Present called" << std::endl;
#endif

	//todo some code protect fps or other things 

	//todo get the surface image
	D3D9Capture(device);

	HRESULT hr = device->PresentEx(pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion, dwFlags);

	present.ReDo();

	return hr;
}

HRESULT __stdcall SwapPresent(IDirect3DSwapChain9 *swap, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion, DWORD dwFlags) {
	swap_present.UnDo();

	D3D9Capture(current_device);

	HRESULT hr = swap->Present(pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion, dwFlags);

	swap_present.ReDo();

	return hr;
}

void Flush();

HRESULT __stdcall Reset(IDirect3DDevice9 *device, D3DPRESENT_PARAMETERS* params) {
	reset.UnDo();

#ifdef _DEBUG
	//logstream << "D3D9Rest called" << std::endl;
#endif

	Flush();
	//todo clear data/build data

	HRESULT hr = device->Reset(params);

	if (current_device == NULL && !target_acquired) {
		current_device = device;
		target_acquired = true;
	}

	if (current_device == device)
		D3D9CaptureSetup(device);

	reset.ReDo();

	return hr;
}

HRESULT __stdcall ResetEx(IDirect3DDevice9Ex *device, D3DPRESENT_PARAMETERS* params, D3DDISPLAYMODEEX* fullScreenData) {
	reset.UnDo();
	resetex.UnDo();

#ifdef _DEBUG
	//logstream << "D3D9Rest called" << std::endl;
#endif

	Flush();
	//todo clear data/build data

	HRESULT hr = device->ResetEx(params, fullScreenData);

	if (current_device == NULL && !target_acquired) {
		current_device = device;
		target_acquired = true;
	}

	if (current_device == device)
		D3D9CaptureSetup(device);//Don't worry that hookapi will deal with second hook do(undo first)

	reset.ReDo();
	resetex.ReDo();

	return hr;
}






void CreateCPUCapture(IDirect3DDevice9* device);

#define NUM_BACKBUFF 3

namespace {
	//CPU copy stuff
	IDirect3DQuery9* d3d9_querys[NUM_BACKBUFF] = { 0,0,0 };
	bool issued_querys[NUM_BACKBUFF] = { 0,0,0 };
	IDirect3DSurface9 * d3d9_textures[NUM_BACKBUFF] = { 0,0,0 };
	bool locked_textures[NUM_BACKBUFF] = { 0,0,0 };
	IDirect3DSurface9* d3d9_copytextures[NUM_BACKBUFF] = { 0,0,0 };
	UINT tex_pitch = 0;

	CRITICAL_SECTION data_mutexs[NUM_BACKBUFF];

	void* copy_data = NULL;
	UINT copy_index = 0;
	HANDLE copy_event = NULL;
	HANDLE copy_thread = NULL;
	bool copy_thread_run = true;
	h3d::MemoryInfo* copy_info = NULL;
	h3d::byte* tex_addrsss[2] = { NULL,NULL };
	static DWORD copy_wait = 0;

	DWORD curr_capture = 0;
}

#include "D3D9CaptureHook.h"

LONGLONG prev_point = 0;
void D3D9Capture(IDirect3DDevice9* device) {
	h3d::EventProcess();

	if (!has_textured && capture_run) {

		//fix for some [backbuffers aren't actually being properly used]
		//get size/format at actual current rt at time of present
		IDirect3DSurface9* back_buffer = NULL;
		if (SUCCEEDED(device->GetRenderTarget(0, &back_buffer))) {
			D3DSURFACE_DESC sd;
			ZeroMemory(&sd, sizeof(sd));

			if (SUCCEEDED(back_buffer->GetDesc(&sd))) {
				//todo something
			}

			back_buffer->Release();
		}


		if (gpu_support)
			CreateGPUCapture(device);
		else
			CreateCPUCapture(device);
	}

	//KeeapAliveProcess(Flush()) Crash In Xp(WMware Workstation)
	//I Think Cry
	if (capture_run) {
			LONGLONG capture_tp = h3d::GetOSMillSeconds(); 
			if (capture_tp - prev_point > KEEP_TIME_DURATION) {
					HANDLE keepAlive = OpenEventW(EVENT_ALL_ACCESS, FALSE, sKeepAlive.c_str()); 
					if (keepAlive)
						CloseHandle(keepAlive); 
					else {
							Flush();
							logstream << "Don't Exist Event[" << sKeepAlive << "] (OBS Process Unexpected Exit,Wait Next Begin Event)" << std::endl;
							capture_run = false; 
					}
						prev_point = capture_tp; 
			}
	}
	//todo some keep alive state check

	if (has_textured && capture_run) {
		if (gpu_support)
			D3D9CaptureGPU(device);
		else {
			//copy texture data only when GetRenderTargetData completes
			for (UINT i = 0; i != NUM_BACKBUFF; ++i) {
				if (issued_querys[i]) {
					if (d3d9_querys[i]->GetData(0, 0, 0) == S_OK) {
						issued_querys[i] = false;

						IDirect3DSurface9* texture = d3d9_textures[i];
						D3DLOCKED_RECT locked_rect;

						if (SUCCEEDED(texture->LockRect(&locked_rect, NULL, D3DLOCK_READONLY))) {
							copy_data = locked_rect.pBits;
							copy_index = i;
							locked_textures[i] = true;

							SetEvent(copy_event);
						}
					}
				}
			}

			//todo fps maintain
			DWORD next_capture = (curr_capture == NUM_BACKBUFF - 1) ? 0 : (curr_capture + 1);
			IDirect3DSurface9* src_texture = d3d9_copytextures[curr_capture];
			IDirect3DSurface9* back_buffer = NULL;

			if (SUCCEEDED(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &back_buffer))) {
				device->StretchRect(back_buffer, NULL, src_texture, NULL, D3DTEXF_NONE);
				back_buffer->Release();

				if (copy_wait < NUM_BACKBUFF - 1)
					++copy_wait;//直到填满三个
				else {
					IDirect3DSurface9* prev_texture = d3d9_copytextures[next_capture];
					IDirect3DSurface9* target_texture = d3d9_textures[next_capture];

					if (locked_textures[next_capture]) {
						EnterCriticalSection(&data_mutexs[next_capture]);

						target_texture->UnlockRect();
						locked_textures[next_capture] = false;

						LeaveCriticalSection(&data_mutexs[next_capture]);
					}

					if (FAILED(device->GetRenderTargetData(prev_texture, target_texture)))
						logstream << "Failed captureing frame via cpu" << std::endl;

					d3d9_querys[next_capture]->Issue(D3DISSUE_END);
					issued_querys[next_capture] = true;

					//logstream << "Successfully captureing frame via cpu" << std::endl;
				}
			}

			curr_capture = next_capture;
		}
	}
	else if (has_textured)
		Flush();
}

DWORD CopyD3D9TextureThread(LPVOID lpUseless);

void CreateCPUCapture(IDirect3DDevice9* device) {
	bool cond = true;

	for (UINT i = 0; i != NUM_BACKBUFF; ++i) {
		if (FAILED(device->CreateOffscreenPlainSurface(d3d9_captureinfo.oWidth, d3d9_captureinfo.oHeight,
			(D3DFORMAT)d3d9_format, D3DPOOL_SYSTEMMEM, &d3d9_textures[i], NULL))) {
			logstream << "CPU Texture IDirect3DDevice9::CreateOffscreenPlainSurface " << i << "th failed" << std::endl;
			cond = false;
			break;
		}
	}

	//try lock get pitch
	if (cond) {
		D3DLOCKED_RECT lr;
		if (FAILED(d3d9_textures[NUM_BACKBUFF - 1]->LockRect(&lr, NULL, D3DLOCK_READONLY))) {
			logstream << "CPU Texture Lock Failed,can't get pitch" << std::endl;
			cond = false;
		}
		tex_pitch = lr.Pitch;
		d3d9_textures[NUM_BACKBUFF - 1]->UnlockRect();
	}

	//create rt for copy/mutex for lock data/query for getdata
	if (cond) {
		for (UINT i = 0; i != NUM_BACKBUFF; ++i) {
			if (FAILED(device->CreateRenderTarget(d3d9_captureinfo.oWidth, d3d9_captureinfo.oHeight, (D3DFORMAT)d3d9_format, D3DMULTISAMPLE_NONE, 0, FALSE, &d3d9_copytextures[i], NULL)))
			{
				logstream << "CPU Texture IDirect3DDevice9::CreateRenderTarget " << i << " failed" << std::endl;
				cond = false;
				break;
			}

			if (FAILED(device->CreateQuery(D3DQUERYTYPE_EVENT, &d3d9_querys[i])))
			{
				logstream << "CPU Texture IDirect3DDevice9::CreateQuery " << i << " failed" << std::endl;
				cond = false;
				break;
			}

			InitializeCriticalSection(&data_mutexs[i]);
		}
	}

	if (cond) {
		copy_thread_run = true;
		if (copy_thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CopyD3D9TextureThread, NULL, 0, NULL)) {
			if (!(copy_event = CreateEvent(NULL, FALSE, FALSE, NULL))) {
				logstream << "Create CopyEvent Failed" << std::endl;
			}
		}
		else
		{
			logstream << "Create CopyD3D9TextureThread Failed" << std::endl;
			cond = false;
		}
	}

	if (cond) {
		d3d9_captureinfo.Reserved2 = h3d::CtorSharedMemCPUCapture(tex_pitch*d3d9_captureinfo.oHeight, d3d9_captureinfo.Reserved3, copy_info, tex_addrsss);

		if (!d3d9_captureinfo.Reserved2) {
			logstream << "Create Shared Memory Failed" << std::endl;
		}
	}

	if (cond) {
		has_textured = true;
		d3d9_captureinfo.Reserved4 = tex_pitch;
		memcpy(ptr_capture_info, &d3d9_captureinfo, sizeof(h3d::CaptureInfo));
		SetEvent(hReadyEvent);
		logstream << "Allthing has ready,HOOK Success[Sent Event to CaptureApp]" << std::endl;
	}
	else {
		Flush();
	}
}

#define SR(var) if(var) {var->Release();var = NULL;}

DWORD CopyD3D9TextureThread(LPVOID lpUseless) {
	logstream << "Begin CopyD3D9TextureThread" << std::endl;

	HANDLE hEvent = NULL;
	if (!DuplicateHandle(GetCurrentProcess(), copy_event, GetCurrentProcess(), &hEvent, 0, FALSE, DUPLICATE_SAME_ACCESS)) {
		logstream << "CopyD3D9TextureThread DuplicateHandle Failed" << std::endl;
	}

	bool address_index = false;

	//得到事件传信时开始工作
	while ((WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0) && copy_thread_run) {
		bool next_address_index = !address_index;

		DWORD local_copy_index = copy_index;
		LPVOID local_copy_data = copy_data;

		if (local_copy_index < NUM_BACKBUFF && local_copy_data != NULL) {
			EnterCriticalSection(&data_mutexs[local_copy_index]);

			int last_rendered = -1;

			if (WaitForSingleObject(texture_mutex[address_index], 0) == WAIT_OBJECT_0)
				last_rendered = address_index;
			else if (WaitForSingleObject(texture_mutex[next_address_index], 0) == WAIT_OBJECT_0)
				last_rendered = next_address_index;


			if (last_rendered != -1) {
				memcpy(tex_addrsss[last_rendered], local_copy_data, tex_pitch*d3d9_captureinfo.oHeight);
				ReleaseMutex(texture_mutex[last_rendered]);
				copy_info->Reserved3 = last_rendered;
			}

			LeaveCriticalSection(&data_mutexs[local_copy_index]);
		}

		address_index = next_address_index;
	}

	CloseHandle(hEvent);
	logstream << "Exit CopyD3D9TextureThread" << std::endl;
	return 0;
}

void Flush() {
	has_textured = false;

	//CPU
	if (copy_thread) {
		copy_thread_run = false;
		SetEvent(copy_event);
		//强制终结线程
		if (WaitForSingleObject(copy_thread, 500) != WAIT_OBJECT_0)
			TerminateThread(copy_thread, 0);

		CloseHandle(copy_thread);
		CloseHandle(copy_data);

		copy_thread = NULL;
		copy_event = NULL;
	}

	for (int i = 0; i != NUM_BACKBUFF; ++i) {
		if (locked_textures[i]) {
			EnterCriticalSection(&data_mutexs[i]);

			d3d9_textures[i]->UnlockRect();
			locked_textures[i] = false;

			LeaveCriticalSection(&data_mutexs[i]);
		}

		issued_querys[i] = false;
		SR(d3d9_textures[i]);
		SR(d3d9_copytextures[i]);
		SR(d3d9_querys[i]);

		DeleteCriticalSection(&data_mutexs[i]);
	}

	copy_data = NULL;
	copy_info = NULL;
	copy_wait = 0;
	copy_index = 0;
	curr_capture = 0;

	FlushGPU();

	h3d::DestroySharedMem();


	logstream << "Flush D3D9 Capture" << std::endl;
}


void h3d::EndD3D9CaptureHook() {
	end_scene.UnDo();
	reset.UnDo();
	present.UnDo();
	resetex.UnDo();
	presentex.UnDo();
	swap_present.UnDo();
	Flush();
}


