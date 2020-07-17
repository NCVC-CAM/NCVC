// MakeNCDlgEx21.cpp : �C���v�������e�[�V���� �t�@�C��
//

#include "stdafx.h"
#include "NCVC.h"
#include "Layer.h"
#include "DXFDoc.h"
#include "MakeNCDlg.h"
#include "MakeNCDlgEx.h"
#include "MakeNCDlgEx21.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

#define	GetNCMakeParent()	static_cast<CMakeNCDlgEx *>(GetParent())

BEGIN_MESSAGE_MAP(CMakeNCDlgEx21, CDialog)
	//{{AFX_MSG_MAP(CMakeNCDlgEx21)
	ON_CBN_SELCHANGE(IDC_MKNCEX_LAYER, OnSelChangeLayer)
	ON_CBN_SETFOCUS(IDC_MKNCEX_LAYER, OnSetFocusLayer)
	ON_EN_KILLFOCUS(IDC_MKNC_NCFILE, OnKillFocusNCFile)
	ON_BN_CLICKED(IDC_MKNCEX_NEW, OnNewLayer)
	ON_BN_CLICKED(IDC_MKNC_NCFILEUP, OnMKNCFileUp)
	ON_BN_CLICKED(IDC_MKNCEX_COPY, OnCopy)
	ON_BN_CLICKED(IDC_MKNCEX_PARTOUT, OnPartOut)
	ON_BN_CLICKED(IDC_MKNCEX21_STEP, OnStep)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMakeNCDlgEx21 �_�C�A���O

CMakeNCDlgEx21::CMakeNCDlgEx21(CMakeNCDlgEx* pParent, int nIndex)
	: CDialog(CMakeNCDlgEx21::IDD, pParent)
{
	// �ݽ�׸��ł� GetNCMakeParent() �͎g���܂���

	//{{AFX_DATA_INIT(CMakeNCDlgEx21)
	//}}AFX_DATA_INIT

	// ڲԏ��̾��(����޲�۸ނ̕\����)
	int		i, nLoop;
	CLayerData*		pLayer;
	try {
		if ( ::IsWindow(pParent->m_dlg2.m_hWnd) ) {
			// ڲ���ނ��L���ȏꍇ�́A���̖��ׂ���
			const CListCtrl& ctLayer = pParent->m_dlg2.m_ctLayerList;
			nLoop = ctLayer.GetItemCount();
			for ( i=0; i<nLoop; i++ ) {
				pLayer = new CLayerData(reinterpret_cast<const CLayerData *>(ctLayer.GetItemData(i)), ctLayer.GetCheck(i));
				m_obLayer.Add(pLayer);
			}
		}
		else {
			// ���݂��޷���Ă���擾
			CDXFDoc*	pDoc = pParent->GetDocument();
			nLoop = pDoc->GetLayerCnt();
			for ( i=0; i<nLoop; i++ ) {
				pLayer = pDoc->GetLayerData(i);
				if ( pLayer->IsCutType() ) {
					pLayer = new CLayerData(pLayer, pLayer->m_bLayerFlg[LAYER_CUTTARGET]);
					m_obLayer.Add(pLayer);
				}
			}
		}
	}
	catch (CMemoryException* e) {
		if ( pLayer )
			delete	pLayer;
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		m_nIndex = -1;
		return;
	}

	// �����\���ް��� nIndex ����擾
	m_nIndex = nIndex<0 || nIndex>m_obLayer.GetUpperBound() ? 0 : nIndex;
	pLayer = m_obLayer[m_nIndex];
	::Path_Name_From_FullPath(pLayer->m_strNCFile, m_strNCPath, m_strNCFileName);
	m_bDrill			= pLayer->m_bLayerFlg[LAYER_DRILLZ];
	m_bPartOut			= m_strNCFileName.IsEmpty() ? FALSE : pLayer->m_bLayerFlg[LAYER_PARTOUT];
	m_bCheck			= pLayer->m_bLayerFlg[LAYER_CUTTARGET];
	m_strLayerComment	= pLayer->m_strLayerComment;
	m_strLayerCode		= pLayer->m_strLayerCode;
	// ���Ѻ��۰ق̏������� CDialog::OnInitDialog() �ȍ~�łȂ��Ʊ��Ĵװ
//	m_dZCut = pLayer->m_dZCut;
}

CMakeNCDlgEx21::~CMakeNCDlgEx21()
{
	for ( int i=0; i<m_obLayer.GetSize(); i++ )
		delete	m_obLayer[i];
}

