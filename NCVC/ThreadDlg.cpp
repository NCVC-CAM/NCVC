// ThreadDlg.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "ThreadDlg.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
extern	CMagaDbg	g_dbg;
#endif

BEGIN_MESSAGE_MAP(CThreadDlg, CDialog)
	//{{AFX_MSG_MAP(CThreadDlg)
	ON_WM_NCHITTEST()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_USERFINISH, &CThreadDlg::OnUserFinish)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CThreadDlg ダイアログ

CThreadDlg::CThreadDlg(int nID, CDocument* pDoc, WPARAM wParam/*=NULL*/, LPARAM lParam/*=NULL*/)
	: CDialog(CThreadDlg::IDD, NULL)
{
	//{{AFX_DATA_INIT(CThreadDlg)
	//}}AFX_DATA_INIT
	m_nID = nID;
	m_paramThread.pParent = this;
	m_paramThread.pDoc   = pDoc;
	m_paramThread.wParam = wParam;
	m_paramThread.lParam = lParam;

	m_pThread = NULL;
	m_bThread = TRUE;
}

void CThreadDlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CThreadDlg)
	DDX_Control(pDX, IDC_READ_TEXT2, m_ctMsgText2);
	DDX_Control(pDX, IDC_READ_TEXT, m_ctMsgText1);
	DDX_Control(pDX, IDC_READ_PROGRESS, m_ctReadProgress);
	//}}AFX_DATA_MAP
}

/////////////////////////////////////////////////////////////////////////////
// CThreadDlg メッセージ ハンドラ

BOOL CThreadDlg::OnInitDialog() 
{
	__super::OnInitDialog();

	AFX_THREADPROC	pfnThread;

	switch ( m_nID ) {
	case IDS_READ_NCD:			// NCﾃﾞｰﾀ内部変換ｽﾚｯﾄﾞ開始
		pfnThread = NCDtoXYZ_Thread;
		break;

	case IDS_CORRECT_NCD:		// 補正座標計算ｽﾚｯﾄﾞ開始
		pfnThread = CorrectCalc_Thread;
		break;

	case IDS_UVTAPER_NCD:		// ﾜｲﾔ加工用UV軸ｵﾌﾞｼﾞｪｸﾄの生成
		pfnThread = UVWire_Thread;
		break;

	case ID_FILE_DXF2NCD:		// NC生成ｽﾚｯﾄﾞ開始
		switch ( m_paramThread.wParam ) {
		case ID_FILE_DXF2NCD_LATHE:
			pfnThread = MakeLathe_Thread;
			break;
		case ID_FILE_DXF2NCD_WIRE:
			pfnThread = MakeWire_Thread;
			break;
		default:
			pfnThread = MakeNCD_Thread;
		}
		break;

	case ID_EDIT_DXFSHAPE:		// 連結ｵﾌﾞｼﾞｪｸﾄの検索ｽﾚｯﾄﾞ開始
		pfnThread = ShapeSearch_Thread;
		break;

	case ID_EDIT_SHAPE_AUTO:	// 自動加工指示ｽﾚｯﾄﾞ開始
		pfnThread = AutoWorkingSet_Thread;
		break;

	default:
		EndDialog(IDCANCEL);
		return TRUE;
	}

	m_pThread = AfxBeginThread(pfnThread, &m_paramThread,
					THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
	if ( m_pThread ) {
		m_ctReadProgress.SetPos(0);
		m_pThread->m_bAutoDelete = FALSE;
		m_pThread->ResumeThread();
	}
	else
		::NCVC_CriticalErrorMsg(__FILE__, __LINE__);

	return TRUE;  // コントロールにフォーカスを設定しないとき、戻り値は TRUE となります
	              // 例外: OCX プロパティ ページの戻り値は FALSE となります
}

void CThreadDlg::OnCancel() 
{
#ifdef _DEBUG
	CMagaDbg	dbg("CThreadDlg::OnCancel()\nStart", DBG_RED);
#endif
	m_bThread = FALSE;		// ｷｬﾝｾﾙﾌﾗｸﾞのみ
							// ここでWaitForSingleObject()を呼び出すと
							// ﾌﾟﾛｸﾞﾚｽﾊﾞｰなどの更新ﾒｯｾｰｼﾞとﾃﾞｯﾄﾞﾛｯｸを引き起こす
//	__super::OnCancel();	// 各ｽﾚｯﾄﾞより OnUserFinishをPostMessage()
}

LRESULT CThreadDlg::OnUserFinish(WPARAM wParam, LPARAM)
{
	if ( m_pThread ) {
#ifdef _DEBUG
		CMagaDbg	dbg("CThreadDlg::OnUserFinish()", DBG_BLUE);
		if ( ::WaitForSingleObject(m_pThread->m_hThread, INFINITE) == WAIT_FAILED ) {
			dbg.printf("WaitForSingleObject() Fail!");
			::NC_FormatMessage();
		}
		else
			dbg.printf("WaitForSingleObject() OK");
#else
		::WaitForSingleObject(m_pThread->m_hThread, INFINITE);
#endif
		delete	m_pThread;
	}
	EndDialog((int)wParam);
	return 0;
}

LRESULT CThreadDlg::OnNcHitTest(CPoint pt)
{
	LRESULT nHitTest = __super::OnNcHitTest(pt);
	if ( nHitTest==HTCLIENT && GetAsyncKeyState(MK_LBUTTON)<0 )
		nHitTest = HTCAPTION;
	return nHitTest;
}
