// TH_AutoWorkingSet.cpp
// 自動加工指示
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"	// DXFView.h
#include "DXFdata.h"
#include "DXFshape.h"
#include "DXFDoc.h"
#include "DXFView.h"	// DXFTREETYPE
#include "Layer.h"
#include "ThreadDlg.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

using namespace boost;
#define	IsThread()	pParent->IsThreadContinue()

typedef	BOOL	(*PFNAUTOPROC)(CThreadDlg*, CLayerData*, DXFTREETYPE*);
static	BOOL	AutoOutlineProc(CThreadDlg*, CLayerData*, DXFTREETYPE*);
static	BOOL	AutoPocketProc(CThreadDlg*, CLayerData*, DXFTREETYPE*);
static	BOOL	AutoAllInside(CThreadDlg*, CLayerData*, DXFTREETYPE*);
static	BOOL	AutoAllOutside(CThreadDlg*, CLayerData*, DXFTREETYPE*);
static	BOOL	AutoRecalcWorking(CThreadDlg*, CLayerData*, DXFTREETYPE*);
static	void	CreateAutoWorking(CThreadDlg*, CLayerData*, BOOL);

//////////////////////////////////////////////////////////////////////
//	自動加工指示ｽﾚｯﾄﾞ
//////////////////////////////////////////////////////////////////////

UINT AutoWorkingSet_Thread(LPVOID pVoid)
{
	LPNCVCTHREADPARAM	pParam = reinterpret_cast<LPNCVCTHREADPARAM>(pVoid);
	CThreadDlg*	pParent = pParam->pParent;
	CDXFDoc*	pDoc = static_cast<CDXFDoc*>(pParam->pDoc);
	int			nType = (int)(pParam->wParam);	// 処理ﾀｲﾌﾟ
	DXFTREETYPE*	vSelect = reinterpret_cast<DXFTREETYPE*>(pParam->lParam);	// 再計算時の形状集合
	ENAUTOWORKINGTYPE enType;
	BOOL		bPocket;
	PFNAUTOPROC	pfnAutoProc;

	if ( nType >= 100 ) {	// ﾎﾟｹｯﾄ指示
		enType = (ENAUTOWORKINGTYPE)(nType - 100);
		bPocket = TRUE;
	}
	else {					// 輪郭または再計算(DXFShapeView)処理
		enType = (ENAUTOWORKINGTYPE)nType;
		bPocket = FALSE;
	}

	switch ( enType ) {
	case AUTOOUTLINE:
		pfnAutoProc = &AutoOutlineProc;
		break;
	case AUTOPOCKET:
		pfnAutoProc = &AutoPocketProc;
		break;
	case AUTOALLINSIDE:
		pfnAutoProc = &AutoAllInside;
		break;
	case AUTOALLOUTSIDE:
		pfnAutoProc = &AutoAllOutside;
		break;
	case AUTORECALCWORKING:
		pfnAutoProc = &AutoRecalcWorking;
		break;
	default:
		pParent->PostMessage(WM_USERFINISH, IDCANCEL);
		return 0;
	}

	int		i, nLoop = pDoc->GetLayerCnt(), nResult = IDOK;
	CString		strMsg;
	CLayerData*	pLayer;

	VERIFY(strMsg.LoadString(IDS_AUTOWORKING));

	// ﾚｲﾔごとに座標ﾏｯﾌﾟを検索
	for ( i=0; i<nLoop && IsThread(); i++ ) {
		pLayer = pDoc->GetLayerData(i);
		if ( !pLayer->IsCutType() || 
				(vSelect && vSelect->which()==DXFTREETYPE_LAYER && pLayer!=get<CLayerData*>(*vSelect)) )
			continue;
		pParent->SetFaseMessage(strMsg, pLayer->GetStrLayer());
		// ﾀｲﾌﾟ別の自動処理
		if ( (*pfnAutoProc)(pParent, pLayer, vSelect) )
			CreateAutoWorking(pParent, pLayer, bPocket);
		if ( vSelect && vSelect->which()==DXFTREETYPE_LAYER )
			break;	// ﾚｲﾔ指定あれば以降必要なし
	}

	pParent->PostMessage(WM_USERFINISH, IsThread() ? nResult : IDCANCEL);
	return 0;	// AfxEndThread(0);
}

