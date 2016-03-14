#include "Extern.h"
#include "CaptureHook.h"
#include <fstream>
#include <sstream>

DWORD WINAPI CaptureThread(LPVOID);

HINSTANCE hDllInstance;



HWND hSender = NULL;
std::wfstream logstream;

std::wstring sKeepAlive;

bool target_acquired = false;
bool capture_run = true;

void* ptr_capture_info = NULL;
HANDLE texture_mutex[2] = { NULL,NULL};

HANDLE hReadyEvent = NULL;
HANDLE hStopEvent = NULL;
HANDLE hBeginEvent = NULL;

bool bCaptureThreadStop = false;
HANDLE hCaptureThread = NULL;

HANDLE hInfoMap = NULL;

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved) {
	if (dwReason == DLL_PROCESS_ATTACH) {
		logstream.open("CaptureHookDebug.log", std::ios_base::in | std::ios_base::out | std::ios_base::trunc, 0X40);
		hDllInstance = hInstance;
		h3d::InitCaptureHook();

		HANDLE hDllMainThread = OpenThread(THREAD_ALL_ACCESS, NULL, GetCurrentThreadId());
		HANDLE hThread = CreateThread(NULL, 0, CaptureThread, hDllMainThread, 0, 0);
		if (!hThread) {
			logstream << "Create CaptureThread Failed LastError:" << GetLastError() << std::endl;
			CloseHandle(hDllMainThread);
			return FALSE;
		}
		hCaptureThread = hThread;
		logstream << "Create CaptureThread Success" << std::endl;
	}

	if (dwReason == DLL_PROCESS_DETACH) {
		bCaptureThreadStop = true;

		if(hCaptureThread)
			WaitForSingleObject(hCaptureThread, 500);

		if (hSender)
			DestroyWindow(hSender);

		if (ptr_capture_info) {
			UnmapViewOfFile(ptr_capture_info);
			CloseHandle(hInfoMap);
		}

		if (hReadyEvent)
			CloseHandle(hReadyEvent);
		if (hStopEvent)
			CloseHandle(hStopEvent);
		if (hBeginEvent)
			CloseHandle(hBeginEvent);

		for (UINT i = 0; i != 2; ++i)
			if (texture_mutex[i])
				CloseHandle(texture_mutex[i]);

		//Waning: 模块的卸载没有顺序，如果一个D3D模块已经被卸载然后再释放其资源，将会发生崩溃[Release模式]
		//h3d::EndDXGICaptureHook();
		//h3d::EndD3D9CaptureHook();
		h3d::ExitCaptureHook();
		logstream << "Exit CaptureHook" << std::endl;
		if (logstream.is_open())
			logstream.close();
	}

	return TRUE;
}

HANDLE GetEvent(LPCTSTR lpEvent)
{
	HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, lpEvent);
	if (!hEvent)
		hEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, lpEvent);

	return hEvent;
}

#include <Psapi.h>

