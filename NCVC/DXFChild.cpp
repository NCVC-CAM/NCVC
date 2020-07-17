// DXFChild.cpp : �C���v�������e�[�V���� �t�@�C��
//

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "DXFChild.h"
// ���Ѻ����ٰèݸ�
#include "DXFDoc.h"
// ���د�����޳�쐬
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

BEGIN_MESSAGE_MAP(CDXFChild, CMDIChildWnd)
	//{{AFX_MSG_MAP(CDXFChild)
	ON_WM_MDIACTIVATE()
	ON_WM_CREATE()
	ON_WM_CLOSE()
	//}}AFX_MSG_MAP
	ON_WM_SYSCOMMAND()
	// հ�޲Ƽ�ُ���
	ON_MESSAGE(WM_USERINITIALUPDATE, OnUserInitialUpdate)
	// ̧�ٕύX�ʒm from DocBase.cpp
	ON_MESSAGE(WM_USERFILECHANGENOTIFY, OnUserFileChangeNotify)
END_MESSAGE_MAP()

BEGIN_MESSAGE_MAP(CDXFFrameSplit, CSplitterWnd)
	ON_WM_LBUTTONDBLCLK()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDXFChild �N���X�̍\�z/����

CDXFChild::CDXFChild()
{
}

CDXFChild::~CDXFChild()
{
}

/////////////////////////////////////////////////////////////////////////////
// CDXFChild �N���X�̃I�[�o���C�h�֐�

BOOL CDXFChild::OnCreateClient(LPCREATESTRUCT /*lpcs*/, CCreateContext* pContext) 
{
	CRect	rc;
	GetParentFrame()->GetClientRect(&rc);
	//	�ÓI���د��쐬
	if ( !m_wndSplitter.CreateStatic(this, 1, 2) ||
			!m_wndSplitter.CreateView(0, 0, RUNTIME_CLASS(CDXFView),
				CSize(rc.Width(), 0), pContext) ||
			!m_wndSplitter.CreateView(0, 1, RUNTIME_CLASS(CDXFShapeView),
				CSize(0, 0), pContext) )
		return FALSE;

	return TRUE;
}

void CDXFChild::ActivateFrame(int nCmdShow) 
{
#ifdef _DEBUG
	g_dbg.printf("CDXFChild::ActivateFrame() Call");
#endif
	CMDIChildWnd::ActivateFrame(CChildBase::ActivateFrame(nCmdShow));
}

BOOL CDXFChild::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) 
{
//	return CMDIChildWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
/*
	���د�����޳�ɂ�镡���ޭ���
	̫������ڂ��Ă����퓮�삳���邽�߂�
	���Ѻ����ٰèݸ�
*/
	if ( CMDIChildWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo) )
		return TRUE;
	CDXFDoc* pDoc = static_cast<CDXFDoc *>(GetActiveDocument());
	if ( !pDoc )
		return FALSE;
	CMDIChildWnd*	pChild = AfxGetNCVCMainWnd()->MDIGetActive();
	return pDoc->RouteCmdToAllViews(pChild ? pChild->GetActiveView() : NULL,
		nID, nCode, pExtra, pHandlerInfo);
}

/////////////////////////////////////////////////////////////////////////////
// CDXFChild �N���X�̐f�f

#ifdef _DEBUG
void CDXFChild::AssertValid() const
{
	CMDIChildWnd::AssertValid();
}

void CDXFChild::Dump(CDumpContext& dc) const
{
	CMDIChildWnd::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CDXFChild ���ފ֐�

void CDXFChild::SetDataInfo(int nLine, int nCircle, int nArc, int nEllipse, int nPoint)
{
	CString		str;
	str.Format(ID_DXFST_DATAINFO_F, nLine, nCircle, nArc, nEllipse, nPoint);
	m_wndStatusBar.SetPaneText(
		m_wndStatusBar.CommandToIndex(ID_DXFST_DATAINFO), str);
}

void CDXFChild::SetFactorInfo(double dFactor)
{
	CString		str;
	str.Format(ID_INDICATOR_FACTOR_F, "", dFactor);
	m_wndStatusBar.SetPaneText(
		m_wndStatusBar.CommandToIndex(ID_INDICATOR_FACTOR), str);
}

void CDXFChild::OnUpdateMouseCursor(const CPointD* lppt)
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
	GetClientRect(&rc);
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
	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	// �ð���ް�쐬
	if (!m_wndStatusBar.Create(this, WS_CHILD|WS_VISIBLE|CBRS_BOTTOM, ID_DXF_STATUSBAR) ||
			!m_wndStatusBar.SetIndicators(g_nIndicators, SIZEOF(g_nIndicators)) ) {
		TRACE0("Failed to DXF child status bar\n");
		return -1;      // �쐬�Ɏ��s
	}

	return 0;
}

void CDXFChild::OnClose() 
{
	AfxGetNCVCMainWnd()->AllModelessDlg_PostSwitchMessage();
	CMDIChildWnd::OnClose();
}

void CDXFChild::OnMDIActivate(BOOL bActivate, CWnd* pActivateWnd, CWnd* pDeactivateWnd) 
{
	CMDIChildWnd::OnMDIActivate(bActivate, pActivateWnd, pDeactivateWnd);
	DBGBOOL(g_dbg, "CDXFChild::bActivate", bActivate);
	CChildBase::OnMDIActivate(this, bActivate);
}

LRESULT CDXFChild::OnUserInitialUpdate(WPARAM, LPARAM)
{
	ShowShapeView();
	return 0;
}

LRESULT CDXFChild::OnUserFileChangeNotify(WPARAM, LPARAM)
{
	CChildBase::OnUserFileChangeNotify(this);
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CDXFFrameSplit �N���X�̃��b�Z�[�W�n���h��

void CDXFFrameSplit::OnLButtonDblClk(UINT, CPoint) 
{
	// �߲݂�������Ԃɖ߂�
	GetParent()->PostMessage(WM_USERINITIALUPDATE);
}
