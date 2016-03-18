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


#endif
