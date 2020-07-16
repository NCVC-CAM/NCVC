// NCViewSplit.cpp: CNCViewSplit �N���X�̃C���v�������e�[�V����
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NCVC.h"
#include "NCdata.h"
#include "NCDoc.h"
#include "NCViewTab.h"
#include "NCViewSplit.h"
#include "NCView.h"
#include "NCViewXY.h"
#include "NCViewXZ.h"
#include "NCViewYZ.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

BEGIN_MESSAGE_MAP(CNCViewSplit, CSplitterWnd)
	ON_WM_DESTROY()
//	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDBLCLK()
	// հ�޲Ƽ�ُ���
	ON_MESSAGE (WM_USERINITIALUPDATE, OnUserInitialUpdate)
	// �߰�ސֲؑ����
	ON_MESSAGE (WM_USERACTIVATEPAGE, OnUserActivatePage)
	// �e�ޭ��ւ�̨��ү����
	ON_MESSAGE (WM_USERVIEWFITMSG, OnUserViewFitMsg)
	// �S�Ă��߲݂̐}�`̨��
	ON_COMMAND(ID_NCVIEW_ALLFIT, OnAllFitCmd)
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////
// �\�z/����
//////////////////////////////////////////////////////////////////////

CNCViewSplit::CNCViewSplit()
{
}

CNCViewSplit::~CNCViewSplit()
{
}

/////////////////////////////////////////////////////////////////////////////
// CNCViewSplit �N���X�̃I�[�o���C�h�֐�

/////////////////////////////////////////////////////////////////////////////
// CNCViewSplit �N���X�̃����o�֐�

void CNCViewSplit::DrawData(CNCdata* pData)
{
	CDC		dc;

	if ( dc.Attach(m_hDC[0]) ) {
		pData->Draw(&dc);
		dc.Detach();
	}
	if ( dc.Attach(m_hDC[1]) ) {
		pData->DrawYZ(&dc);
		dc.Detach();
	}
	if ( dc.Attach(m_hDC[2]) ) {
		pData->DrawXZ(&dc);
		dc.Detach();
	}
	if ( dc.Attach(m_hDC[3]) ) {
		pData->DrawXY(&dc);
		dc.Detach();
	}
}

void CNCViewSplit::AllPane_PostMessage(int nID, UINT msg, WPARAM wParam, LPARAM lParam)
{
	int		i, j;

	if ( nID == NC_SINGLEPANE ) {	// �S��-1
		for ( i=0; i<GetRowCount(); i++ ) {			// �s
			for ( j=0; j<GetColumnCount(); j++ ) {	// ��
				GetPane(i, j)->PostMessage(msg, wParam, lParam);
			}
		}
		SetActivePane(0, 0);	// XYZ�\����è�ނ�
	}
	else {								// �S��-2
		GetPane(0, 1)->PostMessage(msg, wParam, lParam);	// XYZ
		CSplitterWnd* pWnd = (CSplitterWnd *)GetPane(0, 0);
		for ( i=0; i<pWnd->GetRowCount(); i++ ) {
			pWnd->GetPane(i, 0)->PostMessage(msg, wParam, lParam);	// YZ, XZ, XY
		}
		SetActivePane(0, 1);	// XYZ�\����è�ނ�
	}
}

