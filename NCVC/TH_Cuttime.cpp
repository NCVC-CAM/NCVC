// TH_Cuttime.cpp
// 切削時間計算
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "NCVC.h"
#include "MachineOption.h"
#include "NCdata.h"
#include "NCDoc.h"
#include "NCInfoView.h"
#include "ThreadDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
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
	printf("CuttimeCalc_Thread() Start\n");
#endif
	LPNCVCTHREADPARAM	pParam = reinterpret_cast<LPNCVCTHREADPARAM>(pVoid);
	CNCDoc*			pDoc = static_cast<CNCDoc*>(pParam->pDoc);
	CNCInfoView1*	pView = reinterpret_cast<CNCInfoView1*>(pParam->wParam);
	ASSERT(pDoc);
	ASSERT(pView);
	delete	pVoid;	// 完全ﾊﾞｯｸｸﾞﾗｳﾝﾄﾞ処理

	const CMachineOption* pMCopt = AfxGetNCVCApp()->GetMachineOption();	// G00位置決め移動速度取得用
	INT_PTR		i, j, nLoopCnt = pDoc->GetNCsize();
	float		dTmp, dd;
	DWORD		dwValFlags;
	CNCdata*	pData;

#ifdef _DEBUG
	printf("GetNCsize()=%Id\n", nLoopCnt);
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
		printf("pMCopt->IsZeroG0Speed() Can't calc return\n");
#endif
		pDoc->m_dCutTime = -1.0f;	// 初期値「-1」で表示消去
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
				dd = pData->GetMove(j) / (pMCopt->GetG0Speed(j) / 60.0f);
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
				pDoc->m_dCutTime += pData->GetValue(NCA_P) / 1000.0f;
			else if ( dwValFlags & NCD_X )	// X値はsec単位
				pDoc->m_dCutTime += pData->GetValue(NCA_X);
			break;
		case 73:	case 74:	case 76:	// 固定ｻｲｸﾙ
		case 81:	case 82:	case 83:	case 84:	case 85:
		case 86:	case 87:	case 88:	case 89:
			// 早送り分
			dTmp = 0.0f;
			for ( j=0; j<NCXYZ; j++ ) {
				dd = pData->GetMove(j) / (pMCopt->GetG0Speed(j) / 60.0f);
				dTmp += (dd * dd);
			}
			pDoc->m_dCutTime += sqrt(dTmp);
			// 切削分
			if ( pData->GetFeed() != 0 )
				pDoc->m_dCutTime += pData->GetCutLength() / (pData->GetFeed() / 60.0f);
			// ﾄﾞｳｪﾙ時間[分]
			pDoc->m_dCutTime += static_cast<CNCcycle*>(pData)->GetDwell();
			break;
		}
	} // End of Loop

	// ﾌﾞﾛｯｸ処理の重み
	pDoc->m_dCutTime += pMCopt->GetDbl(MC_DBL_BLOCKWAIT) * pDoc->GetNCBlockSize();

	pView->PostMessage(WM_USERPROGRESSPOS);
#ifdef _DEBUG
	printf("PostMessage() Finish!\n");
#endif

	return 0;	// AfxEndThread(0);
}

//////////////////////////////////////////////////////////////////////

float GetCutTime_Milling(const CNCdata* pData)
{
	return pData->GetCutLength() / (pData->GetFeed() / 60.0f);
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
			dResult /= (pData->GetSpindle()/60.0f);	// １回転あたりの時間[sec]を掛ける
													// *= (1.0/pData->GetSpindle());
	}

	return dResult;
}
