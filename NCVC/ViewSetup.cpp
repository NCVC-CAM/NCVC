// ViewSetup.cpp : �C���v�������e�[�V���� �t�@�C��
//

#include "stdafx.h"
#include "NCVC.h"
#include "ViewSetup.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

extern	int		g_nLastPage_ViewSetup;

using std::string;
using namespace boost;

BEGIN_MESSAGE_MAP(CViewSetup, CPropertySheet)
	//{{AFX_MSG_MAP(CViewSetup)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED (ID_APPLY_NOW, &CViewSetup::OnApplyNow)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CViewSetup

CViewSetup::CViewSetup(void) :
	CPropertySheet(IDS_VIEW_SETUP, NULL, g_nLastPage_ViewSetup)
{
	m_psh.dwFlags &= ~PSH_HASHELP;

	AddPage(&m_dlg1);	// ����
	AddPage(&m_dlg4);	// ���ر���ڰ��ݒ�
	AddPage(&m_dlg2);	// �Эڰ��ݴر�ݒ�
	AddPage(&m_dlg6);	// 4�ʽ��دĕ\���ݒ�
	AddPage(&m_dlg5);	// OpenGL�ݒ�
	AddPage(&m_dlg3);	// CAD�ް��\���ݒ�
}

/////////////////////////////////////////////////////////////////////////////
// CViewSetup ���b�Z�[�W �n���h��

void CViewSetup::OnDestroy() 
{
	CPropertySheet::OnDestroy();

	// ׽��߰�ނ̾��
	g_nLastPage_ViewSetup = GetActiveIndex();
}

void CViewSetup::OnApplyNow()
{
	// ����è���߰�ނ��ް�����
	if ( !GetActivePage()->OnKillActive() )
		return;

	// �e�߰�ނ��ް��i�[
	CPropertyPage*	pPage;
	for ( int i=0; i<GetPageCount(); i++ ) {
		pPage = GetPage(i);
		if ( ::IsWindow(pPage->GetSafeHwnd()) )
			if ( !pPage->OnApply() )
				return;
	}

	// �e�߰�ޑ�\����
	// Ҳ��ڰсC�e�ޭ��ւ̍X�V�ʒm
	AfxGetNCVCApp()->ChangeViewOption();
}

CString CViewSetup::GetChangeFontButtonText(LOGFONT* plfFont)
{
	CClientDC	dc(AfxGetMainWnd());
	CString	strText, strFace;
	if ( lstrlen(plfFont->lfFaceName) > 0 )
		strFace = plfFont->lfFaceName;
	else
		strFace = "���ѕW��";
	strText = strFace + " (" +
		lexical_cast<string>((int)(abs(plfFont->lfHeight) * 72.0 / dc.GetDeviceCaps(LOGPIXELSY) + 0.5)).c_str() +
		" pt)";
	return strText;
}