void CNCViewSplit::CalcPane(int nID, BOOL bInitial/*=FALSE*/)
{
	CString	strRegKey, strEntry;
	VERIFY(strRegKey.LoadString(IDS_REGKEY_NC));
	int		nCxEdge = ::GetSystemMetrics(SM_CXEDGE) * 2;	// �O���̉��܂�
	int		nCyEdge = ::GetSystemMetrics(SM_CYEDGE) * 2;
	int		nCol, nRow, nRow2;
	CRect	rc;
	GetParent()->GetClientRect(&rc);

	// ���د�����޳�̏������ސݒ�
	if ( nID == NC_SINGLEPANE ) {	// �S��-1
		nRow = (rc.Height() >> 1) - nCyEdge;
		nCol = (rc.Width()  >> 1) - nCxEdge;
		if ( bInitial ) {
			strEntry.Format(IDS_REG_NCV_VIEWPANESIZE, 1, 1);
			nRow = AfxGetApp()->GetProfileInt(strRegKey, strEntry, nRow);
			strEntry.Format(IDS_REG_NCV_VIEWPANESIZE, 1, 2);
			nCol = AfxGetApp()->GetProfileInt(strRegKey, strEntry, nCol);
		}
		SetRowInfo(0, nRow, 0);
		SetColumnInfo(0, nCol, 0);
	}
	else {							// �S��-2
		nRow = nRow2 = rc.Height() / 3 - nCyEdge;
		nCol = rc.Width() / 3 - nCxEdge;
		if ( bInitial ) {
			strEntry.Format(IDS_REG_NCV_VIEWPANESIZE, 2, 1);
			nCol = AfxGetApp()->GetProfileInt(strRegKey, strEntry, nCol);
			strEntry.Format(IDS_REG_NCV_VIEWPANESIZE, 2, 2);
			nRow = AfxGetApp()->GetProfileInt(strRegKey, strEntry, nRow);
			strEntry.Format(IDS_REG_NCV_VIEWPANESIZE, 2, 3);
			nRow2 = AfxGetApp()->GetProfileInt(strRegKey, strEntry, nRow2);
		}
		SetColumnInfo(0, nCol, 0);
		CSplitterWnd* pWnd = (CSplitterWnd *)GetPane(0, 0);
		pWnd->SetRowInfo(0, nRow,  0);
		pWnd->SetRowInfo(1, nRow2, 0);
	}
	RecalcLayout();
}

/////////////////////////////////////////////////////////////////////////////
// CNCViewSplit �N���X�̃��b�Z�[�W �n���h��

LRESULT CNCViewSplit::OnUserInitialUpdate(WPARAM wParam, LPARAM lParam)
{
	int		i, j;
	CClientDC*	pDC;

	// ���د�����޳�̏������ސݒ�
	CalcPane(wParam, TRUE);
	// �e�߲݂֐}�`̨��ү���ނ̑��M
	if ( wParam == NC_SINGLEPANE ) {	// �S��-1
		for ( i=0; i<GetRowCount(); i++ ) {			// �s
			for ( j=0; j<GetColumnCount(); j++ ) {	// ��
				if ( (BOOL)lParam )
					GetPane(i, j)->SendMessage(WM_USERVIEWFITMSG, 0, lParam);
				// ���޲���÷������ق��擾(XYZ, YZ, XZ, XY ��)
				pDC = new CClientDC(GetPane(i, j));
				m_hDC[j+i*2] = pDC->GetSafeHdc();
				delete	pDC;
			}
		}
		if ( (BOOL)lParam )
			SetActivePane(0, 0);	// XYZ�\����è�ނ�
	}
	else {								// �S��-2
		if ( (BOOL)lParam )
			GetPane(0, 1)->SendMessage(WM_USERVIEWFITMSG, 0, lParam);	// XYZ
		pDC = new CClientDC(GetPane(0, 1));
		m_hDC[0] = pDC->GetSafeHdc();
		delete	pDC;
		CSplitterWnd* pWnd = (CSplitterWnd *)GetPane(0, 0);
		for ( i=0; i<pWnd->GetRowCount(); i++ ) {
			if ( (BOOL)lParam )
				pWnd->GetPane(i, 0)->SendMessage(WM_USERVIEWFITMSG, 0, lParam);	// YZ, XZ, XY
			pDC = new CClientDC(pWnd->GetPane(i, 0));
			m_hDC[i+1] = pDC->GetSafeHdc();
			delete	pDC;
		}
		if ( (BOOL)lParam )
			SetActivePane(0, 1);	// XYZ�\����è�ނ�
	}

	return 0;
}

