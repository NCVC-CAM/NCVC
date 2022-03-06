// MachineSetup.cpp : インプリメンテーション ファイル
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

	// 機械情報の読み込み
	AfxGetNCVCApp()->GetMachineOption()->ReadMachineOption(lpszFile, FALSE);

	AddPage(&m_dlg1);	// 基本
	AddPage(&m_dlg2);	// 座標
	AddPage(&m_dlg3);	// 工具
	AddPage(&m_dlg4);	// マクロ
	AddPage(&m_dlg5);	// CNC
}

/////////////////////////////////////////////////////////////////////////////
// CMachineSetup メッセージ ハンドラ

BOOL CMachineSetup::OnInitDialog() 
{
	// 適用ﾎﾞﾀﾝを「新規保存」に
	CString	strTitle;
	VERIFY(strTitle.LoadString(IDS_NEW_SAVE));	// "新規保存"
	GetDlgItem(ID_APPLY_NOW)->SetWindowText(strTitle);
	GetDlgItem(ID_APPLY_NOW)->EnableWindow();

	return CPropertySheet::OnInitDialog();
}

void CMachineSetup::OnDestroy() 
{
	CPropertySheet::OnDestroy();

	// ﾗｽﾄﾍﾟｰｼﾞのｾｯﾄ
	g_nLastPage_MachineSetup = GetActiveIndex();
}

// 新規保存
void CMachineSetup::OnApplyNow()
{
	// 現ｱｸﾃｨﾌﾞﾍﾟｰｼﾞのﾃﾞｰﾀﾁｪｯｸ
	if ( !GetActivePage()->OnKillActive() )
		return;

	// 各ﾍﾟｰｼﾞのﾃﾞｰﾀ格納
	CPropertyPage*	pPage;
	for ( int i=0; i<GetPageCount(); i++ ) {
		pPage = GetPage(i);
		if ( ::IsWindow(pPage->GetSafeHwnd()) )
			if ( !pPage->OnApply() )
				return;	// OnKillActive() でﾁｪｯｸ済みなので，あり得ない
	}

	CMachineOption*	pMCopt = AfxGetNCVCApp()->GetMachineOption();
	CString	strFileName(pMCopt->GetMCHeadFileName()), strPath, strName;
	if ( !strFileName.IsEmpty() )
		::Path_Name_From_FullPath(strFileName, strPath, strName);

	if ( ::NCVC_FileDlgCommon(IDS_OPTION_MCSAVE, IDS_MC_FILTER, FALSE,
				strFileName, strPath,
				FALSE, OFN_OVERWRITEPROMPT|OFN_HIDEREADONLY) == IDOK ) {
		if ( pMCopt->SaveMachineOption(strFileName) ) {
			// ｷｬﾌﾟｼｮﾝ変更
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
