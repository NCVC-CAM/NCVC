// MakeNCDlgEx11.cpp : �C���v�������e�[�V���� �t�@�C��
//

#include "stdafx.h"
#include "NCVC.h"
#include "Layer.h"
#include "DXFDoc.h"
#include "MakeNCDlg.h"
#include "MakeNCDlgEx.h"
#include "MakeNCDlgEx11.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
extern	CMagaDbg	g_dbg;
#endif

#define	GetNCMakeParent()	static_cast<CMakeNCDlgEx *>(GetParent())

BEGIN_MESSAGE_MAP(CMakeNCDlgEx11, CDialog)
	//{{AFX_MSG_MAP(CMakeNCDlgEx11)
	ON_CBN_SELCHANGE(IDC_MKNC_INIT, OnSelChangeInit)
	ON_CBN_KILLFOCUS(IDC_MKNC_INIT, OnKillFocusInit)
	ON_CBN_SELCHANGE(IDC_MKNCEX_LAYER, OnSelChangeLayer)
	ON_CBN_SETFOCUS(IDC_MKNCEX_LAYER, OnSetFocusLayer)
	ON_EN_KILLFOCUS(IDC_MKNC_NCFILE, OnKillFocusNCFile)
	ON_BN_CLICKED(IDC_MKNCEX_NEW, OnNewLayer)
	ON_BN_CLICKED(IDC_MKNC_INITUP, OnMKNCInitUp)
	ON_BN_CLICKED(IDC_MKNC_INITED, OnMKNCInitEdit)
	ON_BN_CLICKED(IDC_MKNC_NCFILEUP, OnMKNCFileUp)
	ON_BN_CLICKED(IDC_MKNCEX_COPY, OnCopy)
	ON_BN_CLICKED(IDC_MKNCEX_PARTOUT, OnPartOut)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMakeNCDlgEx11 �_�C�A���O

CMakeNCDlgEx11::CMakeNCDlgEx11(CMakeNCDlgEx* pParent, int nIndex)
	: CDialog(CMakeNCDlgEx11::IDD, pParent)
{
	// �ݽ�׸��ł� GetNCMakeParent() �͎g���܂���

	//{{AFX_DATA_INIT(CMakeNCDlgEx11)
	//}}AFX_DATA_INIT

	// ڲԏ��Ɛ؍����̧�ق̾��(����޲�۸ނ̕\����)
	int		i, nLoop;
	CLayerData*	pLayer;
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
					pLayer = new CLayerData(pLayer, pLayer->m_bCutTarget);
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

	const CStringList*	pList = AfxGetNCVCApp()->GetDXFOption()->GetInitList();
	// �����\���ް��� nIndex ����擾
	m_nIndex = nIndex<0 || nIndex>m_obLayer.GetUpperBound() ? 0 : nIndex;
	pLayer = m_obLayer[m_nIndex];
	if ( !pLayer->m_strInitFile.IsEmpty() )
		::Path_Name_From_FullPath(pLayer->m_strInitFile, m_strInitPath, m_strInitFileName);
	else if ( pList->GetCount() > 0 )
		::Path_Name_From_FullPath(pList->GetHead(), m_strInitPath, m_strInitFileName);
	::Path_Name_From_FullPath(pLayer->m_strNCFile, m_strNCPath, m_strNCFileName);
	m_bPartOut			= m_strNCFileName.IsEmpty() ? FALSE : pLayer->m_bPartOut;
	m_bCheck			= pLayer->m_bCutTarget;
	m_strLayerComment	= pLayer->m_strLayerComment;
	m_strLayerCode		= pLayer->m_strLayerCode;
}

CMakeNCDlgEx11::~CMakeNCDlgEx11()
{
	for ( int i=0; i<m_obLayer.GetSize(); i++ )
		delete	m_obLayer[i];
}

void CMakeNCDlgEx11::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMakeNCDlgEx11)
	DDX_Control(pDX, IDOK, m_ctOK);
	DDX_Control(pDX, IDC_MKNCEX_STATIC3, m_ctPartEnable3);
	DDX_Control(pDX, IDC_MKNCEX_STATIC2, m_ctPartEnable2);
	DDX_Control(pDX, IDC_MKNCEX_STATIC1, m_ctPartEnable1);
	DDX_Control(pDX, IDC_MKNCEX_LAYER, m_ctLayer);
	DDX_Control(pDX, IDC_MKNC_NCFILEUP, m_ctPartEnable4);
	DDX_Control(pDX, IDC_MKNC_NCFILE, m_ctNCFileName);
	DDX_Control(pDX, IDC_MKNC_NCPATH, m_ctNCPath);
	DDX_Control(pDX, IDC_MKNC_INITPATH, m_ctInitPath);
	DDX_Control(pDX, IDC_MKNC_INIT, m_ctInitFileName);
	DDX_CBString(pDX, IDC_MKNC_INIT, m_strInitFileName);
	DDX_Text(pDX, IDC_MKNC_NCFILE, m_strNCFileName);
	DDX_Text(pDX, IDC_MKNCEX_LAYERCOMMENT, m_strLayerComment);
	DDX_Text(pDX, IDC_MKNCEX_LAYERCODE, m_strLayerCode);
	DDX_Check(pDX, IDC_MKNCEX_CHECK, m_bCheck);
	DDX_Check(pDX, IDC_MKNCEX_PARTOUT, m_bPartOut);
	//}}AFX_DATA_MAP
}

