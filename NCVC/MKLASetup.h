// MKNCSetup.h : ヘッダー ファイル
//

#pragma once

#include "MKLASetup1.h"
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
	CMKLASetup1	m_dlg1;
	CMKNCSetup2	m_dlg2;
	CMKNCSetup6	m_dlg3;

	CNCMakeLatheOpt*	GetNCMakeOption(void) {
		return m_pNCMake;
	}

protected:
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnApplyNow();

	DECLARE_MESSAGE_MAP()
};
