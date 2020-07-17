// MakeNCDlgEx1.cpp : �C���v�������e�[�V���� �t�@�C��
//

#include "stdafx.h"
#include "NCVC.h"
#include "NCMakeOption.h"
#include "Layer.h"
#include "DXFDoc.h"
#include "MakeNCDlg.h"
#include "MakeNCDlgEx1.h"
#include "MakeNCDlgEx11.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

// MakeNCDlgEx1, MakeNCDlgEx2 ���pؽĺ��۰�ͯ�ް������
extern	LPCTSTR	g_szListHeader[] = {
	"ڲԖ�", "�؍����̧��", "�Ő[Z���W", "�����Ő[Z���W",
	"�ʏo��̧�ٖ�"
};

BEGIN_MESSAGE_MAP(CMakeNCDlgEx1, CDialog)
	//{{AFX_MSG_MAP(CMakeNCDlgEx1)
	ON_NOTIFY(LVN_GETDISPINFO, IDC_MKNCEX_LAYERLIST, OnGetDispInfoLayerList)
	ON_NOTIFY(NM_DBLCLK, IDC_MKNCEX_LAYERLIST, OnDblClkLayerList)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_MKNCEX_LAYERLIST, OnColumnClickLayerList)
	ON_NOTIFY(LVN_ENDSCROLL, IDC_MKNCEX_LAYERLIST, OnEndScrollLayerList)
	ON_EN_KILLFOCUS(IDC_MKNC_NCFILE, OnKillFocusNCFile)
	ON_CBN_KILLFOCUS(IDC_MKNCEX_LAYERTOINITFILE, OnKillFocusLayerToInit)
	ON_CBN_SELCHANGE(IDC_MKNCEX_LAYERTOINITFILE, OnSelChangeLayerToInit)
	ON_BN_CLICKED(IDC_MKNC_NCFILEUP, OnMKNCFileUp)
	ON_BN_CLICKED(IDC_MKNCEX_LAYERTOINIT_ED, OnMKNCLayerEdit)
	ON_BN_CLICKED(IDC_MKNCEX_LAYERTOINIT_UP, OnMKNCLayerUp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMakeNCDlgEx1 �_�C�A���O

CMakeNCDlgEx1::CMakeNCDlgEx1(CDXFDoc* pDoc)
	: CDialog(CMakeNCDlgEx1::IDD, NULL)
{
	m_pDoc = pDoc;
	//{{AFX_DATA_INIT(CMakeNCDlgEx1)
	//}}AFX_DATA_INIT

	// �޷���Ė�����NÇ�ٖ����쐬
	CreateNCFile(pDoc, m_strNCPath, m_strNCFileName);	// MakeNCDlg.cpp

	CDXFOption*	pOpt = AfxGetNCVCApp()->GetDXFOption();
	m_bNCView = pOpt->GetDxfFlag(DXFOPT_VIEW);
	// ��ė�̎擾
	m_nSortLayer = GetMakeNCDlgExSortColumn(IDS_REG_DXF_SORTLAYER1);

	// ڲԏ��̾��
	CString		strFullPath, strPath, strFile;
	CreateLayerFile(pDoc, strPath, strFile);	// �޷����̧�قƓ���ncļ�ٖ�
	strFullPath = strPath + strFile;
	if ( ::IsFileExist(strFullPath, TRUE, FALSE) ) {
		// �޷����̧�قƓ���ncļ�ق������
		pDoc->ReadLayerMap(strFullPath);
		pOpt->AddLayerHistory(strFullPath);
		m_strLayerToInitPath		= strPath;
		m_strLayerToInitFileName	= strFile;
	}
/*
	else if ( pOpt->GetLayerToInitList()->GetCount() > 0 ) {
		// �����ɗL��Η���̧�ق�ǂݍ���
		strFullPath = pOpt->GetLayerToInitList()->GetHead();
		if ( ::IsFileExist(strFullPath, TRUE, FALSE) ) {
			pDoc->ReadLayerMap(strFullPath);
			::Path_Name_From_FullPath(strFullPath, m_strLayerToInitPath, m_strLayerToInitFileName);
		}
		else
			pOpt->DelLayerHistory(strFullPath);	// �����폜
	}
*/
}

CMakeNCDlgEx1::~CMakeNCDlgEx1()
{
}

void CMakeNCDlgEx1::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMakeNCDlgEx1)
	DDX_Control(pDX, IDOK, m_ctOK);
	DDX_Control(pDX, IDC_MKNC_NCFILE, m_ctNCFileName);
	DDX_Text(pDX, IDC_MKNC_NCFILE, m_strNCFileName);
	DDX_Control(pDX, IDC_MKNCEX_LAYERTOINITFILE, m_ctLayerToInitFileName);
	DDX_CBString(pDX, IDC_MKNCEX_LAYERTOINITFILE, m_strLayerToInitFileName);
	DDX_Control(pDX, IDC_MKNCEX_LAYERLIST, m_ctLayerList);
	DDX_Check(pDX, IDC_MKNC_VIEW, m_bNCView);
	//}}AFX_DATA_MAP
}

