#pragma once


// T dialog
#include <string>
#include <vector>

struct WindowData {
	std::wstring sName;
	HWND hWnd;

	WindowData(const wchar_t* name, HWND hwnd)
		:sName(name), hWnd(hwnd)
	{}
};

class T : public CDialog
{
	DECLARE_DYNAMIC(T)

public:
	T(std::vector<WindowData>&,const wchar_t* str_caption = NULL,CWnd* pParent = NULL);   // standard constructor
	virtual ~T();

	const wchar_t* caption;
	std::vector<WindowData>& ref;
	int ref_index;
// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_T };
#endif
	BOOL OnInitDialog() override;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
};
