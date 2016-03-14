
// ChildView.h : CChildView e�Ľ���
//
#include "..\Capture\CaptureHUB.h"
#include "T.h"
#include <string>
#include <vector>
#pragma once


// CChildView ҕ��

class CChildView : public CWnd
{
// ����
public:
	CChildView();

// ����
public:
	h3d::CaptureHUB* camera_capture;
	h3d::CaptureHUB* game_capture;
	HANDLE hKeepAlive;

	CListCtrl cList;
// ���I
public:
	BOOL IterWindow(HWND hwnd);
// ����
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

private:
	
	std::vector<WindowData> windows;

	void GameCapture(DWORD processId);
public:
// ��ʽ�a����
public:
	virtual ~CChildView();

	// �a����ӍϢ������ʽ
protected:
	afx_msg void OnPaint();
	afx_msg void OnD3D9Capture();
	afx_msg void OnDesktopCapture();
	afx_msg void OnD3D11Capture();
	afx_msg void OnD3D10Capture();
	afx_msg void OnGameCapture();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnCameraCapture();
};