LRESULT CNCViewSplit::OnUserActivatePage(WPARAM wParam, LPARAM)
{
	// �e�ֱ߲݂�è��ү���ނ̑��M
	AllPane_PostMessage(wParam, WM_USERACTIVATEPAGE, NULL, TRUE);
	return 0;
}

void CNCViewSplit::OnDestroy() 
{
	int		i, nInfo, nMin;
	CString	strRegKey, strEntry;
	VERIFY(strRegKey.LoadString(IDS_REGKEY_NC));

	if ( GetParent()->IsKindOf(RUNTIME_CLASS(CNCViewTab)) ) {
		if ( GetRowCount() > 1 ) {
			// �S��-1
			GetRowInfo(0, nInfo, nMin);
			if ( nInfo > 0 ) {
				strEntry.Format(IDS_REG_NCV_VIEWPANESIZE, 1, 1);
				AfxGetApp()->WriteProfileInt(strRegKey, strEntry, nInfo);
			}
			GetColumnInfo(0, nInfo, nMin);
			if ( nInfo > 0 ) {
				strEntry.Format(IDS_REG_NCV_VIEWPANESIZE, 1, 2);
				AfxGetApp()->WriteProfileInt(strRegKey, strEntry, nInfo);
			}
		}
		else {
			// �S��-2
			GetColumnInfo(0, nInfo, nMin);
			if ( nInfo > 0 ) {
				strEntry.Format(IDS_REG_NCV_VIEWPANESIZE, 2, 1);
				AfxGetApp()->WriteProfileInt(strRegKey, strEntry, nInfo);
			}
		}
	}
	else {
		// �S��-2 �̍��߲�
		for ( i=0; i<GetRowCount()-1; i++ ) {
			GetRowInfo(i, nInfo, nMin);
			if ( nInfo > 0 ) {
				strEntry.Format(IDS_REG_NCV_VIEWPANESIZE, 2, i+2);
				AfxGetApp()->WriteProfileInt(strRegKey, strEntry, nInfo);
			}
		}
	}
}

void CNCViewSplit::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	// �e�߲݂�������Ԃɖ߂�
	CWnd*	pWnd = GetParent();
	if ( pWnd->IsKindOf(RUNTIME_CLASS(CNCViewTab)) )
		OnUserViewFitMsg( ((CNCViewTab *)pWnd)->GetActivePage(), TRUE );
	else
		pWnd->PostMessage(WM_LBUTTONDBLCLK);
}

void CNCViewSplit::OnAllFitCmd()
{
	// �e�߲݂� ID_VIEW_FIT �����ү���ނ𑗐M
	CWnd*	pParent = GetParent();
	// CNCViewSplit ���Q�d�ɂȂ��Ă���S��-2 �̍��߲݂�
	// ��ɏ�ʂ� CNCViewSplit ������ү���ނ�߂܂���̂�
	// �K�� GetParent() �� CNCViewTab ���w���n�Y
	if ( !pParent->IsKindOf(RUNTIME_CLASS(CNCViewTab)) ) {		// �ی�����
		// ��� CNCViewSplit�� ID_NCVIEW_ALLFIT ү������ڸ�
		pParent->PostMessage(WM_COMMAND, MAKEWPARAM(ID_NCVIEW_ALLFIT, 0));
	}
	else {
		AllPane_PostMessage(((CNCViewTab *)pParent)->GetActivePage(),
			WM_COMMAND, MAKEWPARAM(ID_VIEW_FIT, 0));
	}
}

LRESULT CNCViewSplit::OnUserViewFitMsg(WPARAM wParam, LPARAM lParam)
{
	// ���د�����޳�̏������ސݒ�
	CalcPane(wParam);
	AllPane_PostMessage(wParam, WM_USERVIEWFITMSG, 0, lParam);
	return 0;
}
/*
BOOL CNCViewSplit::OnEraseBkgnd(CDC* pDC) 
{
#ifdef _DEBUG
	CMagaDbg	dbg("CNCViewSplit::OnEraseBkgnd()\nStart");
#endif
	return TRUE;
}
*/
