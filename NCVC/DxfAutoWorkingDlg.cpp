// DxfAutoWorkingDlg.cpp : �C���v�������e�[�V���� �t�@�C��
//

#include "stdafx.h"
#include "NCVC.h"
#include "DxfAutoWorkingDlg.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

BEGIN_MESSAGE_MAP(CDxfAutoWorkingDlg, CDialog)
	//{{AFX_MSG_MAP(CDxfAutoWorkingDlg)
	ON_BN_CLICKED(IDC_AUTO_SELECT_OUTLINE, OnBnClickedSelect)
	ON_BN_CLICKED(IDC_AUTO_SELECT_POCKET, OnBnClickedSelect)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDxfAutoWorkingDlg �_�C�A���O

CDxfAutoWorkingDlg::CDxfAutoWorkingDlg(double dOffset, BOOL bAcute)
	: CDialog(CDxfAutoWorkingDlg::IDD, NULL)
{
	//{{AFX_DATA_INIT(CDxfAutoWorkingDlg)
	m_nSelect = 0;
	m_nDetail = 0;
	m_dOffset = dOffset;
	m_bAcuteRound = bAcute;
	//}}AFX_DATA_INIT
}

void CDxfAutoWorkingDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDxfAutoWorkingDlg)
	DDX_Control(pDX, IDC_AUTO_OFFSET, m_ctOffset);
	DDX_Control(pDX, IDC_AUTO_ACUTEROUND, m_ctAcuteRound);
	DDX_Check(pDX, IDC_AUTO_ACUTEROUND, m_bAcuteRound);
	DDX_Radio(pDX, IDC_AUTO_SELECT_OUTLINE, m_nSelect);
	DDX_Radio(pDX, IDC_AUTOSHAPE_1, m_nDetail);
	//}}AFX_DATA_MAP
	for ( int i=0; i<SIZEOF(m_ctDetail); i++ )
		DDX_Control(pDX, IDC_AUTOSHAPE_1+i, m_ctDetail[i]);
}

void CDxfAutoWorkingDlg::SetDetailCtrl(void)
{
	BOOL	bActive[SIZEOF(m_ctDetail)];
	if ( m_nSelect == 0 ) {		// �֊s���H
		bActive[0] = TRUE;	// �����֊s����
		bActive[1] = FALSE;	// �����߹�ď���
		bActive[2] = TRUE;	// �S������
		bActive[3] = TRUE;	// �S���O��
	}
	else {						// �߹�ĉ��H
		bActive[0] = FALSE;
		bActive[1] = TRUE;
		bActive[2] = TRUE;
		bActive[3] = FALSE;
	}
	for ( int i=0; i<SIZEOF(m_ctDetail); i++ )
		m_ctDetail[i].EnableWindow(bActive[i]);
	m_ctAcuteRound.EnableWindow(bActive[0]);	// �s�p�ۂߏ����͎����֊s�����ɏ���
}

/////////////////////////////////////////////////////////////////////////////
// CDxfAutoWorkingDlg ���b�Z�[�W �n���h��

BOOL CDxfAutoWorkingDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	m_ctOffset = m_dOffset;	// ��̫�ĵ̾��
	SetDetailCtrl();
	return TRUE;
}

void CDxfAutoWorkingDlg::OnOK() 
{
	UpdateData();
	if ( m_ctOffset <= 0 ) {
		AfxMessageBox(IDS_ERR_UNDERZERO, MB_OK|MB_ICONEXCLAMATION);
		m_ctOffset.SetFocus();
		m_ctOffset.SetSel(0, -1);
		return;
	}
	m_dOffset = m_ctOffset;
	EndDialog(IDOK);
}

void CDxfAutoWorkingDlg::OnBnClickedSelect()
{
	UpdateData();
	SetDetailCtrl();
}
