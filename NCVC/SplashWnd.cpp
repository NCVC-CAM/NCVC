// SplashWnd.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "resource.h"
#include "SplashWnd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CSplashWnd, CWnd)
	//{{AFX_MSG_MAP(CSplashWnd)
	ON_WM_CREATE()
	ON_WM_PAINT()
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CSplashWnd::ms_bShowSplashWnd = FALSE;

/////////////////////////////////////////////////////////////////////////////
// CSplashWnd

CSplashWnd::CSplashWnd()
{
	if ( m_bitmap.LoadBitmap(IDB_SPLASH) )
		m_bitmap.GetBitmap(&m_bm);
}

CSplashWnd::~CSplashWnd()
{
}

/////////////////////////////////////////////////////////////////////////////
// CSplashWnd メッセージ ハンドラ

BOOL CSplashWnd::PreTranslateMessage(MSG* pMsg) 
{
	// ｷｰﾎﾞｰﾄﾞまたはﾏｳｽﾒｯｾｰｼﾞを取得した場合は，ｽﾌﾟﾗｯｼｭｽｸﾘｰﾝを終了させる
	if ( pMsg->message == WM_KEYDOWN ||
		 pMsg->message == WM_SYSKEYDOWN ||
		 pMsg->message == WM_LBUTTONDOWN ||
		 pMsg->message == WM_RBUTTONDOWN ||
		 pMsg->message == WM_MBUTTONDOWN ||
		 pMsg->message == WM_NCLBUTTONDOWN ||
		 pMsg->message == WM_NCRBUTTONDOWN ||
		 pMsg->message == WM_NCMBUTTONDOWN ) {
		AfxGetMainWnd()->UpdateWindow();
		KillTimer(IDC_SPLASH_TIMER);
		DestroyWindow();
		return TRUE;
	}
	return CWnd::PreTranslateMessage(pMsg);
}

BOOL CSplashWnd::PreCreateWindow(CREATESTRUCT& cs) 
{
	cs.cx = m_bm.bmWidth;
	cs.cy = m_bm.bmHeight;
	return CWnd::PreCreateWindow(cs);
}

int CSplashWnd::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if ( CWnd::OnCreate(lpCreateStruct) < 0 )
		return -1;

	CenterWindow();
	SetTimer(IDC_SPLASH_TIMER, 1500, NULL);

	return 0;
}

void CSplashWnd::OnPaint() 
{
	CPaintDC dc(this);

	CDC dcImage;
	if ( !dcImage.CreateCompatibleDC(&dc) )
		return;
	CBitmap* pOldBitmap = dcImage.SelectObject(&m_bitmap);
	dc.BitBlt(0, 0, m_bm.bmWidth, m_bm.bmHeight, &dcImage, 0, 0, SRCCOPY);
	dcImage.SelectObject(pOldBitmap);
}

void CSplashWnd::OnTimer(UINT nIDEvent) 
{
	AfxGetMainWnd()->UpdateWindow();
	KillTimer(IDC_SPLASH_TIMER);
	DestroyWindow();
}

void CSplashWnd::PostNcDestroy() 
{
	delete this;
}