/////////////////////////////////////////////////////////////////////////////
// CMakeNCDlgEx1 ���b�Z�[�W �n���h��

BOOL CMakeNCDlgEx1::OnInitDialog() 
{
	CDialog::OnInitDialog();

	// �ݽ�׸��ł͂ł��Ȃ����۰ق̏�����

	// �֌W̧�ٺ����ޯ���ɱ��ђǉ�
	InitialMakeNCDlgComboBox(AfxGetNCVCApp()->GetDXFOption()->GetLayerToInitList(),
			m_ctLayerToInitFileName);	// MakeNCDlg.cpp
	m_ctLayerToInitFileName.SetCurSel(-1);	// �����l�͑I���Ȃ�
	// �߽�\���̍œK��
	::PathSetDlgItemPath(m_hWnd, IDC_MKNC_NCPATH, m_strNCPath);
//	::PathSetDlgItemPath(m_hWnd, IDC_MKNCEX_LAYERTOINITPATH, m_strLayerToInitPath);
	// ؽĺ��۰ق̗�}��
	CRect	rc;
	m_ctLayerList.GetClientRect(&rc);
	int nWidth  = m_ctLayerList.GetStringWidth(CString('W',10));
	int	nWidth2 = rc.Width()-nWidth*2-16;
	m_ctLayerList.InsertColumn(0, g_szListHeader[0], LVCFMT_LEFT, nWidth-2);
	m_ctLayerList.InsertColumn(1, g_szListHeader[1], LVCFMT_LEFT, nWidth2);
	m_ctLayerList.InsertColumn(2, g_szListHeader[2], LVCFMT_RIGHT, nWidth+2);
	m_ctLayerList.InsertColumn(3, g_szListHeader[4], LVCFMT_LEFT, nWidth2);
	// ؽĺ��۰ق̏�����
	InitialMakeNCDlgExLayerListCtrl(m_pDoc, m_ctLayerList);
	// ڲԖ��ŕ��בւ�
	m_ctLayerList.SortItems(CompareFunc, m_nSortLayer);

	return TRUE;  // �R���g���[���Ƀt�H�[�J�X��ݒ肵�Ȃ��Ƃ��A�߂�l�� TRUE �ƂȂ�܂�
	              // ��O: OCX �v���p�e�B �y�[�W�̖߂�l�� FALSE �ƂȂ�܂�
}

