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
	ON_WM_LBUTTONDBLCLK()
	// հ�޲Ƽ�ُ���
	ON_MESSAGE (WM_USERINITIALUPDATE, &CNCViewSplit::OnUserInitialUpdate)
	// �߰�ސֲؑ����
	ON_MESSAGE (WM_USERACTIVATEPAGE, &CNCViewSplit::OnUserActivatePage)
	// �e�ޭ��ւ�̨��ү����
	ON_MESSAGE (WM_USERVIEWFITMSG, &CNCViewSplit::OnUserViewFitMsg)
	// �S�Ă��߲݂̐}�`̨��
	ON_COMMAND(ID_NCVIEW_ALLFIT, &CNCViewSplit::OnAllFitCmd)
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////
// �\�z/����
//////////////////////////////////////////////////////////////////////

CNCViewSplit::CNCViewSplit()
{
#ifdef _DEBUG_FILEOPEN
	g_dbg.printf("CNCViewSplit::CNCViewSplit() Start");
#endif
}

/////////////////////////////////////////////////////////////////////////////
// CNCViewSplit �N���X�̃I�[�o���C�h�֐�

/////////////////////////////////////////////////////////////////////////////
// CNCViewSplit �N���X�̃����o�֐�

void CNCViewSplit::DrawData(CNCdata* pData, BOOL bSelect, PFNNCDRAWPROC pfnDrawProc[])
{
	CDC		dc;
	for ( int i=0; i<SIZEOF(m_hDC); i++ ) {
		if ( dc.Attach(m_hDC[i]) ) {
			dc.SetROP2(R2_COPYPEN);
			(pData->*pfnDrawProc[i])(&dc, bSelect);
			dc.Detach();
		}
	}
}

void CNCViewSplit::AllPane_PostMessage(UINT_PTR nID, UINT msg, WPARAM wParam, LPARAM lParam)
{
	int		i, j;

	if ( nID == NCDRAWVIEW_NUM ) {	// �S��-1
		for ( i=0; i<2; i++ ) {			// �s
			for ( j=0; j<2; j++ ) {		// ��
				GetPane(i, j)->PostMessage(msg, wParam, lParam);
			}
		}
		SetActivePane(0, 0);	// XYZ�\����è�ނ�
	}
	else {								// �S��-2
		GetPane(0, 1)->PostMessage(msg, wParam, lParam);	// XYZ
		CSplitterWnd* pWnd = static_cast<CSplitterWnd *>(GetPane(0, 0));
		for ( i=0; i<3; i++ )
			pWnd->GetPane(i, 0)->PostMessage(msg, wParam, lParam);	// YZ, XZ, XY
		SetActivePane(0, 1);	// XYZ�\����è�ނ�
	}
}

void CNCViewSplit::CalcPane(UINT_PTR nID, BOOL bInitial/*=FALSE*/)
{
	CString	strRegKey, strEntry;
	VERIFY(strRegKey.LoadString(IDS_REGKEY_NC));
	int		nCxEdge = ::GetSystemMetrics(SM_CXEDGE) * 2;	// �O���̉��܂�
	int		nCyEdge = ::GetSystemMetrics(SM_CYEDGE) * 2;
	int		nCol, nRow, nRow2;
	CRect	rc;
	GetParent()->GetClientRect(rc);

	// ���د�����޳�̏������ސݒ�
	if ( nID == NCDRAWVIEW_NUM ) {	// �S��-1
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
		CSplitterWnd* pWnd = static_cast<CSplitterWnd *>(GetPane(0, 0));
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
	if ( wParam == NCDRAWVIEW_NUM ) {	// �S��-1
		// ���޲���÷������ق��擾
		for ( i=0; i<2; i++ ) {				// �s
			for ( j=0; j<2; j++ ) {			// ��
				pDC = new CClientDC(GetPane(i, j));
				m_hDC[i*2+j] = pDC->GetSafeHdc();
				delete	pDC;
			}
		}
		if ( lParam ) {
			for ( i=0; i<2; i++ ) {
				for ( j=0; j<2; j++ ) {
					GetPane(i, j)->SendMessage(WM_USERVIEWFITMSG, 0, lParam);
				}
			}
			SetActivePane(0, 0);
		}
	}
	else {
		// �S��-2
		CSplitterWnd* pWnd = static_cast<CSplitterWnd *>(GetPane(0, 0));
		for ( i=0; i<3; i++ ) {
			pDC = new CClientDC(pWnd->GetPane(i, 0));
			m_hDC[i] = pDC->GetSafeHdc();
			delete	pDC;
		}
		pDC = new CClientDC(GetPane(0, 1));
		m_hDC[i] = pDC->GetSafeHdc();
		delete	pDC;
		if ( lParam ) {
			for ( i=0; i<3; i++ )
				pWnd->GetPane(i, 0)->SendMessage(WM_USERVIEWFITMSG, 0, lParam);
			GetPane(0, 1)->SendMessage(WM_USERVIEWFITMSG, 0, lParam);
			SetActivePane(0, 1);
		}
	}

	return 0;
}

LRESULT CNCViewSplit::OnUserActivatePage(WPARAM wParam, LPARAM)
{
	// �e�ֱ߲݂�è��ү���ނ̑��M(��Ɋg�嗦�X�V)
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
	CWnd*	pParent = GetParent();
	if ( pParent->IsKindOf(RUNTIME_CLASS(CNCViewTab)) )
		OnUserViewFitMsg( static_cast<CNCViewTab *>(pParent)->GetActivePage(), TRUE );
	else
		pParent->PostMessage(WM_LBUTTONDBLCLK);
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
		AllPane_PostMessage(static_cast<CNCViewTab *>(pParent)->GetActivePage(),
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
