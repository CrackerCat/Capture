
// ChildView.cpp : CChildView 類別的實作
//

#include "stdafx.h"
#include "CapturePlay.h"
#include "ChildView.h"
#include "..\Capture\GraphicCapture.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

std::ofstream logstream;

// CChildView

CChildView::CChildView()
:scene_capture(NULL),sws_context(NULL),hBitmap(NULL)
{
	logstream.open("CapturePlay.log", std::ios_base::in | std::ios_base::out | std::ios_base::trunc, 0X40);
}



CChildView::~CChildView()
{
	if (scene_capture)
		scene_capture->Stop();

	delete scene_capture;
	
	for(auto handle:hKeepAlives)
		CloseHandle(handle);

	if (logstream.is_open())
		logstream.close();

	if (sws_context)
		sws_freeContext(sws_context);
}


BEGIN_MESSAGE_MAP(CChildView, CWnd)
	ON_WM_PAINT()
	ON_COMMAND(ID_CAPTURE_GDI, &CChildView::OnGDICapture)
	ON_COMMAND(ID_CAPTURE_GAME,&CChildView::OnGameCapture)
	ON_COMMAND(ID_CAPTURE_CAMERA, &CChildView::OnCameraCapture)
	ON_WM_TIMER()
END_MESSAGE_MAP()



// CChildView 訊息處理常式



BOOL CChildView::PreCreateWindow(CREATESTRUCT& cs)
{
	if (!CWnd::PreCreateWindow(cs))
		return FALSE;

	cs.dwExStyle |= WS_EX_CLIENTEDGE;
	cs.style &= ~WS_BORDER;
	cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS, 
		::LoadCursor(NULL, IDC_ARROW), reinterpret_cast<HBRUSH>(COLOR_WINDOW+1), NULL);

	return TRUE;
}

#pragma warning(disable:4018)
void CChildView::OnPaint() 
{
	CPaintDC dc(this); // 繪製的裝置內容

	CRect rect;
	GetWindowRect(&rect);

	static long window_w = rect.Width();
	static long window_h = rect.Height();

	if (scene_capture) {
		h3d::CaptureTexture*  texture = scene_capture->Capture();

		if (!texture)
			return;

		static long capture_w = texture->GetWidth();
		static long capture_h = texture->GetHeight();

		if (!sws_context)
			sws_context = sws_getContext(texture->GetWidth(), texture->GetHeight(), AV_PIX_FMT_BGRA, rect.Width(), rect.Height(), AV_PIX_FMT_BGRA,  SWS_LANCZOS, NULL, NULL, NULL);

		if (rect.Width() != window_w || rect.Height() != window_h || capture_w != texture->GetWidth() || capture_h != texture->GetHeight())
		{
			sws_freeContext(sws_context);
			sws_context = sws_getContext(texture->GetWidth(), texture->GetHeight(), AV_PIX_FMT_BGRA, rect.Width(), rect.Height(), AV_PIX_FMT_BGRA, SWS_LANCZOS, NULL, NULL, NULL);
			window_w = rect.Width();
			window_h = rect.Height();
			capture_w = texture->GetWidth();
			capture_h = texture->GetHeight();
		}

		static CDC Dc;
		if(!Dc.m_hDC)
			Dc.CreateCompatibleDC(&dc);
		static BITMAPINFO bitmap_info;

		static BYTE* pBitmapData = NULL;
		if (!hBitmap) {
			bitmap_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			bitmap_info.bmiHeader.biWidth = rect.Width();
			bitmap_info.bmiHeader.biHeight = rect.Height();
			bitmap_info.bmiHeader.biPlanes = 1;
			bitmap_info.bmiHeader.biBitCount = 32;
			bitmap_info.bmiHeader.biCompression = BI_RGB;

			hBitmap = ::CreateDIBSection(NULL, &bitmap_info, DIB_RGB_COLORS, reinterpret_cast<void**>(&pBitmapData), NULL, 0);
		}

		if (rect.Width() != window_w || rect.Height() != window_h) {
			bitmap_info.bmiHeader.biWidth = rect.Width();
			bitmap_info.bmiHeader.biHeight = rect.Height();

			DeleteObject(hBitmap);
			pBitmapData = NULL;
			hBitmap = ::CreateDIBSection(NULL, &bitmap_info, DIB_RGB_COLORS, reinterpret_cast<void**>(&pBitmapData), NULL, 0);
		}

		int bitmap_pitch = ((rect.Width() * bitmap_info.bmiHeader.biBitCount + 31) / 32) * 4;

		if (hBitmap)
			Dc.SelectObject(hBitmap);


		h3d::CaptureTexture::MappedData data = texture->Map();
		BYTE* src_slice = data.pData + (texture->GetHeight() - 1)*data.RowPitch;
		int src_pitch = -static_cast<int>(data.RowPitch);
		sws_scale(sws_context, &src_slice, &src_pitch, 0, texture->GetHeight(), &pBitmapData, &bitmap_pitch);
		texture->UnMap();

		dc.StretchBlt(0,0, rect.Width(), rect.Height(), &Dc, 0, 0,rect.Width(), rect.Height(), SRCCOPY);
	}
}


#include <Psapi.h>

