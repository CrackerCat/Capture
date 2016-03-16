
// ChildView.h : CChildView e�Ľ���
//
#include "..\Capture\CaptureHUB.h"
#include "T.h"
#include <string>
#include <vector>
#pragma once


#include <fstream>
extern std::ofstream logstream;
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
	h3d::CaptureHUB* gdi_capture;
	HANDLE hKeepAlive;

	CListCtrl cList;
// ���I
public:
	BOOL IterWindow(HWND hwnd);
	void FilterWindow(bool game = true);
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
	afx_msg void OnGDICapture();
	afx_msg void OnGameCapture();
	afx_msg void OnCameraCapture();

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnTimer(UINT_PTR nIDEvent);
};

