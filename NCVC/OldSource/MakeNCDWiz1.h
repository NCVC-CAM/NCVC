#if !defined(___CMAKENCDWIZ1___)
#define ___CMAKENCDWIZ1___

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMakeNCDWiz1 ダイアログ

class CMakeNCDWiz1 : public CPropertyPage
{
public:
	CMakeNCDWiz1();
	virtual ~CMakeNCDWiz1();

// ダイアログ データ
	enum { IDD = IDD_MAKENCD_WIZ1 };
	int m_nMakeType;

	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardNext();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート

	DECLARE_MESSAGE_MAP()
};

#endif
