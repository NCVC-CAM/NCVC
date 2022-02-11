// TH_UVWire.cpp
// ワイヤ加工機におけるUVオブジェクトの生成
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "NCVC.h"
#include "NCdata.h"
#include "NCDoc.h"
#include "ThreadDlg.h"
#include "NCVCdefine.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace boost;

#define	IsThread()	pParent->IsThreadContinue()

extern	const	DWORD		g_dwSetValFlags[];	// NCDoc.cpp

static	void		InitialVariable(const CNCdata*, LPNCARGV, float);
static	CNCdata*	CreateUVobj(const CNCdata*, LPNCARGV, const CPoint3F&);
static	BOOL		SetArgvCornerRobject(LPNCARGV, CNCblock*, CNCdata*, CNCdata*, float);

//////////////////////////////////////////////////////////////////////

UINT UVWire_Thread(LPVOID pVoid)
{
#ifdef _DEBUG
	printf("UVWire_Thread() Start\n");
#endif
	LPNCVCTHREADPARAM	pParam = reinterpret_cast<LPNCVCTHREADPARAM>(pVoid);
	CNCDoc*			pDoc = static_cast<CNCDoc*>(pParam->pDoc);
	CThreadDlg*		pParent = pParam->pParent;
	ASSERT(pDoc);
	ASSERT(pParent);

	INT_PTR		i, nLoopCnt = pDoc->GetNCsize();
	int			nGcode,
				nTaper = 0,	// 0:ﾃｰﾊﾟｷｬﾝｾﾙ 1: ﾃｰﾊﾟ指示直後 2:ﾃｰﾊﾟ処理中
				nSign1, nSign2,
				nResult = IDOK;
	BOOL		bResult;
	float		dT,			// ﾜｰｸ厚み
				dTaper,		// T値
				dT1, dT2,	// ﾃｰﾊﾟ角度指示によるｵﾌｾｯﾄ量
				z,			// UV軸のZ値
				k;			// UV軸のｺｰﾅR径
	CPointF		pt;
	CPoint3F	ptOffset;
	optional<CPointF>	ptResult;
	DWORD		dwValFlags;
	CNCblock*	pBlock;
	CNCdata*	pData1;
	CNCdata*	pData2;
	CNCdata*	pDataR;
	CNCdata*	pDataFirst = NULL;
	const CNCread*	pRead1;
	const CNCread*	pRead2;

	NCARGV	ncArgv;		// InitialVariable()初期化

	{
		CString	strMsg;
		VERIFY(strMsg.LoadString(IDS_UVTAPER_NCD));
		pParent->SetFaseMessage(strMsg);
		// ﾜｰｸ厚さ
		CRect3F	rc(pDoc->GetWorkRectOrg());
		z  = rc.low + rc.high;
		dT = fabs(rc.high);		// CNCDoc::DataOperation()
	}
	pParent->m_ctReadProgress.SetRange32(0, (int)nLoopCnt);
#ifdef _DEBUG
	printf("GetNCsize()=%Id Work=%f\n", nLoopCnt, z);
#endif

	// 変数初期化のために、最初の G_TYPE ｵﾌﾞｼﾞｪｸﾄを検索
	for ( i=0; i<nLoopCnt && IsThread(); i++ ) {
		pDataR = pDoc->GetNCdata(i);
		if ( pDataR->GetGtype() == G_TYPE ) {
			InitialVariable(pDataR, &ncArgv, z);
			break;
		}
	}

try {
	pDataFirst = pDataR = new CNCdata(&ncArgv);

	// --- メインループ
	while ( i<nLoopCnt && IsThread() ) {

		// 対象ｵﾌﾞｼﾞｪｸﾄ検索ﾙｰﾌﾟ
		for ( ; i<nLoopCnt && nTaper==0 && IsThread(); i++ ) {
			if ( (i & 0x003f) == 0 )	// 64回おき(下位6ﾋﾞｯﾄﾏｽｸ)
				pParent->m_ctReadProgress.SetPos((int)i);		// ﾌﾟﾛｸﾞﾚｽﾊﾞｰ
			pData1 = pDoc->GetNCdata(i);
			if ( pData1->GetGtype() != G_TYPE )
				continue;
			nGcode = pData1->GetGcode();
			pRead1 = pData1->GetReadData();
			// ﾃｰﾊﾟﾓｰﾄﾞﾁｪｯｸ
			if ( pRead1->m_pTaper ) {
				// ﾃｰﾊﾟ角度は TH_NCRead.cpp でﾁｪｯｸ済み
				if ( pData1->GetType() == NCDLINEDATA ) {
					dTaper = pRead1->m_pTaper->dTaper;	// T値保存
					dT1 = dT2 = dT * tan(fabs(dTaper));
					nSign1 = nSign2 = pRead1->m_pTaper->nTaper;
					if ( dTaper < 0 )
						nSign1 = nSign2 = -nSign1;
					nTaper++;	// 0 -> 1 ﾃｰﾊﾟﾓｰﾄﾞ処理へ(break)
				}
				else {
					// ﾃｰﾊﾟ開始ｵﾌﾞｼﾞｪｸﾄｴﾗｰ
					pBlock = pDoc->GetNCblock(pData1->GetBlockLineNo());
					pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_CORRECTLINE);
				}
			}
			else {
				dwValFlags = pData1->GetValFlags();
				ptOffset   = pData1->GetOffsetPoint();
				// 単純UV指示ﾁｪｯｸ
				ncArgv.nc.nLine  = pData1->GetBlockLineNo();
				ncArgv.nc.nGcode = nGcode;
				ncArgv.nc.dwValFlags &= NCD_CLEARVALUE;
				ncArgv.nc.dwValFlags |= (NCD_X|NCD_Y|NCD_Z);
				ncArgv.nc.dValue[NCA_Z] = z;
				if ( dwValFlags & (NCD_X|NCD_Y|NCD_U|NCD_V) ) {
					pt = pData1->GetEndPoint();
					if ( !(dwValFlags & NCD_R) ) {
						// XY軸終点からの偏差
						if ( dwValFlags & NCD_U )
							pt.x += pData1->GetValue(NCA_U);
						if ( dwValFlags & NCD_V )
							pt.y += pData1->GetValue(NCA_V);
					}
				}
				else {
					// 前回ｵﾌﾞｼﾞｪｸﾄの終点(1周する円弧補間)
					pt = pDataR->GetEndPoint();
				}
				ncArgv.nc.dValue[NCA_X] = pt.x - ptOffset.x;
				ncArgv.nc.dValue[NCA_Y] = pt.y - ptOffset.y;
				if ( pData1->GetType() == NCDARCDATA ) {
					pt = static_cast<const CNCcircle *>(pData1)->GetOrg();
					if ( dwValFlags&(NCD_K|NCD_L) && !(dwValFlags&NCD_R) ) {
						// XY軸中心からの偏差
						if ( dwValFlags & NCD_K )
							pt.x += pData1->GetValue(NCA_K);
						if ( dwValFlags & NCD_L )
							pt.y += pData1->GetValue(NCA_L);
					}
					pt -= pDataR->GetEndPoint();
					ncArgv.nc.dValue[NCA_I] = pt.x;
					ncArgv.nc.dValue[NCA_J] = pt.y;
					ncArgv.nc.dwValFlags |= (NCD_I|NCD_J);
				}
				// UV軸ｵﾌﾞｼﾞｪｸﾄ生成
				pDataR = CreateUVobj(pDataR, &ncArgv, ptOffset);
				pData1->SetWireObj(pDataR);
			}
		} // End of search loop
#ifdef _DEBUG
		if ( i < nLoopCnt )
			printf("TaperMode StartBlock=(%d):%s Taper=%f Sign=%d\n",
				pData1->GetBlockLineNo()+1, 
				LPCTSTR(pDoc->GetNCblock(pData1->GetBlockLineNo())->GetStrGcode()),
				DEG(dTaper), nSign1);
#endif

		// ﾃｰﾊﾟ処理ﾙｰﾌﾟ
		for ( ; i<nLoopCnt && nTaper>0 && IsThread(); i++ ) {
			if ( (i & 0x003f) == 0 )
				pParent->m_ctReadProgress.SetPos((int)i);
			pData2 = pDoc->GetNCdata(i);
			if ( pData2->GetGtype() != G_TYPE )
				continue;
			pRead2 = pData2->GetReadData();
			//
			if ( pRead2->m_pTaper && dTaper!=pRead2->m_pTaper->dTaper ) {
				dTaper = pRead2->m_pTaper->dTaper;
				dT2 = dT * tan(fabs(dTaper));
				if ( dTaper != 0.0f ) {	// T0 の時は符号保持
					nSign2 = pRead2->m_pTaper->nTaper;
					if ( dTaper < 0 )
						nSign2 = -nSign2;
				}
			}
			// 
			ncArgv.nc.nLine  = pData1->GetBlockLineNo();
			ncArgv.nc.nGcode = pData1->GetGcode();
			ncArgv.nc.dwValFlags &= NCD_CLEARVALUE;
			ncArgv.nc.dwValFlags |= NCD_Z;
			ncArgv.nc.dValue[NCA_Z] = z;
			//
			if ( nTaper == 1 ) {
				// ﾃｰﾊﾟ処理開始直後
				ptResult = pData2->CalcPerpendicularPoint(STARTPOINT, dT1, nSign1);
				if ( ptResult ) {
					pt = *ptResult;	// pData2 の始点
					ptOffset = pData1->GetOffsetPoint();
					ncArgv.nc.dValue[NCA_X] = pt.x - ptOffset.x;
					ncArgv.nc.dValue[NCA_Y] = pt.y - ptOffset.y;
					ncArgv.nc.dwValFlags |= (NCD_X|NCD_Y);
					pDataR = CreateUVobj(pDataR, &ncArgv, ptOffset);
					pData1->SetWireObj(pDataR);
				}
				else {
					// 基本的に CalcPerpendicularPoint() でｴﾗｰはないが念のため
					pBlock = pDoc->GetNCblock(pData2->GetBlockLineNo());
					pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_CORRECTOBJECT);
				}
				nTaper++;	// 1 -> 2
			}
			else if ( !pRead2->m_pTaper || (pRead2->m_pTaper && pRead2->m_pTaper->bTonly) ) {
				// 次のﾃﾞｰﾀ(pData2)で終了かTｺｰﾄﾞ単体
				if ( pData2->GetType() == NCDLINEDATA ) {
					ptResult = pData1->CalcPerpendicularPoint(ENDPOINT, dT1, nSign1);
					if ( ptResult ) {
						pt = *ptResult;	// pData1 の終点
						ptOffset = pData1->GetOffsetPoint();
						ncArgv.nc.dwValFlags |= (NCD_X|NCD_Y);
						ncArgv.nc.dValue[NCA_X] = pt.x - ptOffset.x;
						ncArgv.nc.dValue[NCA_Y] = pt.y - ptOffset.y;
						pDataR = CreateUVobj(pDataR, &ncArgv, ptOffset);
						pData1->SetWireObj(pDataR);
					}
					else {
						// 基本的に CalcPerpendicularPoint() でｴﾗｰはないが念のため
						pBlock = pDoc->GetNCblock(pData1->GetBlockLineNo());
						pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_CORRECTOBJECT);
					}
				}
				else {
					// ﾃｰﾊﾟﾓｰﾄﾞ終了時は直線補間
					pBlock = pDoc->GetNCblock(pData2->GetBlockLineNo());
					pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_ENDCIRCLE);
				}
				nTaper = pRead2->m_pTaper && pRead2->m_pTaper->bTonly ? 1 : -1;	// 開始直後かbreak
			}
			else {
				dwValFlags = pData1->GetValFlags();
				if ( dwValFlags&(NCD_U|NCD_V) && !(dwValFlags&NCD_R) ) {
					// ﾃｰﾊﾟﾍﾞｸﾄﾙ指令
					pt = pData1->GetEndPoint();
					if ( dwValFlags & NCD_U )
						pt.x += pData1->GetValue(NCA_U);
					if ( dwValFlags & NCD_V )
						pt.y += pData1->GetValue(NCA_V);
					ptResult = pt;
				}
				else {
					// ﾃｰﾊﾟﾓｰﾄﾞ中は径補正と同じ計算(第４引数注意：次の符号)
					ptResult = pData1->CalcOffsetIntersectionPoint(pData2, dT1, dT2, nSign2>0);
				}
				if ( ptResult ) {
					pt = *ptResult;
					ptOffset = pData1->GetOffsetPoint();
					ncArgv.nc.dwValFlags |= (NCD_X|NCD_Y);
					ncArgv.nc.dValue[NCA_X] = pt.x - ptOffset.x;
					ncArgv.nc.dValue[NCA_Y] = pt.y - ptOffset.y;
					if ( pData1->GetType() == NCDARCDATA ) {
						pt = static_cast<const CNCcircle *>(pData1)->GetOrg();
						if ( dwValFlags&(NCD_K|NCD_L) && !(dwValFlags&NCD_R) ) {
							if ( dwValFlags & NCD_K )
								pt.x += pData1->GetValue(NCA_K);
							if ( dwValFlags & NCD_L )
								pt.y += pData1->GetValue(NCA_L);
						}
						pt -= pDataR->GetEndPoint();
						ncArgv.nc.dValue[NCA_I] = pt.x;
						ncArgv.nc.dValue[NCA_J] = pt.y;
						ncArgv.nc.dwValFlags |= (NCD_I|NCD_J);
					}
					pDataR = CreateUVobj(pDataR, &ncArgv, ptOffset);
					pData1->SetWireObj(pDataR);
				}
				else {
					pBlock = pDoc->GetNCblock(pData1->GetBlockLineNo());
					pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_CORRECTOBJECT);
				}
			}
			// 次のﾙｰﾌﾟヘ
			pData1 = pData2;
			pRead1 = pRead2;
			nSign1 = nSign2;
			dT1    = dT2;
		} // End of taper loop

		if ( nTaper < 0 ) {
			// ﾃｰﾊﾟﾓｰﾄﾞの最終処理
			ncArgv.nc.nLine  = pData2->GetBlockLineNo();
			ncArgv.nc.nGcode = pData2->GetGcode();
			ncArgv.nc.dwValFlags &= NCD_CLEARVALUE;
			ncArgv.nc.dwValFlags |= (pData2->GetValFlags()&(NCD_X|NCD_Y)) | NCD_Z;
			pt = pData2->GetEndPoint();
			ptOffset = pData2->GetOffsetPoint();
			ncArgv.nc.dValue[NCA_X] = pt.x - ptOffset.x;
			ncArgv.nc.dValue[NCA_Y] = pt.y - ptOffset.y;
			ncArgv.nc.dValue[NCA_Z] = z;
			pDataR = CreateUVobj(pDataR, &ncArgv, ptOffset);
			pData2->SetWireObj(pDataR);
			nTaper++;	// -1 -> 0
		}

	} // End of Main(while) loop

	// ------------------------------
	// ｺｰﾅR処理ﾙｰﾌﾟ
	//		G60 独立ｺｰﾅRでK値が指定
	//		G61～G63 円錐ｺｰﾅR
	// ------------------------------
	i = 0;
	pParent->m_ctReadProgress.SetPos((int)i);		// ﾌﾟﾛｸﾞﾚｽﾊﾞｰ

	while ( i<nLoopCnt && IsThread() ) {

		// 対象ｵﾌﾞｼﾞｪｸﾄの検索
		for ( nTaper=0; i<nLoopCnt && nTaper==0 && IsThread(); i++ ) {
			if ( (i & 0x003f) == 0 )
				pParent->m_ctReadProgress.SetPos((int)i);
			pData1 = pDoc->GetNCdata(i);
			dwValFlags = pData1->GetValFlags();
			if ( pData1->IsCutCode() && dwValFlags&NCD_R )
				nTaper++;				// break
		}
		if ( i >= nLoopCnt )	// nTaper == 0
			break;

		pDataR = NULL;
		pData2 = pDoc->GetNCdata(i);
		pRead1 = pData1->GetReadData();
		pRead2 = pData2->GetReadData();
		dT1 = pRead1->m_pTaper ? dT*tan(fabs(pRead1->m_pTaper->dTaper)) : 0.0f;
		dT2 = pRead2->m_pTaper ? dT*tan(fabs(pRead2->m_pTaper->dTaper)) : 0.0f;
#ifdef _DEBUG
		printf("--- CornerR\n");
		printf("pData1 Line=%d\n", pData1->GetBlockLineNo()+1);
		printf("pData2 Line=%d\n", pData2->GetBlockLineNo()+1);
#endif
		ncArgv.nSpindle		= pData1->GetSpindle();
		ncArgv.dFeed		= pData1->GetFeed();
		ncArgv.dEndmill		= pData1->GetEndmill();
		ncArgv.nEndmillType	= pData1->GetEndmillType();
		ncArgv.bG98			= pData1->GetG98();
		ncArgv.nc.nLine		= pData1->GetBlockLineNo();
		ncArgv.nc.nGtype	= G_TYPE;
		ncArgv.nc.enPlane	= pData1->GetPlane();
//		memcpy(&(ncArgv.g68),   &(pData1->GetReadData()->m_g68),   sizeof(G68ROUND));
//		memcpy(&(ncArgv.taper), &(pData1->GetReadData()->m_taper), sizeof(TAPER));
		if ( pData1->GetReadData()->m_pG68 ) {
			ncArgv.g68.bG68		= TRUE;
			ncArgv.g68.enPlane	= pData1->GetReadData()->m_pG68->enPlane;
			ncArgv.g68.dRound	= pData1->GetReadData()->m_pG68->dRound;
			for ( int ii=0; ii<SIZEOF(ncArgv.g68.dOrg); ii++ )
				ncArgv.g68.dOrg[ii] = pData1->GetReadData()->m_pG68->dOrg[ii];
		}
		else {
			ncArgv.g68.bG68 = FALSE;
		}
		if ( pData1->GetReadData()->m_pTaper ) {
			ncArgv.taper.nTaper	= pData1->GetReadData()->m_pTaper->nTaper;
			ncArgv.taper.dTaper	= pData1->GetReadData()->m_pTaper->dTaper;
			ncArgv.taper.nDiff	= pData1->GetReadData()->m_pTaper->nDiff;
			ncArgv.taper.bTonly	= pData1->GetReadData()->m_pTaper->bTonly;
		}
		else {
			ncArgv.taper.nTaper = 0;
		}
		ncArgv.nc.dwValFlags &= NCD_CLEARVALUE;
		ncArgv.nc.dwValFlags |= NCD_Z;
		ncArgv.nc.dValue[NCA_Z] = pData1->GetValue(NCA_Z);

		// --- XY軸のｺｰﾅR 処理
		bResult = SetArgvCornerRobject( &ncArgv,
					pDoc->GetNCblock(pData1->GetBlockLineNo()),
					pData1, pData2, dwValFlags&NCD_R ? pData1->GetValue(NCA_R) : 0.0f );
		if ( bResult ) {
			// XY軸のｺｰﾅRｵﾌﾞｼﾞｪｸﾄを挿入
			pDataR = pDoc->DataOperation(pData1, &ncArgv, i, NCINS);
		}
		if ( !pDataR )
			continue;	// XY軸にｺｰﾅRｵﾌﾞｼﾞｪｸﾄがないとUV軸の処理はしない
#ifdef _DEBUG
		printf("XY corner OK\n");
#endif
		i++;			// ｵﾌﾞｼﾞｪｸﾄを挿入したので、ｶｳﾝﾄ調整
		nLoopCnt++;

		// --- UV軸のｺｰﾅR 処理(pDataRに対してGetValue(NCA_R)は使えない)
		if ( pRead1->m_pTaper ) {
			if ( pRead1->m_pTaper->nDiff == 0 ) {
				// G60
				// ﾃｰﾊﾟﾍﾞｸﾄﾙのKと区別するためにRと組み合わせたときだけ上下独立ｺｰﾅR
				k = dwValFlags&NCD_R ? pData1->GetValue(dwValFlags&NCD_K ? NCA_K : NCA_R) : 0.0f;
			}
			else {
				k = pDataR->GetType() == NCDARCDATA ?
						static_cast<const CNCcircle *>(pDataR)->GetR() : 0.0f;
				switch ( pRead1->m_pTaper->nDiff ) {
				case 1:		// G61
					k += dT1<dT2 ? dT1 : dT2;
					break;
				case 2:		// G62
					k += ( dT1 + dT2 ) / 2.0f;
					break;
				case 3:		// G63
					k += dT1>dT2 ? dT1 : dT2;
					break;
				default:
					k = 0.0f;
				}
			}
		}

		// UV軸のｺｰﾅR ｵﾌﾞｼﾞｪｸﾄ生成
		ncArgv.nc.dwValFlags &= NCD_CLEARVALUE;
		ncArgv.nc.dwValFlags |= NCD_Z;
		ncArgv.nc.dValue[NCA_Z] = z;
		//
		bResult = SetArgvCornerRobject(&ncArgv,
					pDoc->GetNCblock(pData1->GetBlockLineNo()),
					pData1->GetWireObj(), pData2->GetWireObj(), k);
		if ( bResult ) {
			pData2 = CreateUVobj(pData1->GetWireObj(), &ncArgv, ptOffset);
			pData1 = pDataR->GetWireObj();
			if ( pData1 )
				delete	pData1;
			pDataR->SetWireObj(pData2);
#ifdef _DEBUG
			printf("UV corner OK\n");
#endif
		}

	} // End of UV Corner Loop

}
catch (CMemoryException* e) {
	AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
	e->Delete();
	nResult = IDCANCEL;
}

	if ( pDataFirst )
		delete	pDataFirst;

	pParent->m_ctReadProgress.SetPos((int)nLoopCnt);
	pParent->PostMessage(WM_USERFINISH, IsThread() ? nResult : IDCANCEL);
