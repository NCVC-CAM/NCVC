/////////////////////////////////////////////////////////////////////////////
// ｴｸｽﾎﾟｰﾄ関数(DXFﾃﾞｰﾀ)

#include "stdafx.h"
#include "NCVC.h"
#include "NCVCaddin.h"
#include "NCVCaddinIF.h"
#include "DXFdata.h"
#include "Layer.h"
#include "DXFDoc.h"
#include "DXFOption.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
extern	CMagaDbg	g_dbg;
#endif

NCEXPORT int WINAPI NCVC_GetDXFDataSize(NCVCHANDLE hDoc)
{
	if ( !IsDXFDocument(hDoc) )
		return -1;
	return ((CDXFDoc *)hDoc)->GetDxfSize();
}

NCEXPORT int WINAPI NCVC_GetDXFTextDataSize(NCVCHANDLE hDoc)
{
	if ( !IsDXFDocument(hDoc) )
		return -1;
	return ((CDXFDoc *)hDoc)->GetDxfTextSize();
}

NCEXPORT int WINAPI NCVC_GetDXFStartSize(NCVCHANDLE hDoc)
{
	if ( !IsDXFDocument(hDoc) )
		return -1;
	return ((CDXFDoc *)hDoc)->GetDxfStartSize();
}

NCEXPORT int WINAPI NCVC_GetDXFStartTextSize(NCVCHANDLE hDoc)
{
	if ( !IsDXFDocument(hDoc) )
		return -1;
	return ((CDXFDoc *)hDoc)->GetDxfStartTextSize();
}

NCEXPORT int WINAPI NCVC_GetDXFMoveSize(NCVCHANDLE hDoc)
{
	if ( !IsDXFDocument(hDoc) )
		return -1;
	return ((CDXFDoc *)hDoc)->GetDxfMoveSize();
}

NCEXPORT int WINAPI NCVC_GetDXFMoveTextSize(NCVCHANDLE hDoc)
{
	if ( !IsDXFDocument(hDoc) )
		return -1;
	return ((CDXFDoc *)hDoc)->GetDxfMoveTextSize();
}

NCEXPORT int WINAPI NCVC_GetDXFCommentSize(NCVCHANDLE hDoc)
{
	if ( !IsDXFDocument(hDoc) )
		return -1;
	return ((CDXFDoc *)hDoc)->GetDxfCommentSize();
}

static void SetStrLayer(const CLayerData* pLayer, LPTSTR lpszLayer)
{
	CString	strLayer( pLayer->GetStrLayer() );
	if ( strLayer.IsEmpty() )
		lpszLayer[0] = '\0';
	else
		lstrcpyn(lpszLayer, strLayer,
				min(64/*sizeof(lpszLayer)*/, strLayer.GetLength()+1));
}

