// MakeNCDlgEx2.cpp : �C���v�������e�[�V���� �t�@�C��
//

#include "stdafx.h"
#include "NCVC.h"
#include "NCMakeOption.h"
#include "Layer.h"
#include "DXFDoc.h"
#include "MakeNCDlg.h"
#include "MakeNCDlgEx2.h"
#include "MakeNCDlgEx21.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

BEGIN_MESSAGE_MAP(CMakeNCDlgEx2, CDialog)
	//{{AFX_MSG_MAP(CMakeNCDlgEx2)
	ON_BN_CLICKED(IDC_MKNC_NCFILEUP, OnMKNCFileUp)
	ON_BN_CLICKED(IDC_MKNC_INITUP, OnMKNCInitUp)
	ON_BN_CLICKED(IDC_MKNC_INITED, OnMKNCInitEdit)
	ON_EN_KILLFOCUS(IDC_MKNC_NCFILE, OnKillFocusNCFile)
	ON_CBN_KILLFOCUS(IDC_MKNC_INIT, OnKillFocusInit)
	ON_CBN_KILLFOCUS(IDC_MKNCEX_LAYERTOINITFILE, OnKillFocusLayerToInit)
	ON_NOTIFY(LVN_GETDISPINFO, IDC_MKNCEX_LAYERLIST, OnGetDispInfoLayerList)
	ON_NOTIFY(NM_DBLCLK, IDC_MKNCEX_LAYERLIST, OnDblClkLayerList)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_MKNCEX_LAYERLIST, OnColumnClickLayerList)
	ON_NOTIFY(LVN_ENDSCROLL, IDC_MKNCEX_LAYERLIST, OnEndScrollLayerList)
	ON_CBN_SELCHANGE(IDC_MKNCEX_LAYERTOINITFILE, OnSelChangeLayerToInit)
	ON_CBN_SELCHANGE(IDC_MKNC_INIT, OnSelChangeInit)
	ON_BN_CLICKED(IDC_MKNCEX_LAYERTOINIT_ED, OnMKNCLayerEdit)
	ON_BN_CLICKED(IDC_MKNCEX_LAYERTOINIT_UP, OnMKNCLayerUp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMakeNCDlgEx2 �_�C�A���O

CMakeNCDlgEx2::CMakeNCDlgEx2(CDXFDoc* pDoc)
	: CDialog(CMakeNCDlgEx2::IDD, NULL)
{
	m_pDoc = pDoc;
	//{{AFX_DATA_INIT(CMakeNCDlgEx2)
	//}}AFX_DATA_INIT

	// �޷���Ė�����NÇ�ٖ����쐬
	CreateNCFile(pDoc, m_strNCPath, m_strNCFileName);	// MakeNCDlg.cpp

	CDXFOption*	pOpt = AfxGetNCVCApp()->GetDXFOption();
	m_bNCView = pOpt->GetDxfFlag(DXFOPT_VIEW);
	// ��ė�̎擾
	m_nSortLayer = GetMakeNCDlgExSortColumn(IDS_REG_DXF_SORTLAYER2);	// MakeNCDlg.cpp

	// ����̧�ق̾��
	if ( pOpt->GetInitList()->GetCount() > 0 )
		::Path_Name_From_FullPath(pOpt->GetInitList()->GetHead(), m_strInitPath, m_strInitFileName);

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

CMakeNCDlgEx2::~CMakeNCDlgEx2()
{
}

void CMakeNCDlgEx2::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMakeNCDlgEx2)
	DDX_Control(pDX, IDOK, m_ctOK);
	DDX_Control(pDX, IDC_MKNC_NCFILE, m_ctNCFileName);
	DDX_Text(pDX, IDC_MKNC_NCFILE, m_strNCFileName);
	DDX_Control(pDX, IDC_MKNC_INIT, m_ctInitFileName);
	DDX_CBString(pDX, IDC_MKNC_INIT, m_strInitFileName);
	DDX_Control(pDX, IDC_MKNCEX_LAYERTOINITFILE, m_ctLayerToInitFileName);
	DDX_CBString(pDX, IDC_MKNCEX_LAYERTOINITFILE, m_strLayerToInitFileName);
	DDX_Control(pDX, IDC_MKNCEX_LAYERLIST, m_ctLayerList);
	DDX_Check(pDX, IDC_MKNC_VIEW, m_bNCView);
	//}}AFX_DATA_MAP
}