#ifdef _DEBUG
	printf("PostMessage() Finish!\n");
#endif

	return 0;	// AfxEndThread(0);
}

//////////////////////////////////////////////////////////////////////
// 補助関数

BOOL SetArgvCornerRobject
	(LPNCARGV pArgv, CNCblock* pBlock, CNCdata* pData1, CNCdata* pData2, float k)
{
	if ( !pData1 || !pData2 )
		return FALSE;

	// TH_NCRead.cpp MakeChamferingObject()
	CNCdata*	pDataResult = NULL;
	BOOL		bResult;
	float		r1, r2, pa, pb;
	CPointF		pts, pte, pto;
	CPoint3F	ptOffset(pData1->GetOffsetPoint());
	optional<CPointF>	ptResult;

	// 計算開始
	// ｺｰﾅｰRの場合は，面取りに相当するC値の計算
	tie(bResult, pto, r1, r2) = pData1->CalcRoundPoint(pData2, k);
	if ( !bResult ) {
		pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_INTERSECTION);
		return FALSE;
	}
	else if ( r1<NCMIN && r2<NCMIN ) {
		// 補正の必要なし
		return FALSE;
	}

	pto -= ptOffset;

	// pData1(前のｵﾌﾞｼﾞｪｸﾄ)の終点を補正
	if ( r1 < NCMIN )
		pts = pData1->GetPlaneValue(pData1->GetEndPoint());
	else {
		ptResult = pData1->SetChamferingPoint(FALSE, r1);
		if ( !ptResult ) {
			pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_LENGTH);
			return FALSE;
		}
		pts = *ptResult;
	}
	pts -= ptOffset.GetXY();
	// pData2(次のｵﾌﾞｼﾞｪｸﾄ)の始点を補正
	if ( r2 < NCMIN )
		pte = pData2->GetPlaneValue(pData2->GetStartPoint());
	else {
		ptResult = pData2->SetChamferingPoint(TRUE, r2);
		if ( !ptResult ) {
			pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_LENGTH);
			return FALSE;
		}
		pte = *ptResult;
	}
	pte -= ptOffset.GetXY();

	// 座標値のｾｯﾄ
	pArgv->nc.dValue[NCA_X] = pte.x;
	pArgv->nc.dValue[NCA_Y] = pte.y;
	pArgv->nc.dwValFlags = NCD_X|NCD_Y;
	if ( k < NCMIN ) {
		pArgv->nc.nGcode = 1;
	}
	else {
		// 求めたｺｰﾅｰRの中心(pto)から回転方向を計算
		pts -= pto;		pte -= pto;
		if ( (pa=pts.arctan()) < 0.0f )
			pa += PI2;
		if ( (pb=pte.arctan()) < 0.0f )
			pb += PI2;
		if ( fabs(pa-pb) > PI ) {
			if ( pa > pb )
				pa -= PI2;
			else
				pb -= PI2;
		}
		pArgv->nc.nGcode = pa > pb ? 2 : 3;
		// IJ指定
		pArgv->nc.dValue[NCA_I] = -pts.x;
		pArgv->nc.dValue[NCA_J] = -pts.y;
		pArgv->nc.dwValFlags |= (NCD_I|NCD_J);
	}

	return TRUE;
}

