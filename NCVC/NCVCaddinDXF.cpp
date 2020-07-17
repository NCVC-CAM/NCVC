/////////////////////////////////////////////////////////////////////////////
// ¥∏ΩŒﬂ∞ƒä÷êî(DXF√ﬁ∞¿)

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "NCVCaddin.h"
#include "NCVCaddinIF.h"
#include "DXFdata.h"
#include "Layer.h"
#include "DXFDoc.h"
#include "DXFOption.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

using namespace boost;

NCEXPORT int WINAPI NCVC_GetDXFLayerSize(NCVCHANDLE hDoc)
{
	return IsDXFDocument(hDoc) ?
		(int)(reinterpret_cast<CDXFDoc *>(hDoc)->GetLayerCnt()) : -1;
}

NCEXPORT int WINAPI NCVC_GetDXFLayerData
	(NCVCHANDLE hDoc, int nIndex, LPTSTR lpszLayer, int nSize)
{
	int	nLength = -1;
	if ( IsDXFDocument(hDoc) ) {
		CDXFDoc*	pDoc = reinterpret_cast<CDXFDoc *>(hDoc);
		if ( nIndex>=0 && nIndex<pDoc->GetLayerCnt() ) {
			CString	strLayer( pDoc->GetLayerData(nIndex)->GetLayerName() );
			nLength = strLayer.GetLength();
			if ( lpszLayer ) {
				if ( nLength+1 <= nSize )
					lstrcpy(lpszLayer, strLayer);
				else
					nLength = -1;
			}
		}
	}
	return nLength;
}

NCEXPORT int WINAPI NCVC_GetDXFDataSize(NCVCHANDLE hDoc, LPCTSTR lpszLayer)
{
	CLayerData*	pLayer;
	if ( IsDXFDocument(hDoc) && lpszLayer ) {
		pLayer = reinterpret_cast<CDXFDoc *>(hDoc)->GetLayerData(lpszLayer);
		if ( pLayer )
			return (int)(pLayer->GetDxfSize());
	}
	return -1;
}

NCEXPORT int WINAPI NCVC_GetDXFTextDataSize(NCVCHANDLE hDoc, LPCTSTR lpszLayer)
{
	CLayerData*	pLayer;
	if ( IsDXFDocument(hDoc) && lpszLayer ) {
		pLayer = reinterpret_cast<CDXFDoc *>(hDoc)->GetLayerData(lpszLayer);
		if ( pLayer )
			return (int)(pLayer->GetDxfTextSize());
	}
	return -1;
}

NCEXPORT BOOL WINAPI NCVC_GetDXFData(NCVCHANDLE hDoc, int nIndex, LPDXFDATA pDataSrc)
{
	if ( !IsDXFDocument(hDoc) || !pDataSrc || pDataSrc->dwSize != sizeof(DXFDATA) ||
			nIndex<0 || lstrlen(pDataSrc->szLayer)<=0 )
		return FALSE;
	CDXFDoc*	pDoc = reinterpret_cast<CDXFDoc *>(hDoc);
	CLayerData*	pLayer = pDoc->GetLayerData(pDataSrc->szLayer);
	if ( !pLayer || nIndex >= pLayer->GetDxfSize() )
		return FALSE;

	CDXFdata*	pData = pLayer->GetDxfData(nIndex); 
	pDataSrc->enType = pData->GetType();

	CPointF			pt;
	CDXFcircle*		pCircle;
	CDXFarc*		pArc;
	CDXFellipse*	pEllipse;

	switch ( pDataSrc->enType ) {
	case DXFPOINTDATA:
		pt = pData->GetNativePoint(0);
		pDataSrc->ptS.x = pt.x;		pDataSrc->ptS.y = pt.y;
		break;
	case DXFLINEDATA:
		pt = pData->GetNativePoint(0);
		pDataSrc->ptS.x = pt.x;		pDataSrc->ptS.y = pt.y;
		pt = pData->GetNativePoint(1);
		pDataSrc->de.ptE.x = pt.x;	pDataSrc->de.ptE.y = pt.y;
		break;
	case DXFCIRCLEDATA:
		pCircle = static_cast<CDXFcircle*>(pData);
		pt = pCircle->GetCenter();
		pDataSrc->ptS.x = pt.x;		pDataSrc->ptS.y = pt.y;
		pDataSrc->de.dR = pCircle->GetR();
		break;
	case DXFARCDATA:
		pArc = static_cast<CDXFarc*>(pData);
		pt = pArc->GetCenter();
		pDataSrc->ptS.x = pt.x;		pDataSrc->ptS.y = pt.y;
		pDataSrc->de.arc.r = pArc->GetR();
		pDataSrc->de.arc.sq = pArc->GetStartAngle();
		pDataSrc->de.arc.eq = pArc->GetEndAngle();
		break;
	case DXFELLIPSEDATA:
		pEllipse = static_cast<CDXFellipse*>(pData);
		pt = pEllipse->GetCenter();
		pDataSrc->ptS.x = pt.x;			pDataSrc->ptS.y = pt.y;
		pt = pEllipse->GetLongPoint();
		pDataSrc->de.elli.ptL.x = pt.x;	pDataSrc->de.elli.ptL.y = pt.y;
		pDataSrc->de.elli.s  = pEllipse->GetShortMagni();
		pDataSrc->de.elli.sq = pEllipse->GetStartAngle();
		pDataSrc->de.elli.eq = pEllipse->GetEndAngle();
		break;
	default:	// DXFPOLYLINE ÇÕÅC√ﬁ∞¿ÇÃéÛÇØìnÇµï˚ñ@ÇçlÇ¶íÜ
		return FALSE;
	}
	return TRUE;
}