void CMakeNCDlgEx21::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMakeNCDlgEx21)
	DDX_Control(pDX, IDC_MKNCEX_STATIC3, m_ctPartEnable3);
	DDX_Control(pDX, IDC_MKNCEX_STATIC2, m_ctPartEnable2);
	DDX_Control(pDX, IDC_MKNCEX_STATIC1, m_ctPartEnable1);
	DDX_Control(pDX, IDC_MKNCEX_LAYER, m_ctLayer);
	DDX_Control(pDX, IDC_MKNC_NCFILE, m_ctNCFileName);
	DDX_Control(pDX, IDC_MKNC_NCPATH, m_ctNCPath);
	DDX_Control(pDX, IDC_MKNC_NCFILEUP, m_ctPartEnable4);
	DDX_Control(pDX, IDOK, m_ctOK);
	DDX_Control(pDX, IDC_MKNC3_ZSTEP, m_dZStep);
	DDX_Control(pDX, IDC_MKNC1_ZCUT, m_dZCut);
	DDX_Text(pDX, IDC_MKNC_NCFILE, m_strNCFileName);
	DDX_Text(pDX, IDC_MKNCEX_LAYERCOMMENT, m_strLayerComment);
	DDX_Text(pDX, IDC_MKNCEX_LAYERCODE, m_strLayerCode);
	DDX_Check(pDX, IDC_MKNCEX21_DRILL, m_bDrill);
	DDX_Check(pDX, IDC_MKNCEX_CHECK, m_bCheck);
	DDX_Check(pDX, IDC_MKNCEX_PARTOUT, m_bPartOut);
	//}}AFX_DATA_MAP
}

void CMakeNCDlgEx21::GetNowState(void)
{
	CLayerData* pLayer = m_obLayer[m_nIndex];
	UpdateData();
	pLayer->m_bLayerFlg.set(LAYER_CUTTARGET, m_bCheck);
	pLayer->m_bLayerFlg.set(LAYER_DRILLZ,    m_bDrill);
	pLayer->m_bLayerFlg.set(LAYER_PARTOUT,   m_bPartOut);
	pLayer->m_dZCut				= m_dZCut;
	pLayer->m_strNCFile			= m_strNCPath + m_strNCFileName;
	pLayer->m_strLayerComment	= m_strLayerComment;
	pLayer->m_strLayerCode		= m_strLayerCode;
}

void CMakeNCDlgEx21::SetNowState(int nIndex)
{
	m_nIndex = nIndex;
	CLayerData* pLayer = m_obLayer[m_nIndex];
	m_bCheck			= pLayer->m_bLayerFlg[LAYER_CUTTARGET];
	m_bDrill			= pLayer->m_bLayerFlg[LAYER_DRILLZ];
	m_bPartOut			= pLayer->m_bLayerFlg[LAYER_PARTOUT];
	m_dZCut				= pLayer->m_dZCut;
	m_strLayerComment	= pLayer->m_strLayerComment;
	m_strLayerCode		= pLayer->m_strLayerCode;
	::Path_Name_From_FullPath(pLayer->m_strNCFile, m_strNCPath, m_strNCFileName);
	::PathSetDlgItemPath(m_hWnd, IDC_MKNC_NCPATH, m_strNCPath);
	UpdateData(FALSE);
	EnablePartOut();
}

