// MakeNCDWiz1.cpp : 実装ファイル
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

// CMakeNCDWiz1 ダイアログ

BEGIN_MESSAGE_MAP(CMakeNCDWiz1, CPropertyPage)
END_MESSAGE_MAP()

CMakeNCDWiz1::CMakeNCDWiz1() : CPropertyPage(CMakeNCDWiz1::IDD)
{
	m_psp.dwFlags &= ~PSP_HASHELP;

	m_nMakeType = 0;
}

CMakeNCDWiz1::~CMakeNCDWiz1()
{
}

void CMakeNCDWiz1::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Radio(pDX, IDC_MAKENCD_TYPE1, m_nMakeType);
}

// CMakeNCDWiz1 メッセージ ハンドラ

BOOL CMakeNCDWiz1::OnSetActive()
{
	((CPropertySheet *)GetParent())->SetWizardButtons(PSWIZB_NEXT);
	return CPropertyPage::OnSetActive();
}

LRESULT CMakeNCDWiz1::OnWizardNext()
{
	UpdateData();
	((CMakeNCDWiz *)GetParent())->m_pParam->nType = m_nMakeType;
	LRESULT	lResult = -1;
	switch ( m_nMakeType ) {
	case 0:
		lResult = IDD_MAKENCD_WIZ2;
		break;
	case 1:
		lResult = IDD_MAKENCD_WIZ3;
		break;
	}
	return lResult;	// 次に表示するﾀﾞｲｱﾛｸﾞID
}
