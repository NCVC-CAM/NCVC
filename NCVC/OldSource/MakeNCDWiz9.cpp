// MakeNCDWiz9.cpp : 実装ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "MakeNCDWiz.h"
#include "MKNCSetup.h"
#include "MakeNCDlg.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
extern	CMagaDbg	g_dbg;
#endif

// CMakeNCDWiz9 ダイアログ

BEGIN_MESSAGE_MAP(CMakeNCDWiz9, CPropertyPage)
	ON_BN_CLICKED(IDC_MKNC_NCFILEUP, OnMKNCFileUp)
	ON_BN_CLICKED(IDC_MKNC_INITUP, OnMKNCInitUp)
	ON_BN_CLICKED(IDC_MKNC_INITED, OnMKNCInitEdit)
	ON_CBN_SELCHANGE(IDC_MKNC_INIT, OnSelChangeInit)
	ON_EN_KILLFOCUS(IDC_MKNC_NCFILE, OnKillFocusNCFile)
	ON_CBN_KILLFOCUS(IDC_MKNC_INIT, OnKillFocusInit)
END_MESSAGE_MAP()

CMakeNCDWiz9::CMakeNCDWiz9() : CPropertyPage(CMakeNCDWiz9::IDD)
{
	m_psp.dwFlags &= ~PSP_HASHELP;

	m_bNCView = TRUE;
}

CMakeNCDWiz9::~CMakeNCDWiz9()
{
}

void CMakeNCDWiz9::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MKNC_NCFILE, m_ctNCFileName);
	DDX_Text(pDX, IDC_MKNC_NCFILE, m_strNCFileName);
	DDX_Control(pDX, IDC_MKNC_INIT, m_ctInitFileName);
	DDX_CBString(pDX, IDC_MKNC_INIT, m_strInitFileName);
	DDX_Check(pDX, IDC_MKNC_VIEW, m_bNCView);
	DDX_Control(pDX, IDC_MKNC_VIEW, m_ctNCView);
}

// CMakeNCDWiz9 メッセージ ハンドラ

BOOL CMakeNCDWiz9::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	// ｺﾝｽﾄﾗｸﾀではできないｺﾝﾄﾛｰﾙの初期化
	
	// ﾄﾞｷｭﾒﾝﾄ名からNCﾌｧｲﾙ名を作成
	CreateNCFile( ((CMakeNCDWiz *)GetParent())->m_pDoc, m_strNCPath, m_strNCFileName);	// MakeNCD.cpp
	// 切削条件履歴から初期表示ﾌｧｲﾙを取得
	const	CDXFOption*	pOpt = AfxGetNCVCApp()->GetDXFOption();
	if ( pOpt->GetInitList()->GetCount() > 0 )
		::Path_Name_From_FullPath(pOpt->GetInitList()->GetHead(),
				m_strInitPath, m_strInitFileName);
	m_bNCView = pOpt->GetDxfFlag(DXFOPT_VIEW);
	UpdateData(FALSE);
	// 切削条件ｺﾝﾎﾞﾎﾞｯｸｽにｱｲﾃﾑ追加
	InitialMakeNCDlgComboBox(AfxGetNCVCApp()->GetDXFOption()->GetInitList(), m_ctInitFileName);
	// ﾊﾟｽ表示の最適化(shlwapi.h)
	::PathSetDlgItemPath(m_hWnd, IDC_MKNC_NCPATH,   m_strNCPath);
	::PathSetDlgItemPath(m_hWnd, IDC_MKNC_INITPATH, m_strInitPath);

	return TRUE;  // return TRUE unless you set the focus to a control
	// 例外 : OCX プロパティ ページは必ず FALSE を返します。
}

BOOL CMakeNCDWiz9::OnSetActive()
{
	((CPropertySheet *)GetParent())->SetWizardButtons(PSWIZB_BACK | PSWIZB_FINISH);
	return CPropertyPage::OnSetActive();
}

