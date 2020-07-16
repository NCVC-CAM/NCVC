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
	ON_MESSAGE (WM_USERFINISH, OnUserFinish)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CThreadDlg ダイアログ

CThreadDlg::CThreadDlg(int nID, CDocument* pDoc, WPARAM wParam, LPARAM lParam)
	: CDialog(CThreadDlg::IDD, NULL)
{
	//{{AFX_DATA_INIT(CThreadDlg)
	//}}AFX_DATA_INIT
	m_nID = nID;
	m_paramThread.pParent = this;
	m_paramThread.pDoc   = pDoc;
	m_paramThread.wParam = wParam;
	m_paramThread.lParam = lParam;

	m_hThread = NULL;
	m_bThread = TRUE;
}

void CThreadDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
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
	CDialog::OnInitDialog();

	CWinThread*		pThread;
	m_ctReadProgress.SetPos(0);

	switch ( m_nID ) {
	case IDS_READ_NCD:			// NCﾃﾞｰﾀ内部変換ｽﾚｯﾄﾞ開始
		pThread = AfxBeginThread(NCDtoXYZ_Thread, &m_paramThread,
			THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
		break;

	case IDS_CORRECT_NCD:		// 補正座標計算ｽﾚｯﾄﾞ開始
		pThread = AfxBeginThread(CorrectCalc_Thread, &m_paramThread,
			THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
		break;

	case ID_FILE_DXF2NCD:		// NC生成ｽﾚｯﾄﾞ開始
		pThread = AfxBeginThread(MakeNCD_Thread, &m_paramThread,
			THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
		break;

	case ID_EDIT_DXFSHAPE:		// 連結ｵﾌﾞｼﾞｪｸﾄの検索ｽﾚｯﾄﾞ開始
		pThread = AfxBeginThread(ShapeSearch_Thread, &m_paramThread,
			THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
		break;

	case ID_EDIT_SHAPE_AUTO:	// 自動加工指示ｽﾚｯﾄﾞ開始
		pThread = AfxBeginThread(AutoWorkingSet_Thread, &m_paramThread,
			THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
		break;

	default:
		EndDialog(IDCANCEL);
		return TRUE;
	}

	m_hThread = ::NCVC_DuplicateHandle(pThread->m_hThread);
	if ( !m_hThread )
		::NCVC_CriticalErrorMsg(__FILE__, __LINE__);
	pThread->ResumeThread();

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
//	CDialog::OnCancel();	// 各ｽﾚｯﾄﾞより OnUserFinishをPostMessage()
}

LRESULT CThreadDlg::OnUserFinish(WPARAM wParam, LPARAM)
{
	if ( m_hThread ) {
#ifdef _DEBUG
		CMagaDbg	dbg("CThreadDlg::OnUserFinish()", DBG_BLUE);
		if ( ::WaitForSingleObject(m_hThread, INFINITE) == WAIT_FAILED ) {
			dbg.printf("WaitForSingleObject() Fail!");
			::NC_FormatMessage();
		}
		else
			dbg.printf("WaitForSingleObject() OK");
		if ( !::CloseHandle(m_hThread) ) {
			dbg.printf("CloseHandle() Fail!");
			::NC_FormatMessage();
		}
		else
			dbg.printf("CloseHandle() OK");
#else
		::WaitForSingleObject(m_hThread, INFINITE);
		::CloseHandle(m_hThread);
#endif
	}
	EndDialog(wParam);
	return 0;
}

LRESULT CThreadDlg::OnNcHitTest(CPoint pt)
{
	LRESULT nHitTest = CDialog::OnNcHitTest(pt);
	if ( nHitTest==HTCLIENT && GetAsyncKeyState(MK_LBUTTON)<0 )
		nHitTest = HTCAPTION;
	return nHitTest;
}
