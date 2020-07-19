// MKLASetup3.h : ヘッダー ファイル
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMKLASetup3 ダイアログ

class CMKLASetup3 : public CPropertyPage
{
public:
	CMKLASetup3();

// ダイアログ データ
	enum { IDD = IDD_MKLA_SETUP3 };
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
	afx_msg void OnCopyFromOut();

	DECLARE_MESSAGE_MAP()
};
