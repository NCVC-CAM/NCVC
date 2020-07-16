// TH_ShapeSearch.cpp
// 連結ｵﾌﾞｼﾞｪｸﾄを検索
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "NCVC.h"
#include "DXFdata.h"
#include "DXFshape.h"
#include "Layer.h"
#include "DXFDoc.h"
#include "ThreadDlg.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

static	CThreadDlg*	g_pParent;
#define	IsThread()	g_pParent->IsThreadContinue()

struct	CHECKMAPTHREADPARAM
{
	CEvent		evStart,	// ﾙｰﾌﾟ開始ｲﾍﾞﾝﾄ
				evEnd;		// 終了待ち確認
	BOOL		bThread;	// ｽﾚｯﾄﾞの継続ﾌﾗｸﾞ
	CLayerData*	pLayer;		// 対象ﾚｲﾔ
	CDXFmap*	pMap;		// 検査対象ﾏｯﾌﾟ
	// CHECKMAPTHREADPARAM::CEvent を手動ｲﾍﾞﾝﾄにするためのｺﾝｽﾄﾗｸﾀ
	CHECKMAPTHREADPARAM() : evStart(FALSE, TRUE), evEnd(TRUE, TRUE),
		bThread(TRUE), pMap(NULL)
	{}
};
#define	LPCHECKMAPTHREADPARAM	CHECKMAPTHREADPARAM *
static	UINT	CheckMapWorking_Thread(LPVOID);

static	void	SetChainMap(const CDXFmap*, LPCHECKMAPTHREADPARAM);
static	void	SearchChainMap(const CDXFmap*, const CPointD&, CDXFmap*, CDXFmap&);

//////////////////////////////////////////////////////////////////////
//	連結ｵﾌﾞｼﾞｪｸﾄの検索ｽﾚｯﾄﾞ
//////////////////////////////////////////////////////////////////////

UINT ShapeSearch_Thread(LPVOID pVoid)
{
	LPNCVCTHREADPARAM	pParam = (LPNCVCTHREADPARAM)pVoid;
	CDXFDoc*	pDoc = (CDXFDoc *)(pParam->pDoc);
	g_pParent = pParam->pParent;
	
	int		i, j, nResult = IDOK;
	INT_PTR	nLayerCnt = pDoc->GetLayerCnt(), nDataCnt;
	CString		strMsg;
	CLayerData*	pLayer;
	CDXFdata*	pData;
	CDXFmap		mpDXFdata;	// ﾚｲﾔごとの座標ﾏｯﾌﾟ母体
	HANDLE		hCheckMapThread;
	CHECKMAPTHREADPARAM	chkParam;

	VERIFY(strMsg.LoadString(IDS_SHAPESEARCH));

	// 座標ﾏｯﾌﾟ検査ｽﾚｯﾄﾞ起動
	CWinThread*	pThread = AfxBeginThread(CheckMapWorking_Thread, &chkParam);
	hCheckMapThread = ::NCVC_DuplicateHandle(pThread->m_hThread);
	if ( !hCheckMapThread )
		::NCVC_CriticalErrorMsg(__FILE__, __LINE__);
	// SetPointMap() 前の静的変数初期化
	CDXFmap::ms_dTolerance = NCMIN;

	try {
		// 全ﾃﾞｰﾀ対象にﾚｲﾔごとの座標ﾏｯﾌﾟ母体を作成
		for ( i=0; i<nLayerCnt && IsThread(); i++ ) {
			pLayer = pDoc->GetLayerData(i);
			if ( !pLayer->IsCutType() )
				continue;
			nDataCnt = pLayer->GetDxfSize();
			g_pParent->SetFaseMessage(strMsg, pLayer->GetStrLayer());
			g_pParent->m_ctReadProgress.SetRange32(0, nDataCnt);
			mpDXFdata.InitHashTable(max(17, GetPrimeNumber(nDataCnt*2)));
			for ( j=0; j<nDataCnt && IsThread(); j++ ) {
				pData = pLayer->GetDxfData(j);
				if ( pData->GetType() != DXFPOINTDATA ) {
					mpDXFdata.SetPointMap(pData);
					pData->ClearMakeFlg();
					pData->ClearSearchFlg();
				}
				if ( (j & 0x003f) == 0 )	// 64回おき(下位6ﾋﾞｯﾄﾏｽｸ)
					g_pParent->m_ctReadProgress.SetPos(j);		// ﾌﾟﾛｸﾞﾚｽﾊﾞｰ
			}
			g_pParent->m_ctReadProgress.SetPos(nDataCnt);
			// ﾚｲﾔごとの固有座標ﾏｯﾌﾟ母体から連結ｵﾌﾞｼﾞｪｸﾄの検索
			chkParam.pLayer = pLayer;
			SetChainMap(&mpDXFdata, &chkParam);
			// 座標ﾏｯﾌﾟ母体を消去
			mpDXFdata.RemoveAll();
		}
		// 最終検査ｽﾚｯﾄﾞ終了待ち
		chkParam.evEnd.Lock();
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		nResult = IDCANCEL;
	}

	// 検査ｽﾚｯﾄﾞ終了指示
	chkParam.bThread = FALSE;
	chkParam.evStart.SetEvent();
	WaitForSingleObject(hCheckMapThread, INFINITE);
	CloseHandle(hCheckMapThread);

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

	g_pParent->PostMessage(WM_USERFINISH, IsThread() ? nResult : IDCANCEL);

	return 0;	// AfxEndThread(0);
}

