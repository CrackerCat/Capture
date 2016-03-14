#pragma once


// CameraDialog dialog

class CameraDialog : public CDialog
{
	DECLARE_DYNAMIC(CameraDialog)

public:
	CameraDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CameraDialog();

	int camera_index;
// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CAMERADIALOG };
#endif
	BOOL OnInitDialog() override;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
};
