// TH_AutoWorkingSet.cpp
// 自動加工指示
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"	// DXFView.h
#include "DXFdata.h"
#include "DXFshape.h"
#include "Layer.h"
#include "DXFDoc.h"
#include "DXFView.h"	// DXFTREETYPE
#include "ThreadDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using std::vector;
using namespace boost;

static	CThreadDlg*	g_pParent;
#define	IsThread()	g_pParent->IsThreadContinue()
static	inline	void	_SetProgressRange(INT_PTR n)
{
	g_pParent->m_ctReadProgress.SetRange32(0, (int)n);
}
static	inline	void	_SetProgressPos(INT_PTR n)
{
	g_pParent->m_ctReadProgress.SetPos((int)n);
}

//	SeparateOutline_Thread
struct	SEPARATEOUTLINETHREADPARAM
{
	CEvent		evStart;			// ﾙｰﾌﾟ開始ｲﾍﾞﾝﾄ
	CEvent		evEnd;				// 終了待ち確認
	BOOL		bThread;			// ｽﾚｯﾄﾞの継続ﾌﾗｸﾞ
	CDXFworkingOutline*	pOutline;	// 対象輪郭ｵﾌﾞｼﾞｪｸﾄ集合
	// SEPARATEOUTLINETHREADPARAM::CEvent を手動ｲﾍﾞﾝﾄにするためのｺﾝｽﾄﾗｸﾀ
	SEPARATEOUTLINETHREADPARAM() : evStart(FALSE, TRUE), evEnd(FALSE, TRUE), bThread(TRUE), pOutline(NULL) {}
};
typedef	SEPARATEOUTLINETHREADPARAM*		LPSEPARATEOUTLINETHREADPARAM;
static	UINT	SeparateOutline_Thread(LPVOID);

static	void	AutoWorkingProc(const CLayerData*, LPVOID);
static	void	AutoRecalcWorking(const CLayerData*, LPVOID);
static	void	CheckStrictOffset(const CLayerData*, LPVOID);

static	BOOL	SelectOutline(const CLayerData*);
static	BOOL	SelectPocket(const CLayerData*);
static	void	SelectPocket_Reflex(const CLayerData*, INT_PTR, int);
static	void	CreateAutoWorking(const CLayerData*, float);
static	BOOL	CreateScanLine(CDXFshape*, LPAUTOWORKINGDATA);
static	void	CheckStrictOffset_forScan(const CLayerData*);
static	BOOL	CheckOffsetIntersection(CDXFchain*, CDXFchain*, BOOL);
static	void	CheckCircleIntersection(const CLayerData*, const CDXFshape*, const CDXFworkingOutline*);
static	CDXFdata*	ChangeCircleToArc(const CDXFcircle*, const CDXFchain*, CPointF&, CPointF&);
static	void	SetAllExcludeData(CDXFchain*);
static	CDXFworkingOutline*	GetOutlineHierarchy(CDXFshape*, INT_PTR);
static	CDXFworkingOutline*	GetOutlineLastObj(CDXFshape*, INT_PTR);

//////////////////////////////////////////////////////////////////////
//	自動加工指示ｽﾚｯﾄﾞ
//////////////////////////////////////////////////////////////////////

UINT AutoWorkingSet_Thread(LPVOID pThread)
{
	LPNCVCTHREADPARAM	pVoid = reinterpret_cast<LPNCVCTHREADPARAM>(pThread);
	g_pParent = pVoid->pParent;
	CDXFDoc*	pDoc = static_cast<CDXFDoc*>(pVoid->pDoc);
	INT_PTR			i;
	const INT_PTR	nLoop = pDoc->GetLayerCnt();
	int				nType = (int)(pVoid->wParam);	// 処理ﾀｲﾌﾟ
	DXFTREETYPE*	vSelect = NULL;
	LPVOID			pParam = NULL;
	function<void (const CLayerData*, LPVOID)>	pfnAutoProc;

	switch ( nType ) {
	case AUTOWORKING:
		pfnAutoProc = &AutoWorkingProc;
		pParam = reinterpret_cast<LPVOID>(pVoid->lParam);			// ﾀﾞｲｱﾛｸﾞ情報
		break;
	case AUTORECALCWORKING:
		pfnAutoProc = &AutoRecalcWorking;
		vSelect = reinterpret_cast<DXFTREETYPE*>(pVoid->lParam);	// 再計算時の形状集合
		pParam = vSelect;
		break;
	case AUTOSTRICTOFFSET:
		pfnAutoProc = &CheckStrictOffset;
		pParam = reinterpret_cast<LPVOID>(TRUE);		// not NULL (処理にﾙｰﾌﾟが必要)
		break;
	default:
		g_pParent->PostMessage(WM_USERFINISH, IDCANCEL);
		return 0;
	}

	CString		strMsg;
	CLayerData*	pLayer;

	VERIFY(strMsg.LoadString(IDS_AUTOWORKING));

	// ﾚｲﾔごとに座標ﾏｯﾌﾟを検索
	for ( i=0; i<nLoop && IsThread(); i++ ) {
		pLayer = pDoc->GetLayerData(i);
		if ( !pLayer->IsCutType() || 
				(vSelect && vSelect->which()==DXFTREETYPE_LAYER && pLayer!=get<CLayerData*>(*vSelect)) )
			continue;
		g_pParent->SetFaseMessage(strMsg, pLayer->GetLayerName());
		// ﾀｲﾌﾟ別の自動処理
		pfnAutoProc(pLayer, pParam);
		//
		if ( vSelect && vSelect->which()==DXFTREETYPE_LAYER )
			break;	// ﾚｲﾔ指定あれば以降必要なし
	}

	g_pParent->PostMessage(WM_USERFINISH, IsThread() ? IDOK : IDCANCEL);
	return 0;	// AfxEndThread(0);
}

//////////////////////////////////////////////////////////////////////
//	自動加工指示
//		輪郭・ﾎﾟｹｯﾄの処理選択
void AutoWorkingProc(const CLayerData* pLayer, LPVOID pParam)
{
	int		i;
	LPAUTOWORKINGDATA	pAuto = reinterpret_cast<LPAUTOWORKINGDATA>(pParam);	// ﾀﾞｲｱﾛｸﾞ情報

	pLayer->AllShapeClearSideFlg();

	// 処理に応じた内外判定
	if ( pAuto->nSelect == 0 )
		SelectOutline(pLayer);
	else
		SelectPocket(pLayer);

	// 輪郭生成
	for ( i=0; i<pAuto->nLoopCnt && IsThread(); i++ ) {
		CreateAutoWorking(pLayer, pAuto->dOffset*(i+1));
		CheckStrictOffset(pLayer, NULL);
	}

	// ﾎﾟｹｯﾄ加工は
	if ( pAuto->nSelect == 1 ) {
		CDXFshape*	pShape;
		// 走査線生成
		for ( i=0; i<pLayer->GetShapeSize() && IsThread(); i++ ) {
			pShape = pLayer->GetShapeData(i);
			if ( pShape->GetShapeFlag() & DXFMAPFLG_INSIDE )
				CreateScanLine(pShape, pAuto);
		}
		// 走査線だけを対象にしたｵﾌｾｯﾄ同士の交点除去
		CheckStrictOffset_forScan(pLayer);
		// 複数の輪郭ｵﾌﾞｼﾞｪｸﾄを束ねる
	}
}

