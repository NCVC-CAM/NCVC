// ViewSetup2.h : ヘッダー ファイル
//

#pragma once

#include "NCVCdefine.h"

/////////////////////////////////////////////////////////////////////////////
// CViewSetup2 ダイアログ

class CViewSetup2 : public CPropertyPage
{
	COLORREF	m_colView[14];
	CBrush		m_brColor[14];
	
	void	EnableControl(void);

// コンストラクション
public:
	CViewSetup2();
	~CViewSetup2();

// ダイアログ データ
	//{{AFX_DATA(CViewSetup2)
	enum { IDD = IDD_VIEW_SETUP2 };
	BOOL	m_bDrawCircleCenter;
	CButton	m_ctGuide;
	//}}AFX_DATA
	BOOL		m_bGuide[2];
	CStatic		m_ctColor[14];
	CComboBox	m_cbLineType[9];
	CFloatEdit	m_dGuide[NCXYZ];

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。

	//{{AFX_VIRTUAL(CViewSetup2)
	public:
	virtual BOOL OnApply();
	virtual BOOL OnKillActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:
	// 生成されたメッセージ マップ関数
	//{{AFX_MSG(CViewSetup2)
	virtual BOOL OnInitDialog();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnChange();
	afx_msg void OnColorButton();
	afx_msg void OnScale();
	afx_msg void OnDefColor();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
