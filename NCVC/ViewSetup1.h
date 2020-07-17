// ViewSetup1.h : ヘッダー ファイル
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CViewSetup1 ダイアログ

class CViewSetup1 : public CPropertyPage
{
	COLORREF	m_colView[COMCOL_NUMS];
	CBrush		m_brColor[COMCOL_NUMS];

	void	EnableControl(void);

// コンストラクション
public:
	CViewSetup1();
	~CViewSetup1();

// ダイアログ データ
	//{{AFX_DATA(CViewSetup1)
	enum { IDD = IDD_VIEW_SETUP1 };
	BOOL	m_bMouseWheel;
	int		m_nWheelType;
	CButton m_ctMouseWheel[COMCOL_NUMS];
	//}}AFX_DATA
	CStatic		m_ctColor[COMCOL_NUMS];
	CComboBox	m_cbLineType[COMCOL_NUMS];

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。

	//{{AFX_VIRTUAL(CViewSetup1)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:
	// 生成されたメッセージ マップ関数
	//{{AFX_MSG(CViewSetup1)
	virtual BOOL OnInitDialog();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnChange();
	afx_msg void OnColorButton();
	afx_msg void OnWheel();
	afx_msg void OnDefColor();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
