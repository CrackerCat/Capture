
// CapturePlay.h : CapturePlay ���ó�ʽ�������^�n
//
#pragma once

#ifndef __AFXWIN_H__
	#error "�� PCH �����˙n��ǰ�Ȱ��� 'stdafx.h'"
#endif

#include "resource.h"       // ��Ҫ��̖


// CCapturePlayApp:
// Ո��醌�����e�� CapturePlay.cpp
//

class CCapturePlayApp : public CWinApp
{
public:
	CCapturePlayApp();


// ����
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

// ��ʽ�a����

public:
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
};

extern CCapturePlayApp theApp;