/////////////////////////////////////////////////////////////////////////////
// CMakeNCDlgEx2 ���b�Z�[�W �n���h��

BOOL CMakeNCDlgEx2::OnInitDialog() 
{
	extern	LPCTSTR	g_szListHeader[];

	CDialog::OnInitDialog();

	// �ݽ�׸��ł͂ł��Ȃ����۰ق̏�����
	
	// �؍���������ޯ���ɱ��ђǉ�
	InitialMakeNCDlgComboBox(AfxGetNCVCApp()->GetDXFOption()->GetInitList(),
			m_ctInitFileName);	// MakeNCDlg.cpp
	// �֌W̧�ٺ����ޯ���ɱ��ђǉ�
	InitialMakeNCDlgComboBox(AfxGetNCVCApp()->GetDXFOption()->GetLayerToInitList(),
			m_ctLayerToInitFileName);
	m_ctLayerToInitFileName.SetCurSel(-1);	// �����l�͑I���Ȃ�
	// �߽�\���̍œK��
	::PathSetDlgItemPath(m_hWnd, IDC_MKNC_NCPATH, m_strNCPath);
	::PathSetDlgItemPath(m_hWnd, IDC_MKNC_INITPATH, m_strInitPath);
//	::PathSetDlgItemPath(m_hWnd, IDC_MKNCEX_LAYERTOINITPATH, m_strLayerToInitPath);
	// ؽĺ��۰ق̗�}��
	CRect	rc;
	m_ctLayerList.GetClientRect(&rc);
	int nWidth  = m_ctLayerList.GetStringWidth(CString('W',10));
	m_ctLayerList.InsertColumn(0, g_szListHeader[0], LVCFMT_LEFT, nWidth-2);
	m_ctLayerList.InsertColumn(1, g_szListHeader[3], LVCFMT_RIGHT, nWidth+2);
	m_ctLayerList.InsertColumn(2, g_szListHeader[4], LVCFMT_LEFT, rc.Width()-nWidth*2-17);
	// ؽĺ��۰ق̏�����
	InitialMakeNCDlgExLayerListCtrl(m_pDoc, m_ctLayerList);
	// ڲԖ��ŕ��בւ�
	m_ctLayerList.SortItems(CompareFunc, m_nSortLayer);

	return TRUE;  // �R���g���[���Ƀt�H�[�J�X��ݒ肵�Ȃ��Ƃ��A�߂�l�� TRUE �ƂȂ�܂�
	              // ��O: OCX �v���p�e�B �y�[�W�̖߂�l�� FALSE �ƂȂ�܂�
}

