// MCSetup.cpp : �C���v�������e�[�V���� �t�@�C��
//

#include "stdafx.h"
#include "NCVC.h"
#include "MCOption.h"
#include "MCSetup.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

extern	int		g_nLastPage_MCSetup;

BEGIN_MESSAGE_MAP(CMCSetup, CPropertySheet)
	//{{AFX_MSG_MAP(CMCSetup)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED (ID_APPLY_NOW, OnApplyNow)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMCSetup

CMCSetup::CMCSetup(LPCTSTR lpszCaption, LPCTSTR lpszFile) :
	CPropertySheet(lpszCaption, NULL, g_nLastPage_MCSetup)
{
	m_psh.dwFlags &= ~PSH_HASHELP;

	m_strFileName = lpszFile;
	m_bCalcThread = m_bReload = FALSE;

	// �@�B���̓ǂݍ���
	AfxGetNCVCApp()->GetMCOption()->ReadMCoption(lpszFile, FALSE);

	AddPage(&m_dlg1);	// ��{
	AddPage(&m_dlg2);	// ���W
	AddPage(&m_dlg3);	// �H��
	AddPage(&m_dlg4);	// �}�N��
	AddPage(&m_dlg5);	// CNC
}

CMCSetup::~CMCSetup()
{
}

/////////////////////////////////////////////////////////////////////////////
// CMCSetup ���b�Z�[�W �n���h��

BOOL CMCSetup::OnInitDialog() 
{
	// �K�p���݂��u�V�K�ۑ��v��
	GetDlgItem(ID_APPLY_NOW)->SetWindowText("�V�K�ۑ�");
	GetDlgItem(ID_APPLY_NOW)->EnableWindow();

	return CPropertySheet::OnInitDialog();
}

void CMCSetup::OnDestroy() 
{
	CPropertySheet::OnDestroy();

	// ׽��߰�ނ̾��
	g_nLastPage_MCSetup = GetActiveIndex();
}

// �V�K�ۑ�
void CMCSetup::OnApplyNow()
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

	CMCOption*	pMCopt = AfxGetNCVCApp()->GetMCOption();
	CString	strFileName(pMCopt->GetMCHeadFileName()), strPath, strName;
	if ( !strFileName.IsEmpty() )
		::Path_Name_From_FullPath(strFileName, strPath, strName);

	if ( ::NCVC_FileDlgCommon(IDS_OPTION_MCSAVE, IDS_MC_FILTER, FALSE,
				strFileName, strPath,
				FALSE, OFN_OVERWRITEPROMPT|OFN_HIDEREADONLY) == IDOK ) {
		if ( pMCopt->SaveMCoption(strFileName) ) {
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