//////////////////////////////////////////////////////////////////////
//	ｵﾌｾｯﾄ値変更による再計算
void AutoRecalcWorking(const CLayerData* pLayer, LPVOID pParam)
{
	DXFTREETYPE*	vSelect = reinterpret_cast<DXFTREETYPE*>(pParam);		// 再計算時の形状集合
	INT_PTR			i, j, nOutline;
	const INT_PTR	nLoop = pLayer->GetShapeSize();
	int			nInOut;
	float		dOffset, dOffsetOrg;
	CDXFshape*	pShape;
	CDXFshape*	pShapeSrc = NULL;
	CDXFchain	ltOutline;
	COutlineList*		pOutlineList;
	CDXFworkingOutline*	pOutline;

	// 対象集合
	if ( vSelect && vSelect->which()==DXFTREETYPE_SHAPE )
		pShapeSrc = get<CDXFshape*>(*vSelect);

	_SetProgressRange(nLoop);

	try {
		for ( i=0; i<nLoop && IsThread(); i++ ) {
			pShape = pLayer->GetShapeData(i);
			// ﾌﾟﾛｸﾞﾚｽﾊﾞｰ
			_SetProgressPos(i);
			// 形状集合が指定されていれば、それにﾏｯﾁするものだけ
			if ( pShapeSrc && pShapeSrc != pShape )
				continue;
			// 既にある加工指示を抽出
			pOutlineList = pShape->GetOutlineList();
			nOutline = pOutlineList->GetCount();
			if ( nOutline <= 0 )
				continue;
			dOffset = dOffsetOrg = pShape->GetOffset();
			nInOut = pShape->GetInOutFlag();
			// 現在登録されている方向で輪郭ｵﾌﾞｼﾞｪｸﾄ生成
			for ( j=0; j<nOutline; j++ ) {
				if ( !pShape->CreateOutlineTempObject(nInOut, &ltOutline, dOffset) ) {
					// ！！失敗！！一時ｵﾌﾞｼﾞｪｸﾄ全削除
					PLIST_FOREACH(CDXFdata* pData, &ltOutline)
						if ( pData )
							delete	pData;
					END_FOREACH
					continue;
				}
				// 加工指示登録
				if ( !ltOutline.IsEmpty() ) {
					pOutline = new CDXFworkingOutline(pShape, &ltOutline, dOffset, DXFWORKFLG_AUTO);
					pShape->AddOutlineData(pOutline, nInOut);
				}
				// 次のﾙｰﾌﾟに備える
				pOutline = NULL;
				dOffset += dOffsetOrg;
				ltOutline.RemoveAll();
			}
			if ( pShapeSrc )
				break;	// 形状指定あれば以降必要なし
		}
	}
	catch ( CMemoryException* e ) {
		if ( pOutline )
			delete	pOutline;
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
	}

	_SetProgressPos(nLoop);

	// ｵﾌｾｯﾄｵﾌﾞｼﾞｪｸﾄ同士の交点ﾁｪｯｸ
	CheckStrictOffset(pLayer, reinterpret_cast<LPVOID>(TRUE));	// 処理にﾙｰﾌﾟが必要
}

//////////////////////////////////////////////////////////////////////
//	ｵﾌｾｯﾄｵﾌﾞｼﾞｪｸﾄ同士の交点を検索し、厳密なｵﾌｾｯﾄを計算する