void CMakeNCDlgEx2::OnOK() 
{
	extern	LPCTSTR	gg_szCat;

	m_ctOK.SetFocus();
	UpdateData();
	CString	strInitPath(m_strInitPath+m_strInitFileName),
			strNCPath(m_strNCPath+m_strNCFileName);
	
	// �ŏI��������(�ڍאݒ�Ő؍�ΏۊO�Ƃ��C�����Ő؍�Ώۂ�������������\��)
	// �؍폇�������׸ނ̏�Ԃ��擾�C�؍����̧�ق̍ŏI����
	if ( !CheckMakeNCDlgExLayerState(strNCPath, m_ctNCFileName, m_ctLayerList, FALSE) )
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
	// �؍����̧�ق�����
	if ( !::IsFileExist(strInitPath) ) {	// NCVC.cpp
		m_ctInitFileName.SetEditSel(0, -1);
		m_ctInitFileName.SetFocus();
		return;
	}
	// R�_�Ƌ���Z���W������
	CNCMakeOption	optMake(strInitPath);
	CLayerData*	pLayer;
	CString		strMiss;
	int			i, nResult = -1, nLoop = m_pDoc->GetLayerCnt();
	for ( i=0; i<nLoop; i++ ) {
		pLayer = m_pDoc->GetLayerData(i);
		if ( !pLayer->m_bCutTarget || optMake.GetDbl(MKNC_DBL_ZG0STOP) > pLayer->m_dZCut )
			continue;
		// �װү���ނ̐���
		if ( !strMiss.IsEmpty() )
			strMiss += gg_szCat;
		strMiss += pLayer->m_strLayer;
		if ( nResult < 0 )
			nResult = i;
	}
	if ( !strMiss.IsEmpty() ) {
		CString	strMsg;
		strMsg.Format(IDS_ERR_MAKEMULTILAYER_Z, strMiss);
		AfxMessageBox(strMsg, MB_OK|MB_ICONEXCLAMATION);
		SetFocusListCtrl(m_ctLayerList, nResult);
		return;
	}
	m_strNCFileName = strNCPath;
	// ��߼�ݏ�ԕۑ�
	CDXFOption*	pOpt = AfxGetNCVCApp()->GetDXFOption();
	pOpt->SetViewFlag(m_bNCView);
	// �؍��������
	pOpt->AddInitHistory(strInitPath);
	// ڲԖ��Ɛ؍����̧�ق̊֌W
	if ( !m_strLayerToInitFileName.IsEmpty() ) {
		// �ۑ��O���߽����
		if ( m_strLayerToInitPath.IsEmpty() )
			m_strLayerToInitFileName = m_strInitPath + m_strLayerToInitFileName;
		else
			m_strLayerToInitFileName = m_strLayerToInitPath + m_strLayerToInitFileName;
		// �g���q����(�����u��)
		CString	strPath, strName, strExt;
		::Path_Name_From_FullPath(m_strLayerToInitFileName, strPath, strName, FALSE);
		VERIFY(strExt.LoadString(IDS_NCL_FILTER));
		m_strLayerToInitFileName = strPath + strName + strExt.Right(4);		// .ncl
		// �ݒ�̕ۑ�
		if ( m_pDoc->SaveLayerMap(m_strLayerToInitFileName) )
			pOpt->AddLayerHistory(m_strLayerToInitFileName);
	}
	pOpt->SaveInitHistory();

	// ��ė�̕ۑ�
	SetMakeNCDlgExSortColumn(IDS_REG_DXF_SORTLAYER2, m_nSortLayer);

	// CDialog::OnOK()���ĂԂ�UpdateData()����m_strNCFileName���㏑�������
	EndDialog(IDOK);
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
	VERIFY(strFilter.LoadString(IDS_NCI_FILTER));
	MakeDlgFileRefer(IDS_OPTION_INIT, strFilter, this, IDC_MKNC_INITPATH,
			m_strInitPath, m_strInitFileName, TRUE);
	// �����I�����
	m_ctInitFileName.SetEditSel(0, -1);
	m_ctInitFileName.SetFocus();
}

void CMakeNCDlgEx2::OnMKNCInitEdit() 
{
	UpdateData();
	MakeNCDlgInitFileEdit(m_strInitPath, m_strInitFileName,
		this, IDC_MKNC_INITPATH, m_ctInitFileName);
	m_ctOK.SetFocus();
}

void CMakeNCDlgEx2::OnSelChangeInit() 
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
	m_ctLayerToInitFileName.SetEditSel(0, -1);
	m_ctLayerToInitFileName.SetFocus();
	// �ް��̔��f�Cڲ�ؽ��ް��̕ύX
	m_pDoc->ReadLayerMap(strResult);
	// ؽĺ��۰ٍĕ`��
	m_ctLayerList.Invalidate();
}

void CMakeNCDlgEx2::OnMKNCLayerEdit() 
{
	// �ڍאݒ�
	OnLayerEdit(0);
}

void CMakeNCDlgEx2::OnSelChangeLayerToInit() 
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

void CMakeNCDlgEx2::OnKillFocusNCFile() 
{
	UpdateData();
	MakeDlgKillFocus(m_strNCPath, m_strNCFileName, this, IDC_MKNC_NCPATH);
}

void CMakeNCDlgEx2::OnKillFocusInit() 
{
	UpdateData();
	MakeDlgKillFocus(m_strInitPath, m_strInitFileName, this, IDC_MKNC_INITPATH);
}

