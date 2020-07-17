// MCSetup4.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "MCOption.h"
#include "MCSetup.h"
#include <afxshellmanager.h>

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

BEGIN_MESSAGE_MAP(CMCSetup4, CPropertyPage)
	//{{AFX_MSG_MAP(CMCSetup4)
	ON_BN_CLICKED(IDC_MCST4_MACROFOLDER_BT, &CMCSetup4::OnFolder)
	ON_BN_CLICKED(IDC_MCST4_MACROIF_BT, &CMCSetup4::OnMacroIF)
	ON_EN_KILLFOCUS(IDC_MCST4_MACROCODE, &CMCSetup4::OnKillFocusMacroCode)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMCSetup4 プロパティ ページ

CMCSetup4::CMCSetup4() : CPropertyPage(CMCSetup4::IDD)
{
	m_psp.dwFlags &= ~PSP_HASHELP;
	//{{AFX_DATA_INIT(CMCSetup4)
	//}}AFX_DATA_INIT
	ASSERT( SIZEOF(m_strMacro) == MCMACROSTRING );
	const CMCOption*	pMCopt = AfxGetNCVCApp()->GetMCOption();
	for ( int i=0; i<SIZEOF(m_strMacro); i++ )
		m_strMacro[i] = pMCopt->m_strMacroOpt[i];
}

void CMCSetup4::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMCSetup4)
	//}}AFX_DATA_MAP
	for ( int i=0; i<SIZEOF(m_strMacro); i++ ) {
		DDX_Text(pDX, i+IDC_MCST4_MACROCODE, m_strMacro[i]);
		DDX_Control(pDX, i+IDC_MCST4_MACROCODE, m_ctMacro[i]);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CMCSetup4 メッセージ ハンドラ

void CMCSetup4::OnFolder() 
{
	CString	strResult;
	CShellManager*	pShell = AfxGetNCVCApp()->GetShellManager();
	pShell->BrowseForFolder(strResult, this, NULL, NULL,
		BIF_BROWSEFORCOMPUTER | BIF_DONTGOBELOWDOMAIN | BIF_RETURNONLYFSDIRS);

	if ( !strResult.IsEmpty() ) {
		m_strMacro[MCMACROFOLDER] = strResult;
		UpdateData(FALSE);
		m_ctMacro[MCMACROFOLDER].SetSel(0, -1);
		m_ctMacro[MCMACROFOLDER].SetFocus();
	}
}

void CMCSetup4::OnMacroIF() 
{
	UpdateData();
	CString		strPath, strFile;
	::Path_Name_From_FullPath(m_strMacro[MCMACROIF], strPath, strFile);
	if ( strFile.IsEmpty() )
		strFile = m_strMacro[MCMACROIF];
	if ( ::NCVC_FileDlgCommon(IDS_OPTION_MC_MACRO, IDS_EXE_FILTER, TRUE, strFile, strPath) == IDOK ) {
		m_strMacro[MCMACROIF] = strFile;
		UpdateData(FALSE);
	}
}

void CMCSetup4::OnKillFocusMacroCode() 
{
	UpdateData();
	if ( !m_strMacro[MCMACROCODE].IsEmpty() && m_strMacro[MCMACROARGV].IsEmpty() ) {
		// 引数補間
		m_strMacro[MCMACROARGV] = AfxGetNCVCApp()->GetMCOption()->GetDefaultOption();
		UpdateData(FALSE);
	}
}

BOOL CMCSetup4::OnApply() 
{
	CMCSetup*	pParent = static_cast<CMCSetup *>(GetParentSheet());
	CMCOption*	pMCopt = AfxGetNCVCApp()->GetMCOption();

	// 再読込ﾁｪｯｸ
	for ( int i=0; i<SIZEOF(m_strMacro); i++ ) {
		if ( pMCopt->m_strMacroOpt[i] != m_strMacro[i] ) {
			pMCopt->m_strMacroOpt[i] = m_strMacro[i];
			pParent->m_bReload = TRUE;		// 再読込が必要
		}
	}

	return TRUE;
}

BOOL CMCSetup4::OnKillActive() 
{
	if ( !__super::OnKillActive() )
		return FALSE;

	// ﾏｸﾛ呼び出し関連はﾌｫﾙﾀﾞ以外全ての項目が必要
	if ( !m_strMacro[MCMACROCODE].IsEmpty() ) {
		for ( int i=MCMACROIF/*2*/; i<SIZEOF(m_strMacro); i++ ) {
			if ( m_strMacro[i].IsEmpty() ) {
				AfxMessageBox(IDS_ERR_MCMACRO, MB_OK|MB_ICONEXCLAMATION);
				m_ctMacro[i].SetFocus();
				return FALSE;
			}
		}
	}

	return TRUE;
}
