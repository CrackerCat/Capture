#include "GraphicCapture.h"
#include "CaptureInject.h"
#include "MemoryCapture.h"
#include "TextureCapture.h"
#include <sstream>

static HANDLE GetEvent(LPCTSTR lpEvent)
{
	HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, lpEvent);
	if (!hEvent)
		hEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, lpEvent);

	return hEvent;
}


static bool GetCaptureInfo(h3d::CaptureInfo& info, DWORD processID) {
	std::wstringstream wss;
	wss << INFO_MEMORY << processID;
	HANDLE hInfoMap = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, wss.str().c_str());

	if (!hInfoMap)
		return false;

	h3d::CaptureInfo* pInfo = (h3d::CaptureInfo*)MapViewOfFile(hInfoMap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(h3d::CaptureInfo));
	if (!pInfo) {
		CloseHandle(pInfo);
		CloseHandle(hInfoMap);
		return false;
	}

	info = *pInfo;

	UnmapViewOfFile(pInfo);
	CloseHandle(hInfoMap);

	return true;
}

#include <fstream>
extern std::ofstream logstream;


h3d::CaptureHUB* h3d::GraphicCapture(unsigned long processId)
{
	//先探测对象是否注册，注册了直接发送开启
	std::wstringstream restart;
	restart << EVENT_OBS_BEGIN << processId;
	HANDLE hBeginEvent = OpenEventW(EVENT_ALL_ACCESS, FALSE, restart.str().c_str());
	if (hBeginEvent)
	{
		SetEvent(hBeginEvent);
		CloseHandle(hBeginEvent);
	}

	//32位程序无法向64位进程注入64位的dll，需要编写一个小型exe来完成注入
	h3d::AdjustToken();

	wchar_t directory[MAX_PATH] = {};
	GetCurrentDirectoryW(MAX_PATH, directory);
#ifndef	_WIN64
#ifndef _DEBUG
	bool inject_result = h3d::InjectDLL(OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId),std::wstring(directory)+L"/CaptureHook.x86.dll");
#else
	bool inject_result = h3d::InjectDLL(OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId), std::wstring(directory) + L"/CaptureHook.x86.debug.dll");
#endif
#else
#ifdef _DEBUG
	bool inject_result = h3d::InjectDLL(OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId), L"CaptureHook.x64.debug.dll");
#else
	bool inject_result = h3d::InjectDLL(OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId), L"CaptureHook.x64.dll");
#endif
#endif
	if (!inject_result)
		return nullptr;

	std::wstringstream wss;
	wss << EVNET_CAPTURE_READY << processId;

	HANDLE hReadyEvent = NULL;
	while(!(hReadyEvent = OpenEventW(EVENT_ALL_ACCESS,FALSE,wss.str().c_str())))
			;
	if (!hReadyEvent)
		return nullptr;

	logstream << "WaitForSingleObject hReadyEvent " << std::endl;
	WaitForSingleObject(hReadyEvent, INFINITE);
	CloseHandle(hReadyEvent);

	h3d::CaptureInfo info;
	GetCaptureInfo(info, processId);
	info.sPID = processId;

	if (info.Reserved3 == -1)
	{
		info.Reserved3 = sizeof(unsigned __int64);
		logstream << "CaptureType : TextureCapture" << std::endl;
		return new h3d::TextureCapture(info);
	}
	logstream << "CaptureType : MemoryCapture" << std::endl;
	return new h3d::MemoryCapture(info);
}

void h3d::EndCapture(CaptureHUB * capture)
{
	capture->Stop();
	delete capture;
}
