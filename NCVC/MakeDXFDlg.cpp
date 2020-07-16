// MakeDXFDlg.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "NCDoc.h"
#include "MakeDXFDlg.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

BEGIN_MESSAGE_MAP(CMakeDXFDlg, CPropertySheet)
	//{{AFX_MSG_MAP(CMakeDXFDlg)
		// メモ - ClassWizard はこの位置にマッピング用のマクロを追加または削除します。
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMakeDXFDlg

CMakeDXFDlg::CMakeDXFDlg(CNCDoc* pDoc) : CPropertySheet(IDS_MAKE_DXF, NULL, 0)
{
	m_psh.dwFlags |= PSH_NOAPPLYNOW;
	m_psh.dwFlags &= ~PSH_HASHELP;

	m_pDoc = pDoc;
	ASSERT(m_pDoc);

	AddPage(&m_dlg1);
	AddPage(&m_dlg2);
}

CMakeDXFDlg::~CMakeDXFDlg()
{
}

/////////////////////////////////////////////////////////////////////////////
// CMakeDXFDlg メッセージ ハンドラ
