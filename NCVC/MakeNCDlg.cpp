// MakeNCD.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "Layer.h"
#include "DXFDoc.h"
#include "3dModelDoc.h"
#include "MakeNCSetup.h"
#include "MakeLatheSetup.h"
#include "MakeWireSetup.h"
#include "MakeNurbsSetup.h"
#include "MakeNCDlg.h"
#include "MakeBindOptDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CMakeNCDlg, CDialog)
	//{{AFX_MSG_MAP(CMakeNCDlg)
	ON_BN_CLICKED(IDC_MKNC_NCFILEUP, &CMakeNCDlg::OnMKNCFileUp)
	ON_BN_CLICKED(IDC_MKNC_INITUP, &CMakeNCDlg::OnMKNCInitUp)
	ON_BN_CLICKED(IDC_MKNC_INITED, &CMakeNCDlg::OnMKNCInitEdit)
	ON_BN_CLICKED(IDC_MKNC_BINDOPT, &CMakeNCDlg::OnBindOpt)
	ON_CBN_SELCHANGE(IDC_MKNC_INIT, &CMakeNCDlg::OnSelChangeInit)
	ON_CBN_KILLFOCUS(IDC_MKNC_INIT, &CMakeNCDlg::OnKillFocusInit)
	ON_EN_KILLFOCUS(IDC_MKNC_NCFILE, &CMakeNCDlg::OnKillFocusNCFile)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMakeNCDlg ダイアログ

CMakeNCDlg::CMakeNCDlg(UINT nTitle, NCMAKETYPE enType, CDXFDoc* pDoc)
	: CDialog(CMakeNCDlg::IDD, NULL)
{
	CommonConstructor(nTitle, enType, pDoc);
	// ﾄﾞｷｭﾒﾝﾄ名からNCﾌｧｲﾙ名を作成
	CreateNCFile(pDoc, m_strNCPath, m_strNCFileName);
}

CMakeNCDlg::CMakeNCDlg(UINT nTitle, NCMAKETYPE enType, C3dModelDoc* pDoc)
	: CDialog(CMakeNCDlg::IDD, NULL)
{
	CommonConstructor(nTitle, enType, pDoc);
	// ﾄﾞｷｭﾒﾝﾄ名からNCﾌｧｲﾙ名を作成
	CreateNCFile(pDoc, m_nTitle-IDS_MAKENCD_TITLE_ROUGH, m_strNCPath, m_strNCFileName);
}

void CMakeNCDlg::CommonConstructor(UINT nTitle, NCMAKETYPE enType, CDocBase* pDoc)
{
	m_nTitle = nTitle;
	m_enType = enType;
	m_pDoc = pDoc;
	// 切削条件履歴から初期表示ﾌｧｲﾙを取得
	const CDXFOption*  pOpt = AfxGetNCVCApp()->GetDXFOption();
	if ( pOpt->GetInitList(m_enType)->GetCount() > 0 )
		::Path_Name_From_FullPath(pOpt->GetInitList(m_enType)->GetHead(), m_strInitPath, m_strInitFileName);
	m_bNCView = pOpt->GetDxfOptFlg(DXFOPT_VIEW);
}

void CMakeNCDlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMakeNCDlg)
	DDX_Control(pDX, IDOK, m_ctOK);
	DDX_Control(pDX, IDC_MKNC_BINDOPT, m_ctBindOpt);
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
	__super::OnInitDialog();
	
	// ｺﾝｽﾄﾗｸﾀではできないｺﾝﾄﾛｰﾙの初期化
	CString	strTitle;
	VERIFY(strTitle.LoadString(m_nTitle));
	SetWindowText(strTitle);

	// 切削条件ｺﾝﾎﾞﾎﾞｯｸｽにｱｲﾃﾑ追加
	InitialMakeNCDlgComboBox(
		AfxGetNCVCApp()->GetDXFOption()->GetInitList(m_enType),
		m_ctInitFileName);
	// ﾊﾟｽ表示の最適化(shlwapi.h)
	::PathSetDlgItemPath(m_hWnd, IDC_MKNC_NCPATH,   m_strNCPath);
	::PathSetDlgItemPath(m_hWnd, IDC_MKNC_INITPATH, m_strInitPath);

	// 統合ｵﾌﾟｼｮﾝの有効化
	if ( m_pDoc->IsDocFlag(DXFDOC_BINDPARENT) )
		m_ctBindOpt.ShowWindow(SW_SHOW);

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
	// Nurbs曲面切削のチェック
	if ( m_nTitle >= IDS_MAKENCD_TITLE_ROUGH ) {
		UINT	id = m_nTitle - IDS_MAKENCD_TITLE_ROUGH;
		C3dModelDoc* pDoc = static_cast<C3dModelDoc*>(m_pDoc);
		if ( strNCPath.CompareNoCase(pDoc->Get3dOption()->Get3dStr(1-id)) == 0 ) {	// 逆のファイル名をチェック
			// ファイル名が同じ警告
			if ( AfxMessageBox(IDS_ANA_ROUGH+id, MB_YESNO|MB_ICONQUESTION) != IDYES )
				m_ctNCFileName.SetFocus();
				m_ctNCFileName.SetSel(0, -1);
				return;
		}
	}

	m_strInitFileName = strInitPath;
	m_strNCFileName   = strNCPath;

	// __super::OnOK()を呼ぶとUpdateData()されm_strNCFileNameが上書きされる
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
	VERIFY(strFilter.LoadString(m_enType+IDS_NCIM_FILTER));
	MakeDlgFileRefer(IDS_OPTION_INIT, strFilter, this, IDC_MKNC_INITPATH,
			m_strInitPath, m_strInitFileName, TRUE);
	// 文字選択状態
	m_ctInitFileName.SetEditSel(0, -1);
	m_ctInitFileName.SetFocus();
}

