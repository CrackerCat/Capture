#ifndef D3D8_D3D9_OEPNGL_CPU_INL
#define D3D8_D3D9_OEPNGL_CPU_INL

//该文件的用途只是分离相同代码。not a header!

#include "../Capture/CaptureHUB.h"
#include "CaptureHook.h"
#include "Extern.h"
#include <Windows.h>

#define NUM_BACKBUFF 3

namespace {
	void* copy_data = NULL;
	UINT copy_index = 0;
	HANDLE copy_event = NULL;
	HANDLE copy_thread = NULL;
	bool copy_thread_run = true;
	h3d::MemoryInfo* copy_info = NULL;

	CRITICAL_SECTION data_mutexs[NUM_BACKBUFF];

	h3d::byte* tex_addrsss[2] = { NULL,NULL };
}


static DWORD WINAPI CopyTextureThread(LPVOID lpUseless) {
	logstream << "Begin CopyTextureThread" << std::endl;
	h3d::CaptureInfo* pThreadInfo = reinterpret_cast<h3d::CaptureInfo*>(lpUseless);

	HANDLE hEvent = NULL;
	if (!DuplicateHandle(GetCurrentProcess(), copy_event, GetCurrentProcess(), &hEvent, 0, FALSE, DUPLICATE_SAME_ACCESS)) {
		logstream << "CopyTextureThread DuplicateHandle Failed" << std::endl;
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
				memcpy(tex_addrsss[last_rendered], local_copy_data, pThreadInfo->Reserved4*pThreadInfo->oHeight);
				ReleaseMutex(texture_mutex[last_rendered]);
				copy_info->Reserved3 = last_rendered;
			}

			LeaveCriticalSection(&data_mutexs[local_copy_index]);
		}

		address_index = next_address_index;
	}

	CloseHandle(hEvent);
	logstream << "Exit CopyTextureThread" << std::endl;
	return 0;
}

static bool CopySignal(h3d::CaptureInfo& info, UINT ptich) {
	copy_thread_run = true;
	if (copy_thread = CreateThread(NULL, 0, CopyTextureThread, &info, 0, NULL)) {
		if (!(copy_event = CreateEvent(NULL, FALSE, FALSE, NULL))) {
			logstream << "Create CopyEvent Failed" << std::endl;
			return false;
		}
	}
	else
		return false;


	info.Reserved2 = h3d::CtorSharedMemCPUCapture(ptich*info.oHeight, info.Reserved3, copy_info, tex_addrsss);

	if (!info.Reserved2) {
		logstream << "Create Shared Memory Failed" << std::endl;
		return false;
	}

	has_textured = true;
	info.Reserved4 = ptich;
	memcpy(ptr_capture_info, &info, sizeof(h3d::CaptureInfo));
	SetEvent(hReadyEvent);
	return true;
}


#endif
