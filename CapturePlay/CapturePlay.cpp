
// CapturePlay.cpp : 定義應用程式的類別行為。
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


// CCapturePlayApp 建構

CCapturePlayApp::CCapturePlayApp()
{
	// TODO: 以唯一的 ID 字串取代下面的應用程式 ID 字串; 建議的
	// 字串格式為 CompanyName.ProductName.SubProduct.VersionInformation
	SetAppID(_T("CapturePlay.AppID.NoVersion"));

	// TODO: 在此加入建構程式碼，
	// 將所有重要的初始設定加入 InitInstance 中
}

// 僅有的一個 CCapturePlayApp 物件

CCapturePlayApp theApp;


// CCapturePlayApp 初始設定

BOOL CCapturePlayApp::InitInstance()
{
	CWinApp::InitInstance();


	EnableTaskbarInteraction(FALSE);

	// 需要有 AfxInitRichEdit2() 才能使用 RichEdit 控制項	
	// AfxInitRichEdit2();

	// 標準初始設定
	// 如果您不使用這些功能並且想減少
	// 最後完成的可執行檔大小，您可以
	// 從下列程式碼移除不需要的初始化常式，
	// 變更儲存設定值的登錄機碼
	// TODO: 您應該適度修改此字串
	// (例如，公司名稱或組織名稱)
	SetRegistryKey(_T("本機 AppWizard 所產生的應用程式"));


	// 若要建立主視窗，此程式碼建立新的框架視窗物件，且將其設定為
	// 應用程式的主視窗物件
	CMainFrame* pFrame = new CMainFrame;
	if (!pFrame)
		return FALSE;
	m_pMainWnd = pFrame;
	// 使用其資源建立並載入框架
	pFrame->LoadFrame(IDR_MAINFRAME,
		WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, NULL,
		NULL);


	h3d::LoadPlugin();
	h3d::GetEngine().Construct(AfxGetMainWnd()->GetSafeHwnd());

	// 僅初始化一個視窗，所以顯示並更新該視窗
	pFrame->ShowWindow(SW_SHOW);
	pFrame->UpdateWindow();
	return TRUE;
}

int CCapturePlayApp::ExitInstance()
{
	h3d::GetEngine().Destroy();
	h3d::UnLoadPlugin();


	// TODO: 處理其他您已經加入的資源
	return CWinApp::ExitInstance();
}

// CCapturePlayApp 訊息處理常式


// 對 App About 使用 CAboutDlg 對話方塊

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 對話方塊資料
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支援

// 程式碼實作
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

// 執行對話方塊的應用程式命令
void CCapturePlayApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}


// CCapturePlayApp 訊息處理常式



