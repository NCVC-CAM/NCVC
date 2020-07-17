// MakeNCD.cpp : �C���v�������e�[�V���� �t�@�C��
//

#include "stdafx.h"
#include "NCVC.h"
#include "Layer.h"
#include "DXFDoc.h"
#include "MKNCSetup.h"
#include "MKLASetup.h"
#include "MKWISetup.h"
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
	ON_CBN_KILLFOCUS(IDC_MKNC_INIT, OnKillFocusInit)
	ON_EN_KILLFOCUS(IDC_MKNC_NCFILE, OnKillFocusNCFile)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMakeNCDlg �_�C�A���O

CMakeNCDlg::CMakeNCDlg(UINT nTitle, enMAKETYPE enType, CDXFDoc* pDoc)
	: CDialog(CMakeNCDlg::IDD, NULL)
{
	m_nTitle = nTitle;
	m_enType = enType;
	//{{AFX_DATA_INIT(CMakeNCDlg)
	//}}AFX_DATA_INIT

	// �޷���Ė�����NÇ�ٖ����쐬
	CreateNCFile(pDoc, m_strNCPath, m_strNCFileName);
	// �؍�����������珉���\��̧�ق��擾
	const CDXFOption*  pOpt = AfxGetNCVCApp()->GetDXFOption();
	if ( pOpt->GetInitList(enType)->GetCount() > 0 )
		::Path_Name_From_FullPath(pOpt->GetInitList(enType)->GetHead(), m_strInitPath, m_strInitFileName);
	m_bNCView = pOpt->GetDxfFlag(DXFOPT_VIEW);
}

void CMakeNCDlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
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
	__super::OnInitDialog();
	
	// �ݽ�׸��ł͂ł��Ȃ����۰ق̏�����
	CString	strTitle;
	VERIFY(strTitle.LoadString(m_nTitle));
	SetWindowText(strTitle);

	// �؍���������ޯ���ɱ��ђǉ�
	InitialMakeNCDlgComboBox(
		AfxGetNCVCApp()->GetDXFOption()->GetInitList(m_enType),
		m_ctInitFileName);
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

	m_strInitFileName = strInitPath;
	m_strNCFileName   = strNCPath;

	// __super::OnOK()���ĂԂ�UpdateData()����m_strNCFileName���㏑�������
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
	VERIFY(strFilter.LoadString(m_enType+IDS_NCIM_FILTER));
	MakeDlgFileRefer(IDS_OPTION_INIT, strFilter, this, IDC_MKNC_INITPATH,
			m_strInitPath, m_strInitFileName, TRUE);
	// �����I�����
	m_ctInitFileName.SetEditSel(0, -1);
	m_ctInitFileName.SetFocus();
}

void CMakeNCDlg::OnMKNCInitEdit() 
{
	CString	strInitFile, strCaption, strResult;

	if ( m_enType != NCMAKEMILL ) {
		strInitFile = m_strInitPath + m_strInitFileName;
		VERIFY(strCaption.LoadString(IDS_MAKE_NCD));
	}

	UpdateData();

	switch ( m_enType ) {
	case NCMAKEMILL:
		MakeNCDlgInitFileEdit(m_strInitPath, m_strInitFileName,
			this, IDC_MKNC_INITPATH, m_ctInitFileName);
		break;

	case NCMAKELATHE:
		VERIFY(strResult.LoadString(IDCV_LATHE));
		{
			// �����菇�� MakeNCDlgInitFileEdit() �Ɠ���
			CMKLASetup	ps(::AddDialogTitle2File(strCaption, strInitFile)+strResult, strInitFile);
			if ( ps.DoModal() != IDOK )
				return;
			ps.GetNCMakeOption()->SaveMakeOption();
#ifdef _DEBUGOLD
			ps.GetNCMakeOption()->DbgDump();
#endif
			strResult = ps.GetNCMakeOption()->GetInitFile();
		}
		break;

	case NCMAKEWIRE:
		VERIFY(strResult.LoadString(IDCV_WIRE));
		{
			CMKWISetup	ps(::AddDialogTitle2File(strCaption, strInitFile)+strResult, strInitFile);
			if ( ps.DoModal() != IDOK )
				return;
			ps.GetNCMakeOption()->SaveMakeOption();
#ifdef _DEBUGOLD
			ps.GetNCMakeOption()->DbgDump();
#endif
			strResult = ps.GetNCMakeOption()->GetInitFile();
		}
		break;
	}

	if ( m_enType != NCMAKEMILL ) {
		if ( strInitFile.CompareNoCase(strResult) != 0 ) {
			CString	strPath, strFile;
			::Path_Name_From_FullPath(strResult, strPath, strFile);
			::PathSetDlgItemPath(m_hWnd, IDC_MKNC_INITPATH, strPath);
			UpdateData(FALSE);
			m_ctInitFileName.SetEditSel(0, -1);
			m_ctInitFileName.SetFocus();
		}
	}

	m_ctOK.SetFocus();
}

void CMakeNCDlg::OnSelChangeInit() 
{
	int nIndex = MakeNCDlgSelChange(m_ctInitFileName, m_hWnd, IDC_MKNC_INITPATH,
			m_strInitPath, m_strInitFileName);
	// ����̧�ق̑�������
	CString	strFullPath(m_strInitPath+m_strInitFileName);
	if ( !::IsFileExist(strFullPath) ) {
		m_ctInitFileName.DeleteString(nIndex);
		// �����폜
		AfxGetNCVCApp()->GetDXFOption()->DelInitHistory(m_enType, strFullPath);
	}
}

void CMakeNCDlg::OnKillFocusNCFile() 
{
	UpdateData();
	if ( !m_strNCFileName.IsEmpty() )
		MakeDlgKillFocus(m_strNCPath, m_strNCFileName, this, IDC_MKNC_NCPATH);
}

void CMakeNCDlg::OnKillFocusInit() 
{
	UpdateData();
	if ( !m_strInitFileName.IsEmpty() )
		MakeDlgKillFocus(m_strInitPath, m_strInitFileName, this, IDC_MKNC_INITPATH);
}

/////////////////////////////////////////////////////////////////////////////
// NC�����޲�۸ދ��ʊ֐�

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

CString MakeDlgFileRefer
	(int nTitle, const CString& strFilter, CDialog* pDlg, int nID,
			CString& strPath, CString& strFile, BOOL bRead)
{
	CString	strInitialFile;
	if ( !strFile.IsEmpty() )
		strInitialFile = strPath + strFile;
	if ( ::NCVC_FileDlgCommon(nTitle, strFilter, FALSE, strInitialFile, strPath,
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
	CString	strInitFile(strPath+strFile),
			strCaption;
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
}

BOOL InitialMakeNCDlgComboBox(const CStringList* pList, CComboBox& ctCombo)
{
	CString	strPath, strFile;
	LPCTSTR	pszFullPath;
	// �����ޯ�������߽������ւ��߲�������蓖�Ă�
	for ( POSITION pos = pList->GetHeadPosition(); pos; ) {
		pszFullPath = pList->GetNext(pos);
		::Path_Name_From_FullPath(pszFullPath, strPath, strFile);
		ctCombo.SetItemDataPtr( ctCombo.AddString(strFile), (LPVOID)pszFullPath );
	}
	return TRUE;
}
