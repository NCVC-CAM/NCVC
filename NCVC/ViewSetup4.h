// ViewSetup4.h : ヘッダー ファイル
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CViewSetup4 ダイアログ

class CViewSetup4 : public CPropertyPage
{
	LOGFONT		m_lfFont;
	COLORREF	m_colView[3];
	CBrush		m_brColor[3];

// コンストラクション
public:
	CViewSetup4();
	~CViewSetup4();

// ダイアログ データ
	//{{AFX_DATA(CViewSetup4)
	enum { IDD = IDD_VIEW_SETUP4 };
	CButton	m_ctFontButton;
	//}}AFX_DATA
	CStatic		m_ctColor[3];
	CIntEdit	m_nTraceSpeed[3];
	BOOL		m_bTraceMarker;

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。

	//{{AFX_VIRTUAL(CViewSetup4)
	public:
	virtual BOOL OnApply();
	virtual BOOL OnKillActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:
	// 生成されたメッセージ マップ関数
	//{{AFX_MSG(CViewSetup4)
	virtual BOOL OnInitDialog();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnFontChange();
	afx_msg void OnColorButton();
	afx_msg void OnDefColor();
	afx_msg void OnChange();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
