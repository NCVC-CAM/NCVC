// MCSetup4.h : ヘッダー ファイル
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMCSetup4 ダイアログ

class CMCSetup4 : public CPropertyPage
{
// コンストラクション
public:
	CMCSetup4();
	~CMCSetup4();

// ダイアログ データ
	//{{AFX_DATA(CMCSetup4)
	enum { IDD = IDD_MC_SETUP4 };
	//}}AFX_DATA
	CString	m_strMacro[MCMACROSTRING];	// 呼び出しｺｰﾄﾞ，ﾌｫﾙﾀﾞ，I/F，出力ﾌｫﾙﾀﾞ，出力名
	CEdit	m_ctMacro[MCMACROSTRING];

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。

	//{{AFX_VIRTUAL(CMCSetup4)
	public:
	virtual BOOL OnApply();
	virtual BOOL OnKillActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:
	// 生成されたメッセージ マップ関数
	//{{AFX_MSG(CMCSetup4)
	afx_msg void OnFolder();
	afx_msg void OnMacroIF();
	afx_msg void OnKillFocusMacroCode();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
