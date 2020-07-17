// NCWorkDlg2.cpp : 実装ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "NCDoc.h"
#include "NCWorkDlg.h"

#include "MagaDbgMac.h"
#include "NCWorkDlg2.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

BEGIN_MESSAGE_MAP(CNCWorkDlg2, CPropertyPage)
	ON_BN_CLICKED(IDC_WORK_SHOW, &CNCWorkDlg2::OnShow)
	ON_BN_CLICKED(IDC_WORK_HIDE, &CNCWorkDlg2::OnHide)
	ON_BN_CLICKED(IDC_WORK_RECOVER, &CNCWorkDlg2::OnRecover)
	ON_BN_CLICKED(IDC_WORK_COMMENT, &CNCWorkDlg2::OnComment)
	ON_MESSAGE(WM_USERSWITCHDOCUMENT, &CNCWorkDlg2::OnUserSwitchDocument)
END_MESSAGE_MAP()

#define	GetParentSheet()	static_cast<CNCWorkDlg *>(GetParentSheet())

/////////////////////////////////////////////////////////////////////////////
// CNCWorkDlg2 ダイアログ

CNCWorkDlg2::CNCWorkDlg2() : CPropertyPage(CNCWorkDlg2::IDD)
{
//	m_psp.dwFlags &= ~PSP_HASHELP;
}

void CNCWorkDlg2::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_WORK_SHOW, m_ctShow);
	DDX_Control(pDX, IDC_WORK_HIDE, m_ctHide);
	DDX_Control(pDX, IDC_WORK_RECOVER, m_ctRecover);
	DDX_Control(pDX, IDC_WORK_COMMENT, m_ctComment);
	DDX_Control(pDX, IDC_WORK_X,  m_ctWork[0]);
	DDX_Control(pDX, IDC_WORK_Z,  m_ctWork[1]);
	DDX_Control(pDX, IDC_WORK_XS, m_ctWorkLabel[0]);
	DDX_Control(pDX, IDC_WORK_ZS, m_ctWorkLabel[1]);
	for ( int i=0; i<SIZEOF(m_ctOffset); i++ ) {
		DDX_Control(pDX, i+IDC_WORK_XO,  m_ctOffset[i]);
		DDX_Control(pDX, i+IDC_WORK_XOS, m_ctOffsetLabel[i]);
	}
}

void CNCWorkDlg2::EnableButton(BOOL bEnable)
{
	int		i;

	m_ctShow.EnableWindow(bEnable);
	m_ctHide.EnableWindow(bEnable);
	m_ctRecover.EnableWindow(bEnable);
	m_ctComment.EnableWindow(bEnable);

	for ( i=0; i<SIZEOF(m_ctWork); i++ ) {
		m_ctWork[i].EnableWindow(bEnable);
		m_ctWorkLabel[i].EnableWindow(bEnable);
	}
	for ( i=0; i<SIZEOF(m_ctOffset); i++ ) {
		m_ctOffset[i].EnableWindow(bEnable);
		m_ctOffsetLabel[i].EnableWindow(bEnable);
	}
}

void CNCWorkDlg2::SetValue(const CRect3F& rc)
{
	m_ctWork[0] = rc.Width();
	m_ctWork[1] = rc.Depth();
	CPointF	ptc = rc.CenterPoint();
	m_ctOffset[NCA_X] = ptc.x;
	m_ctOffset[NCA_Y] = ptc.y;
	m_ctOffset[NCA_Z] = rc.low;
	UpdateData(FALSE);
}

/////////////////////////////////////////////////////////////////////////////
// CNCWorkDlg2 メッセージ ハンドラー

BOOL CNCWorkDlg2::OnInitDialog() 
{
	__super::OnInitDialog();

	// 表示/非表示ﾎﾞﾀﾝの初期化
	OnUserSwitchDocument(NULL, NULL);

	return TRUE;  // コントロールにフォーカスを設定しないとき、戻り値は TRUE となります
	              // 例外: OCX プロパティ ページの戻り値は FALSE となります
}

BOOL CNCWorkDlg2::PreTranslateMessage(MSG* pMsg)
{
	// CPropertySheetﾓｰﾄﾞﾚｽでESCが効かないので
	// 強制的にﾒｯｾｰｼﾞを捕まえて閉じる
	if ( pMsg->message == WM_KEYDOWN ) {
		switch ( pMsg->wParam ) {
		case VK_ESCAPE:
			GetParentSheet()->PostMessage(WM_CLOSE);
			return TRUE;
		case VK_RETURN:
			OnShow();
			return TRUE;
		}
	}
	return CPropertyPage::PreTranslateMessage(pMsg);
}

void CNCWorkDlg2::OnShow() 
{
	CWaitCursor		wait;

	CNCDoc*	pDoc = GetParentSheet()->GetNCDocument();
	if ( pDoc ) {
		CPoint3F	pt(m_ctOffset[NCA_X], m_ctOffset[NCA_Y], m_ctOffset[NCA_Z]);
		pDoc->SetWorkCylinder(TRUE, m_ctWork[0], m_ctWork[1], pt);
	}
}

void CNCWorkDlg2::OnHide() 
{
	CNCDoc*	pDoc = GetParentSheet()->GetNCDocument();
	if ( pDoc )
		pDoc->SetWorkCylinder(FALSE, 0, 0, CPoint3F());
}

void CNCWorkDlg2::OnComment()
{
	extern	LPCTSTR	g_szNCcomment[];	// "WorkCylinder" from NCDoc.cpp
	CNCDoc*	pDoc = GetParentSheet()->GetNCDocument();
	if ( pDoc ) {
		CString	strComment;
		strComment.Format("(%s=%.3f,%.3f,%.3f,%.3f,%.3f)",
			WORKCYLINDER_S,
			float(m_ctWork[0]), float(m_ctWork[1]),
			float(m_ctOffset[NCA_X]), float(m_ctOffset[NCA_Y]), float(m_ctOffset[NCA_Z]));
		pDoc->SetCommentStr(strComment);
		OnShow();	// 表示の更新
	}
}

void CNCWorkDlg2::OnRecover()
{
	CNCDoc*	pDoc = GetParentSheet()->GetNCDocument();
	if ( pDoc )
		SetValue(pDoc->GetWorkRectOrg());		// Org
}

LRESULT CNCWorkDlg2::OnUserSwitchDocument(WPARAM, LPARAM)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CNCWorkDlg2::OnUserSwitchDocument()\nCalling");
#endif
	CNCDoc*	pDoc = GetParentSheet()->GetNCDocument();
	if ( pDoc ) {
		// 旋盤,ﾜｲﾔ放電加工機ﾓｰﾄﾞは不可
		EnableButton( pDoc->IsDocMill() );
		SetValue(pDoc->GetWorkRect());
	}
	else
		EnableButton(FALSE);

	return 0;
}