void CheckStrictOffset(const CLayerData* pLayer, LPVOID pParam)
{
	INT_PTR			i, j, ii, jj, n, nMax = 0;
	const INT_PTR	nLoop = pLayer->GetShapeSize();
	float			dOffset1, dOffset2;
	CRect3F			rc, rcCross;
	CDXFshape*		pShape1;
	CDXFshape*		pShape2;
	CDXFworkingOutline*	pOutline1;
	CDXFworkingOutline*	pOutline2;
	CDXFchain*		pChain1;
	CDXFchain*		pChain2;
	function<CDXFworkingOutline* (CDXFshape*, INT_PTR)>	pfnGetOutline;

	// ｵﾌｾｯﾄｵﾌﾞｼﾞｪｸﾄ分離ｽﾚｯﾄﾞ起動
	SEPARATEOUTLINETHREADPARAM	thSepParam;
	CWinThread*	pSepThread = AfxBeginThread(SeparateOutline_Thread, &thSepParam);
	if ( !pSepThread )
		::NCVC_CriticalErrorMsg(__FILE__, __LINE__);

	_SetProgressRange(nLoop);
	thSepParam.evEnd.SetEvent();

	if ( pParam ) {
		// ｎ番目の輪郭ｵﾌﾞｼﾞｪｸﾄを取得
		pfnGetOutline = &GetOutlineHierarchy;
		// 階層の最大数を取得
		for ( i=0; i<nLoop && IsThread(); i++ ) {
			ii = pLayer->GetShapeData(i)->GetOutlineList()->GetCount();
			if ( nMax < ii )
				nMax = ii;
		}
	}
	else {
		// 最後に登録された輪郭ｵﾌﾞｼﾞｪｸﾄを取得
		pfnGetOutline = &GetOutlineLastObj;
		nMax = 1;
	}

	// ﾒｲﾝﾙｰﾌﾟ
	for ( n=0; n<nMax && IsThread(); n++ ) {		// 階層ﾙｰﾌﾟ
		for ( i=0; i<nLoop && IsThread(); i++ ) {
			pShape1 = pLayer->GetShapeData(i);
			_SetProgressPos(i);
			pOutline1 = pfnGetOutline(pShape1, n);	// 処理に応じた輪郭ｵﾌﾞｼﾞｪｸﾄ取得
			if ( !pOutline1 )
				continue;
			dOffset1 = pOutline1->GetOutlineOffset();
			for ( ii=0; ii<pOutline1->GetOutlineSize() && IsThread(); ii++ ) {
				pChain1 = pOutline1->GetOutlineObject(ii);
				rc = pChain1->GetMaxRect();
				for ( j=i+1; j<nLoop && IsThread(); j++ ) {
					pShape2 = pLayer->GetShapeData(j);
					pOutline2 = pfnGetOutline(pShape2, n);
					if ( !pOutline2 )
						continue;
					dOffset2 = pOutline2->GetOutlineOffset();
					for ( jj=0; jj<pOutline2->GetOutlineSize() && IsThread(); jj++ ) {
						pChain2 = pOutline2->GetOutlineObject(jj);
						if ( rcCross.CrossRect(rc, pChain2->GetMaxRect()) ) {
#ifdef _DEBUG
							printf("ShapeName1=%s No.%d Obj=%d\n", LPCTSTR(pShape1->GetShapeName()),
								ii, pChain1->GetSize());
							printf("ShapeName2=%s No.%d Obj=%d\n", LPCTSTR(pShape2->GetShapeName()),
								jj, pChain2->GetSize());
#endif
							// 内外判定のためのﾌﾗｸﾞ設定
							pChain1->ClearSideFlg();
							pChain1->SetChainFlag( pChain1->GetChainFlag() | 
								(pShape1->GetShapeFlag()&(DXFMAPFLG_INSIDE|DXFMAPFLG_OUTSIDE)) );
							pChain2->ClearSideFlg();
							pChain2->SetChainFlag( pChain2->GetChainFlag() | 
								(pShape2->GetShapeFlag()&(DXFMAPFLG_INSIDE|DXFMAPFLG_OUTSIDE)) );
							// ｵﾌｾｯﾄｵﾌﾞｼﾞｪｸﾄの併合処理
							if ( CheckOffsetIntersection(pChain1, pChain2, FALSE) ) {
								pOutline1->SetMergeHandle(pShape2->GetShapeHandle());
								pOutline2->SetMergeHandle(pShape1->GetShapeHandle());
							}
						}
					}
				}
			}
			// 最終ﾁｪｯｸ
//			CheckCircleIntersection(pLayer, pShape1, pOutline1);
			// 分離ｽﾚｯﾄﾞ開始
			thSepParam.evEnd.Lock();
			thSepParam.evEnd.ResetEvent();
			thSepParam.pOutline = pOutline1;
			thSepParam.evStart.SetEvent();
		}
	}

	// ｽﾚｯﾄﾞ終了指示
	thSepParam.evEnd.Lock();
	thSepParam.evEnd.ResetEvent();
	thSepParam.bThread = FALSE;
	thSepParam.evStart.SetEvent();
	WaitForSingleObject(pSepThread->m_hThread, INFINITE);

	_SetProgressPos(nLoop);

#ifdef _DEBUG
	// 分割数の確認
	extern	LPCTSTR	gg_szCat;		// ", "
	POSITION	dbgPos;
	CPointF		dbgPts, dbgPte;
	BOOL		dbgConnect;
	CDXFdata*	dbgData;
	CString		dbgMsg;
	for ( i=0; i<nLoop; i++ ) {
		pShape1 = pLayer->GetShapeData(i);
		pOutline1  = pShape1->GetOutlineLastObj();
		if ( !pOutline1 )
			continue;
		printf("Name=%s Sep=%d Merge=%d\n",
			LPCTSTR(pShape1->GetShapeName()),
			pOutline1->GetOutlineSize(), pOutline1->GetMergeHandleSize());
		// 併合輪郭のﾃﾞｰﾀﾁｪｯｸ
		for ( j=0; j<pOutline1->GetOutlineSize(); j++ ) {
			dbgConnect = TRUE;
			pChain1 = pOutline1->GetOutlineObject(j);
			dbgPos = pChain1->GetHeadPosition();
			if ( !dbgPos )
				continue;
			dbgPte = pChain1->GetNext(dbgPos)->GetNativePoint(1);
			while ( dbgPos ) {
				dbgData = pChain1->GetNext(dbgPos);
				dbgPts = dbgData->GetNativePoint(0);
				if ( sqrt(GAPCALC(dbgPts-dbgPte)) >= NCMIN ) {
					dbgConnect = FALSE;
					break;
				}
				dbgPte = dbgData->GetNativePoint(1);
			}
			if ( !dbgConnect ) {
				printf("---> No.%d Connect Error!\n", j);
				PLIST_FOREACH(dbgData, pChain1)
					dbgPts = dbgData->GetNativePoint(0);
					dbgPte = dbgData->GetNativePoint(1);
					printf("(%.3f, %.3f)-(%.3f, %.3f)\n",
						dbgPts.x, dbgPts.y, dbgPte.x, dbgPte.y);
				END_FOREACH
			}
			else if ( pChain1->GetChainFlag() & DXFMAPFLG_SEPARATE ) {
				dbgPts = pChain1->GetHead()->GetNativePoint(0);
				dbgPte = pChain1->GetTail()->GetNativePoint(1);
				if ( sqrt(GAPCALC(dbgPts-dbgPte)) < NCMIN )
					printf("---> No.%d Loop Outline ? size=%d\n", j, pChain1->GetSize());
			}
		}
		// 併合状況
		if ( pOutline1->GetMergeHandleSize() > 0 ) {
			for ( j=0; j<pOutline1->GetMergeHandleSize(); j++ ) {
				if ( !dbgMsg.IsEmpty() )
					dbgMsg += gg_szCat;
				dbgMsg += pOutline1->GetMergeHandle(j);
			}
			printf("---> %s\n", LPCTSTR(dbgMsg));
			dbgMsg.Empty();
		}
	}
#endif
}