void CMakeNCDlgEx2::OnKillFocusLayerToInit() 
{
	UpdateData();
	MakeDlgKillFocus(m_strLayerToInitPath, m_strLayerToInitFileName,
		this, IDC_MKNCEX_LAYERTOINITPATH);
}

void CMakeNCDlgEx2::OnGetDispInfoLayerList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NMLVDISPINFO* pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
	CString	strBuf;

	if ( pDispInfo->item.mask & LVIF_TEXT ) {
		CLayerData* pLayer = (CLayerData *)pDispInfo->item.lParam;
		switch ( pDispInfo->item.iSubItem ) {
		case 0:		// ڲԖ�
			lstrcpy(pDispInfo->item.pszText, pLayer->m_strLayer);
			break;
		case 1:		// �����Ő[Z���W
			strBuf.Format(IDS_MAKENCD_FORMAT, pLayer->m_dZCut);
			lstrcpy(pDispInfo->item.pszText, strBuf);
			break;
		case 2:		// �ʏo��̧��
			if ( pLayer->m_bPartOut )
				lstrcpy(pDispInfo->item.pszText, pLayer->m_strNCFile);
			else
				pDispInfo->item.pszText[0] = '\0';
			break;
		}
	}
	*pResult = 0;
}

void CMakeNCDlgEx2::OnDblClkLayerList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// ����ٸد����ꂽ���ڂ𒲂ׁC�ڍאݒ��޲�۸ނ�
	OnLayerEdit(m_ctLayerList.HitTest(GetMakeNCDlgExLayerListState(m_ctLayerList)));
	*pResult = 0;
}

void CMakeNCDlgEx2::OnColumnClickLayerList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LPNMLISTVIEW pNMListView = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	m_nSortLayer = pNMListView->iSubItem;
	m_ctLayerList.SortItems(CompareFunc, m_nSortLayer);
	*pResult = 0;
}

int CALLBACK CMakeNCDlgEx2::CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lSubItem)
{
	CLayerData*	pData1 = (CLayerData *)lParam1;
	CLayerData*	pData2 = (CLayerData *)lParam2;
	int nResult = 0;

	switch ( lSubItem ) {
	case 0:	// ڲԖ�
		nResult = pData1->m_strLayer.CompareNoCase(pData2->m_strLayer);
		break;
	case 1:	// �����Ő[Z���W(�~��)
		nResult = (int)(pData2->m_dZCut * 1000.0 - pData1->m_dZCut * 1000.0);
		break;
	}
	return nResult;
}

void CMakeNCDlgEx2::OnLayerEdit(int nIndex)
{
	UpdateData();

	// �ڍאݒ�p�޲�۸ނ̌Ăяo��
	CString	strFullPath(m_strLayerToInitPath+m_strLayerToInitFileName);
	CMakeNCDlgEx21	dlg(this, nIndex, strFullPath);
	int	nResult = dlg.DoModal();
	// ��̫�����݂�̫������ڂ��Ă���
	m_ctOK.SetFocus();
	if ( nResult != IDOK )
		return;

	// ���ʂ̔��f
	CLayerData*	pData1;
	CLayerData*	pData2;
	for ( int i=0; i<m_ctLayerList.GetItemCount()/*dlg.GetLayerArray()->GetSize()*/; i++ ) {
		pData1 = (CLayerData *)(m_ctLayerList.GetItemData(i));
		pData2 = dlg.m_obLayer[i];
		pData1->m_bCutTarget= pData2->m_bCutTarget;
		pData1->m_dZCut		= pData2->m_dZCut;
		pData1->m_bDrillZ	= pData2->m_bDrillZ;
		pData1->m_bPartOut	= pData2->m_bPartOut;
		pData1->m_strNCFile	= pData2->m_strNCFile;
		m_ctLayerList.SetCheck(i, pData1->m_bCutTarget);
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

void CMakeNCDlgEx2::OnEndScrollLayerList(NMHDR *, LRESULT *)
{
	// �����ĕ`�悳���邱�ƂŌr���������邱�Ƃ�h��
	m_ctLayerList.Invalidate();
}
