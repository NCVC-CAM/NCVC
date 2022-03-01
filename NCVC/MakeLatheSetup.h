// MakeLatheSetup.h : ヘッダー ファイル
//

#pragma once

#include "MakeLatheSetup0.h"
#include "MakeLatheSetup1.h"
#include "MakeLatheSetup2.h"
#include "MakeLatheSetup3.h"
#include "MakeLatheSetup4.h"
#include "MakeLatheSetup5.h"
#include "MakeNCSetup2.h"
#include "MakeNCSetup6.h"
#include "NCMakeLatheOpt.h"

/////////////////////////////////////////////////////////////////////////////
// CMakeLatheSetup

class CMakeLatheSetup : public CPropertySheet
{
	CNCMakeLatheOpt*	m_pNCMake;		// NC生成ｵﾌﾟｼｮﾝ

public:
	CMakeLatheSetup(LPCTSTR, LPCTSTR);
	virtual ~CMakeLatheSetup();

	// 内部ﾍﾟｰｼﾞﾀﾞｲｱﾛｸﾞ
	CMakeLatheSetup0	m_dlg0;		// 基本
	CMakeNCSetup2		m_dlg1;		// 生成
	CMakeNCSetup6		m_dlg2;		// 表記
	CMakeLatheSetup4	m_dlg3;		// 端面
	CMakeLatheSetup2	m_dlg4;		// 下穴
	CMakeLatheSetup3	m_dlg5;		// 内径
	CMakeLatheSetup1	m_dlg6;		// 外径
	CMakeLatheSetup5	m_dlg7;		// 突切

	CNCMakeLatheOpt*	GetNCMakeOption(void) {
		return m_pNCMake;
	}

	CMakeLatheSetup3*	GetInsideDialog(void) {
		return &m_dlg5;
	}
	CMakeLatheSetup1*	GetOutsideDialog(void) {
		return &m_dlg6;
	}

protected:
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnApplyNow();

	DECLARE_MESSAGE_MAP()
};
