// NCWorkDlg1.cpp : �C���v�������e�[�V���� �t�@�C��
//

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "NCDoc.h"
#include "NCWorkDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CNCWorkDlg1, CPropertyPage)
	ON_BN_CLICKED(IDC_WORK_SHOW, &CNCWorkDlg1::OnShow)
	ON_BN_CLICKED(IDC_WORK_HIDE, &CNCWorkDlg1::OnHide)
	ON_BN_CLICKED(IDC_WORK_RECOVER, &CNCWorkDlg1::OnRecover)
	ON_BN_CLICKED(IDC_WORK_COMMENT, &CNCWorkDlg1::OnComment)
	ON_MESSAGE(WM_USERSWITCHDOCUMENT, &CNCWorkDlg1::OnUserSwitchDocument)
END_MESSAGE_MAP()

#define	GetParentSheet()	static_cast<CNCWorkDlg *>(GetParentSheet())

/////////////////////////////////////////////////////////////////////////////
// CNCWorkDlg1 �_�C�A���O

CNCWorkDlg1::CNCWorkDlg1() : CPropertyPage(CNCWorkDlg1::IDD)
{
//	m_psp.dwFlags &= ~PSP_HASHELP;
}

void CNCWorkDlg1::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNCWorkDlg1)
	DDX_Control(pDX, IDC_WORK_SHOW, m_ctShow);
	DDX_Control(pDX, IDC_WORK_HIDE, m_ctHide);
	DDX_Control(pDX, IDC_WORK_RECOVER, m_ctRecover);
	DDX_Control(pDX, IDC_WORK_COMMENT, m_ctComment);
	//}}AFX_DATA_MAP
	int		i, j, n;
	for ( i=0; i<SIZEOF(m_ctWork); i++ ) {
		for ( j=0; j<NCXYZ; j++ ) {
			n = i * NCXYZ + j;
			DDX_Control(pDX, n+IDC_WORK_X,  m_ctWork[i][j]);
			DDX_Control(pDX, n+IDC_WORK_XS, m_ctLabel[i][j]);
		}
	}
}

void CNCWorkDlg1::EnableButton(BOOL bEnable, BOOL bLathe)
{
	static	LPCTSTR	szLabel[][NCXYZ] = {
		{"��(&W)", "���s(&D)", "����(&H)"},
		{"�a(&D)", "Zmin(&L)", "Zmax(&R)"}
	};
	int		i, n = (bEnable & bLathe) ? 1 : 0;

	m_ctShow.EnableWindow(bEnable);
	m_ctHide.EnableWindow(bEnable);
	m_ctRecover.EnableWindow(bEnable);
	m_ctComment.EnableWindow(bEnable);

	for ( i=0; i<NCXYZ; i++ ) {
		// �傫���w���� bEnable
		m_ctWork[0][i].EnableWindow(bEnable);
		// �̾�Ďw���� bEnable & !bLathe
		m_ctWork[1][i].EnableWindow(bEnable & !bLathe);
		// �̾�����ق� !bLathe
		m_ctLabel[1][i].EnableWindow(!bLathe);
		// ���ٕύX
		m_ctLabel[0][i].SetWindowText(szLabel[n][i]);
	}
}

void CNCWorkDlg1::SetValue(const CNCDoc* pDoc, const CRect3F& rc)
{
	if ( pDoc->IsDocFlag(NCDOC_LATHE) ) {
		m_ctWork[0][NCA_X] = rc.high * 2.0f;	// ���a�\��
		m_ctWork[0][NCA_Y] = rc.left;
		m_ctWork[0][NCA_Z] = rc.right;
		m_ctWork[1][NCA_X] = 0;
		m_ctWork[1][NCA_Y] = 0;
		m_ctWork[1][NCA_Z] = 0;
	}
	else {
		m_ctWork[0][NCA_X] = rc.Width();
		m_ctWork[0][NCA_Y] = rc.Height();
		m_ctWork[0][NCA_Z] = rc.Depth();
		m_ctWork[1][NCA_X] = rc.left;
		m_ctWork[1][NCA_Y] = rc.top;
		m_ctWork[1][NCA_Z] = rc.low;
	}
	UpdateData(FALSE);
}

/////////////////////////////////////////////////////////////////////////////
// CNCWorkDlg1 ���b�Z�[�W �n���h��

