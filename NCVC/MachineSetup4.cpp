// MachineSetup4.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "MachineOption.h"
#include "MachineSetup.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CMachineSetup4, CPropertyPage)
	//{{AFX_MSG_MAP(CMachineSetup4)
	ON_BN_CLICKED(IDC_MCST4_MACROFOLDER_BT, &CMachineSetup4::OnFolder)
	ON_BN_CLICKED(IDC_MCST4_MACROIF_BT, &CMachineSetup4::OnMacroIF)
	ON_EN_KILLFOCUS(IDC_MCST4_MACROCODE, &CMachineSetup4::OnKillFocusMacroCode)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMachineSetup4 プロパティ ページ

CMachineSetup4::CMachineSetup4() : CPropertyPage(CMachineSetup4::IDD)
{
	m_psp.dwFlags &= ~PSP_HASHELP;
	//{{AFX_DATA_INIT(CMachineSetup4)
	//}}AFX_DATA_INIT
	ASSERT( SIZEOF(m_strMacro) == MCMACROSTRING );
	const CMachineOption*	pMCopt = AfxGetNCVCApp()->GetMachineOption();
	for ( int i=0; i<SIZEOF(m_strMacro); i++ )
		m_strMacro[i] = pMCopt->m_strMacroOpt[i];
}

void CMachineSetup4::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMachineSetup4)
	//}}AFX_DATA_MAP
	for ( int i=0; i<SIZEOF(m_strMacro); i++ ) {
		DDX_Text(pDX, i+IDC_MCST4_MACROCODE, m_strMacro[i]);
		DDX_Control(pDX, i+IDC_MCST4_MACROCODE, m_ctMacro[i]);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CMachineSetup4 メッセージ ハンドラ

void CMachineSetup4::OnFolder() 
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

void CMachineSetup4::OnMacroIF() 
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

void CMachineSetup4::OnKillFocusMacroCode() 
{
	UpdateData();
	if ( !m_strMacro[MCMACROCODE].IsEmpty() && m_strMacro[MCMACROARGV].IsEmpty() ) {
		// 引数補間
		m_strMacro[MCMACROARGV] = AfxGetNCVCApp()->GetMachineOption()->GetDefaultOption();
		UpdateData(FALSE);
	}
}

BOOL CMachineSetup4::OnApply() 
{
	CMachineSetup*	pParent = static_cast<CMachineSetup *>(GetParentSheet());
	CMachineOption*	pMCopt = AfxGetNCVCApp()->GetMachineOption();

	// 再読込ﾁｪｯｸ
	if ( pMCopt->m_strMacroOpt[0] != m_strMacro[0] ) {
		try {
			pMCopt->m_regMacroCode = boost::xpressive::sregex::compile(LPCTSTR(m_strMacro[0]));
		}
		catch (boost::xpressive::regex_error&) {
			AfxMessageBox(IDS_ERR_REGEX, MB_OK|MB_ICONEXCLAMATION);
			m_ctMacro[0].SetFocus();
			return FALSE;
		}
		pMCopt->m_strMacroOpt[0] = m_strMacro[0];
		pParent->m_bReload = TRUE;
	}
	for ( int i=1; i<SIZEOF(m_strMacro); i++ ) {
		if ( pMCopt->m_strMacroOpt[i] != m_strMacro[i] ) {
			pMCopt->m_strMacroOpt[i] = m_strMacro[i];
			pParent->m_bReload = TRUE;
		}
	}

	return TRUE;
}

BOOL CMachineSetup4::OnKillActive() 
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