void CheckStrictOffset_forScan(const CLayerData* pLayer)
{
	INT_PTR			i, ii, j, jj;
	const INT_PTR	nLoop = pLayer->GetShapeSize();
	float		dOffset1, dOffset2;
	CRect3F		rc1, rc2, rcCross;
	CDXFshape*	pShape1;
	CDXFshape*	pShape2;
	CDXFworkingOutline*	pOutline1;
	CDXFworkingOutline*	pOutline2;
	CDXFchain*	pChain1;
	CDXFchain*	pChain2;

	// ｵﾌｾｯﾄｵﾌﾞｼﾞｪｸﾄ分離ｽﾚｯﾄﾞ起動
	SEPARATEOUTLINETHREADPARAM	thSepParam;
	CWinThread*	pSepThread = AfxBeginThread(SeparateOutline_Thread, &thSepParam);
	if ( !pSepThread )
		::NCVC_CriticalErrorMsg(__FILE__, __LINE__);

	_SetProgressRange(nLoop);
	thSepParam.evEnd.SetEvent();

	// ﾒｲﾝﾙｰﾌﾟ
	for ( i=0; i<nLoop && IsThread(); i++ ) {
		pShape1 = pLayer->GetShapeData(i);
		pOutline1 = pShape1->GetOutlineLastObj();
		_SetProgressPos(i);
		if ( !pOutline1 || !(pShape1->GetShapeFlag()&DXFMAPFLG_INSIDE) )
			continue;
		dOffset1 = pOutline1->GetOutlineOffset();
		for ( ii=0; ii<pOutline1->GetOutlineSize() && IsThread(); ii++ ) {
			pChain1 = pOutline1->GetOutlineObject(ii);
			rc1 = pChain1->GetMaxRect();
			for ( j=i+1; j<nLoop && IsThread(); j++ ) {
				pShape2 = pLayer->GetShapeData(j);
				pOutline2 = pShape2->GetOutlineLastObj();
				if ( !pOutline2 || !(pShape2->GetShapeFlag()&DXFMAPFLG_OUTSIDE) )
					continue;
				dOffset2 = pOutline2->GetOutlineOffset();
				for ( jj=0; jj<pOutline2->GetOutlineSize() && IsThread(); jj++ ) {
					pChain2 = pOutline2->GetOutlineObject(jj);
					rc2 = pChain2->GetMaxRect();
					if ( rc2.RectInRect(rc1) ) {
						// InsideのﾃﾞｰﾀがOutsideの内側にあるので削除
						SetAllExcludeData(pChain1);
					}
					else if ( rcCross.CrossRect(rc2, rc1) ) {	// 矩形の交差も含む
#ifdef _DEBUG
						printf("ShapeName1=%s No.%d Obj=%d\n", LPCTSTR(pShape1->GetShapeName()),
							ii, pChain1->GetSize());
						printf("ShapeName2=%s No.%d Obj=%d\n", LPCTSTR(pShape2->GetShapeName()),
							jj, pChain2->GetSize());
#endif
						pChain1->ClearSideFlg();
						pChain1->SetChainFlag( pChain1->GetChainFlag() | 
							(pShape1->GetShapeFlag()&(DXFMAPFLG_INSIDE|DXFMAPFLG_OUTSIDE)) );
						pChain2->ClearSideFlg();
						pChain2->SetChainFlag( pChain2->GetChainFlag() | 
							(pShape2->GetShapeFlag()&(DXFMAPFLG_INSIDE|DXFMAPFLG_OUTSIDE)) );
						// INSIDE側だけ交点処理
						if ( CheckOffsetIntersection(pChain1, pChain2, TRUE) )
							pOutline1->SetMergeHandle(pShape2->GetShapeHandle());	// たぶん使わないﾃﾞｰﾀ
					}
				}
			}
		}
		// 分離ｽﾚｯﾄﾞ開始
		thSepParam.evEnd.Lock();
		thSepParam.evEnd.ResetEvent();
		thSepParam.pOutline = pOutline1;
		thSepParam.evStart.SetEvent();
	}

	// ｽﾚｯﾄﾞ終了指示
	thSepParam.evEnd.Lock();
	thSepParam.evEnd.ResetEvent();
	thSepParam.bThread = FALSE;
	thSepParam.evStart.SetEvent();
	WaitForSingleObject(pSepThread->m_hThread, INFINITE);

	_SetProgressPos(nLoop);

#ifdef _DEBUG
	// 分割数の確認
	COutlineList*	dbgOutlineList;
	for ( i=0; i<nLoop; i++ ) {
		pShape1 = pLayer->GetShapeData(i);
		if ( !pShape1->IsOutlineList() )
			continue;
		dbgOutlineList = pShape1->GetOutlineList();
		printf("Name=%s Outline=%d\n",
			LPCTSTR(pShape1->GetShapeName()), dbgOutlineList->GetCount());
		ii=0;
		PLIST_FOREACH(pOutline1, dbgOutlineList)
			printf("No.%d Sep=%d\n", ii++, pOutline1->GetOutlineSize());
		END_FOREACH
	}
#endif
}

//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//	自動輪郭加工指示
//		他のｵﾌﾞｼﾞｪｸﾄに含まれる最小内周ｵﾌﾞｼﾞｪｸﾄに内側の加工指示，
//		それ以外は外側の加工指示
BOOL SelectOutline(const CLayerData* pLayer)
{
	INT_PTR			i, j;
	const INT_PTR	nLoop = pLayer->GetShapeSize();
	BOOL		bResult = FALSE;
	CDXFshape*	pShape;
	CDXFshape*	pShapeTmp;
	CRectF		rcBase;

	_SetProgressRange(nLoop);
	const_cast<CLayerData*>(pLayer)->AscendingShapeSort();	// 面積で昇順並べ替え

	// 占有矩形の内外判定
	for ( i=0; i<nLoop && IsThread(); i++ ) {
		pShape = pLayer->GetShapeData(i);
		// ﾌﾟﾛｸﾞﾚｽﾊﾞｰ
		_SetProgressPos(i);	// 件数少ないので１件ずつ更新
		// 自動処理対象か否か(CDXFchain* だけを対象とする)
		if ( pShape->GetShapeType()!=DXFSHAPETYPE_CHAIN || pShape->GetShapeFlag()&DXFMAPFLG_CANNOTAUTOWORKING )
			continue;
		// rcBaseよりも大きな面積を持つｵﾌﾞｼﾞｪｸﾄ(i+1)がrcBaseを含むかどうかの判定
		rcBase = pShape->GetMaxRect();
		for ( j=i+1; j<nLoop && IsThread(); j++ ) {
			pShapeTmp = pLayer->GetShapeData(j);
			if ( pShapeTmp->GetShapeType() != DXFSHAPETYPE_CHAIN )
				continue;
			if ( pShapeTmp->GetMaxRect().RectInRect(rcBase) )
				break;	// 以後の検査は必要なし
		}
		pShape->SetShapeFlag(j<nLoop ? DXFMAPFLG_INSIDE : DXFMAPFLG_OUTSIDE);
		bResult = TRUE;		// 処理済み
	}

	_SetProgressPos(nLoop);
	return bResult;
}

