// ViewSetup.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "resource.h"
#include "ViewSetup.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

extern	int		g_nLastPage_ViewSetup;

BEGIN_MESSAGE_MAP(CViewSetup, CPropertySheet)
	//{{AFX_MSG_MAP(CViewSetup)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CViewSetup

CViewSetup::CViewSetup(void) :
	CPropertySheet(IDS_VIEW_SETUP, NULL, g_nLastPage_ViewSetup)
{
	m_psh.dwFlags &= ~PSH_HASHELP;

	AddPage(&m_dlg1);
	AddPage(&m_dlg4);
	AddPage(&m_dlg2);
	AddPage(&m_dlg3);
}

CViewSetup::~CViewSetup()
{
}

/////////////////////////////////////////////////////////////////////////////
// CViewSetup メッセージ ハンドラ

void CViewSetup::OnDestroy() 
{
	CPropertySheet::OnDestroy();

	// ﾗｽﾄﾍﾟｰｼﾞのｾｯﾄ
	g_nLastPage_ViewSetup = GetActiveIndex();
}

CString CViewSetup::GetChangeFontButtonText(LOGFONT* plfFont)
{
	CClientDC	dc(AfxGetMainWnd());
	CString	strText, strFace;
	if ( lstrlen(plfFont->lfFaceName) > 0 )
		strFace = plfFont->lfFaceName;
	else
		strFace = "ｼｽﾃﾑ標準";
	strText.Format("%s (%d pt)", strFace,
		(int)(abs(plfFont->lfHeight) * 72.0 / dc.GetDeviceCaps(LOGPIXELSY) + 0.5) );

	return strText;
}
