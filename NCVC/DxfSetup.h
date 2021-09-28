// DxfSetup.h : ヘッダー ファイル
//

#pragma once

#include "DxfSetup1.h"
#include "DxfSetup2.h"
#include "DxfSetup3.h"

/////////////////////////////////////////////////////////////////////////////
// CDxfSetup

class CDxfSetup : public CPropertySheet
{
// コンストラクション
public:
	CDxfSetup(UINT);

// アトリビュート
public:
	// 内部ﾍﾟｰｼﾞﾀﾞｲｱﾛｸﾞ
	CDxfSetup1	m_dlg1;
	CDxfSetup2	m_dlg2;
	CDxfSetup3	m_dlg3;

// オペレーション
public:
	BOOL	OnReload(CPropertyPage*);

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CDxfSetup)
	//}}AFX_VIRTUAL

// インプリメンテーション
public:

	// 生成されたメッセージ マップ関数
protected:
	//{{AFX_MSG(CDxfSetup)
	afx_msg void OnDestroy();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
