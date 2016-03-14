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

		//SB win10 ÄÚºË°æ±¾ Ìø10
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


#endif
