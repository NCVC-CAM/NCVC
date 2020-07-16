#include "afxwin.h"
#if !defined(___CMAKENCDWIZ9___)
#define ___CMAKENCDWIZ9___

#pragma once

// CMakeNCDWiz9 ダイアログ

class CMakeNCDWiz9 : public CPropertyPage
{
	// ｽﾀﾃｨｯｸｺﾝﾄﾛｰﾙに表示する前の省略形文字列
	CString			m_strNCPath,	// 本物のﾊﾟｽ名
					m_strInitPath;
public:
	CMakeNCDWiz9();
	virtual ~CMakeNCDWiz9();

// ダイアログ データ
	enum { IDD = IDD_MAKENCD_WIZ9 };

	CEdit m_ctNCFileName;
	CString m_strNCFileName;
	CComboBox m_ctInitFileName;
	CString m_strInitFileName;
	BOOL m_bNCView;
	CButton m_ctNCView;

	virtual BOOL OnInitDialog();
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardBack();
	virtual BOOL OnWizardFinish();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート

	afx_msg void OnMKNCFileUp();
	afx_msg void OnMKNCInitUp();
	afx_msg void OnMKNCInitEdit();
	afx_msg void OnSelChangeInit();
	afx_msg void OnKillFocusNCFile();
	afx_msg void OnKillFocusInit();

	DECLARE_MESSAGE_MAP()
};

#endif