NCEXPORT BOOL WINAPI NCVC_GetDXFData(NCVCHANDLE hDoc, int nIndex, LPDXFDATA pDataSrc)
{
	if ( !IsDXFDocument(hDoc) || !pDataSrc || pDataSrc->dwSize != sizeof(DXFDATA) )
		return FALSE;
	CDXFDoc* pDoc = (CDXFDoc *)hDoc;
	if ( nIndex < 0 || nIndex >= pDoc->GetDxfSize() )
		return FALSE;

	ZeroMemory(pDataSrc, sizeof(DXFDATA));

	CDXFdata*	pData = pDoc->GetDxfData(nIndex); 
	pDataSrc->enType = pData->GetType();
	SetStrLayer(pData->GetLayerData(), pDataSrc->szLayer);

	CPointD			pt;
	CDXFpoint*		pPoint;
	CDXFline*		pLine;
	CDXFcircle*		pCircle;
	CDXFarc*		pArc;
	CDXFellipse*	pEllipse;

	switch ( pDataSrc->enType ) {
	case DXFPOINTDATA:
		pPoint = (CDXFpoint *)pData;
		pt = pPoint->GetNativePoint(0);
		pDataSrc->ptS.x = pt.x;		pDataSrc->ptS.y = pt.y;
		break;
	case DXFLINEDATA:
		pLine = (CDXFline *)pData;
		pt = pLine->GetNativePoint(0);
		pDataSrc->ptS.x = pt.x;		pDataSrc->ptS.y = pt.y;
		pt = pLine->GetNativePoint(1);
		pDataSrc->de.ptE.x = pt.x;	pDataSrc->de.ptE.y = pt.y;
		break;
	case DXFCIRCLEDATA:
		pCircle = (CDXFcircle *)pData;
		pt = pCircle->GetCenter();
		pDataSrc->ptS.x = pt.x;		pDataSrc->ptS.y = pt.y;
		pDataSrc->de.dR = pCircle->GetR();
		break;
	case DXFARCDATA:
		pArc = (CDXFarc *)pData;
		pt = pArc->GetCenter();
		pDataSrc->ptS.x = pt.x;		pDataSrc->ptS.y = pt.y;
		pDataSrc->de.arc.r = pArc->GetR();
		pDataSrc->de.arc.sq = pArc->GetStartAngle();
		pDataSrc->de.arc.eq = pArc->GetEndAngle();
		break;
	case DXFELLIPSEDATA:
		pEllipse = (CDXFellipse *)pData;
		pt = pEllipse->GetCenter();
		pDataSrc->ptS.x = pt.x;			pDataSrc->ptS.y = pt.y;
		pt = pEllipse->GetLongPoint();
		pDataSrc->de.elli.ptL.x = pt.x;	pDataSrc->de.elli.ptL.y = pt.y;
		pDataSrc->de.elli.s  = pEllipse->GetShortMagni();
		pDataSrc->de.elli.sq = pEllipse->GetStartAngle();
		pDataSrc->de.elli.eq = pEllipse->GetEndAngle();
		break;
	default:	// DXFPOLYLINE は，ﾃﾞｰﾀの受け渡し方法を考え中
		return FALSE;
	}
	return TRUE;
}

NCEXPORT BOOL WINAPI NCVC_GetDXFStartData(NCVCHANDLE hDoc, int nIndex, LPDXFDATA pDataSrc)
{
	if ( !IsDXFDocument(hDoc) || !pDataSrc || pDataSrc->dwSize != sizeof(DXFDATA) )
		return FALSE;
	CDXFDoc* pDoc = (CDXFDoc *)hDoc;
	if ( nIndex < 0 || nIndex >= pDoc->GetDxfStartSize() )
		return FALSE;

	ZeroMemory(pDataSrc, sizeof(DXFDATA));

	CDXFdata*	pData = pDoc->GetDxfStartData(nIndex); 
	pDataSrc->enType = pData->GetType();
	SetStrLayer(pData->GetLayerData(), pDataSrc->szLayer);

	CPointD		pt;
	CDXFline*	pLine;

	switch ( pDataSrc->enType ) {
	case DXFLINEDATA:
		pLine = (CDXFline *)pData;
		pt = pLine->GetNativePoint(0);
		pDataSrc->ptS.x = pt.x;		pDataSrc->ptS.y = pt.y;
		pt = pLine->GetNativePoint(1);
		pDataSrc->de.ptE.x = pt.x;	pDataSrc->de.ptE.y = pt.y;
		break;
	default:	// DXFPOLYLINE は，ﾃﾞｰﾀの受け渡し方法を考え中
		return FALSE;
	}
	return TRUE;
}