//////////////////////////////////////////////////////////////////////
//	自動ﾎﾟｹｯﾄ加工指示
//		最大外周と最小内周に内側の加工指示
BOOL SelectPocket(const CLayerData* pLayer)
{
	INT_PTR			i, j;
	const INT_PTR	nLoop = pLayer->GetShapeSize();
	BOOL		bResult = FALSE, bInRect;
	CDXFshape*	pShape;
	CDXFshape*	pShapeTmp;
	CRectF		rcBase;

	_SetProgressRange(nLoop);
	const_cast<CLayerData*>(pLayer)->DescendingShapeSort();	// 面積で降順並べ替え

	// 外周判定
	for ( i=0; i<nLoop && IsThread(); i++ ) {
		pShape = pLayer->GetShapeData(i);
		// ﾌﾟﾛｸﾞﾚｽﾊﾞｰ
		_SetProgressPos(i);
		// 自動処理対象か否か(CDXFchain* だけを対象とする)
		if ( pShape->GetShapeType()!=DXFSHAPETYPE_CHAIN ||
				pShape->GetShapeFlag()&DXFMAPFLG_CANNOTAUTOWORKING ||
				pShape->IsSideFlg() )
			continue;
		bInRect = FALSE;
		// rcBaseがｵﾌﾞｼﾞｪｸﾄを含んでいるかどうかの判定
		rcBase = pShape->GetMaxRect();
		for ( j=i+1; j<nLoop && IsThread(); j++ ) {
			pShapeTmp = pLayer->GetShapeData(j);
			if ( pShapeTmp->GetShapeType()!=DXFSHAPETYPE_CHAIN || pShapeTmp->IsSideFlg() )
				continue;
			if ( rcBase.RectInRect(pShapeTmp->GetMaxRect()) ) {
				// 再帰呼び出しによる階層ﾁｪｯｸ
				SelectPocket_Reflex(pLayer, i, 0);
				bInRect = TRUE;
			}
		}
		if ( !bInRect ) {
			// 単独
			pShape->SetShapeFlag(DXFMAPFLG_INSIDE);
		}
	}

	_SetProgressPos(nLoop);
	return bResult;
}

void SelectPocket_Reflex(const CLayerData* pLayer, INT_PTR i, int n)
{
	CDXFshape*	pShape = pLayer->GetShapeData(i++);	// i++ で次のﾃﾞｰﾀから
	CRectF		rcBase(pShape->GetMaxRect());

	// 当該集合の判定は階層の数値から
	pShape->SetShapeFlag( n&0x01 ? DXFMAPFLG_OUTSIDE : DXFMAPFLG_INSIDE);

	for ( ; i<pLayer->GetShapeSize() && IsThread(); i++ ) {
		pShape = pLayer->GetShapeData(i);
		if ( pShape->GetShapeType()!=DXFSHAPETYPE_CHAIN || pShape->IsSideFlg() )
			continue;
		if ( rcBase.RectInRect(pShape->GetMaxRect()) ) {
			// 再帰
			SelectPocket_Reflex(pLayer, i, n+1);
		}
	}
}

//////////////////////////////////////////////////////////////////////
//	内外判定から加工指示の生成

void CreateAutoWorking(const CLayerData* pLayer, float dOffset)
{
	INT_PTR			i;
	const INT_PTR	nLoop = pLayer->GetShapeSize();
	size_t		n, j;
	float		dArea[2];
	DWORD		dwError;
	CRect3F		rcMax;
	CDXFchain	ltOutline[2];
	CDXFdata*	pData;
	CDXFshape*	pShape;
	CDXFworkingOutline*	pWork;

	_SetProgressRange(nLoop);

	try {
		for ( i=0; i<nLoop && IsThread(); i++ ) {
			pWork = NULL;
			pShape = pLayer->GetShapeData(i);
			// ﾌﾟﾛｸﾞﾚｽﾊﾞｰ
			_SetProgressPos(i);
			// 処理対象ﾁｪｯｸ
			if ( !pShape->IsSideFlg() )
				continue;
#ifdef _DEBUG
			printf("ShapeName=%s\n", LPCTSTR(pShape->GetShapeName()));
			CRectF	rcDbg(pShape->GetMaxRect());
			printf("(%f, %f)-(%f, %f)\n", rcDbg.left, rcDbg.top, rcDbg.right, rcDbg.bottom);
#endif
			// 輪郭一時ｵﾌﾞｼﾞｪｸﾄの生成
			dwError = 0;
			for ( j=0; j<SIZEOF(ltOutline) && IsThread(); j++ ) {
				if ( pShape->CreateOutlineTempObject((BOOL)j, &ltOutline[j], dOffset) ) {
					// 矩形領域の大きさ計算
					rcMax = ltOutline[j].GetMaxRect();
				}
				else {
					// 元集合の矩形領域をｾｯﾄ
					rcMax = pShape->GetMaxRect();
					dwError |= ( 1 << j );
				}
#ifdef _DEBUG
				printf("Outline (%f, %f)-(%f, %f)\n", rcMax.left, rcMax.top, rcMax.right, rcMax.bottom);
#endif
				dArea[j] = rcMax.Width() * rcMax.Height();
			}
			// 内外を矩形の大きさで決定
			if ( pShape->GetShapeFlag() & DXFMAPFLG_INSIDE )
				j = dArea[0] > dArea[1] ? 1 : 0;	// 小さい方を採用
			else if ( pShape->GetShapeFlag() & DXFMAPFLG_OUTSIDE )
				j = dArea[0] > dArea[1] ? 0 : 1;	// 大きい方を採用
			else
				j = -1;
			switch ( dwError ) {
			case 1:		// １回目ｴﾗｰ
				if ( j == 0 )
					j = -1;
				break;
			case 2:		// ２回目ｴﾗｰ
				if ( j == 1 )
					j = -1;
				break;
			case 3:		// 両方ｴﾗｰ
				j = -1;
				break;
			}
			// 加工指示登録
			if ( j >= 0 ) {
				if ( !ltOutline[j].IsEmpty() ) {
					pWork = new CDXFworkingOutline(pShape, &ltOutline[j], dOffset, DXFWORKFLG_AUTO);
					pShape->AddOutlineData(pWork, (int)j);
#ifdef _DEBUG
					printf("Select OutLine = %d\n", j);
#endif
				}
				// Select分はCDXFworkingOutlineのﾃﾞｽﾄﾗｸﾀにてdelete
				n = 1 - j;	// 1->0, 0->1
				PLIST_FOREACH(pData, &ltOutline[n])
					if ( pData )
						delete	pData;
				END_FOREACH
			}
			else {
				// ！！失敗！！一時ｵﾌﾞｼﾞｪｸﾄ全削除
				for ( n=0; n<SIZEOF(ltOutline); n++ ) {
					PLIST_FOREACH(pData, &ltOutline[n])
						if ( pData )
							delete	pData;
					END_FOREACH
				}
			}
			// 次のﾙｰﾌﾟに備え、矩形の初期化
			for ( n=0; n<SIZEOF(ltOutline); n++ )
				ltOutline[n].RemoveAll();
		}
	}
	catch ( CMemoryException* e ) {
		if ( pWork )
			delete	pWork;
		for ( n=0; n<SIZEOF(ltOutline); n++ ) {
			PLIST_FOREACH(pData, &ltOutline[n])
				if ( pData )
					delete	pData;
			END_FOREACH
		}
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
	}

	_SetProgressPos(nLoop);
}

//////////////////////////////////////////////////////////////////////
//	最大矩形の走査線を生成