NCEXPORT BOOL WINAPI NCVC_GetDXFTextData(NCVCHANDLE hDoc, int nIndex, LPDXFDATA pDataSrc)
{
	if ( !IsDXFDocument(hDoc) || !pDataSrc || pDataSrc->dwSize != sizeof(DXFDATA) ||
			nIndex<0 || lstrlen(pDataSrc->szLayer)<=0 )
		return FALSE;
	CDXFDoc* pDoc = reinterpret_cast<CDXFDoc *>(hDoc);
	CLayerData*	pLayer = pDoc->GetLayerData(pDataSrc->szLayer);
	if ( !pLayer || nIndex >= pLayer->GetDxfTextSize() )
		return FALSE;

	CDXFtext*	pData = pLayer->GetDxfTextData(nIndex); 
	pDataSrc->enType = DXFTEXTDATA;		// pData->GetType()
	CPointF		pt( pData->GetNativePoint(0) );
	pDataSrc->ptS.x = pt.x;
	pDataSrc->ptS.y = pt.y;
	int	n = pData->GetStrValue().GetLength() + 1;
	lstrcpyn(pDataSrc->de.szText, pData->GetStrValue(),
		min(sizeof(pDataSrc->de.szText), n));

	return TRUE;
}

static BOOL DXFdataOperation
	(NCVCHANDLE hDoc, LPDXFDATA pArgv, ENDXFOPERATION enOperation, int nIndex)
{
	if ( !IsDXFDocument(hDoc) || pArgv->dwSize != sizeof(DXFDATA) )
		return FALSE;
	CDXFDoc*	pDoc = reinterpret_cast<CDXFDoc *>(hDoc);
	CDXFdata*	pData = NULL;
	DXFPARGV	dxfPoint;
	DXFLARGV	dxfLine;
	DXFCARGV	dxfCircle;
	DXFAARGV	dxfArc;
	DXFEARGV	dxfEllipse;
	DXFTARGV	dxfText;

	switch ( pArgv->enType ) {
	case DXFPOINTDATA:
		dxfPoint.pLayer = pDoc->AddLayerMap(pArgv->szLayer, pArgv->nLayer);
		dxfPoint.c.x	= (float)pArgv->ptS.x;
		dxfPoint.c.y	= (float)pArgv->ptS.y;
		pData = new CDXFpoint(&dxfPoint);
		break;
	case DXFLINEDATA:
		dxfLine.pLayer	= pDoc->AddLayerMap(pArgv->szLayer, pArgv->nLayer);
		dxfLine.s.x		= (float)pArgv->ptS.x;
		dxfLine.s.y		= (float)pArgv->ptS.y;
		dxfLine.e.x		= (float)pArgv->de.ptE.x;
		dxfLine.e.y		= (float)pArgv->de.ptE.y;
		pData = new CDXFline(&dxfLine);
		break;
	case DXFCIRCLEDATA:
		if ( pArgv->nLayer == DXFSTRLAYER ) {
			CPointF	pt((float)pArgv->ptS.x, (float)pArgv->ptS.y);
			pData = new CDXFcircleEx(DXFSTADATA,
				pDoc->AddLayerMap(pArgv->szLayer, pArgv->nLayer),
				pt, (float)pArgv->de.dR);
		}
		else {
			dxfCircle.pLayer= pDoc->AddLayerMap(pArgv->szLayer, pArgv->nLayer);
			dxfCircle.c.x	= (float)pArgv->ptS.x;
			dxfCircle.c.y	= (float)pArgv->ptS.y;
			dxfCircle.r		= (float)pArgv->de.dR;
			pData = new CDXFcircle(&dxfCircle);
		}
		break;
	case DXFARCDATA:
		dxfArc.pLayer	= pDoc->AddLayerMap(pArgv->szLayer, pArgv->nLayer);
		dxfArc.c.x		= (float)pArgv->ptS.x;
		dxfArc.c.y		= (float)pArgv->ptS.y;
		dxfArc.r		= (float)pArgv->de.arc.r;
		dxfArc.sq		= (float)pArgv->de.arc.sq;
		dxfArc.eq		= (float)pArgv->de.arc.eq;
		pData = new CDXFarc(&dxfArc);
		break;
	case DXFELLIPSEDATA:
		dxfEllipse.pLayer = pDoc->AddLayerMap(pArgv->szLayer, pArgv->nLayer);
		dxfEllipse.c.x	= (float)pArgv->ptS.x;
		dxfEllipse.c.y	= (float)pArgv->ptS.y;
		dxfEllipse.l.x	= (float)pArgv->de.elli.ptL.x;
		dxfEllipse.l.y	= (float)pArgv->de.elli.ptL.y;
		dxfEllipse.s	= (float)pArgv->de.elli.s;
		dxfEllipse.sq	= (float)pArgv->de.elli.sq;
		dxfEllipse.eq	= (float)pArgv->de.elli.eq;
		dxfEllipse.bRound = TRUE;		// default
		pData = new CDXFellipse(&dxfEllipse);
		break;
	case DXFTEXTDATA:
		dxfText.pLayer	= pDoc->AddLayerMap(pArgv->szLayer, pArgv->nLayer);
		dxfText.c.x		= (float)pArgv->ptS.x;
		dxfText.c.y		= (float)pArgv->ptS.y;
		dxfText.strValue= pArgv->de.szText;
		pData = new CDXFtext(&dxfText);
		break;
	}
	if ( pData ) {
		pDoc->DataOperation(pData, enOperation, nIndex);
		return TRUE;
	}
	return FALSE;
}

