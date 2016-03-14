
// ChildView.cpp : CChildView 類別的實作
//

#include "stdafx.h"
#include "CapturePlay.h"
#include "ChildView.h"
#include "..\Capture\GraphicCapture.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CChildView

CChildView::CChildView()
:game_capture(NULL),camera_capture(NULL), hKeepAlive(NULL)
{
	//cList.SetExtendedStyle(LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);
}



CChildView::~CChildView()
{
	if (game_capture)
		h3d::EndCapture(game_capture);

	if (camera_capture)
		h3d::EndCapture(camera_capture);

	if (hKeepAlive)
		CloseHandle(hKeepAlive);
}


BEGIN_MESSAGE_MAP(CChildView, CWnd)
	ON_WM_PAINT()
	ON_COMMAND(ID_D3D9_CAPTURE,&CChildView::OnD3D9Capture)
	ON_COMMAND(ID_DESKTOP_CAPTURE,&CChildView::OnDesktopCapture)
	ON_COMMAND(ID_D3D11_CAPTURE,&CChildView::OnD3D11Capture)
	ON_COMMAND(ID_D3D10_CAPTURE,&CChildView::OnD3D10Capture)
	ON_COMMAND(ID_GAME_CAPTURE,&CChildView::OnGameCapture)
	ON_WM_TIMER()
	ON_COMMAND(ID_CAMERA_CAPTURE, &CChildView::OnCameraCapture)
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

		CRect rect;
		GetWindowRect(&rect);
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

		CRect rect;
		GetWindowRect(&rect);

		dc.StretchBlt(0,0, texture->GetWidth(), texture->GetHeight(), &Dc, 0, 0, texture->GetWidth(), texture->GetHeight(), SRCCOPY);
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

			HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, aProcesses[i]);

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

void CChildView::OnD3D9Capture()
{
	GameCapture(FindProcess(L"D3D9.exe"));
}

#include "../Capture/GDICapture.h"

void CChildView::OnDesktopCapture()
{
	/*CRect rect;
	GetWindowRect(&rect);

	if (capture)
		h3d::EndCapture(capture);
	hKeepAlive = NULL;

	h3d::CaptureInfo info;
	info.oHeight = rect.Height();
	info.oWidth = rect.Width();
	info.sNative = NULL;

	capture = new h3d::GDICapture(info);*/
}

void CChildView::OnD3D11Capture()
{
	GameCapture(FindProcess(L"D3D11.exe"));
}

void CChildView::OnD3D10Capture()
{
	GameCapture(FindProcess(L"D3D10.exe"));
}

BOOL __stdcall EnumVisibleWindow(HWND hwnd, LPARAM lParam) {
	return reinterpret_cast<CChildView*>(lParam)->IterWindow(hwnd);
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

	HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
	if (!hProcess)
		return TRUE;

	DWORD dwSize = MAX_PATH;
	wchar_t fileName[MAX_PATH];
	QueryFullProcessImageNameW(hProcess, 0, fileName, &dwSize);


	std::wstringstream wss;
	wss << windowName << "(" << GetFileNameNoExtenion(fileName) << ")";

	windows.push_back(WindowData(wss.str().c_str(),hwnd));

	return TRUE;
}

#include "T.h"

void CChildView::OnGameCapture()
{
	//枚举窗口
	windows.clear();
	EnumWindows(EnumVisibleWindow,(LPARAM)this);
	//显示列表
	T box(windows);
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
