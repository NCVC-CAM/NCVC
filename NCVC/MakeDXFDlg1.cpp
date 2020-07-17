// MakeDXFDlg1.cpp : �C���v�������e�[�V���� �t�@�C��
//

#include "stdafx.h"
#include "NCVC.h"
#include "Layer.h"
#include "NCDoc.h"
#include "MakeDXFDlg.h"
#include "MakeNCDlg.h"		// �����޲�۸ދ��ʊ֐���`
#include "ViewOption.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

BEGIN_MESSAGE_MAP(CMakeDXFDlg1, CPropertyPage)
	//{{AFX_MSG_MAP(CMakeDXFDlg1)
	ON_BN_CLICKED(IDC_MKNC_NCFILEUP, &CMakeDXFDlg1::OnMKDXFileUp)
	ON_EN_KILLFOCUS(IDC_MKNC_NCFILE, &CMakeDXFDlg1::OnKillFocusDXFFile)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// �װ�����ޯ���ւ̏��(DXF�װ���ނ̏���)
static	const	COLORREF	g_dwColor[] = {
	RGB(255,0,0), RGB(255,255,0), RGB(0,255,0),		// ��, ��, ��
	RGB(0,255,255), RGB(0,0,255), RGB(255,0,255),	// ��(Cyan), ��, ��(Magenta)
	RGB(255,255,255)								// ��
};

#define	GetParentSheet()	static_cast<CMakeDXFDlg *>(GetParentSheet())

/////////////////////////////////////////////////////////////////////////////
// CMakeDXFDlg1 �v���p�e�B �y�[�W

CMakeDXFDlg1::CMakeDXFDlg1() : CPropertyPage(CMakeDXFDlg1::IDD)
{
	m_psp.dwFlags &= ~PSP_HASHELP;
	//{{AFX_DATA_INIT(CMakeDXFDlg1)
	m_nPlane = -1;
	m_bOut[0] = FALSE;
	m_bOut[1] = FALSE;
	m_bOut[2] = FALSE;
	m_bOut[3] = FALSE;
	//}}AFX_DATA_INIT
}

void CMakeDXFDlg1::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMakeDXFDlg1)
	DDX_Control(pDX, IDC_MKNC_NCFILE, m_ctDXFFileName);
	DDX_Text(pDX, IDC_MKNC_NCFILE, m_strDXFFileName);
	DDX_CBIndex(pDX, IDC_MKDX1_PLANE, m_nPlane);
	//}}AFX_DATA_MAP
	DDX_Text(pDX, IDC_DXF_ORIGIN,  m_strLayer[0]);	// IDC_ ���A���łȂ��̂� for() �g���Ȃ�
	DDX_Text(pDX, IDC_DXF_CAMLINE, m_strLayer[1]);
	DDX_Text(pDX, IDC_DXF_MOVE,    m_strLayer[2]);
	DDX_Text(pDX, IDC_DXF_CORRECT, m_strLayer[3]);
	DDX_Control(pDX, IDC_VIEWSETUP3_CB_ORIGIN,  m_cbLineType[0]);
	DDX_Control(pDX, IDC_VIEWSETUP3_CB_CUTTER,  m_cbLineType[1]);
	DDX_Control(pDX, IDC_VIEWSETUP3_CB_MOVE,    m_cbLineType[2]);
	DDX_Control(pDX, IDC_VIEWSETUP3_CB_OUTLINE, m_cbLineType[3]);
	DDX_Control(pDX, IDC_MKDX1_COLOR_O, m_ctColor[0]);
	DDX_Control(pDX, IDC_MKDX1_COLOR_C, m_ctColor[1]);
	DDX_Control(pDX, IDC_MKDX1_COLOR_M, m_ctColor[2]);
	DDX_Control(pDX, IDC_MKDX1_COLOR_H, m_ctColor[3]);
	DDX_Check(pDX, IDC_MKDX1_OUT_O, m_bOut[0]);
	DDX_Check(pDX, IDC_MKDX1_OUT_C, m_bOut[1]);
	DDX_Check(pDX, IDC_MKDX1_OUT_M, m_bOut[2]);
	DDX_Check(pDX, IDC_MKDX1_OUT_H, m_bOut[3]);
}

/////////////////////////////////////////////////////////////////////////////
// CMakeDXFDlg1 ���b�Z�[�W �n���h��

