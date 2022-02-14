// TH_ShapeSearch.cpp
// 連結ｵﾌﾞｼﾞｪｸﾄを検索
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "DXFdata.h"
#include "DXFshape.h"
#include "Layer.h"
#include "DXFDoc.h"
#include "ThreadDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

static	CThreadDlg*	g_pParent;
static	BOOL	IsThread_Dlg(void)
{
	return g_pParent->IsThreadContinue();
}
static	BOOL	IsThread_NoChk(void)
{
	return TRUE;
}
static	boost::function<BOOL ()>	IsThread;
static	inline	void	_SetProgressPos(INT_PTR n)
{
	g_pParent->m_ctReadProgress.SetPos((int)n);
}

struct	CHECKMAPTHREADPARAM
{
	CEvent		evStart;	// ﾙｰﾌﾟ開始ｲﾍﾞﾝﾄ
	CEvent		evEnd;		// 終了待ち確認
	BOOL		bThread;	// ｽﾚｯﾄﾞの継続ﾌﾗｸﾞ
	CLayerData*	pLayer;		// 対象ﾚｲﾔ
	CDXFmap*	pMap;		// 検査対象ﾏｯﾌﾟ
	// CEvent を手動ｲﾍﾞﾝﾄにするためのｺﾝｽﾄﾗｸﾀ
	CHECKMAPTHREADPARAM() : evStart(FALSE, TRUE), evEnd(FALSE, TRUE), bThread(TRUE), pMap(NULL) {}
};
typedef	CHECKMAPTHREADPARAM*	LPCHECKMAPTHREADPARAM;
static	UINT	CheckMapWorking_Thread(LPVOID);

static	void	SetChainMap(const CDXFmap*, CLayerData*, LPCHECKMAPTHREADPARAM);
static	void	SearchChainMap(const CDXFmap*, const CPointF&, CDXFmap*, CDXFmap&);

//////////////////////////////////////////////////////////////////////
//	連結ｵﾌﾞｼﾞｪｸﾄの検索ｽﾚｯﾄﾞ
//////////////////////////////////////////////////////////////////////

