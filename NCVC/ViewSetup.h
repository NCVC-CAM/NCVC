// ViewSetup.h : ヘッダー ファイル
//

#pragma once

#include "ViewSetup1.h"
#include "ViewSetup2.h"
#include "ViewSetup3.h"
#include "ViewSetup4.h"

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

// オペレーション
public:
	CString	GetChangeFontButtonText(LOGFONT*);

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CViewSetup)
	//}}AFX_VIRTUAL

// インプリメンテーション
public:
	virtual ~CViewSetup();

	// 生成されたメッセージ マップ関数
protected:
	//{{AFX_MSG(CViewSetup)
	afx_msg void OnDestroy();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
