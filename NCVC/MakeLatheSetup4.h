// MakeLatheSetup4.h : ヘッダー ファイル
//

#pragma once
#include "afxwin.h"

/////////////////////////////////////////////////////////////////////////////
// CMakeLatheSetup1 ダイアログ

class CMakeLatheSetup4 : public CPropertyPage
{
	void	EnableControl(void);

public:
	CMakeLatheSetup4();

	// ダイアログ データ
	enum { IDD = IDD_MKLA_SETUP4 };
	BOOL	m_bEndFace;
	CString m_strCustom;
	CEdit m_ctCustom;
	CIntEdit m_nSpindle;
	CFloatEdit m_dXFeed;
	CFloatEdit m_dCut;
	CFloatEdit m_dStep;
	CFloatEdit m_dPullZ;
	CFloatEdit m_dPullX;

protected:
	virtual BOOL OnApply();
	virtual BOOL OnKillActive();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();
	afx_msg void OnCheck();

	DECLARE_MESSAGE_MAP()
};
