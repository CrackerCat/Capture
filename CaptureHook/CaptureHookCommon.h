#ifndef VIDEOENGINECAPTURHOOK_HOOKCOMMON_H
#define VIDEOENGINECAPTURHOOK_HOOKCOMMON_H

#include "../Capture/WinPlatform.h"
#include "CaptureHook.h"
#include "Extern.h"

namespace h3d {

	inline void EventProcess() {
		if (capture_run && WaitForSingleObject(hStopEvent, 0) == WAIT_OBJECT_0) {
			capture_run = false;
			logstream << "received stop event" << std::endl;
		}

		if (!capture_run && WaitForSingleObject(hBeginEvent, 0) == WAIT_OBJECT_0) {
			capture_run = true;
			logstream << "received begin(restart) event " << std::endl;
		}
	}

#define KeepAliveProcess(clear_statement) static LONGLONG prev_point = h3d::GetOSMillSeconds(); \
	if (capture_run) { \
		LONGLONG capture_tp = h3d::GetOSMillSeconds(); \
		if(capture_tp - prev_point > KEEP_TIME_DURATION) {\
				HANDLE keepAlive = OpenEventW(EVENT_ALL_ACCESS, FALSE, sKeepAlive.c_str()); \
				if (keepAlive)\
					CloseHandle(keepAlive); \
				else {\
						clear_statement; \
						logstream << "Don't Exist Event[" <<sKeepAlive<<"] (OBS Process Unexpected Exit,Wait Next Begin Event)" << std::endl; \
						capture_run = false; \
				}\
				prev_point = capture_tp; \
		}\
	}

}
#endif
