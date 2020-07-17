// MKNCSetup.h : ヘッダー ファイル
//

#pragma once

#include "MKNCSetup1.h"
#include "MKNCSetup2.h"
#include "MKNCSetup3.h"
#include "MKNCSetup4.h"
#include "MKNCSetup5.h"
#include "MKNCSetup6.h"
#include "MKNCSetup8.h"
#include "NCMakeMillOpt.h"

/////////////////////////////////////////////////////////////////////////////
// CMKNCSetup

class CMKNCSetup : public CPropertySheet
{
	CNCMakeMillOpt*		m_pNCMake;		// NC生成ｵﾌﾟｼｮﾝ

// コンストラクション
public:
	CMKNCSetup(LPCTSTR, LPCTSTR);
	~CMKNCSetup();
	DECLARE_DYNAMIC(CMKNCSetup)

// アトリビュート
public:
	// 内部ﾍﾟｰｼﾞﾀﾞｲｱﾛｸﾞ
	CMKNCSetup1	m_dlg1;
	CMKNCSetup2	m_dlg2;
	CMKNCSetup3	m_dlg3;
	CMKNCSetup4	m_dlg4;
	CMKNCSetup5	m_dlg5;
	CMKNCSetup6	m_dlg6;
	CMKNCSetup8	m_dlg8;

	CNCMakeMillOpt*	GetNCMakeOption(void) {
		return m_pNCMake;
	}

// オペレーション
public:

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CMKNCSetup)
	public:
	virtual BOOL OnInitDialog();
	//}}AFX_VIRTUAL

// インプリメンテーション
public:

	// 生成されたメッセージ マップ関数
protected:
	//{{AFX_MSG(CMKNCSetup)
	afx_msg void OnDestroy();
	//}}AFX_MSG
	afx_msg void OnApplyNow();

	DECLARE_MESSAGE_MAP()
};