BOOL CMakeDXFDlg1::OnInitDialog() 
{
	__super::OnInitDialog();

	int		i, j;
	CMakeDXFDlg*	pParent = GetParentSheet();
	// �ݽ�׸��ł͂ł��Ȃ����۰ق̏�����
	CDXFMakeOption*	pDXFMake = pParent->GetDXFMakeOption();

	// �޷���Ė�����DXF̧�ٖ����쐬(GetParentSheet()�ͺݽ�׸��Ŏg���Ȃ�)
	CString	strDXFFile(pParent->GetDoc()->GetDXFFileName());
	if ( strDXFFile.IsEmpty() ) {
		::Path_Name_From_FullPath(pParent->GetDoc()->GetPathName(),
				m_strDXFPath, m_strDXFFileName, FALSE);
		CString	strFilter;
		VERIFY(strFilter.LoadString(IDS_DXF_FILTER));
		m_strDXFFileName += '.' + strFilter.Left(3);	// .dxf
	}
	else
		::Path_Name_From_FullPath(strDXFFile, m_strDXFPath, m_strDXFFileName);

	// ڲԏ��C���������߂̑I�����o�^
	extern	const	PENSTYLE	g_penStyle[];		// ViewOption.cpp
	for ( i=0; i<SIZEOF(m_strLayer); i++ ) {
		// �o�͏��
		m_bOut[i] = pDXFMake->m_bOut[i];
		// ڲԖ�
		m_strLayer[i] = pDXFMake->m_strOption[i];
		// ����
		for ( j=0; j<MAXPENSTYLE; j++ )
			m_cbLineType[i].AddString(g_penStyle[j].lpszPenName);
		m_cbLineType[i].SetCurSel(pDXFMake->m_nLType[i]);
		// �F
		for ( j=0; j<SIZEOF(g_dwColor); j++ ) 
			m_ctColor[i].AddString( (LPCTSTR)g_dwColor[j] );
		m_ctColor[i].SetCurSel(pDXFMake->m_nLColor[i]);
		m_ctColor[i].SetItemHeight(-1, m_cbLineType[i].GetItemHeight(-1));
	}
	m_nPlane = pDXFMake->m_nPlane;

	UpdateData(FALSE);

	// �߽�\���̍œK��(shlwapi.h)
	::PathSetDlgItemPath(m_hWnd, IDC_MKNC_NCPATH, m_strDXFPath);

	return TRUE;  // �R���g���[���Ƀt�H�[�J�X��ݒ肵�Ȃ��Ƃ��A�߂�l�� TRUE �ƂȂ�܂�
	              // ��O: OCX �v���p�e�B �y�[�W�̖߂�l�� FALSE �ƂȂ�܂�
}

void CMakeDXFDlg1::OnKillFocusDXFFile() 
{
	UpdateData();
	MakeDlgKillFocus(m_strDXFPath, m_strDXFFileName, this, IDC_MKNC_NCPATH);	// MakeNCDlg.cpp
}

void CMakeDXFDlg1::OnMKDXFileUp() 
{
	UpdateData();
	CString	strFilter;
	VERIFY(strFilter.LoadString(IDS_DXF_FILTER));
	MakeDlgFileRefer(IDS_NCD2DXF_FILE, strFilter, this, IDC_MKNC_NCPATH,
		m_strDXFPath, m_strDXFFileName, FALSE);
	// �����I�����
	m_ctDXFFileName.SetFocus();
	m_ctDXFFileName.SetSel(0, -1);
}

BOOL CMakeDXFDlg1::OnApply() 
{
	CDXFMakeOption*	pDXFMake = GetParentSheet()->GetDXFMakeOption();
	for ( int i=0; i<SIZEOF(m_strLayer); i++ ) {
		pDXFMake->m_bOut[i]			= m_bOut[i];
		pDXFMake->m_strOption[i]	= m_strLayer[i];
		pDXFMake->m_nLType[i]		= m_cbLineType[i].GetCurSel();
		pDXFMake->m_nLColor[i]		= m_ctColor[i].GetCurSel();
	}
	pDXFMake->m_nPlane = m_nPlane;

	return TRUE;
}

BOOL CMakeDXFDlg1::OnKillActive() 
{
	if ( !__super::OnKillActive() )
		return FALSE;

	BOOL	bCheck = TRUE;
	for ( int i=0; i<SIZEOF(m_bOut); i++ ) {
		if ( m_bOut[i] ) {
			bCheck = FALSE;
			break;
		}
	}
	if ( bCheck ) {
		AfxMessageBox(IDS_ERR_MAKEDXFLAYER, MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}

	CString	strDXFFile(m_strDXFPath+m_strDXFFileName);
	if ( !CheckMakeDlgFileExt(TYPE_DXF, strDXFFile) ) {	// MakeNCDlg.cpp
		m_ctDXFFileName.SetFocus();
		m_ctDXFFileName.SetSel(0, -1);
		return FALSE;
	}

	::Path_Name_From_FullPath(strDXFFile, m_strDXFPath, m_strDXFFileName);
	UpdateData(FALSE);
	return TRUE;
}
