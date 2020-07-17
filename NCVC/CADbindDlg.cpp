// CCADbindDlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "DXFDoc.h"
#include "CADbindDlg.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

BEGIN_MESSAGE_MAP(CCADbindDlg, CDialog)
	ON_WM_DROPFILES()
	ON_BN_CLICKED(IDC_BIND_ADD, &CCADbindDlg::OnBindFileAdd)
	ON_BN_CLICKED(IDC_BIND_CLR, &CCADbindDlg::OnBindFileClr)
	ON_BN_CLICKED(IDC_BIND_ALLCLR, &CCADbindDlg::OnBindFileAllClr)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCADbindDlg ダイアログ

CCADbindDlg::CCADbindDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CCADbindDlg::IDD, pParent)
{
	const CDXFOption* pOpt = AfxGetNCVCApp()->GetDXFOption();
	m_nOrgType	= pOpt->m_nBindOrg;
}

void CCADbindDlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BIND_LIST, m_ctBindList);
	DDX_Control(pDX, IDC_BIND_WORK_X, m_dBindWork[NCA_X]);
	DDX_Control(pDX, IDC_BIND_WORK_Y, m_dBindWork[NCA_Y]);
	DDX_Control(pDX, IDC_BIND_MARGIN, m_dBindMargin);
	DDX_CBIndex(pDX, IDC_BIND_ORGTYPE, m_nOrgType);
}

void CCADbindDlg::PathOptimizeAdd(const CString& strPath)
{
	// 重複ﾁｪｯｸ
	for ( int i=0; i<m_aryFile.GetCount(); i++ ) {
		if ( strPath.CompareNoCase(m_aryFile[i]) == 0 )
			return;
	}
	// 登録拡張子ﾁｪｯｸ
	CDocument*	dummy;
	if ( AfxGetNCVCApp()->GetDocTemplate(TYPE_DXF)->MatchDocType(strPath, dummy) != CDocTemplate::yesAttemptNative )
		return;

	m_aryFile.Add(strPath);
	// 一旦ｽﾀﾃｨｯｸｺﾝﾄﾛｰﾙにｾｯﾄしてﾊﾟｽ表示の最適化(shlwapi.h)
	::PathSetDlgItemPath(m_hWnd, IDC_BIND_LIST_ST, strPath);
	// そのﾃｷｽﾄを取得してﾘｽﾄｺﾝﾄﾛｰﾙにｾｯﾄ
	CString	strFile;
	GetDlgItem(IDC_BIND_LIST_ST)->GetWindowText(strFile);
	m_ctBindList.AddString(strFile);
}

/////////////////////////////////////////////////////////////////////////////
// CCADbindDlg メッセージ ハンドラー

BOOL CCADbindDlg::OnInitDialog() 
{
	__super::OnInitDialog();

	const CDXFOption*	pOpt = AfxGetNCVCApp()->GetDXFOption();
	m_dBindWork[NCA_X] = pOpt->m_dBindWork[NCA_X];
	m_dBindWork[NCA_Y] = pOpt->m_dBindWork[NCA_Y];
	m_dBindMargin	   = pOpt->m_dBindMargin;

	// ｱｲﾃﾑ追加(DxfSetupReload.cpp)
	CDXFDoc*	pDoc;
	for ( POSITION pos=AfxGetNCVCApp()->GetDocTemplate(TYPE_DXF)->GetFirstDocPosition(); pos; ) {
		pDoc = (CDXFDoc *)(AfxGetNCVCApp()->GetDocTemplate(TYPE_DXF)->GetNextDoc(pos));
		PathOptimizeAdd(pDoc->GetPathName());
	}

	return TRUE;
}

void CCADbindDlg::OnOK()
{
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

	__super::OnOK();
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
	int		i, nCnt = m_ctBindList.GetSelCount();
	CArray<int, int>	arySelect;
	arySelect.SetSize(nCnt);
	m_ctBindList.GetSelItems(nCnt, arySelect.GetData());
	for ( i=arySelect.GetCount()-1; i>=0; i-- )	 {	// 後ろから削除しないと順番変わる
		m_ctBindList.DeleteString(arySelect[i]);
		m_aryFile.RemoveAt(arySelect[i]);
	}
}

void CCADbindDlg::OnBindFileAllClr()
{
	m_ctBindList.ResetContent();
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
