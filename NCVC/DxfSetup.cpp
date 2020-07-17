// DxfSetup.cpp : �C���v�������e�[�V���� �t�@�C��
//

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "DXFDoc.h"
#include "DxfSetup.h"
#include "DxfSetupReload.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

extern	int		g_nLastPage_DXFSetup;

BEGIN_MESSAGE_MAP(CDxfSetup, CPropertySheet)
	//{{AFX_MSG_MAP(CDxfSetup)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDxfSetup

CDxfSetup::CDxfSetup(UINT nIDCaption) :
	CPropertySheet(nIDCaption, NULL, g_nLastPage_DXFSetup)
{
	m_psh.dwFlags |= PSH_NOAPPLYNOW;
	m_psh.dwFlags &= ~PSH_HASHELP;

	AddPage(&m_dlg1);
	AddPage(&m_dlg2);
}

/////////////////////////////////////////////////////////////////////////////
// ���ފ֐�

BOOL CDxfSetup::OnReload(CPropertyPage* pActPage)
{
	// �e�߰�ނ��ް��i�[
	CPropertyPage*	pPage;
	for ( int i=0; i<GetPageCount(); i++ ) {
		pPage = GetPage(i);
		if ( ::IsWindow(pPage->GetSafeHwnd()) && pActPage!=pPage )
			pPage->OnApply();
	}

	// �޷���Ă̐����P�����Ȃ�C������ēǍ�
	if ( AfxGetNCVCApp()->GetDXFOpenDocumentCount() == 1 ) {
		CDXFDoc* pDoc = static_cast<CDXFDoc*>(AfxGetNCVCApp()->GetAlreadyDocument(TYPE_DXF));
		if ( !pDoc->GetPathName().IsEmpty() )
			pDoc->SetDocFlag(DXFDOC_RELOAD);
	}
	else {
		CDxfSetupReload	dlg(this);
		if ( dlg.DoModal() != IDOK )
			return FALSE;
	}

	AfxGetNCVCApp()->ReloadDXFDocument();
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CDxfSetup ���b�Z�[�W �n���h��

void CDxfSetup::OnDestroy() 
{
	CPropertySheet::OnDestroy();

	// ׽��߰�ނ̾��
	g_nLastPage_DXFSetup = GetActiveIndex();
}
