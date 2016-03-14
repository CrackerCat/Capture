// CameraDialog.cpp : implementation file
//

#include "stdafx.h"
#include "CapturePlay.h"
#include "CameraDialog.h"
#include "afxdialogex.h"

#include "../Capture/CameraCapture.h"


// CameraDialog dialog

IMPLEMENT_DYNAMIC(CameraDialog, CDialog)

CameraDialog::CameraDialog(CWnd* pParent /*=NULL*/)
	: CDialog(IDD_CAMERADIALOG, pParent)
{

}

CameraDialog::~CameraDialog()
{
}

BOOL CameraDialog::OnInitDialog()
{
	CDialog::OnInitDialog();
	CListBox* box = static_cast<CListBox*>(GetDlgItem(IDC_LIST1));

	auto camera_list = h3d::GetCameraInfos();
	int i = 0;
	for (auto & camera : camera_list) {
		box->InsertString(i++, camera.Name.c_str());
	}

	return 0;
}

void CameraDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CameraDialog, CDialog)
	ON_BN_CLICKED(IDOK, &CameraDialog::OnBnClickedOk)
END_MESSAGE_MAP()


// CameraDialog message handlers


void CameraDialog::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	CListBox* box = static_cast<CListBox*>(GetDlgItem(IDC_LIST1));

	camera_index = box->GetCurSel();

	CDialog::OnOK();
}