//////////////////////////////////////////////////////////////////////
//	自動輪郭加工指示
//		他のｵﾌﾞｼﾞｪｸﾄに含まれる最小内周ｵﾌﾞｼﾞｪｸﾄに内側の加工指示，
//		それ以外は外側の加工指示
BOOL AutoOutlineProc(CThreadDlg* pParent, CLayerData* pLayer, DXFTREETYPE*)
{
	int		i, j, nLoop = pLayer->GetShapeSize();
	BOOL	bResult = FALSE;
	CDXFshape*	pShape;
	CDXFshape*	pShapeTmp;
	CRectD		rcBase;

	pParent->m_ctReadProgress.SetRange32(0, nLoop);
	pLayer->AscendingShapeSort();	// 面積で昇順並べ替え

	// 占有矩形の内外判定
	for ( i=0; i<nLoop && IsThread(); i++ ) {
		pShape = pLayer->GetShapeData(i);
		// ﾌﾟﾛｸﾞﾚｽﾊﾞｰ
		pParent->m_ctReadProgress.SetPos(i);	// 件数少ないので１件ずつ更新
		pShape->ClearSideFlg();
		// 自動処理対象か否か(CDXFchain* だけを対象とする)
		if ( pShape->GetShapeType()!=0 || pShape->GetShapeFlag()&DXFMAPFLG_CANNOTAUTOWORKING )
			continue;
		// rcBaseよりも大きな面積を持つｵﾌﾞｼﾞｪｸﾄ(i+1)がrcBaseを含むかどうかの判定
		rcBase = pShape->GetMaxRect();
		for ( j=i+1; j<nLoop && IsThread(); j++ ) {
			pShapeTmp = pLayer->GetShapeData(j);
			if ( pShapeTmp->GetShapeType() != 0 )
				continue;
			if ( pShapeTmp->GetMaxRect().PtInRect(rcBase) )
				break;	// 以後の検査は必要なし
		}
		pShape->SetShapeFlag(j<nLoop ? DXFMAPFLG_INSIDE : DXFMAPFLG_OUTSIDE);
		bResult = TRUE;		// 処理済み
	}

	pParent->m_ctReadProgress.SetPos(nLoop);
	return bResult;
}

//////////////////////////////////////////////////////////////////////
//	自動ﾎﾟｹｯﾄ加工指示
//		最大外周と最小内周に内側の加工指示
BOOL AutoPocketProc(CThreadDlg* pParent, CLayerData* pLayer, DXFTREETYPE*)
{
	int		i, j, nLoop = pLayer->GetShapeSize();
	BOOL	bResult = FALSE;
	CDXFshape*	pShape;
	CDXFshape*	pShapeTmp;
	CRectD		rcBase;

	pParent->m_ctReadProgress.SetRange32(0, nLoop);
	pLayer->DescendingShapeSort();	// 面積で降順並べ替え

	// 最大外周判定
	for ( i=0; i<nLoop && IsThread(); i++ ) {
		pShape = pLayer->GetShapeData(i);
		// ﾌﾟﾛｸﾞﾚｽﾊﾞｰ
		pParent->m_ctReadProgress.SetPos(i);
		pShape->ClearSideFlg();
		// CDXFchain* だけを対象とする
		if ( pShape->GetShapeType()!=0 || pShape->GetShapeFlag()&DXFMAPFLG_CANNOTAUTOWORKING )
			continue;
		// rcBaseが全てのｵﾌﾞｼﾞｪｸﾄを含んでいるかどうかの判定
		rcBase = pShape->GetMaxRect();
		for ( j=i+1; j<nLoop && IsThread(); j++ ) {
			pShapeTmp = pLayer->GetShapeData(j);
			if ( pShapeTmp->GetShapeType() != 0 )
				continue;
			if ( !rcBase.PtInRect(pShapeTmp->GetMaxRect()) )
				break;	// 含まなければ以後の検査は必要なし
		}
		if ( j >= nLoop ) {	// 含んでいれば
			pShape->SetShapeFlag(DXFMAPFLG_INSIDE);
			bResult = TRUE;
		}
		break;	// 最大矩形のみ
	}

	// 最小内周判定
	for ( ; i<nLoop && IsThread(); i++ ) {	// [0]～は判定済み
		pShape = pLayer->GetShapeData(i);
		pParent->m_ctReadProgress.SetPos(i);
		pShape->ClearSideFlg();
		if ( pShape->GetShapeType()!=0 || pShape->GetShapeFlag()&DXFMAPFLG_CANNOTAUTOWORKING )
			continue;
		// rcBaseが他のｵﾌﾞｼﾞｪｸﾄ(i+1)を含むかどうかの判定
		rcBase = pShape->GetMaxRect();
		for ( j=i+1; j<nLoop && IsThread(); j++ ) {
			pShapeTmp = pLayer->GetShapeData(j);
			if ( pShapeTmp->GetShapeType() != 0 )
				continue;
			if ( rcBase.PtInRect(pShapeTmp->GetMaxRect()) )
				break;	// 含んでいれば以後の検査は必要なし
		}
		if ( j >= nLoop ) {	// 含まなければ
			pShape->SetShapeFlag(DXFMAPFLG_INSIDE);
			bResult = TRUE;
		}
	}

	pParent->m_ctReadProgress.SetPos(nLoop);
	return bResult;
}

