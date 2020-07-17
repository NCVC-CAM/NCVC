// CCADbindDlg.cpp : �����t�@�C��
//

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "CADbindDlg.h"
#include "DXFDoc.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

struct BINDFILEHEADER {
	LPCTSTR		lpszName;
	int			nFormat;
};
static	BINDFILEHEADER	g_BindFileHeader[] = {
	{"��", LVCFMT_RIGHT},
	{"̧�ٖ�", LVCFMT_LEFT}
};

using namespace std;
using namespace boost;

BEGIN_MESSAGE_MAP(CCADbindDlg, CDialog)
	ON_WM_DROPFILES()
	ON_BN_CLICKED(IDC_BIND_ADD, &CCADbindDlg::OnBindFileAdd)
	ON_BN_CLICKED(IDC_BIND_CLR, &CCADbindDlg::OnBindFileClr)
	ON_BN_CLICKED(IDC_BIND_ALLCLR, &CCADbindDlg::OnBindFileAllClr)
	ON_NOTIFY(NM_DBLCLK, IDC_BIND_LIST, &CCADbindDlg::OnNMDblclkBindList)
	ON_NOTIFY(LVN_BEGINLABELEDIT, IDC_BIND_LIST, &CCADbindDlg::OnLvnBeginlabeledit)
	ON_NOTIFY(LVN_ENDLABELEDIT, IDC_BIND_LIST, &CCADbindDlg::OnLvnEndlabeledit)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCADbindDlg �_�C�A���O

CCADbindDlg::CCADbindDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CCADbindDlg::IDD, pParent)
{
	const CDXFOption* pOpt = AfxGetNCVCApp()->GetDXFOption();
	m_nOrgType	= pOpt->m_nBindOrg;
	m_bMarge = FALSE;
}

void CCADbindDlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BIND_LIST, m_ctBindList);
	DDX_Control(pDX, IDC_BIND_WORK_X, m_dBindWork[NCA_X]);
	DDX_Control(pDX, IDC_BIND_WORK_Y, m_dBindWork[NCA_Y]);
	DDX_Control(pDX, IDC_BIND_MARGIN, m_dBindMargin);
	DDX_Control(pDX, IDC_BIND_MARGE, m_ctMarge);
	DDX_Check(pDX, IDC_BIND_MARGE, m_bMarge);
	DDX_CBIndex(pDX, IDC_BIND_ORGTYPE, m_nOrgType);
}