bool CpatureHook();
DWORD WINAPI DummyWindowThread(LPVOID);
DWORD WINAPI CaptureThread(LPVOID lpMainThread) {
	HANDLE hMainThread = lpMainThread;

	if (hMainThread) {
		WaitForSingleObject(hMainThread, 150);
		CloseHandle(hMainThread);
	}

	
	//log path should be logs/CapthreHook******.log

	wchar_t sProcessName[MAX_PATH] = {};
	GetModuleBaseNameW(GetCurrentProcess(), NULL, sProcessName, MAX_PATH);
	logstream << "CaptureThread Attach to Process : " << sProcessName << std::endl;
	//LogInfo("CaptureThread Attach to Process : ",sProcessName);

	DWORD dThreadId;
	HANDLE hWindowThread = CreateThread(NULL, 0, DummyWindowThread, NULL, 0, &dThreadId);
	if (!hWindowThread) {
		logstream << "CaptureThread Create DummyWindowThread Failed " << std::endl;
		//LogErr("CaptureThread Create DummyWindowThread Failed");
		return 0;
	}
	CloseHandle(hWindowThread);

	DWORD currproces_id = GetCurrentProcessId();
	std::wstringstream wss;
	wss << INFO_MEMORY << currproces_id;
	hInfoMap = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(h3d::CaptureInfo), wss.str().c_str());

	if (!hInfoMap) {
		logstream << "CaptureThread Can't CreateFileMappingW lpName = " <<wss.str()<<" ErrorCode = " <<GetLastError() << std::endl;
		return 0;
	}


	ptr_capture_info = MapViewOfFile(hInfoMap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(h3d::CaptureInfo));
	if (!ptr_capture_info) {
		logstream << "CaptureThread Can't MapViewOfFile(it's impossible)" << std::endl;
		CloseHandle(hInfoMap);
		hInfoMap = NULL;
		return 0;
	}

	wss.swap(std::wstringstream());
	wss << EVNET_CAPTURE_READY << currproces_id;
	hReadyEvent = GetEvent(wss.str().c_str());
	logstream << "Create Ready Event :" << wss.str()<<std::endl;

	wss.swap(std::wstringstream());
	wss << EVENT_OBS_STOP << currproces_id;
	hStopEvent = GetEvent(wss.str().c_str());
	logstream << "Create Stop Event :" << wss.str() << std::endl;

	wss.swap(std::wstringstream());
	wss << EVENT_OBS_BEGIN << currproces_id;
	hBeginEvent = GetEvent(wss.str().c_str());

	wss.swap(std::wstringstream());
	wss << OBS_KEEP_ALIVE << currproces_id;
	sKeepAlive = wss.str();

	//可以考虑从共享内存读出想要的信息

	//插入一些锁逻辑

	texture_mutex[0] = OpenMutexW(MUTEX_ALL_ACCESS, FALSE, TEXTURE_FIRST_MUTEX);
	if(texture_mutex[0])//循环捕获
	{
		texture_mutex[1] = OpenMutex(MUTEX_ALL_ACCESS, FALSE, TEXTURE_SECOND_MUTEX);
		if (texture_mutex[1]) {
			while (!CpatureHook())
				Sleep(50);

			for (size_t n = 0; !bCaptureThreadStop; ++n) {
				if (n % 100 == 0) CpatureHook();
				Sleep(50);
			}

			CloseHandle(texture_mutex[1]);
			texture_mutex[1] = NULL;
		}
		else 
			logstream << "CaptureThread Can't Open Second TextureMutex" << std::endl;
		
	}
	else
		logstream << "CaptureThread Can't Open First TextureMutex" << std::endl;


	logstream << "CapturethThread Exit Main Thread Loop" << std::endl;
	return 0;
}

bool bD3D8Capture = false;
bool bD3D9Capture = false;

bool bDXGICapture = false;
bool CpatureHook() {
	if (!hSender)
		return false;

	if (!bD3D9Capture) {
		if (h3d::BeginD3D9CaptureHook()) {
			bD3D9Capture = true;
			return true;
		}
	}

	if (!bDXGICapture) {
		if (h3d::BeginDXGICaptureHook()) {
			bDXGICapture = true;
			return true;
		}
	}

	if (!bD3D8Capture) {
		if (h3d::BeginD3D8CaptureHook()) {
			bD3D8Capture = true;
			return true;
		}
	}

	return false;
}

#ifdef _DEBUG
#define DUMMYWINDOW_X CW_USEDEFAULT
#define DUMMYWINDOW_Y CW_USEDEFAULT
#define DUMMYWINDOW_W 800
#define DUMMYWINDOW_H 600
#else
#define DUMMYWINDOW_X 0
#define DUMMYWINDOW_Y 0
#define DUMMYWINDOW_W 1
#define DUMMYWINDOW_H 1
#endif

HWND CreateDummyWindow(LPCWSTR sClass, LPCWSTR sName) {
	return CreateWindowExW(0, sClass, sName, WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
		DUMMYWINDOW_X, DUMMYWINDOW_Y,
		DUMMYWINDOW_W, DUMMYWINDOW_H,
		NULL,
		NULL,
		hDllInstance, NULL);
}

#define DUMMYWINDOW_CLASS L"DummyWindowClass"
#define DUMMYWINDOW_NAME L"TempDeviceDummyWindow"

#define SENDERDUMMYWINDOW_CLASS L"SenderDummyWindowClass"

DWORD WINAPI DummyWindowThread(LPVOID) {
	WNDCLASSW wc;
	ZeroMemory(&wc, sizeof(wc));
	wc.style = CS_OWNDC;
	wc.hInstance = hDllInstance;
	wc.lpfnWndProc = DefWindowProcW;
	wc.lpszClassName = SENDERDUMMYWINDOW_CLASS;

	if (RegisterClassW(&wc)) {
		hSender = CreateDummyWindow(SENDERDUMMYWINDOW_CLASS, DUMMYWINDOW_NAME);

		if (!hSender) {
			//LogErr("Could not create sender window");
			return 0;
		}
	}
	else {
		//LogErr("Register Failed");
		return 0;
	}

	MSG msg;
	while (GetMessageW(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
	return 0;
}

