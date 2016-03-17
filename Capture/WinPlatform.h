#ifndef OS_WIN_H
#define OS_WIN_H

#include <Windows.h>
#include <mmsystem.h>
#pragma comment(lib,"winmm.lib")
namespace h3d {

#pragma warning(disable:4996)
	inline UINT GetOSVersion() {
		OSVERSIONINFO os_version;
		os_version.dwOSVersionInfoSize = sizeof(os_version);
		GetVersionExW(&os_version);

		//SB win10 内核版本 跳10
		if (os_version.dwMajorVersion > 6)
			return 10;

		//Win 8/8.1 => 8
		//7 =>7
		//Vista => 6
		if (os_version.dwMajorVersion == 6) {
			if (os_version.dwMinorVersion >= 2)
				return 8;

			return os_version.dwMajorVersion + os_version.dwMinorVersion;
		}

		//<= XP
		return 5;
	}

	inline LONGLONG GetOSMillSeconds() {
		return timeGetTime();
	}
}

#pragma warning(push)
#pragma warning(disable:4091)
#include <ImageHlp.h>
#pragma warning(pop)
#pragma comment(lib,"dbghelp.lib")

#include <string>

namespace h3d {
	inline BOOL IsDataSectionNeeded(const WCHAR* pModuleName)
	{
		if (pModuleName == 0)
		{
			return FALSE;
		}

		WCHAR szFileName[_MAX_FNAME] = L"";
		_wsplitpath(pModuleName, NULL, NULL, szFileName, NULL);
		if (wcsicmp(szFileName, L"ntdll") == 0)
			return TRUE;
		return FALSE;
	}

	inline BOOL CALLBACK MiniDumpCallback(PVOID                            pParam,
		const PMINIDUMP_CALLBACK_INPUT   pInput,
		PMINIDUMP_CALLBACK_OUTPUT        pOutput)
	{
		if (pInput == 0 || pOutput == 0)
			return FALSE;

		switch (pInput->CallbackType)
		{
		case ModuleCallback:
			if (pOutput->ModuleWriteFlags & ModuleWriteDataSeg)
				if (!IsDataSectionNeeded(pInput->Module.FullPath))
					pOutput->ModuleWriteFlags &= (~ModuleWriteDataSeg);
		case IncludeModuleCallback:
		case IncludeThreadCallback:
		case ThreadCallback:
		case ThreadExCallback:
			return TRUE;
		default:;
		}
		return FALSE;
	}

	inline void CreateMiniDump(EXCEPTION_POINTERS* pep, LPCTSTR strFileName)
	{
		HANDLE hFile = CreateFile(strFileName, GENERIC_READ | GENERIC_WRITE,
			0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

		if ((hFile != NULL) && (hFile != INVALID_HANDLE_VALUE))
		{
			MINIDUMP_EXCEPTION_INFORMATION mdei;
			mdei.ThreadId = GetCurrentThreadId();
			mdei.ExceptionPointers = pep;
			mdei.ClientPointers = FALSE;
			MINIDUMP_CALLBACK_INFORMATION mci;
			mci.CallbackRoutine = (MINIDUMP_CALLBACK_ROUTINE)MiniDumpCallback;
			mci.CallbackParam = 0;
			MINIDUMP_TYPE mdt = (MINIDUMP_TYPE)(MiniDumpWithPrivateReadWriteMemory |
				MiniDumpWithDataSegs |
				MiniDumpWithHandleData |
				0x00000800 /*MiniDumpWithFullMemoryInfo*/ |
				0x00001000 /*MiniDumpWithThreadInfo*/ |
				MiniDumpWithUnloadedModules);
			MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
				hFile, mdt, (pep != 0) ? &mdei : 0, 0, &mci);
			CloseHandle(hFile);
		}
	}


	inline std::wstring GetFileNameNoExtenion(const std::wstring& fullName)
	{
		std::wstring fileName = fullName;

		size_t i = fullName.rfind('\\');
		if (i == std::wstring::npos)
			i = i = fullName.rfind('/');

		if (i != std::wstring::npos)
			fileName = fullName.substr(i + 1, fullName.length() - i - 1);

		i = fileName.find('.');
		if (i != std::wstring::npos)
			fileName = fileName.substr(0, i);

		return fileName;
	}

	inline LONG WINAPI GPTUnhandledExceptionFilter(PEXCEPTION_POINTERS pExceptionInfo)
	{
		//得到当前时间
		SYSTEMTIME st;
		::GetLocalTime(&st);
		//得到程序所在文件夹
		wchar_t exeFullPath[256]; // MAX_PATH
		GetModuleFileNameW(NULL, exeFullPath, 256);//得到程序模块名称，全路径 
		std::wstring strPath(GetFileNameNoExtenion(exeFullPath));
		
		wchar_t szFileName[1024];
		wsprintf(szFileName, TEXT("%sERLOG_%04d%02d%02d%02d%02d%02d%02d%02d.dmp"), strPath, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, rand() % 100);
		CreateMiniDump(pExceptionInfo, szFileName);
		exit(pExceptionInfo->ExceptionRecord->ExceptionCode);
		return EXCEPTION_EXECUTE_HANDLER;    // 程序停止运行
	}

}

#endif
