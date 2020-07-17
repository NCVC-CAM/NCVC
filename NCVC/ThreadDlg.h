// ThreadDlg.h : �w�b�_�[ �t�@�C��
//

#pragma once

// �گ�ފ֐�
UINT NCDtoXYZ_Thread(LPVOID);
UINT CorrectCalc_Thread(LPVOID);
UINT MakeNCD_Thread(LPVOID);
UINT ShapeSearch_Thread(LPVOID);
UINT AutoWorkingSet_Thread(LPVOID);

class CThreadDlg;
// �گ�ނւ̈���
typedef struct tagNCVCTHREADPARAM {
	CThreadDlg*	pParent;
	CDocument*	pDoc;
	WPARAM		wParam;		// �e�گ�ޓƎ����
	LPARAM		lParam;
} NCVCTHREADPARAM, *LPNCVCTHREADPARAM;

/////////////////////////////////////////////////////////////////////////////
// CThreadDlg �_�C�A���O

class CThreadDlg : public CDialog
{
	int		m_nID;
	NCVCTHREADPARAM		m_paramThread;		// �گ�ނւ̈���

	CWinThread*	m_pThread;
	BOOL		m_bThread;		// �گ�ތp���׸�

// �R���X�g���N�V����
public:
	CThreadDlg(int, CDocument*, WPARAM = NULL, LPARAM = NULL);

// �I�y���[�V����
	BOOL	IsThreadContinue(void) {
		return m_bThread;
	}
	void	SetFaseMessage(LPCTSTR lpszMsg1, LPCTSTR lpszMsg2 = NULL) {
		// CString�^��UpdataData(FALSE)���Ăяo���ƁC
		// Thread�֐�����̱����ű��Ĵװ�ɂȂ�
		if ( lpszMsg1 )
			m_ctMsgText1.SetWindowText(lpszMsg1);
		if ( lpszMsg2 )
			m_ctMsgText2.SetWindowText(lpszMsg2);
		m_ctReadProgress.SetPos(0);
	}

// �_�C�A���O �f�[�^
	//{{AFX_DATA(CThreadDlg)
	enum { IDD = IDD_THREADDLG };
	CStatic	m_ctMsgText2;
	CStatic	m_ctMsgText1;
	CProgressCtrl	m_ctReadProgress;
	//}}AFX_DATA

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B
	//{{AFX_VIRTUAL(CThreadDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
protected:

	// �������ꂽ���b�Z�[�W �}�b�v�֐�
	//{{AFX_MSG(CThreadDlg)
	virtual BOOL OnInitDialog();
	virtual void OnCancel();
	afx_msg LRESULT OnNcHitTest(CPoint point);
	//}}AFX_MSG
	afx_msg LRESULT OnUserFinish(WPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()
};
