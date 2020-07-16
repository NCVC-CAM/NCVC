// MakeNCDWiz.cpp : 実装ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "MakeNCDWiz.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
extern	CMagaDbg	g_dbg;
#endif

// CMakeNCDWiz

BEGIN_MESSAGE_MAP(CMakeNCDWiz, CPropertySheet)
END_MESSAGE_MAP()

CMakeNCDWiz::CMakeNCDWiz(CDXFDoc* pDoc, LPMAKENCDWIZARDPARAM pParam) : CPropertySheet(IDS_SHAPE_NCD, NULL, 0)
{
	m_psh.dwFlags &= ~PSH_HASHELP;

	m_pDoc = pDoc;
	m_pParam = pParam;
	::ZeroMemory(m_pParam, sizeof(MAKENCDWIZARDPARAM));

	AddPage(&m_dlg1);
	AddPage(&m_dlg2);
	AddPage(&m_dlg3);
	AddPage(&m_dlg9);
	SetWizardMode();
}

CMakeNCDWiz::~CMakeNCDWiz()
{
}

// CMakeNCDWiz メッセージ ハンドラ

