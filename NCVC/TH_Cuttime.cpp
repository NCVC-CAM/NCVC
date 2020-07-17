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

#define	IsThread()	pDoc->IsCalcContinue()

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

	const CMCOption*	pMCopt = AfxGetNCVCApp()->GetMCOption();	// G0位置決め移動速度取得用
	int			i, j, nLoopCnt = pDoc->GetNCsize();
	double		dTmp, dd;
	DWORD		dwValFlags;
	CNCdata*	pData;

#ifdef _DEBUG
	dbg.printf("GetNCsize()=%d", nLoopCnt);
#endif

	// 移動・切削長の合計計算
	pDoc->m_dMove[0] = pDoc->m_dMove[1] = 0.0;
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
	pDoc->m_dCutTime = 0.0;
	for ( i=0; i<nLoopCnt && IsThread(); i++ ) {
		pData = pDoc->GetNCdata(i);
		if ( pData->GetGtype() != G_TYPE )
			continue;
		// ｺｰﾄﾞごとの時間計算
		switch ( pData->GetGcode() ) {
		case 0:	// 早送り
			dTmp = 0.0;
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
				pDoc->m_dCutTime += pData->GetCutLength() / pData->GetFeed();
			break;
		case 4:		// ﾄﾞｳｪﾙ
			dwValFlags = pData->GetValFlags();
			if ( dwValFlags & NCD_P )		// P値はmsec単位
				pDoc->m_dCutTime += pData->GetValue(NCA_P) / 1000.0 / 60.0;
			else if ( dwValFlags & NCD_X )	// X値はsec単位
				pDoc->m_dCutTime += pData->GetValue(NCA_X) / 60.0;
			break;
		case 81:	// 固定ｻｲｸﾙ
		case 82:
		case 85:
		case 86:
		case 89:
			// 早送り分
			dTmp = 0.0;
			for ( j=0; j<NCXYZ; j++ ) {
				dd = pData->GetMove(j) / pMCopt->GetG0Speed(j);
				dTmp += (dd * dd);
			}
			pDoc->m_dCutTime += sqrt(dTmp);
			// 切削分
			if ( pData->GetFeed() != 0 )
				pDoc->m_dCutTime += pData->GetCutLength() / pData->GetFeed();
			// ﾄﾞｳｪﾙ時間[分]
			pDoc->m_dCutTime += static_cast<CNCcycle*>(pData)->GetDwell() / 60.0;
			break;
		}
	} // End of Loop

	// ﾌﾞﾛｯｸ処理の重み
	pDoc->m_dCutTime += pMCopt->GetBlockTime() * pDoc->GetNCBlockSize() / 60.0;

	pView->PostMessage(WM_USERPROGRESSPOS);
#ifdef _DEBUG
	dbg.printf("PostMessage() Finish!");
#endif

	return 0;	// AfxEndThread(0);
}
