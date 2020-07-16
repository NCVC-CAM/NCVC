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
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
extern	CMagaDbg	g_dbg;
#endif

static	CThreadDlg*	g_pParent;
#define	IsThread()	g_pParent->IsThreadContinue()

class	CHECKMAPTHREADPARAM {
public:
	CEvent		evStart,	// ﾙｰﾌﾟ開始ｲﾍﾞﾝﾄ
				evEnd;		// 終了待ち確認
	BOOL		bThread;	// ｽﾚｯﾄﾞの継続ﾌﾗｸﾞ
	CDXFmap*	pMap;		// 検査対象ﾏｯﾌﾟ
	// CHECKMAPTHREADPARAM::CEvent を手動ｲﾍﾞﾝﾄにするためのｺﾝｽﾄﾗｸﾀ
	CHECKMAPTHREADPARAM() : evStart(FALSE, TRUE), evEnd(TRUE, TRUE),
		bThread(TRUE), pMap(NULL)
	{}
};
#define	LPCHECKMAPTHREADPARAM	CHECKMAPTHREADPARAM *
static	UINT	CheckMapWorking_Thread(LPVOID);

static	void	SetChainMap(CLayerData*, LPCHECKMAPTHREADPARAM);
static	void	SearchChainMap(const CDXFmap*, CPointD&, CDXFmap*, CDXFmap&);

//////////////////////////////////////////////////////////////////////
//	連結ｵﾌﾞｼﾞｪｸﾄの検索ｽﾚｯﾄﾞ
//////////////////////////////////////////////////////////////////////

