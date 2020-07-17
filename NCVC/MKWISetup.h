// MKNCSetup.h : ヘッダー ファイル
//

#pragma once

#include "MKWISetup1.h"
#include "MKWISetup2.h"
#include "MKNCSetup2.h"
#include "MKNCSetup6.h"
#include "NCMakeWireOpt.h"

/////////////////////////////////////////////////////////////////////////////
// CMKWISetup

class CMKWISetup : public CPropertySheet
{
	CNCMakeWireOpt*		m_pNCMake;		// NC生成ｵﾌﾟｼｮﾝ

public:
	CMKWISetup(LPCTSTR, LPCTSTR);
	virtual ~CMKWISetup();
	DECLARE_DYNAMIC(CMKWISetup)

	// 内部ﾍﾟｰｼﾞﾀﾞｲｱﾛｸﾞ
	CMKWISetup1	m_dlg1;
	CMKWISetup2	m_dlg2;
	CMKNCSetup2	m_dlg3;
	CMKNCSetup6	m_dlg4;

	CNCMakeWireOpt*		GetNCMakeOption(void) {
		return m_pNCMake;
	}

protected:
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnApplyNow();

	DECLARE_MESSAGE_MAP()
};


