// TH_UVWire.cpp
// ワイヤ加工機におけるUVオブジェクトの生成
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "NCVC.h"
#include "NCdata.h"
#include "NCDoc.h"
#include "ThreadDlg.h"
#include "NCVCdefine.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

using namespace boost;

#define	IsThread()	pParent->IsThreadContinue()

extern	const	DWORD		g_dwSetValFlags[];	// NCDoc.cpp

static	void	InitialVariable(const CNCdata*, LPNCARGV, double);
static	CNCdata*	CreateUVobj(const CNCdata*, LPNCARGV, const CPoint3D&);

//////////////////////////////////////////////////////////////////////

UINT UVWire_Thread(LPVOID pVoid)
{
#ifdef _DEBUG
	CMagaDbg	dbg("UVWire_Thread()\nStart", DBG_BLUE);
#endif
	LPNCVCTHREADPARAM	pParam = reinterpret_cast<LPNCVCTHREADPARAM>(pVoid);
	CNCDoc*			pDoc = static_cast<CNCDoc*>(pParam->pDoc);
	CThreadDlg*		pParent = pParam->pParent;
	ASSERT(pDoc);
	ASSERT(pParent);

	int			i, nLoopCnt = pDoc->GetNCsize(), nGcode,
				nTaper = 0,	// 0:ﾃｰﾊﾟｷｬﾝｾﾙ 1: ﾃｰﾊﾟ指示直後 2:ﾃｰﾊﾟ処理中
				nResult = IDOK;
	double		dZ,			// ﾜｰｸ厚み==UV軸のZ値
				dTaper,		// T値
				dT;			// ﾃｰﾊﾟ角度指示によるｵﾌｾｯﾄ量
	CPointD		pt;
	optional<CPointD>	ptResult;
	DWORD		dwValFlags;
	CNCblock*	pBlock;
	CNCdata*	pData1;
	CNCdata*	pData2;
	CNCdata*	pDataFirst = NULL;
	CNCdata*	pData;
	const CNCread*	pRead1;
	const CNCread*	pRead2;

	NCARGV	ncArgv;

	{
		CString	strMsg;
		VERIFY(strMsg.LoadString(IDS_UVTAPER_NCD));
		pParent->SetFaseMessage(strMsg);
		// ﾜｰｸ厚さ
		CRect3D	rc(pDoc->GetWorkRectOrg());
		dZ = rc.high - rc.low;
	}
	pParent->m_ctReadProgress.SetRange32(0, nLoopCnt);
#ifdef _DEBUG
	dbg.printf("GetNCsize()=%d Work=%f", nLoopCnt, dZ);
#endif

	// 変数初期化のために、最初の G_TYPE ｵﾌﾞｼﾞｪｸﾄを検索
	for ( i=0; i<nLoopCnt && IsThread(); i++ ) {
		pData = pDoc->GetNCdata(i);
		if ( pData->GetGtype() == G_TYPE ) {
			InitialVariable(pData, &ncArgv, dZ);
			break;
		}
	}

try {
	pDataFirst = pData = new CNCdata(&ncArgv);

	// --- メインループ
	while ( i<nLoopCnt && IsThread() ) {

		// 対象ｵﾌﾞｼﾞｪｸﾄ検索ﾙｰﾌﾟ
		for ( ; i<nLoopCnt && nTaper==0 && IsThread(); i++ ) {
			if ( (i & 0x003f) == 0 )	// 64回おき(下位6ﾋﾞｯﾄﾏｽｸ)
				pParent->m_ctReadProgress.SetPos(i);		// ﾌﾟﾛｸﾞﾚｽﾊﾞｰ
			pData1 = pDoc->GetNCdata(i);
			if ( pData1->GetGtype() != G_TYPE )
				continue;
			nGcode = pData1->GetGcode();
			pRead1 = pData1->GetReadData();
			// ﾃｰﾊﾟﾓｰﾄﾞﾁｪｯｸ
			if ( pRead1->m_taper.nTaper!=0 && pRead1->m_taper.dTaper!=0.0 ) {
				if ( fabs(pRead1->m_taper.dTaper) <= 45.0 ) {
					if ( pData1->GetType() == NCDLINEDATA ) {
						dTaper = pRead1->m_taper.dTaper;	// T値保存
						dT = dZ * tan(dTaper);
						nTaper++;	// 0 -> 1 ﾃｰﾊﾟﾓｰﾄﾞ処理へ(break)
					}
					else {
						// ﾃｰﾊﾟ開始ｵﾌﾞｼﾞｪｸﾄｴﾗｰ
						pBlock = pDoc->GetNCblock(pData1->GetBlockLineNo());
						pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_CORRECTSTART);
					}
				}
				// ﾃｰﾊﾟ角度ｴﾗｰは TH_NCRead.cpp で処理済み
			}
//			else if ( 0<=nGcode && nGcode<=3 ) {
			else {
				// 単純UV指示ﾁｪｯｸ
				ncArgv.nc.nLine  = pData1->GetBlockLineNo();
				ncArgv.nc.nGcode = nGcode;
				ncArgv.nc.dwValFlags &= NCD_CLEARVALUE;
				ncArgv.nc.dwValFlags |= NCD_Z;
				ncArgv.nc.dValue[NCA_Z] = dZ;
				dwValFlags = pData1->GetValFlags();
				if ( dwValFlags & (NCD_U|NCD_V) ) {
					// UVﾌﾗｸﾞをXYﾌﾗｸﾞに変換(3bit右ｼﾌﾄ)
					ncArgv.nc.dwValFlags |= (dwValFlags&(NCD_U|NCD_V)) >> NCXYZ;
					ncArgv.nc.dValue[NCA_X] = pData1->GetValue(NCA_U);
					ncArgv.nc.dValue[NCA_Y] = pData1->GetValue(NCA_V);
				}
				else {
					ncArgv.nc.dwValFlags |= (dwValFlags&(NCD_X|NCD_Y));
					ncArgv.nc.dValue[NCA_X] = pData1->GetValue(NCA_X);
					ncArgv.nc.dValue[NCA_Y] = pData1->GetValue(NCA_Y);
				}
				if ( 2<=nGcode && nGcode<=3 ) {
					// 円弧補間情報
					if ( dwValFlags & (NCD_K|NCD_L) ) {
						if ( dwValFlags & NCD_K ) {
							ncArgv.nc.dwValFlags |= NCD_I;
							ncArgv.nc.dValue[NCA_I] = pData1->GetValue(NCA_K);
						}
						if ( dwValFlags & NCD_L ) {
							ncArgv.nc.dwValFlags |= NCD_J;
							ncArgv.nc.dValue[NCA_J] = pData1->GetValue(NCA_L);
						}
					}
					else {
						ncArgv.nc.dwValFlags |= (dwValFlags&(NCD_I|NCD_J));
						ncArgv.nc.dValue[NCA_I] = pData1->GetValue(NCA_I);
						ncArgv.nc.dValue[NCA_J] = pData1->GetValue(NCA_J);
					}
				}
				// UV軸ｵﾌﾞｼﾞｪｸﾄ生成
				pData = CreateUVobj(pData, &ncArgv, pData1->GetOffsetPoint());
				pData1->SetWireObj(pData);
			}
		}
#ifdef _DEBUG
		dbg.printf("TaperMode StartBlock=%s Taper=%f",
			pDoc->GetNCblock(pData1->GetBlockLineNo())->GetStrGcode(), DEG(dTaper));
#endif
		// ﾃｰﾊﾟ処理ﾙｰﾌﾟ
		for ( ; i<nLoopCnt && nTaper>0 && IsThread(); i++ ) {
			if ( (i & 0x003f) == 0 )
				pParent->m_ctReadProgress.SetPos(i);
			pData2 = pDoc->GetNCdata(i);
			if ( pData2->GetGtype() != G_TYPE )
				continue;
			pRead2 = pData2->GetReadData();
			// 
			ncArgv.nc.nLine  = pData1->GetBlockLineNo();
			ncArgv.nc.nGcode = pData1->GetGcode();
			ncArgv.nc.dwValFlags &= NCD_CLEARVALUE;
			ncArgv.nc.dwValFlags |= NCD_Z;
			ncArgv.nc.dValue[NCA_Z] = dZ;
			//
			if ( nTaper == 1 ) {
				// ﾃｰﾊﾟ処理開始直後
				ptResult = pData2->CalcPerpendicularPoint(STARTPOINT, dT, pRead2->m_taper.nTaper);
				if ( ptResult ) {
					pt = *ptResult;	// pData1 の終点
					ncArgv.nc.dValue[NCA_X] = pt.x;
					ncArgv.nc.dValue[NCA_Y] = pt.y;
				}
				else {
					// 基本的に CalcPerpendicularPoint() でｴﾗｰはないが念のため
					pBlock = pDoc->GetNCblock(pData2->GetBlockLineNo());
					pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_CORRECTOBJECT);
				}
				ncArgv.nc.dwValFlags |= (NCD_X|NCD_Y);
				pData = CreateUVobj(pData, &ncArgv, pData1->GetOffsetPoint());
				pData1->SetWireObj(pData);
				nTaper++;	// 1 -> 2
			}
			else if ( pRead2->m_taper.nTaper == 0 ) {
				if ( pData2->GetType() == NCDLINEDATA ) {
					// 次のﾃﾞｰﾀ(pData2)で終了
					ptResult = pData1->CalcPerpendicularPoint(ENDPOINT, dT, pRead1->m_taper.nTaper);
					if ( ptResult ) {
						pt = *ptResult;	// pData1 の終点
						ncArgv.nc.dwValFlags |= (NCD_X|NCD_Y);
						ncArgv.nc.dValue[NCA_X] = pt.x;
						ncArgv.nc.dValue[NCA_Y] = pt.y;
						pData = CreateUVobj(pData, &ncArgv, pData1->GetOffsetPoint());
						pData1->SetWireObj(pData);
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
				nTaper = -1;		// break
			}
			else {
				// ﾃｰﾊﾟﾓｰﾄﾞ中は径補正と同じ計算
				ptResult = pData1->CalcOffsetIntersectionPoint(pData2, dT, pRead2->m_taper.nTaper);
				if ( ptResult ) {
					pt = *ptResult;
					ncArgv.nc.dwValFlags |= (NCD_X|NCD_Y);
					ncArgv.nc.dValue[NCA_X] = pt.x;
					ncArgv.nc.dValue[NCA_Y] = pt.y;
					pData = CreateUVobj(pData, &ncArgv, pData1->GetOffsetPoint());
					pData1->SetWireObj(pData);
				}
				else {
					pBlock = pDoc->GetNCblock(pData1->GetBlockLineNo());
					pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_CORRECTOBJECT);
				}
			}
			// 次のﾙｰﾌﾟヘ
			pData1 = pData2;
			pRead1 = pRead2;
		}
		if ( nTaper < 0 ) {
			// ﾃｰﾊﾟﾓｰﾄﾞの最終処理
			ncArgv.nc.nLine  = pData2->GetBlockLineNo();
			ncArgv.nc.nGcode = pData2->GetGcode();
			ncArgv.nc.dwValFlags &= NCD_CLEARVALUE;
			ncArgv.nc.dwValFlags |= (pData1->GetValFlags()&(NCD_X|NCD_Y)) | NCD_Z;
			ncArgv.nc.dValue[NCA_X] = pData2->GetValue(NCA_X);
			ncArgv.nc.dValue[NCA_Y] = pData2->GetValue(NCA_Y);
			ncArgv.nc.dValue[NCA_Z] = dZ;
			pData = CreateUVobj(pData, &ncArgv, pData2->GetOffsetPoint());
			pData2->SetWireObj(pData);
			nTaper++;	// -1 -> 0
		}
	} // End of Main(while) loop

}
catch (CMemoryException* e) {
	AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
	e->Delete();
	nResult = IDCANCEL;
}

	if ( pDataFirst )
		delete	pDataFirst;

	pParent->m_ctReadProgress.SetPos(nLoopCnt);
	pParent->PostMessage(WM_USERFINISH, IsThread() ? nResult : IDCANCEL);
#ifdef _DEBUG
	dbg.printf("PostMessage() Finish!");
#endif

	return 0;	// AfxEndThread(0);
}

//////////////////////////////////////////////////////////////////////
// 補助関数

CNCdata* CreateUVobj(const CNCdata* pData, LPNCARGV pArgv, const CPoint3D& ptOffset)
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
		pDataResult = new CNCcircle(pData, pArgv, ptOffset);
		break;
	default:
		pDataResult = new CNCdata(pData, pArgv, ptOffset);
	}

	return pDataResult;
}

void InitialVariable(const CNCdata* pData, LPNCARGV pArgv, double dZ)
{
	ZeroMemory(pArgv, sizeof(NCARGV));

	pArgv->nc.nGtype	= G_TYPE;
	pArgv->nc.nGcode	= pData->GetGcode();
	pArgv->nc.enPlane	= pData->GetPlane();
	pArgv->bAbs			= TRUE;

	CPoint3D	pts(pData->GetStartPoint());
	pArgv->nc.dValue[NCA_X] = pts.x;
	pArgv->nc.dValue[NCA_Y] = pts.y;
	pArgv->nc.dValue[NCA_Z] = dZ;
}
