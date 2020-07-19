// MKNCSetup.h : ヘッダー ファイル
//

#pragma once

#include "MKLASetup0.h"
#include "MKLASetup1.h"
#include "MKLASetup2.h"
#include "MKLASetup3.h"
#include "MKLASetup4.h"
#include "MKNCSetup2.h"
#include "MKNCSetup6.h"
#include "NCMakeLatheOpt.h"

/////////////////////////////////////////////////////////////////////////////
// CMKLASetup

class CMKLASetup : public CPropertySheet
{
	CNCMakeLatheOpt*	m_pNCMake;		// NC生成ｵﾌﾟｼｮﾝ

public:
	CMKLASetup(LPCTSTR, LPCTSTR);
	virtual ~CMKLASetup();
	DECLARE_DYNAMIC(CMKLASetup)

	// 内部ﾍﾟｰｼﾞﾀﾞｲｱﾛｸﾞ
	CMKLASetup0	m_dlg0;		// 基本
	CMKNCSetup2	m_dlg1;		// 生成
	CMKNCSetup6	m_dlg2;		// 表記
	CMKLASetup4	m_dlg3;		// 端面
	CMKLASetup2	m_dlg4;		// 下穴
	CMKLASetup3	m_dlg5;		// 内径
	CMKLASetup1	m_dlg6;		// 外径

	CNCMakeLatheOpt*	GetNCMakeOption(void) {
		return m_pNCMake;
	}

	CMKLASetup3*	GetInsideDialog(void) {
		return &m_dlg5;
	}
	CMKLASetup1*	GetOutsideDialog(void) {
		return &m_dlg6;
	}

protected:
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnApplyNow();

	DECLARE_MESSAGE_MAP()
};
