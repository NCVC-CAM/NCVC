// MakeNCD.cpp : �C���v�������e�[�V���� �t�@�C��
//

#include "stdafx.h"
#include "NCVC.h"
#include "NCMakeOption.h"
#include "Layer.h"
#include "DXFDoc.h"
#include "MKNCSetup.h"
#include "MakeNCDlg.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

BEGIN_MESSAGE_MAP(CMakeNCDlg, CDialog)
	//{{AFX_MSG_MAP(CMakeNCDlg)
	ON_BN_CLICKED(IDC_MKNC_NCFILEUP, OnMKNCFileUp)
	ON_BN_CLICKED(IDC_MKNC_INITUP, OnMKNCInitUp)
	ON_BN_CLICKED(IDC_MKNC_INITED, OnMKNCInitEdit)
	ON_CBN_SELCHANGE(IDC_MKNC_INIT, OnSelChangeInit)
	ON_EN_KILLFOCUS(IDC_MKNC_NCFILE, OnKillFocusNCFile)
	ON_CBN_KILLFOCUS(IDC_MKNC_INIT, OnKillFocusInit)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMakeNCDlg �_�C�A���O

CMakeNCDlg::CMakeNCDlg(UINT nTitle, CDXFDoc* pDoc)
	: CDialog(CMakeNCDlg::IDD, NULL)
{
	m_nTitle = nTitle;
	//{{AFX_DATA_INIT(CMakeNCDlg)
	//}}AFX_DATA_INIT

	// �޷���Ė�����NÇ�ٖ����쐬
	CreateNCFile(pDoc, m_strNCPath, m_strNCFileName);
	// �؍�����������珉���\��̧�ق��擾
	const	CDXFOption*	pOpt = AfxGetNCVCApp()->GetDXFOption();
	if ( pOpt->GetInitList()->GetCount() > 0 )
		::Path_Name_From_FullPath(pOpt->GetInitList()->GetHead(),
				m_strInitPath, m_strInitFileName);
	m_bNCView = pOpt->GetDxfFlag(DXFOPT_VIEW);
}

void CMakeNCDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMakeNCDlg)
	DDX_Control(pDX, IDOK, m_ctOK);
	DDX_Control(pDX, IDC_MKNC_NCFILE, m_ctNCFileName);
	DDX_Text(pDX, IDC_MKNC_NCFILE, m_strNCFileName);
	DDX_Control(pDX, IDC_MKNC_INIT, m_ctInitFileName);
	DDX_CBString(pDX, IDC_MKNC_INIT, m_strInitFileName);
	DDX_Check(pDX, IDC_MKNC_VIEW, m_bNCView);
	//}}AFX_DATA_MAP
}

/////////////////////////////////////////////////////////////////////////////
// CMakeNCDlg ���b�Z�[�W �n���h��

BOOL CMakeNCDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// �ݽ�׸��ł͂ł��Ȃ����۰ق̏�����
	CString	strTitle;
	strTitle.LoadString(m_nTitle);
	SetWindowText(strTitle);

	// �؍���������ޯ���ɱ��ђǉ�
	InitialMakeNCDlgComboBox(AfxGetNCVCApp()->GetDXFOption()->GetInitList(), m_ctInitFileName);
	// �߽�\���̍œK��(shlwapi.h)
	::PathSetDlgItemPath(m_hWnd, IDC_MKNC_NCPATH,   m_strNCPath);
	::PathSetDlgItemPath(m_hWnd, IDC_MKNC_INITPATH, m_strInitPath);

	return TRUE;  // �R���g���[���Ƀt�H�[�J�X��ݒ肵�Ȃ��Ƃ��A�߂�l�� TRUE �ƂȂ�܂�
	              // ��O: OCX �v���p�e�B �y�[�W�̖߂�l�� FALSE �ƂȂ�܂�
}

