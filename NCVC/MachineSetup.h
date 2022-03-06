// MachineSetup.h : ヘッダー ファイル
//

#pragma once

#include "MachineSetup1.h"
#include "MachineSetup2.h"
#include "MachineSetup3.h"
#include "MachineSetup4.h"
#include "MachineSetup5.h"

/////////////////////////////////////////////////////////////////////////////
// CMachineSetup

class CMachineSetup : public CPropertySheet
{
// コンストラクション
public:
	CMachineSetup(LPCTSTR, LPCTSTR);

// アトリビュート
public:
	CString		m_strFileName;	// 現在処理中の機械情報ﾌｧｲﾙ名
	BOOL		m_bReload,		// 再読込が必要
				m_bCalcThread;	// 再計算が必要

	// 内部ﾍﾟｰｼﾞﾀﾞｲｱﾛｸﾞ
	CMachineSetup1	m_dlg1;
	CMachineSetup2	m_dlg2;
	CMachineSetup3	m_dlg3;
	CMachineSetup4	m_dlg4;
	CMachineSetup5	m_dlg5;

// オペレーション
public:

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CMachineSetup)
	public:
	virtual BOOL OnInitDialog();
	//}}AFX_VIRTUAL

// インプリメンテーション
public:

	// 生成されたメッセージ マップ関数
protected:
	//{{AFX_MSG(CMachineSetup)
	afx_msg void OnDestroy();
	//}}AFX_MSG
	afx_msg void OnApplyNow();

	DECLARE_MESSAGE_MAP()
};
