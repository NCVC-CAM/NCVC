// TH_CADbind.cpp
//	CAD�ް��̓���
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "DXFdata.h"
#include "DXFDoc.h"
#include "DXFView.h"
#include "ThreadDlg.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

// ��۰��ٕϐ���`
static	CThreadDlg*		g_pParent;
static	CDXFDoc*		g_pDoc;

// �悭�g���ϐ���Ăяo���̊ȗ��u��
#define	IsThread()	g_pParent->IsThreadContinue()

// ��ފ֐�
static	BOOL	ReadCAD_MainFunc(const CString&, CDXFView*, UINT);	// CAD�ް��̓ǂݍ���Ҳ�ٰ��

//////////////////////////////////////////////////////////////////////
//	CAD�ް��̓����گ��
//////////////////////////////////////////////////////////////////////

UINT CADbind_Thread(LPVOID pVoid)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CADbind_Thread()\nStart", DBG_GREEN);
#endif

	int			i, nLoopCnt,
				nResult = IDOK;

	// ��۰��ٕϐ�������
	LPNCVCTHREADPARAM	pParam = reinterpret_cast<LPNCVCTHREADPARAM>(pVoid);
	g_pParent = pParam->pParent;
	g_pDoc = static_cast<CDXFDoc*>(pParam->pDoc);
	CStringArray*	pAryFile = reinterpret_cast<CStringArray *>(pParam->wParam);
	CDXFView*		pView = reinterpret_cast<CDXFView *>(pParam->lParam);
	ASSERT(g_pParent);
	ASSERT(g_pDoc);
	ASSERT(pAryFile);
	ASSERT(pView);

	nLoopCnt = pAryFile->GetCount();
	g_pParent->m_ctReadProgress.SetRange32(0, nLoopCnt);

	try {
		for ( i=0; i<nLoopCnt && IsThread(); i++ ) {
			if ( !ReadCAD_MainFunc(pAryFile->GetAt(i), pView, i+1) ) {
				nResult = IDCANCEL;
				break;
			}
			g_pParent->m_ctReadProgress.SetPos(i+1);	// ��۸�ڽ�ް
		}
		// �ʐςŕ��בւ�
		g_pDoc->SortBindInfo();
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		nResult = IDCANCEL;
	}

	g_pParent->m_ctReadProgress.SetPos(nLoopCnt);
	g_pParent->PostMessage(WM_USERFINISH, nResult);	// ���̽گ�ނ����޲�۸ޏI��

	return 0;
}

//////////////////////////////////////////////////////////////////////

BOOL ReadCAD_MainFunc(const CString& strFile, CDXFView* pParentView, UINT nID)
{
	BOOL			bResult = TRUE;
	CStatic*		pParent = NULL;
	CDXFDoc*		pDoc = NULL;
	CDXFView*		pView = NULL;
	CRect			rc;

	// �e�ڰ�(CStatic)����
	rc.SetRectEmpty();
	pParent = new CStatic;
	if ( !pParent->Create(NULL, WS_CHILD|WS_VISIBLE|SS_SUNKEN|SS_ENHMETAFILE|SS_NOTIFY,
			rc, pParentView, nID) ) {
		delete pParent;
		return FALSE;
	}

	// �޷���Đ���
	pDoc  = static_cast<CDXFDoc*>(RUNTIME_CLASS(CDXFDoc)->CreateObject());
	if ( pDoc )
		pDoc->SetDocFlag(DXFDOC_BIND);
	else
		bResult = FALSE;

	// �ޭ�����
	if ( bResult ) {
		pView = static_cast<CDXFView*>(RUNTIME_CLASS(CDXFView)->CreateObject());
		if ( pView ) {
			if ( pView->Create(NULL, NULL, AFX_WS_DEFAULT_VIEW, rc, pParent, AFX_IDW_PANE_FIRST) ) {
				pDoc->AddView(pView);
				pView->OnInitialUpdate();	// �Ȃ����K�v
			}
			else
				bResult = FALSE;
		}
		else
			bResult = FALSE;
	}

	// Serialize�֐��̌����̧�ق̓ǂݍ���
	if ( bResult ) {
		CDocument*	dummy;
		if ( AfxGetNCVCApp()->GetDocTemplate(TYPE_DXF)->MatchDocType(strFile, dummy) == CDocTemplate::yesAttemptNative ) {
			// ̧�ق̓ǂݍ���
			if ( pDoc->OnOpenDocument(strFile) )
				pView->OnInitialUpdate();
			else
				bResult = FALSE;
		}
		else
			bResult = FALSE;
	}

	// bind���̓o�^
	if ( bResult ) {
		LPCADBINDINFO pInfo = new CADBINDINFO;
		pInfo->pParent	= pParent;
		pInfo->pDoc		= pDoc;
		pInfo->pView	= pView;
		g_pDoc->AddBindInfo(pInfo);
	}
	else {
		if ( pView )
			pView->DestroyWindow();
		if ( pDoc )
			delete	pDoc;
		delete	pParent;
	}
	
	return TRUE;
}
