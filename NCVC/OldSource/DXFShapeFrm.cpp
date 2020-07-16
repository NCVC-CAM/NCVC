// DXFShapeFrm.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "DXFdata.h"
// ｶｽﾀﾑｺﾏﾝﾄﾞﾙｰﾃｨﾝｸﾞ
#include "DXFDoc.h"
//
#include "DXFShapeFrm.h"
#include "DXFShapeView.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
extern	CMagaDbg	g_dbg;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDXFShapeFrm

IMPLEMENT_DYNCREATE(CDXFShapeFrm, CFrameWnd)

BEGIN_MESSAGE_MAP(CDXFShapeFrm, CFrameWnd)
	//{{AFX_MSG_MAP(CDXFShapeFrm)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CDXFShapeFrm::CDXFShapeFrm()
{
}

CDXFShapeFrm::~CDXFShapeFrm()
{
}

/////////////////////////////////////////////////////////////////////////////
// CDXFShapeFrm クラスのオーバライド関数

BOOL CDXFShapeFrm::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext) 
{
	// CDXFShapeFrm管理下のCDXFShapeViewを作成
	pContext->m_pNewViewClass = RUNTIME_CLASS(CDXFShapeView);
	return CFrameWnd::OnCreateClient(lpcs, pContext);
}

BOOL CDXFShapeFrm::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) 
{
//	return CFrameWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
/*
	ｽﾌﾟﾘｯﾀｳｨﾝﾄﾞｳによる複数ﾋﾞｭｰで
	ﾌｫｰｶｽが移っても正常動作させるための
	ｶｽﾀﾑｺﾏﾝﾄﾞﾙｰﾃｨﾝｸﾞ
*/
	if ( CFrameWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo) )
		return TRUE;
	CDXFDoc* pDoc = (CDXFDoc *)GetActiveDocument();
	if ( !pDoc )
		return FALSE;
	// CDXFShapeView以外(CDXFView)にｺﾏﾝﾄﾞﾙｰﾃｨﾝｸﾞ
	return pDoc->RouteCmdToAllViews(GetActiveView(), nID, nCode, pExtra, pHandlerInfo);
}

/////////////////////////////////////////////////////////////////////////////
// CDXFShapeFrm クラスの診断

#ifdef _DEBUG
void CDXFShapeFrm::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CDXFShapeFrm::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CDXFShapeFrm メッセージ ハンドラ

int CDXFShapeFrm::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	// 形状処理用ﾂｰﾙﾊﾞｰの作成
	m_wndToolBar.Create(this, WS_CHILD|WS_VISIBLE|CBRS_TOP|CBRS_TOOLTIPS);
	m_wndToolBar.LoadToolBar(IDR_SHAPE);

	return 0;
}
