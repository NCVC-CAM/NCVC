// DXFBlock.cpp: CDXFBlockData クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "DXFdata.h"
#include "DXFBlock.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

void CDXFBlockData::CopyBlock(const CDXFBlockData* pBlock, LPCDXFBLOCK pBlockArgv)
{
#ifdef _DEBUG
	printf("CopyOrg x=%f y=%f\n", pBlockArgv->ptOrg.x, pBlockArgv->ptOrg.y);
	if ( pBlockArgv->dwBlockFlg & DXFBLFLG_X )
		printf(" Xmagni=%f\n", pBlockArgv->dMagni[NCA_X]);
	if ( pBlockArgv->dwBlockFlg & DXFBLFLG_Y )
		printf(" Ymagni=%f\n", pBlockArgv->dMagni[NCA_Y]);
	if ( pBlockArgv->dwBlockFlg & DXFBLFLG_R )
		printf(" Round =%f\n", pBlockArgv->dRound);
	printf("CopyCnt  =%Id\n", pBlock->GetSize());
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
