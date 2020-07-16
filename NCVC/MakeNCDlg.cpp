// MakeNCD.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "NCMakeOption.h"
#include "Layer.h"
#include "DXFDoc.h"
#include "MKNCSetup.h"
#include "MakeNCDlg.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

BEGIN_MESSAGE_MAP(CMakeNCDlg, CDialog)
	//{{AFX_MSG_MAP(CMakeNCDlg)
	ON_BN_CLICKED(IDC_MKNC_NCFILEUP, OnMKNCFileUp)
	ON_BN_CLICKED(IDC_MKNC_INITUP, OnMKNCInitUp)
	ON_BN_CLICKED(IDC_MKNC_INITED, OnMKNCInitEdit)
	ON_CBN_SELCHANGE(IDC_MKNC_INIT, OnSelChangeInit)
	ON_EN_KILLFOCUS(IDC_MKNC_NCFILE, OnKillFocusNCFile)
	ON_CBN_KILLFOCUS(IDC_MKNC_INIT, OnKillFocusInit)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMakeNCDlg ダイアログ

CMakeNCDlg::CMakeNCDlg(UINT nTitle, CDXFDoc* pDoc)
	: CDialog(CMakeNCDlg::IDD, NULL)
{
	m_nTitle = nTitle;
	//{{AFX_DATA_INIT(CMakeNCDlg)
	//}}AFX_DATA_INIT

	// ﾄﾞｷｭﾒﾝﾄ名からNCﾌｧｲﾙ名を作成
	CreateNCFile(pDoc, m_strNCPath, m_strNCFileName);
	// 切削条件履歴から初期表示ﾌｧｲﾙを取得
	const	CDXFOption*	pOpt = AfxGetNCVCApp()->GetDXFOption();
	if ( pOpt->GetInitList()->GetCount() > 0 )
		::Path_Name_From_FullPath(pOpt->GetInitList()->GetHead(),
				m_strInitPath, m_strInitFileName);
	m_bNCView = pOpt->GetDxfFlag(DXFOPT_VIEW);
}

void CMakeNCDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMakeNCDlg)
	DDX_Control(pDX, IDOK, m_ctOK);
	DDX_Control(pDX, IDC_MKNC_NCFILE, m_ctNCFileName);
	DDX_Text(pDX, IDC_MKNC_NCFILE, m_strNCFileName);
	DDX_Control(pDX, IDC_MKNC_INIT, m_ctInitFileName);
	DDX_CBString(pDX, IDC_MKNC_INIT, m_strInitFileName);
	DDX_Check(pDX, IDC_MKNC_VIEW, m_bNCView);
	//}}AFX_DATA_MAP
}

/////////////////////////////////////////////////////////////////////////////
// CMakeNCDlg メッセージ ハンドラ

BOOL CMakeNCDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// ｺﾝｽﾄﾗｸﾀではできないｺﾝﾄﾛｰﾙの初期化
	CString	strTitle;
	strTitle.LoadString(m_nTitle);
	SetWindowText(strTitle);

	// 切削条件ｺﾝﾎﾞﾎﾞｯｸｽにｱｲﾃﾑ追加
	InitialMakeNCDlgComboBox(AfxGetNCVCApp()->GetDXFOption()->GetInitList(), m_ctInitFileName);
	// ﾊﾟｽ表示の最適化(shlwapi.h)
	::PathSetDlgItemPath(m_hWnd, IDC_MKNC_NCPATH,   m_strNCPath);
	::PathSetDlgItemPath(m_hWnd, IDC_MKNC_INITPATH, m_strInitPath);

	return TRUE;  // コントロールにフォーカスを設定しないとき、戻り値は TRUE となります
	              // 例外: OCX プロパティ ページの戻り値は FALSE となります
}

