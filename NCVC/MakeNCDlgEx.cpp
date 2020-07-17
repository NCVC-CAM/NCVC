// MakeNCDlgEx.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "DXFDoc.h"
#include "MakeNCDlg.h"
#include "MakeNCDlgEx.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

BEGIN_MESSAGE_MAP(CMakeNCDlgEx, CPropertySheet)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMakeNCDlgEx

CMakeNCDlgEx::CMakeNCDlgEx(UINT nID, CDXFDoc* pDoc) : CPropertySheet(nID)
{
	m_psh.dwFlags &= ~PSH_HASHELP;

	m_nID  = nID;
	m_pDoc = pDoc;
	AddPage(&m_dlg1);	// 出力設定
	AddPage(&m_dlg2);	// ﾚｲﾔ明細

	// ﾀﾞｲｱﾛｸﾞ共通項の設定
	CString	strPath, strFile, strExt;
	CDXFOption*	pOpt = AfxGetNCVCApp()->GetDXFOption();
	// ﾄﾞｷｭﾒﾝﾄ名からNCﾌｧｲﾙ名を作成
	CreateNCFile(pDoc, strPath, strFile);	// MakeNCDlg.cpp
	m_strNCFileName = strPath + strFile;
	// 切削条件ﾌｧｲﾙ
	if ( pOpt->GetInitList(NCMAKEMILL)->GetCount() > 0 )
		m_strInitFileName = pOpt->GetInitList(NCMAKEMILL)->GetHead();
	// ﾚｲﾔ情報のｾｯﾄ(NCﾌｧｲﾙと同じﾊﾟｽ)
	::Path_Name_From_FullPath(m_strNCFileName, strPath, strFile, FALSE);
	VERIFY(strExt.LoadString(IDS_NCL_FILTER));
	strPath += strFile + strExt.Right(4);		// .ncl
	if ( ::IsFileExist(strPath, TRUE, FALSE) ) {
		m_strLayerToInitFileName = strPath;
		pOpt->AddInitHistory(NCMAKELAYER, strPath);
	}
}

CMakeNCDlgEx::~CMakeNCDlgEx()
{
}

/////////////////////////////////////////////////////////////////////////////
// CMakeNCDlgEx メッセージ ハンドラ
