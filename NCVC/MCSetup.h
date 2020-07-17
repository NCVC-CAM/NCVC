// MCSetup.h : ヘッダー ファイル
//

#pragma once

#include "MCSetup1.h"
#include "MCSetup2.h"
#include "MCSetup3.h"
#include "MCSetup4.h"
#include "MCSetup5.h"

/////////////////////////////////////////////////////////////////////////////
// CMCSetup

class CMCSetup : public CPropertySheet
{
// コンストラクション
public:
	CMCSetup(LPCTSTR, LPCTSTR);

// アトリビュート
public:
	CString		m_strFileName;	// 現在処理中の機械情報ﾌｧｲﾙ名
	BOOL		m_bReload,		// 再読込が必要
				m_bCalcThread;	// 再計算が必要

	// 内部ﾍﾟｰｼﾞﾀﾞｲｱﾛｸﾞ
	CMCSetup1	m_dlg1;
	CMCSetup2	m_dlg2;
	CMCSetup3	m_dlg3;
	CMCSetup4	m_dlg4;
	CMCSetup5	m_dlg5;

// オペレーション
public:

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CMCSetup)
	public:
	virtual BOOL OnInitDialog();
	//}}AFX_VIRTUAL

// インプリメンテーション
public:
	virtual ~CMCSetup();

	// 生成されたメッセージ マップ関数
protected:
	//{{AFX_MSG(CMCSetup)
	afx_msg void OnDestroy();
	//}}AFX_MSG
	afx_msg void OnApplyNow();

	DECLARE_MESSAGE_MAP()
};
