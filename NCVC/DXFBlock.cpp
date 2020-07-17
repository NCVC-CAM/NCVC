// DXFBlock.cpp: CDXFBlockData クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "DXFdata.h"
#include "DXFBlock.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

void CDXFBlockData::CopyBlock(const CDXFBlockData* pBlock, LPDXFBLOCK pBlockArgv)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CopyBlock()", DBG_MAGENTA);
	dbg.printf("CopyOrg x=%f y=%f", pBlockArgv->ptOrg.x, pBlockArgv->ptOrg.y);
	if ( pBlockArgv->dwBlockFlg & DXFBLFLG_X )
		dbg.printf(" Xmagni=%f", pBlockArgv->dMagni[NCA_X]);
	if ( pBlockArgv->dwBlockFlg & DXFBLFLG_Y )
		dbg.printf(" Ymagni=%f", pBlockArgv->dMagni[NCA_Y]);
	if ( pBlockArgv->dwBlockFlg & DXFBLFLG_R )
		dbg.printf(" Round =%f", pBlockArgv->dRound);
	dbg.printf("CopyCnt  =%d", pBlock->GetSize());
#endif
	CDXFdata*	pData;
	DXFEARGV	dxfEllipse;

	for ( int i=0; i<pBlock->GetSize(); i++ ) {
		pData = pBlock->GetBlockData(i);
		switch ( pData->GetType() ) {
		case DXFPOINTDATA:
			AddData(static_cast<CDXFpoint*>(pData), pBlockArgv);
			break;

		case DXFLINEDATA:
			AddData(static_cast<CDXFline*>(pData), pBlockArgv);
			break;

		case DXFCIRCLEDATA:
			// 各軸独自の拡大率は CDXFcircle -> CDXFellipse
			if ( pBlockArgv->dMagni[NCA_X] != pBlockArgv->dMagni[NCA_Y] ) {
				(static_cast<CDXFcircle*>(pData))->SetEllipseArgv(pBlockArgv, &dxfEllipse);
				AddData(&dxfEllipse);
			}
			else
				AddData(static_cast<CDXFcircle*>(pData), pBlockArgv);
			break;

		case DXFARCDATA:
			// 各軸独自の拡大率は CDXFarc -> CDXFellipse
			if ( pBlockArgv->dMagni[NCA_X] != pBlockArgv->dMagni[NCA_Y] ) {
				(static_cast<CDXFarc*>(pData))->SetEllipseArgv(pBlockArgv, &dxfEllipse);
				AddData(&dxfEllipse);
			}
			else
				AddData(static_cast<CDXFarc*>(pData), pBlockArgv);
			break;

		case DXFELLIPSEDATA:
			AddData(static_cast<CDXFellipse*>(pData), pBlockArgv);
			break;

		case DXFPOLYDATA:
			AddData(static_cast<CDXFpolyline*>(pData), pBlockArgv);
			break;

		case DXFTEXTDATA:
			AddData(static_cast<CDXFtext*>(pData), pBlockArgv);
			break;
		}
	}
}
