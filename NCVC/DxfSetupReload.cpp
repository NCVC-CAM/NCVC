// DxfSetupReload.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "DXFDoc.h"
#include "DxfSetupReload.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

BEGIN_MESSAGE_MAP(CDxfSetupReload, CDialog)
	//{{AFX_MSG_MAP(CDxfSetupReload)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDxfSetupReload ダイアログ

CDxfSetupReload::CDxfSetupReload(CWnd* pParent)
	: CDialog(CDxfSetupReload::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDxfSetupReload)
		// メモ - ClassWizard はこの位置にマッピング用のマクロを追加または削除します。
	//}}AFX_DATA_INIT
}

void CDxfSetupReload::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDxfSetupReload)
		// メモ - ClassWizard はこの位置にマッピング用のマクロを追加または削除します。
	//}}AFX_DATA_MAP
}

/////////////////////////////////////////////////////////////////////////////
// CDxfSetupReload メッセージ ハンドラ

BOOL CDxfSetupReload::OnInitDialog() 
{
	CDialog::OnInitDialog();

	// 標準ﾘｽﾄﾎﾞｯｸｽをｻﾌﾞｸﾗｽ化
	m_ctReloadList.SubclassDlgItem(IDC_DXF_RELOADLIST, this);

	CDXFDoc*	pDoc;
	CWnd*		pStatic = GetDlgItem(IDC_DXF_RELOADLIST_ST);
	CString		strPath;
	ASSERT( pStatic );

	m_ctReloadList.SendMessage(WM_SETREDRAW, FALSE);
	for ( POSITION pos=AfxGetNCVCApp()->GetDocTemplate(TYPE_DXF)->GetFirstDocPosition(); pos; ) {
		pDoc = (CDXFDoc *)(AfxGetNCVCApp()->GetDocTemplate(TYPE_DXF)->GetNextDoc(pos));
		// 一旦ｽﾀﾃｨｯｸｺﾝﾄﾛｰﾙにｾｯﾄしてﾊﾟｽ表示の最適化(shlwapi.h)
		::PathSetDlgItemPath(m_hWnd, IDC_DXF_RELOADLIST_ST, pDoc->GetPathName());
		// そのﾃｷｽﾄを取得してﾘｽﾄｺﾝﾄﾛｰﾙにｾｯﾄ
		pStatic->GetWindowText(strPath);
		m_ctReloadList.SetCheck(m_ctReloadList.AddString(strPath), pDoc->IsReload());
	}
	m_ctReloadList.SendMessage(WM_SETREDRAW, TRUE);
	m_ctReloadList.SetCurSel(0);

	return TRUE;  // コントロールにフォーカスを設定しないとき、戻り値は TRUE となります
	              // 例外: OCX プロパティ ページの戻り値は FALSE となります
}

void CDxfSetupReload::OnOK() 
{
	int		nCnt = 0;
	CDXFDoc*	pDoc;
	for ( POSITION pos=AfxGetNCVCApp()->GetDocTemplate(TYPE_DXF)->GetFirstDocPosition(); pos; ) {
		pDoc = (CDXFDoc *)(AfxGetNCVCApp()->GetDocTemplate(TYPE_DXF)->GetNextDoc(pos));
		pDoc->SetReload( m_ctReloadList.GetCheck(nCnt++)==1 ? TRUE : FALSE );
	}

	CDialog::OnOK();
}
