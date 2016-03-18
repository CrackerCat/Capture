#include "GraphicCapture.h"
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
	else {
		HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, processId);
		BOOL bX86 = TRUE;
		IsWow64Process(hProcess, &bX86);

		std::wstring ApplicationName = L"CaptureInject";
		if (bX86)
			ApplicationName.append(L".x86");
		else
			ApplicationName.append(L".x64");

#ifdef _DEBUG
		ApplicationName.append(L".debug");
#endif
		ApplicationName.append(L".exe");

		std::wstring CommandLine = std::to_wstring(processId);

		PROCESS_INFORMATION pi;
		ZeroMemory(&pi, sizeof(pi));
		STARTUPINFO si = { sizeof(STARTUPINFO)};


		CreateProcessW(ApplicationName.c_str(), &CommandLine[0], NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
		CloseHandle(pi.hThread);
		WaitForSingleObject(pi.hProcess, INFINITE);
		DWORD inject_result = 0;
		GetExitCodeProcess(pi.hProcess,&inject_result);

		CloseHandle(pi.hProcess);
		CloseHandle(hProcess);
		if (inject_result)
			return nullptr;
	}

	std::wstringstream wss;
	wss << EVNET_CAPTURE_READY << processId;

	HANDLE hReadyEvent = NULL;
	while(!(hReadyEvent = OpenEventW(EVENT_ALL_ACCESS,FALSE,wss.str().c_str())))
			;
	if (!hReadyEvent)
		return nullptr;

	logstream << "WaitForSingleObject hReadyEvent " << std::endl;
	if (WaitForSingleObject(hReadyEvent, 1000) == WAIT_TIMEOUT) {
		CloseHandle(hReadyEvent);
		return nullptr;
	}
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
