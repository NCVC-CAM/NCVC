// MainStatusBar.h : ヘッダー ファイル
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMainStatusBar ウィンドウ

class CMainStatusBar : public CStatusBar
{
	CProgressCtrl	m_ctProgress;

// コンストラクション
public:
	CMainStatusBar();

// アトリビュート
public:
	CProgressCtrl*	GetProgressCtrl(void) {
		return &m_ctProgress;
	}

// オペレーション
public:
	void	EnableProgress(BOOL f = TRUE) {
		m_ctProgress.ShowWindow(f ? SW_SHOWNA : SW_HIDE);
	}
	void	ChangeProgressSize(int, int);	// 親ﾌﾚｰﾑごとにﾌﾟﾛｸﾞﾚｽｺﾝﾄﾛｰﾙのｻｲｽﾞ変更
											// OnSize() の代わり

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CMainStatusBar)
	//}}AFX_VIRTUAL

// インプリメンテーション
public:
	virtual ~CMainStatusBar();

	// 生成されたメッセージ マップ関数
protected:
	//{{AFX_MSG(CMainStatusBar)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	afx_msg LRESULT OnUserProgressPos(WPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()
};
