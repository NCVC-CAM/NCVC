// DxfEditOrgDlg.cpp : �C���v�������e�[�V���� �t�@�C��
//

#include "stdafx.h"
#include "NCVC.h"
#include "DxfEditOrgDlg.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

BEGIN_MESSAGE_MAP(CDxfEditOrgDlg, CDialog)
	//{{AFX_MSG_MAP(CDxfEditOrgDlg)
	ON_BN_CLICKED(IDC_DXFEDIT_SEL_NUM, OnSelect)
	ON_BN_CLICKED(IDC_DXFEDIT_SEL_RECT, OnSelect)
	ON_BN_CLICKED(IDC_DXFEDIT_SEL_ORIG, OnSelect)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/*
	�I�����F���������̂��߁C(ڼ޽�؂ւ�)�ۑ��͂��Ȃ�
		NCVC�ғ����Ɍ���ێ�����
*/
static	int		ss_nSelect = 0;
static	int		ss_nRectType = 0;
static	CString	ss_strNumeric;

/////////////////////////////////////////////////////////////////////////////
// CDxfEditOrgDlg �_�C�A���O

CDxfEditOrgDlg::CDxfEditOrgDlg(DWORD dwControl)
	: CDialog(CDxfEditOrgDlg::IDD, NULL)
{
	m_dwControl = dwControl;
	//{{AFX_DATA_INIT(CDxfEditOrgDlg)
	//}}AFX_DATA_INIT
	if ( m_dwControl & EDITORG_NUMERIC )
		m_nSelect = 1;	// ��`�w��
	else
		m_nSelect = ss_nSelect;
	m_strNumeric = ss_strNumeric;
	m_nRectType = ss_nRectType;
}

void CDxfEditOrgDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDxfEditOrgDlg)
	DDX_Control(pDX, IDC_DXFEDIT_SEL_NUM, m_ctSelNumeric);
	DDX_Control(pDX, IDC_DXFEDIT_SEL_ORIG, m_ctSelOriginal);
	DDX_Control(pDX, IDC_DXF_ORGTYPE, m_ctRectType);
	DDX_Control(pDX, IDC_DXFEDIT_NUM, m_ctNumeric);
	DDX_Radio(pDX, IDC_DXFEDIT_SEL_NUM, m_nSelect);
	DDX_Text(pDX, IDC_DXFEDIT_NUM, m_strNumeric);
	DDX_CBIndex(pDX, IDC_DXF_ORGTYPE, m_nRectType);
	//}}AFX_DATA_MAP
}

void CDxfEditOrgDlg::SelectControl(int nIndex)
{
	BOOL	bNumeric, bRectType;

	switch ( nIndex ) {
	case 0:		// IDC_DXFEDIT_SEL_NUM
		bNumeric  = TRUE;
		bRectType = FALSE;
		break;
	case 1:		// IDC_DXF_ORGTYPE
		bNumeric  = FALSE;
		bRectType = TRUE;
		break;
	default:
		bNumeric  = FALSE;
		bRectType = FALSE;
		break;
	}
	m_ctNumeric.EnableWindow(bNumeric);
	m_ctRectType.EnableWindow(bRectType);
}

/////////////////////////////////////////////////////////////////////////////
// CDxfEditOrgDlg ���b�Z�[�W �n���h��

BOOL CDxfEditOrgDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	SelectControl( m_nSelect );

	if ( m_dwControl & EDITORG_NUMERIC ) {
		m_ctSelNumeric.EnableWindow(FALSE);
		m_ctNumeric.EnableWindow(FALSE);
	}
	if ( m_dwControl & EDITORG_ORIGINAL )
		m_ctSelOriginal.EnableWindow(FALSE);

	return TRUE;  // �R���g���[���Ƀt�H�[�J�X��ݒ肵�Ȃ��Ƃ��A�߂�l�� TRUE �ƂȂ�܂�
	              // ��O: OCX �v���p�e�B �y�[�W�̖߂�l�� FALSE �ƂȂ�܂�
}

void CDxfEditOrgDlg::OnSelect() 
{
	SelectControl( GetFocus()->GetDlgCtrlID() - IDC_DXFEDIT_SEL_NUM );
}

void CDxfEditOrgDlg::OnOK() 
{
	UpdateData();

	// �ÓI�ϐ��ɋL��
	ss_nSelect = m_nSelect;
	ss_strNumeric = m_strNumeric;
	ss_nRectType = m_nRectType;

//	CDialog::OnOK();
	EndDialog(IDOK);
}
