// MakeLatheSetup.cpp : �����t�@�C��
//

#include "stdafx.h"
#include "NCVC.h"
#include "MakeLatheSetup.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

extern	int		g_nLastPage_NCMakeLathe;

IMPLEMENT_DYNAMIC(CMakeLatheSetup, CPropertySheet)

BEGIN_MESSAGE_MAP(CMakeLatheSetup, CPropertySheet)
	ON_WM_DESTROY()
	ON_BN_CLICKED (ID_APPLY_NOW, &CMakeLatheSetup::OnApplyNow)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMakeLatheSetup

CMakeLatheSetup::CMakeLatheSetup(LPCTSTR pszCaption, LPCTSTR lpszInitFile)
	: CPropertySheet(pszCaption, NULL, g_nLastPage_NCMakeLathe)
{
	m_psh.dwFlags &= ~PSH_HASHELP;

	AddPage(&m_dlg0);	// ��{
	AddPage(&m_dlg1);	// ����
	AddPage(&m_dlg2);	// �\�L
	AddPage(&m_dlg3);	// �[��
	AddPage(&m_dlg4);	// ����
	AddPage(&m_dlg5);	// ���a
	AddPage(&m_dlg6);	// �O�a
	AddPage(&m_dlg7);	// �ː�

	// �؍����Ұ���޼ު�Ă̐���
	try {
		m_pNCMake = new CNCMakeLatheOpt(lpszInitFile);
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

CMakeLatheSetup::~CMakeLatheSetup()
{
	if ( m_pNCMake )
		delete	m_pNCMake;
}

/////////////////////////////////////////////////////////////////////////////
// CMakeLatheSetup ���b�Z�[�W �n���h��

BOOL CMakeLatheSetup::OnInitDialog() 
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

void CMakeLatheSetup::OnDestroy() 
{
	CPropertySheet::OnDestroy();

	// ׽��߰�ނ̾��
	g_nLastPage_NCMakeLathe = GetActiveIndex();
}

// �V�K�ۑ�
void CMakeLatheSetup::OnApplyNow()
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

	if ( ::NCVC_FileDlgCommon(IDS_OPTION_INITSAVE, IDS_NCIL_FILTER, FALSE,
				strFileName, strPath,
				FALSE, OFN_OVERWRITEPROMPT|OFN_HIDEREADONLY) == IDOK ) {
		if ( m_pNCMake->SaveMakeOption(strFileName) ) {
			// ���߼�ݕύX
			VERIFY(strName.LoadString(IDS_MAKE_NCD));
			SetTitle(::AddDialogTitle2File(strName, m_pNCMake->GetInitFile()));
		}
		else {
			strName.Format(IDS_ERR_WRITESETTING, strFileName);
			AfxMessageBox(strName, MB_OK|MB_ICONEXCLAMATION);
		}
	}
}