UINT ShapeSearch_Thread(LPVOID pVoid)
{
	static	LPCTSTR	szMother = "座標ﾏｯﾌﾟ母体生成";

	LPNCVCTHREADPARAM	pParam = (LPNCVCTHREADPARAM)pVoid;
	CDXFDoc*	pDoc = (CDXFDoc *)(pParam->pDoc);
	g_pParent = pParam->pParent;
	
	int		i, nLoop = pDoc->GetDxfSize(), nResult = IDOK;
	POSITION	pos;
	CLayerMap*	pLayerMap = pDoc->GetLayerMap();
	CString		strMsg, strLayer;
	CLayerData*	pLayer;
	CDXFdata*	pData;
	HANDLE		hCheckMapThread;
	CHECKMAPTHREADPARAM	chkParam;

	VERIFY(strMsg.LoadString(IDS_SHAPESEARCH));
	g_pParent->SetFaseMessage(strMsg, szMother);

	// 座標ﾏｯﾌﾟ検査ｽﾚｯﾄﾞ起動
	CWinThread*	pThread = AfxBeginThread(CheckMapWorking_Thread, &chkParam);
	hCheckMapThread = NC_DuplicateHandle(pThread->m_hThread);
	// SetPointMap() 前の静的変数初期化
	CDXFmap::ms_dTolerance = NCMIN;
	for ( pos=pLayerMap->GetStartPosition(); pos && IsThread(); ) {
		pLayerMap->GetNextAssoc(pos, strLayer, pLayer);
		pLayer->GetMasterMap()->InitHashTable(max(17, GetPrimeNumber(pLayer->GetCount()*2)));
	}
	
	try {
		// 全ﾃﾞｰﾀ対象にﾚｲﾔごとの座標ﾏｯﾌﾟ母体を作成
		g_pParent->m_ctReadProgress.SetRange32(0, nLoop);
		for ( i=0; i<nLoop && IsThread(); i++ ) {
			pData = pDoc->GetDxfData(i);
			if ( pData->GetType() != DXFPOINTDATA ) {
				pData->GetLayerData()->SetPointMasterMap(pData);
				pData->ClearMakeFlg();
				pData->ClearSearchFlg();
			}
			if ( (i & 0x003f) == 0 )	// 64回おき(下位6ﾋﾞｯﾄﾏｽｸ)
				g_pParent->m_ctReadProgress.SetPos(i);		// ﾌﾟﾛｸﾞﾚｽﾊﾞｰ
		}
		g_pParent->m_ctReadProgress.SetPos(nLoop);
		// ﾚｲﾔごとの固有座標ﾏｯﾌﾟ母体から連結ｵﾌﾞｼﾞｪｸﾄの検索
		for ( pos=pLayerMap->GetStartPosition(); pos && IsThread(); ) {
			pLayerMap->GetNextAssoc(pos, strLayer, pLayer);
			if ( pLayer->IsCutType() ) {
				g_pParent->SetFaseMessage(strMsg, strLayer);
				SetChainMap(pLayer, &chkParam);
				// 座標ﾏｯﾌﾟ母体を消去(ｽﾚｯﾄﾞ起動)
				pLayer->RemoveMasterMap();
			}
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

	if ( nResult==IDCANCEL || !IsThread() ) {
		// 途中でｷｬﾝｾﾙされたら座標ﾏｯﾌﾟをｸﾘｱ
		CDXFmapArray*	pMapArray;
		for ( pos=pLayerMap->GetStartPosition(); pos; ) {
			pLayerMap->GetNextAssoc(pos, strLayer, pLayer);
			if ( pLayer->IsCutType() ) {
				// 座標ﾏｯﾌﾟ母体を消去
				pLayer->RemoveMasterMap();
				// 連結集団を消去
				pMapArray = pLayer->GetChainMap();
				for ( i=0; i<pMapArray->GetSize(); i++ )
					delete	pMapArray->GetAt(i);
				pMapArray->RemoveAll();
			}
		}
		// ﾌﾗｸﾞも初期化
		for ( i=0; i<nLoop; i++ ) {
			pData = pDoc->GetDxfData(i);
			if ( pData->GetType() != DXFPOINTDATA ) {
				pData->ClearMakeFlg();
				pData->ClearSearchFlg();
			}
		}
	}

	g_pParent->PostMessage(WM_USERFINISH, IsThread() ? nResult : IDCANCEL);

	return 0;	// AfxEndThread(0);
}

//////////////////////////////////////////////////////////////////////

void SetChainMap(CLayerData* pLayer, LPCHECKMAPTHREADPARAM pParam)
{
	int			nCnt = 0, nShape = 0;
	CPointD		pt;
	const	CDXFmap*	pMasterMap = pLayer->GetMasterMap();
	CDXFmap		mapRegist;	// 連結座標登録済みﾏｯﾌﾟ
	CDXFmap*	pMap;
	CDXFarray*	pDummy;
	CString		strShape;

	g_pParent->m_ctReadProgress.SetRange32(0, pMasterMap->GetCount());
	pLayer->GetChainMap()->SetSize(0, 1024);
	mapRegist.InitHashTable(max(17, GetPrimeNumber(pLayer->GetCount())));
	
	// 座標ﾏｯﾌﾟ母体をｼｰｹﾝｼｬﾙにｱｸｾｽし連結固体を作成
	for ( POSITION pos=pMasterMap->GetStartPosition(); pos && IsThread(); nCnt++ ) {
		pMasterMap->GetNextAssoc(pos, pt, pDummy);
		if ( !mapRegist.Lookup(pt, pDummy) ) {
			strShape.Format("形状%04d", ++nShape);
			pMap = new CDXFmap(0, strShape);
			pMap->InitHashTable(max(17, GetPrimeNumber(pLayer->GetCount())));
			// 連結座標の検索
			SearchChainMap(pMasterMap, pt, pMap, mapRegist);	// 再帰呼び出し
#ifdef _DEBUG
			int		n = 0;
			BOOL	b = FALSE;
			for ( POSITION pos2=pMap->GetStartPosition(); pos2; ) {
				pMap->GetNextAssoc(pos2, pt, pDummy);
				n += pDummy->GetSize();
				if ( pDummy->GetSize() != 2 )
					b = TRUE;
			}
			g_dbg.printf("%s TotalObject=%d%s", strShape, n, b ? "(Warning!)" : "");
#endif
			// 前回の検査ｽﾚｯﾄﾞ終了待ち
			pParam->evEnd.Lock();
			pParam->evEnd.ResetEvent();
			// 検査ｽﾚｯﾄﾞ開始
			pParam->pMap = pMap;
			pParam->evStart.SetEvent();
			// 連結座標ﾏｯﾌﾟを登録
			pLayer->GetChainMap()->Add(pMap);
		}
		if ( (nCnt & 0x003f) == 0 )
			g_pParent->m_ctReadProgress.SetPos(nCnt);
	}
#ifdef _DEBUG
	g_dbg.printf("m_obChainMap.GetSize()=%d", pLayer->GetChainMap()->GetSize());
#endif
}

void SearchChainMap
	(const CDXFmap* pMasterMap, CPointD& pt, CDXFmap* pMap, CDXFmap& mapRegist)
{
	int			i, j;
	CPointD		ptSrc;
	CDXFarray*	pArray;
	CDXFarray*	pDummy;
	CDXFdata*	pData;

	VERIFY( pMasterMap->Lookup(pt, pArray) );		// 必ず存在する

	// 未登録の座標ﾃﾞｰﾀを検索
	if ( !mapRegist.Lookup(pt, pDummy) ) {
		pDummy = NULL;
		mapRegist.SetAt(pt, pDummy);
		for ( i=0; i<pArray->GetSize() && IsThread(); i++ ) {
			pData = pArray->GetAt(i);
			if ( !pData->IsMakeFlg() ) {
				pMap->SetPointMap(pData);
				pData->SetMakeFlg();
				// ｵﾌﾞｼﾞｪｸﾄの端点をさらに検索
				for ( j=0; j<pData->GetPointNumber() && IsThread(); j++ ) {
					ptSrc = pData->GetNativePoint(j);
					if ( ptSrc != HUGE_VAL && ptSrc != pt )
						SearchChainMap(pMasterMap, ptSrc, pMap, mapRegist);
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////
//	座標ﾏｯﾌﾟの検査ｽﾚｯﾄﾞ
//		交点や端点があれば
//			DXFMAPFLG_CANNOTWORKING -> 輪郭，ﾎﾟｹｯﾄ加工ができない
//		１つの座標に３つ以上のｵﾌﾞｼﾞｪｸﾄがあれば
//			DXFMAPFLG_CANNOTAUTOWORKING -> 自動処理から除外

UINT CheckMapWorking_Thread(LPVOID pVoid)
{
	LPCHECKMAPTHREADPARAM	pParam = (LPCHECKMAPTHREADPARAM)pVoid;
	int			i, j, nLoop;
	POSITION	pos;
	CPointD		pt, ptChk[4];
	CDXFarray*	pArray;
	CDXFarray	obArray;
	CDXFdata*	pData;

	obArray.SetSize(0, 1024);

	while ( IsThread() ) {
		// 上位の実行許可が下りるまでｳｪｲﾄ
		pParam->evStart.Lock();
		pParam->evStart.ResetEvent();
		// 継続ﾌﾗｸﾞﾁｪｯｸ
		if ( !pParam->bThread )
			break;
		// 検査開始
		for ( pos=pParam->pMap->GetStartPosition(); pos && IsThread(); ) {
			pParam->pMap->GetNextAssoc(pos, pt, pArray);
			// 端点や１つの座標に３つ以上のｵﾌﾞｼﾞｪｸﾄがないか
			if ( pArray->GetSize() != 2 )
				pParam->pMap->SetMapFlag(DXFMAPFLG_CANNOTAUTOWORKING);
			// 交点ﾁｪｯｸのためにｵﾌﾞｼﾞｪｸﾄをｺﾋﾟｰ
			for ( i=0; i<pArray->GetSize(); i++ ) {
				pData = pArray->GetAt(i);
				if ( !pData->IsSearchFlg() ) {
					obArray.Add(pData);
					pData->SetSearchFlg();
				}
			}
		}
		// 交点ﾁｪｯｸ
		nLoop = obArray.GetSize();
		for ( i=0; i<nLoop && IsThread(); i++ ) {
			pData = obArray[i];
			for ( j=i+1; j<nLoop && IsThread(); j++ ) {
				if ( pData->GetIntersectionPoint(obArray[j], ptChk) > 0 ) {
					pParam->pMap->SetMapFlag(DXFMAPFLG_CANNOTWORKING);
					break;
				}
			}
		}
		obArray.RemoveAll();
		// 検査終了
		pParam->evEnd.SetEvent();
	}

	pParam->evEnd.SetEvent();
	return 0;
}