NCEXPORT BOOL WINAPI NCVC_AddDXFData(NCVCHANDLE hDoc, LPDXFDATA pData)
{
	return DXFdataOperation(hDoc, pData, DXFADD, -1);
}

NCEXPORT BOOL WINAPI NCVC_InsDXFData(NCVCHANDLE hDoc, int nIndex, LPDXFDATA pData)
{
	return DXFdataOperation(hDoc, pData, DXFINS, nIndex)==-1 ? FALSE : TRUE;
}

NCEXPORT BOOL WINAPI NCVC_ModDXFData(NCVCHANDLE hDoc, int nIndex, LPDXFDATA pData)
{
	return DXFdataOperation(hDoc, pData, DXFMOD, nIndex)==-1 ? FALSE : TRUE;
}

NCEXPORT void WINAPI NCVC_DelDXFData(NCVCHANDLE hDoc, LPCTSTR lpszLayer, int nIndex, int nCnt)
{
	if ( IsDXFDocument(hDoc) )
		reinterpret_cast<CDXFDoc *>(hDoc)->RemoveAt(lpszLayer, nIndex, nCnt);
}

NCEXPORT void WINAPI NCVC_DelDXFTextData(NCVCHANDLE hDoc, LPCTSTR lpszLayer, int nIndex, int nCnt)
{
	if ( IsDXFDocument(hDoc) )
		reinterpret_cast<CDXFDoc *>(hDoc)->RemoveAtText(lpszLayer, nIndex, nCnt);
}