void CMakeNCDlgEx21::EnablePartOut(void)
{
	if ( m_bPartOut ) {
		m_ctPartEnable1.EnableWindow(TRUE);
		m_ctPartEnable2.EnableWindow(TRUE);
		m_ctPartEnable3.EnableWindow(TRUE);
		m_ctPartEnable4.EnableWindow(TRUE);
		m_ctNCPath.EnableWindow(TRUE);
		m_ctNCFileName.EnableWindow(TRUE);
		if ( m_strNCFileName.IsEmpty() ) {
			// �e�޲�۸ނ���NÇ�ٖ��擾
			::Path_Name_From_FullPath(GetNCMakeParent()->m_dlg1.GetNCFileName(),
					m_strNCPath, m_strNCFileName, FALSE);
			m_strNCFileName += "_" + m_obLayer[m_nIndex]->m_strLayer +
				AfxGetNCVCApp()->GetDocExtString(TYPE_NCD);
			::PathSetDlgItemPath(m_hWnd, IDC_MKNC_NCPATH, m_strNCPath);
			UpdateData(FALSE);
		}
	}
	else {
		m_ctPartEnable1.EnableWindow(FALSE);
		m_ctPartEnable2.EnableWindow(FALSE);
		m_ctPartEnable3.EnableWindow(FALSE);
		m_ctPartEnable4.EnableWindow(FALSE);
		m_ctNCPath.EnableWindow(FALSE);
		m_ctNCFileName.EnableWindow(FALSE);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CMakeNCDlgEx21 ���b�Z�[�W �n���h��

BOOL CMakeNCDlgEx21::OnInitDialog() 
{
	CDialog::OnInitDialog();

	if ( m_nIndex < 0 ) {
		EndDialog(IDCANCEL);
		return TRUE;
	}

	// ���ِݒ�
	GetWindowText(m_strCaption);	// ���̳���޳���ق��擾
	if ( !GetNCMakeParent()->m_strLayerToInitFileName.IsEmpty() )
		SetWindowText(::AddDialogTitle2File(m_strCaption, GetNCMakeParent()->m_strLayerToInitFileName));

	m_dZCut = m_obLayer[m_nIndex]->m_dZCut;
	// ڲ�ؽĂ̏�����
	for ( int i=0; i<m_obLayer.GetSize(); i++ )
		m_ctLayer.AddString( m_obLayer[i]->m_strLayer );
	// �߽�\���̍œK��
	::PathSetDlgItemPath(m_hWnd, IDC_MKNC_NCPATH, m_strNCPath);
	// �ʏo��̧�ق̗L����
	EnablePartOut();
	// ����ٸد����ꂽ���т�I��
	m_ctLayer.SetCurSel(m_nIndex);
	m_ctLayer.SetFocus();

	return FALSE;
}

void CMakeNCDlgEx21::OnOK() 
{
	CString	strNCFile;

	// Enter���������ꂽ�Ƃ����̺��۰ق�KillFocus�����ɂ����ɗ���
	m_ctOK.SetFocus();

	// �Ō�̏����擾
	GetNowState();

	// �ȩ�ق�����
	CLayerData* pLayer;
	for ( int i=0; i<m_obLayer.GetSize(); i++ ) {
		pLayer = m_obLayer[i];
		if ( !pLayer->m_bLayerFlg[LAYER_CUTTARGET] )
			continue;
		// ��̧�ٖ�������
		if ( pLayer->m_bLayerFlg[LAYER_PARTOUT] ) {
			strNCFile = pLayer->m_strNCFile;
			if ( !CheckMakeDlgFileExt(TYPE_NCD, strNCFile) ) {
				m_ctNCFileName.SetFocus();
				m_ctNCFileName.SetSel(0, -1);
				SetNowState(i);
				return;
			}
			if ( strNCFile.CompareNoCase(pLayer->m_strNCFile) != 0 )
				pLayer->m_strNCFile = strNCFile;
		}
	}

	// ��̧�ق̏d������(���̧�قƂ̏d�������͂Ȃ�)
	CString	strResult( GetNCMakeParent()->GetDocument()->CheckDuplexFile(CString(), &m_obLayer) );
	if ( !strResult.IsEmpty() ) {
		int	nResult = m_ctLayer.FindString(-1, strResult);
		m_ctLayer.SetCurSel(nResult);
		SetNowState(nResult);
		return;
	}

	CDialog::OnOK();
}

void CMakeNCDlgEx21::OnNewLayer()
{
	GetNowState();

	CDXFDoc*	pDoc = GetNCMakeParent()->GetDocument();
	CString		strPath, strFile;
	if ( GetNCMakeParent()->m_strLayerToInitFileName.IsEmpty() )
		CreateLayerFile(pDoc, strPath, strFile);
	else
		::Path_Name_From_FullPath(GetNCMakeParent()->m_strLayerToInitFileName, strPath, strFile);

	if ( ::NCVC_FileDlgCommon(IDS_OPTION_LAYER2INITSAVE, IDS_NCL_FILTER,
				strFile, strPath, FALSE, OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT) == IDOK ) {
		pDoc->SaveLayerMap(strFile, &m_obLayer);
		if ( GetNCMakeParent()->m_strLayerToInitFileName.CompareNoCase(strFile) != 0 ) {
			GetNCMakeParent()->m_strLayerToInitFileName = strFile;
			SetWindowText(::AddDialogTitle2File(m_strCaption, strFile));
		}
	}
}

void CMakeNCDlgEx21::OnSetFocusLayer() 
{
	m_nIndex = m_ctLayer.GetCurSel();
}

void CMakeNCDlgEx21::OnSelChangeLayer() 
{
	int	nIndex = m_ctLayer.GetCurSel();
	if ( nIndex<0 || nIndex>m_obLayer.GetUpperBound() ) {
		m_ctNCPath.SetWindowText("");
		m_strNCFileName.Empty();
		m_bCheck = m_bDrill = m_bPartOut = FALSE;
		m_dZCut = 0.0;
		UpdateData(FALSE);
		EnablePartOut();
	}
	else {
		// ���݂̏����擾
		GetNowState();
		// �ؑ֌�̏����
		SetNowState(nIndex);
	}
}

void CMakeNCDlgEx21::OnMKNCFileUp() 
{
	UpdateData();
	MakeDlgFileRefer(IDS_DXF2NCD_FILE, AfxGetNCVCApp()->GetFilterString(TYPE_NCD),
			this, IDC_MKNC_NCPATH, m_strNCPath, m_strNCFileName, FALSE);
	m_ctLayer.SetFocus();
	// �����I�����
	m_ctNCFileName.SetFocus();
	m_ctNCFileName.SetSel(0, -1);
}

void CMakeNCDlgEx21::OnKillFocusNCFile() 
{
	UpdateData();
	MakeDlgKillFocus(m_strNCPath, m_strNCFileName, this, IDC_MKNC_NCPATH);
}

void CMakeNCDlgEx21::OnCopy() 
{
	// ���݂̏����擾
	GetNowState();
	// ���݈ʒu�ȍ~���ް����߰
	CLayerData* pLayer = m_obLayer[m_nIndex];
	BOOL	bCheck		= pLayer->m_bLayerFlg[LAYER_CUTTARGET],
			bDrill		= pLayer->m_bLayerFlg[LAYER_DRILLZ],
			bPartOut	= pLayer->m_bLayerFlg[LAYER_PARTOUT];
	double	dZCut		= pLayer->m_dZCut;
	CString	strComment	= pLayer->m_strLayerComment,
			strCode		= pLayer->m_strLayerCode;
	CString	strPath, strFile, strNCFile;
	::Path_Name_From_FullPath(GetNCMakeParent()->m_dlg1.GetNCFileName(),
			strPath, strFile, FALSE);
	strNCFile = strPath + strFile + "_";
	for ( int i=m_nIndex+1; i<m_obLayer.GetSize(); i++ ) {
		pLayer = m_obLayer[i];
		pLayer->m_bLayerFlg.set(LAYER_CUTTARGET, bCheck);
		pLayer->m_bLayerFlg.set(LAYER_DRILLZ,    bDrill);
		pLayer->m_bLayerFlg.set(LAYER_PARTOUT,   bPartOut);
		pLayer->m_dZCut			= dZCut;
		pLayer->m_strLayerComment	= strComment;
		pLayer->m_strLayerCode		= strCode;
		if ( bPartOut && pLayer->m_strNCFile.IsEmpty() )
			pLayer->m_strNCFile = strNCFile + pLayer->m_strLayer + AfxGetNCVCApp()->GetDocExtString(TYPE_NCD);
	}
	m_ctLayer.SetFocus();
}

void CMakeNCDlgEx21::OnStep() 
{
	// ���݂̏����擾
	GetNowState();
	double	dZStep = m_dZStep;	// �ϊ��ݽ�׸��Ȃ̂�ٰ�ߊO��
	// ���݈ʒu�ȍ~�ɽï���ް������Z
	double	dZCut = m_obLayer[m_nIndex]->m_dZCut;
	for ( int i=m_nIndex+1, nCnt=1; i<m_obLayer.GetSize(); i++, nCnt++ )
		m_obLayer[i]->m_dZCut = dZCut+dZStep*nCnt;
	m_ctOK.SetFocus();
}

void CMakeNCDlgEx21::OnPartOut() 
{
	int	nIndex = m_ctLayer.GetCurSel();
	if ( nIndex<0 || nIndex>m_obLayer.GetUpperBound() ) {
		m_ctPartEnable1.EnableWindow(FALSE);
		m_ctPartEnable2.EnableWindow(FALSE);
		m_ctPartEnable3.EnableWindow(FALSE);
		m_ctPartEnable4.EnableWindow(FALSE);
		m_ctNCPath.EnableWindow(FALSE);
		m_ctNCFileName.EnableWindow(FALSE);
	}
	else {
		UpdateData();
		EnablePartOut();
	}
}