UINT ShapeSearch_Thread(LPVOID pVoid)
{
	LPNCVCTHREADPARAM	pParam = reinterpret_cast<LPNCVCTHREADPARAM>(pVoid);
	CDXFDoc*	pDoc = static_cast<CDXFDoc*>(pParam->pDoc);
	g_pParent = pParam->pParent;
	IsThread = g_pParent ? &IsThread_Dlg : &IsThread_NoChk;

	int			nResult = IDOK;
	INT_PTR		i, j, nLayerCnt = pDoc->GetLayerCnt(), nDataCnt;
	CString		strMsg;
	CLayerData*	pLayer;
	CDXFdata*	pData;
	CDXFmap		mpDXFdata;	// ﾚｲﾔごとの座標ﾏｯﾌﾟ母体
	CHECKMAPTHREADPARAM	chkParam;

	VERIFY(strMsg.LoadString(IDS_SHAPESEARCH));

	// ｲﾍﾞﾝﾄ初期設定
	chkParam.evStart.ResetEvent();
	// 座標ﾏｯﾌﾟ検査ｽﾚｯﾄﾞ起動
	CWinThread*	pThread = AfxBeginThread(CheckMapWorking_Thread, &chkParam);
	if ( !pThread )
		::NCVC_CriticalErrorMsg(__FILE__, __LINE__);

	// SetPointMap() 前の静的変数初期化
	CDXFmap::ms_dTolerance = NCMIN;

	try {
		// 全ﾃﾞｰﾀ対象にﾚｲﾔごとの座標ﾏｯﾌﾟ母体を作成
		for ( i=0; i<nLayerCnt && IsThread(); i++ ) {
			pLayer = pDoc->GetLayerData(i);
			if ( !pLayer->IsCutType() )
				continue;
			pLayer->RemoveAllShape();
			nDataCnt = pLayer->GetDxfSize();
			if ( g_pParent ) {
				g_pParent->SetFaseMessage(strMsg, pLayer->GetLayerName());
				g_pParent->m_ctReadProgress.SetRange32(0, (int)nDataCnt);
			}
			j = GetPrimeNumber((UINT)nDataCnt*2);
			mpDXFdata.InitHashTable((UINT)max(17, j));
			for ( j=0; j<nDataCnt && IsThread(); j++ ) {
				pData = pLayer->GetDxfData(j);
				if ( pData->GetType() != DXFPOINTDATA ) {
					mpDXFdata.SetPointMap(pData);
					pData->ClearMakeFlg();
					pData->ClearSearchFlg();
					// ﾎﾟﾘﾗｲﾝ自身の交点ﾁｪｯｸ
					if ( pData->GetType() == DXFPOLYDATA )
						static_cast<CDXFpolyline*>(pData)->CheckPolylineIntersection();
				}
				if ( g_pParent && (j & 0x003f) == 0 )	// 64回おき(下位6ﾋﾞｯﾄﾏｽｸ)
					_SetProgressPos(j);		// ﾌﾟﾛｸﾞﾚｽﾊﾞｰ
			}
			if ( g_pParent )
				_SetProgressPos(nDataCnt);
			// ﾚｲﾔごとの固有座標ﾏｯﾌﾟ母体から連結ｵﾌﾞｼﾞｪｸﾄの検索
			SetChainMap(&mpDXFdata, pLayer, &chkParam);
			// 座標ﾏｯﾌﾟ母体を消去
			mpDXFdata.RemoveAll();
		}
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		nResult = IDCANCEL;
	}

	// 検査ｽﾚｯﾄﾞ終了指示
	chkParam.bThread = FALSE;
	chkParam.evStart.SetEvent();
	WaitForSingleObject(pThread->m_hThread, INFINITE);

	// 途中でｷｬﾝｾﾙされたら座標ﾏｯﾌﾟをｸﾘｱ
	if ( nResult==IDCANCEL || !IsThread() ) {
		for ( i=0; i<nLayerCnt; i++ ) {
			pLayer = pDoc->GetLayerData(i);
			if ( !pLayer->IsCutType() )
				continue;
			// 連結集団を消去
			pLayer->RemoveAllShape();
			// ﾌﾗｸﾞも初期化
			nDataCnt = pLayer->GetDxfSize();
			for ( j=0; j<nDataCnt; j++ ) {
				pData = pLayer->GetDxfData(j);
				if ( pData->GetType() != DXFPOINTDATA ) {
					pData->ClearMakeFlg();
					pData->ClearSearchFlg();
				}
			}
		}
	}

	if ( g_pParent )
		g_pParent->PostMessage(WM_USERFINISH, IsThread() ? nResult : IDCANCEL);

	return 0;	// AfxEndThread(0);
}

//////////////////////////////////////////////////////////////////////

void SetChainMap(const CDXFmap* pMasterMap, CLayerData* pLayer, LPCHECKMAPTHREADPARAM pParam)
{
	int			nCnt = 0,
				nPrime = GetPrimeNumber((UINT)pLayer->GetDxfSize());
	CPointF		pt;
	CDXFmap		mapRegist;	// 連結座標登録済みﾏｯﾌﾟ
	CDXFmap*	pMap;
	CDXFarray*	pDummy;

	if ( g_pParent )
		g_pParent->m_ctReadProgress.SetRange32(0, (int)pMasterMap->GetCount());
	mapRegist.InitHashTable(max(17, nPrime));
	pParam->evEnd.SetEvent();

	// 座標ﾏｯﾌﾟ母体をｼｰｹﾝｼｬﾙにｱｸｾｽし連結固体を作成
	for ( POSITION pos=pMasterMap->GetStartPosition(); pos && IsThread(); nCnt++ ) {
		pMasterMap->GetNextAssoc(pos, pt, pDummy);
		if ( !mapRegist.PLookup(pt) ) {
			pMap = new CDXFmap;
			pMap->InitHashTable(max(17, nPrime));
			// 連結座標の検索
			SearchChainMap(pMasterMap, pt, pMap, mapRegist);	// 再帰呼び出し
			// 前回の検査ｽﾚｯﾄﾞ終了待ち
			pParam->evEnd.Lock();
			pParam->evEnd.ResetEvent();
			// 検査ｽﾚｯﾄﾞ開始
			pParam->pMap	= pMap;
			pParam->pLayer	= pLayer;
			pParam->evStart.SetEvent();
		}
		if ( g_pParent && (nCnt & 0x003f) == 0 )
			_SetProgressPos(nCnt);
	}
	pParam->evEnd.Lock();
	pParam->evEnd.ResetEvent();

#ifdef _DEBUG
	printf("m_obShapeArray.GetSize()=%Id\n", pLayer->GetShapeSize());
#endif
}

