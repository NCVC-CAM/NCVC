// MakeWireSetup1.h : ヘッダー ファイル
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMakeWireSetup1 ダイアログ

class CMakeWireSetup1 : public CPropertyPage
{
	// ｽﾀﾃｨｯｸｺﾝﾄﾛｰﾙに表示する前の省略形文字列
	CString		m_strHeaderPath,	// 本物のﾊﾟｽ名
				m_strFooterPath;

public:
	CMakeWireSetup1();

// ダイアログ データ
	enum { IDD = IDD_MKWI_SETUP1 };
	CButton	m_ctButton2;
	CButton	m_ctButton1;
	CEdit	m_ctHeader;
	CEdit	m_ctFooter;
	CString	m_strTaperMode;
	CString	m_strFooter;
	CString	m_strHeader;
	CFloatEdit m_dDepth;
	CFloatEdit m_dTaper;
	CFloatEdit m_dFeed;
	CFloatEdit m_dG92X;
	CFloatEdit m_dG92Y;

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
