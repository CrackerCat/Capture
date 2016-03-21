#ifndef VIDEOENGINECAPTUREEXTERN_H
#define VIDEOENGINECAPTUREEXTERN_H

#include <Windows.h>
#include <fstream>
#include <string>

extern std::wfstream logstream;
extern HWND hSender;

extern bool target_acquired;
extern bool capture_run;

extern void* ptr_capture_info;
extern HANDLE texture_mutex[2];

extern wchar_t game_name[MAX_PATH];

extern HANDLE hReadyEvent;//send to Observer
extern HANDLE hStopEvent;//send to Capturer
extern HANDLE hBeginEvent;//send to Capturer

extern std::wstring sKeepAlive;
#define KEEP_TIME_DURATION 1000

#endif
