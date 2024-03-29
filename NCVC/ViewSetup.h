// ViewSetup.h : ヘッダー ファイル
//

#pragma once

#include "ViewSetup1.h"
#include "ViewSetup2.h"
#include "ViewSetup3.h"
#include "ViewSetup4.h"
#include "ViewSetup5.h"
#include "ViewSetup6.h"

/////////////////////////////////////////////////////////////////////////////
// CViewSetup

class CViewSetup : public CPropertySheet
{
// コンストラクション
public:
	CViewSetup(void);

// アトリビュート
public:
	// 内部ﾍﾟｰｼﾞﾀﾞｲｱﾛｸﾞ
	CViewSetup1	m_dlg1;
	CViewSetup2	m_dlg2;
	CViewSetup3	m_dlg3;
	CViewSetup4	m_dlg4;
	CViewSetup5	m_dlg5;
	CViewSetup6	m_dlg6;

// オペレーション
public:
	CString	GetChangeFontButtonText(LOGFONT*);

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CViewSetup)
	//}}AFX_VIRTUAL

// インプリメンテーション
public:

	// 生成されたメッセージ マップ関数
protected:
	//{{AFX_MSG(CViewSetup)
	afx_msg void OnDestroy();
	//}}AFX_MSG
	afx_msg void OnApplyNow();

	DECLARE_MESSAGE_MAP()
};
