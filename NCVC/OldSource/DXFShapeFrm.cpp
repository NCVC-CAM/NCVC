// DXFShapeFrm.cpp : �C���v�������e�[�V���� �t�@�C��
//

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "DXFdata.h"
// ���Ѻ����ٰèݸ�
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
// CDXFShapeFrm �N���X�̃I�[�o���C�h�֐�

BOOL CDXFShapeFrm::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext) 
{
	// CDXFShapeFrm�Ǘ�����CDXFShapeView���쐬
	pContext->m_pNewViewClass = RUNTIME_CLASS(CDXFShapeView);
	return CFrameWnd::OnCreateClient(lpcs, pContext);
}

BOOL CDXFShapeFrm::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) 
{
//	return CFrameWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
/*
	���د�����޳�ɂ�镡���ޭ���
	̫������ڂ��Ă����퓮�삳���邽�߂�
	���Ѻ����ٰèݸ�
*/
	if ( CFrameWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo) )
		return TRUE;
	CDXFDoc* pDoc = (CDXFDoc *)GetActiveDocument();
	if ( !pDoc )
		return FALSE;
	// CDXFShapeView�ȊO(CDXFView)�ɺ����ٰèݸ�
	return pDoc->RouteCmdToAllViews(GetActiveView(), nID, nCode, pExtra, pHandlerInfo);
}

/////////////////////////////////////////////////////////////////////////////
// CDXFShapeFrm �N���X�̐f�f

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
// CDXFShapeFrm ���b�Z�[�W �n���h��

int CDXFShapeFrm::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	// �`�󏈗��p°��ް�̍쐬
	m_wndToolBar.Create(this, WS_CHILD|WS_VISIBLE|CBRS_TOP|CBRS_TOOLTIPS);
	m_wndToolBar.LoadToolBar(IDR_SHAPE);

	return 0;
}
