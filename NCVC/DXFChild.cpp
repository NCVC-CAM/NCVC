// DXFChild.cpp : �C���v�������e�[�V���� �t�@�C��
//

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "DXFChild.h"
#include "DXFDoc.h"
#include "DXFView.h"
#include "DXFShapeView.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

static const UINT g_nIndicators[] =
{
	ID_DXFST_DATAINFO,
	ID_DXFST_MOUSE,
	ID_INDICATOR_FACTOR
};

/////////////////////////////////////////////////////////////////////////////
// CDXFChild

IMPLEMENT_DYNCREATE(CDXFChild, CMDIChildWnd)

BEGIN_MESSAGE_MAP(CDXFChild, CChildBase)
	//{{AFX_MSG_MAP(CDXFChild)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_MDIACTIVATE()
	//}}AFX_MSG_MAP
	// հ�޲Ƽ�ُ���
	ON_MESSAGE(WM_USERINITIALUPDATE, &CDXFChild::OnUserInitialUpdate)
END_MESSAGE_MAP()

BEGIN_MESSAGE_MAP(CDXFFrameSplit, CSplitterWnd)
	ON_WM_LBUTTONDBLCLK()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDXFChild �N���X�̍\�z/����

CDXFChild::CDXFChild()
{
}

/////////////////////////////////////////////////////////////////////////////
// CDXFChild �N���X�̃I�[�o���C�h�֐�

BOOL CDXFChild::OnCreateClient(LPCREATESTRUCT /*lpcs*/, CCreateContext* pContext) 
{
	CRect	rc;
	GetParentFrame()->GetClientRect(rc);
	//	�ÓI���د��쐬
	if ( !m_wndSplitter.CreateStatic(this, 1, 2) ||
			!m_wndSplitter.CreateView(0, 0, RUNTIME_CLASS(CDXFView),
				CSize(rc.Width(), 0), pContext) ||
			!m_wndSplitter.CreateView(0, 1, RUNTIME_CLASS(CDXFShapeView),
				CSize(0, 0), pContext) )
		return FALSE;

	return TRUE;
}

BOOL CDXFChild::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) 
{
//	return __super::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
/*
	���د�����޳�ɂ�镡���ޭ���
	̫������ڂ��Ă����퓮�삳���邽�߂�
	���Ѻ����ٰèݸ�
*/
	if ( __super::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo) )
		return TRUE;
	CDXFDoc* pDoc = static_cast<CDXFDoc *>(GetActiveDocument());
	if ( !pDoc )
		return FALSE;
	CMDIChildWnd*	pChild = AfxGetNCVCMainWnd()->MDIGetActive();
	CView*	pView;
	if ( pDoc->IsDocFlag(DXFDOC_BIND) ) {
		pDoc = pDoc->GetBindParentDoc();
		if ( !pDoc )
			return FALSE;
		pView = NULL;
	}
	else
		pView = pChild ? pChild->GetActiveView() : NULL;
	return pDoc->RouteCmdToAllViews(pView, nID, nCode, pExtra, pHandlerInfo);
}

/////////////////////////////////////////////////////////////////////////////
// CDXFChild ���ފ֐�

void CDXFChild::SetDataInfo(const CDXFDoc* pDoc)
{
	CString		str;
	str.Format(ID_DXFST_DATAINFO_F,
				pDoc->GetDxfDataCnt(DXFLINEDATA),
				pDoc->GetDxfDataCnt(DXFCIRCLEDATA),
				pDoc->GetDxfDataCnt(DXFARCDATA),
				pDoc->GetDxfDataCnt(DXFELLIPSEDATA),
				pDoc->GetDxfDataCnt(DXFPOINTDATA) );
	m_wndStatusBar.SetPaneText(
		m_wndStatusBar.CommandToIndex(ID_DXFST_DATAINFO), str);
}

void CDXFChild::SetFactorInfo(float dFactor)
{
	CString		str;
	str.Format(ID_INDICATOR_FACTOR_F, "---", dFactor, "     ");
	m_wndStatusBar.SetPaneText(
		m_wndStatusBar.CommandToIndex(ID_INDICATOR_FACTOR), str);
}

void CDXFChild::OnUpdateMouseCursor(const CPointF* lppt)
{
	CString		str;
	if ( lppt )
		str.Format(ID_DXFST_MOUSE_F, lppt->x, lppt->y);
	else
		VERIFY(str.LoadString(ID_DXFST_MOUSE));
	m_wndStatusBar.SetPaneText(
		m_wndStatusBar.CommandToIndex(ID_DXFST_MOUSE), str);
}

void CDXFChild::ShowShapeView(void)
{
	// 40������������޳���ނ�1/4�C�ǂ��炩�����������ذ�ޭ����߲݂�
	CRect	rc;
	GetClientRect(rc);
	int		nInfo,
			n1 = AfxGetNCVCMainWnd()->GetNCTextWidth() * 40,
			n2 = rc.Width() / 4;
	nInfo = min(n1, n2);
	m_wndSplitter.SetColumnInfo(0, rc.Width()-nInfo, 0);
	m_wndSplitter.RecalcLayout();
	m_wndSplitter.SetActivePane(0, 1);
	// Ҳ��ޭ��̐}�`̨��
	GetMainView()->PostMessage(WM_USERVIEWFITMSG);
}

/////////////////////////////////////////////////////////////////////////////
// CDXFChild ���b�Z�[�W �n���h��

int CDXFChild::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if ( __super::OnCreate(lpCreateStruct) < 0 )
		return -1;
	
	// �ð���ް�쐬
	if ( !m_wndStatusBar.Create(this, WS_CHILD|WS_VISIBLE|CBRS_BOTTOM, ID_DXF_STATUSBAR) ||
			!m_wndStatusBar.SetIndicators(g_nIndicators, SIZEOF(g_nIndicators)) ) {
		TRACE0("Failed to DXF child status bar\n");
		return -1;      // �쐬�Ɏ��s
	}

	return 0;
}

void CDXFChild::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);
	if ( cx<=0 || cy<=0 )
		return;

	CDXFDoc* pDoc = static_cast<CDXFDoc*>(GetActiveDocument());
	if ( pDoc && !pDoc->IsDocFlag(DXFDOC_SHAPE) ) {
		m_wndSplitter.SetColumnInfo(0, cx, 0);
		m_wndSplitter.RecalcLayout();
		// �ĕ`�掞��°��ް�̈�Ŗ��ȃX�W���ł�
	}
}

void CDXFChild::OnMDIActivate(BOOL bActivate, CWnd* pActivateWnd, CWnd* pDeactivateWnd) 
{
	__super::OnMDIActivate(bActivate, pActivateWnd, pDeactivateWnd);
	DBGBOOL(g_dbg, "CDXFChild::bActivate", bActivate);
	// Ӱ��ڽ�޲�۸ނւ��޷���Đؑ֒ʒm
	if ( bActivate )
		AfxGetNCVCMainWnd()->AllModelessDlg_PostSwitchMessage();
}

LRESULT CDXFChild::OnUserInitialUpdate(WPARAM, LPARAM)
{
	ShowShapeView();
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CDXFFrameSplit �N���X�̃��b�Z�[�W�n���h��

void CDXFFrameSplit::OnLButtonDblClk(UINT, CPoint) 
{
	// �߲݂�������Ԃɖ߂�
	GetParent()->PostMessage(WM_USERINITIALUPDATE);
}
