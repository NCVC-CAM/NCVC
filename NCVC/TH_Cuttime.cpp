// TH_Cuttime.cpp
// 切削時間計算
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "NCVC.h"
#include "MCOption.h"
#include "NCdata.h"
#include "NCDoc.h"
#include "NCInfoView.h"
#include "ThreadDlg.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

#define	IsThread()	pDoc->IsDocFlag(NCDOC_CUTCALC)

static	float	GetCutTime_Milling(const CNCdata*);
static	float	GetCutTime_Lathe(const CNCdata*);

//////////////////////////////////////////////////////////////////////
//	切削時間の計算ｽﾚｯﾄﾞ
//////////////////////////////////////////////////////////////////////

UINT CNCDoc::CuttimeCalc_Thread(LPVOID pVoid)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CuttimeCalc_Thread()\nStart", DBG_BLUE);
#endif
	LPNCVCTHREADPARAM	pParam = reinterpret_cast<LPNCVCTHREADPARAM>(pVoid);
	CNCDoc*			pDoc = static_cast<CNCDoc*>(pParam->pDoc);
	CNCInfoView1*	pView = reinterpret_cast<CNCInfoView1*>(pParam->wParam);
	ASSERT(pDoc);
	ASSERT(pView);
	delete	pVoid;	// 完全ﾊﾞｯｸｸﾞﾗｳﾝﾄﾞ処理

	const CMCOption*	pMCopt = AfxGetNCVCApp()->GetMCOption();	// G00位置決め移動速度取得用
	INT_PTR		i, j, nLoopCnt = pDoc->GetNCsize();
	float		dTmp, dd;
	DWORD		dwValFlags;
	CNCdata*	pData;

#ifdef _DEBUG
	dbg.printf("GetNCsize()=%d", nLoopCnt);
#endif

	// 移動・切削長の合計計算
	ZEROCLR(pDoc->m_dMove);
	for ( i=0; i<nLoopCnt && IsThread(); i++ ) {
		pData = pDoc->GetNCdata(i);
		if ( pData->GetGtype() == G_TYPE ) {
			switch ( pData->GetType() ) {
			case NCDLINEDATA:	// 直線補間
			case NCDARCDATA:	// 円弧補間
				// 径補正等で終点が変わる可能性があるため、ここで計算
				j = pData->GetGcode() > 0 ? 1 : 0;
				pDoc->m_dMove[j] += pData->SetCalcLength();
				break;
			case NCDCYCLEDATA:	// 固定ｻｲｸﾙ
				pDoc->m_dMove[0] += static_cast<CNCcycle*>(pData)->GetCycleMove();
				pDoc->m_dMove[1] += pData->GetCutLength();
				break;
			}
		}
		// 読み込み後に不要になるﾃﾞｰﾀの削除
		pData->DeleteReadData();
	}

	// 切削時間の計算
	if ( pMCopt->IsZeroG0Speed() ) {
#ifdef _DEBUG
		dbg.printf("pMCopt->IsZeroG0Speed() Can't calc return");
#endif
		pDoc->m_dCutTime = -1.0;	// 初期値「-1」で表示消去
		pView->PostMessage(WM_USERPROGRESSPOS);
		return 0;
	}

	pDoc->m_dCutTime = 0.0f;
	boost::function<float (const CNCdata*)>	pfnGetCutTime =
		pDoc->IsDocFlag(NCDOC_LATHE) ?
		GetCutTime_Lathe : GetCutTime_Milling;

	for ( i=0; i<nLoopCnt && IsThread(); i++ ) {
		pData = pDoc->GetNCdata(i);
		if ( pData->GetGtype() != G_TYPE )
			continue;
		// ｺｰﾄﾞごとの時間計算
		switch ( pData->GetGcode() ) {
		case 0:	// 早送り
			dTmp = 0.0f;
			for ( j=0; j<NCXYZ; j++ ) {
				dd = pData->GetMove(j) / pMCopt->GetG0Speed(j);
				dTmp += (dd * dd);
			}
			pDoc->m_dCutTime += sqrt(dTmp);
			break;
		case 1:		// 直線補間
		case 2:		// 円弧補間
		case 3:
			if ( pData->GetFeed() != 0 )
				pDoc->m_dCutTime += pfnGetCutTime(pData);
			break;
		case 4:		// ﾄﾞｳｪﾙ
			dwValFlags = pData->GetValFlags();
			if ( dwValFlags & NCD_P )		// P値はmsec単位
				pDoc->m_dCutTime += pData->GetValue(NCA_P) / 1000.0f / 60.0f;
			else if ( dwValFlags & NCD_X )	// X値はsec単位
				pDoc->m_dCutTime += pData->GetValue(NCA_X) / 60.0f;
			break;
		case 81:	// 固定ｻｲｸﾙ
		case 82:
		case 85:
		case 86:
		case 89:
			// 早送り分
			dTmp = 0.0f;
			for ( j=0; j<NCXYZ; j++ ) {
				dd = pData->GetMove(j) / pMCopt->GetG0Speed(j);
				dTmp += (dd * dd);
			}
			pDoc->m_dCutTime += sqrt(dTmp);
			// 切削分
			if ( pData->GetFeed() != 0 )
				pDoc->m_dCutTime += pData->GetCutLength() / pData->GetFeed();
			// ﾄﾞｳｪﾙ時間[分]
			pDoc->m_dCutTime += static_cast<CNCcycle*>(pData)->GetDwell() / 60.0f;
			break;
		}
	} // End of Loop

	// ﾌﾞﾛｯｸ処理の重み
	pDoc->m_dCutTime += pMCopt->GetDbl(MC_DBL_BLOCKWAIT) * pDoc->GetNCBlockSize() / 60.0f;

	pView->PostMessage(WM_USERPROGRESSPOS);
#ifdef _DEBUG
	dbg.printf("PostMessage() Finish!");
#endif

	return 0;	// AfxEndThread(0);
}

//////////////////////////////////////////////////////////////////////

float GetCutTime_Milling(const CNCdata* pData)
{
	return pData->GetCutLength() / pData->GetFeed();
}

float GetCutTime_Lathe(const CNCdata* pData)
{
	// 毎分送り計算
	float	dResult = GetCutTime_Milling(pData);

	// G99毎回転送りなら
	if ( !pData->GetG98() ) {	
		if ( pData->GetSpindle() == 0 )	// 主軸回転数は必須
			dResult = 0.0f;
		else
			dResult /= pData->GetSpindle();		// １回転あたりの時間[min]を掛ける
												// *= (1.0/pData->GetSpindle());
	}

	return dResult;
}