NCEXPORT BOOL WINAPI NCVC_GetDXFMoveData(NCVCHANDLE hDoc, int nIndex, LPDXFDATA pDataSrc)
{
	if ( !IsDXFDocument(hDoc) || !pDataSrc || pDataSrc->dwSize != sizeof(DXFDATA) )
		return FALSE;
	CDXFDoc* pDoc = (CDXFDoc *)hDoc;
	if ( nIndex < 0 || nIndex >= pDoc->GetDxfMoveSize() )
		return FALSE;

	ZeroMemory(pDataSrc, sizeof(DXFDATA));
	pDataSrc->enType = pDoc->GetDxfMoveData(nIndex)->GetType();
	SetStrLayer(pDoc->GetDxfMoveData(nIndex)->GetLayerData(), pDataSrc->szLayer);

	CPointD		pt;
	CDXFline*	pLine;

	switch ( pDataSrc->enType ) {
	case DXFLINEDATA:
		pLine = (CDXFline *)(pDoc->GetDxfMoveData(nIndex));
		pt = pLine->GetNativePoint(0);
		pDataSrc->ptS.x = pt.x;		pDataSrc->ptS.y = pt.y;
		pt = pLine->GetNativePoint(1);
		pDataSrc->de.ptE.x = pt.x;	pDataSrc->de.ptE.y = pt.y;
		break;
	default:	// DXFPOLYLINE は，ﾃﾞｰﾀの受け渡し方法を考え中
		return FALSE;
	}
	return TRUE;
}

static BOOL SetTextInfo(const CDXFtext* pText, LPDXFDATA pData)
{
	ZeroMemory(pData, sizeof(DXFDATA));
	pData->enType = DXFTEXTDATA;
	SetStrLayer(pText->GetLayerData(), pData->szLayer);
	CPointD		pt( pText->GetNativePoint(0) );
	pData->ptS.x = pt.x;	pData->ptS.y = pt.y;
	lstrcpyn(pData->de.szText, pText->GetStrValue(),
		min(sizeof(pData->de.szText), pText->GetStrValue().GetLength()+1));

	return TRUE;
}

NCEXPORT BOOL WINAPI NCVC_GetDXFTextData(NCVCHANDLE hDoc, int nIndex, LPDXFDATA pData)
{
	if ( !IsDXFDocument(hDoc) || !pData || pData->dwSize != sizeof(DXFDATA) )
		return FALSE;
	CDXFDoc* pDoc = (CDXFDoc *)hDoc;
	if ( nIndex < 0 || nIndex >= pDoc->GetDxfTextSize() )
		return FALSE;

	return SetTextInfo( pDoc->GetDxfTextData(nIndex), pData );
}

NCEXPORT BOOL WINAPI NCVC_GetDXFStartTextData(NCVCHANDLE hDoc, int nIndex, LPDXFDATA pData)
{
	if ( !IsDXFDocument(hDoc) || !pData || pData->dwSize != sizeof(DXFDATA) )
		return FALSE;
	CDXFDoc* pDoc = (CDXFDoc *)hDoc;
	if ( nIndex < 0 || nIndex >= pDoc->GetDxfStartTextSize() )
		return FALSE;

	return SetTextInfo( pDoc->GetDxfStartTextData(nIndex), pData );
}

NCEXPORT BOOL WINAPI NCVC_GetDXFMoveTextData(NCVCHANDLE hDoc, int nIndex, LPDXFDATA pData)
{
	if ( !IsDXFDocument(hDoc) || !pData || pData->dwSize != sizeof(DXFDATA) )
		return FALSE;
	CDXFDoc* pDoc = (CDXFDoc *)hDoc;
	if ( nIndex < 0 || nIndex >= pDoc->GetDxfMoveTextSize() )
		return FALSE;

	return SetTextInfo( pDoc->GetDxfMoveTextData(nIndex), pData );
}

NCEXPORT BOOL WINAPI NCVC_GetDXFCommentData(NCVCHANDLE hDoc, int nIndex, LPDXFDATA pData)
{
	if ( !IsDXFDocument(hDoc) || !pData || pData->dwSize != sizeof(DXFDATA) )
		return FALSE;
	CDXFDoc* pDoc = (CDXFDoc *)hDoc;
	if ( nIndex < 0 || nIndex >= pDoc->GetDxfCommentSize() )
		return FALSE;

	return SetTextInfo( pDoc->GetDxfCommentData(nIndex), pData );
}

