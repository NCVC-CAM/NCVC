// NCInfoView.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "MachineOption.h"
#include "NCChild.h"
#include "NCDoc.h"
#include "NCViewTab.h"
#include "NCListView.h"
#include "NCInfoView.h"
#include "ViewOption.h"
#include "boost/format.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace boost;
using namespace boost::gregorian;
using namespace boost::posix_time;
using std::string;

extern	LPCTSTR	gg_szDelimiter;	// ":"
extern	LPCTSTR	g_szNdelimiter;	// "XYZUVWIJKRPLDH" from NCDoc.cpp
static	LPCTSTR	g_szSpace = " ";
static	LPCTSTR	g_szNumFormat = "%12d";
static	LPCTSTR	g_szMinFormat = "%2d ";

// ﾛｰｶﾙ共通関数
static	void	CopyNCInfoForClipboard(CView*, CNCDoc*);	// ｸﾘｯﾌﾟﾎﾞｰﾄﾞへのｺﾋﾟｰ

/////////////////////////////////////////////////////////////////////////////
// CNCInfoBase

IMPLEMENT_DYNAMIC(CNCInfoBase, CView)

BEGIN_MESSAGE_MAP(CNCInfoBase, CView)
	ON_WM_CONTEXTMENU()
	ON_WM_KEYDOWN()
	ON_WM_ERASEBKGND()
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, &CNCInfoBase::OnUpdateEditCopy)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_UP,  ID_VIEW_RT,  &CNCInfoBase::OnUpdateMoveRoundKey)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_RUP, ID_VIEW_RRT, &CNCInfoBase::OnUpdateMoveRoundKey)
	ON_COMMAND(ID_EDIT_COPY, &CNCInfoBase::OnEditCopy)
END_MESSAGE_MAP()

BOOL CNCInfoBase::PreCreateWindow(CREATESTRUCT& cs)
{
	// ｽﾌﾟﾘｯﾀからの境界ｶｰｿﾙが移るので，IDC_IBEAM を明示的に指定
	cs.lpszClass = AfxRegisterWndClass(
			CS_HREDRAW|CS_VREDRAW,
			AfxGetApp()->LoadStandardCursor(IDC_IBEAM) );
	return CView::PreCreateWindow(cs);
}

BOOL CNCInfoBase::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) 
{
	// ここから CDocument::OnCmdMsg() を呼ばないようにする
//	return CView::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
	return CWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

void CNCInfoBase::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	switch ( lHint ) {
	case UAV_STARTSTOPTRACE:
	case UAV_TRACECURSOR:
	case UAV_DRAWWORKRECT:
	case UAV_DRAWMAXRECT:
		return;		// 再描画不要
	case UAV_CHANGEFONT:
		{
			CClientDC	dc(this);
			dc.SelectObject(AfxGetNCVCMainWnd()->GetTextFont(TYPE_NCD));
		}
		break;
	}
	CView::OnUpdate(pSender, lHint, pHint);
}

#ifdef _DEBUG
CNCDoc* CNCInfoBase::GetDocument()
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CNCDoc)));
	return static_cast<CNCDoc *>(m_pDocument);
}
#endif

/////////////////////////////////////////////////////////////////////////////
// CNCInfoBase メッセージ ハンドラ

void CNCInfoBase::OnUpdateEditCopy(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(!GetDocument()->IsDocFlag(NCDOC_CUTCALC));
}

void CNCInfoBase::OnEditCopy() 
{
	CopyNCInfoForClipboard(this, GetDocument());
}

void CNCInfoBase::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	CViewBase::_OnContextMenu(point, IDR_NCPOPUP3);
}

void CNCInfoBase::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if ( nChar == VK_TAB ) {
		CNCChild*	pFrame = static_cast<CNCChild *>(GetParentFrame());
		if ( ::GetKeyState(VK_SHIFT) < 0 ) {
			pFrame->GetListView()->SetFocus();
		}
		else {
			CNCViewTab* pParent = pFrame->GetMainView();
			pParent->GetActivePageWnd()->SetFocus();
		}
	}

	CView::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CNCInfoBase::OnUpdateMoveRoundKey(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(FALSE);
}

