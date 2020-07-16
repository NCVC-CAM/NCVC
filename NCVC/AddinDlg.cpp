// AddinDlg.cpp : �C���v�������e�[�V���� �t�@�C��
//

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "AddinDlg.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

BEGIN_MESSAGE_MAP(CAddinDlg, CDialog)
	//{{AFX_MSG_MAP(CAddinDlg)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_ADDIN_LIST, OnItemChangedAddinList)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAddinDlg �_�C�A���O

CAddinDlg::CAddinDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAddinDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAddinDlg)
	//}}AFX_DATA_INIT
}

void CAddinDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAddinDlg)
	DDX_Control(pDX, IDC_ADDIN_INFO, m_ctAddnInfo);
	DDX_Control(pDX, IDC_ADDIN_README, m_ctReadMe);
	DDX_Control(pDX, IDC_ADDIN_LIST, m_ctList);
	DDX_Text(pDX, IDC_ADDIN_INFO, m_strAddinInfo);
	DDX_Text(pDX, IDC_ADDIN_README, m_strReadMe);
	//}}AFX_DATA_MAP
}

/////////////////////////////////////////////////////////////////////////////
// CAddinDlg ���ފ֐�

void CAddinDlg::SetDetailData(CNCVCaddinIF* pAddin)
{
	ASSERT( pAddin );

	// �ް�ޮݏ��
	CString	strVerInfo;
	LPDWORD	pdwTrans;
	LPVOID	pVerInfo = ::GetVersionResource( pAddin->GetFileName(), &pdwTrans );	// StdAfx.cpp
	if ( pVerInfo ) {
		::GetVersionValue( strVerInfo, pVerInfo, *pdwTrans, "ProductVersion" ); // ���i�ް�ޮ݂��擾
		delete[]	pVerInfo;
	}
	// �ڍ׏��
	m_strAddinInfo.Format(IDS_ADDIN_INFO, 
		pAddin->GetCopyright(), strVerInfo, pAddin->GetSupport(), pAddin->GetComment() );
	// ReadMe ̧�ٌ���
	m_strReadMe.Empty();
	CString	strPath, strName, strFilter;
	::Path_Name_From_FullPath(pAddin->GetFileName(), strPath, strName, FALSE);
	VERIFY(strFilter.LoadString(IDS_TXT_FILTER));
	strName += strFilter.Right(4);	// ".txt";
	CFile	fp;
	if ( fp.Open(strPath+strName, CFile::modeRead) ) {
		HANDLE hMap = CreateFileMapping((HANDLE)(fp.m_hFile), NULL,
							PAGE_READONLY, 0, 0, NULL);
		if ( hMap ) {
			LPVOID pMap = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
			if ( pMap ) {
				m_strReadMe = (LPCTSTR)pMap;
				UnmapViewOfFile(pMap);
			}
			CloseHandle(hMap);
		}
	}

	UpdateData(FALSE);
}

/////////////////////////////////////////////////////////////////////////////
// CAddinDlg ���b�Z�[�W �n���h��

BOOL CAddinDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	// ؽĺ��۰ق̲Ұ�޾��
	m_ctList.SetImageList(AfxGetNCVCMainWnd()->GetAddinImage(), LVSIL_NORMAL);
	// ؽĺ��۰قւ̓o�^
	int		i, nCnt = AfxGetNCVCApp()->GetAddinArray()->GetSize();
	CNCVCaddinIF*	pAddin;
	LV_ITEM	lvi;
	lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
	lvi.iSubItem = 0;
	for ( i=0; i<nCnt; i++ ) {
		pAddin = AfxGetNCVCApp()->GetAddinArray()->GetAt(i);
		lvi.iItem	= i;
		lvi.pszText = const_cast<LPTSTR>((LPCTSTR)pAddin->GetDLLName());
		lvi.iImage	= i;
		lvi.lParam = (LPARAM)pAddin;
		if ( m_ctList.InsertItem(&lvi) < 0 ) {
			CString	strMsg;
			strMsg.Format(IDS_ERR_ADDITEM, i+1);
			AfxMessageBox(strMsg, MB_OK|MB_ICONSTOP);
			break;
		}
	}
	if ( nCnt > 0 )
		m_ctList.SetItemState(0, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);

	// XpStyle����
	::DisableXpStyle(m_ctList.m_hWnd);	// StdAfx.h
	::DisableXpStyle(m_ctAddnInfo.m_hWnd);
	::DisableXpStyle(m_ctReadMe.m_hWnd);

	return TRUE;  // �R���g���[���Ƀt�H�[�J�X��ݒ肵�Ȃ��Ƃ��A�߂�l�� TRUE �ƂȂ�܂�
	              // ��O: OCX �v���p�e�B �y�[�W�̖߂�l�� FALSE �ƂȂ�܂�
}

void CAddinDlg::OnItemChangedAddinList(NMHDR* pNMHDR, LRESULT* pResult) 
{
#ifdef _DEBUG
	CMagaDbg	dbg("OnItemChangedExeList()");
#endif
	LPNMLISTVIEW pNMListView = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	if ( pNMListView->uNewState & LVIS_SELECTED ) {
#ifdef _DEBUG
		dbg.printf("LVIS_SELECTED id=%d", pNMListView->iItem);
#endif
		SetDetailData( (CNCVCaddinIF *)(pNMListView->lParam) );
	}
	*pResult = 0;
}