void CMakeNCDlgEx11::GetNowState(void)
{
	CLayerData* pLayer = m_obLayer[m_nIndex];
	UpdateData();
	pLayer->m_bCutTarget		= m_bCheck;
	pLayer->m_bPartOut			= m_bPartOut;
	pLayer->m_strNCFile			= m_strNCPath + m_strNCFileName;
	pLayer->m_strInitFile		= m_strInitPath + m_strInitFileName;
	pLayer->m_strLayerComment	= m_strLayerComment;
	pLayer->m_strLayerCode		= m_strLayerCode;
}

void CMakeNCDlgEx11::SetNowState(int nIndex)
{
	m_nIndex = nIndex;
	CLayerData* pLayer = m_obLayer[m_nIndex];
	m_bCheck			= pLayer->m_bCutTarget;
	m_bPartOut			= pLayer->m_bPartOut;
	m_strLayerComment	= pLayer->m_strLayerComment;
	m_strLayerCode		= pLayer->m_strLayerCode;
	const CStringList*	pList = AfxGetNCVCApp()->GetDXFOption()->GetInitList();
	if ( !pLayer->m_strInitFile.IsEmpty() )
		::Path_Name_From_FullPath(pLayer->m_strInitFile, m_strInitPath, m_strInitFileName);
	else if ( pList->GetCount() > 0 )
		::Path_Name_From_FullPath(pList->GetHead(), m_strInitPath, m_strInitFileName);
	::PathSetDlgItemPath(m_hWnd, IDC_MKNC_INITPATH, m_strInitPath);
	::Path_Name_From_FullPath(pLayer->m_strNCFile, m_strNCPath, m_strNCFileName);
	::PathSetDlgItemPath(m_hWnd, IDC_MKNC_NCPATH, m_strNCPath);
	UpdateData(FALSE);
	EnablePartOut();
}

void CMakeNCDlgEx11::EnablePartOut(void)
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
// CMakeNCDlgEx11 ���b�Z�[�W �n���h��

BOOL CMakeNCDlgEx11::OnInitDialog() 
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

	int		i;
	// ڲ�ؽĂ̏�����
	for ( i=0; i<m_obLayer.GetSize(); i++ )
		m_ctLayer.AddString( m_obLayer[i]->m_strLayer );
	// �؍���������ޯ���ɱ��ђǉ�
	InitialMakeNCDlgComboBox(AfxGetNCVCApp()->GetDXFOption()->GetInitList(), m_ctInitFileName);	// MakeNCDlg.cpp
	// �߽�\���̍œK��
	::PathSetDlgItemPath(m_hWnd, IDC_MKNC_INITPATH, m_strInitPath);
	::PathSetDlgItemPath(m_hWnd, IDC_MKNC_NCPATH, m_strNCPath);
	// �ʏo��̧�ق̗L����
	EnablePartOut();
	// ����ٸد����ꂽ���т�I��
	m_ctLayer.SetCurSel(m_nIndex);
	m_ctLayer.SetFocus();

	return FALSE;
}