void CMakeNCDlg::OnMKNCInitEdit() 
{
	CString	strInitFile, strCaption, strResult;

	if ( m_enType != NCMAKEMILL ) {
		strInitFile = m_strInitPath + m_strInitFileName;
		VERIFY(strCaption.LoadString(IDS_MAKE_NCD));
	}

	UpdateData();

	switch ( m_enType ) {
	case NCMAKEMILL:
		// 共通化した MakeNCDlgInitFileEdit() 関数内で完結
		MakeNCDlgInitFileEdit(m_strInitPath, m_strInitFileName,
			this, IDC_MKNC_INITPATH, m_ctInitFileName);
		break;

	case NCMAKELATHE:
		VERIFY(strResult.LoadString(IDCV_LATHE));
		{
			// 処理手順は MakeNCDlgInitFileEdit() と同じ
			CMakeLatheSetup	ps(::AddDialogTitle2File(strCaption, strInitFile)+strResult, strInitFile);
			if ( ps.DoModal() != IDOK )
				return;
			ps.GetNCMakeOption()->SaveMakeOption();
#ifdef _DEBUGOLD
			ps.GetNCMakeOption()->DbgDump();
#endif
			strResult = ps.GetNCMakeOption()->GetInitFile();
		}
		break;

	case NCMAKEWIRE:
		VERIFY(strResult.LoadString(IDCV_WIRE));
		{
			CMakeWireSetup	ps(::AddDialogTitle2File(strCaption, strInitFile)+strResult, strInitFile);
			if ( ps.DoModal() != IDOK )
				return;
			ps.GetNCMakeOption()->SaveMakeOption();
#ifdef _DEBUGOLD
			ps.GetNCMakeOption()->DbgDump();
#endif
			strResult = ps.GetNCMakeOption()->GetInitFile();
		}
		break;
	case NCMAKENURBS:
		VERIFY(strResult.LoadString(IDCV_NURBS));
		{
			CMakeNurbsSetup	ps(::AddDialogTitle2File(strCaption, strInitFile)+strResult, strInitFile);
			if ( ps.DoModal() != IDOK )
				return;
			ps.GetNCMakeOption()->SaveMakeOption();
#ifdef _DEBUGOLD
			ps.GetNCMakeOption()->DbgDump();
#endif
			strResult = ps.GetNCMakeOption()->GetInitFile();
		}
		break;
	}

	if ( m_enType != NCMAKEMILL ) {
		if ( strInitFile.CompareNoCase(strResult) != 0 ) {
			CString	strPath, strFile;
			::Path_Name_From_FullPath(strResult, strPath, strFile);
			::PathSetDlgItemPath(m_hWnd, IDC_MKNC_INITPATH, strPath);
			m_strInitPath = strPath;
			m_strInitFileName = strFile;
			UpdateData(FALSE);
			m_ctInitFileName.SetEditSel(0, -1);
			m_ctInitFileName.SetFocus();
		}
	}

	m_ctOK.SetFocus();
}

void CMakeNCDlg::OnBindOpt()
{
	CMakeBindOptDlg	dlg;
	dlg.DoModal();
}

void CMakeNCDlg::OnSelChangeInit() 
{
	int nIndex = MakeNCDlgSelChange(m_ctInitFileName, m_hWnd, IDC_MKNC_INITPATH,
			m_strInitPath, m_strInitFileName);
	// 履歴ﾌｧｲﾙの存在ﾁｪｯｸ
	CString	strFullPath(m_strInitPath+m_strInitFileName);
	if ( !::IsFileExist(strFullPath) ) {
		m_ctInitFileName.DeleteString(nIndex);
		// 履歴削除
		AfxGetNCVCApp()->GetDXFOption()->DelInitHistory(m_enType, strFullPath);
	}
}

