// NCChild.cpp : CNCChild クラスの動作の定義を行います。
//

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "NCdata.h"
#include "NCChild.h"
// ｶｽﾀﾑｺﾏﾝﾄﾞﾙｰﾃｨﾝｸﾞ
#include "NCDoc.h"
// ｽﾌﾟﾘｯﾀｳｨﾝﾄﾞｳ作成
#include "NCViewTab.h"
#include "NCListView.h"
#include "NCInfoTab.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

using namespace boost;

static const UINT g_nIndicators[] =
{
	ID_NCDST_LINENO,
	ID_NCDST_COORDINATES,
	ID_INDICATOR_FACTOR,
	ID_SEPARATOR	// ﾌﾟﾛｸﾞﾚｽﾊﾞｰ領域
};
static	const	int		PROGRESS_INDEX = 3;

/////////////////////////////////////////////////////////////////////////////
// CNCChild

IMPLEMENT_DYNCREATE(CNCChild, CMDIChildWnd)

BEGIN_MESSAGE_MAP(CNCChild, CMDIChildWnd)
	//{{AFX_MSG_MAP(CNCChild)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_MDIACTIVATE()
	ON_WM_CLOSE()
	//}}AFX_MSG_MAP
	ON_WM_SYSCOMMAND()
	// ﾌｧｲﾙ変更通知 from DocBase.cpp
	ON_MESSAGE(WM_USERFILECHANGENOTIFY, OnUserFileChangeNotify)
END_MESSAGE_MAP()

BEGIN_MESSAGE_MAP(CNCFrameSplit, CSplitterWnd)
	ON_WM_DESTROY()
	ON_WM_LBUTTONDBLCLK()
	// ﾕｰｻﾞｲﾆｼｬﾙ処理
	ON_MESSAGE(WM_USERINITIALUPDATE, OnUserInitialUpdate)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNCChild クラスの構築/消滅

CNCChild::CNCChild()
{
}

CNCChild::~CNCChild()
{
}

/////////////////////////////////////////////////////////////////////////////
// CNCChild クラスのオーバライド関数

BOOL CNCChild::OnCreateClient(LPCREATESTRUCT /*lpcs*/, CCreateContext* pContext)
{
	//	静的ｽﾌﾟﾘｯﾀ作成
	if ( !m_wndSplitter1.CreateStatic(this, 1, 2) ||
		 !m_wndSplitter2.CreateStatic(&m_wndSplitter1, 2, 1,
							WS_CHILD|WS_VISIBLE, m_wndSplitter1.IdFromRowCol(0, 0)) ||
		 !m_wndSplitter2.CreateView(0, 0, RUNTIME_CLASS(CNCInfoTab),  CSize(0, 0), pContext) ||
		 !m_wndSplitter2.CreateView(1, 0, RUNTIME_CLASS(CNCListView), CSize(0, 0), pContext) ||
		 !m_wndSplitter1.CreateView(0, 1, RUNTIME_CLASS(CNCViewTab),  CSize(0, 0), pContext) )
		return FALSE;

	return TRUE;
}

void CNCChild::ActivateFrame(int nCmdShow) 
{
#ifdef _DEBUG
	g_dbg.printf("CNCChild::ActivateFrame() Call");
#endif
	CMDIChildWnd::ActivateFrame(CChildBase::ActivateFrame(nCmdShow));
}

BOOL CNCChild::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) 
{
//	return CMDIChildWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
/*
	ｽﾌﾟﾘｯﾀｳｨﾝﾄﾞｳによる複数ﾋﾞｭｰで
	ﾌｫｰｶｽが移っても正常動作させるための
	ｶｽﾀﾑｺﾏﾝﾄﾞﾙｰﾃｨﾝｸﾞ
*/
	if ( CMDIChildWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo) )
		return TRUE;
	CNCDoc* pDoc = static_cast<CNCDoc *>(GetActiveDocument());
	if ( !pDoc )
		return FALSE;
	CMDIChildWnd*	pChild = AfxGetNCVCMainWnd()->MDIGetActive();
	return pDoc->RouteCmdToAllViews(pChild ? pChild->GetActiveView() : NULL,
		nID, nCode, pExtra, pHandlerInfo);
}

/////////////////////////////////////////////////////////////////////////////
// CNCChild クラスの診断