BOOL CreateScanLine(CDXFshape* pShape, LPAUTOWORKINGDATA pAuto)
{
	CDXFchain	ltScan;
	CDXFworkingOutline*	pWork;

	if ( pAuto->bCircleScroll ) {
		// 図形集合が単一の円ﾃﾞｰﾀならｽｸﾛｰﾙ切削用ﾃﾞｰﾀの生成
		CDXFchain*	pChain = pShape->GetShapeChain();
		if ( pChain && pChain->GetCount()==1 && pChain->GetHead()->GetType()==DXFCIRCLEDATA )
			pShape->CreateScanLine_ScrollCircle(&ltScan);
	}

	if ( ltScan.IsEmpty() ) {
		switch ( pAuto->nScanLine ) {
		case 1:		// Ｘ方向
			pShape->CreateScanLine_X(&ltScan);
			break;
		case 2:		// Ｙ方向
			pShape->CreateScanLine_Y(&ltScan);
			break;
		default:	// なし(輪郭)
			pShape->CreateScanLine_Outline(&ltScan);
			break;
		}
	}

	if ( !ltScan.IsEmpty() ) {
		pWork = new CDXFworkingOutline(pShape, &ltScan, pShape->GetOffset(),
						DXFWORKFLG_AUTO|DXFWORKFLG_SCAN);
		pShape->AddOutlineData(pWork, 0);
	}

	return TRUE;
}

//////////////////////////////////////////////////////////////////////

