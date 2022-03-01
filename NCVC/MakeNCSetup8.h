// MakeNCSetup8.h : ヘッダー ファイル
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMakeNCSetup8 ダイアログ

class CMakeNCSetup8 : public CPropertyPage
{
// コンストラクション
public:
	CMakeNCSetup8();

// ダイアログ データ
	//{{AFX_DATA(CMakeNCSetup8)
	enum { IDD = IDD_MKNC_SETUP8 };
	BOOL	m_bLayerComment;
	BOOL	m_bL0Cycle;
	int		m_nMoveZ;
	CString	m_strCustMoveB;
	CString	m_strCustMoveA;
	//}}AFX_DATA

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。

	//{{AFX_VIRTUAL(CMakeNCSetup8)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:
	// 生成されたメッセージ マップ関数
	//{{AFX_MSG(CMakeNCSetup8)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
