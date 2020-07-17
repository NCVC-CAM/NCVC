// MKNCSetup.cpp : �C���v�������e�[�V���� �t�@�C��
//

#include "stdafx.h"
#include "NCVC.h"
#include "MKNCSetup.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

extern	int		g_nLastPage_NCMake;

BEGIN_MESSAGE_MAP(CMKNCSetup, CPropertySheet)
	//{{AFX_MSG_MAP(CMKNCSetup)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED (ID_APPLY_NOW, OnApplyNow)
END_MESSAGE_MAP()

IMPLEMENT_DYNAMIC(CMKNCSetup, CPropertySheet)

/////////////////////////////////////////////////////////////////////////////
// CMKNCSetup

CMKNCSetup::CMKNCSetup(LPCTSTR lpszCaption, LPCTSTR lpszInitFile)
	: CPropertySheet(lpszCaption, NULL, g_nLastPage_NCMake)
{
	m_psh.dwFlags &= ~PSH_HASHELP;

	AddPage(&m_dlg1);	// ��{
	AddPage(&m_dlg2);	// ����
	AddPage(&m_dlg6);	// �\�L
	AddPage(&m_dlg3);	// �[��
	AddPage(&m_dlg4);	// �����H
	AddPage(&m_dlg8);	// ���C��
	AddPage(&m_dlg5);	// �œK��

	// �؍����Ұ���޼ު�Ă̐���
	try {
		m_pNCMake = new CNCMakeMillOpt(lpszInitFile);
	}
	catch (CMemoryException* e) {
		if ( m_pNCMake ) {
			delete m_pNCMake;
			m_pNCMake = NULL;
		}
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
	}
}

CMKNCSetup::~CMKNCSetup()
{
	if ( m_pNCMake )
		delete	m_pNCMake;
}

/////////////////////////////////////////////////////////////////////////////
// CMKNCSetup ���b�Z�[�W �n���h��

BOOL CMKNCSetup::OnInitDialog() 
{
	// �؍����Ұ���޼ު�Ă������ł��Ă��Ȃ����
	if ( !m_pNCMake ) {
		EndDialog(IDCANCEL);
		return TRUE;
	}

	// �K�p���݂��u�V�K�ۑ��v��
	CString	strTitle;
	VERIFY(strTitle.LoadString(IDS_NEW_SAVE));	// "�V�K�ۑ�"
	GetDlgItem(ID_APPLY_NOW)->SetWindowText(strTitle);
	GetDlgItem(ID_APPLY_NOW)->EnableWindow();

	return CPropertySheet::OnInitDialog();
}

void CMKNCSetup::OnDestroy() 
{
	CPropertySheet::OnDestroy();

	// ׽��߰�ނ̾��
	g_nLastPage_NCMake = GetActiveIndex();
}

// �V�K�ۑ�
void CMKNCSetup::OnApplyNow()
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
				return;	// OnKillActive() �������ς݂Ȃ̂ŁC���蓾�Ȃ�
	}

	CString	strFileName, strPath, strName;
	::Path_Name_From_FullPath(m_pNCMake->GetInitFile(), strPath, strName);

	if ( ::NCVC_FileDlgCommon(IDS_OPTION_INITSAVE, IDS_NCIM_FILTER, FALSE,
				strFileName, strPath,
				FALSE, OFN_OVERWRITEPROMPT|OFN_HIDEREADONLY) == IDOK ) {
		if ( m_pNCMake->SaveMakeOption(strFileName) ) {
			// ���߼�ݕύX
			VERIFY(strName.LoadString(IDS_MAKE_NCD));
			SetTitle(::AddDialogTitle2File(strName, m_pNCMake->GetInitFile()));
		}
	}
}