void CMakeNCDlgEx11::OnOK() 
{
	// Enter���������ꂽ�Ƃ����̺��۰ق�KillFocus�����ɂ����ɗ���
	m_ctOK.SetFocus();

	CString	strNCFile;
	const	CStringList*	pList = AfxGetNCVCApp()->GetDXFOption()->GetInitList();
	// �Ō�̏����擾
	GetNowState();
	
	// �ȩ�ق�����
	CLayerData* pLayer;
	for ( int i=0; i<m_obLayer.GetSize(); i++ ) {
		pLayer = m_obLayer[i];
		if ( !pLayer->m_bCutTarget )
			continue;
		// ����̧�ق��󗓂Ȃ狭���I�ɐݒ�
		if ( pLayer->m_strInitFile.IsEmpty() ) {
			if ( pList->GetCount() > 0 )
				pLayer->m_strInitFile = pList->GetHead();
			else {
				AfxMessageBox(IDS_ERR_FILEPATH, MB_OK|MB_ICONEXCLAMATION);
				SetNowState(i);
				return;
			}
		}
		// ���������Ȃ�
		if ( !::IsFileExist(pLayer->m_strInitFile) ) {	// NCVC.cpp
			m_ctLayer.SetCurSel(i);
			SetNowState(i);
			m_ctInitFileName.SetEditSel(0, -1);
			m_ctInitFileName.SetFocus();
			return;
		}
		// ��̧�ٖ�������
		if ( pLayer->m_bPartOut ) {
			strNCFile = pLayer->m_strNCFile;
			if ( !CheckMakeDlgFileExt(TYPE_NCD, strNCFile) ) {
				SetNowState(i);
				m_ctNCFileName.SetFocus();
				m_ctNCFileName.SetSel(0, -1);
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

void CMakeNCDlgEx11::OnNewLayer()
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

void CMakeNCDlgEx11::OnSetFocusLayer() 
{
	m_nIndex = m_ctLayer.GetCurSel();
}

void CMakeNCDlgEx11::OnSelChangeLayer() 
{
	int	nIndex = m_ctLayer.GetCurSel();
	if ( nIndex<0 || nIndex>m_obLayer.GetUpperBound() ) {
		m_ctInitPath.SetWindowText("");
		m_strInitFileName.Empty();
		m_ctNCPath.SetWindowText("");
		m_strNCFileName.Empty();
		m_bCheck = m_bPartOut = FALSE;
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

void CMakeNCDlgEx11::OnMKNCInitUp() 
{
	UpdateData();
	CString	strFilter;
	VERIFY(strFilter.LoadString(IDS_NCI_FILTER));
	MakeDlgFileRefer(IDS_OPTION_INIT, strFilter, this, IDC_MKNC_INITPATH,
			m_strInitPath, m_strInitFileName, TRUE);
	m_ctLayer.SetFocus();
	// �����I�����
	m_ctInitFileName.SetEditSel(0, -1);
	m_ctInitFileName.SetFocus();
}

void CMakeNCDlgEx11::OnMKNCInitEdit() 
{
	UpdateData();
	MakeNCDlgInitFileEdit(m_strInitPath, m_strInitFileName,
		this, IDC_MKNC_INITPATH, m_ctInitFileName);
	m_ctLayer.SetFocus();
}

void CMakeNCDlgEx11::OnSelChangeInit() 
{
	int nIndex = MakeNCDlgSelChange(m_ctInitFileName, m_hWnd, IDC_MKNC_INITPATH,
			m_strInitPath, m_strInitFileName);
	// ����̧�ق̑�������
	CString	strFullPath(m_strInitPath+m_strInitFileName);
	if ( !::IsFileExist(strFullPath) ) {
		AfxGetNCVCApp()->GetDXFOption()->DelInitHistory(strFullPath);	// �����폜
		m_ctInitFileName.DeleteString(nIndex);
	}
}

void CMakeNCDlgEx11::OnMKNCFileUp() 
{
	UpdateData();
	MakeDlgFileRefer(IDS_DXF2NCD_FILE, AfxGetNCVCApp()->GetFilterString(TYPE_NCD),
			this, IDC_MKNC_NCPATH, m_strNCPath, m_strNCFileName, FALSE);
	m_ctLayer.SetFocus();
	// �����I�����
	m_ctNCFileName.SetFocus();
	m_ctNCFileName.SetSel(0, -1);
}

void CMakeNCDlgEx11::OnKillFocusInit() 
{
	UpdateData();
	MakeDlgKillFocus(m_strInitPath, m_strInitFileName, this, IDC_MKNC_INITPATH);
}

void CMakeNCDlgEx11::OnKillFocusNCFile() 
{
	UpdateData();
	MakeDlgKillFocus(m_strNCPath, m_strNCFileName, this, IDC_MKNC_NCPATH);
}

void CMakeNCDlgEx11::OnCopy() 
{
	// ���݂̏����擾
	GetNowState();
	// ���݈ʒu�ȍ~���ް����߰
	CLayerData* pLayer = m_obLayer[m_nIndex];
	BOOL	bCheck		= pLayer->m_bCutTarget,
			bPartOut	= pLayer->m_bPartOut;
	CString	strInit		= pLayer->m_strInitFile,
			strComment	= pLayer->m_strLayerComment,
			strCode		= pLayer->m_strLayerCode;
	CString	strPath, strFile, strNCFile;
	::Path_Name_From_FullPath(GetNCMakeParent()->m_dlg1.GetNCFileName(),
			strPath, strFile, FALSE);
	strNCFile = strPath + strFile + "_";
	for ( int i=m_nIndex+1; i<m_obLayer.GetSize(); i++ ) {
		pLayer = m_obLayer[i];
		pLayer->m_bCutTarget		= bCheck;
		pLayer->m_bPartOut			= bPartOut;
		pLayer->m_strInitFile		= strInit;
		pLayer->m_strLayerComment	= strComment;
		pLayer->m_strLayerCode		= strCode;
		if ( bPartOut && pLayer->m_strNCFile.IsEmpty() )
			pLayer->m_strNCFile = strNCFile + pLayer->m_strLayer +
					AfxGetNCVCApp()->GetDocExtString(TYPE_NCD);
	}
	m_ctLayer.SetFocus();
}

void CMakeNCDlgEx11::OnPartOut() 
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