#ifdef _DEBUG
void CNCChild::AssertValid() const
{
	CMDIChildWnd::AssertValid();
}

void CNCChild::Dump(CDumpContext& dc) const
{
	CMDIChildWnd::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CNCChild クラスのﾒﾝﾊﾞ関数

void CNCChild::SetWorkRect(BOOL bHide, CRect3D& rc)
{
	CNCDoc* pDoc = static_cast<CNCDoc *>(GetActiveDocument());
	if ( pDoc )
		pDoc->SetWorkRect(bHide, rc);
}

void CNCChild::SetJumpList(int nJump)
{
	GetListView()->SetJumpList(nJump);
}

void CNCChild::SetFindList(int nUpDown, const CString& strFind)
{
	GetListView()->SetFindList(nUpDown, strFind);
}

void CNCChild::SetFactorInfo(ENNCVPLANE enView, double dFactor)
{
	static	LPCTSTR	szView[] = {
		"(XYZ)", "(XY)", "(XZ)", "(YZ)"
	};
	CString		str;
	str.Format(ID_INDICATOR_FACTOR_F, szView[enView], dFactor);
	m_wndStatusBar.SetPaneText(
		m_wndStatusBar.CommandToIndex(ID_INDICATOR_FACTOR), str);
}

void CNCChild::OnUpdateStatusLineNo
	(int nPos, int nSize, variant<CNCblock*, CNCdata*>& vSelect)
{
	CString		strInfo;
	CNCdata*	pData = NULL;

	strInfo.Format(ID_NCDST_LINENO_F, nPos, nSize);
	m_wndStatusBar.SetPaneText(m_wndStatusBar.CommandToIndex(ID_NCDST_LINENO), strInfo);
	strInfo.Empty();

	if ( vSelect.which() == 0 ) {
		CNCblock*	pBlock = get<CNCblock*>(vSelect);
		if ( pBlock ) {
			UINT	nError = pBlock->GetNCBlkErrorCode();
			if ( nError > 0 ) {
				if ( nError>=NCERROR_RES_MIN && nError<=NCERROR_RES_MAX )
					strInfo.LoadString(nError);
				else
					VERIFY(strInfo.LoadString(IDS_ERR_NCBLK_UNKNOWN));
			}
			else
				pData = pBlock->GetBlockToNCdata();
		}
		else
			VERIFY(strInfo.LoadString(ID_NCDST_COORDINATES));
	}
	else
		pData = get<CNCdata*>(vSelect);

	if ( pData ) {
		CPoint3D	pt(pData->GetEndCorrectPoint());
		strInfo.Format(ID_NCDST_COORDINATES_F, pt.x, pt.y, pt.z);
	}
	else if ( strInfo.IsEmpty() )
		VERIFY(strInfo.LoadString(ID_NCDST_COORDINATES));

	m_wndStatusBar.SetPaneText(m_wndStatusBar.CommandToIndex(ID_NCDST_COORDINATES), strInfo);
}

/////////////////////////////////////////////////////////////////////////////
// CNCChild クラスのメッセージハンドラ

int CNCChild::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	// ｽﾃｰﾀｽﾊﾞｰ作成
	if (!m_wndStatusBar.Create(this, WS_CHILD|WS_VISIBLE|CBRS_BOTTOM, ID_NC_STATUSBAR) ||
		!m_wndStatusBar.SetIndicators(g_nIndicators, SIZEOF(g_nIndicators)) ) {
		TRACE0("Failed to NC child status bar\n");
		return -1;      // 作成に失敗
	}

	// 各ﾍﾟｲﾝの調整
	m_wndSplitter1.PostMessage(WM_USERINITIALUPDATE, TRUE);

	return 0;
}

void CNCChild::OnClose() 
{
	AfxGetNCVCMainWnd()->AllModelessDlg_PostSwitchMessage();
	CMDIChildWnd::OnClose();
}

void CNCChild::OnSize(UINT nType, int cx, int cy) 
{
	CMDIChildWnd::OnSize(nType, cx, cy);

	if ( cx<=0 || cy<=0 || !::IsWindow(m_wndStatusBar.m_hWnd) )
		return;

	UINT	nID, nStyle;
	int		nWidth=0, nWidth1;
	// ﾌﾟﾛｸﾞﾚｽ領域の計算
	for ( int i=0; i<SIZEOF(g_nIndicators)-1; i++ ) {
		m_wndStatusBar.GetPaneInfo(
			m_wndStatusBar.CommandToIndex(g_nIndicators[i]), nID, nStyle, nWidth1);
		nWidth += nWidth1;
	}

	m_wndStatusBar.ChangeProgressSize(PROGRESS_INDEX, cx - nWidth - 2);
}

