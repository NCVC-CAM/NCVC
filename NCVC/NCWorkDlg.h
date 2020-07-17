// NCWorkDlg.h : ヘッダー ファイル
//

#pragma once

#include "NCWorkDlg1.h"
#include "NCWorkDlg2.h"

/////////////////////////////////////////////////////////////////////////////
// CNCWorkDlg

class CNCWorkDlg : public CPropertySheet
{
	DECLARE_DYNAMIC(CNCWorkDlg)

public:
	CNCWorkDlg(UINT nIDCaption, UINT iSelectPage);
	virtual ~CNCWorkDlg();
	virtual BOOL OnInitDialog();

	// 内部ﾍﾟｰｼﾞﾀﾞｲｱﾛｸﾞ
	CNCWorkDlg1	m_dlg1;
	CNCWorkDlg2	m_dlg2;

	CNCDoc*	GetNCDocument(void);

protected:
	virtual void PostNcDestroy();

	afx_msg void OnDestroy();
	afx_msg LRESULT OnUserSwitchDocument(WPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()
};


