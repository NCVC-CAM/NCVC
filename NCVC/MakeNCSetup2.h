// MakeNCSetup2.h : ヘッダー ファイル
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMakeNCSetup2 ダイアログ

class CMakeNCSetup2 : public CPropertyPage
{
	void	EnableControl_ProgNo(void);
	void	EnableControl_LineAdd(void);

// コンストラクション
public:
	CMakeNCSetup2();

// ダイアログ データ
	//{{AFX_DATA(CMakeNCSetup2)
	enum { IDD = IDD_MKNC_SETUP2 };
	CButton		m_ctProgAuto;
	CButton		m_ctDisableSpindle;
	CStatic		m_ctZReturnS;
	CComboBox	m_ctLineAdd;
	CComboBox	m_ctZReturn;
	CEdit		m_ctLineForm;
	CIntEdit	m_nProg;
	BOOL	m_bProg;
	BOOL	m_bProgAuto;
	BOOL	m_bLineAdd;
	BOOL	m_bGclip;
	BOOL	m_bDisableSpindle;
	int		m_nLineAdd;
	int		m_nG90;
	int		m_nZReturn;
	CString	m_strLineForm;
	CString	m_strEOB;
	//}}AFX_DATA

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。

	//{{AFX_VIRTUAL(CMakeNCSetup2)
public:
	virtual BOOL OnApply();
	virtual BOOL OnKillActive();
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:
	// 生成されたメッセージ マップ関数
	//{{AFX_MSG(CMakeNCSetup2)
	virtual BOOL OnInitDialog();
	afx_msg void OnProgNo();
	afx_msg void OnLineAdd();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
