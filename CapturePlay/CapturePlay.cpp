
// CapturePlay.cpp : ���x���ó�ʽ��e�О顣
//

#include "stdafx.h"
#include "afxwinappex.h"
#include "afxdialogex.h"
#include "CapturePlay.h"
#include "MainFrm.h"

#include "../Capture/CaptureHUB.h"
#include "../Capture//D3D11RenderSystem.hpp"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CCapturePlayApp

BEGIN_MESSAGE_MAP(CCapturePlayApp, CWinApp)
	ON_COMMAND(ID_APP_ABOUT, &CCapturePlayApp::OnAppAbout)
END_MESSAGE_MAP()


// CCapturePlayApp ����

CCapturePlayApp::CCapturePlayApp()
{
	// TODO: ��Ψһ�� ID �ִ�ȡ������đ��ó�ʽ ID �ִ�; ���h��
	// �ִ���ʽ�� CompanyName.ProductName.SubProduct.VersionInformation
	SetAppID(_T("CapturePlay.AppID.NoVersion"));

	// TODO: �ڴ˼��뽨����ʽ�a��
	// ��������Ҫ�ĳ�ʼ�O������ InitInstance ��
}

// �H�е�һ�� CCapturePlayApp ���

CCapturePlayApp theApp;


// CCapturePlayApp ��ʼ�O��

BOOL CCapturePlayApp::InitInstance()
{
	CWinApp::InitInstance();


	EnableTaskbarInteraction(FALSE);

	// ��Ҫ�� AfxInitRichEdit2() ����ʹ�� RichEdit �����	
	// AfxInitRichEdit2();

	// �˜ʳ�ʼ�O��
	// �������ʹ���@Щ���܁K����p��
	// ������ɵĿɈ��Йn��С��������
	// �����г�ʽ�a�Ƴ�����Ҫ�ĳ�ʼ����ʽ��
	// ׃�������O��ֵ�ĵ�䛙C�a
	// TODO: ����ԓ�m���޸Ĵ��ִ�
	// (���磬��˾���Q��M�����Q)
	SetRegistryKey(_T("���C AppWizard ���a���đ��ó�ʽ"));


	// ��Ҫ������ҕ�����˳�ʽ�a�����µĿ��ҕ��������Ҍ����O����
	// ���ó�ʽ����ҕ�����
	CMainFrame* pFrame = new CMainFrame;
	if (!pFrame)
		return FALSE;
	m_pMainWnd = pFrame;
	// ʹ�����YԴ�����K�d����
	pFrame->LoadFrame(IDR_MAINFRAME,
		WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, NULL,
		NULL);


	h3d::LoadPlugin();
	h3d::GetEngine().Construct(AfxGetMainWnd()->GetSafeHwnd());

	// �H��ʼ��һ��ҕ���������@ʾ�K����ԓҕ��
	pFrame->ShowWindow(SW_SHOW);
	pFrame->UpdateWindow();
	return TRUE;
}

int CCapturePlayApp::ExitInstance()
{
	h3d::GetEngine().Destroy();
	h3d::UnLoadPlugin();


	// TODO: ̎���������ѽ�������YԴ
	return CWinApp::ExitInstance();
}

// CCapturePlayApp ӍϢ̎��ʽ


// �� App About ʹ�� CAboutDlg ��Ԓ���K

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// ��Ԓ���K�Y��
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧Ԯ

// ��ʽ�a����
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

// ���Ќ�Ԓ���K�đ��ó�ʽ����
void CCapturePlayApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}


// CCapturePlayApp ӍϢ̎��ʽ



