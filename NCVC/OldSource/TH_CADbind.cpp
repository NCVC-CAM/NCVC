// TH_CADbind.cpp
//	CADﾃﾞｰﾀの統合
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

// ｸﾞﾛｰﾊﾞﾙ変数定義
static	CThreadDlg*		g_pParent;
static	CDXFDoc*		g_pDoc;

// よく使う変数や呼び出しの簡略置換
#define	IsThread()	g_pParent->IsThreadContinue()

// ｻﾌﾞ関数
static	BOOL	ReadCAD_MainFunc(const CString&, CDXFView*, UINT);	// CADﾃﾞｰﾀの読み込みﾒｲﾝﾙｰﾌﾟ

//////////////////////////////////////////////////////////////////////
//	CADﾃﾞｰﾀの統合ｽﾚｯﾄﾞ
//////////////////////////////////////////////////////////////////////

UINT CADbind_Thread(LPVOID pVoid)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CADbind_Thread()\nStart", DBG_GREEN);
#endif

	int			i, nLoopCnt,
				nResult = IDOK;

	// ｸﾞﾛｰﾊﾞﾙ変数初期化
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
			g_pParent->m_ctReadProgress.SetPos(i+1);	// ﾌﾟﾛｸﾞﾚｽﾊﾞｰ
		}
		// 面積で並べ替え
		g_pDoc->SortBindInfo();
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		nResult = IDCANCEL;
	}

	g_pParent->m_ctReadProgress.SetPos(nLoopCnt);
	g_pParent->PostMessage(WM_USERFINISH, nResult);	// このｽﾚｯﾄﾞからﾀﾞｲｱﾛｸﾞ終了

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

	// 親ﾌﾚｰﾑ(CStatic)生成
	rc.SetRectEmpty();
	pParent = new CStatic;
	if ( !pParent->Create(NULL, WS_CHILD|WS_VISIBLE|SS_SUNKEN|SS_ENHMETAFILE|SS_NOTIFY,
			rc, pParentView, nID) ) {
		delete pParent;
		return FALSE;
	}

	// ﾄﾞｷｭﾒﾝﾄ生成
	pDoc  = static_cast<CDXFDoc*>(RUNTIME_CLASS(CDXFDoc)->CreateObject());
	if ( pDoc )
		pDoc->SetDocFlag(DXFDOC_BIND);
	else
		bResult = FALSE;

	// ﾋﾞｭｰ生成
	if ( bResult ) {
		pView = static_cast<CDXFView*>(RUNTIME_CLASS(CDXFView)->CreateObject());
		if ( pView ) {
			if ( pView->Create(NULL, NULL, AFX_WS_DEFAULT_VIEW, rc, pParent, AFX_IDW_PANE_FIRST) ) {
				pDoc->AddView(pView);
				pView->OnInitialUpdate();	// なぜか必要
			}
			else
				bResult = FALSE;
		}
		else
			bResult = FALSE;
	}

	// Serialize関数の決定とﾌｧｲﾙの読み込み
	if ( bResult ) {
		CDocument*	dummy;
		if ( AfxGetNCVCApp()->GetDocTemplate(TYPE_DXF)->MatchDocType(strFile, dummy) == CDocTemplate::yesAttemptNative ) {
			// ﾌｧｲﾙの読み込み
			if ( pDoc->OnOpenDocument(strFile) )
				pView->OnInitialUpdate();
			else
				bResult = FALSE;
		}
		else
			bResult = FALSE;
	}

	// bind情報の登録
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
