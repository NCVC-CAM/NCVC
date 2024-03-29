/////////////////////////////////////////////////////////////////////////////
// ｴｸｽﾎﾟｰﾄ関数(NCﾃﾞｰﾀ)

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "NCDoc.h"
#include "NCChild.h"
#include "NCViewTab.h"
#include "NCVCaddin.h"
#include "NCVCaddinIF.h"
#include "NCdata.h"
#include "NCDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

NCEXPORT int WINAPI NCVC_GetNCBlockDataSize(NCVCHANDLE hDoc)
{
#ifdef _DEBUG
	printf("NCVC_GetNCBlockDataSize()\n");
#endif
	if ( !IsNCDocument(hDoc) )
		return -1;
	return (int)(reinterpret_cast<CNCDoc *>(hDoc)->GetNCBlockSize());
}

NCEXPORT int WINAPI NCVC_GetNCBlockData(NCVCHANDLE hDoc, int nIndex, LPSTR pszBuf, int nMax)
{
#ifdef _DEBUG
	printf("NCVC_GetNCBlockData()\n");
#endif
	if ( !IsNCDocument(hDoc) )
		return -1;
	CNCDoc* pDoc = reinterpret_cast<CNCDoc *>(hDoc);
	if ( nIndex < 0 || nIndex >= pDoc->GetNCBlockSize() || !pszBuf )
		return -1;

	CString	strBuf(pDoc->GetNCblock(nIndex)->GetStrBlock());
	int		nResult = strBuf.GetLength();
	if ( nResult >= nMax )
		return -1;
	lstrcpy(pszBuf, strBuf);
	return nResult;
}

NCEXPORT DWORD WINAPI NCVC_GetNCBlockFlag(NCVCHANDLE hDoc, int nIndex)
{
	if ( !IsNCDocument(hDoc) )
		return 0;
	return reinterpret_cast<CNCDoc *>(hDoc)->GetNCblock(nIndex)->GetBlockFlag();
}

NCEXPORT int WINAPI NCVC_GetNCDataSize(NCVCHANDLE hDoc)
{
	if ( !IsNCDocument(hDoc) )
		return -1;
	return (int)(reinterpret_cast<CNCDoc *>(hDoc)->GetNCsize());
}

NCEXPORT BOOL WINAPI NCVC_GetNCData(NCVCHANDLE hDoc, int nIndex, LPNCDATA pData)
{
	if ( !IsNCDocument(hDoc) || !pData || pData->dwSize != sizeof(NCDATA) )
		return FALSE;
	CNCDoc* pDoc = reinterpret_cast<CNCDoc *>(hDoc);
	if ( nIndex < 0 || nIndex >= pDoc->GetNCsize() )
		return FALSE;

	CNCdata* pNC = pDoc->GetNCdata(nIndex);
	pData->nErrorCode = pNC->GetNCObjErrorCode();
	pData->nLine = pNC->GetBlockLineNo();
	pData->nGtype = pNC->GetGtype();
	pData->nGcode = pNC->GetGcode();
	pData->dwValFlags = pNC->GetValFlags();
	for ( int i=0; i<SIZEOF(pData->dValue); i++ )
		pData->dValue[i] = pNC->GetValue(i);
	pData->dLength = pNC->GetCutLength();
	return TRUE;
}

NCEXPORT BOOL WINAPI NCVC_AddNCStrData(NCVCHANDLE hDoc, LPCTSTR pszBuf)
{
	if ( !IsNCDocument(hDoc) )
		return FALSE;
	reinterpret_cast<CNCDoc *>(hDoc)->StrOperation(pszBuf, -1, NCADD);
	return TRUE;
}

NCEXPORT BOOL WINAPI NCVC_InsNCStrData(NCVCHANDLE hDoc, int nIndex, LPCTSTR pszBuf)
{
	if ( !IsNCDocument(hDoc) )
		return FALSE;
	reinterpret_cast<CNCDoc *>(hDoc)->StrOperation(pszBuf, nIndex, NCINS);
	return TRUE;
}

NCEXPORT BOOL WINAPI NCVC_ModNCStrData(NCVCHANDLE hDoc, int nIndex, LPCTSTR pszBuf)
{
	if ( !IsNCDocument(hDoc) )
		return FALSE;
	reinterpret_cast<CNCDoc *>(hDoc)->StrOperation(pszBuf, nIndex, NCMOD);
	return TRUE;
}

NCEXPORT void WINAPI NCVC_DelNCStrData(NCVCHANDLE hDoc, int nIndex, int nCnt)
{
	if ( IsNCDocument(hDoc) )
		reinterpret_cast<CNCDoc *>(hDoc)->RemoveStr(nIndex, nCnt);
}

