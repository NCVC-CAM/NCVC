// ThreadDlg.cpp : �C���v�������e�[�V���� �t�@�C��
//

#include "stdafx.h"
#include "NCVC.h"
#include "ThreadDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BEGIN_MESSAGE_MAP(CThreadDlg, CDialog)
	//{{AFX_MSG_MAP(CThreadDlg)
	ON_WM_NCHITTEST()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_USERFINISH, &CThreadDlg::OnUserFinish)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CThreadDlg �_�C�A���O

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
// CThreadDlg ���b�Z�[�W �n���h��

BOOL CThreadDlg::OnInitDialog() 
{
	__super::OnInitDialog();

	AFX_THREADPROC	pfnThread;

	switch ( m_nID ) {
	case IDS_READ_NCD:			// NC�ް������ϊ��گ�ފJ�n
		pfnThread = NCDtoXYZ_Thread;
		break;

	case IDS_CORRECT_NCD:		// �␳���W�v�Z�گ�ފJ�n
		pfnThread = CorrectCalc_Thread;
		break;

	case IDS_UVTAPER_NCD:		// ܲԉ��H�pUV����޼ު�Ă̐���
		pfnThread = UVWire_Thread;
		break;

	case ID_FILE_DXF2NCD:		// NC�����گ�ފJ�n
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

	case ID_FILE_3DPATH:		// Nurbs�Ȗʂ�NC�����X���b�h
		pfnThread = MakeNurbs_Thread;
		break;

	case ID_EDIT_DXFSHAPE:		// �A����޼ު�Ă̌����گ�ފJ�n
		pfnThread = ShapeSearch_Thread;
		break;

	case ID_EDIT_SHAPE_AUTO:	// �������H�w���گ�ފJ�n
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

	return TRUE;  // �R���g���[���Ƀt�H�[�J�X��ݒ肵�Ȃ��Ƃ��A�߂�l�� TRUE �ƂȂ�܂�
	              // ��O: OCX �v���p�e�B �y�[�W�̖߂�l�� FALSE �ƂȂ�܂�
}

void CThreadDlg::OnCancel() 
{
#ifdef _DEBUG
	printf("CThreadDlg::OnCancel() Start\n");
#endif
	m_bThread = FALSE;		// ��ݾ��׸ނ̂�
							// ������WaitForSingleObject()���Ăяo����
							// ��۸�ڽ�ް�Ȃǂ̍X�Vү���ނ��ޯ��ۯ��������N����
//	__super::OnCancel();	// �e�گ�ނ�� OnUserFinish��PostMessage()
}

LRESULT CThreadDlg::OnUserFinish(WPARAM wParam, LPARAM)
{
	if ( m_pThread ) {
#ifdef _DEBUG
		printf("CThreadDlg::OnUserFinish()\n");
		if ( ::WaitForSingleObject(m_pThread->m_hThread, INFINITE) == WAIT_FAILED ) {
			printf("WaitForSingleObject() Fail!\n");
			::NC_FormatMessage();
		}
		else
			printf("WaitForSingleObject() OK\n");
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

/////////////////////////////////////////////////////////////////////////////

// ̪��ޏo��
void SendFaseMessage
	(CThreadDlg* pParent, int& nFase, INT_PTR nRange/*=-1*/, int nMsgID/*=-1*/, LPCTSTR lpszMsg/*=NULL*/)
{
#ifdef _DEBUG
	printf("MakeXXX_Thread() Phase%d Start\n", nFase);
#endif
	if ( nRange > 0 )
		pParent->m_ctReadProgress.SetRange32(0, (int)nRange);

	CString	strMsg;
	if ( nMsgID > 0 )
		VERIFY(strMsg.LoadString(nMsgID));
	else
		strMsg.Format(IDS_MAKENCD_FASE, nFase);
	pParent->SetFaseMessage(strMsg, lpszMsg);
	nFase++;
}

void SetProgressPos64(CThreadDlg* pParent, INT_PTR n)
{
	if ((n & 0x003f) == 0)	// 64�񂨂�(����6�ޯ�Ͻ�)
		pParent->m_ctReadProgress.SetPos((int)n);
}

void SetProgressPos(CThreadDlg* pParent, INT_PTR n)
{
	pParent->m_ctReadProgress.SetPos((int)n);
}