NCEXPORT BOOL WINAPI NCVC_GetDXFCutterOrigin(NCVCHANDLE hDoc, LPDPOINT lpptOrg)
{
	if ( IsDXFDocument(hDoc) ) {
		optional<CPointF>	ptResult = reinterpret_cast<CDXFDoc *>(hDoc)->GetCutterOrigin();
		if ( ptResult ) {
			CPointF	pt( *ptResult );
			lpptOrg->x = pt.x;
			lpptOrg->y = pt.y;
			return TRUE;
		}
	}
	return FALSE;
}

NCEXPORT void WINAPI NCVC_SetDXFCutterOrigin
	(NCVCHANDLE hDoc, LPDPOINT lpptOrg, double dR, BOOL bRedraw)
{
	if ( IsDXFDocument(hDoc) ) {
		CPointF	pt((float)lpptOrg->x, (float)lpptOrg->y);
		reinterpret_cast<CDXFDoc *>(hDoc)->CreateCutterOrigin(pt, (float)dR, bRedraw);
	}
}

NCEXPORT void WINAPI NCVC_SetDXFLatheLine
	(NCVCHANDLE hDoc, LPDPOINT lppts, LPDPOINT lppte)
{
	if ( IsDXFDocument(hDoc) ) {
		CPointF	pts((float)lppts->x, (float)lppts->y),
				pte((float)lppte->x, (float)lppte->y);
		reinterpret_cast<CDXFDoc *>(hDoc)->CreateLatheLine(pts, pte);
	}
}

NCEXPORT void WINAPI NCVC_SetDXFReady(NCVCHANDLE hDoc, BOOL bReady)
{
	if ( IsDXFDocument(hDoc) )
		reinterpret_cast<CDXFDoc *>(hDoc)->SetDocFlag(DXFDOC_READY, bReady);
}

NCEXPORT BOOL WINAPI NCVC_GetDXFoption(LPDXFOPTION pOpt)
{
	if ( pOpt->dwSize != sizeof(DXFOPTION) )
		return FALSE;

	POSITION	pos;
	int			i;
	const	CDXFOption*	pSrc = AfxGetNCVCApp()->GetDXFOption();

	for ( i=0; i<DXFLAYERSIZE; i++ )
		pOpt->pszLayer[i] = (LPCTSTR)(pSrc->GetReadLayer(i));
	pOpt->nRegex	= 1;	// Ver0.15.00Å`ê≥ãKï\åªÇÃÇ›
	pOpt->nMatch	= 1;
	pOpt->nAccept	= 0;
	pOpt->nOrgType	= pSrc->GetDxfOptNum(DXFOPT_ORGTYPE);
	pOpt->bView		= pSrc->GetDxfOptFlg(DXFOPT_VIEW);
	const	CStringList*	pList1 = pSrc->GetInitList(NCMAKEMILL);
	const	CStringList*	pList2 = pSrc->GetInitList(NCMAKELAYER);
	for( i=0, pos=pList1->GetHeadPosition(); pos && i<DXFMAXINITFILE; i++ )
		pOpt->pszInitList[i] = (LPCTSTR)(pList1->GetNext(pos));
	for( i=0, pos=pList2->GetHeadPosition(); pos && i<DXFMAXINITFILE; i++ )
		pOpt->pszLayerToInitList[i]	= (LPCTSTR)(pList2->GetNext(pos));

	return TRUE;
}

NCEXPORT BOOL WINAPI NCVC_IsOriginLayer(LPCTSTR pszLayer)
{
	return AfxGetNCVCApp()->GetDXFOption()->IsOriginLayer(pszLayer);
}

NCEXPORT BOOL WINAPI NCVC_IsCutterLayer(LPCTSTR pszLayer)
{
	return AfxGetNCVCApp()->GetDXFOption()->IsCutterLayer(pszLayer);
}

NCEXPORT BOOL WINAPI NCVC_IsStartLayer(LPCTSTR pszLayer)
{
	return AfxGetNCVCApp()->GetDXFOption()->IsStartLayer(pszLayer);
}

NCEXPORT BOOL WINAPI NCVC_IsMoveLayer(LPCTSTR pszLayer)
{
	return AfxGetNCVCApp()->GetDXFOption()->IsMoveLayer(pszLayer);
}

NCEXPORT BOOL WINAPI NCVC_IsCommentLayer(LPCTSTR pszLayer)
{
	return AfxGetNCVCApp()->GetDXFOption()->IsCommentLayer(pszLayer);
}