static BOOL DXFdataOperation
	(NCVCHANDLE hDoc, LPDXFDATA pData, int nIndex, ENDXFOPERATION enOperation)
{
	if ( !IsDXFDocument(hDoc) || pData->dwSize != sizeof(DXFDATA) )
		return FALSE;
	CDXFDoc*		pDoc = (CDXFDoc *)hDoc;
	DXFPARGV		dxfPoint;
	DXFLARGV		dxfLine;
	DXFCARGV		dxfCircle;
	DXFAARGV		dxfArc;
	DXFEARGV		dxfEllipse;
	DXFTARGV		dxfText;

	switch ( pData->enType ) {
	case DXFPOINTDATA:
		dxfPoint.strLayer	= pData->szLayer;
		dxfPoint.c.x		= pData->ptS.x;
		dxfPoint.c.y		= pData->ptS.y;
		pDoc->DataOperation(&dxfPoint, nIndex, enOperation);
		break;
	case DXFLINEDATA:
		dxfLine.strLayer	= pData->szLayer;
		dxfLine.s.x			= pData->ptS.x;
		dxfLine.s.y			= pData->ptS.y;
		dxfLine.e.x			= pData->de.ptE.x;
		dxfLine.e.y			= pData->de.ptE.y;
		pDoc->DataOperation(&dxfLine, nIndex, enOperation);
		break;
	case DXFCIRCLEDATA:
		dxfCircle.strLayer	= pData->szLayer;
		dxfCircle.c.x		= pData->ptS.x;
		dxfCircle.c.y		= pData->ptS.y;
		dxfCircle.r			= pData->de.dR;
		pDoc->DataOperation(&dxfCircle, nIndex, enOperation);
		break;
	case DXFARCDATA:
		dxfArc.strLayer		= pData->szLayer;
		dxfArc.c.x			= pData->ptS.x;
		dxfArc.c.y			= pData->ptS.y;
		dxfArc.r			= pData->de.arc.r;
		dxfArc.sq			= pData->de.arc.sq;
		dxfArc.eq			= pData->de.arc.eq;
		pDoc->DataOperation(&dxfArc, nIndex, enOperation);
		break;
	case DXFELLIPSEDATA:
		dxfEllipse.strLayer	= pData->szLayer;
		dxfEllipse.c.x		= pData->ptS.x;
		dxfEllipse.c.y		= pData->ptS.y;
		dxfEllipse.l.x		= pData->de.elli.ptL.x;
		dxfEllipse.l.y		= pData->de.elli.ptL.y;
		dxfEllipse.s		= pData->de.elli.s;
		dxfEllipse.sq		= pData->de.elli.sq;
		dxfEllipse.eq		= pData->de.elli.eq;
		dxfEllipse.bRound	= TRUE;		// default
		pDoc->DataOperation(&dxfEllipse, nIndex, enOperation);
		break;
	case DXFTEXTDATA:
		dxfText.strLayer	= pData->szLayer;
		dxfText.c.x			= pData->ptS.x;
		dxfText.c.y			= pData->ptS.y;
		dxfText.strValue	= pData->de.szText;
		pDoc->DataOperation(&dxfText, nIndex, enOperation);
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

static BOOL DXFdataOperationSta
	(NCVCHANDLE hDoc, LPDXFDATA pData, int nIndex, ENDXFOPERATION enOperation)
{
	if ( !IsDXFDocument(hDoc) || pData->dwSize != sizeof(DXFDATA) )
		return FALSE;
	CDXFDoc*		pDoc = (CDXFDoc *)hDoc;
	DXFLARGV		dxfLine;
	DXFTARGV		dxfText;

	switch ( pData->enType ) {
	case DXFLINEDATA:
		dxfLine.strLayer	= pData->szLayer;
		dxfLine.s.x			= pData->ptS.x;
		dxfLine.s.y			= pData->ptS.y;
		dxfLine.e.x			= pData->de.ptE.x;
		dxfLine.e.y			= pData->de.ptE.y;
		pDoc->DataOperation_STR(&dxfLine, nIndex, enOperation);
		break;
	case DXFTEXTDATA:
		dxfText.strLayer	= pData->szLayer;
		dxfText.c.x			= pData->ptS.x;
		dxfText.c.y			= pData->ptS.y;
		dxfText.strValue	= pData->de.szText;
		pDoc->DataOperation_STR(&dxfText, nIndex, enOperation);
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

static BOOL DXFdataOperationMov
	(NCVCHANDLE hDoc, LPDXFDATA pData, int nIndex, ENDXFOPERATION enOperation)
{
	if ( !IsDXFDocument(hDoc) || pData->dwSize != sizeof(DXFDATA) )
		return FALSE;
	CDXFDoc*		pDoc = (CDXFDoc *)hDoc;
	DXFLARGV		dxfLine;
	DXFTARGV		dxfText;

	switch ( pData->enType ) {
	case DXFLINEDATA:
		dxfLine.strLayer	= pData->szLayer;
		dxfLine.s.x			= pData->ptS.x;
		dxfLine.s.y			= pData->ptS.y;
		dxfLine.e.x			= pData->de.ptE.x;
		dxfLine.e.y			= pData->de.ptE.y;
		pDoc->DataOperation_MOV(&dxfLine, nIndex, enOperation);
		break;
	case DXFTEXTDATA:
		dxfText.strLayer	= pData->szLayer;
		dxfText.c.x			= pData->ptS.x;
		dxfText.c.y			= pData->ptS.y;
		dxfText.strValue	= pData->de.szText;
		pDoc->DataOperation_MOV(&dxfText, nIndex, enOperation);
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

static BOOL DXFdataOperationTxt
	(NCVCHANDLE hDoc, LPDXFDATA pData, int nIndex, ENDXFOPERATION enOperation)
{
	if ( !IsDXFDocument(hDoc) || pData->dwSize != sizeof(DXFDATA) )
		return FALSE;
	CDXFDoc*		pDoc = (CDXFDoc *)hDoc;
	DXFTARGV		dxfText;

	dxfText.strLayer = pData->szLayer;
	dxfText.c.x = pData->ptS.x;
	dxfText.c.y = pData->ptS.y;
	dxfText.strValue = pData->de.szText;
	pDoc->DataOperation_COM(&dxfText, nIndex, enOperation);

	return TRUE;
}

NCEXPORT BOOL WINAPI NCVC_AddDXFData(NCVCHANDLE hDoc, LPDXFDATA pData)
{
	return DXFdataOperation(hDoc, pData, -1, DXFADD);
}

NCEXPORT BOOL WINAPI NCVC_InsDXFData(NCVCHANDLE hDoc, int nIndex, LPDXFDATA pData)
{
	return DXFdataOperation(hDoc, pData, nIndex, DXFINS)==-1 ? FALSE : TRUE;
}

NCEXPORT BOOL WINAPI NCVC_ModDXFData(NCVCHANDLE hDoc, int nIndex, LPDXFDATA pData)
{
	return DXFdataOperation(hDoc, pData, nIndex, DXFMOD)==-1 ? FALSE : TRUE;
}

NCEXPORT void WINAPI NCVC_DelDXFData(NCVCHANDLE hDoc, int nIndex, int nCnt)
{
	if ( IsDXFDocument(hDoc) )
		((CDXFDoc *)hDoc)->RemoveAt(nIndex, nCnt);
}

NCEXPORT BOOL WINAPI NCVC_AddDXFTextData(NCVCHANDLE hDoc, LPDXFDATA pData)
{
	return DXFdataOperation(hDoc, pData, -1, DXFADD);
}

NCEXPORT BOOL WINAPI NCVC_InsDXFTextData(NCVCHANDLE hDoc, int nIndex, LPDXFDATA pData)
{
	return DXFdataOperation(hDoc, pData, nIndex, DXFINS)==-1 ? FALSE : TRUE;
}

NCEXPORT BOOL WINAPI NCVC_ModDXFTextData(NCVCHANDLE hDoc, int nIndex, LPDXFDATA pData)
{
	return DXFdataOperation(hDoc, pData, nIndex, DXFMOD)==-1 ? FALSE : TRUE;
}

NCEXPORT void WINAPI NCVC_DelDXFTextData(NCVCHANDLE hDoc, int nIndex, int nCnt)
{
	if ( IsDXFDocument(hDoc) )
		((CDXFDoc *)hDoc)->RemoveAtText(nIndex, nCnt);
}

NCEXPORT BOOL WINAPI NCVC_AddDXFStartData(NCVCHANDLE hDoc, LPDXFDATA pData)
{
	return DXFdataOperationSta(hDoc, pData, -1, DXFADD);
}

NCEXPORT BOOL WINAPI NCVC_InsDXFStartData(NCVCHANDLE hDoc, int nIndex, LPDXFDATA pData)
{
	return DXFdataOperationSta(hDoc, pData, nIndex, DXFINS)==-1 ? FALSE : TRUE;
}

NCEXPORT BOOL WINAPI NCVC_ModDXFStartData(NCVCHANDLE hDoc, int nIndex, LPDXFDATA pData)
{
	return DXFdataOperationSta(hDoc, pData, nIndex, DXFMOD)==-1 ? FALSE : TRUE;
}

NCEXPORT void WINAPI NCVC_DelDXFStartData(NCVCHANDLE hDoc, int nIndex, int nCnt)
{
	if ( IsDXFDocument(hDoc) )
		((CDXFDoc *)hDoc)->RemoveAt_STR(nIndex, nCnt);
}

NCEXPORT BOOL WINAPI NCVC_AddDXFStartTextData(NCVCHANDLE hDoc, LPDXFDATA pData)
{
	return DXFdataOperationSta(hDoc, pData, -1, DXFADD);
}

NCEXPORT BOOL WINAPI NCVC_InsDXFStartTextData(NCVCHANDLE hDoc, int nIndex, LPDXFDATA pData)
{
	return DXFdataOperationSta(hDoc, pData, nIndex, DXFINS)==-1 ? FALSE : TRUE;
}

NCEXPORT BOOL WINAPI NCVC_ModDXFStartTextData(NCVCHANDLE hDoc, int nIndex, LPDXFDATA pData)
{
	return DXFdataOperationSta(hDoc, pData, nIndex, DXFMOD)==-1 ? FALSE : TRUE;
}

NCEXPORT void WINAPI NCVC_DelDXFStartTextData(NCVCHANDLE hDoc, int nIndex, int nCnt)
{
	if ( IsDXFDocument(hDoc) )
		((CDXFDoc *)hDoc)->RemoveAtText_STR(nIndex, nCnt);
}

NCEXPORT BOOL WINAPI NCVC_AddDXFMoveData(NCVCHANDLE hDoc, LPDXFDATA pData)
{
	return DXFdataOperationMov(hDoc, pData, -1, DXFADD);
}

NCEXPORT BOOL WINAPI NCVC_InsDXFMoveData(NCVCHANDLE hDoc, int nIndex, LPDXFDATA pData)
{
	return DXFdataOperationMov(hDoc, pData, nIndex, DXFINS)==-1 ? FALSE : TRUE;
}

NCEXPORT BOOL WINAPI NCVC_ModDXFMoveData(NCVCHANDLE hDoc, int nIndex, LPDXFDATA pData)
{
	return DXFdataOperationMov(hDoc, pData, nIndex, DXFMOD)==-1 ? FALSE : TRUE;
}

NCEXPORT void WINAPI NCVC_DelDXFMoveData(NCVCHANDLE hDoc, int nIndex, int nCnt)
{
	if ( IsDXFDocument(hDoc) )
		((CDXFDoc *)hDoc)->RemoveAt_MOV(nIndex, nCnt);
}

NCEXPORT BOOL WINAPI NCVC_AddDXFMoveTextData(NCVCHANDLE hDoc, LPDXFDATA pData)
{
	return DXFdataOperationMov(hDoc, pData, -1, DXFADD);
}

NCEXPORT BOOL WINAPI NCVC_InsDXFMoveTextData(NCVCHANDLE hDoc, int nIndex, LPDXFDATA pData)
{
	return DXFdataOperationMov(hDoc, pData, nIndex, DXFINS)==-1 ? FALSE : TRUE;
}

NCEXPORT BOOL WINAPI NCVC_ModDXFMoveTextData(NCVCHANDLE hDoc, int nIndex, LPDXFDATA pData)
{
	return DXFdataOperationMov(hDoc, pData, nIndex, DXFMOD)==-1 ? FALSE : TRUE;
}

NCEXPORT void WINAPI NCVC_DelDXFMoveTextData(NCVCHANDLE hDoc, int nIndex, int nCnt)
{
	if ( IsDXFDocument(hDoc) )
		((CDXFDoc *)hDoc)->RemoveAtText_MOV(nIndex, nCnt);
}

NCEXPORT BOOL WINAPI NCVC_AddDXFCommentData(NCVCHANDLE hDoc, LPDXFDATA pData)
{
	return DXFdataOperationTxt(hDoc, pData, -1, DXFADD);
}

NCEXPORT BOOL WINAPI NCVC_InsDXFCommentData(NCVCHANDLE hDoc, int nIndex, LPDXFDATA pData)
{
	return DXFdataOperationTxt(hDoc, pData, nIndex, DXFINS)==-1 ? FALSE : TRUE;
}

NCEXPORT BOOL WINAPI NCVC_ModDXFCommentData(NCVCHANDLE hDoc, int nIndex, LPDXFDATA pData)
{
	return DXFdataOperationTxt(hDoc, pData, nIndex, DXFMOD)==-1 ? FALSE : TRUE;
}

NCEXPORT void WINAPI NCVC_DelDXFCommentData(NCVCHANDLE hDoc, int nIndex, int nCnt)
{
	if ( IsDXFDocument(hDoc) )
		((CDXFDoc *)hDoc)->RemoveAt_COM(nIndex, nCnt);
}

NCEXPORT void WINAPI NCVC_GetDXFCutterOrigin(NCVCHANDLE hDoc, LPDPOINT lpptOrg)
{
	if ( IsDXFDocument(hDoc) ) {
		CPointD	pt( ((CDXFDoc *)hDoc)->GetCutterOrigin() );
		lpptOrg->x = pt.x;
		lpptOrg->y = pt.y;
	}
}

NCEXPORT void WINAPI NCVC_SetDXFCutterOrigin
	(NCVCHANDLE hDoc, LPDPOINT lpptOrg, double dR, BOOL bRedraw)
{
	if ( IsDXFDocument(hDoc) ) {
		CPointD	pt;
		pt.x = lpptOrg->x;	pt.y = lpptOrg->y;
		((CDXFDoc *)hDoc)->SetCutterOrigin(pt, dR, bRedraw);
	}
}

NCEXPORT void WINAPI NCVC_GetDXFStartOrigin(NCVCHANDLE hDoc, LPDPOINT lpptOrg)
{
	if ( IsDXFDocument(hDoc) ) {
		CPointD	pt( ((CDXFDoc *)hDoc)->GetStartOrigin() );
		lpptOrg->x = pt.x;
		lpptOrg->y = pt.y;
	}
}

NCEXPORT void WINAPI NCVC_SetDXFStartOrigin
	(NCVCHANDLE hDoc, LPDPOINT lpptOrg, double dR, BOOL bRedraw)
{
	if ( IsDXFDocument(hDoc) ) {
		CPointD	pt;
		pt.x = lpptOrg->x;	pt.y = lpptOrg->y;
		((CDXFDoc *)hDoc)->SetStartOrigin(pt, dR, bRedraw);
	}
}

NCEXPORT void WINAPI NCVC_SetDXFReady(NCVCHANDLE hDoc, BOOL bReady)
{
	if ( IsDXFDocument(hDoc) )
		((CDXFDoc *)hDoc)->SetReadyFlg(bReady);
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
	pOpt->nRegex	= pSrc->GetDxfFlag(DXFOPT_REGEX);
	pOpt->nMatch	= pSrc->GetDxfFlag(DXFOPT_MATCH);
	pOpt->nAccept	= pSrc->GetDxfFlag(DXFOPT_ACCEPT);
	pOpt->nOrgType	= pSrc->GetDxfFlag(DXFOPT_ORGTYPE);
	pOpt->bView		= pSrc->GetDxfFlag(DXFOPT_VIEW);
	const	CStringList*	pList1 = pSrc->GetInitList();
	const	CStringList*	pList2 = pSrc->GetLayerToInitList();
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
