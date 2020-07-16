// ToolBarSetupDlg.h : ヘッダー ファイル
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CToolBarSetupDlg ダイアログ

class CToolBarSetupDlg : public CDialog
{
// コンストラクション
public:
	CToolBarSetupDlg(DWORD);

// ダイアログ データ
	//{{AFX_DATA(CToolBarSetupDlg)
	enum { IDD = IDD_TOOLBAR };
	//}}AFX_DATA

	// 順序は MainFrm.cpp の g_tbInfo による
	BOOL	m_bToolBar[7];	// Main, Trace, MakeNCD, Shape, Addin, Exec, Machine

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CToolBarSetupDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:

	// 生成されたメッセージ マップ関数
	//{{AFX_MSG(CToolBarSetupDlg)
	afx_msg void OnToolBarCustomize();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