void CMakeNCDlg::OnKillFocusNCFile() 
{
	UpdateData();
	if ( !m_strNCFileName.IsEmpty() )
		MakeDlgKillFocus(m_strNCPath, m_strNCFileName, this, IDC_MKNC_NCPATH);
}

void CMakeNCDlg::OnKillFocusInit() 
{
	UpdateData();
	if ( !m_strInitFileName.IsEmpty() )
		MakeDlgKillFocus(m_strInitPath, m_strInitFileName, this, IDC_MKNC_INITPATH);
}

/////////////////////////////////////////////////////////////////////////////
// NC生成ﾀﾞｲｱﾛｸﾞ共通関数

void CreateNCFile(CDXFDoc* pDoc, CString& strPath, CString& strFile)
{
	// ドキュメント名からＮＣファイル名を作成
	//　ただし既に生成したファイル名があるならそれを採用
	CString	strNCFile(pDoc->GetNCFileName());
	if ( strNCFile.IsEmpty() ) {
		strNCFile = pDoc->GetPathName();
		if ( strNCFile.IsEmpty() ) {	// from Bind
			if ( pDoc->GetBindInfoCnt() >= 0 ) {
				strNCFile = pDoc->GetBindInfoData(0)->pDoc->GetPathName();
				::Path_Name_From_FullPath(strNCFile, strPath, strFile, FALSE);
				strFile.Empty();
			}
		}
		else {
			::Path_Name_From_FullPath(strNCFile, strPath, strFile, FALSE);
			strFile += AfxGetNCVCApp()->GetDocTemplate(TYPE_NCD)->GetUserDefaultExt();
		}
	}
	else {
		::Path_Name_From_FullPath(strNCFile, strPath, strFile);
	}
}

void CreateNCFile(C3dModelDoc* pDoc, UINT id, CString& strPath, CString& strFile)
{
	static LPCTSTR	szSuffix[] = {
		"_Scan", "_Contour"
	};
	// 3dOptionに保存したファイル名からＮＣファイル名を作成
	// 新規の場合のみサフィックスを付与
	CString	strNCFile(pDoc->Get3dOption()->Get3dStr(id));
	if ( strNCFile.IsEmpty() ) {
		strNCFile = pDoc->GetPathName();
		::Path_Name_From_FullPath(strNCFile, strPath, strFile, FALSE);
		strFile += szSuffix[id];
		strFile += AfxGetNCVCApp()->GetDocTemplate(TYPE_NCD)->GetUserDefaultExt();
		return;
	}
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
	_tsplitpath_s(strFile, NULL, 0, NULL, 0,
		szFileName, SIZEOF(szFileName), szExt, SIZEOF(szExt));
	if ( lstrlen(szFileName) <= 0 ) {
		AfxMessageBox(IDS_ERR_FILEPATH, MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}
	// 登録拡張子ならそのまま，でないならﾃﾞﾌｫﾙﾄ拡張子を付与
	if ( lstrlen(szExt) <= 1 ||		// 「.」のみか，それ以下
			!AfxGetNCVCApp()->GetDocTemplate(enType)->IsExtension(szExt+1) ) {	// 「.」除く
		if ( enType == TYPE_NCD ) {
			strFile += AfxGetNCVCApp()->GetDocTemplate(TYPE_NCD)->GetUserDefaultExt();
		}
		else {
			CString	strFilter;
			VERIFY(strFilter.LoadString(IDS_DXF_FILTER));
			strFile += '.' + strFilter.Left(3);	// .dxf
		}
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
	if ( ::NCVC_FileDlgCommon(nTitle, strFilter, FALSE, strInitialFile, strPath,
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
	CString	strInitFile(strPath+strFile),
			strCaption;
	VERIFY(strCaption.LoadString(IDS_MAKE_NCD));
	CMakeNCSetup	ps(::AddDialogTitle2File(strCaption, strInitFile), strInitFile);
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
}

BOOL InitialMakeNCDlgComboBox(const CStringList* pList, CComboBox& ctCombo)
{
	CString	strPath, strFile;
	LPCTSTR	pszFullPath;
	// ｺﾝﾎﾞﾎﾞｯｸｽにﾌﾙﾊﾟｽ文字列へのﾎﾟｲﾝﾀを割り当てる
	PLIST_FOREACH(pszFullPath, pList)
		::Path_Name_From_FullPath(pszFullPath, strPath, strFile);
		ctCombo.SetItemDataPtr( ctCombo.AddString(strFile), (LPVOID)pszFullPath );
	END_FOREACH

	return TRUE;
}
