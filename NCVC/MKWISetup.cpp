// MKWISetup.cpp : �����t�@�C��
//

#include "stdafx.h"
#include "NCVC.h"
#include "MKWISetup.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

extern	int		g_nLastPage_NCMakeWire;

BEGIN_MESSAGE_MAP(CMKWISetup, CPropertySheet)
END_MESSAGE_MAP()

IMPLEMENT_DYNAMIC(CMKWISetup, CPropertySheet)

/////////////////////////////////////////////////////////////////////////////
// CMKWISetup

CMKWISetup::CMKWISetup(LPCTSTR pszCaption, LPCTSTR lpszInitFile)
	: CPropertySheet(pszCaption, NULL, g_nLastPage_NCMakeWire)
{
	m_psh.dwFlags &= ~PSH_HASHELP;

	AddPage(&m_dlg1);	// ��{
	AddPage(&m_dlg2);	// AWF
	AddPage(&m_dlg3);	// ����
	AddPage(&m_dlg4);	// �\�L

	// �؍����Ұ���޼ު�Ă̐���
	try {
		m_pNCMake = new CNCMakeWireOpt(lpszInitFile);
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

CMKWISetup::~CMKWISetup()
{
	if ( m_pNCMake )
		delete	m_pNCMake;
}

/////////////////////////////////////////////////////////////////////////////
// CMKWISetup ���b�Z�[�W �n���h��

BOOL CMKWISetup::OnInitDialog() 
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

void CMKWISetup::OnDestroy() 
{
	CPropertySheet::OnDestroy();

	// ׽��߰�ނ̾��
	g_nLastPage_NCMakeWire = GetActiveIndex();
}

// �V�K�ۑ�
void CMKWISetup::OnApplyNow()
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

	if ( ::NCVC_FileDlgCommon(IDS_OPTION_INITSAVE, IDS_NCIW_FILTER, FALSE,
				strFileName, strPath,
				FALSE, OFN_OVERWRITEPROMPT|OFN_HIDEREADONLY) == IDOK ) {
		if ( m_pNCMake->SaveMakeOption(strFileName) ) {
			// ���߼�ݕύX
			VERIFY(strName.LoadString(IDS_MAKE_NCD));
			SetTitle(::AddDialogTitle2File(strName, m_pNCMake->GetInitFile()));
		}
	}
}