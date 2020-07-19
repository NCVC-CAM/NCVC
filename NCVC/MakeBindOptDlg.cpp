// MakeBindOptDlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "DXFOption.h"
#include "MakeBindOptDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CMakeBindOptDlg, CDialog)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMakeBindOptDlg ダイアログ

CMakeBindOptDlg::CMakeBindOptDlg(CWnd* pParent /*=NULL*/)
	: CDialog(IDD_MAKEBINDOPT, pParent)
{
	CDXFOption*	pOpt = AfxGetNCVCApp()->GetDXFOption();
	m_nSort = pOpt->m_nBindSort;
	m_bFileComment = pOpt->m_bFileComment;
}

void CMakeBindOptDlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_CBIndex(pDX, IDC_MKBD_SORT, m_nSort);
	DDX_Check(pDX, IDC_MKBD_FILECOMMENT, m_bFileComment);
}

/////////////////////////////////////////////////////////////////////////////
// CMakeBindOptDlg メッセージ ハンドラー

void CMakeBindOptDlg::OnOK()
{
	UpdateData();

	CDXFOption*	pOpt = AfxGetNCVCApp()->GetDXFOption();
	pOpt->m_nBindSort = m_nSort;
	pOpt->m_bFileComment = m_bFileComment;
	pOpt->SaveBindOption();

	EndDialog(IDOK);
}
