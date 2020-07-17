// MKLASetup1.h : ヘッダー ファイル
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMKLASetup1 ダイアログ

class CMKLASetup1 : public CPropertyPage
{
public:
	CMKLASetup1();
	virtual ~CMKLASetup1();

// ダイアログ データ
	enum { IDD = IDD_MKLA_SETUP1 };
	CButton	m_ctButton2;
	CButton	m_ctButton1;
	CEdit	m_ctHeader;
	CEdit	m_ctFooter;
	CIntEdit m_nSpindle;
	CIntEdit m_nMargin;
	CFloatEdit m_dFeed;
	CFloatEdit m_dXFeed;
	CFloatEdit m_dCut;
	CFloatEdit m_dPullZ;
	CFloatEdit m_dPullX;
	CFloatEdit m_dMargin;
	CString	m_strFooter;
	CString	m_strHeader;

protected:
	virtual BOOL OnApply();
	virtual BOOL OnKillActive();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();
	afx_msg void OnHeaderLoopup();
	afx_msg void OnFooterLoopup();
	afx_msg void OnHeaderEdit();
	afx_msg void OnFooterEdit();

	DECLARE_MESSAGE_MAP()
};