BOOL CheckOffsetIntersection
	(CDXFchain* pChain1, CDXFchain* pChain2, BOOL bScan)
{
	POSITION	pos1, pos2, pos;
	int			i, j, n;
	CPointF		pt[4], pts;
	CDXFdata*	pData1;
	CDXFdata*	pData2;

	// 交点情報一時格納
	struct INTER_INFO {
		POSITION	pos;		// 該当ｵﾌﾞｼﾞｪｸﾄのPOSITION
		CPointF		pt;			// 交点座標
		int			nCnt;		// 次の交点情報までのｵﾌﾞｼﾞｪｸﾄ数
		float		dGap;		// 始点との距離(並べ替え用)
		CDXFchain*	pChain;		// 相手集合
		bool operator < (const INTER_INFO& src) const {	// sort用
			return dGap < src.dGap;
		}
	};
	INTER_INFO			info;
	vector<INTER_INFO>	vTemp, vInfo[2];
	vector<INTER_INFO>::iterator	it;

	// 座標の遅延更新用構造体
	struct	DELAYUPDATE {
		CDXFdata*	pData;		// 更新対象
		CPointF		pt;			// 更新座標
		int			n;			// 始点か終点か
	};
	DELAYUPDATE			d;
	vector<DELAYUPDATE>	v;

	// ｵﾌｾｯﾄｵﾌﾞｼﾞｪｸﾄ同士の交点ﾁｪｯｸ
	for ( i=0; i<SIZEOF(vInfo) && IsThread(); i++ ) {
		for ( pos1=pChain1->GetHeadPosition(); (pos=pos1) && IsThread(); ) {
			pData1 = pChain1->GetNext(pos1);
			if ( !pData1 )
				continue;
			pts = pData1->GetNativePoint(0);
			for ( pos2=pChain2->GetHeadPosition(); pos2 && IsThread(); ) {
				pData2 = pChain2->GetNext(pos2);
				if ( !pData2 )
					continue;
				n = pData1->GetIntersectionPoint(pData2, pt);
				for ( j=0; j<n && IsThread(); j++ ) {	// 交点数分生成
					info.pos	= pos;
					info.pt		= pt[j];
					info.nCnt	= 0;
					info.dGap	= GAPCALC(pt[j] - pts);
					info.pChain	= pChain2;
					vTemp.push_back(info);
				}
			}
			// １つのｵﾌﾞｼﾞｪｸﾄに複数の交点がある場合は
			// 始点からの距離で並べ替え
			if ( vTemp.size() > 1 ) {
//				std::sort(vTemp.begin(), vTemp.end());
				boost::sort(vTemp);	// boost/range/algorithm.hpp
			}
			vInfo[i].insert(vInfo[i].end(), vTemp.begin(), vTemp.end());
			vTemp.clear();
		}
		// 対象集合の切り替え
		if ( bScan )
			break;		// 走査線の場合は不要
		swap(pChain1, pChain2);	// 処理方法は同じ
	}

	// 基本的に交点数は偶数のハズやけど、
	// 既に行われた交点除去処理で奇数の場合もあり得る
	if ( vInfo[0].size()<=0 || (!bScan && vInfo[0].size()!=vInfo[1].size()) )
		return FALSE;

	// 交点記録から削除範囲の確認(以降交点が１つだけのときは処理しない)
	if ( vInfo[0].size() > 1 ) {
		for ( i=0; i<SIZEOF(vInfo) && IsThread(); i++ ) {
			for ( it=vInfo[i].begin(); it!=vInfo[i].end() && IsThread(); ++it ) {
				pos1 = it->pos;
				pos2 = boost::next(it)!=vInfo[i].end() ? boost::next(it)->pos : vInfo[i][0].pos;
				ASSERT(pos1);
				ASSERT(pos2);
				for ( n=0; pos1!=pos2 && IsThread(); ) {
					pData1 = pChain1->GetNext(pos1);
					if ( pData1 )
						n++;
					if ( !pos1 )
						pos1 = pChain1->GetHeadPosition();
				}
				it->nCnt = n;	// 次の交点までのｵﾌﾞｼﾞｪｸﾄ数
			}
			// 対象集合の切り替え
			if ( bScan )
				break;
			swap(pChain1, pChain2);
		}
		if ( !IsThread() )
			return FALSE;

		// 削除処理(以降 IsThread() は不要)
		for ( i=0; i<SIZEOF(vInfo); i++ ) {
			// 削除開始位置の決定(相手集合への内外判定)
			pos = vInfo[i][0].pos;
			ASSERT(pos);
			pts = pChain1->GetNext(pos)->GetNativePoint(1);		// 交点ｵﾌﾞｼﾞｪｸﾄの終点が
			if ( vInfo[i][0].nCnt == 0 ) {
				if ( pChain2->IsPointInPolygon(pts) ) {		// 相手集合の内側にある
					// １つのｵﾌﾞｼﾞｪｸﾄで貫いて相手集合に入る交点線なので、
					// 次の交点情報から開始
					vInfo[i].push_back(vInfo[i][0]);	// 先頭要素を末尾に追加して
					vInfo[i].erase(vInfo[i].begin());	// 先頭要素を削除
				}
			}
			else {
				if ( !pChain2->IsPointInPolygon(pts) ) {	// 相手集合の外側にある
					// 相手集合から出ていくような交点線なので、
					// 次の交点情報から開始
					vInfo[i].push_back(vInfo[i][0]);
					vInfo[i].erase(vInfo[i].begin());
				}
			}
			// 交点情報２個飛ばしで、その間のｵﾌﾞｼﾞｪｸﾄを消去
			for ( it=vInfo[i].begin(); it!=vInfo[i].end(); it+=2 ) {
				if ( boost::next(it) == vInfo[i].end() )
					break;	// 奇数分は除く
				pos1 = it->pos;
				pos2 = boost::next(it)->pos;
				ASSERT(pos1);
				ASSERT(pos2);
				// pos1 が示すｵﾌﾞｼﾞｪｸﾄの終点を更新
				pData1 = pChain1->GetNext(pos1);
				if ( !pos1 )
					pos1 = pChain1->GetHeadPosition();
				ASSERT( pData1 );
				if ( !pData1->IsStartEqEnd() ) {
					pt[0] = pData1->GetNativePoint(1);			// 変更前の終点
					d.pData	= pData1;
					d.pt	= it->pt;
					d.n		= 1;	// 終点変更ﾏｰｸ
					v.push_back(d);
				}
				// pos1 から pos2 までｵﾌﾞｼﾞｪｸﾄ削除
				if ( it->nCnt > 0 ) {
					while ( (pos=pos1) != pos2 ) {
						pData1 = pChain1->GetNext(pos1);
						if ( pData1 )
							pData1->SetDxfFlg(DXFFLG_OFFSET_EXCLUDE);
						if ( !pos1 )
							pos1 = pChain1->GetHeadPosition();
					}
					pData1 = pChain1->GetAt(pos1);
					d.pData	= pData1;
					d.pt	= boost::next(it)->pt;
					d.n		= 0;	// 始点変更ﾏｰｸ
					v.push_back(d);
					pChain1->InsertBefore(pos1, (CDXFdata *)NULL);		// 分離ﾏｰｸ
				}
				else {
					// 同一ｵﾌﾞｼﾞｪｸﾄに複数の交点
					if ( pData1->IsStartEqEnd() ) {
						// 円・楕円の場合は、円・楕円孤に変身
						pData2 = pData1;	// delete用
						pData1 = ChangeCircleToArc(static_cast<CDXFcircle *>(pData1),
							it->pChain, it->pt, boost::next(it)->pt);
						if ( pData1 ) {
							pData2->SetDxfFlg(DXFFLG_OFFSET_EXCLUDE);
							pChain1->AddTail(pData1);
							pChain1->SetChainFlag(DXFMAPFLG_SEPARATE);	// SeparateModify() では立てられないﾌﾗｸﾞ
						}
					}
					else {
						pData1 = CreateDxfOffsetObject(pData1, boost::next(it)->pt, pt[0]);	// 次の交点と変更前終点(CDXFshape)
						pos = pChain1->InsertBefore(pos1, pData1);		// ｵﾌﾞｼﾞｪｸﾄ挿入
						pChain1->InsertBefore(pos, (CDXFdata *)NULL);	// 分離ﾏｰｸ
					}
				}
			}
			// 対象集合の切り替え
			if ( bScan )
				break;
			swap(pChain1,  pChain2);
		}
	}

	// 余った奇数交点(交点１つだけも含む)に対する処理
	if ( vInfo[0].size() & 0x01 ) {
		for ( i=0; i<SIZEOF(vInfo); i++ ) {
			info = vInfo[i].back();
			pos  = info.pos;
			ASSERT(pos);
			pData1  = pChain1->GetNext(pos);
			d.pData	= pData1;
			d.pt	= info.pt;
			pts = pData1->GetNativePoint(0);
			pt[0] = (info.pt - pts) / 2.0 + pts;	// ｵﾌﾞｼﾞｪｸﾄの始点と交点の中点が
			if ( pChain2->IsPointInPolygon(pt[0]) ) {		// 相手集合の内側にある
				d.n		= 0;
				v.push_back(d);
			}
			else {
				d.n		= 1;
				v.push_back(d);
				// 以降、終点が引っかからないところまでｵﾌﾞｼﾞｪｸﾄ削除
				if ( !pos )
					pos = pChain1->GetHeadPosition();
				while ( TRUE ) {
					pData1 = pChain1->GetNext(pos);
					if ( !pos )
						pos = pChain1->GetHeadPosition();
					if ( pData1 ) {
						if ( pData1->GetDxfFlg() & DXFFLG_OFFSET_EXCLUDE )
							break;	// 以降の処理は必要なし
						if ( pChain2->IsPointInPolygon(pData1->GetNativePoint(1)) )
							pData1->SetDxfFlg(DXFFLG_OFFSET_EXCLUDE);
						else
							break;
					}
				}
			}

			// 対象集合の切り替え
			if ( bScan )
				break;
			swap(pChain1,  pChain2);
		}
	}

	// 遅延ﾃﾞｰﾀの座標更新
	BOOST_FOREACH(DELAYUPDATE& dd, v) {
		dd.pData->SetNativePoint(dd.n, dd.pt);
	}

#ifdef _DEBUG
	printf("CheckOffsetIntersection()\n");
	optional<CPointF>	ptDbgE;
	for ( i=0; i<SIZEOF(vInfo); i++ ) {
		ptDbgE.reset();
		PLIST_FOREACH(pData1, pChain1)
			if ( pData1 ) {
				pt[0] = pData1->GetNativePoint(0);
				pt[1] = pData1->GetNativePoint(1);
				if ( pData1->GetDxfFlg() & DXFFLG_OFFSET_EXCLUDE ) {
					printf("pt=(%.3f, %.3f) - (%.3f, %.3f) DEL\n",
						pt[0].x, pt[0].y, pt[1].x, pt[1].y);
				}
				else {
					printf("pt=(%.3f, %.3f) - (%.3f, %.3f) %s\n",
						pt[0].x, pt[0].y, pt[1].x, pt[1].y,
						ptDbgE && sqrt(GAPCALC(*ptDbgE-pt[0]))>=NCMIN ? "X" : " ");
					ptDbgE = pt[1];
				}
			}
			else {
				printf("null\n");
				ptDbgE.reset();
			}
		END_FOREACH
		printf("---\n");
		if ( bScan )
			break;
		swap(pChain1, pChain2);
	}
#endif

	return IsThread();
}

