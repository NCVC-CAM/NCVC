// NCWorkDlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "NCChild.h"
#include "NCDoc.h"
#include "NCWorkDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CNCWorkDlg, CPropertySheet)
	ON_WM_DESTROY()
	ON_MESSAGE (WM_USERSWITCHDOCUMENT, &CNCWorkDlg::OnUserSwitchDocument)
END_MESSAGE_MAP()

IMPLEMENT_DYNAMIC(CNCWorkDlg, CPropertySheet)

/////////////////////////////////////////////////////////////////////////////
// CNCWorkDlg

CNCWorkDlg::CNCWorkDlg(UINT nIDCaption, UINT iSelectPage)
	: CPropertySheet(nIDCaption, NULL, iSelectPage)
{
//	m_psh.dwFlags &= ~PSH_HASHELP;

	AddPage(&m_dlg1);
	AddPage(&m_dlg2);
}

CNCWorkDlg::~CNCWorkDlg()
{
}

CNCDoc* CNCWorkDlg::GetNCDocument(void)
{
	CNCDoc*	pDocResult = NULL;
	CMDIChildWnd* pFrame = AfxGetNCVCMainWnd()->MDIGetActive();
	if ( pFrame && pFrame->IsKindOf(RUNTIME_CLASS(CNCChild)) ) {
		CNCDoc* pDoc = static_cast<CNCDoc *>(pFrame->GetActiveDocument());
		if ( pDoc && pDoc->IsKindOf(RUNTIME_CLASS(CNCDoc)) )
			pDocResult = pDoc;
	}
	return pDocResult;
}

/////////////////////////////////////////////////////////////////////////////
// CNCWorkDlg メッセージ ハンドラー

BOOL CNCWorkDlg::OnInitDialog()
{
	// ﾓｰﾄﾞﾚｽﾌﾟﾛﾊﾟﾃｨｼｰﾄでOK/Cancelﾎﾞﾀﾝを有効にする
	// 　逆に無い方が良さそう...
//	m_bModeless = FALSE;
//	m_nFlags |= WF_CONTINUEMODAL;
	BOOL	bResult = __super::OnInitDialog();
//	m_bModeless = TRUE;
//	m_nFlags &= ~WF_CONTINUEMODAL;

	// ｳｨﾝﾄﾞｳ位置読み込み
	CPoint	pt;
	if ( AfxGetNCVCApp()->GetDlgWindow(IDS_REGKEY_WINDOW_WORKDLG, &pt) )
		SetWindowPos(NULL, pt.x, pt.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

	return bResult;
}

void CNCWorkDlg::PostNcDestroy()
{
	AfxGetNCVCMainWnd()->SetModelessDlg(MLD_NCWORK, NULL);
	delete	this;
	__super::PostNcDestroy();
}

void CNCWorkDlg::OnDestroy()
{
	// ｳｨﾝﾄﾞｳ位置保存
	AfxGetNCVCApp()->SaveDlgWindow(IDS_REGKEY_WINDOW_WORKDLG, this);

	__super::OnDestroy();
}

LRESULT CNCWorkDlg::OnUserSwitchDocument(WPARAM, LPARAM)
{
#ifdef _DEBUG
	printf("CNCWorkDlg::OnUserSwitchDocument() Calling\n");
#endif
	CNCDoc*	pDoc = GetNCDocument();

	CPropertyPage*	pPage;
	for ( int i=0; i<GetPageCount(); i++ ) {
		pPage = GetPage(i);
		if ( ::IsWindow(pPage->GetSafeHwnd()) )
			pPage->SendMessage(WM_USERSWITCHDOCUMENT);
	}

	return 0;
}
