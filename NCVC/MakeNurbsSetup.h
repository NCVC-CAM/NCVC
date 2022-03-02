// MakeNurbsSetup.h : ヘッダー ファイル
//

#pragma once

#include "MakeNCSetup1.h"
#include "MakeNCSetup2.h"
#include "MakeNCSetup6.h"
#include "NCMakeMillOpt.h"

/////////////////////////////////////////////////////////////////////////////
// CMakeNurbsSetup

class CMakeNurbsSetup : public CPropertySheet
{
	CNCMakeMillOpt*		m_pNCMake;		// NC生成ｵﾌﾟｼｮﾝ

// コンストラクション
public:
	CMakeNurbsSetup(LPCTSTR, LPCTSTR);
	virtual ~CMakeNurbsSetup();
	DECLARE_DYNAMIC(CMakeNurbsSetup)	// RUNTIME_CLASSマクロ用

// アトリビュート
public:
	// 内部ﾍﾟｰｼﾞﾀﾞｲｱﾛｸﾞ
	CMakeNCSetup1	m_dlg1;
	CMakeNCSetup2	m_dlg2;
	CMakeNCSetup6	m_dlg6;

	CNCMakeMillOpt*	GetNCMakeOption(void) {
		return m_pNCMake;
	}

// オペレーション
public:

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CMakeNurbsSetup)
	public:
	virtual BOOL OnInitDialog();
	//}}AFX_VIRTUAL

// インプリメンテーション
public:

	// 生成されたメッセージ マップ関数
protected:
	//{{AFX_MSG(CMakeNurbsSetup)
	afx_msg void OnDestroy();
	//}}AFX_MSG
	afx_msg void OnApplyNow();

	DECLARE_MESSAGE_MAP()
};
