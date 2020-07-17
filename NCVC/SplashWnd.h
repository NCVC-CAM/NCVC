// SplashWnd.h : ヘッダー ファイル
//
#pragma once

/////////////////////////////////////////////////////////////////////////////
// CSplashWnd ウィンドウ

class CSplashWnd : public CWnd
{
	CBitmap	m_bitmap;
	BITMAP	m_bm;

// コンストラクション
public:
	CSplashWnd();

// アトリビュート
public:
	static	BOOL	ms_bShowSplashWnd;

// オペレーション
public:

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。

	//{{AFX_VIRTUAL(CSplashWnd)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void PostNcDestroy();
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// インプリメンテーション
public:

	// 生成されたメッセージ マップ関数
protected:
	//{{AFX_MSG(CSplashWnd)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPaint();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