//////////////////////////////////////////////////////////////////////

void SetChainMap(const CDXFmap* pMasterMap, LPCHECKMAPTHREADPARAM pParam)
{
	int			nCnt = 0;
	CPointD		pt;
	CDXFmap		mapRegist;	// 連結座標登録済みﾏｯﾌﾟ
	CDXFmap*	pMap;
	CDXFarray*	pDummy;

	g_pParent->m_ctReadProgress.SetRange32(0, pMasterMap->GetCount());
	mapRegist.InitHashTable(max(17, GetPrimeNumber(pParam->pLayer->GetDxfSize())));
	
	// 座標ﾏｯﾌﾟ母体をｼｰｹﾝｼｬﾙにｱｸｾｽし連結固体を作成
	for ( POSITION pos=pMasterMap->GetStartPosition(); pos && IsThread(); nCnt++ ) {
		pMasterMap->GetNextAssoc(pos, pt, pDummy);
		if ( !mapRegist.PLookup(pt) ) {
			pMap = new CDXFmap;
			pMap->InitHashTable(max(17, GetPrimeNumber(pParam->pLayer->GetDxfSize())));
			// 連結座標の検索
			SearchChainMap(pMasterMap, pt, pMap, mapRegist);	// 再帰呼び出し
			// 前回の検査ｽﾚｯﾄﾞ終了待ち
			pParam->evEnd.Lock();
			pParam->evEnd.ResetEvent();
			// 検査ｽﾚｯﾄﾞ開始
			pParam->pMap = pMap;
			pParam->evStart.SetEvent();
		}
		if ( (nCnt & 0x003f) == 0 )
			g_pParent->m_ctReadProgress.SetPos(nCnt);
	}
#ifdef _DEBUG
	g_dbg.printf("m_obShapeArray.GetSize()=%d", pParam->pLayer->GetShapeSize());
#endif
}

void SearchChainMap
	(const CDXFmap* pMasterMap, const CPointD& pt, CDXFmap* pMap, CDXFmap& mapRegist)
{
	int			i, j;
	CPointD		ptSrc;
	CDXFarray*	pArray;
	CDXFarray*	pDummy = NULL;
	CDXFdata*	pData;

	VERIFY( pMasterMap->Lookup(const_cast<CPointD&>(pt), pArray) );		// 必ず存在する

	// 未登録の座標ﾃﾞｰﾀを検索
	if ( !mapRegist.PLookup(const_cast<CPointD&>(pt)) ) {
		mapRegist.SetAt(const_cast<CPointD&>(pt), pDummy);
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
	LPCHECKMAPTHREADPARAM	pParam = (LPCHECKMAPTHREADPARAM)pVoid;
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
			pParam->pMap->CopyToChain(pChain);
			delete	pParam->pMap;	// 元のCDXFmapは消去
			strShape.Format("輪郭%04d", ++nChain);
			pShape = new CDXFshape(DXFSHAPE_OUTLINE, strShape, 0, pChain);
		}
		else {
			strShape.Format("軌跡%04d", ++nMap);
			pShape = new CDXFshape(DXFSHAPE_LOCUS, strShape, dwFlags, pParam->pMap);
		}
		// 形状情報登録
		pParam->pLayer->AddShape(pShape);
		// 検査終了
		pParam->evEnd.SetEvent();
	}

	pParam->evEnd.SetEvent();
	return 0;
}
