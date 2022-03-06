// MachineSetup.cpp : �C���v�������e�[�V���� �t�@�C��
//

#include "stdafx.h"
#include "NCVC.h"
#include "MachineOption.h"
#include "MachineSetup.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

extern	int		g_nLastPage_MachineSetup;

BEGIN_MESSAGE_MAP(CMachineSetup, CPropertySheet)
	//{{AFX_MSG_MAP(CMachineSetup)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED (ID_APPLY_NOW, &CMachineSetup::OnApplyNow)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMachineSetup

CMachineSetup::CMachineSetup(LPCTSTR lpszCaption, LPCTSTR lpszFile) :
	CPropertySheet(lpszCaption, NULL, g_nLastPage_MachineSetup)
{
	m_psh.dwFlags &= ~PSH_HASHELP;

	m_strFileName = lpszFile;
	m_bCalcThread = m_bReload = FALSE;

	// �@�B���̓ǂݍ���
	AfxGetNCVCApp()->GetMachineOption()->ReadMachineOption(lpszFile, FALSE);

	AddPage(&m_dlg1);	// ��{
	AddPage(&m_dlg2);	// ���W
	AddPage(&m_dlg3);	// �H��
	AddPage(&m_dlg4);	// �}�N��
	AddPage(&m_dlg5);	// CNC
}

/////////////////////////////////////////////////////////////////////////////
// CMachineSetup ���b�Z�[�W �n���h��

BOOL CMachineSetup::OnInitDialog() 
{
	// �K�p���݂��u�V�K�ۑ��v��
	CString	strTitle;
	VERIFY(strTitle.LoadString(IDS_NEW_SAVE));	// "�V�K�ۑ�"
	GetDlgItem(ID_APPLY_NOW)->SetWindowText(strTitle);
	GetDlgItem(ID_APPLY_NOW)->EnableWindow();

	return CPropertySheet::OnInitDialog();
}

void CMachineSetup::OnDestroy() 
{
	CPropertySheet::OnDestroy();

	// ׽��߰�ނ̾��
	g_nLastPage_MachineSetup = GetActiveIndex();
}

// �V�K�ۑ�
void CMachineSetup::OnApplyNow()
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

	CMachineOption*	pMCopt = AfxGetNCVCApp()->GetMachineOption();
	CString	strFileName(pMCopt->GetMCHeadFileName()), strPath, strName;
	if ( !strFileName.IsEmpty() )
		::Path_Name_From_FullPath(strFileName, strPath, strName);

	if ( ::NCVC_FileDlgCommon(IDS_OPTION_MCSAVE, IDS_MC_FILTER, FALSE,
				strFileName, strPath,
				FALSE, OFN_OVERWRITEPROMPT|OFN_HIDEREADONLY) == IDOK ) {
		if ( pMCopt->SaveMachineOption(strFileName) ) {
			// ���߼�ݕύX
			::Path_Name_From_FullPath(strFileName, strPath, strName);
			CString	strCaption;
			VERIFY(strCaption.LoadString(IDS_SETUP_MC));
			SetTitle(::AddDialogTitle2File(strCaption, strName));
			m_strFileName = strFileName;
		}
		else {
			strName.Format(IDS_ERR_WRITESETTING, strFileName);
			AfxMessageBox(strName, MB_OK|MB_ICONEXCLAMATION);
		}
	}
}
