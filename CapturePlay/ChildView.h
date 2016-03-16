
// ChildView.h : CChildView 類別的介面
//
#include "..\Capture\CaptureHUB.h"
#include "T.h"
#include <string>
#include <vector>
#pragma once


#include <fstream>
extern std::ofstream logstream;
// CChildView 視窗

class CChildView : public CWnd
{
// 建構
public:
	CChildView();

// 屬性
public:
	h3d::CaptureHUB* camera_capture;
	h3d::CaptureHUB* game_capture;
	h3d::CaptureHUB* gdi_capture;
	HANDLE hKeepAlive;

	CListCtrl cList;
// 作業
public:
	BOOL IterWindow(HWND hwnd);
	void FilterWindow(bool game = true);
// 覆寫
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

private:
	
	std::vector<WindowData> windows;

	void GameCapture(DWORD processId);
public:
// 程式碼實作
public:
	virtual ~CChildView();

	// 產生的訊息對應函式
protected:
	afx_msg void OnPaint();
	afx_msg void OnGDICapture();
	afx_msg void OnGameCapture();
	afx_msg void OnCameraCapture();

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnTimer(UINT_PTR nIDEvent);
};