void CMakeNCDlg::OnOK() 
{
	m_ctOK.SetFocus();
	UpdateData();
	CString	strInitPath(m_strInitPath+m_strInitFileName),
			strNCPath(m_strNCPath+m_strNCFileName);

	// 切削条件ﾌｧｲﾙのﾁｪｯｸ(ﾌｧｲﾙ必須)
	if ( !::IsFileExist(strInitPath) ) {	// NCVC.cpp
		m_ctInitFileName.SetEditSel(0, -1);
		m_ctInitFileName.SetFocus();
		return;
	}
	// NC生成ﾌｧｲﾙのﾁｪｯｸ(あれば上書き確認)
	if ( !CheckMakeDlgFileExt(TYPE_NCD, strNCPath) ||
			!::IsFileExist(strNCPath, FALSE) ) {
		m_ctNCFileName.SetFocus();
		m_ctNCFileName.SetSel(0, -1);
		return;
	}
	m_strNCFileName = strNCPath;

	// 条件履歴更新
	CDXFOption*	pOpt = AfxGetNCVCApp()->GetDXFOption();
	pOpt->SetViewFlag(m_bNCView);
	pOpt->AddInitHistory(strInitPath);
	pOpt->SaveInitHistory();

	// CDialog::OnOK()を呼ぶとUpdateData()されm_strNCFileNameが上書きされる
	EndDialog(IDOK);
}

void CMakeNCDlg::OnMKNCFileUp() 
{
	UpdateData();
	MakeDlgFileRefer(IDS_DXF2NCD_FILE, AfxGetNCVCApp()->GetFilterString(TYPE_NCD),
			this, IDC_MKNC_NCPATH, m_strNCPath, m_strNCFileName, FALSE);
	// 文字選択状態
	m_ctNCFileName.SetFocus();
	m_ctNCFileName.SetSel(0, -1);
}

void CMakeNCDlg::OnMKNCInitUp() 
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

void CMakeNCDlg::OnMKNCInitEdit() 
{
	UpdateData();
	MakeNCDlgInitFileEdit(m_strInitPath, m_strInitFileName,
		this, IDC_MKNC_INITPATH, m_ctInitFileName);
	m_ctOK.SetFocus();
}

void CMakeNCDlg::OnSelChangeInit() 
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

void CMakeNCDlg::OnKillFocusNCFile() 
{
	UpdateData();
	MakeDlgKillFocus(m_strNCPath, m_strNCFileName, this, IDC_MKNC_NCPATH);
}

void CMakeNCDlg::OnKillFocusInit() 
{
	UpdateData();
	MakeDlgKillFocus(m_strInitPath, m_strInitFileName, this, IDC_MKNC_INITPATH);
}

/////////////////////////////////////////////////////////////////////////////
// NC生成ﾀﾞｲｱﾛｸﾞ共通関数

