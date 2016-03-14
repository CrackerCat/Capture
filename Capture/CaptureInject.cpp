#include "CaptureInject.h"
#include <Windows.h>
#include <fstream>

extern std::ofstream logstream;

bool h3d::AdjustToken() {

	HANDLE hToken;
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
		return false;

	LUID val;
	if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &val)) {
		CloseHandle(hToken);
		return false;
	}

	TOKEN_PRIVILEGES tp;
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = val;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL)) {
		CloseHandle(hToken);
		logstream << "AdjustTokenPrivileges Failed" << std::endl;
		return false;
	}
	logstream << "AdjustTokenPrivileges Succ" << std::endl;
	CloseHandle(hToken);
	return true;
}


typedef BOOL(WINAPI *WriteProcessMemoryProc)(HANDLE, LPVOID, LPCVOID, SIZE_T, SIZE_T*);
typedef HANDLE(WINAPI *CreateRemoteThreadProc)(HANDLE, LPSECURITY_ATTRIBUTES, SIZE_T, LPTHREAD_START_ROUTINE, LPVOID, DWORD, LPDWORD);
typedef LPVOID(WINAPI *VirtualAllocExProc)(HANDLE, LPVOID, SIZE_T, DWORD, DWORD);
typedef BOOL(WINAPI *VirtualFreeExProc)(HANDLE, LPVOID, SIZE_T, DWORD);
typedef HANDLE(WINAPI *LoadLibrayWProc) (DWORD, BOOL, DWORD);

bool h3d::InjectDLL(void * hProcess,const std::wstring & dllpath)
{
	//混淆函数名,动态载入注入所需函数,防止程序被误杀
	//也许更容易被查杀?
	char str_WriteProcessMemory[19];
	char str_CreateRemoteThread[19];
	char str_VirtualAllocEx[15];
	char str_VirtualFreeEx[14];
	char str_LoadLibray[13];

	memcpy(str_WriteProcessMemory, "RvnrdPqmni|}Dmfegm", 19);
	memcpy(str_CreateRemoteThread, "FvbgueQg`c{k]`yotp", 19);
	memcpy(str_VirtualAllocEx, "WiqvpekGeddiHt", 15);
	memcpy(str_VirtualFreeEx, "Wiqvpek@{mnOu", 14);
	memcpy(str_LoadLibray, "MobfImethzr", 12);

	str_LoadLibray[11] = 'W';
	str_LoadLibray[12] = 0;

	int obfSize = 12;

	obfSize += 6;
	for (int i = 0; i < obfSize; ++i) str_WriteProcessMemory[i] ^= i ^ 5;
	for (int i = 0; i < obfSize; ++i) str_CreateRemoteThread[i] ^= i ^ 5;

	obfSize -= 4;
	for (int i = 0; i < obfSize; ++i) str_VirtualAllocEx[i] ^= i ^ 1;
	
	obfSize -= 1;
	for (int i = 0; i < obfSize; ++i) str_VirtualFreeEx[i] ^= i ^ 1;

	obfSize -= 2;
	for (int i = 0; i < obfSize; ++i) str_LoadLibray[i] ^= i ^ 1;

	//内核32一定会被载入
	HMODULE hModule = GetModuleHandleW(L"KERNEL32");

	//获得函数地址
	WriteProcessMemoryProc fWriteProcessMemory = (WriteProcessMemoryProc)GetProcAddress(hModule, str_WriteProcessMemory);
	CreateRemoteThreadProc fCreateRemoteThread = (CreateRemoteThreadProc)GetProcAddress(hModule, str_CreateRemoteThread);
	VirtualAllocExProc fVirtualAllocEx = (VirtualAllocExProc)GetProcAddress(hModule, str_VirtualAllocEx);
	VirtualFreeExProc fVirtualFreeEx = (VirtualFreeExProc)GetProcAddress(hModule, str_VirtualFreeEx);

	//在目标进程分配一个内存，用于存放dll的路径，以便进行LoadLibrary
	DWORD  dwSize = (dllpath.size() + 1) * sizeof(wchar_t);
	LPVOID pStr = (*fVirtualAllocEx)(hProcess, NULL, dwSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (!pStr)
		return false;

	DWORD dwWriteSize = 0;
	BOOL bStrWrite = (*fWriteProcessMemory)(hProcess, pStr, dllpath.c_str(), dwSize, &dwWriteSize);

	LoadLibrayWProc fLoadLibrary = (LoadLibrayWProc)GetProcAddress(hModule, str_LoadLibray);
	if (!fLoadLibrary) {
		(*fVirtualFreeEx)(hProcess, pStr, 0, MEM_RELEASE);
		return false;
	}

	DWORD returnValue = 0;//IA-64 may be crash,ignore it
	HANDLE hThread = (*fCreateRemoteThread)(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)fLoadLibrary, pStr, 0, &returnValue);

	if (!hThread) {
		(*fVirtualFreeEx)(hProcess, pStr, 0, MEM_RELEASE);
		return false;
	}

	if (WaitForSingleObject(hThread, 200) == WAIT_OBJECT_0){
		DWORD exitCode;
		GetExitCodeThread(hThread, &exitCode);

		//LoadLibrary fails, the return value is NULL
		if (exitCode == 0) {
			CloseHandle(hThread);
			(*fVirtualFreeEx)(hProcess, pStr, 0, MEM_RELEASE);
			logstream << "LoadLibrary Failed" << std::endl;
			return false;
		}
	}

	CloseHandle(hThread);
	(*fVirtualFreeEx)(hProcess, pStr, 0, MEM_RELEASE);
	logstream << "InjectDLL Succ" << std::endl;
	return true;
}
