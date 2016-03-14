#include "CaptureHook.h"
#include "Extern.h"
#include "MinHook\MinHook.h"
#include <Windows.h>
#include <psapi.h>

h3d::WINAPIPROC h3d::GetVirtual(void* header_addrss, unsigned offset)
{
	h3d::PTR*  vtable = *(h3d::PTR**)header_addrss;

	return (WINAPIPROC)*(vtable + offset);
}


HANDLE hFileMap = NULL;
LPVOID mShare = NULL;


void h3d::InitCaptureHook()
{
	//MH_Initialize();
}

void h3d::ExitCaptureHook()
{
	//MH_Uninitialize();
}

h3d::APIHook::~APIHook()
{
	UnDo();
	//MH_RemoveHook(target_proc);
}

bool h3d::APIHook::Do(WINAPIPROC target, WINAPIPROC hook)
{
	if (is_hooked) {
		if (target == target_proc && hook_proc != hook) {
			hook_proc = hook;
			ReDo();
			return true;
		}
		UnDo();
	}

	target_proc = target;
	hook_proc = hook;

	if (!Probe())
		return false;

	ReDo();
	return true;
}

void h3d::APIHook::ReDo(bool force)
{
	if (!force && (is_hooked || !target_proc))
		return;

	//MH_EnableHook(target_proc);

	PTR start_addr = PTR(target_proc);//源地址，虚表中的函数指针值
	PTR target_addr = PTR(hook_proc);//目标地址

	ULONG64 offset = target_addr - (start_addr + 5);

	ULONG64 diff = 0;
	if (start_addr + 5 > target_addr)
		diff = start_addr + 5 - target_addr;
	else
		diff = target_addr - start_addr + 5;

	//64位系统常跳处理
#ifdef _WIN64
	// for 64 bit, try to use a shorter instruction sequence if we're too far apart, or we
	// risk overwriting other function prologues due to the 64 bit jump opcode length
	if (diff > 0x7fff0000 && !attempted_bounce)
	{
		MEMORY_BASIC_INFORMATION mem;

		// if we fail we don't want to continuously search memory every other call
		attempted_bounce = true;

		LPVOID bounce_address = NULL;
		if (VirtualQueryEx(GetCurrentProcess(), (LPCVOID)start_addr, &mem, sizeof(mem)))
		{
			int i, pagesize;
			ULONG64 address;
			SYSTEM_INFO systemInfo;

			GetSystemInfo(&systemInfo);
			pagesize = systemInfo.dwAllocationGranularity;

			// try to allocate a page somewhere below the target
			for (i = 0, address = (ULONG64)mem.AllocationBase - pagesize; i < 256; i++, address -= pagesize)
			{
				bounce_address = VirtualAlloc((LPVOID)address, pagesize, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
				if (bounce_address)
					break;
			}

			// if that failed, let's try above
			if (!bounce_address)
			{
				for (i = 0, address = (ULONG64)mem.AllocationBase + mem.RegionSize + pagesize; i < 256; i++, address += pagesize)
				{
					bounce_address = VirtualAlloc((LPVOID)address, pagesize, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
					if (bounce_address)
						break;
				}
			}

			if (bounce_address)
			{
				// we found some space, let's try to put the 64 bit jump code there and fix up values for the original hook
				ULONG64 newdiff;

				if (start_addr + 5 > PTR(bounce_address))
					newdiff = start_addr + 5 - PTR(bounce_address);
				else
					newdiff = PTR(bounce_address) - start_addr + 5;

				// first, see if we can reach it with a 32 bit jump
				if (newdiff <= 0x7fff0000)
				{
					// we can! update values so the shorter hook is written below
					logstream  << "Creating bounce hook at " << std::hex << bounce_address << " for " << std::hex <<start_addr <<
						" -> " << std::hex << target_addr << " (diff " << std::hex << diff << ")\n";
					FillMemory(bounce_address, pagesize, 0xCC);

					// write new jmp
					LPBYTE addrData = (LPBYTE)bounce_address;
					*(addrData++) = 0xFF;
					*(addrData++) = 0x25;
					*((LPDWORD)(addrData)) = 0;
					*((unsigned __int64*)(addrData + 4)) = target_addr;

					target_addr =PTR(bounce_address);
					offset = target_addr - (start_addr + 5);
					diff = newdiff;
				}
			}
		}
	}
#endif

	DWORD oldProtect;

#ifdef _WIN64
	is_64bitjump = (diff > 0x7fff0000);

	if (is_64bitjump)
	{
		LPBYTE addrData = (LPBYTE)target_proc;
		VirtualProtect(target_proc, 14, PAGE_EXECUTE_READWRITE, &oldProtect);
		*(addrData++) = 0xFF;
		*(addrData++) = 0x25;
		*((LPDWORD)(addrData)) = 0;
		*((unsigned __int64*)(addrData + 4)) = target_addr;
	}
	else
#endif
	{
		VirtualProtect(target_proc, 5, PAGE_EXECUTE_READWRITE, &oldProtect);

		LPBYTE addrData = (LPBYTE)target_proc;
		*addrData = 0xE9;
		*(DWORD*)(addrData + 1) = DWORD(offset);
	}

	is_hooked = true;
}

void h3d::APIHook::UnDo()
{
	if (!is_hooked || !target_proc)
		return;

	UINT count = is_64bitjump ? 14 : 5;
	DWORD old_protect;
	VirtualProtect(target_proc, count, PAGE_EXECUTE_READWRITE, &old_protect);
	memcpy(target_proc, data, count);
	is_hooked = false;
}

bool h3d::APIHook::Probe()
{
	DWORD oldProtect;
	if (!VirtualProtect(target_proc, 14, PAGE_EXECUTE_READWRITE, &oldProtect))
		return false;

#ifndef _WIN64
	//check other hooking application
	if (*(byte*)target_proc == 0XE9 || *(byte*)target_proc == 0XE8) {
		CHAR		*modName, *ourName;
		CHAR		szModName[MAX_PATH];
		CHAR		szOurName[MAX_PATH];
		DWORD		memAddress;

		MEMORY_BASIC_INFORMATION mem;

		INT_PTR jumpAddress = *(DWORD *)((BYTE *)target_proc + 1) + (DWORD)target_proc;

		// try to identify target
		if (VirtualQueryEx(GetCurrentProcess(), (LPVOID)jumpAddress, &mem, sizeof(mem)) && mem.State == MEM_COMMIT)
			memAddress = (DWORD)mem.AllocationBase;
		else
			memAddress = jumpAddress;

		if (GetMappedFileNameA(GetCurrentProcess(), (LPVOID)memAddress, szModName, _countof(szModName) - 1))
			modName = szModName;
		else if (GetModuleFileNameA((HMODULE)memAddress, szModName, _countof(szModName) - 1))
			modName = szModName;
		else
			modName = "unknown";

		// and ourselves
		if (VirtualQueryEx(GetCurrentProcess(), (LPVOID)target_proc, &mem, sizeof(mem)) && mem.State == MEM_COMMIT)
			memAddress = (DWORD)mem.AllocationBase;
		else
			memAddress = (DWORD)target_proc;

		if (GetMappedFileNameA(GetCurrentProcess(), (LPVOID)memAddress, szOurName, _countof(szOurName) - 1))
			ourName = szOurName;
		else if (GetModuleFileNameA((HMODULE)memAddress, szOurName, _countof(szOurName) - 1))
			ourName = szOurName;
		else
			ourName = "unknown";

		CHAR *p = strrchr(ourName, '\\');
		if (p)
			ourName = p + 1;

		//<<"WARNING: Another hook is already present while trying to hook "<<ourname << ", hook target is " << modName <<
		 //". If you experience crashes, try disabling the other hooking application"
	}
#endif
	memcpy(data, target_proc, 14);
	return true;
}

#include <sstream>
#include <string>

static UINT share_memory_count = 0;
unsigned int h3d::CtorSharedMemCPUCapture(unsigned int texsize, unsigned int & totalsize, MemoryInfo *& share, byte ** texaddress)
{
	UINT alignedHeader = (sizeof(MemoryInfo) + 15) & 0XFFFFFFF0;
	UINT alignedTex = (texsize + 15) & 0XFFFFFFF0;

	totalsize = alignedHeader + alignedTex * 2;

	std::wstringstream memNamestream;
	memNamestream << TEXTURE_MEMORY << ++share_memory_count;

	hFileMap = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, totalsize, memNamestream.str().c_str());
	if (!hFileMap)
		return 0;

	mShare = MapViewOfFile(hFileMap, FILE_MAP_ALL_ACCESS, 0, 0, totalsize);
	if (!mShare) {
		CloseHandle(hFileMap);
		hFileMap = NULL;
		return 0;
	}

	share = (MemoryInfo*)mShare;
	share->Reserved1 = alignedHeader;//texture1offset;
	share->Reserved2 = alignedHeader + alignedTex;//texture2offset

	texaddress[0] = (h3d::byte*)mShare + alignedHeader;
	texaddress[1] = (h3d::byte*)mShare + alignedHeader + alignedTex;

	return share_memory_count;
}

unsigned int h3d::CtorSharedMemGPUCapture(unsigned __int64 ** handle)
{
	int totalsize = sizeof(unsigned __int64);

	std::wstringstream memNamestream;
	memNamestream << TEXTURE_MEMORY << ++share_memory_count;

	hFileMap = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, totalsize, memNamestream.str().c_str());
	if (!hFileMap)
		return 0;

	mShare = MapViewOfFile(hFileMap, FILE_MAP_ALL_ACCESS, 0, 0, totalsize);
	if (!mShare) {
		CloseHandle(hFileMap);
		hFileMap = NULL;
		return 0;
	}

	*handle = reinterpret_cast<unsigned __int64*>(mShare);

	return share_memory_count;
}

void h3d::DestroySharedMem()
{
	if (mShare && hFileMap) {
		UnmapViewOfFile(mShare);
		CloseHandle(hFileMap);

		hFileMap = NULL;
		mShare = NULL;
	}
}
