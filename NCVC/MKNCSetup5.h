// MKNCSetup5.h : ヘッダー ファイル
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMKNCSetup5 ダイアログ

class CMKNCSetup5 : public CPropertyPage
{
	void	EnableControl_Drill(void);

// コンストラクション
public:
	CMKNCSetup5();
	~CMKNCSetup5();

// ダイアログ データ
	//{{AFX_DATA(CMKNCSetup5)
	enum { IDD = IDD_MKNC_SETUP5 };
	CFloatEdit	m_dTolerance;
	CFloatEdit	m_dDrillMargin;
	int		m_nOptimaizeDrill;
	int		m_nTolerance;
	//}}AFX_DATA

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。

	//{{AFX_VIRTUAL(CMKNCSetup5)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:
	// 生成されたメッセージ マップ関数
	//{{AFX_MSG(CMKNCSetup5)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeDrill();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