LRESULT CMakeNCDWiz9::OnWizardBack()
{
	LRESULT	lResult = -1;
	switch ( ((CMakeNCDWiz *)GetParent())->m_pParam->nType ) {
	case 0:
		lResult = IDD_MAKENCD_WIZ2;
		break;
	case 1:
		lResult = IDD_MAKENCD_WIZ3;
		break;
	}
	return lResult;
}

BOOL CMakeNCDWiz9::OnWizardFinish()
{
	m_ctNCView.SetFocus();
	UpdateData();
	CString	strInitPath(m_strInitPath+m_strInitFileName),
			strNCPath(m_strNCPath+m_strNCFileName);

	// 切削条件ﾌｧｲﾙのﾁｪｯｸ(ﾌｧｲﾙ必須)
	if ( !::IsFileExist(strInitPath) ) {	// NCVC.cpp
		m_ctInitFileName.SetEditSel(0, -1);
		m_ctInitFileName.SetFocus();
		return FALSE;
	}
	// NC生成ﾌｧｲﾙのﾁｪｯｸ(あれば上書き確認)
	if ( !CheckMakeDlgFileExt(TYPE_NCD, strNCPath) || !::IsFileExist(strNCPath, FALSE) ) {
		m_ctNCFileName.SetFocus();
		m_ctNCFileName.SetSel(0, -1);
		return FALSE;
	}

	CMakeNCDWiz*	pParent = (CMakeNCDWiz *)GetParent();
	pParent->m_strNCFileName = strNCPath;

	// 条件履歴更新
	CDXFOption*	pOpt = AfxGetNCVCApp()->GetDXFOption();
	pOpt->SetViewFlag(m_bNCView);
	pOpt->AddInitHistory(strInitPath);
	pOpt->SaveInitHistory();
	// 機械情報の更新
	AfxGetNCVCApp()->ChangeMachine( pParent->m_dlg2.m_strMachine );

	return CPropertyPage::OnWizardFinish();
}

void CMakeNCDWiz9::OnMKNCFileUp()
{
	UpdateData();
	MakeDlgFileRefer(IDS_DXF2NCD_FILE, AfxGetNCVCApp()->GetFilterString(TYPE_NCD),
			this, IDC_MKNC_NCPATH, m_strNCPath, m_strNCFileName, FALSE);
	// 文字選択状態
	m_ctNCFileName.SetFocus();
	m_ctNCFileName.SetSel(0, -1);
}

void CMakeNCDWiz9::OnMKNCInitUp()
{
	UpdateData();
	CString	strFilter;
	VERIFY(strFilter.LoadString(IDS_NCI_FILTER));
	MakeDlgFileRefer(IDS_OPTION_INIT, strFilter, this, IDC_MKNC_INITPATH,
			m_strInitPath, m_strInitFileName, TRUE);
	// 文字選択状態
	m_ctInitFileName.SetEditSel(0, -1);
	m_ctInitFileName.SetFocus();
}

void CMakeNCDWiz9::OnMKNCInitEdit()
{
	UpdateData();
	MakeNCDlgInitFileEdit(m_strInitPath, m_strInitFileName,
		this, IDC_MKNC_INITPATH, m_ctInitFileName);
}

void CMakeNCDWiz9::OnSelChangeInit()
{
	int nIndex = MakeNCDlgSelChange(m_ctInitFileName, m_hWnd, IDC_MKNC_INITPATH,
			m_strInitPath, m_strInitFileName);
	// 履歴ﾌｧｲﾙの存在ﾁｪｯｸ
	CString	strFullPath(m_strInitPath+m_strInitFileName);
	if ( !::IsFileExist(strFullPath) ) {
		AfxGetNCVCApp()->GetDXFOption()->DelInitHistory(strFullPath);	// 履歴削除
		m_ctInitFileName.DeleteString(nIndex);
	}
}

void CMakeNCDWiz9::OnKillFocusNCFile()
{
	UpdateData();
	MakeDlgKillFocus(m_strNCPath, m_strNCFileName, this, IDC_MKNC_NCPATH);
}

void CMakeNCDWiz9::OnKillFocusInit()
{
	UpdateData();
	MakeDlgKillFocus(m_strInitPath, m_strInitFileName, this, IDC_MKNC_INITPATH);
}