void CMakeNCDlg::OnOK() 
{
	m_ctOK.SetFocus();
	UpdateData();
	CString	strInitPath(m_strInitPath+m_strInitFileName),
			strNCPath(m_strNCPath+m_strNCFileName);

	// �؍����̧�ق�����(̧�ٕK�{)
	if ( !::IsFileExist(strInitPath) ) {	// NCVC.cpp
		m_ctInitFileName.SetEditSel(0, -1);
		m_ctInitFileName.SetFocus();
		return;
	}
	// NC����̧�ق�����(����Ώ㏑���m�F)
	if ( !CheckMakeDlgFileExt(TYPE_NCD, strNCPath) ||
			!::IsFileExist(strNCPath, FALSE) ) {
		m_ctNCFileName.SetFocus();
		m_ctNCFileName.SetSel(0, -1);
		return;
	}
	m_strNCFileName = strNCPath;

	// ���������X�V
	CDXFOption*	pOpt = AfxGetNCVCApp()->GetDXFOption();
	pOpt->SetViewFlag(m_bNCView);
	pOpt->AddInitHistory(strInitPath);
	pOpt->SaveInitHistory();

	// CDialog::OnOK()���ĂԂ�UpdateData()����m_strNCFileName���㏑�������
	EndDialog(IDOK);
}

void CMakeNCDlg::OnMKNCFileUp() 
{
	UpdateData();
	MakeDlgFileRefer(IDS_DXF2NCD_FILE, AfxGetNCVCApp()->GetFilterString(TYPE_NCD),
			this, IDC_MKNC_NCPATH, m_strNCPath, m_strNCFileName, FALSE);
	// �����I�����
	m_ctNCFileName.SetFocus();
	m_ctNCFileName.SetSel(0, -1);
}

void CMakeNCDlg::OnMKNCInitUp() 
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

void CMakeNCDlg::OnMKNCInitEdit() 
{
	UpdateData();
	MakeNCDlgInitFileEdit(m_strInitPath, m_strInitFileName,
		this, IDC_MKNC_INITPATH, m_ctInitFileName);
	m_ctOK.SetFocus();
}

void CMakeNCDlg::OnSelChangeInit() 
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

void CMakeNCDlg::OnKillFocusNCFile() 
{
	UpdateData();
	MakeDlgKillFocus(m_strNCPath, m_strNCFileName, this, IDC_MKNC_NCPATH);
}

void CMakeNCDlg::OnKillFocusInit() 
{
	UpdateData();
	MakeDlgKillFocus(m_strInitPath, m_strInitFileName, this, IDC_MKNC_INITPATH);
}

/////////////////////////////////////////////////////////////////////////////
// NC�����޲�۸ދ��ʊ֐�

