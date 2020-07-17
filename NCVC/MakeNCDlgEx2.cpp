// MakeNCDlgEx2.cpp : �C���v�������e�[�V���� �t�@�C��
//

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "Layer.h"
#include "DXFDoc.h"
#include "MakeNCDlg.h"
#include "MakeNCDlgEx.h"
#include "MakeNCDlgEx11.h"
#include "MakeNCDlgEx21.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

#define	GetNCMakeParent()	static_cast<CMakeNCDlgEx *>(GetParentSheet())
#define	IsMakeEx1()	( GetNCMakeParent()->GetNCMakeID() == ID_FILE_DXF2NCD_EX1 )
#define	IsMakeEx2()	( GetNCMakeParent()->GetNCMakeID() == ID_FILE_DXF2NCD_EX2 )

BEGIN_MESSAGE_MAP(CMakeNCDlgEx2, CPropertyPage)
	//{{AFX_MSG_MAP(CMakeNCDlgEx2)
	ON_EN_KILLFOCUS(IDC_MKNC_NCFILE, &CMakeNCDlgEx2::OnKillFocusNCFile)
	ON_CBN_SELCHANGE(IDC_MKNCEX_LAYERTOINITFILE, &CMakeNCDlgEx2::OnSelChangeLayerToInit)
	ON_CBN_SELCHANGE(IDC_MKNC_INIT, &CMakeNCDlgEx2::OnSelChangeInit)
	ON_CBN_KILLFOCUS(IDC_MKNC_INIT, &CMakeNCDlgEx2::OnKillFocusInit)
	ON_CBN_KILLFOCUS(IDC_MKNCEX_LAYERTOINITFILE, &CMakeNCDlgEx2::OnKillFocusLayerToInit)
	ON_BN_CLICKED(IDC_MKNC_NCFILEUP, &CMakeNCDlgEx2::OnMKNCFileUp)
	ON_BN_CLICKED(IDC_MKNC_INITUP, &CMakeNCDlgEx2::OnMKNCInitUp)
	ON_BN_CLICKED(IDC_MKNC_INITED, &CMakeNCDlgEx2::OnMKNCInitEdit)
	ON_BN_CLICKED(IDC_MKNCEX_LAYERTOINIT_ED, &CMakeNCDlgEx2::OnMKNCLayerEdit)
	ON_BN_CLICKED(IDC_MKNCEX_LAYERTOINIT_UP, &CMakeNCDlgEx2::OnMKNCLayerUp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMakeNCDlgEx2 �_�C�A���O

CMakeNCDlgEx2::CMakeNCDlgEx2() : CPropertyPage(CMakeNCDlgEx2::IDD)
{
	m_psp.dwFlags &= ~PSP_HASHELP;
	//{{AFX_DATA_INIT(CMakeNCDlgEx2)
	//}}AFX_DATA_INIT
	m_bNCView	= FALSE;
	m_bNewLayer	= FALSE;
}

void CMakeNCDlgEx2::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMakeNCDlgEx2)
	DDX_Control(pDX, IDC_MKNC_NCFILE, m_ctNCFileName);
	DDX_Text(pDX, IDC_MKNC_NCFILE, m_strNCFileName);
	DDX_Control(pDX, IDC_MKNC_INIT, m_ctInitFileName);
	DDX_CBString(pDX, IDC_MKNC_INIT, m_strInitFileName);
	DDX_Control(pDX, IDC_MKNCEX_LAYERTOINITFILE, m_ctLayerToInitFileName);
	DDX_CBString(pDX, IDC_MKNCEX_LAYERTOINITFILE, m_strLayerToInitFileName);
	DDX_Check(pDX, IDC_MKNC_VIEW, m_bNCView);
	//}}AFX_DATA_MAP
	int		i;
	for ( i=0; i<SIZEOF(m_ctInit_st)-1; i++ )
		DDX_Control(pDX, IDC_MKNCEX_INIT_CT1+i, m_ctInit_st[i]);
	DDX_Control(pDX, IDC_MKNC_INITPATH, m_ctInit_st[i]);
	for ( i=0; i<SIZEOF(m_ctInit_bt); i++ )
		DDX_Control(pDX, IDC_MKNC_INITUP+i, m_ctInit_bt[i]);
}