//////////////////////////////////////////////////////////////////////
//	全て内側指示
BOOL AutoAllInside(CThreadDlg* pParent, CLayerData* pLayer, DXFTREETYPE*)
{
	int		i, nLoop = pLayer->GetShapeSize();
	BOOL	bResult = FALSE;
	CDXFshape*	pShape;

	pParent->m_ctReadProgress.SetRange32(0, nLoop);

	for ( i=0; i<nLoop && IsThread(); i++ ) {
		pShape = pLayer->GetShapeData(i);
		// ﾌﾟﾛｸﾞﾚｽﾊﾞｰ
		pParent->m_ctReadProgress.SetPos(i);
		pShape->ClearSideFlg();
		// 自動処理対象か否か(CDXFchain* だけを対象とする)
		if ( pShape->GetShapeType()!=0 || pShape->GetShapeFlag()&DXFMAPFLG_CANNOTAUTOWORKING )
			continue;
		pShape->SetShapeFlag(DXFMAPFLG_INSIDE);		// 全て内側
		bResult = TRUE;		// 処理済み
	}

	pParent->m_ctReadProgress.SetPos(nLoop);
	return bResult;
}

//////////////////////////////////////////////////////////////////////
//	全て外側指示
BOOL AutoAllOutside(CThreadDlg* pParent, CLayerData* pLayer, DXFTREETYPE*)
{
	int		i, nLoop = pLayer->GetShapeSize();
	BOOL	bResult = FALSE;
	CDXFshape*	pShape;

	pParent->m_ctReadProgress.SetRange32(0, nLoop);

	for ( i=0; i<nLoop && IsThread(); i++ ) {
		pShape = pLayer->GetShapeData(i);
		// ﾌﾟﾛｸﾞﾚｽﾊﾞｰ
		pParent->m_ctReadProgress.SetPos(i);
		pShape->ClearSideFlg();
		// 自動処理対象か否か(CDXFchain* だけを対象とする)
		if ( pShape->GetShapeType()!=0 || pShape->GetShapeFlag()&DXFMAPFLG_CANNOTAUTOWORKING )
			continue;
		pShape->SetShapeFlag(DXFMAPFLG_OUTSIDE);	// 全て外側
		bResult = TRUE;		// 処理済み
	}

	pParent->m_ctReadProgress.SetPos(nLoop);
	return bResult;
}

//////////////////////////////////////////////////////////////////////
//	ｵﾌｾｯﾄ値変更による再計算
BOOL AutoRecalcWorking(CThreadDlg* pParent, CLayerData* pLayer, DXFTREETYPE* vSelect)
{
	int		i, nInOut, nLoop = pLayer->GetShapeSize();
	DWORD	dwFlags;
	CDXFdata*	pData;
	CDXFshape*	pShape;
	CDXFshape*	pShapeSrc = NULL;
	CDXFchain	ltOutline;
	CDXFworking*	pWork;

	// 対象集合
	if ( vSelect && vSelect->which()==DXFTREETYPE_SHAPE )
		pShapeSrc = get<CDXFshape*>(*vSelect);

	pParent->m_ctReadProgress.SetRange32(0, nLoop);

	try {
		for ( i=0; i<nLoop && IsThread(); i++ ) {
			pShape = pLayer->GetShapeData(i);
			// ﾌﾟﾛｸﾞﾚｽﾊﾞｰ
			pParent->m_ctReadProgress.SetPos(i);
			// 形状集合が指定されていれば、それにﾏｯﾁするものだけ
			if ( pShapeSrc && pShapeSrc != pShape )
				continue;
			// 既にある加工指示を抽出
			dwFlags = pShape->GetShapeFlag();
			if ( !pShape->GetOutlineObject() )
				continue;
			// 現在登録されている方向で輪郭ｵﾌﾞｼﾞｪｸﾄ生成
			nInOut = pShape->GetInOutFlag();
			if ( !pShape->CreateOutlineTempObject(nInOut, &ltOutline) ) {
				// 一時ｵﾌﾞｼﾞｪｸﾄ全削除
				for ( POSITION pos=ltOutline.GetHeadPosition(); pos; ) {
					pData = ltOutline.GetNext(pos);
					if ( pData )
						delete	pData;
				}
				continue;
			}
			// 加工指示登録
			pWork = new CDXFworkingOutline(pShape, &ltOutline, DXFWORKFLG_AUTO);
			pShape->AddWorkingData(pWork, nInOut);
			pWork = NULL;
			// 次のﾙｰﾌﾟに備える
			ltOutline.RemoveAll();
			if ( pShapeSrc )
				break;	// 形状指定あれば以降必要なし
		}
	}
	catch ( CMemoryException* e ) {
		if ( pWork )
			delete	pWork;
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
	}

	pParent->m_ctReadProgress.SetPos(nLoop);
	return FALSE;	// 全てこの関数で処理するので CreateAutoWorking() を実行しない
}