void CMakeNCDlgEx1::OnOK() 
{
	m_ctOK.SetFocus();
	UpdateData();
	CString	strNCPath(m_strNCPath+m_strNCFileName);

	// �ŏI��������(�ڍאݒ�Ő؍�ΏۊO�Ƃ��C�����Ő؍�Ώۂ�������������\��)
	// �؍폇�������׸ނ̏�Ԃ��擾�C���ׂ̍ŏI����
	if ( !CheckMakeNCDlgExLayerState(strNCPath, m_ctNCFileName, m_ctLayerList, TRUE) )
		return;
	// �d������
	CString	strResult( m_pDoc->CheckDuplexFile(strNCPath) );
	if ( !strResult.IsEmpty() ) {
		LVFINDINFO	infoFind;
		infoFind.flags = LVFI_STRING;
		infoFind.psz = strResult;
		SetFocusListCtrl(m_ctLayerList, m_ctLayerList.FindItem(&infoFind));
		return;
	}
	m_strNCFileName = strNCPath;
	// ��߼�ݏ�ԕۑ�
	CDXFOption*	pOpt = AfxGetNCVCApp()->GetDXFOption();
	pOpt->SetViewFlag(m_bNCView);
	// �؍��������
	CLayerData*	pLayer;
	for ( int i=0; i<m_pDoc->GetLayerCnt(); i++ ) {
		pLayer = m_pDoc->GetLayerData(i);
		if ( pLayer->IsCutTarget() )
			pOpt->AddInitHistory(pLayer->GetInitFile());
	}
	// ڲԖ��Ɛ؍����̧�ق̊֌W
	if ( !m_strLayerToInitFileName.IsEmpty() ) {
		CString	strPath, strName, strExt;
		// �ۑ��O���߽����
		if ( m_strLayerToInitPath.IsEmpty() ) {
			::Path_Name_From_FullPath(m_pDoc->GetLayerData(0)->GetInitFile(), strPath, strName);
			m_strLayerToInitFileName = strPath + m_strLayerToInitFileName;
		}
		else
			m_strLayerToInitFileName = m_strLayerToInitPath + m_strLayerToInitFileName;
		// �g���q����(�����u��)
		::Path_Name_From_FullPath(m_strLayerToInitFileName, strPath, strName, FALSE);
		VERIFY(strExt.LoadString(IDS_NCL_FILTER));
		m_strLayerToInitFileName = strPath + strName + strExt.Right(4);		// .ncl
		// �ݒ�̕ۑ�
		if ( m_pDoc->SaveLayerMap(m_strLayerToInitFileName) )
			pOpt->AddLayerHistory(m_strLayerToInitFileName);
	}
	pOpt->SaveInitHistory();

	// ��ė�̕ۑ�
	SetMakeNCDlgExSortColumn(IDS_REG_DXF_SORTLAYER1, m_nSortLayer);

	// CDialog::OnOK()���ĂԂ�UpdateData()����m_strNCFileName���㏑�������
	EndDialog(IDOK);
}

void CMakeNCDlgEx1::OnMKNCFileUp() 
{
	UpdateData();
	MakeDlgFileRefer(IDS_DXF2NCD_FILE, AfxGetNCVCApp()->GetFilterString(TYPE_NCD),
			this, IDC_MKNC_NCPATH, m_strNCPath, m_strNCFileName, FALSE);
	// �����I�����
	m_ctNCFileName.SetFocus();
	m_ctNCFileName.SetSel(0, -1);
}

void CMakeNCDlgEx1::OnKillFocusNCFile() 
{
	UpdateData();
	MakeDlgKillFocus(m_strNCPath, m_strNCFileName, this, IDC_MKNC_NCPATH);
}

void CMakeNCDlgEx1::OnKillFocusLayerToInit() 
{
	UpdateData();
	MakeDlgKillFocus(m_strLayerToInitPath, m_strLayerToInitFileName,
		this, IDC_MKNCEX_LAYERTOINITPATH);
}

void CMakeNCDlgEx1::OnMKNCLayerUp() 
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
	m_ctLayerToInitFileName.SetEditSel(0, -1);
	m_ctLayerToInitFileName.SetFocus();
	// �ް��̔��f�Cڲ�ؽ��ް��̕ύX
	m_pDoc->ReadLayerMap(strResult);
	// ؽĺ��۰ٍĕ`��
	m_ctLayerList.Invalidate();
}

void CMakeNCDlgEx1::OnMKNCLayerEdit() 
{
	// �ڍאݒ�
	OnLayerEdit(0);
}

void CMakeNCDlgEx1::OnSelChangeLayerToInit() 
{
	int nIndex	= MakeNCDlgSelChange(m_ctLayerToInitFileName,
			m_hWnd, IDC_MKNCEX_LAYERTOINITPATH, m_strLayerToInitPath, m_strLayerToInitFileName);

	CString	strFullPath(m_strLayerToInitPath+m_strLayerToInitFileName);
	// �ް��̔��f�Cڲ�ؽ��ް��̕ύX
	if ( !m_pDoc->ReadLayerMap(strFullPath) ) {
		// �����폜(�����ޯ��������폜)
		AfxGetNCVCApp()->GetDXFOption()->DelLayerHistory(strFullPath);
		m_ctLayerToInitFileName.DeleteString(nIndex);
	}
	else {
		// ؽĺ��۰ٍĕ`��
		m_ctLayerList.Invalidate();
	}
}

