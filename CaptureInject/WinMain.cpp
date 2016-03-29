#include <fstream>
#include "CaptureInject.h"
#include <sdkddkver.h>
#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif

#define _WIN32_WINNT _WIN32_WINNT_WINXP //!important
#include <Windows.h>
std::wofstream logstream;

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,PWSTR pCmdLine, int nShowCmd)
{
	logstream.open("InjectHelper.log", std::ios_base::trunc);
	int argc = 0;
	LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	if (argc < 1)
	{
		LocalFree(argv);
		return -1;
	}
	logstream << L"Parse Params" << std::endl;
	for (unsigned i = 0; i != argc; ++i)
		logstream << '\t' << i << L". "<<argv[i]<<std::endl;
	logstream << std::endl;
	DWORD processId = wcstoul(argv[0], NULL,10);

	LocalFree(argv);

	h3d::AdjustToken();
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
	if (!hProcess) {
		logstream << L"OpenProcess Failed PID = " << processId <<L" Error Code = " <<GetLastError()<<std::endl;
		return ERROR_INVALID_HANDLE;
	}

	wchar_t directory[MAX_PATH] = {};
	GetCurrentDirectoryW(MAX_PATH, directory);
#ifndef	_WIN64
#ifndef _DEBUG
	bool inject_result = h3d::InjectDLL(OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId), std::wstring(directory) + L"/CaptureHook.x86.dll");
#else
	bool inject_result = h3d::InjectDLL(OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId), std::wstring(directory) + L"/CaptureHook.x86.debug.dll");
#endif
#else
#ifdef _DEBUG
	bool inject_result = h3d::InjectDLL(OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId), std::wstring(directory) + L"/CaptureHook.x64.debug.dll");
#else
	bool inject_result = h3d::InjectDLL(OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId), std::wstring(directory) + L"/CaptureHook.x64.dll");
#endif
#endif
	if (logstream.is_open())
		logstream.close();
	return !inject_result;
}