//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//	内外判定から加工指示の生成

void CreateAutoWorking(CThreadDlg* pParent, CLayerData* pLayer, BOOL)
{
	int		i, j, n, nLoop = pLayer->GetShapeSize();
	double		dArea[2];
	CRect3D		rcMax;
	CDXFchain	ltOutline[2];
	CDXFdata*		pData;
	CDXFshape*		pShape;
	CDXFworking*	pWork;
	POSITION		pos;
#ifdef _DEBUG
	CRect	rcDbg;
#endif

	pParent->m_ctReadProgress.SetRange32(0, nLoop);

	try {
		for ( i=0; i<nLoop && IsThread(); i++ ) {
			pWork = NULL;
			pShape = pLayer->GetShapeData(i);
			// ﾌﾟﾛｸﾞﾚｽﾊﾞｰ
			pParent->m_ctReadProgress.SetPos(i);
			// 処理対象ﾁｪｯｸ
			if ( !(pShape->GetShapeFlag() & (DXFMAPFLG_INSIDE|DXFMAPFLG_OUTSIDE)) )
				continue;
#ifdef _DEBUG
			g_dbg.printf("ShapeName=%s", pShape->GetShapeName());
			rcDbg = pShape->GetMaxRect();
			g_dbg.printStruct(&rcDbg, "Orig");
#endif
			// 輪郭一時ｵﾌﾞｼﾞｪｸﾄの生成
			for ( j=0; j<SIZEOF(ltOutline) && IsThread(); j++ ) {
				if ( !pShape->CreateOutlineTempObject(j, &ltOutline[j]) ) {
					// 一時ｵﾌﾞｼﾞｪｸﾄ全削除
					for ( n=0; n<SIZEOF(ltOutline); n++ ) {
						for ( pos=ltOutline[n].GetHeadPosition(); pos; ) {
							pData = ltOutline[n].GetNext(pos);
							if ( pData )
								delete	pData;
						}
						ltOutline[n].RemoveAll();
					}
					break;
				}
				if ( ltOutline[j].IsEmpty() )
					rcMax = pShape->GetMaxRect();	// 元集合の矩形領域をｾｯﾄ
				else
					rcMax = ltOutline[j].GetMaxRect();
				// 矩形領域の大きさ計算
				dArea[j] = rcMax.Width() * rcMax.Height();
#ifdef _DEBUG
				rcDbg = ltOutline[j].GetMaxRect();
				g_dbg.printStruct(&rcDbg, "OutLine");
#endif
			}
			if ( j < SIZEOF(ltOutline) )	// CreateOutlineTempObject() でのｴﾗｰ
				continue;
			// 内外を矩形の大きさで決定
			if ( pShape->GetShapeFlag() & DXFMAPFLG_INSIDE )
				j = dArea[0] > dArea[1] ? 1 : 0;	// 小さい方を採用
			else
				j = dArea[0] > dArea[1] ? 0 : 1;	// 大きい方を採用
			// 加工指示登録
			if ( !ltOutline[j].IsEmpty() ) {
				pWork = new CDXFworkingOutline(pShape, &ltOutline[j], DXFWORKFLG_AUTO);
				pShape->AddWorkingData(pWork, j);
#ifdef _DEBUG
				g_dbg.printf("Select OutLine = %d", j);
#endif
			}
			// Select分はCDXFworkingOutlineのﾃﾞｽﾄﾗｸﾀにてdelete
			n = 1 - j;	// 1->0, 0->1
			for ( pos=ltOutline[n].GetHeadPosition(); pos; ) {
				pData = ltOutline[n].GetNext(pos);
				if ( pData )
					delete	pData;
			}
			// 次のﾙｰﾌﾟに備え、矩形の初期化
			for ( j=0; j<SIZEOF(ltOutline); j++ )
				ltOutline[j].RemoveAll();
		}
	}
	catch ( CMemoryException* e ) {
		if ( pWork )
			delete	pWork;
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
	}

	pParent->m_ctReadProgress.SetPos(nLoop);
	return;
}