void CMakeNCDlgEx1::OnGetDispInfoLayerList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NMLVDISPINFO* pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
	CString	strBuf;

	if ( pDispInfo->item.mask & LVIF_TEXT ) {
		CLayerData* pLayer = reinterpret_cast<CLayerData *>(pDispInfo->item.lParam);
		switch ( pDispInfo->item.iSubItem ) {
		case 0:		// ڲԖ�
			lstrcpy(pDispInfo->item.pszText, pLayer->m_strLayer);
			break;
		case 1:		// �؍����̧��
			lstrcpy(pDispInfo->item.pszText, pLayer->m_strInitFile);
			break;
		case 2:		// �Ő[Z���W
			strBuf.Format(IDS_MAKENCD_FORMAT, pLayer->m_dInitZCut);
			lstrcpy(pDispInfo->item.pszText, strBuf);
			break;
		case 3:		// �ʏo��̧��
			if ( pLayer->m_bPartOut )
				lstrcpy(pDispInfo->item.pszText, pLayer->m_strNCFile);
			else
				pDispInfo->item.pszText[0] = '\0';
			break;
		}
	}
	*pResult = 0;
}

void CMakeNCDlgEx1::OnDblClkLayerList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// ����ٸد����ꂽ���ڂ𒲂ׁC�ڍאݒ��޲�۸ނ�
	OnLayerEdit(m_ctLayerList.HitTest(GetMakeNCDlgExLayerListState(m_ctLayerList)));
	*pResult = 0;
}

void CMakeNCDlgEx1::OnColumnClickLayerList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LPNMLISTVIEW pNMListView = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	m_nSortLayer = pNMListView->iSubItem;
	m_ctLayerList.SortItems(CompareFunc, m_nSortLayer);
	*pResult = 0;
}

int CALLBACK CMakeNCDlgEx1::CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lSubItem)
{
	CLayerData*	pData1 = reinterpret_cast<CLayerData *>(lParam1);
	CLayerData*	pData2 = reinterpret_cast<CLayerData *>(lParam2);
	int nResult = 0;

	switch ( lSubItem ) {
	case 0:	// ڲԖ�
		nResult = pData1->m_strLayer.CompareNoCase(pData2->m_strLayer);
		break;
	case 1:	// �؍����̧��
		nResult = pData1->m_strInitFile.CompareNoCase(pData2->m_strInitFile);
		break;
	case 2:	// �Ő[Z���W(�~��)
		nResult = (int)(pData2->m_dInitZCut * 1000.0 - pData1->m_dInitZCut * 1000.0);
		break;
	}
	return nResult;
}

void CMakeNCDlgEx1::OnLayerEdit(int nIndex)
{
	UpdateData();

	// �ڍאݒ�p�޲�۸ނ̌Ăяo��
	CString	strFullPath(m_strLayerToInitPath+m_strLayerToInitFileName);
	CMakeNCDlgEx11	dlg(this, nIndex, strFullPath);
	int	nResult = dlg.DoModal();
	// ��̫�����݂�̫������ڂ��Ă���
	m_ctOK.SetFocus();
	if ( nResult != IDOK )
		return;

	// ���ʂ̔��f
	CLayerData*	pLayer1;
	CLayerData*	pLayer2;
	try {
		for ( int i=0; i<m_ctLayerList.GetItemCount()/*dlg.m_obLayer.GetSize()*/; i++ ) {
			pLayer1 = reinterpret_cast<CLayerData *>(m_ctLayerList.GetItemData(i));
			pLayer2 = dlg.m_obLayer[i];
			pLayer1->m_bCutTarget	= pLayer2->m_bCutTarget;
			pLayer1->m_bPartOut		= pLayer2->m_bPartOut;
			pLayer1->m_strNCFile	= pLayer2->m_strNCFile;
			pLayer1->SetInitFile(pLayer2->m_strInitFile);
			m_ctLayerList.SetCheck(i, pLayer1->m_bCutTarget);
		}
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
	}

	// �V�K�ۑ�����Ă���������ޯ���̕\���X�V
	if ( strFullPath.CompareNoCase(dlg.m_strLayerFile) != 0 ) {
		::Path_Name_From_FullPath(dlg.m_strLayerFile, m_strLayerToInitPath, m_strLayerToInitFileName);
		::PathSetDlgItemPath(m_hWnd, IDC_MKNCEX_LAYERTOINITPATH, m_strLayerToInitPath);
		UpdateData(FALSE);
	}

	// ؽĺ��۰ٍĕ`��
	m_ctLayerList.Invalidate();
}

void CMakeNCDlgEx1::OnEndScrollLayerList(NMHDR *, LRESULT *)
{
	// �����ĕ`�悳���邱�ƂŌr���������邱�Ƃ�h��
	m_ctLayerList.Invalidate();
}
