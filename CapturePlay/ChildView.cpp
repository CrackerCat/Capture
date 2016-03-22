
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
:game_capture(NULL),camera_capture(NULL), hKeepAlive(NULL),gdi_capture(NULL)
{
	logstream.open("CapturePlay.log", std::ios_base::in | std::ios_base::out | std::ios_base::trunc, 0X40);
}



CChildView::~CChildView()
{
	if (game_capture)
		h3d::EndCapture(game_capture);

	if (camera_capture)
		h3d::EndCapture(camera_capture);

	if (hKeepAlive)
		CloseHandle(hKeepAlive);

	if (logstream.is_open())
		logstream.close();
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
#include "../Capture/GDICapture.h"

#pragma warning(disable:4018)
void CChildView::OnPaint() 
{
	CPaintDC dc(this); // 繪製的裝置內容

	CRect rect;
	GetWindowRect(&rect);

	if (camera_capture) {
		h3d::CaptureTexture*  texture = camera_capture->Capture();

		if (!texture)
			return;

		CDC Dc;
		Dc.CreateCompatibleDC(&dc);
		BITMAPINFO bitmap_info;
		bitmap_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bitmap_info.bmiHeader.biWidth = texture->GetWidth();
		bitmap_info.bmiHeader.biHeight = texture->GetHeight();
		bitmap_info.bmiHeader.biPlanes = 1;
		bitmap_info.bmiHeader.biBitCount = 32;
		bitmap_info.bmiHeader.biCompression = BI_RGB;

		BYTE* pBitmapData = NULL;
		HBITMAP hBitmap = ::CreateDIBSection(NULL, &bitmap_info, DIB_RGB_COLORS,reinterpret_cast<void**>(&pBitmapData), NULL, 0);

		h3d::CaptureTexture::MappedData data = texture->Map();
		if (pBitmapData)
			memcpy(pBitmapData, data.pData, texture->GetWidth()*texture->GetHeight() * 4);
		texture->UnMap();

		CBitmap Bitmap;
		Bitmap.Attach(hBitmap);
		Dc.SelectObject(&Bitmap);

		//坑爹HBITMAP坐标系 源开始
		dc.StretchBlt(0,rect.Height(), rect.Width(),-rect.Height(), &Dc, 0,0,texture->GetWidth(),texture->GetHeight(),SRCCOPY);
	}
	
	//无聊的代码。绘制 1/8区域的大小
	if (game_capture) {
		h3d::CaptureTexture*  texture = game_capture->Capture();

		if (!texture)
			return;

		CDC Dc;
		Dc.CreateCompatibleDC(&dc);
		BITMAPINFO bitmap_info;
		bitmap_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bitmap_info.bmiHeader.biWidth = texture->GetWidth();
		bitmap_info.bmiHeader.biHeight = texture->GetHeight();
		bitmap_info.bmiHeader.biPlanes = 1;
		bitmap_info.bmiHeader.biBitCount = 32;
		bitmap_info.bmiHeader.biCompression = BI_RGB;

		BYTE* pBitmapData = NULL;
		HBITMAP hBitmap = ::CreateDIBSection(NULL, &bitmap_info, DIB_RGB_COLORS, reinterpret_cast<void**>(&pBitmapData), NULL, 0);
		int bitmap_pitch = ((texture->GetWidth() * bitmap_info.bmiHeader.biBitCount + 31) / 32) * 4;

		h3d::CaptureTexture::MappedData data = texture->Map();
		if (pBitmapData)
			for (int y = 0; y != texture->GetHeight();++y)//在这里处理翻转逻辑
				memcpy(pBitmapData+bitmap_pitch*(texture->GetHeight()-1-y), data.pData+data.RowPitch*y, texture->GetWidth()*4);
		texture->UnMap();

		CBitmap Bitmap;
		Bitmap.Attach(hBitmap);
		Dc.SelectObject(&Bitmap);

		

		dc.StretchBlt(0,0, texture->GetWidth(), texture->GetHeight(), &Dc, 0, 0, texture->GetWidth(), texture->GetHeight(), SRCCOPY);
	}

	if (gdi_capture) {
		h3d::CaptureTexture* texture = gdi_capture->Capture();
		if (!texture)
			return;

		CDC Dc;
		Dc.CreateCompatibleDC(&dc);
		BITMAPINFO bitmap_info;
		bitmap_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bitmap_info.bmiHeader.biWidth = texture->GetWidth();
		bitmap_info.bmiHeader.biHeight = texture->GetHeight();
		bitmap_info.bmiHeader.biPlanes = 1;
		bitmap_info.bmiHeader.biBitCount = 32;
		bitmap_info.bmiHeader.biCompression = BI_RGB;

		BYTE* pBitmapData = NULL;
		HBITMAP hBitmap = ::CreateDIBSection(NULL, &bitmap_info, DIB_RGB_COLORS, reinterpret_cast<void**>(&pBitmapData), NULL, 0);
		int bitmap_pitch = ((texture->GetWidth() * bitmap_info.bmiHeader.biBitCount + 31) / 32) * 4;

		h3d::CaptureTexture::MappedData data = texture->Map();
		if (pBitmapData && data.pData)
			for (int y = 0; y != texture->GetHeight(); ++y)//在这里处理翻转逻辑
				memcpy(pBitmapData + bitmap_pitch*(texture->GetHeight() - 1 - y), data.pData + data.RowPitch*y, texture->GetWidth() * 4);
		texture->UnMap();

		CBitmap Bitmap;
		Bitmap.Attach(hBitmap);
		Dc.SelectObject(&Bitmap);



		dc.StretchBlt(0, 0, texture->GetWidth(), texture->GetHeight(), &Dc, 0, 0, texture->GetWidth(), texture->GetHeight(), SRCCOPY);
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

void CChildView::GameCapture(DWORD processId)
{
	if (!processId)
		return;

	if (game_capture) {
		h3d::EndCapture(game_capture);
		KillTimer(PAINT_TIMER);
	}

	if (hKeepAlive)
		CloseHandle(hKeepAlive);
	hKeepAlive = NULL;


	std::wstringstream wss;
	wss << OBS_KEEP_ALIVE << processId;
	hKeepAlive = CreateEventW(NULL, FALSE, FALSE, wss.str().c_str());

	game_capture = h3d::GraphicCapture(processId);
	SetTimer(PAINT_TIMER,40, 0);
}



BOOL __stdcall EnumVisibleWindow(HWND hwnd, LPARAM lParam) {
	return reinterpret_cast<CChildView*>(lParam)->IterWindow(hwnd);
}


void CChildView::OnGDICapture()
{
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

		CRect rect;
		GetWindowRect(&rect);

		h3d::CaptureInfo info;
		info.sNative =reinterpret_cast<unsigned __int64>(window.hWnd);

		if (gdi_capture)
			delete gdi_capture;
		gdi_capture = new h3d::GDICapture(info);
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
	CameraDialog box;
	box.DoModal();
	if (box.camera_index != LB_ERR) {
		if (camera_capture) {
			h3d::EndCapture(camera_capture);
			KillTimer(PAINT_TIMER);
		}

		camera_capture = new h3d::CameraCapture(h3d::CaptureInfo(), box.camera_index);
		SetTimer(PAINT_TIMER, 40, 0);
	}
}
