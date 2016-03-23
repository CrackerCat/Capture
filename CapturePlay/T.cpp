// T.cpp : implementation file
//

#include "stdafx.h"
#include "CapturePlay.h"
#include "T.h"
#include "afxdialogex.h"


// T dialog

IMPLEMENT_DYNAMIC(T, CDialog)

T::T(std::vector<WindowData>& wins,const wchar_t* str_caption,CWnd* pParent /*=NULL*/)
	:ref(wins), CDialog(IDD_T, pParent),caption(str_caption), ref_index(LB_ERR)
{
}

T::~T()
{

	//如何获得这个玩意？
}

BOOL T::OnInitDialog()
{
	CDialog::OnInitDialog();
	CListBox* box =static_cast<CListBox*>(GetDlgItem(IDC_LIST1));

	if (caption)
		SetWindowTextW(caption);

	int i = 0;
	for (auto & win : ref)
		box->InsertString(i++, win.sName.c_str());
	return 0;
}

void T::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(T, CDialog)
	ON_BN_CLICKED(IDOK, &T::OnBnClickedOk)
END_MESSAGE_MAP()


// T message handlers





void T::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	CListBox* box = static_cast<CListBox*>(GetDlgItem(IDC_LIST1));

	ref_index = box->GetCurSel();
	CDialog::OnOK();
}