void CNCChild::OnMDIActivate(BOOL bActivate, CWnd* pActivateWnd, CWnd* pDeactivateWnd) 
{
	CMDIChildWnd::OnMDIActivate(bActivate, pActivateWnd, pDeactivateWnd);
	DBGBOOL(g_dbg, "CNCChild::bActivate", bActivate);
	CChildBase::OnMDIActivate(this, bActivate);
	if ( !bActivate )
		GetMainView()->OnUserTraceStop();
}

LRESULT CNCChild::OnUserFileChangeNotify(WPARAM, LPARAM)
{
	CChildBase::OnUserFileChangeNotify(this);
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CNCFrameSplit クラスのメッセージハンドラ

LRESULT CNCFrameSplit::OnUserInitialUpdate(WPARAM wParam, LPARAM)
{
	// 各ﾍﾟｲﾝのｻｲｽﾞを取得
	CString	strRegKey, strEntry;
	VERIFY(strRegKey.LoadString(IDS_REGKEY_NC));

	// 40文字分かｳｨﾝﾄﾞｳｻｲｽﾞの1/4，どちらか小さい方を左側のﾍﾟｲﾝに
	CRect	rc;
	GetParentFrame()->GetClientRect(&rc);
	int		nInfo,
			n1 = AfxGetNCVCMainWnd()->GetNCTextWidth() * 40,
			n2 = rc.Width() / 4;
	nInfo = min(n1, n2);
	if ( (BOOL)wParam ) {
		strEntry.Format(IDS_REG_NCV_FRAMEPANESIZE, 1);
		nInfo = AfxGetApp()->GetProfileInt(strRegKey, strEntry, nInfo);
	}
	SetColumnInfo(0, nInfo, 0);
	// 10行分かｳｨﾝﾄﾞｳｻｲｽﾞの1/3，どちらか小さい方を上側のﾍﾟｲﾝに
	n1 = AfxGetNCVCMainWnd()->GetNCTextHeight() * 10;
	n2 = rc.Height() / 3;
	nInfo = min(n1, n2);
	if ( (BOOL)wParam ) {
		strEntry.Format(IDS_REG_NCV_FRAMEPANESIZE, 2);
		nInfo = AfxGetApp()->GetProfileInt(strRegKey, strEntry, nInfo);
	}
	((CSplitterWnd *)GetPane(0, 0))->SetRowInfo(0, nInfo, 0);

	RecalcLayout();
	SetActivePane(0, 1);	// ｸﾞﾗﾌｨｯｸｽ表示をｱｸﾃｨﾌﾞに

	return 0;
}

void CNCFrameSplit::OnDestroy() 
{
	int		nInfo, nMin;
	CString	strRegKey, strEntry;
	VERIFY(strRegKey.LoadString(IDS_REGKEY_NC));

	if ( GetParent()->IsKindOf(RUNTIME_CLASS(CNCChild)) ) {
		GetColumnInfo(0, nInfo, nMin);
		if ( nInfo > 0 ) {
			strEntry.Format(IDS_REG_NCV_FRAMEPANESIZE, 1);
			AfxGetApp()->WriteProfileInt(strRegKey, strEntry, nInfo);
		}
	}
	else {
		GetRowInfo(0, nInfo, nMin);
		if ( nInfo > 0 ) {
			strEntry.Format(IDS_REG_NCV_FRAMEPANESIZE, 2);
			AfxGetApp()->WriteProfileInt(strRegKey, strEntry, nInfo);
		}
	}
}

void CNCFrameSplit::OnLButtonDblClk(UINT, CPoint) 
{
	// 各ﾍﾟｲﾝを初期状態に戻す
	CWnd*	pWnd = GetParent();
	if ( pWnd->IsKindOf(RUNTIME_CLASS(CNCChild)) )
		OnUserInitialUpdate(FALSE, 0);
	else
		pWnd->PostMessage(WM_LBUTTONDBLCLK);
}