void CheckCircleIntersection
	(const CLayerData* pLayer, const CDXFshape* pShapeSrc, const CDXFworkingOutline* pOutline)
{
	INT_PTR		i, j,
				nShapeLoop = pLayer->GetShapeSize(),
				nOutlineLoop = pOutline->GetOutlineSize();
	float		dOffset = pOutline->GetOutlineOffset();
	CPointF		pts, pte;
	CRectF		rc(pOutline->GetMaxRect()), rcCross;
	CDXFshape*	pShape;

	//	ｵﾌｾｯﾄｵﾌﾞｼﾞｪｸﾄの最終ﾁｪｯｸ
	for ( i=0; i<nShapeLoop && IsThread(); i++ ) {
		pShape = pLayer->GetShapeData(i);
		if ( pShape==pShapeSrc || !pShape->IsOutlineList() )
			continue;
		if ( rcCross.CrossRect(rc, pShape->GetMaxRect()) ) {
			for ( j=0; j<nOutlineLoop && IsThread(); j++ ) {
				PLIST_FOREACH(CDXFdata* pData, pOutline->GetOutlineObject(j));
					if ( IsThread() )
						break;
					if ( pData && !(pData->GetDxfFlg()&DXFFLG_OFFSET_EXCLUDE) ) {
						pts = pData->GetNativePoint(0);
						pte = pData->GetNativePoint(1);
						if ( pShape->CheckIntersectionCircle(pts, dOffset) ) {
							pData->SetDxfFlg(DXFFLG_OFFSET_EXCLUDE);
						}
						else if ( pShape->CheckIntersectionCircle(pte, dOffset) ) {
							pData->SetDxfFlg(DXFFLG_OFFSET_EXCLUDE);
						}
					}
				END_FOREACH
			}
		}
	}
}

CDXFdata* ChangeCircleToArc
	(const CDXFcircle* pDataSrc, const CDXFchain* pChain, CPointF& pts, CPointF& pte)
{
	ENDXFTYPE	enType = pDataSrc->GetType();
	if ( enType!=DXFCIRCLEDATA && enType!=DXFELLIPSEDATA )
		return NULL;

	CDXFdata*	pData = NULL;
	CPointF		ptc(pDataSrc->GetCenter()), pt;
	float		sq, eq, q, r = pDataSrc->GetR();

	// 回転方向（切り取り円弧）の決定
	if ( (sq=ptc.arctan(pts)) < 0.0f )
		sq += PI2;
	if ( (eq=ptc.arctan(pte)) < 0.0f )
		eq += PI2;
	while ( sq > eq )
		eq += PI2;		// sq < eq => 基本の反時計回りに角度設定
	q = (eq - sq) / 2.0f + sq;	// sqとeqのあいだ
	pt.x = r * cos(q);		pt.y = r * sin(q);
	pt += ptc;
	// 相手が内側処理か外側処理かで切り取る円弧を決定
	if ( pChain->GetChainFlag() & DXFMAPFLG_INSIDE ) {
		if ( !pChain->GetMaxRect().PtInRect(pt) ) {
			// 角度の真ん中の座標が相手の集合矩形外なら
			swap(sq, eq);		// 角度を入れ替え反対側の円弧で生成
			swap(pts, pte);		// 楕円孤用
		}
	}
	else {
		if ( pChain->GetMaxRect().PtInRect(pt) ) {
			// 角度の真ん中の座標が相手の集合矩形内なら
			swap(sq, eq);		// 角度を入れ替え反対側の円弧で生成
			swap(pts, pte);		// 楕円孤用
		}
	}

	// 円弧 または 楕円孤の生成
	if ( enType == DXFCIRCLEDATA ) {
		DXFAARGV	dxfArc;
		dxfArc.pLayer = pDataSrc->GetParentLayer();
		dxfArc.c = ptc;
		dxfArc.r = r;
		dxfArc.sq = DEG(sq);	// ARC登録はDEG
		dxfArc.eq = DEG(eq);
		pData = new CDXFarc(&dxfArc);
	}
	else {
		const CDXFellipse* pEllipse = static_cast<const CDXFellipse *>(pDataSrc);
		DXFEARGV	dxfEllipse;
		dxfEllipse.pLayer = pEllipse->GetParentLayer();
		dxfEllipse.c = ptc;
		dxfEllipse.l = pEllipse->GetLongPoint();
		dxfEllipse.s = pEllipse->GetShortMagni();
		dxfEllipse.bRound = TRUE;
		// 楕円用に角度の再計算(DXFshape.cpp CreateDxfOffsetObject() 参照)
		pts.RoundPoint(-pEllipse->GetLean());
		pte.RoundPoint(-pEllipse->GetLean());
		float	l = pEllipse->GetLongLength();
		q = pts.x / l;
		if ( q < -1.0f || 1.0f < q )
			q = copysign(1.0f, q);	// -1.0 or 1.0
		dxfEllipse.sq = copysign(acos(q), pts.y);
		if ( dxfEllipse.sq < 0.0f )
			dxfEllipse.sq += PI2;
		q = pte.x / l;
		if ( q < -1.0f || 1.0f < q )
			q = copysign(1.0f, q);
		dxfEllipse.eq = copysign(acos(q), pte.y);
		if ( dxfEllipse.eq < 0.0f )
			dxfEllipse.eq += PI2;
		pData = new CDXFellipse(&dxfEllipse);
	}

	return pData;
}

void SetAllExcludeData(CDXFchain* pChain)
{
	PLIST_FOREACH(CDXFdata* pData, pChain)
		if ( pData )
			pData->SetDxfFlg(DXFFLG_OFFSET_EXCLUDE);
	END_FOREACH
}

CDXFworkingOutline*	GetOutlineHierarchy(CDXFshape* pShape, INT_PTR n)
{
	// ｎ番目の輪郭ｵﾌﾞｼﾞｪｸﾄを取得
	COutlineList*	pOutlineList = pShape->GetOutlineList();
	CDXFworkingOutline*	pOutline = NULL;
	INT_PTR		i;
	POSITION	pos;
	// 0回実行でbreakの可能性 -> PLIST_FOREACH 使えない
	for ( i=0, pos=pOutlineList->GetHeadPosition(); pos && i<=n; i++ )
		pOutline = pOutlineList->GetNext(pos);

	return pOutline;
}

CDXFworkingOutline*	GetOutlineLastObj(CDXFshape* pShape, INT_PTR)
{
	return pShape->GetOutlineLastObj();
}

//////////////////////////////////////////////////////////////////////
//	ｵﾌｾｯﾄｵﾌﾞｼﾞｪｸﾄの分離ﾏｰｸ(NULL)で分離

UINT SeparateOutline_Thread(LPVOID pVoid)
{
	LPSEPARATEOUTLINETHREADPARAM	pParam = reinterpret_cast<LPSEPARATEOUTLINETHREADPARAM>(pVoid);

	while ( TRUE ) {
		// 上位の実行許可が下りるまでｳｪｲﾄ
		pParam->evStart.Lock();
		pParam->evStart.ResetEvent();
		// 継続ﾌﾗｸﾞﾁｪｯｸ
		if ( !pParam->bThread )
			break;
		// 分割処理開始
		pParam->pOutline->SeparateModify();
		// 処理終了
		pParam->evEnd.SetEvent();
	}

	return 0;
}