BOOL CNCWorkDlg1::OnInitDialog() 
{
	extern	LPCTSTR	g_szNdelimiter;	// "XYZUVWIJKRPLDH" from NCDoc.cpp

	__super::OnInitDialog();
	int		i, j;

	// �ݽ�׸��ł͏������ł��Ȃ����۰ق̏�����
	CString	strRegKey(GetSubTreeRegKey(IDS_REGKEY_NC, IDS_REGKEY_WINDOW_WORKDLG)),	// stdafx.h
			strTmp;

	// ڼ޽�؂ɕۑ����ꂽܰ���`���͍폜
	for ( i=0; i<SIZEOF(m_ctWork); i++ ) {
		for ( j=0; j<NCXYZ; j++ )
			AfxGetApp()->WriteProfileString(strRegKey, g_szNdelimiter[j]+strTmp, NULL);
		strTmp = 'O';
	}

	// �\��/��\�����݂̏�����
	OnUserSwitchDocument(NULL, NULL);

	return TRUE;  // �R���g���[���Ƀt�H�[�J�X��ݒ肵�Ȃ��Ƃ��A�߂�l�� TRUE �ƂȂ�܂�
	              // ��O: OCX �v���p�e�B �y�[�W�̖߂�l�� FALSE �ƂȂ�܂�
}

BOOL CNCWorkDlg1::PreTranslateMessage(MSG* pMsg)
{
	// CPropertySheetӰ��ڽ��ESC�������Ȃ��̂�
	// �����I��ү���ނ�߂܂��ĕ���
	if ( pMsg->message == WM_KEYDOWN ) {
		switch ( pMsg->wParam ) {
		case VK_ESCAPE:
			GetParentSheet()->PostMessage(WM_CLOSE);
			return TRUE;
		case VK_RETURN:
			OnShow();
			return TRUE;
		}
	}
	return CPropertyPage::PreTranslateMessage(pMsg);
}

void CNCWorkDlg1::OnShow() 
{
	CWaitCursor		wait;

	// �޷�����ޭ��ւ̕ύX�ʒm
	// ���̲���Ă���������Ƃ��́COnUserSwitchDocument() ���
	// �K�� AfxGetNCVCMainWnd()->MDIGetActive() �� CNCChild ���w���Ă���
	CNCDoc*	pDoc = GetParentSheet()->GetNCDocument();
	if ( pDoc ) {
		CRect3F	rc;
		if ( pDoc->IsDocFlag(NCDOC_LATHE) ) {
			rc.high   = m_ctWork[0][NCA_X] / 2.0f;	// �a
			rc.left   = m_ctWork[0][NCA_Y];			// Zmin
			rc.right  = m_ctWork[0][NCA_Z];			// Zmax
		}
		else {
			rc.right  = m_ctWork[0][NCA_X];			// ��
			rc.bottom = m_ctWork[0][NCA_Y];			// ���s
			rc.high   = m_ctWork[0][NCA_Z];			// ����
			rc.OffsetRect(m_ctWork[1][NCA_X], m_ctWork[1][NCA_Y], m_ctWork[1][NCA_Z]);	// X, Y, Z �̾��
		}
		// ���X�V
		pDoc->SetWorkRect(TRUE, rc);
	}
}

void CNCWorkDlg1::OnHide() 
{
	CNCDoc*	pDoc = GetParentSheet()->GetNCDocument();
	if ( pDoc )
		pDoc->SetWorkRect(FALSE, CRect3F());
}

void CNCWorkDlg1::OnComment()
{
	extern	LPCTSTR	g_szNCcomment[];	// "WorkRect", "LatheView" from NCDoc.cpp
	CNCDoc*	pDoc = GetParentSheet()->GetNCDocument();
	if ( pDoc ) {
		CString	strComment;
		if ( pDoc->IsDocFlag(NCDOC_LATHE) ) {
			strComment.Format("(%s=%.3f,%.3f,%.3f)",
				LATHEVIEW_S,
				float(m_ctWork[0][NCA_X]), float(m_ctWork[0][NCA_Y]), float(m_ctWork[0][NCA_Z]));
		}
		else {
			strComment.Format("(%s=%.3f,%.3f,%.3f,%.3f,%.3f,%.3f)",
				WORKRECT_S,
				float(m_ctWork[0][NCA_X]), float(m_ctWork[0][NCA_Y]), float(m_ctWork[0][NCA_Z]),
				float(m_ctWork[1][NCA_X]), float(m_ctWork[1][NCA_Y]), float(m_ctWork[1][NCA_Z]));
		}
		pDoc->SetCommentStr(strComment);
		OnShow();	// �\���̍X�V
	}
}

void CNCWorkDlg1::OnRecover()
{
	CNCDoc*	pDoc = GetParentSheet()->GetNCDocument();
	if ( pDoc )
		SetValue(pDoc, pDoc->GetWorkRectOrg());		// Org
}

LRESULT CNCWorkDlg1::OnUserSwitchDocument(WPARAM, LPARAM)
{
#ifdef _DEBUG
	printf("CNCWorkDlg1::OnUserSwitchDocument() Calling\n");
#endif
	CNCDoc*	pDoc = GetParentSheet()->GetNCDocument();
	if ( pDoc ) {
		// ܲԕ��d���H�@Ӱ�ނ͕s��
		EnableButton(!pDoc->IsDocFlag(NCDOC_WIRE), pDoc->IsDocFlag(NCDOC_LATHE));
		SetValue(pDoc, pDoc->GetWorkRect());
	}
	else
		EnableButton(FALSE, FALSE);

	return 0;
}