void SetFocusListCtrl(CListCtrl& ctListCtrl, int nIndex)
{
	if ( nIndex < 0 )
		nIndex = 0;
	ctListCtrl.SetFocus();
	ctListCtrl.SetItemState(nIndex, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
	ctListCtrl.EnsureVisible(nIndex, FALSE);
}

void CreateNCFile(const CDXFDoc* pDoc, CString& strPath, CString& strFile)
{
	// ﾄﾞｷｭﾒﾝﾄ名からNCﾌｧｲﾙ名を作成．ただし既に生成したﾌｧｲﾙ名があるならそれを採用
	CString	strNCFile(pDoc->GetNCFileName());
	if ( strNCFile.IsEmpty() ) {
		::Path_Name_From_FullPath(pDoc->GetPathName(), strPath, strFile, FALSE);
		strFile += AfxGetNCVCApp()->GetDocExtString(TYPE_NCD);
	}
	else
		::Path_Name_From_FullPath(strNCFile, strPath, strFile);
}

void CreateLayerFile(const CDXFDoc* pDoc, CString& strPath, CString& strFile)
{
	// ﾄﾞｷｭﾒﾝﾄ名からﾚｲﾔﾌｧｲﾙ名を作成
	::Path_Name_From_FullPath(pDoc->GetPathName(), strPath, strFile, FALSE);
	CString	strExt;
	VERIFY(strExt.LoadString(IDS_NCL_FILTER));
	strFile += strExt.Right(4);		// .ncl
}

BOOL CheckMakeDlgFileExt(DOCTYPE enType, CString& strFile)
{
	// 保存ﾌｧｲﾙに拡張子の追加
	if ( strFile.IsEmpty() ) {
		AfxMessageBox(IDS_ERR_FILEPATH, MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}
	TCHAR	szFileName[_MAX_FNAME],
			szExt[_MAX_EXT];
	_splitpath_s(strFile, NULL, 0, NULL, 0,
		szFileName, SIZEOF(szFileName), szExt, SIZEOF(szExt));
	if ( lstrlen(szFileName) <= 0 ) {
		AfxMessageBox(IDS_ERR_FILEPATH, MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}
	// 登録拡張子ならそのまま，でないならﾃﾞﾌｫﾙﾄ拡張子を付与
	if ( lstrlen(szExt) <= 1 ||		// 「.」のみか，それ以下
			!AfxGetNCVCApp()->GetDocTemplate(enType)->IsExtension(szExt+1) ) {	// 「.」除く
		if ( enType == TYPE_NCD )
			strFile += AfxGetNCVCApp()->GetDocExtString(TYPE_NCD);	// .ncd
		else {
			CString	strFilter;
			VERIFY(strFilter.LoadString(IDS_DXF_FILTER));
			strFile += '.' + strFilter.Left(3);	// .dxf
		}
	}

	return TRUE;
}

// CLayerData friend 関数
BOOL CheckMakeNCDlgExLayerState
	(CString& strNCFile, CEdit& ctNCFile, CListCtrl& ctLayerList, BOOL bIniCheck)
{
	CLayerData*	pLayer;
	CString		strFile;
	BOOL		bCutFlg, bMainCheck = TRUE, bFirstCheck = TRUE, bCutCheck = FALSE;

	// ﾘｽﾄｺﾝﾄﾛｰﾙからCLayerData*を取得し，明細のﾁｪｯｸ
	for ( int i=0; i<ctLayerList.GetItemCount(); i++ ) {
		pLayer = (CLayerData *)(ctLayerList.GetItemData(i));
		// 切削対象か否か
		bCutFlg = ctLayerList.GetCheck(i);
		pLayer->SetCutTargetFlg(bCutFlg);
		// ﾒｲﾝ出力ﾌｧｲﾙのﾁｪｯｸ(個別出力ﾌﾗｸﾞが設定されていないとき)
		if ( bMainCheck && bCutFlg && !pLayer->IsPartOut() ) {
			bMainCheck = FALSE;	// ﾒｲﾝ出力ﾌｧｲﾙのﾁｪｯｸは一度だけ
			if ( !CheckMakeDlgFileExt(TYPE_NCD, strNCFile) ||
					!::IsFileExist(strNCFile, FALSE) ) {
				ctNCFile.SetFocus();
				ctNCFile.SetSel(0, -1);
				return FALSE;
			}
		}
		// 以下切削対象のみ
		if ( !bCutFlg )
			continue;
		bCutCheck = TRUE;	// 切削対象あり！
		// 個別出力順の設定
		if ( pLayer->IsPartOut() ) {
			// 個別出力ﾌｧｲﾙのﾁｪｯｸ
			strFile = pLayer->GetNCFile();
			if ( !CheckMakeDlgFileExt(TYPE_NCD, strFile) ) {
				SetFocusListCtrl(ctLayerList, i);
				return FALSE;
			}
			// 拡張子付加されてればﾚｲﾔ情報に再設定
			if ( strFile.CompareNoCase(pLayer->GetNCFile()) )
				pLayer->SetNCFile(strFile);
			// 上書き確認
			if ( bFirstCheck ) {
				bFirstCheck = FALSE;	// 先頭だけ上書き判定
				if ( !::IsFileExist(strFile, FALSE) ) {
					SetFocusListCtrl(ctLayerList, i);
					return FALSE;
				}
			}
			// 個別出力が先頭になるようﾘｽﾄ№を設定
			pLayer->SetListNo(-1);
		}
		else
			pLayer->SetListNo(i);
		// 切削条件ﾌｧｲﾙのﾁｪｯｸ(CMakeNCDlgEx1のみ)
		if ( bIniCheck && !::IsFileExist(pLayer->GetInitFile()) ) {	// NCVC.cpp
			SetFocusListCtrl(ctLayerList, i);
			return FALSE;
		}
	}

	// 切削対象が１つもない
	if ( !bCutCheck ) {
		AfxMessageBox(IDS_ERR_MAKEMULTILAYER, MB_OK|MB_ICONEXCLAMATION);
		SetFocusListCtrl(ctLayerList, 0);
		return FALSE;
	}

	return TRUE;
}

CString MakeDlgFileRefer
	(int nTitle, const CString& strFilter, CDialog* pDlg, int nID,
			CString& strPath, CString& strFile, BOOL bRead)
{
	CString	strInitialFile;
	if ( !strFile.IsEmpty() )
		strInitialFile = strPath + strFile;
	if ( ::NCVC_FileDlgCommon(nTitle, strFilter, strInitialFile, strPath,
				bRead, OFN_HIDEREADONLY|OFN_PATHMUSTEXIST) == IDOK ) {
		::Path_Name_From_FullPath(strInitialFile, strPath, strFile);
		::PathSetDlgItemPath(pDlg->m_hWnd, nID, strPath);
		// ﾃﾞｰﾀの反映
		pDlg->UpdateData(FALSE);
	}
	else
		strInitialFile.Empty();
	// 選択ﾌｧｲﾙ
	return strInitialFile;
}

void MakeNCDlgInitFileEdit
	(CString& strPath, CString& strFile, CDialog* pDlg, int nID, CComboBox& ctFile)
{
	CString	strInitFile(strPath+strFile);
	CString	strCaption;
	VERIFY(strCaption.LoadString(IDS_MAKE_NCD));
	CMKNCSetup	ps(::AddDialogTitle2File(strCaption, strInitFile), strInitFile);
	if ( ps.DoModal() != IDOK )
		return;

	// ｵﾌﾟｼｮﾝの保存
	ps.GetNCMakeOption()->SaveMakeOption();
#ifdef _DEBUGOLD
	ps.GetNCMakeOption()->DbgDump();
#endif

	// 新規保存されていたら，パス表示の更新
	CString	strResult = ps.GetNCMakeOption()->GetInitFile();
	if ( strInitFile.CompareNoCase(strResult) != 0 ) {
		::Path_Name_From_FullPath(strResult, strPath, strFile);
		::PathSetDlgItemPath(pDlg->m_hWnd, nID, strPath);
		pDlg->UpdateData(FALSE);
		ctFile.SetEditSel(0, -1);
		ctFile.SetFocus();
	}
}

int MakeNCDlgSelChange
	(const CComboBox& ctCombo, HWND hWnd, int nID, CString& strPath, CString& strFile)
{
	// ｺﾝﾎﾞﾎﾞｯｸｽに設定された32ﾋﾞｯﾄ値 -> ﾌﾙﾊﾟｽ文字列へのﾎﾟｲﾝﾀ
	int		nIndex = ctCombo.GetCurSel();
	ASSERT( nIndex >= 0 );
	::Path_Name_From_FullPath((LPCTSTR)ctCombo.GetItemDataPtr(nIndex), strPath, strFile);
	::PathSetDlgItemPath(hWnd, nID, strPath);	// ﾊﾟｽの省略形
	return nIndex;
}

void MakeDlgKillFocus
	(CString& strPath, CString& strFile, CDialog* pDlg, int nID)
{
	CString		strResultPath, strResultFile;
	::Path_Name_From_FullPath(strFile, strResultPath, strResultFile);
	if ( !strResultPath.IsEmpty() ) {
		strPath = strResultPath;
		strFile = strResultFile;
		::PathSetDlgItemPath(pDlg->m_hWnd, nID, strPath);
		pDlg->UpdateData(FALSE);
	}
/*
	::Path_Name_From_FullPath(strFile, strPath, strFile);
	if ( !strPath.IsEmpty() )
		::PathSetDlgItemPath(pDlg->m_hWnd, nID, strPath);
	else {
		HWND hWnd = ::GetDlgItem(pDlg->m_hWnd, nID);
		if ( hWnd )
			::SetWindowText(hWnd, "");
	}
	pDlg->UpdateData(FALSE);
*/
}

int GetMakeNCDlgExSortColumn(UINT nID)
{
	CString		strRegKey, strEntry;
	VERIFY(strRegKey.LoadString(IDS_REGKEY_DXF));
	VERIFY(strEntry.LoadString(nID));
	return (int)(AfxGetApp()->GetProfileInt(strRegKey, strEntry, 0));
}

void SetMakeNCDlgExSortColumn(UINT nID, int nSort)
{
	CString		strRegKey, strEntry;
	VERIFY(strRegKey.LoadString(IDS_REGKEY_DXF));
	VERIFY(strEntry.LoadString(nID));
	AfxGetApp()->WriteProfileInt(strRegKey, strEntry, nSort);
}

CPoint GetMakeNCDlgExLayerListState(const CListCtrl& ctLayerList)
{
	// ﾀﾞﾌﾞﾙｸﾘｯｸされた項目を調べる
	DWORD	dwPos = ::GetMessagePos();
	CPoint	pt((int)LOWORD(dwPos), (int)HIWORD(dwPos));
	ctLayerList.ScreenToClient(&pt);
	return pt;
}

BOOL InitialMakeNCDlgComboBox(const CStringList* pList, CComboBox& ctCombo)
{
	CString		strPath, strFile;
	LPCTSTR		pszFullPath;
	// ｺﾝﾎﾞﾎﾞｯｸｽにﾌﾙﾊﾟｽ文字列へのﾎﾟｲﾝﾀを割り当てる
	for ( POSITION pos = pList->GetHeadPosition(); pos; ) {
		pszFullPath = pList->GetNext(pos);
		::Path_Name_From_FullPath(pszFullPath, strPath, strFile);
		ctCombo.SetItemDataPtr( ctCombo.AddString(strFile), (LPVOID)pszFullPath );
	}
	return TRUE;
}

BOOL InitialMakeNCDlgExLayerListCtrl(CDXFDoc* pDoc, CListCtrl& ctLayerList)
{
	// ﾘｽﾄｺﾝﾄﾛｰﾙの拡張ｽﾀｲﾙ
	DWORD	dwStyle = ctLayerList.GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT|LVS_EX_CHECKBOXES|LVS_EX_GRIDLINES;
	ctLayerList.SetExtendedStyle(dwStyle);
	// ﾚｲﾔﾘｽﾄｺﾝﾄﾛｰﾙへの登録
	LV_ITEM		lvi;
	lvi.mask = LVIF_TEXT | LVIF_PARAM;
	lvi.iSubItem = 0;
	lvi.pszText = LPSTR_TEXTCALLBACK;
	CLayerData*	pLayer;
	int		i, nCnt, nLoop = pDoc->GetLayerCnt();

	for ( i=0, nCnt=0; i<nLoop; i++ ) {
		pLayer = pDoc->GetLayerData(i);
		if ( pLayer->IsCutType() ) {
			lvi.iItem = nCnt;
			lvi.lParam = (LPARAM)pLayer;
			if ( ctLayerList.InsertItem(&lvi) < 0 ) {
				CString	strMsg;
				strMsg.Format(IDS_ERR_ADDITEM, nCnt+1);
				AfxMessageBox(strMsg, MB_OK|MB_ICONSTOP);
				return FALSE;
			}
			ctLayerList.SetCheck(nCnt++, pLayer->IsViewLayer());
		}
	}
	return TRUE;
}
