// MKLASetup2.h : ヘッダー ファイル
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMKLASetup2 ダイアログ

class CMKLASetup2 : public CPropertyPage
{
	void	EnableControl_Cycle(void);

public:
	CMKLASetup2();

// ダイアログ データ
	enum { IDD = IDD_MKLA_SETUP2 };
	BOOL m_bCycle;
	CString m_strDrill;
	CString m_strSpindle;
	CString m_strFeed;
	CString m_strCustom;
	CFloatEdit m_dDrillZ;
	CFloatEdit m_dDrillR;
	CFloatEdit m_dDrillQ;
	CFloatEdit m_dDrillD;
	CFloatEdit m_dHole;
	CIntEdit m_nDwell;
	CStatic	m_ctDrillDtext;

protected:
	virtual BOOL OnApply();
	virtual BOOL OnKillActive();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();
	afx_msg void OnCycle();

	DECLARE_MESSAGE_MAP()
};