void SearchChainMap
	(const CDXFmap* pMasterMap, const CPointF& pt, CDXFmap* pMap, CDXFmap& mapRegist)
{
	int			i, j;
	CPointF		ptSrc;
	CDXFarray*	pArray;
	CDXFarray*	pDummy = NULL;
	CDXFdata*	pData;

	if ( !pMasterMap->Lookup(const_cast<CPointF&>(pt), pArray) )	// 必ず存在する
		NCVC_CriticalErrorMsg(__FILE__, __LINE__);

	// 未登録の座標ﾃﾞｰﾀを検索
	if ( !mapRegist.PLookup(const_cast<CPointF&>(pt)) ) {
		mapRegist.SetAt(const_cast<CPointF&>(pt), pDummy);
		for ( i=0; i<pArray->GetSize() && IsThread(); i++ ) {
			pData = pArray->GetAt(i);
			if ( !pData->IsMakeFlg() ) {
				pMap->SetPointMap(pData);
				pData->SetMakeFlg();
				// ｵﾌﾞｼﾞｪｸﾄの端点をさらに検索
				for ( j=0; j<pData->GetPointNumber() && IsThread(); j++ ) {
					ptSrc = pData->GetNativePoint(j);
					if ( ptSrc != pt )
						SearchChainMap(pMasterMap, ptSrc, pMap, mapRegist);
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////
//	各集団(座標ﾏｯﾌﾟ)ごとの検査ｽﾚｯﾄﾞ

UINT CheckMapWorking_Thread(LPVOID pVoid)
{
	LPCHECKMAPTHREADPARAM	pParam = reinterpret_cast<LPCHECKMAPTHREADPARAM>(pVoid);
	int			nChain = 0, nMap = 0;
	CString		strShape;
	DWORD		dwFlags;
	CDXFchain*	pChain;
	CDXFshape*	pShape;

	while ( IsThread() ) {
		// 上位の実行許可が下りるまでｳｪｲﾄ
		pParam->evStart.Lock();
		pParam->evStart.ResetEvent();
		// 継続ﾌﾗｸﾞﾁｪｯｸ
		if ( !pParam->bThread )
			break;
		// 検査開始
		dwFlags = pParam->pMap->GetMapTypeFlag();
		// 形状情報生成
		if ( dwFlags == 0 ) {
			// CDXFmapからCDXFchainに昇格
			pChain = new CDXFchain;
			if ( pParam->pMap->CopyToChain(pChain) ) {
				delete	pParam->pMap;	// 元のCDXFmapは消去
				strShape.Format("輪郭%04d", ++nChain);
				pShape = new CDXFshape(DXFSHAPE_OUTLINE, strShape, 0, pParam->pLayer, pChain);
			}
			else {
				delete	pChain;			// 昇格失敗
				dwFlags = 1;			// 軌跡処理へ
			}
		}
		if ( dwFlags != 0 ) {
			strShape.Format("軌跡%04d", ++nMap);
			pShape = new CDXFshape(DXFSHAPE_LOCUS, strShape, dwFlags, pParam->pLayer, pParam->pMap);
		}
		// 形状情報登録
		pParam->pLayer->AddShape(pShape);
#ifdef _DEBUG
		printf("Layer=%s %s Add ok\n", LPCTSTR(pParam->pLayer->GetLayerName()), LPCTSTR(strShape));
#endif
		// 検査終了
		pParam->evEnd.SetEvent();
	}

	return 0;
}
