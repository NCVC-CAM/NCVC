// ViewSetup5.h : ヘッダー ファイル
//

#pragma once
#include "afxwin.h"

/////////////////////////////////////////////////////////////////////////////
// CViewSetup5 ダイアログ

class CViewSetup5 : public CPropertyPage
{
	void	EnableControl(void);

// コンストラクション
public:
	CViewSetup5();
	virtual ~CViewSetup5();

// ダイアログ データ
	enum { IDD = IDD_VIEW_SETUP5 };
	BOOL	m_bSolid,
			m_bG00View,
			m_bDrag,
			m_bMillT,
			m_bMillC;
	int		m_nMillType;
	CButton m_ctG00View,
			m_ctDrag;
	CFloatEdit	m_dEndmill;

// オーバーライド
public:
	virtual BOOL OnKillActive();
	virtual BOOL OnApply();
protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート

// インプリメンテーション
protected:
	// 生成されたメッセージ マップ関数
	afx_msg void OnSolidClick();
	afx_msg void OnChange();

	DECLARE_MESSAGE_MAP()
};
