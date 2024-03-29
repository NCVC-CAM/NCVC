// MakeNCSetup.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "MakeNCSetup.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

extern	int		g_nLastPage_NCMake;

IMPLEMENT_DYNAMIC(CMakeNCSetup, CPropertySheet)

BEGIN_MESSAGE_MAP(CMakeNCSetup, CPropertySheet)
	//{{AFX_MSG_MAP(CMakeNCSetup)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED (ID_APPLY_NOW, &CMakeNCSetup::OnApplyNow)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMakeNCSetup

CMakeNCSetup::CMakeNCSetup(LPCTSTR lpszCaption, LPCTSTR lpszInitFile)
	: CPropertySheet(lpszCaption, NULL, g_nLastPage_NCMake)
{
	m_psh.dwFlags &= ~PSH_HASHELP;

	AddPage(&m_dlg1);	// 基本
	AddPage(&m_dlg2);	// 生成
	AddPage(&m_dlg6);	// 表記
	AddPage(&m_dlg3);	// 深彫
	AddPage(&m_dlg4);	// 穴加工
	AddPage(&m_dlg8);	// レイヤ
	AddPage(&m_dlg5);	// 最適化

	// 切削ﾊﾟﾗﾒｰﾀｵﾌﾞｼﾞｪｸﾄの生成
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

CMakeNCSetup::~CMakeNCSetup()
{
	if ( m_pNCMake )
		delete	m_pNCMake;
}

/////////////////////////////////////////////////////////////////////////////
// CMakeNCSetup メッセージ ハンドラ

BOOL CMakeNCSetup::OnInitDialog() 
{
	// 切削ﾊﾟﾗﾒｰﾀｵﾌﾞｼﾞｪｸﾄが生成できていなければ
	if ( !m_pNCMake ) {
		EndDialog(IDCANCEL);
		return TRUE;
	}

	// 適用ﾎﾞﾀﾝを「新規保存」に
	CString	strTitle;
	VERIFY(strTitle.LoadString(IDS_NEW_SAVE));	// "新規保存"
	GetDlgItem(ID_APPLY_NOW)->SetWindowText(strTitle);
	GetDlgItem(ID_APPLY_NOW)->EnableWindow();

	return __super::OnInitDialog();
}

void CMakeNCSetup::OnDestroy() 
{
	__super::OnDestroy();

	// ﾗｽﾄﾍﾟｰｼﾞのｾｯﾄ
	g_nLastPage_NCMake = GetActiveIndex();
}

// 新規保存
void CMakeNCSetup::OnApplyNow()
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

	CString	strFileName, strPath, strName;
	::Path_Name_From_FullPath(m_pNCMake->GetInitFile(), strPath, strName);

	if ( ::NCVC_FileDlgCommon(IDS_OPTION_INITSAVE, IDS_NCIM_FILTER, FALSE,
				strFileName, strPath,
				FALSE, OFN_OVERWRITEPROMPT|OFN_HIDEREADONLY) == IDOK ) {
		if ( m_pNCMake->SaveMakeOption(strFileName) ) {
			// ｷｬﾌﾟｼｮﾝ変更
			VERIFY(strName.LoadString(IDS_MAKE_NCD));
			SetTitle(::AddDialogTitle2File(strName, m_pNCMake->GetInitFile()));
		}
		else {
			strName.Format(IDS_ERR_WRITESETTING, strFileName);
			AfxMessageBox(strName, MB_OK|MB_ICONEXCLAMATION);
		}
	}
}
