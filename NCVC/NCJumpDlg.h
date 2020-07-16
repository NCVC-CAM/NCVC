// NCJumpDlg.h : ヘッダー ファイル
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CNCJumpDlg ダイアログ

class CNCJumpDlg : public CDialog
{
	void	EnableButton(BOOL bEnable) {
		m_ctOK.EnableWindow(bEnable);
		m_nJump.EnableWindow(bEnable);
	}

// コンストラクション
public:
	CNCJumpDlg(CWnd* pParent = NULL);   // 標準のコンストラクタ

// ダイアログ データ
	//{{AFX_DATA(CNCJumpDlg)
	enum { IDD = IDD_NCVIEW_JUMP };
	CButton	m_ctOK;
	CIntEdit	m_nJump;
	//}}AFX_DATA

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CNCJumpDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:

	// 生成されたメッセージ マップ関数
	//{{AFX_MSG(CNCJumpDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	//}}AFX_MSG
	afx_msg LRESULT OnUserSwitchDocument(WPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()
};