CNCdata* CreateUVobj(const CNCdata* pData, LPNCARGV pArgv, const CPoint3F& ptOffset)
{
	CNCdata*	pDataResult = NULL;

	switch ( pArgv->nc.nGcode ) {
	case 0:		// 直線
	case 1:
		// ｵﾌﾞｼﾞｪｸﾄ生成
		pDataResult = new CNCline(pData, pArgv, ptOffset);
		break;
	case 2:		// 円弧
	case 3:
		pDataResult = new CNCcircle(pData, pArgv, ptOffset, NCMAKEWIRE);
		break;
	default:
		pDataResult = new CNCdata(pData, pArgv, ptOffset);
	}

	return pDataResult;
}

void InitialVariable(const CNCdata* pData, LPNCARGV pArgv, float z)
{
	ZeroMemory(pArgv, sizeof(NCARGV));

	pArgv->bAbs			= TRUE;
	pArgv->nc.nGtype	= G_TYPE;
	pArgv->nc.nGcode	= pData->GetGcode();
	pArgv->nc.enPlane	= pData->GetPlane();

	CPoint3F	pts(pData->GetStartPoint());
	pArgv->nc.dValue[NCA_X] = pts.x;
	pArgv->nc.dValue[NCA_Y] = pts.y;
	pArgv->nc.dValue[NCA_Z] = z;
}
