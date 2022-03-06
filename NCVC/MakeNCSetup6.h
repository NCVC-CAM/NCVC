// MakeNCSetup6.h : ヘッダー ファイル
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMakeNCSetup6 ダイアログ

class CMakeNCSetup6 : public CPropertyPage
{
// コンストラクション
public:
	CMakeNCSetup6();

// ダイアログ データ
	//{{AFX_DATA(CMakeNCSetup6)
	enum { IDD = IDD_MKNC_SETUP6 };
	CStatic m_ctCircleGroup;
	CButton m_ctCircleR;
	CButton m_ctCircleIJ;
	CButton	m_ctCircleHalf;
	CButton	m_ctZeroCutIJ;
	CFloatEdit	m_dEllipse;
	int		m_nDot;
	int		m_nFDot;
	int		m_nCircleCode;
	int		m_nIJ;
	BOOL	m_bZeroCut;
	BOOL	m_bCircleHalf;
	BOOL	m_bZeroCutIJ;
	BOOL	m_bEllipse;
	//}}AFX_DATA

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。

	//{{AFX_VIRTUAL(CMakeNCSetup6)
public:
	virtual BOOL OnApply();
	virtual BOOL OnKillActive();
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:
	// 生成されたメッセージ マップ関数
	//{{AFX_MSG(CMakeNCSetup6)
	virtual BOOL OnInitDialog();
	afx_msg void OnCircleR();
	afx_msg void OnCircleIJ();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
