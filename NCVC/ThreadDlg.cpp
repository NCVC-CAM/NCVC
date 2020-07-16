// ThreadDlg.cpp : �C���v�������e�[�V���� �t�@�C��
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
// CThreadDlg �_�C�A���O

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
// CThreadDlg ���b�Z�[�W �n���h��

BOOL CThreadDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	CWinThread*		pThread;
	m_ctReadProgress.SetPos(0);

	switch ( m_nID ) {
	case IDS_READ_NCD:			// NC�ް������ϊ��گ�ފJ�n
		pThread = AfxBeginThread(NCDtoXYZ_Thread, &m_paramThread,
			THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
		break;

	case IDS_CORRECT_NCD:		// �␳���W�v�Z�گ�ފJ�n
		pThread = AfxBeginThread(CorrectCalc_Thread, &m_paramThread,
			THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
		break;

	case ID_FILE_DXF2NCD:		// NC�����گ�ފJ�n
		pThread = AfxBeginThread(MakeNCD_Thread, &m_paramThread,
			THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
		break;

	case ID_EDIT_DXFSHAPE:		// �A����޼ު�Ă̌����گ�ފJ�n
		pThread = AfxBeginThread(ShapeSearch_Thread, &m_paramThread,
			THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
		break;

	case ID_EDIT_SHAPE_AUTO:	// �������H�w���گ�ފJ�n
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

	return TRUE;  // �R���g���[���Ƀt�H�[�J�X��ݒ肵�Ȃ��Ƃ��A�߂�l�� TRUE �ƂȂ�܂�
	              // ��O: OCX �v���p�e�B �y�[�W�̖߂�l�� FALSE �ƂȂ�܂�
}

void CThreadDlg::OnCancel() 
{
#ifdef _DEBUG
	CMagaDbg	dbg("CThreadDlg::OnCancel()\nStart", DBG_RED);
#endif
	m_bThread = FALSE;		// ��ݾ��׸ނ̂�
							// ������WaitForSingleObject()���Ăяo����
							// ��۸�ڽ�ް�Ȃǂ̍X�Vү���ނ��ޯ��ۯ��������N����
//	CDialog::OnCancel();	// �e�گ�ނ�� OnUserFinish��PostMessage()
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
