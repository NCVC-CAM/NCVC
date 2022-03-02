// MakeWireSetup.h : ヘッダー ファイル
//

#pragma once

#include "MakeWireSetup1.h"
#include "MakeWireSetup2.h"
#include "MakeNCSetup2.h"
#include "MakeNCSetup6.h"
#include "NCMakeWireOpt.h"

/////////////////////////////////////////////////////////////////////////////
// CMakeWireSetu

class CMakeWireSetup : public CPropertySheet
{
	CNCMakeWireOpt*		m_pNCMake;		// NC生成ｵﾌﾟｼｮﾝ

public:
	CMakeWireSetup(LPCTSTR, LPCTSTR);
	virtual ~CMakeWireSetup();
	DECLARE_DYNAMIC(CMakeWireSetup)	// RUNTIME_CLASSマクロ用

	// 内部ﾍﾟｰｼﾞﾀﾞｲｱﾛｸﾞ
	CMakeWireSetup1	m_dlg1;
	CMakeWireSetup2	m_dlg2;
	CMakeNCSetup2	m_dlg3;
	CMakeNCSetup6	m_dlg4;

	CNCMakeWireOpt*		GetNCMakeOption(void) {
		return m_pNCMake;
	}

protected:
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnApplyNow();

	DECLARE_MESSAGE_MAP()
};


