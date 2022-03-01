// MakeNCSetup.h : ヘッダー ファイル
//

#pragma once

#include "MakeNCSetup1.h"
#include "MakeNCSetup2.h"
#include "MakeNCSetup3.h"
#include "MakeNCSetup4.h"
#include "MakeNCSetup5.h"
#include "MakeNCSetup6.h"
#include "MakeNCSetup8.h"
#include "NCMakeMillOpt.h"

/////////////////////////////////////////////////////////////////////////////
// CMakeNCSetup

class CMakeNCSetup : public CPropertySheet
{
	CNCMakeMillOpt*		m_pNCMake;		// NC生成ｵﾌﾟｼｮﾝ

// コンストラクション
public:
	CMakeNCSetup(LPCTSTR, LPCTSTR);
	~CMakeNCSetup();
	DECLARE_DYNAMIC(CMakeNCSetup)

// アトリビュート
public:
	// 内部ﾍﾟｰｼﾞﾀﾞｲｱﾛｸﾞ
	CMakeNCSetup1	m_dlg1;
	CMakeNCSetup2	m_dlg2;
	CMakeNCSetup3	m_dlg3;
	CMakeNCSetup4	m_dlg4;
	CMakeNCSetup5	m_dlg5;
	CMakeNCSetup6	m_dlg6;
	CMakeNCSetup8	m_dlg8;

	CNCMakeMillOpt*	GetNCMakeOption(void) {
		return m_pNCMake;
	}

// オペレーション
public:

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CMakeNCSetup)
	public:
	virtual BOOL OnInitDialog();
	//}}AFX_VIRTUAL

// インプリメンテーション
public:

	// 生成されたメッセージ マップ関数
protected:
	//{{AFX_MSG(CMakeNCSetup)
	afx_msg void OnDestroy();
	//}}AFX_MSG
	afx_msg void OnApplyNow();

	DECLARE_MESSAGE_MAP()
};