static void NCArgvInitialize(LPNCARGV lpArgv)
{
	int		i;
	const CMachineOption* pMCopt  = theApp.GetMachineOption();
	const CViewOption*	  pVopt = theApp.GetViewOption();

	lpArgv->nc.nGtype = G_TYPE;
	lpArgv->nc.nGcode = pMCopt->GetModalSetting(MODALGROUP0);
	switch ( pMCopt->GetModalSetting(MODALGROUP1) ) {
	case 1:
		lpArgv->nc.enPlane = XZ_PLANE;
		break;
	case 2:
		lpArgv->nc.enPlane = YZ_PLANE;
		break;
	default:	// or 「0」
		lpArgv->nc.enPlane = XY_PLANE;
		break;
	}
	lpArgv->nc.dwValFlags = 0;
	for ( i=0; i<NCXYZ; i++ )
		lpArgv->nc.dValue[i] = pMCopt->GetInitialXYZ(i);
	for ( ; i<VALUESIZE; i++ )
		lpArgv->nc.dValue[i] = 0.0;
	lpArgv->bAbs		= pMCopt->GetModalSetting(MODALGROUP3) == 0 ? TRUE : FALSE;
	lpArgv->bG98		= pMCopt->GetModalSetting(MODALGROUP4) == 0 ? TRUE : FALSE;
	lpArgv->nSpindle	= 0;
	lpArgv->dFeed		= pMCopt->GetDbl(MC_DBL_FEED);
	lpArgv->dEndmill	= pVopt->GetDefaultEndmill();
}

static BOOL NCdataOperation
	(NCVCHANDLE hDoc, LPNCARGV lpArgv, int nIndex, ENNCOPERATION enOperation)
{
	if ( !IsNCDocument(hDoc) || lpArgv->dwSize != sizeof(NCARGV) )
		return FALSE;
	CNCDoc*		pDoc = reinterpret_cast<CNCDoc *>(hDoc);
	CNCdata*	pData;
	if ( nIndex <= 0 ) {
		NCARGV	ncArgv;
		NCArgvInitialize(&ncArgv);
		pData = new CNCdata(&ncArgv);
	}
	else {
		pData = pDoc->GetNCdata(nIndex - 1);
	}
	pDoc->DataOperation(pData, lpArgv, nIndex, enOperation);
	if ( nIndex <= 0 )
		delete	pData;
	return TRUE;
}

NCEXPORT BOOL WINAPI NCVC_AddNCData(NCVCHANDLE hDoc, LPNCARGV lpArgv)
{
	return NCdataOperation(hDoc, lpArgv, -1, NCADD);
}

NCEXPORT BOOL WINAPI NCVC_InsNCData(NCVCHANDLE hDoc, int nIndex, LPNCARGV lpArgv)
{
	return NCdataOperation(hDoc, lpArgv, nIndex, NCINS)==-1 ? FALSE : TRUE;
}

NCEXPORT BOOL WINAPI NCVC_ModNCData(NCVCHANDLE hDoc, int nIndex, LPNCARGV lpArgv)
{
	return NCdataOperation(hDoc, lpArgv, nIndex, NCMOD)==-1 ? FALSE : TRUE;
}

NCEXPORT void WINAPI NCVC_DelNCData(NCVCHANDLE hDoc, int nIndex, int nCnt)
{
	if ( IsNCDocument(hDoc) )
		reinterpret_cast<CNCDoc *>(hDoc)->RemoveObj(nIndex, nCnt);
}

NCEXPORT void WINAPI NCVC_TraceStart(NCVCHANDLE hDoc)
{
/*	
	// ↓このアプローチでは AfxGetNCVCMainWnd() で NULL が返るのでダメ
	CMDIChildWnd*	pChild = AfxGetNCVCMainWnd()->MDIGetActive();
	if ( pChild->IsKindOf(RUNTIME_CLASS(CNCChild)) )
		static_cast<CNCChild*>(pChild)->GetMainView()->SendMessage(WM_COMMAND, ID_NCVIEW_TRACE_RUN);
*/
	if ( IsNCDocument(hDoc) ) {
		CNCDoc* pDoc = reinterpret_cast<CNCDoc *>(hDoc);
		for ( POSITION pos=pDoc->GetFirstViewPosition(); pos; ) {
			CView* pView = pDoc->GetNextView(pos);
			if ( pView && pView->IsKindOf(RUNTIME_CLASS(CNCViewTab)) ) {
				pView->SendMessage(WM_COMMAND, ID_NCVIEW_TRACE_RUN);
				break;
			}
		}
	}
}

NCEXPORT void WINAPI NCVC_TracePause(NCVCHANDLE hDoc)
{
	if ( IsNCDocument(hDoc) ) {
		CNCDoc* pDoc = reinterpret_cast<CNCDoc *>(hDoc);
		for ( POSITION pos=pDoc->GetFirstViewPosition(); pos; ) {
			CView* pView = pDoc->GetNextView(pos);
			if ( pView && pView->IsKindOf(RUNTIME_CLASS(CNCViewTab)) ) {
				pView->SendMessage(WM_COMMAND, ID_NCVIEW_TRACE_PAUSE);
				break;
			}
		}
	}
}

NCEXPORT void WINAPI NCVC_TraceStop(NCVCHANDLE hDoc)
{
	if ( IsNCDocument(hDoc) ) {
		CNCDoc* pDoc = reinterpret_cast<CNCDoc *>(hDoc);
		for ( POSITION pos=pDoc->GetFirstViewPosition(); pos; ) {
			CView* pView = pDoc->GetNextView(pos);
			if ( pView && pView->IsKindOf(RUNTIME_CLASS(CNCViewTab)) ) {
				pView->SendMessage(WM_COMMAND, ID_NCVIEW_TRACE_STOP);
				break;
			}
		}
	}
}

NCEXPORT int WINAPI NCVC_TraceNextDraw(NCVCHANDLE hDoc)
{
	int	nResult = -1;
	if ( IsNCDocument(hDoc) ) {
		// intでキャストしないと64bitでwarning
		nResult = (int)(reinterpret_cast<CNCDoc *>(hDoc)->GetTraceDraw());
	}
	return nResult;
}