void CCADbindDlg::PathOptimizeAdd(const CString& strPath)
{
	if ( strPath.IsEmpty() )
		return;

	// �d������
	int		i = 0;
	for ( auto& v : m_arBind ) {
		if ( strPath.CompareNoCase(v.strFile) == 0 ) {
			m_ctBindList.SetItemText(i, 0, lexical_cast<string>(++v.num).c_str());
			return;
		}
		i++;
	}
	// �o�^�g���q����
	CDocument*	dummy;
	BINDFILE	bind;
	bind.num = 1;
	i = m_ctBindList.GetItemCount();
	CDocTemplate::Confidence result = AfxGetNCVCApp()->GetDocTemplate(TYPE_DXF)->MatchDocType(strPath, dummy);
	if ( result==CDocTemplate::yesAttemptNative || result==CDocTemplate::yesAlreadyOpen ) {
		bind.strFile = strPath;
		m_arBind.push_back(bind);
		// ��U��è�����۰قɾ�Ă����߽�\���̍œK��(shlwapi.h)
		::PathSetDlgItemPath(m_hWnd, IDC_BIND_LIST_ST, strPath);
		// ����÷�Ă��擾����ؽĺ��۰قɾ��
		CString	strFile;
		GetDlgItem(IDC_BIND_LIST_ST)->GetWindowText(strFile);
		m_ctBindList.InsertItem(i, "1");
		m_ctBindList.SetItemText(i, 1, strFile);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CCADbindDlg ���b�Z�[�W �n���h���[

BOOL CCADbindDlg::OnInitDialog() 
{
	__super::OnInitDialog();

	CMDIChildWnd* pChild = AfxGetNCVCMainWnd()->MDIGetActive();
	CDocument* pDoc = pChild ? pChild->GetActiveDocument() : NULL;
	if ( !pDoc || !pDoc->IsKindOf(RUNTIME_CLASS(CDXFDoc)) )
		m_ctMarge.EnableWindow(FALSE);

	const CDXFOption*	pOpt = AfxGetNCVCApp()->GetDXFOption();
	m_dBindWork[NCA_X] = pOpt->m_dBindWork[NCA_X];
	m_dBindWork[NCA_Y] = pOpt->m_dBindWork[NCA_Y];
	m_dBindMargin	   = pOpt->m_dBindMargin;

	// ͯ�ް��`
	int		i = 0;
	BOOST_FOREACH(auto v, g_BindFileHeader) {
		m_ctBindList.InsertColumn(i++, v.lpszName, v.nFormat);
	}
	// ��
	CRect		rc;
	m_ctBindList.GetClientRect(rc);
	i = m_ctBindList.GetStringWidth(g_BindFileHeader[0].lpszName) + 16;
	m_ctBindList.SetColumnWidth(0, i);
	m_ctBindList.SetColumnWidth(1, rc.Width()-m_ctBindList.GetColumnWidth(0)-8);
	// �P�s�I��
	DWORD	dwStyle = m_ctBindList.GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT;
	m_ctBindList.SetExtendedStyle(dwStyle);

/*	�ӊO�Ƃ����Ƃ������̂Ŕp�~
	// ���݊J���Ă��鱲�т̒ǉ�(DxfSetupReload.cpp)
	for ( POSITION pos=AfxGetNCVCApp()->GetDocTemplate(TYPE_DXF)->GetFirstDocPosition(); pos; ) {
		pDoc = AfxGetNCVCApp()->GetDocTemplate(TYPE_DXF)->GetNextDoc(pos);
		PathOptimizeAdd(pDoc->GetPathName());
	}
*/
	return TRUE;
}

void CCADbindDlg::OnOK()
{
	UpdateData();

	if ( m_dBindWork[NCA_X]<=0 || m_dBindWork[NCA_Y]<=0 ) {
		AfxMessageBox(IDS_ERR_UNDERZERO, MB_OK|MB_ICONEXCLAMATION);
		m_dBindWork[NCA_X].SetFocus();
		m_dBindWork[NCA_X].SetSel(0, -1);
		return;
	}

	CDXFOption*	pOpt = AfxGetNCVCApp()->GetDXFOption();
	pOpt->m_dBindWork[NCA_X] = m_dBindWork[NCA_X];
	pOpt->m_dBindWork[NCA_Y] = m_dBindWork[NCA_Y];
	pOpt->m_dBindMargin		 = m_dBindMargin;
	pOpt->m_nBindOrg = m_nOrgType;

//	__super::OnOK();
	EndDialog(IDOK);
}

void CCADbindDlg::OnBindFileAdd()
{
	CStringArray	aryFile;
	if ( !AfxGetNCVCApp()->DoPromptFileNameEx(aryFile, TYPE_DXF) )
		return;

	for ( int i=0; i<aryFile.GetCount(); i++ )
		PathOptimizeAdd(aryFile[i]);
}

void CCADbindDlg::OnBindFileClr()
{
	// ؽĺ��۰ق��������Ė�������폜
	for ( int n = m_ctBindList.GetItemCount()-1; n>=0; n-- ) {
		if ( m_ctBindList.GetItemState(n, LVIS_SELECTED) ) {
			m_ctBindList.DeleteItem(n);
			m_arBind.erase(m_arBind.begin()+n);
		}
	}
}

void CCADbindDlg::OnBindFileAllClr()
{
	m_ctBindList.DeleteAllItems();
	m_arBind.clear();
}

void CCADbindDlg::OnDropFiles(HDROP hDropInfo)
{
	UINT	i, nLen, nCnt = ::DragQueryFile(hDropInfo, -1, NULL, 0);
	CString	strFile;

	for ( i=0; i<nCnt; i++ ) {
		nLen = ::DragQueryFile(hDropInfo, i, NULL, 0);
		::DragQueryFile(hDropInfo, i, strFile.GetBuffer(nLen+1), nLen+1);
		strFile.ReleaseBuffer();
		PathOptimizeAdd(strFile);
	}
	::DragFinish(hDropInfo);
}

void CCADbindDlg::OnNMDblclkBindList(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	if ( pNMItemActivate->iItem >= 0 )
		m_ctBindEdit = m_ctBindList.EditLabel(pNMItemActivate->iItem);
	*pResult = 0;
}

void CCADbindDlg::OnLvnBeginlabeledit(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
	m_ctBindEdit = m_ctBindList.GetEditControl();
	*pResult = 0;
}

void CCADbindDlg::OnLvnEndlabeledit(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
	CString	strBuf;
	m_ctBindEdit->GetWindowText(strBuf);
	auto	it = m_arBind.begin() + pDispInfo->item.iItem;
	int		n  = atoi(strBuf); 
	(*it).num = n <= 0 ? 1 : n;
	m_ctBindList.SetItemText(pDispInfo->item.iItem, 0, lexical_cast<string>((*it).num).c_str());
	m_ctBindList.Update(pDispInfo->item.iItem);
	*pResult = TRUE;
}
