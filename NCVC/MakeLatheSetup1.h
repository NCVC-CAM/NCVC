// MakeLatheSetup1.h : ヘッダー ファイル
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMakeLatheSetup1 ダイアログ

class CMakeLatheSetup1 : public CPropertyPage
{
public:
	CMakeLatheSetup1();

// ダイアログ データ
	enum { IDD = IDD_MKLA_SETUP1 };
	CString m_strCustom;
	CIntEdit m_nSpindle;
	CIntEdit m_nMargin;
	CFloatEdit m_dFeed;
	CFloatEdit m_dXFeed;
	CFloatEdit m_dCut;
	CFloatEdit m_dPullZ;
	CFloatEdit m_dPullX;
	CFloatEdit m_dMargin;

protected:
	virtual BOOL OnApply();
	virtual BOOL OnKillActive();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();
	afx_msg void OnCopyFromIn();

	DECLARE_MESSAGE_MAP()
};
