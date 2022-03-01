// MakeNCSetup1.h : ヘッダー ファイル
//

#pragma once

#include "NCVCdefine.h"

/////////////////////////////////////////////////////////////////////////////
// CMakeNCSetup1 ダイアログ

class CMakeNCSetup1 : public CPropertyPage
{
	// ｽﾀﾃｨｯｸｺﾝﾄﾛｰﾙに表示する前の省略形文字列
	CString		m_strHeaderPath,	// 本物のﾊﾟｽ名
				m_strFooterPath;

// コンストラクション
public:
	CMakeNCSetup1();

// ダイアログ データ
	//{{AFX_DATA(CMakeNCSetup1)
	enum { IDD = IDD_MKNC_SETUP1 };
	CButton	m_ctHeaderBt;
	CButton	m_ctFooterBt;
	CEdit	m_ctHeader;
	CEdit	m_ctFooter;
	CFloatEdit	m_dZCut;
	CFloatEdit	m_dZG0Stop;
	CFloatEdit	m_dZFeed;
	CFloatEdit	m_dFeed;
	CIntEdit	m_nSpindle;
	CString	m_strFooter;
	CString	m_strHeader;
	BOOL	m_bXrev;
	BOOL	m_bYrev;
	//}}AFX_DATA
	CFloatEdit	m_dG92[NCXYZ];

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。

	//{{AFX_VIRTUAL(CMakeNCSetup1)
	public:
	virtual BOOL OnApply();
	virtual BOOL OnKillActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:
	// 生成されたメッセージ マップ関数
	//{{AFX_MSG(CMakeNCSetup1)
	virtual BOOL OnInitDialog();
	afx_msg void OnHeaderLoopup();
	afx_msg void OnFooterLoopup();
	afx_msg void OnHeaderEdit();
	afx_msg void OnFooterEdit();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