DWORD FindProcess(const wchar_t* processName) {
	DWORD aProcesses[1024], cbNeeded, cProcesses;

	if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded))
		return 0;

	cProcesses = cbNeeded / sizeof(DWORD);

	HANDLE hTargetProcess = NULL;

	for (int i = 0; i < cProcesses; ++i) {
		if (aProcesses[i] != 0) {

			wchar_t szProcessName[MAX_PATH] = L"<unknown>";

			HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, aProcesses[i]);

			if (NULL != hProcess) {
				HMODULE hMod;
				DWORD cbNeeded;
				if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded)) {
					GetModuleBaseName(hProcess, hMod, szProcessName, sizeof(szProcessName) / sizeof(wchar_t));

					if (StrCmpCW(szProcessName, processName) == 0)
					{
						return aProcesses[i];
					}
				}
			}
		}
	}

	return 0;
}

#define PAINT_TIMER 1
#include <sstream>

#include "../Capture/SceneCapture.h"
void CChildView::GameCapture(DWORD processId)
{
	if (!processId)
		return;

	if (!scene_capture)
		scene_capture = h3d::SceneCapture::Serialize(NULL);
	

	std::wstringstream wss;
	wss << OBS_KEEP_ALIVE << processId;
	HANDLE keep_alive = CreateEventW(NULL, FALSE, FALSE, wss.str().c_str());
	hKeepAlives.push_back(keep_alive);

	if (scene_capture->AddCapture("123", h3d::GAME_CAPTURE, processId))
		SetTimer(PAINT_TIMER,40, 0);
}



BOOL __stdcall EnumVisibleWindow(HWND hwnd, LPARAM lParam) {
	return reinterpret_cast<CChildView*>(lParam)->IterWindow(hwnd);
}


void CChildView::OnGDICapture()
{
	KillTimer(PAINT_TIMER);
	//枚举窗口
	windows.clear();
	windows.emplace_back(L"桌面", ::GetDesktopWindow());
	EnumWindows(EnumVisibleWindow, (LPARAM)this);
	//显示列表
	T box(windows, L"请选择GDI源");
	box.DoModal();
	//开始抓取
	if (box.ref_index != LB_ERR) {
		auto& window = windows[box.ref_index];

		if (!scene_capture)
			scene_capture = h3d::SceneCapture::Serialize(NULL);

		if (scene_capture->AddCapture("321", h3d::WINDOW_CAPTURE,reinterpret_cast<unsigned long>(window.hWnd)))
			SetTimer(PAINT_TIMER, 40, 0);
	}
}


const wchar_t* special_game_windowname[] = {
	L"battlefield"
};

#include <sstream>

std::wstring GetFileNameNoExtenion(const std::wstring& fullName)
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


BOOL CChildView::IterWindow(HWND hwnd)
{
	//滤掉
	if (!::IsWindowVisible(hwnd))
		return TRUE;

	//滤掉
	if (::IsIconic(hwnd))
		return TRUE;

	std::wstring windowName;
	windowName.resize(::GetWindowTextLengthW(hwnd));
	::GetWindowTextW(hwnd, &windowName[0], windowName.size() + 1);


	DWORD ex_style = ::GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
	DWORD style = ::GetWindowLongPtrW(hwnd, GWL_STYLE);

	//避免滤掉了奇葩游戏
	for (auto special_name : special_game_windowname)
		if (windowName.find(special_name) != std::wstring::npos)
			ex_style &= ~WS_EX_TOOLWINDOW;

	//滤掉
	if (ex_style & WS_EX_TOOLWINDOW)
		return TRUE;

	//滤掉
	if (style & WS_CHILD)
		return TRUE;
	

	//打开进程，打不开也滤掉
	DWORD processId;
	GetWindowThreadProcessId(hwnd, &processId);
	//你难道要抓取自己直播？
	if (processId == GetCurrentProcessId())
		return TRUE;

	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, processId);
	if (!hProcess) {
		logstream << "CChildView::IterWindow OpenProcess Failed " << " Error Code = " << GetLastError();
		logstream<<" PID = "	<<processId << std::endl;
		return TRUE;
	}

	DWORD dwSize = MAX_PATH;
	wchar_t fileName[MAX_PATH];
	GetProcessImageFileNameW(hProcess, fileName,dwSize);
	CloseHandle(hProcess);

	std::wstringstream wss;
	wss << windowName << "(" << GetFileNameNoExtenion(fileName) << ")";

	windows.push_back(WindowData(wss.str().c_str(),hwnd));

	return TRUE;
}


void CChildView::FilterWindow(bool game)
{
	if (!game)
		windows.emplace_back( L"桌面",::GetDesktopWindow());
}

#include "T.h"

void CChildView::OnGameCapture()
{
	KillTimer(PAINT_TIMER);
	//枚举窗口
	windows.clear();
	EnumWindows(EnumVisibleWindow,(LPARAM)this);
	//显示列表
	T box(windows,L"请选择游戏源");
	box.DoModal();
	//开始抓取
	if (box.ref_index != LB_ERR) {
		auto& window = windows[box.ref_index];
		DWORD processId = 0;
		GetWindowThreadProcessId(window.hWnd, &processId);
		GameCapture(processId);
	}
}



void CChildView::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: Add your message handler code here and/or call default
	switch (nIDEvent)
	{
	case PAINT_TIMER:
		Invalidate(FALSE);
		return;
	}
	CWnd::OnTimer(nIDEvent);
}

#include "CameraDialog.h"
#include "../Capture/CameraCapture.h"
void CChildView::OnCameraCapture()
{
	KillTimer(PAINT_TIMER);
	CameraDialog box;
	box.DoModal();
	if (box.camera_index != LB_ERR) {
		if (!scene_capture)
			scene_capture = h3d::SceneCapture::Serialize(NULL);

		if (scene_capture->AddCapture("camera", h3d::GAME_CAPTURE, box.camera_index))
			SetTimer(PAINT_TIMER, 40, 0);
	}
}
