#if !defined(___CMAKENCDWIZ3___)
#define ___CMAKENCDWIZ3___

#pragma once

// CMakeNCDWiz3 ダイアログ

class CMakeNCDWiz3 : public CPropertyPage
{
public:
	CMakeNCDWiz3();
	virtual ~CMakeNCDWiz3();

// ダイアログ データ
	enum { IDD = IDD_MAKENCD_WIZ3 };
	CFloatEdit m_dTool;
	CFloatEdit m_dRemain;

	virtual BOOL OnInitDialog();
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardNext();
	virtual LRESULT OnWizardBack();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート

	DECLARE_MESSAGE_MAP()
};

#endif
