// ViewSetup3.h : ヘッダー ファイル
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CViewSetup3 ダイアログ

class CViewSetup3 : public CPropertyPage
{
	LOGFONT		m_lfFont;
	COLORREF	m_colView[8];
	CBrush		m_brColor[8];

// コンストラクション
public:
	CViewSetup3();
	~CViewSetup3();

// ダイアログ データ
	//{{AFX_DATA(CViewSetup3)
	enum { IDD = IDD_VIEW_SETUP3 };
	CButton	m_ctFontButton;
	//}}AFX_DATA
	CStatic		m_ctColor[8];
	CComboBox	m_cbLineType[5];

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。

	//{{AFX_VIRTUAL(CViewSetup3)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:
	// 生成されたメッセージ マップ関数
	//{{AFX_MSG(CViewSetup3)
	virtual BOOL OnInitDialog();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnChange();
	afx_msg void OnColorButton();
	afx_msg void OnFontChange();
	afx_msg void OnDefColor();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