void CMakeNCDlgEx2::CheckNewLayer(void)
{
	if ( GetNCMakeParent()->m_strLayerToInitFileName.CompareNoCase(m_strLayerToInitPath+m_strLayerToInitFileName) != 0 ) {
		::Path_Name_From_FullPath(GetNCMakeParent()->m_strLayerToInitFileName,
				m_strLayerToInitPath, m_strLayerToInitFileName);
		::PathSetDlgItemPath(m_hWnd, IDC_MKNCEX_LAYERTOINITPATH, m_strLayerToInitPath);
		UpdateData(FALSE);
		m_bNewLayer = TRUE;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CMakeNCDlgEx2 ���b�Z�[�W �n���h��

BOOL CMakeNCDlgEx2::OnInitDialog() 
{
	__super::OnInitDialog();

	if ( IsMakeEx1() ) {
		// ڲԂ��Ƃ̕��������Ŏg��Ȃ��؍�������۰ق͔�\��
		int		i;
		for ( i=0; i<SIZEOF(m_ctInit_st); i++ )
			m_ctInit_st[i].ShowWindow(SW_HIDE);
		for ( i=0; i<SIZEOF(m_ctInit_bt); i++ )
			m_ctInit_bt[i].ShowWindow(SW_HIDE);
		m_ctInitFileName.ShowWindow(SW_HIDE);
	}

	// �ݽ�׸��ł� GetNCMakeParent() ���g���Ȃ�
	CDXFOption*	pOpt = AfxGetNCVCApp()->GetDXFOption();
	m_bNCView = pOpt->GetDxfOptFlg(DXFOPT_VIEW);

	// ����޲�۸ދ��ʍ�����̐ݒ�l�擾
	::Path_Name_From_FullPath(GetNCMakeParent()->m_strNCFileName, m_strNCPath, m_strNCFileName);
	if ( !GetNCMakeParent()->m_strLayerToInitFileName.IsEmpty() )
		::Path_Name_From_FullPath(GetNCMakeParent()->m_strLayerToInitFileName,
			m_strLayerToInitPath, m_strLayerToInitFileName);

	// �߽�\���̍œK��
	::PathSetDlgItemPath(m_hWnd, IDC_MKNC_NCPATH, m_strNCPath);
	if ( IsMakeEx2() ) {
		if ( !GetNCMakeParent()->m_strInitFileName.IsEmpty() )
			::Path_Name_From_FullPath(GetNCMakeParent()->m_strInitFileName,
				m_strInitPath, m_strInitFileName);
		// �؍���������ޯ���ɱ��ђǉ�
		InitialMakeNCDlgComboBox(pOpt->GetInitList(NCMAKEMILL), m_ctInitFileName);
		::PathSetDlgItemPath(m_hWnd, IDC_MKNC_INITPATH, m_strInitPath);
	}
	// �֌W̧�ٺ����ޯ���ɱ��ђǉ�
	InitialMakeNCDlgComboBox(pOpt->GetInitList(NCMAKELAYER), m_ctLayerToInitFileName);
	if ( !m_strLayerToInitPath.IsEmpty() )
		::PathSetDlgItemPath(m_hWnd, IDC_MKNCEX_LAYERTOINITPATH, m_strLayerToInitPath);
	if ( !m_strLayerToInitFileName.IsEmpty() ) {
		CString	strFile(m_strLayerToInitPath+m_strLayerToInitFileName);
		if ( ::IsFileExist(strFile, TRUE, FALSE) ) {
			if ( GetNCMakeParent()->GetDocument()->ReadLayerMap(strFile) ) {
				// ڲԏ��̕��בւ�
				GetNCMakeParent()->GetDocument()->UpdateLayerSequence();
				m_ctLayerToInitFileName.SetCurSel(0);
			}
			else {
				// �����폜(�����ޯ��������폜)
				AfxGetNCVCApp()->GetDXFOption()->DelInitHistory(NCMAKELAYER, strFile);
				int	nIndex = m_ctLayerToInitFileName.FindString(-1, m_strLayerToInitFileName);
				if ( nIndex != CB_ERR )
					m_ctLayerToInitFileName.DeleteString(nIndex);
			}
		}
	}

	UpdateData(FALSE);

	return TRUE;  // �R���g���[���Ƀt�H�[�J�X��ݒ肵�Ȃ��Ƃ��A�߂�l�� TRUE �ƂȂ�܂�
	              // ��O: OCX �v���p�e�B �y�[�W�̖߂�l�� FALSE �ƂȂ�܂�
}

BOOL CMakeNCDlgEx2::OnSetActive()
{
	// �u���ցv���݂̂ݗL��
	GetNCMakeParent()->SetWizardButtons(PSWIZB_NEXT);
	// MakeNCDlgEx3 �ŐV�K�ۑ�����Ă���ꍇ�ɔ�����
	// MakeNCDlgEx3 ����߂�ꍇ�����肤��
	CheckNewLayer();

	return TRUE;
}

LRESULT CMakeNCDlgEx2::OnWizardNext()
{
	UpdateData();
	CString	strInit(GetInitFileName()), strFile(GetNCFileName());

	// NC����̧�ق�����(�㏑���m�F�� CMakeNCDlgEx2::OnWizardFinish() �ɂ�)
	if ( !CheckMakeDlgFileExt(TYPE_NCD, strFile) ) {
		m_ctNCFileName.SetFocus();
		m_ctNCFileName.SetSel(0, -1);
		return -1;
	}

	// �؍����̧�ق�����
	if ( IsMakeEx2() && !::IsFileExist(strInit) ) {
		m_ctInitFileName.SetEditSel(0, -1);
		m_ctInitFileName.SetFocus();
		return -1;
	}

	// �޲�۸ދ��ʍ���̨����ޯ�
	GetNCMakeParent()->m_strNCFileName = strFile;
	if ( IsMakeEx2() )
		GetNCMakeParent()->m_strInitFileName = strInit;
	if ( m_strLayerToInitFileName.IsEmpty() )
		GetNCMakeParent()->m_strLayerToInitFileName.Empty();
	else {
		strFile = m_strLayerToInitPath + m_strLayerToInitFileName;
		if ( GetNCMakeParent()->m_strLayerToInitFileName.CompareNoCase(strFile) != 0 ) {
			GetNCMakeParent()->m_strLayerToInitFileName = strFile;
			m_bNewLayer = TRUE;		// ReadLayerMap������
		}
		if ( m_bNewLayer && ::IsFileExist(strFile, TRUE, FALSE) ) {
			// �ް��̔��f�Cڲ�ؽ��ް��̕ύX
			GetNCMakeParent()->GetDocument()->ReadLayerMap(strFile);
		}
	}

	return 0;	// �����߰�ނ�
}

void CMakeNCDlgEx2::OnMKNCFileUp() 
{
	UpdateData();
	MakeDlgFileRefer(IDS_DXF2NCD_FILE, AfxGetNCVCApp()->GetFilterString(TYPE_NCD),
			this, IDC_MKNC_NCPATH, m_strNCPath, m_strNCFileName, FALSE);
	// �����I�����
	m_ctNCFileName.SetFocus();
	m_ctNCFileName.SetSel(0, -1);
}

void CMakeNCDlgEx2::OnMKNCInitUp() 
{
	UpdateData();
	CString	strFilter;
	VERIFY(strFilter.LoadString(IDS_NCIM_FILTER));
	MakeDlgFileRefer(IDS_OPTION_INIT, strFilter, this, IDC_MKNC_INITPATH,
			m_strInitPath, m_strInitFileName, TRUE);
	// �����I�����
	m_ctInitFileName.SetFocus();
	m_ctInitFileName.SetEditSel(0, -1);
}

void CMakeNCDlgEx2::OnMKNCLayerUp() 
{
	UpdateData();
	CString	strFilter;
	VERIFY(strFilter.LoadString(IDS_NCL_FILTER));
	CString	strResult( MakeDlgFileRefer(IDS_OPTION_LAYER2INIT, strFilter,
				this, IDC_MKNCEX_LAYERTOINITPATH, 
				m_strLayerToInitPath, m_strLayerToInitFileName, TRUE) );
	if ( strResult.IsEmpty() )
		return;
	// �����I�����
	m_ctLayerToInitFileName.SetFocus();
	m_ctLayerToInitFileName.SetEditSel(0, -1);
}

void CMakeNCDlgEx2::OnMKNCInitEdit() 
{
	UpdateData();
	MakeNCDlgInitFileEdit(m_strInitPath, m_strInitFileName,
		this, IDC_MKNC_INITPATH, m_ctInitFileName);
}

void CMakeNCDlgEx2::OnMKNCLayerEdit() 
{
	UpdateData();
	CLayerArray*	pArray;
	CMakeNCDlgEx11	dlg1(GetNCMakeParent(), 0);	// �޽�׸���dlg.m_obLayer�̓��e���������̂�
	CMakeNCDlgEx21	dlg2(GetNCMakeParent(), 0);	// if�����ɂ͓�����Ȃ�

	// �ڍאݒ�p�޲�۸ނ̌Ăяo��
	if ( IsMakeEx1() ) {
		if ( dlg1.DoModal() != IDOK )
			return;
		pArray = &(dlg1.m_obLayer);
	}
	else {
		if ( dlg2.DoModal() != IDOK )
			return;
		pArray = &(dlg2.m_obLayer);
	}

	// ���ʂ̔��f
	if ( ::IsWindow(GetNCMakeParent()->m_dlg2.m_hWnd) ) {
		// CMakeNCDlgEx3 �֌�����ү���ޑ��M�����A
		// �߰�ނ���è�ނɂ���Ȃ��ƁAү���ޑ��M����Ȃ�
		QuerySiblings(0, (LPARAM)pArray);
	}
	else {
		CLayerData*	pLayer1;
		CLayerData*	pLayer2;
		CDXFDoc*	pDoc = GetNCMakeParent()->GetDocument();
		int			i, j;
		for ( i=0, j=0; i<pDoc->GetLayerCnt(); i++ ) {
			pLayer1 = pDoc->GetLayerData(i);
			if ( pLayer1->IsCutType() ) {
				pLayer2 = pArray->GetAt(j++);
				pLayer1->m_bLayerFlg.set(LAYER_CUT_TARGET, pLayer2->m_bLayerFlg[LAYER_CUT_TARGET]);
				if ( IsMakeEx1() )
					pLayer1->m_strInitFile	= pLayer2->m_strInitFile;
				else {
					pLayer1->m_dZCut		= pLayer2->m_dZCut;
					pLayer1->m_bLayerFlg.set(LAYER_DRILL_Z, pLayer2->m_bLayerFlg[LAYER_DRILL_Z]);
				}
				pLayer1->m_strLayerComment	= pLayer2->m_strLayerComment;
				pLayer1->m_strLayerCode		= pLayer2->m_strLayerCode;
				pLayer1->m_strNCFile		= pLayer2->m_strNCFile;
				pLayer1->m_bLayerFlg.set(LAYER_PART_OUT, pLayer2->m_bLayerFlg[LAYER_PART_OUT]);
			}
		}
	}

	// �V�K�ۑ�����Ă���ꍇ
	CheckNewLayer();
}

void CMakeNCDlgEx2::OnSelChangeInit() 
{
	int nIndex = MakeNCDlgSelChange(m_ctInitFileName, m_hWnd, IDC_MKNC_INITPATH,
			m_strInitPath, m_strInitFileName);
	// ����̧�ق̑�������
	CString	strFullPath(m_strInitPath+m_strInitFileName);
	if ( !::IsFileExist(strFullPath) ) {
		// �����폜
		AfxGetNCVCApp()->GetDXFOption()->DelInitHistory(NCMAKEMILL, strFullPath);
		m_ctInitFileName.DeleteString(nIndex);
	}
}

void CMakeNCDlgEx2::OnSelChangeLayerToInit() 
{
	int nIndex	= MakeNCDlgSelChange(m_ctLayerToInitFileName,
			m_hWnd, IDC_MKNCEX_LAYERTOINITPATH, m_strLayerToInitPath, m_strLayerToInitFileName);
}

void CMakeNCDlgEx2::OnKillFocusNCFile() 
{
	UpdateData();
	if ( !m_strNCFileName.IsEmpty() )
		MakeDlgKillFocus(m_strNCPath, m_strNCFileName, this, IDC_MKNC_NCPATH);
}

void CMakeNCDlgEx2::OnKillFocusInit() 
{
	UpdateData();
	if ( !m_strInitFileName.IsEmpty() )
		MakeDlgKillFocus(m_strInitPath, m_strInitFileName, this, IDC_MKNC_INITPATH);
}

void CMakeNCDlgEx2::OnKillFocusLayerToInit() 
{
	UpdateData();
	if ( !m_strLayerToInitFileName.IsEmpty() )
		MakeDlgKillFocus(m_strLayerToInitPath, m_strLayerToInitFileName,
			this, IDC_MKNCEX_LAYERTOINITPATH);
	// ڲԏ��Ɍ���g���q����(�����u��)
	if ( !m_strLayerToInitFileName.IsEmpty() ) {
		CString	strName, strExt;
		::Path_Name_From_FullPath(m_strLayerToInitFileName, strExt, strName, FALSE);
		VERIFY(strExt.LoadString(IDS_NCL_FILTER));
		strName += strExt.Right(4);		// .ncl
		if ( m_strLayerToInitFileName.CompareNoCase(strName) != 0 ) {
			m_strLayerToInitFileName = strName;
			UpdateData(FALSE);
		}
	}
}
