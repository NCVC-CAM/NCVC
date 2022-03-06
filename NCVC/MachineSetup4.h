// MachineSetup4.h : ヘッダー ファイル
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMachineSetup4 ダイアログ

class CMachineSetup4 : public CPropertyPage
{
// コンストラクション
public:
	CMachineSetup4();

// ダイアログ データ
	//{{AFX_DATA(CMachineSetup4)
	enum { IDD = IDD_MC_SETUP4 };
	//}}AFX_DATA
	CString	m_strMacro[MCMACROSTRING];	// 呼び出しｺｰﾄﾞ，ﾌｫﾙﾀﾞ，I/F，出力ﾌｫﾙﾀﾞ，出力名
	CEdit	m_ctMacro[MCMACROSTRING];

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。

	//{{AFX_VIRTUAL(CMachineSetup4)
	public:
	virtual BOOL OnApply();
	virtual BOOL OnKillActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:
	// 生成されたメッセージ マップ関数
	//{{AFX_MSG(CMachineSetup4)
	afx_msg void OnFolder();
	afx_msg void OnMacroIF();
	afx_msg void OnKillFocusMacroCode();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
