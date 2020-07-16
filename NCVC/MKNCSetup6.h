// MKNCSetup6.h : ヘッダー ファイル
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMKNCSetup6 ダイアログ

class CMKNCSetup6 : public CPropertyPage
{
// コンストラクション
public:
	CMKNCSetup6();
	~CMKNCSetup6();

// ダイアログ データ
	//{{AFX_DATA(CMKNCSetup6)
	enum { IDD = IDD_MKNC_SETUP6 };
	CButton	m_ctCircleHalf;
	int		m_nDot;
	int		m_nFDot;
	BOOL	m_bZeroCut;
	int		m_nCircleCode;
	int		m_nIJ;
	BOOL	m_bCircleHalf;
	CFloatEdit	m_dEllipse;
	BOOL	m_bEllipse;
	//}}AFX_DATA

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。

	//{{AFX_VIRTUAL(CMKNCSetup6)
	public:
	virtual BOOL OnApply();
	virtual BOOL OnKillActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:
	// 生成されたメッセージ マップ関数
	//{{AFX_MSG(CMKNCSetup6)
	virtual BOOL OnInitDialog();
	afx_msg void OnCircleR();
	afx_msg void OnCircleIJ();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
