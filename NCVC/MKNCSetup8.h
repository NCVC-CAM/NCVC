// MKNCSetup8.h : ヘッダー ファイル
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMKNCSetup8 ダイアログ

class CMKNCSetup8 : public CPropertyPage
{
// コンストラクション
public:
	CMKNCSetup8();
	~CMKNCSetup8();

// ダイアログ データ
	//{{AFX_DATA(CMKNCSetup8)
	enum { IDD = IDD_MKNC_SETUP8 };
	BOOL	m_bLayerComment;
	BOOL	m_bL0Cycle;
	int		m_nMoveZ;
	CString	m_strCustMoveB;
	CString	m_strCustMoveA;
	//}}AFX_DATA

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。

	//{{AFX_VIRTUAL(CMKNCSetup8)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:
	// 生成されたメッセージ マップ関数
	//{{AFX_MSG(CMKNCSetup8)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