BOOL CNCInfoBase::OnEraseBkgnd(CDC* pDC) 
{
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	COLORREF	col1 = pOpt->GetNcInfoDrawColor(NCINFOCOL_BACKGROUND1),
				col2 = pOpt->GetNcInfoDrawColor(NCINFOCOL_BACKGROUND2);
	return CViewBase::_OnEraseBkgnd(pDC, col1, col2);
}

/////////////////////////////////////////////////////////////////////////////
// CNCInfoView1

IMPLEMENT_DYNCREATE(CNCInfoView1, CNCInfoBase)

CNCInfoView1::CNCInfoView1()
{
}

BEGIN_MESSAGE_MAP(CNCInfoView1, CNCInfoBase)
	ON_MESSAGE (WM_USERPROGRESSPOS, &CNCInfoView1::OnUserCalcMsg)
END_MESSAGE_MAP()

LRESULT CNCInfoView1::OnUserCalcMsg(WPARAM, LPARAM)
{
	Invalidate();
	// ｽﾚｯﾄﾞの終了処理
	GetDocument()->WaitCalcThread();

	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CNCInfoView2

IMPLEMENT_DYNCREATE(CNCInfoView2, CNCInfoBase)

CNCInfoView2::CNCInfoView2()
{
}

BEGIN_MESSAGE_MAP(CNCInfoView2, CNCInfoBase)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNCInfoView 描画

void CNCInfoView1::OnDraw(CDC* pDC)
{
	ASSERT_VALID(GetDocument());
	int			nHeight = AfxGetNCVCMainWnd()->GetNCTextHeight(),
				nWidth  = AfxGetNCVCMainWnd()->GetNCTextWidth(),
				i;
	CString		strBuf, strFormat;
	CRect		rc;

	const	int		X  =  9 * nWidth;
	const	int		W  = 26 * nWidth;	// %12.3f
	const	int		XX = 27 * nWidth;

	CFont*	pFontOld = pDC->SelectObject(AfxGetNCVCMainWnd()->GetTextFont(TYPE_NCD));
	pDC->SetTextColor( AfxGetNCVCApp()->GetViewOption()->GetNcInfoDrawColor(NCINFOCOL_TEXT) );
	pDC->SetBkMode(TRANSPARENT);

	// 加工情報
	strFormat = g_szSpace;	strFormat += gg_szDelimiter;
	for ( i=0; i<3; i++ ) {
		VERIFY(strBuf.LoadString(i+IDCV_G0LENGTH));
		pDC->TextOut(0, i*nHeight, strBuf+strFormat);
		VERIFY(strBuf.LoadString(IDCV_MILI));
		pDC->TextOut(XX, i*nHeight, strBuf);
	}
	VERIFY(strBuf.LoadString(IDCV_CUTTIME));
	pDC->TextOut(0, (i+1)*nHeight, strBuf+strFormat);
	// ---
	if ( GetDocument()->IsDocFlag(NCDOC_ERROR) )
		strFormat.Empty();
	else if ( GetDocument()->IsDocFlag(NCDOC_CUTCALC) )
		VERIFY(strFormat.LoadString(IDCV_CUTCALC));
	else {
		float	dMove = 0.0f, dTime = GetDocument()->GetCutTime();
		for ( i=0; i<2; i++ ) {
			strFormat.Format(GetDocument()->GetDecimalID(), GetDocument()->GetMoveData(i));
			rc.SetRect(X, i*nHeight, W, (i+1)*nHeight);
			pDC->DrawText(strFormat, &rc, DT_SINGLELINE|DT_VCENTER|DT_RIGHT);
			dMove += GetDocument()->GetMoveData(i);
		}
		strFormat.Format(GetDocument()->GetDecimalID(), dMove);
		rc.SetRect(X, i*nHeight, W, (i+1)*nHeight);
		pDC->DrawText(strFormat, &rc, DT_SINGLELINE|DT_VCENTER|DT_RIGHT);
		//
		if ( dTime == -1 ) {		// 初期状態
			VERIFY(strFormat.LoadString(IDCV_FEEDERROR));
		}
		else {
			strFormat = to_simple_string(time_duration(seconds((long)dTime))).c_str();
		}
/*
		else if ( dTime < 1 ) {		// 1分以下
			VERIFY(strBuf.LoadString(IDCV_MINUTE));
			strFormat = "< 1";
			strFormat += g_szSpace + strBuf;
		}
		else if ( dTime > 60 ) {	// 1時間以上
			VERIFY(strBuf.LoadString(IDCV_HOUR));
			strFormat = (lexical_cast<string>((int)(dTime/60)) + g_szSpace).c_str() + strBuf + g_szSpace +
							str(format(g_szMinFormat) % (int)fmod(dTime, 60)).c_str();
			VERIFY(strBuf.LoadString(IDCV_MINUTE));
			strFormat += strBuf;
		}
		else {
			VERIFY(strBuf.LoadString(IDCV_MINUTE));
			strFormat = (str(format(g_szNumFormat) % (int)dTime) + g_szSpace).c_str() + strBuf;
		}
*/
	}
	i = 5;
	rc.SetRect(0, i*nHeight, W, (i+1)*nHeight);
	pDC->DrawText(strFormat, &rc, DT_SINGLELINE|DT_VCENTER|DT_RIGHT);

	pDC->SelectObject(pFontOld);
}

void CNCInfoView2::OnDraw(CDC* pDC)
{
	ASSERT_VALID(GetDocument());
	int			nHeight = AfxGetNCVCMainWnd()->GetNCTextHeight(),
				nWidth  = AfxGetNCVCMainWnd()->GetNCTextWidth(),
				i;
	CString		strBuf, strDelimiter;
	CRect		rc;

	strDelimiter = g_szSpace;	strDelimiter += gg_szDelimiter;
	const	int		W = 10*nWidth;

	CFont*	pFontOld = pDC->SelectObject(AfxGetNCVCMainWnd()->GetTextFont(TYPE_NCD));
	pDC->SetTextColor( AfxGetNCVCApp()->GetViewOption()->GetNcInfoDrawColor(NCINFOCOL_TEXT) );
	pDC->SetBkMode(TRANSPARENT);

	VERIFY(strBuf.LoadString(IDCV_MAXRECT));
	pDC->TextOut(0, 0, strBuf);
	// 矩形情報
	if ( GetDocument()->IsDocFlag(NCDOC_LATHE) ) {
		int		ZX[] = {NCA_Z, NCA_X};
		for ( i=0; i<SIZEOF(ZX); i++ ) {
			strBuf = g_szNdelimiter[ZX[i]] + strDelimiter;
			pDC->TextOut(0, (i+1)*nHeight, strBuf);
			// ---
			VERIFY(strBuf.LoadString(IDCV_KARA));
			pDC->TextOut(6*nWidth+W, (i+1)*nHeight, strBuf);
			// ---
			VERIFY(strBuf.LoadString(IDCV_MILI));
			pDC->TextOut(11*nWidth+W*2, (i+1)*nHeight, strBuf);
		}
		if ( !GetDocument()->IsDocFlag(NCDOC_ERROR) ) {
			float	dResult[2], dHosei[] = {1.0, 2.0};	// X軸直径表示補正
			int		XZ[] = {NCA_X, NCA_Z};
			for ( i=0; i<SIZEOF(XZ); i++ ) {
				GetDocument()->GetWorkRectPP(XZ[i], dResult);
				// ---
				strBuf.Format(GetDocument()->GetDecimalID(), dResult[0] * dHosei[i]);
				rc.SetRect(4*nWidth, (i+1)*nHeight, 5*nWidth+W, (i+2)*nHeight);
				pDC->DrawText(strBuf, &rc, DT_SINGLELINE|DT_VCENTER|DT_RIGHT);
				// ---
				strBuf.Format(GetDocument()->GetDecimalID(), dResult[1] * dHosei[i]);
				rc.SetRect(9*nWidth+W, (i+1)*nHeight, 10*nWidth+W*2, (i+2)*nHeight);
				pDC->DrawText(strBuf, &rc, DT_SINGLELINE|DT_VCENTER|DT_RIGHT);
			}
		}
	}
	else {
		for ( i=0; i<NCXYZ; i++ ) {
			strBuf = g_szNdelimiter[i] + strDelimiter;
			pDC->TextOut(0, (i+1)*nHeight, strBuf);
			// ---
			VERIFY(strBuf.LoadString(IDCV_KARA));
			pDC->TextOut(6*nWidth+W, (i+1)*nHeight, strBuf);
			// ---
			VERIFY(strBuf.LoadString(IDCV_MILI));
			pDC->TextOut(11*nWidth+W*2, (i+1)*nHeight, strBuf);
		}
		if ( !GetDocument()->IsDocFlag(NCDOC_ERROR) ) {
			float	dResult[2];
			for ( i=0; i<NCXYZ; i++ ) {
				GetDocument()->GetWorkRectPP(i, dResult);
				// ---
				strBuf.Format(GetDocument()->GetDecimalID(), dResult[0]);
				rc.SetRect(4*nWidth, (i+1)*nHeight, 5*nWidth+W, (i+2)*nHeight);
				pDC->DrawText(strBuf, &rc, DT_SINGLELINE|DT_VCENTER|DT_RIGHT);
				// ---
				strBuf.Format(GetDocument()->GetDecimalID(), dResult[1]);
				rc.SetRect(9*nWidth+W, (i+1)*nHeight, 10*nWidth+W*2, (i+2)*nHeight);
				pDC->DrawText(strBuf, &rc, DT_SINGLELINE|DT_VCENTER|DT_RIGHT);
			}
		}
	}

	pDC->SelectObject(pFontOld);
}

/////////////////////////////////////////////////////////////////////////////
// 共通関数

void CopyNCInfoForClipboard(CView* pView, CNCDoc* pDoc)
{
	static	LPCTSTR	szBracket[] = {"<<< ", " >>>"};

	CWaitCursor		wait;	// 砂時計ｶｰｿﾙ
	CFileStatus		fStatus;

	const CMachineOption*	pMCopt = AfxGetNCVCApp()->GetMachineOption();
	CString			strFormat, strItem, strBuf, strMM, strDelimiter,
					strSpace(' ', 7);
	CStringArray	strarrayInfo;
	int		i;
	int		ZX[] = {NCA_Z, NCA_X},
			XZ[] = {NCA_X, NCA_Z};
	float	dMove = 0.0f, dTime = pDoc->GetCutTime(),
			dResult[2];

	VERIFY(strBuf.LoadString(IDCV_MILI));
	strMM = g_szSpace + strBuf;
	strDelimiter  = g_szSpace;
	strDelimiter += gg_szDelimiter;
	strDelimiter += g_szSpace;

	// ｸﾘｯﾌﾟﾎﾞｰﾄﾞへのﾃﾞｰﾀ準備
	try {
		// NCﾌｧｲﾙ情報
		VERIFY(strItem.LoadString(IDS_TAB_INFO1));
		strarrayInfo.Add(szBracket[0] + strItem + szBracket[1]);
		Path_Name_From_FullPath(pDoc->GetPathName(), strFormat, strBuf);
		VERIFY(strItem.LoadString(IDCV_FILENAME));
		strarrayInfo.Add(strItem + strDelimiter + strBuf);
		VERIFY(strItem.LoadString(IDCV_FOLDER));
		strarrayInfo.Add(strItem + strDelimiter + strFormat);
		if ( CFile::GetStatus(pDoc->GetPathName(), fStatus) ) {
			VERIFY(strItem.LoadString(IDCV_UPDATE));
			VERIFY(strBuf.LoadString(ID_INDICATOR_DATE_F2));
			VERIFY(strFormat.LoadString(ID_INDICATOR_TIME_F));
			strItem += strDelimiter + fStatus.m_mtime.Format(strBuf) + g_szSpace +
				fStatus.m_mtime.Format(strFormat) + g_szSpace + g_szSpace;
			if ( fStatus.m_size >= 1024*1024 )
				strFormat = (lexical_cast<string>(fStatus.m_size/(1024*1024)) + " MB").c_str();
			else if ( fStatus.m_size >= 1024 )
				strFormat = (lexical_cast<string>(fStatus.m_size/1024) + " KB").c_str();
			else
				strFormat = (lexical_cast<string>(fStatus.m_size) + " bytes").c_str();
			strarrayInfo.Add(strItem + strFormat);
		}
		strItem.Empty();
		strarrayInfo.Add(strItem);
		// 加工情報
		VERIFY(strItem.LoadString(IDS_TAB_INFO2));
		strarrayInfo.Add(szBracket[0] + strItem + szBracket[1]);
		VERIFY(strBuf.LoadString(pDoc->GetDecimalID()));
		for ( i=0; i<2; i++ ) {
			VERIFY(strItem.LoadString(i+IDCV_G0LENGTH));
			strFormat.Format(strBuf, pDoc->GetMoveData(i));
			dMove += pDoc->GetMoveData(i);
			strarrayInfo.Add(strItem + strDelimiter + strFormat + strMM);
		}
		VERIFY(strItem.LoadString(IDCV_SUMLENGTH));
		strFormat.Format(strBuf, dMove);
		strarrayInfo.Add(strItem + strDelimiter + strFormat + strMM);
		VERIFY(strItem.LoadString(IDCV_CUTTIME2));
		if ( dTime == -1 ) {
			VERIFY(strFormat.LoadString(IDCV_FEEDERROR));
		}
		else {
			strFormat = to_simple_string(time_duration(seconds((long)dTime))).c_str();
			i = 12 - strFormat.GetLength();
			strFormat.Insert(0, CString(' ', max(0, i)));
/*
			if ( dTime < 1 ) {
				VERIFY(strBuf.LoadString(IDCV_MINUTE));
				strFormat = "< 1";
				strFormat += g_szSpace + strBuf;
			}
			else if ( dTime > 60 ) {
				VERIFY(strBuf.LoadString(IDCV_HOUR));
				strFormat  = (lexical_cast<string>((int)(dTime/60)) + g_szSpace).c_str() + strBuf + g_szSpace +
								str(format(g_szMinFormat) % (int)fmod(dTime, 60)).c_str();
				VERIFY(strBuf.LoadString(IDCV_MINUTE));
				strFormat += strBuf;
			}
			else {
				VERIFY(strBuf.LoadString(IDCV_MINUTE));
				strFormat = (str(format(g_szNumFormat) % (int)dTime) + g_szSpace).c_str() + strBuf;
			}
			if ( strFormat.GetLength() < 15 ) {	// %12d + " mm"
				i = 15 - strFormat.GetLength();
				strFormat.Insert(0, CString(' ', max(0, i)));
			}
*/
		}
		strarrayInfo.Add(strItem + strDelimiter + strFormat);
		// 機械情報(早送り速度)
		VERIFY(strItem.LoadString(IDCV_G0MOVESPEED));
		strarrayInfo.Add(CString(' ', 2) + strItem);
		VERIFY(strBuf.LoadString(IDCV_MILIpMIN));
		if ( pDoc->IsDocFlag(NCDOC_LATHE) ) {
			for ( i=0; i<SIZEOF(ZX); i++ ) {
				strItem = strSpace + g_szNdelimiter[ZX[i]] + strDelimiter +
					str(format(g_szNumFormat) % pMCopt->GetG0Speed(ZX[i])).c_str() +
					strMM + strBuf;
				strarrayInfo.Add(strItem);
			}
		}
		else {
			for ( i=0; i<NCXYZ; i++ ) {
				strItem = strSpace + g_szNdelimiter[i] + strDelimiter +
					str(format(g_szNumFormat) % pMCopt->GetG0Speed(i)).c_str() +
					strMM + strBuf;
				strarrayInfo.Add(strItem);
			}
		}
		strItem.Empty();
		strarrayInfo.Add(strItem);
		// 矩形情報
		VERIFY(strItem.LoadString(IDS_TAB_INFO3));
		strarrayInfo.Add(szBracket[0] + strItem + szBracket[1]);
		VERIFY(strBuf.LoadString(IDCV_KARA));
		if ( pDoc->IsDocFlag(NCDOC_LATHE) ) {
			float	dHosei[] = {1.0, 2.0};	// X軸直径表示補正
			for ( i=0; i<SIZEOF(XZ); i++ ) {
				pDoc->GetWorkRectPP(XZ[i], dResult);
				strItem = g_szNdelimiter[ZX[i]] + strDelimiter;
				strFormat.Format(pDoc->GetDecimalID(), dResult[0] * dHosei[i]);
				strItem += strFormat + g_szSpace + strBuf + g_szSpace;
				strFormat.Format(pDoc->GetDecimalID(), dResult[1] * dHosei[i]);
				strarrayInfo.Add(strItem + strFormat + strMM);
			}
		}
		else {
			for ( i=0; i<NCXYZ; i++ ) {
				pDoc->GetWorkRectPP(i, dResult);
				strItem = g_szNdelimiter[i] + strDelimiter;
				strFormat.Format(pDoc->GetDecimalID(), dResult[0]);
				strItem += strFormat + g_szSpace + strBuf + g_szSpace;
				strFormat.Format(pDoc->GetDecimalID(), dResult[1]);
				strarrayInfo.Add(strItem + strFormat + strMM);
			}
		}
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		return;
	}

	// ﾒﾓﾘ確保ｻｲｽﾞの計算
	DWORD	dwStrSize = 0;
	for ( i=0; i<strarrayInfo.GetSize(); i++ )
		dwStrSize += strarrayInfo[i].GetLength() + 2;	// CR + LF
	dwStrSize++;	// 終端 NULL

	// ｸﾞﾛｰﾊﾞﾙﾒﾓﾘへの書き込み
	HGLOBAL	hGlobalMem;
	LPTSTR	pGlobalMem;
	if ( !(hGlobalMem=GlobalAlloc(GHND, dwStrSize)) ) {
		strBuf.Format(IDS_ERR_CLIPBOARD, "GlobalAlloc()");
		AfxMessageBox(strBuf, MB_OK|MB_ICONEXCLAMATION);
		return;
	}
	if ( !(pGlobalMem=(LPTSTR)GlobalLock(hGlobalMem)) ) {
		GlobalFree(hGlobalMem);
		strBuf.Format(IDS_ERR_CLIPBOARD, "GlobalLock()");
		AfxMessageBox(strBuf, MB_OK|MB_ICONEXCLAMATION);
		return;
	}
	for ( i=0; i<strarrayInfo.GetSize(); i++ ) {
		lstrcpy(pGlobalMem, strarrayInfo[i]);
		pGlobalMem += strarrayInfo[i].GetLength();
		*pGlobalMem++ = '\x0d';	// CR
		*pGlobalMem++ = '\x0a';	// LF
	}
	GlobalUnlock(hGlobalMem);

	// ｸﾘｯﾌﾟﾎﾞｰﾄﾞへのﾃﾞｰﾀｺﾋﾟｰ
	if ( !pView->OpenClipboard() ) {
		GlobalFree(hGlobalMem);
		AfxMessageBox(IDS_ERR_CLIPBOARD, MB_OK|MB_ICONEXCLAMATION);
		return;
	}
	if ( !EmptyClipboard() ) {
		GlobalFree(hGlobalMem);
		CloseClipboard();
		AfxMessageBox(IDS_ERR_CLIPBOARD, MB_OK|MB_ICONEXCLAMATION);
		return;
	}
	if ( !SetClipboardData(CF_TEXT, hGlobalMem) ) {
		GlobalFree(hGlobalMem);
		AfxMessageBox(IDS_ERR_CLIPBOARD, MB_OK|MB_ICONEXCLAMATION);
	}

	CloseClipboard();
}
