// MakeNCSetup5.h : ヘッダー ファイル
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMakeNCSetup5 ダイアログ

class CMakeNCSetup5 : public CPropertyPage
{
	// ｽﾀﾃｨｯｸｺﾝﾄﾛｰﾙに表示する前の省略形文字列
	CString		m_strScriptPath;	// 本物のﾊﾟｽ名

	void	EnableControl_Drill(void);

// コンストラクション
public:
	CMakeNCSetup5();

// ダイアログ データ
	//{{AFX_DATA(CMakeNCSetup5)
	enum { IDD = IDD_MKNC_SETUP5 };
	CFloatEdit	m_dTolerance;
	CFloatEdit	m_dDrillMargin;
	CFloatEdit	m_dZApproach;
	CIntEdit	m_nZAppDwell;
	CEdit	m_ctScript;
	int		m_nOptimaizeDrill;
	int		m_nTolerance;
	CString	m_strScript;
	//}}AFX_DATA

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。

	//{{AFX_VIRTUAL(CMakeNCSetup5)
	public:
	virtual BOOL OnApply();
	virtual BOOL OnKillActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:
	// 生成されたメッセージ マップ関数
	//{{AFX_MSG(CMakeNCSetup5)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeDrill();
	afx_msg void OnScriptLookup();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