void SetFocusListCtrl(CListCtrl& ctListCtrl, int nIndex)
{
	if ( nIndex < 0 )
		nIndex = 0;
	ctListCtrl.SetFocus();
	ctListCtrl.SetItemState(nIndex, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
	ctListCtrl.EnsureVisible(nIndex, FALSE);
}

void CreateNCFile(const CDXFDoc* pDoc, CString& strPath, CString& strFile)
{
	// �޷���Ė�����NÇ�ٖ����쐬�D���������ɐ�������̧�ٖ�������Ȃ炻����̗p
	CString	strNCFile(pDoc->GetNCFileName());
	if ( strNCFile.IsEmpty() ) {
		::Path_Name_From_FullPath(pDoc->GetPathName(), strPath, strFile, FALSE);
		strFile += AfxGetNCVCApp()->GetDocExtString(TYPE_NCD);
	}
	else
		::Path_Name_From_FullPath(strNCFile, strPath, strFile);
}

void CreateLayerFile(const CDXFDoc* pDoc, CString& strPath, CString& strFile)
{
	// �޷���Ė�����ڲ�̧�ٖ����쐬
	::Path_Name_From_FullPath(pDoc->GetPathName(), strPath, strFile, FALSE);
	CString	strExt;
	VERIFY(strExt.LoadString(IDS_NCL_FILTER));
	strFile += strExt.Right(4);		// .ncl
}

BOOL CheckMakeDlgFileExt(DOCTYPE enType, CString& strFile)
{
	// �ۑ�̧�قɊg���q�̒ǉ�
	if ( strFile.IsEmpty() ) {
		AfxMessageBox(IDS_ERR_FILEPATH, MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}
	TCHAR	szFileName[_MAX_FNAME],
			szExt[_MAX_EXT];
	_splitpath_s(strFile, NULL, 0, NULL, 0,
		szFileName, SIZEOF(szFileName), szExt, SIZEOF(szExt));
	if ( lstrlen(szFileName) <= 0 ) {
		AfxMessageBox(IDS_ERR_FILEPATH, MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}
	// �o�^�g���q�Ȃ炻�̂܂܁C�łȂ��Ȃ���̫�Ċg���q��t�^
	if ( lstrlen(szExt) <= 1 ||		// �u.�v�݂̂��C����ȉ�
			!AfxGetNCVCApp()->GetDocTemplate(enType)->IsExtension(szExt+1) ) {	// �u.�v����
		if ( enType == TYPE_NCD )
			strFile += AfxGetNCVCApp()->GetDocExtString(TYPE_NCD);	// .ncd
		else {
			CString	strFilter;
			VERIFY(strFilter.LoadString(IDS_DXF_FILTER));
			strFile += '.' + strFilter.Left(3);	// .dxf
		}
	}

	return TRUE;
}

// CLayerData friend �֐�
BOOL CheckMakeNCDlgExLayerState
	(CString& strNCFile, CEdit& ctNCFile, CListCtrl& ctLayerList, BOOL bIniCheck)
{
	CLayerData*	pLayer;
	CString		strFile;
	BOOL		bCutFlg, bMainCheck = TRUE, bFirstCheck = TRUE, bCutCheck = FALSE;

	// ؽĺ��۰ق���CLayerData*���擾���C���ׂ�����
	for ( int i=0; i<ctLayerList.GetItemCount(); i++ ) {
		pLayer = (CLayerData *)(ctLayerList.GetItemData(i));
		// �؍�Ώۂ��ۂ�
		bCutFlg = ctLayerList.GetCheck(i);
		pLayer->SetCutTargetFlg(bCutFlg);
		// Ҳݏo��̧�ق�����(�ʏo���׸ނ��ݒ肳��Ă��Ȃ��Ƃ�)
		if ( bMainCheck && bCutFlg && !pLayer->IsPartOut() ) {
			bMainCheck = FALSE;	// Ҳݏo��̧�ق������͈�x����
			if ( !CheckMakeDlgFileExt(TYPE_NCD, strNCFile) ||
					!::IsFileExist(strNCFile, FALSE) ) {
				ctNCFile.SetFocus();
				ctNCFile.SetSel(0, -1);
				return FALSE;
			}
		}
		// �ȉ��؍�Ώۂ̂�
		if ( !bCutFlg )
			continue;
		bCutCheck = TRUE;	// �؍�Ώۂ���I
		// �ʏo�͏��̐ݒ�
		if ( pLayer->IsPartOut() ) {
			// �ʏo��̧�ق�����
			strFile = pLayer->GetNCFile();
			if ( !CheckMakeDlgFileExt(TYPE_NCD, strFile) ) {
				SetFocusListCtrl(ctLayerList, i);
				return FALSE;
			}
			// �g���q�t������Ă��ڲԏ��ɍĐݒ�
			if ( strFile.CompareNoCase(pLayer->GetNCFile()) )
				pLayer->SetNCFile(strFile);
			// �㏑���m�F
			if ( bFirstCheck ) {
				bFirstCheck = FALSE;	// �擪�����㏑������
				if ( !::IsFileExist(strFile, FALSE) ) {
					SetFocusListCtrl(ctLayerList, i);
					return FALSE;
				}
			}
			// �ʏo�͂��擪�ɂȂ�悤ؽć���ݒ�
			pLayer->SetListNo(-1);
		}
		else
			pLayer->SetListNo(i);
		// �؍����̧�ق�����(CMakeNCDlgEx1�̂�)
		if ( bIniCheck && !::IsFileExist(pLayer->GetInitFile()) ) {	// NCVC.cpp
			SetFocusListCtrl(ctLayerList, i);
			return FALSE;
		}
	}

	// �؍�Ώۂ��P���Ȃ�
	if ( !bCutCheck ) {
		AfxMessageBox(IDS_ERR_MAKEMULTILAYER, MB_OK|MB_ICONEXCLAMATION);
		SetFocusListCtrl(ctLayerList, 0);
		return FALSE;
	}

	return TRUE;
}

CString MakeDlgFileRefer
	(int nTitle, const CString& strFilter, CDialog* pDlg, int nID,
			CString& strPath, CString& strFile, BOOL bRead)
{
	CString	strInitialFile;
	if ( !strFile.IsEmpty() )
		strInitialFile = strPath + strFile;
	if ( ::NCVC_FileDlgCommon(nTitle, strFilter, strInitialFile, strPath,
				bRead, OFN_HIDEREADONLY|OFN_PATHMUSTEXIST) == IDOK ) {
		::Path_Name_From_FullPath(strInitialFile, strPath, strFile);
		::PathSetDlgItemPath(pDlg->m_hWnd, nID, strPath);
		// �ް��̔��f
		pDlg->UpdateData(FALSE);
	}
	else
		strInitialFile.Empty();
	// �I��̧��
	return strInitialFile;
}

void MakeNCDlgInitFileEdit
	(CString& strPath, CString& strFile, CDialog* pDlg, int nID, CComboBox& ctFile)
{
	CString	strInitFile(strPath+strFile);
	CString	strCaption;
	VERIFY(strCaption.LoadString(IDS_MAKE_NCD));
	CMKNCSetup	ps(::AddDialogTitle2File(strCaption, strInitFile), strInitFile);
	if ( ps.DoModal() != IDOK )
		return;

	// ��߼�݂̕ۑ�
	ps.GetNCMakeOption()->SaveMakeOption();
#ifdef _DEBUGOLD
	ps.GetNCMakeOption()->DbgDump();
#endif

	// �V�K�ۑ�����Ă�����C�p�X�\���̍X�V
	CString	strResult = ps.GetNCMakeOption()->GetInitFile();
	if ( strInitFile.CompareNoCase(strResult) != 0 ) {
		::Path_Name_From_FullPath(strResult, strPath, strFile);
		::PathSetDlgItemPath(pDlg->m_hWnd, nID, strPath);
		pDlg->UpdateData(FALSE);
		ctFile.SetEditSel(0, -1);
		ctFile.SetFocus();
	}
}

int MakeNCDlgSelChange
	(const CComboBox& ctCombo, HWND hWnd, int nID, CString& strPath, CString& strFile)
{
	// �����ޯ���ɐݒ肳�ꂽ32�ޯĒl -> ���߽������ւ��߲��
	int		nIndex = ctCombo.GetCurSel();
	ASSERT( nIndex >= 0 );
	::Path_Name_From_FullPath((LPCTSTR)ctCombo.GetItemDataPtr(nIndex), strPath, strFile);
	::PathSetDlgItemPath(hWnd, nID, strPath);	// �߽�̏ȗ��`
	return nIndex;
}

void MakeDlgKillFocus
	(CString& strPath, CString& strFile, CDialog* pDlg, int nID)
{
	CString		strResultPath, strResultFile;
	::Path_Name_From_FullPath(strFile, strResultPath, strResultFile);
	if ( !strResultPath.IsEmpty() ) {
		strPath = strResultPath;
		strFile = strResultFile;
		::PathSetDlgItemPath(pDlg->m_hWnd, nID, strPath);
		pDlg->UpdateData(FALSE);
	}
/*
	::Path_Name_From_FullPath(strFile, strPath, strFile);
	if ( !strPath.IsEmpty() )
		::PathSetDlgItemPath(pDlg->m_hWnd, nID, strPath);
	else {
		HWND hWnd = ::GetDlgItem(pDlg->m_hWnd, nID);
		if ( hWnd )
			::SetWindowText(hWnd, "");
	}
	pDlg->UpdateData(FALSE);
*/
}

int GetMakeNCDlgExSortColumn(UINT nID)
{
	CString		strRegKey, strEntry;
	VERIFY(strRegKey.LoadString(IDS_REGKEY_DXF));
	VERIFY(strEntry.LoadString(nID));
	return (int)(AfxGetApp()->GetProfileInt(strRegKey, strEntry, 0));
}

void SetMakeNCDlgExSortColumn(UINT nID, int nSort)
{
	CString		strRegKey, strEntry;
	VERIFY(strRegKey.LoadString(IDS_REGKEY_DXF));
	VERIFY(strEntry.LoadString(nID));
	AfxGetApp()->WriteProfileInt(strRegKey, strEntry, nSort);
}

CPoint GetMakeNCDlgExLayerListState(const CListCtrl& ctLayerList)
{
	// ����ٸد����ꂽ���ڂ𒲂ׂ�
	DWORD	dwPos = ::GetMessagePos();
	CPoint	pt((int)LOWORD(dwPos), (int)HIWORD(dwPos));
	ctLayerList.ScreenToClient(&pt);
	return pt;
}

BOOL InitialMakeNCDlgComboBox(const CStringList* pList, CComboBox& ctCombo)
{
	CString		strPath, strFile;
	LPCTSTR		pszFullPath;
	// �����ޯ�������߽������ւ��߲�������蓖�Ă�
	for ( POSITION pos = pList->GetHeadPosition(); pos; ) {
		pszFullPath = pList->GetNext(pos);
		::Path_Name_From_FullPath(pszFullPath, strPath, strFile);
		ctCombo.SetItemDataPtr( ctCombo.AddString(strFile), (LPVOID)pszFullPath );
	}
	return TRUE;
}

BOOL InitialMakeNCDlgExLayerListCtrl(CDXFDoc* pDoc, CListCtrl& ctLayerList)
{
	// ؽĺ��۰ق̊g������
	DWORD	dwStyle = ctLayerList.GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT|LVS_EX_CHECKBOXES|LVS_EX_GRIDLINES;
	ctLayerList.SetExtendedStyle(dwStyle);
	// ڲ�ؽĺ��۰قւ̓o�^
	LV_ITEM		lvi;
	lvi.mask = LVIF_TEXT | LVIF_PARAM;
	lvi.iSubItem = 0;
	lvi.pszText = LPSTR_TEXTCALLBACK;
	CLayerData*	pLayer;
	int		i, nCnt, nLoop = pDoc->GetLayerCnt();

	for ( i=0, nCnt=0; i<nLoop; i++ ) {
		pLayer = pDoc->GetLayerData(i);
		if ( pLayer->IsCutType() ) {
			lvi.iItem = nCnt;
			lvi.lParam = (LPARAM)pLayer;
			if ( ctLayerList.InsertItem(&lvi) < 0 ) {
				CString	strMsg;
				strMsg.Format(IDS_ERR_ADDITEM, nCnt+1);
				AfxMessageBox(strMsg, MB_OK|MB_ICONSTOP);
				return FALSE;
			}
			ctLayerList.SetCheck(nCnt++, pLayer->IsViewLayer());
		}
	}
	return TRUE;
}
