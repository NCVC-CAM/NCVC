// DXFshape.cpp: CDXFmap クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "DXFdata.h"
#include "DXFshape.h"
#include "Layer.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
//#define	_DEBUGDRAW_DXF
#endif

using namespace boost;

IMPLEMENT_DYNAMIC(CDXFworking, CObject)
IMPLEMENT_SERIAL(CDXFworkingDirection, CDXFworking, NCVCSERIALVERSION|VERSIONABLE_SCHEMA)
IMPLEMENT_SERIAL(CDXFworkingOutline, CDXFworking, NCVCSERIALVERSION|VERSIONABLE_SCHEMA)
IMPLEMENT_SERIAL(CDXFworkingPocket, CDXFworking, NCVCSERIALVERSION|VERSIONABLE_SCHEMA)
IMPLEMENT_SERIAL(CDXFmap, CMapPointToDXFarray, NCVCSERIALVERSION|VERSIONABLE_SCHEMA)
IMPLEMENT_SERIAL(CDXFchain, CDXFlist, NCVCSERIALVERSION|VERSIONABLE_SCHEMA)
IMPLEMENT_SERIAL(CDXFshape, CObject, NCVCSERIALVERSION|VERSIONABLE_SCHEMA)

/////////////////////////////////////////////////////////////////////////////
// 静的変数の初期化
double		CDXFmap::ms_dTolerance = NCMIN;

static	DWORD	g_dwMapFlag[] = {
	DXFMAPFLG_DIRECTION, DXFMAPFLG_OUTLINE, DXFMAPFLG_POCKET
};

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// ＤＸＦデータ加工指示のベースクラス
/////////////////////////////////////////////////////////////////////////////
CDXFworking::CDXFworking
	(ENWORKINGTYPE enType, CDXFshape* pShape, CDXFdata* pData, DWORD dwFlags)
{
	static	CString	strAuto("自動");
	static	TCHAR*	szWork[] = {"内側", "外側"};

	m_enType = enType;
	m_pShape = pShape;
	m_pData = pData;
	m_dwFlags = dwFlags;

	// 自動処理
	if ( m_dwFlags & DXFWORKFLG_AUTO ) {
		if ( !m_pShape )
			return;
		if ( m_pShape->GetShapeFlag() & DXFMAPFLG_INSIDE )
			m_strWorking = strAuto + szWork[0];
		else if ( m_pShape->GetShapeFlag() & DXFMAPFLG_OUTSIDE )
			m_strWorking = strAuto + szWork[1];
	}
	// ﾃﾞﾌｫﾙﾄ加工指示名
	CString	strWork;
	VERIFY(strWork.LoadString(enType+ID_EDIT_SHAPE_VEC));
	m_strWorking += strWork.Mid(strWork.Find('\n')+1);
}

void CDXFworking::Serialize(CArchive& ar)
{
	if ( ar.IsStoring() ) {
		ar << (m_dwFlags & ~DXFWORKFLG_SELECT) << m_strWorking;	// 選択状態除く
		if ( m_pData ) {
			ar << (BYTE)1;
			ar << m_pData->GetSerializeSeq();
		}
		else
			ar << (BYTE)0;
	}
	else {
		BYTE	bExist;
		DWORD	nIndex;
		CLayerData*	pLayer = (CLayerData *)(ar.m_pDocument);
		m_pShape = pLayer->GetActiveShape();
		ar >> m_dwFlags >> m_strWorking;
		ar >> bExist;
		if ( bExist ) {
			ar >> nIndex;
			m_pData = pLayer->GetDxfData(nIndex);
		}
	}
}

//////////////////////////////////////////////////////////////////////
// ＤＸＦデータの「方向」加工指示クラス
//////////////////////////////////////////////////////////////////////
CDXFworkingDirection::CDXFworkingDirection(CDXFshape* pShape, CDXFdata* pData, CPointD pt[]) :
	CDXFworking(DIRECTION, pShape, pData, 0)
{
	for ( int i=0; i<SIZEOF(m_ptArraw); i++ )
		m_ptArraw[i] = pt[i];
}

void CDXFworkingDirection::DrawTuning(double f)
{
	m_ptDraw[1] = m_ptArraw[1] * f;
	m_ptDraw[0] = m_ptArraw[0] + m_ptDraw[1];
	m_ptDraw[2] = m_ptArraw[2] + m_ptDraw[1];
}

void CDXFworkingDirection::Draw(CDC* pDC) const
{
#ifdef _DEBUGDRAW_DXF
	CMagaDbg	dbg("CDXFworkingDirection::Draw()", DBG_RED);
	dbg.printf("pt.x=%d pt.y=%d", m_ptDraw[1].x, m_ptDraw[1].y);
#endif
	pDC->Polyline(m_ptDraw, SIZEOF(m_ptDraw));
}

void CDXFworkingDirection::Serialize(CArchive& ar)
{
	int		i;
	CDXFworking::Serialize(ar);
	if ( ar.IsStoring() ) {
		for ( i=0; i<SIZEOF(m_ptArraw); i++ )
			ar << m_ptArraw[i].x << m_ptArraw[i].y;
	}
	else {
		for ( i=0; i<SIZEOF(m_ptArraw); i++ )
			ar >> m_ptArraw[i].x >> m_ptArraw[i].y;
	}
}

//////////////////////////////////////////////////////////////////////
// ＤＸＦデータの「輪郭」加工指示クラス
//////////////////////////////////////////////////////////////////////
CDXFworkingOutline::CDXFworkingOutline(CDXFshape* pShape, const CDXFarray& obOutline, DWORD dwFlags) :
	CDXFworking(OUTLINE, pShape, NULL, dwFlags)
{
	m_obOutline.Copy(obOutline);
}

CDXFworkingOutline::~CDXFworkingOutline()
{
	for ( int i=0; i<m_obOutline.GetSize(); i++ )
		delete	m_obOutline[i];
}

void CDXFworkingOutline::DrawTuning(double f)
{
	for ( int i=0; i<m_obOutline.GetSize(); i++ )
		m_obOutline[i]->DrawTuning(f);
}

void CDXFworkingOutline::Draw(CDC* pDC) const
{
	for ( int i=0; i<m_obOutline.GetSize(); i++ )
		m_obOutline[i]->Draw(pDC);
}

void CDXFworkingOutline::Serialize(CArchive& ar)
{
	CDXFworking::Serialize(ar);
	m_obOutline.Serialize(ar);
}

//////////////////////////////////////////////////////////////////////
// ＤＸＦデータの「ポケット」加工指示クラス
//////////////////////////////////////////////////////////////////////
CDXFworkingPocket::CDXFworkingPocket(CDXFshape* pShape, DWORD dwFlags) :
	CDXFworking(POCKET, pShape, NULL, dwFlags)
{
	m_obPocket.SetSize(0, 1024);
}

CDXFworkingPocket::~CDXFworkingPocket()
{
	for ( int i=0; i<m_obPocket.GetSize(); i++ )
		delete	m_obPocket[i];
}

void CDXFworkingPocket::DrawTuning(double f)
{
	for ( int i=0; i<m_obPocket.GetSize(); i++ )
		m_obPocket[i]->DrawTuning(f);
}

void CDXFworkingPocket::Draw(CDC* pDC) const
{
	for ( int i=0; i<m_obPocket.GetSize(); i++ )
		m_obPocket[i]->Draw(pDC);
}

void CDXFworkingPocket::Serialize(CArchive& ar)
{
	CDXFworking::Serialize(ar);
	m_obPocket.Serialize(ar);
}

/////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// ＤＸＦデータの座標マップクラス
//////////////////////////////////////////////////////////////////////
CDXFmap::CDXFmap() : CMapPointToDXFarray(1024)
{
}

CDXFmap::~CDXFmap()
{
	RemoveAll();
}

void CDXFmap::Serialize(CArchive& ar)
{
	int			i, j, nDataCnt;
	CPointD		pt;
	CDXFarray*	pArray;

	if ( ar.IsStoring() ) {
		ar << GetCount() << GetHashTableSize();
		for ( POSITION pos=GetStartPosition(); pos; ) {
			GetNextAssoc(pos, pt, pArray);
			nDataCnt = pArray->GetSize();
			ar << pt.x << pt.y << nDataCnt;
			for ( i=0; i<nDataCnt; i++ )
				ar << pArray->GetAt(i)->GetSerializeSeq();
		}
	}
	else {
		INT_PTR		nMapLoop;
		UINT		nHash;
		DWORD		nIndex;
		CDXFdata*	pData;
		CLayerData*	pLayer = (CLayerData *)(ar.m_pDocument);
		ar >> nMapLoop >> nHash;
		InitHashTable(nHash);
		for ( i=0; i<nMapLoop; i++ ) {
			ar >> pt.x >> pt.y >> nDataCnt;
			if ( nDataCnt > 0 ) {
				pArray = new CDXFarray;
				pArray->SetSize(0, nDataCnt);
				for ( j=0; j<nDataCnt; j++ ) {
					ar >> nIndex;
					pData = pLayer->GetDxfData(nIndex);
					pArray->Add(pData);
				}
				SetAt(pt, pArray);
			}
		}
	}
}

void CDXFmap::RemoveAll()
{
	CPointD	pt;
	CDXFarray*	pArray;

	for ( POSITION pos=GetStartPosition(); pos; ) {
		GetNextAssoc(pos, pt, pArray);
		if ( pArray )
			delete	pArray;
	}
	CMapPointToDXFarray::RemoveAll();
}

void CDXFmap::SetPointMap(CDXFdata* pData)
{
	CDXFarray*	pArray;
	CPointD	pt;
#ifdef _DEBUGOLD
	CDumpContext	dc;
	dc.SetDepth(1);
	Dump(dc);
#endif

	// 各ｵﾌﾞｼﾞｪｸﾄ頂点の座標登録
	for ( int i=0; i<pData->GetPointNumber(); i++ ) {
		pt = pData->GetNativePoint(i);	// 固有座標値
		if ( pt != HUGE_VAL ) {
			if ( Lookup(pt, pArray) ) {
				pArray->Add(pData);
			}
			else {
				pArray = new CDXFarray;
				ASSERT( pArray );
				pArray->SetSize(0, 16);
				pArray->Add(pData);
				SetAt(pt, pArray);
			}
		}
	}
}

void CDXFmap::SetMakePointMap(CDXFdata* pData)
{
	CDXFarray*	pArray;
	CPointD	pt;

	// 各ｵﾌﾞｼﾞｪｸﾄ頂点の座標登録
	for ( int i=0; i<pData->GetPointNumber(); i++ ) {
		pt = pData->GetTunPoint(i);		// 原点調整座標
		if ( pt != HUGE_VAL ) {
			if ( Lookup(pt, pArray) )
				pArray->Add(pData);
			else {
				pArray = new CDXFarray;
				ASSERT( pArray );
				pArray->SetSize(0, 16);
				pArray->Add(pData);
				SetAt(pt, pArray);
			}
		}
	}
}

tuple<BOOL, CDXFarray*, CPointD>
CDXFmap::IsEulerRequirement(const CPointD& ptKey) const
{
	int			i, nObCnt, nOddCnt = 0;
	BOOL		bEuler = FALSE;	// 一筆書き要件を満たしているか
	double		dGap, dGapMin = HUGE_VAL, dGapMin2 = HUGE_VAL;
	CPointD		pt, ptStart, ptStart2;
	CDXFdata*	pData;
	CDXFarray*	pArray;
	CDXFarray*	pStartArray = NULL;
	CDXFarray*	pStartArray2;

	// ｵﾌﾞｼﾞｪｸﾄ登録数の奇数を探す + ｻｰﾁﾌﾗｸﾞのｸﾘｱ
	for ( POSITION pos=GetStartPosition(); pos; ) {
		GetNextAssoc(pos, pt, pArray);
		// 座標ｷｰに対する登録ｵﾌﾞｼﾞｪｸﾄ数が奇数の近接ｵﾌﾞｼﾞｪｸﾄを検索
		// (円ﾃﾞｰﾀはｶｳﾝﾄしないため obArray->GetSize() が使えない)
		for ( i=0, nObCnt=0; i<pArray->GetSize(); i++ ) {
			pData = pArray->GetAt(i);
			pData->ClearSearchFlg();
			if ( !pData->IsStartEqEnd() )	// 始点に戻るﾃﾞｰﾀ以外
				nObCnt++;
		}
		dGap = GAPCALC(ptKey - pt);
		if ( nObCnt & 0x01 ) {
			nOddCnt++;
			// 奇数を優先的に検索
			if ( dGap < dGapMin ) {
				dGapMin = dGap;
				pStartArray = pArray;
				ptStart = pt;
			}
		}
		else {
			// 偶数の場合でも一番近い座標を確保
			if ( dGap < dGapMin2 ) {
				dGapMin2 = dGap;
				pStartArray2 = pArray;
				ptStart2 = pt;
			}
		}
	}
	if ( !pStartArray ) {		// nOddCnt == 0
		bEuler = TRUE;		// 一筆書き要件を満たしている
		// 奇数座標がない場合は偶数座標の最も近いｵﾌﾞｼﾞｪｸﾄ配列を使う
		pStartArray = pStartArray2;
		ptStart = ptStart2;
	}
	else if ( nOddCnt == 2 ) {	// 奇点が２個
		bEuler = TRUE;		// 一筆書き要件を満たしている
	}

	return make_tuple(bEuler, pStartArray, ptStart);
}

DWORD CDXFmap::GetShapeFlag(void)
{
	// １つの座標に３つ以上のｵﾌﾞｼﾞｪｸﾄがあれば
	//		DXFMAPFLG_CANNOTWORKING -> 加工指示できない
	// 交点や端点があれば
	//		DXFMAPFLG_CANNOTOUTLINE -> 方向指示は可能，輪郭・ﾎﾟｹｯﾄ加工ができない
	int			i, j, nLoop;
	DWORD		dwFlags = 0;
	CDXFarray	obWorkArray;
	CDXFarray*	pArray;
	CDXFdata*	pData;
	CPointD		pt, ptChk[4];

	obWorkArray.SetSize(0, GetSize());
	AllMapObject_ClearSearchFlg(FALSE);

	try {
		for ( POSITION pos=GetStartPosition(); pos; ) {
			GetNextAssoc(pos, pt, pArray);
			nLoop = pArray->GetSize();
			if ( nLoop != 2 ) {
				if ( nLoop == 1 )	// 端点
					dwFlags |= DXFMAPFLG_CANNOTOUTLINE;
				else				// ３つ以上のｵﾌﾞｼﾞｪｸﾄ
					dwFlags |= DXFMAPFLG_CANNOTWORKING;
			}
			// 交点ﾁｪｯｸのためにｵﾌﾞｼﾞｪｸﾄをｺﾋﾟｰ
			for ( i=0; i<nLoop; i++ ) {
				pData = pArray->GetAt(i);
				if ( !pData->IsSearchFlg() ) {
					obWorkArray.Add(pData);		// ﾜｰｸ配列に登録
					pData->SetSearchFlg();
					pData->ClearMakeFlg();
				}
			}
		}
		// 交点ﾁｪｯｸ
		nLoop = obWorkArray.GetSize();
		for ( i=0; i<nLoop; i++ ) {
			pData = obWorkArray[i];
			for ( j=i+1; j<nLoop; j++ ) {
				if ( pData->GetIntersectionPoint(obWorkArray[j], ptChk) > 0 ) {
					dwFlags |= DXFMAPFLG_CANNOTOUTLINE;
					i = nLoop;	// 外側ﾙｰﾌﾟも終了
					break;
				}
			}
		}
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
	}

	return dwFlags;
}

BOOL CDXFmap::IsAllSearchFlg(void) const
{
	int			i;
	BOOL		bResult = TRUE;
	CPointD		pt;
	CDXFarray*	pArray;

	for ( POSITION pos=GetStartPosition(); pos; ) {
		GetNextAssoc(pos, pt, pArray);
		for ( i=0; i<pArray->GetSize() && pos; i++ ) {
			if ( !pArray->GetAt(i)->IsSearchFlg() ) {
				pos = NULL;
				bResult = FALSE;
				break;
			}
		}
	}

	return bResult;
}

void CDXFmap::AllMapObject_ClearSearchFlg(BOOL bMake/*=TRUE*/) const
{
	int			i;
	CPointD		pt;
	CDXFdata*	pData;
	CDXFarray*	pArray;

	for ( POSITION pos=GetStartPosition(); pos; ) {
		GetNextAssoc(pos, pt, pArray);
		for ( i=0; i<pArray->GetSize(); i++ ) {
			pData = pArray->GetAt(i);
			if ( !bMake || (bMake && !pData->IsMakeFlg()) )
				pData->ClearSearchFlg();
		}
	}
}

void CDXFmap::AllMapObject_ClearMakeFlg() const
{
	int			i;
	CPointD		pt;
	CDXFarray*	pArray;

	for ( POSITION pos=GetStartPosition(); pos; ) {
		GetNextAssoc(pos, pt, pArray);
		for ( i=0; i<pArray->GetSize(); i++ )
			pArray->GetAt(i)->ClearMakeFlg();
	}
}

CDXFdata* CDXFmap::GetFirstObject(void) const
{
	CPointD		pt;
	CDXFarray*	pArray;
	CDXFdata*	pData = NULL;
	POSITION	pos = GetStartPosition();

	if ( pos ) {
		GetNextAssoc(pos, pt, pArray);
		if ( !pArray->IsEmpty() )
			pData = pArray->GetAt(0);
	}

	return pData;
}

void CDXFmap::Copy_ToChain(CDXFchain* pChain)
{
	int			i, nLoop;
	CDXFarray*	pArray;
	CDXFdata*	pData = GetFirstObject();	// ﾗﾝﾀﾞﾑ(?)な最初のｵﾌﾞｼﾞｪｸﾄを取得
	CPointD		pt(pData->GetNativePoint(1));

	AllMapObject_ClearMakeFlg();
	pChain->AddTail(pData);
	pData->SetMakeFlg();

	while ( Lookup(pt, pArray) ) {
		nLoop = pArray->GetSize();
		for ( i=0; i<nLoop; i++ ) {
			pData = pArray->GetAt(i);
			if ( !pData->IsMakeFlg() ) {
				pData->SetDirectionFixed(pt, pt);
				pChain->AddTail(pData);
				pData->SetMakeFlg();
				pt = pData->GetNativePoint(1);
				break;
			}
		}
		if ( i >= nLoop )
			break;
	}
}

void CDXFmap::Append(const CDXFmap* pMap)
{
	CPointD		pt;
	CDXFarray*	pArraySrc;
	CDXFarray*	pArrayDst;

	for ( POSITION pos=pMap->GetStartPosition(); pos; ) {
		pMap->GetNextAssoc(pos, pt, pArraySrc);
		if ( !pArraySrc->IsEmpty() ) {
			if ( Lookup(pt, pArrayDst) )
				pArrayDst->InsertAt(pArrayDst->GetCount(), pArraySrc);	// Append使えない
			else {
				pArrayDst = new CDXFarray;
				ASSERT( pArrayDst );
				pArrayDst->SetSize(0, 16);
				pArrayDst->InsertAt(0, pArraySrc);		// Copy使えない
				SetAt(pt, pArrayDst);
			}
		}
	}
}

void CDXFmap::Append(const CDXFchain* pChain)
{
	for ( POSITION pos=pChain->GetHeadPosition(); pos; )
		SetPointMap( pChain->GetNext(pos) );
}

int CDXFmap::GetObjectCount(void)
{
	int			i, nCnt = 0;
	CPointD		pt;
	CDXFdata*	pData;
	CDXFarray*	pArray;

	AllMapObject_ClearSearchFlg(FALSE);

	for ( POSITION pos=GetStartPosition(); pos; ) {
		GetNextAssoc(pos, pt, pArray);
		for ( i=0; i<pArray->GetSize(); i++ ) {
			pData = pArray->GetAt(i);
			if ( !pData->IsSearchFlg() ) {
				nCnt++;
				pData->SetSearchFlg();
			}
		}
	}

	return nCnt;
}

tuple<CDXFdata*, double> CDXFmap::GetSelectViewGap
	(const CPointD& pt, const CRectD& rcView)
{
	int		i;
	CPointD	ptKey;
	double	dGap, dGapMin = HUGE_VAL;
	CDXFarray*	pArray;
	CDXFdata*	pData;
	CDXFdata*	pDataResult = NULL;

	// 指定座標に一番近いｵﾌﾞｼﾞｪｸﾄを検索
	for ( POSITION pos=GetStartPosition(); pos; ) {
		GetNextAssoc(pos, ptKey, pArray);
		for ( i=0; i<pArray->GetSize(); i++ ) {
			pData = pArray->GetAt(i);
			if ( rcView.PtInRect(pData->GetMaxRect()) ) {
				dGap = pData->GetSelectPointGap(pt);
				if ( dGap < dGapMin ) {
					dGapMin = dGap;
					pDataResult = pData;
				}
			}
		}
	}

	return make_tuple(pDataResult, dGapMin);
}

tuple<CDXFdata*, double> CDXFmap::GetSelectMakeGap
	(const CPointD& pt, BOOL bObject)
{
	CDXFarray*	pArray;
	CDXFarray*	pArrayMin = NULL;
	CDXFdata*	pData;
	CDXFdata*	pDataResult = NULL;
	CPointD	ptKey;
	double	dGap, dGapMin = HUGE_VAL;

	// 指定座標に一番近い座標ｷｰを検索
	for ( POSITION pos=GetStartPosition(); pos; ) {
		GetNextAssoc(pos, ptKey, pArray);
		dGap = GAPCALC(ptKey - pt);
		if ( dGap < dGapMin ) {
			dGapMin = dGap;
			pArrayMin = pArray;
		}
	}

	if ( bObject ) {
		// その座標ｷｰの配列から一番近いｵﾌﾞｼﾞｪｸﾄの距離を返す
		ASSERT( pArrayMin );
		dGapMin = HUGE_VAL;
		for ( int i=0; i<pArrayMin->GetSize(); i++ ) {
			pData = pArrayMin->GetAt(i);
			dGap = pData->GetSelectPointGap(pt);
			if ( dGap < dGapMin ) {
				dGapMin = dGap;
				pDataResult = pData;
			}
		}
	}

	return make_tuple(pDataResult, dGapMin);
}

void CDXFmap::SetShapeSwitch(BOOL bSelect)
{
	CDXFarray*	pArray;
	CPointD	pt;
	int		i;

	for ( POSITION pos=GetStartPosition(); pos; ) {
		GetNextAssoc(pos, pt, pArray);
		for ( i=0; i<pArray->GetSize(); i++ )
			pArray->GetAt(i)->SetSelectFlg(bSelect);
	}
}

void CDXFmap::RemoveObject(const CDXFdata* pData)
{
	CDXFarray*	pArray;
	CPointD	pt;
	int		i, j;

	for ( i=0; i<pData->GetPointNumber(); i++ ) {
		pt = pData->GetNativePoint(i);	// 固有座標値
		if ( pt!=HUGE_VAL && Lookup(pt, pArray) ) {
			for ( j=0; j<pArray->GetSize(); j++ ) {
				if ( pData == pArray->GetAt(j) ) {
					pArray->RemoveAt(j);
					break;
				}
			}
			if ( pArray->IsEmpty() ) {
				delete	pArray;
				RemoveKey(pt);
			}
		}
	}
}

void CDXFmap::DrawShape(CDC* pDC)
{
	CDXFarray*	pArray;
	CDXFdata*	pData;
	CPointD	pt;
	int		i;
	DWORD	dwSel, dwSelBak = 0;

	CBrush* pOldBrush = (CBrush *)pDC->SelectStockObject(NULL_BRUSH);
	CPen* pOldPen = pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenDXF(DXFPEN_CUTTER));
	pDC->SetROP2(R2_COPYPEN);
	for ( POSITION pos=GetStartPosition(); pos; ) {
		GetNextAssoc(pos, pt, pArray);
		for ( i=0; i<pArray->GetSize(); i++ ) {
			pData = pArray->GetAt(i);
			dwSel = pData->GetSelectFlg() & DXFSEL_SELECT;
			if ( dwSel != dwSelBak ) {
				dwSelBak = dwSel;
				pDC->SelectObject(pData->GetDrawPen());
			}
			pData->Draw(pDC);
		}
	}
	pDC->SelectObject(pOldPen);
	pDC->SelectObject(pOldBrush);
}

/////////////////////////////////////////////////////////////////////////////
// ＤＸＦデータの輪郭集団クラス
/////////////////////////////////////////////////////////////////////////////
CDXFchain::CDXFchain() : CDXFlist(1024)
{
}

CDXFchain::~CDXFchain()
{
}

void CDXFchain::Serialize(CArchive& ar)
{
	if ( ar.IsStoring() ) {
		ar << GetCount();
		for ( POSITION pos=GetHeadPosition(); pos; )
			ar << GetNext(pos)->GetSerializeSeq();
	}
	else {
		INT_PTR		nListLoop;
		DWORD		nIndex;
		CDXFdata*	pData;
		CLayerData*	pLayer = (CLayerData *)(ar.m_pDocument);
		ar >> nListLoop;
		for ( int i=0; i<nListLoop; i++ ) {
			ar >> nIndex;
			pData = pLayer->GetDxfData(nIndex);
			AddTail(pData);
		}
	}
}

DWORD CDXFchain::GetShapeFlag(void)
{
	DWORD		dwFlags = 0;

	return dwFlags;
}

void CDXFchain::Copy_ToMap(CDXFmap* pMap)
{
	for ( POSITION pos=GetHeadPosition(); pos; )
		pMap->SetPointMap( GetNext(pos) );
}

int CDXFchain::GetObjectCount(void)
{
	return GetCount();
}

tuple<CDXFdata*, double> CDXFchain::GetSelectViewGap
	(const CPointD& pt, const CRectD& rcView)
{
	double	dGap, dGapMin = HUGE_VAL;
	CDXFdata*	pData;
	CDXFdata*	pDataResult = NULL;

	// 指定座標に一番近いｵﾌﾞｼﾞｪｸﾄを検索
	for ( POSITION pos=GetHeadPosition(); pos; ) {
		pData = GetNext(pos);
		if ( rcView.PtInRect(pData->GetMaxRect()) ) {
			dGap = pData->GetSelectPointGap(pt);
			if ( dGap < dGapMin ) {
				dGapMin = dGap;
				pDataResult = pData;
			}
		}
	}

	return make_tuple(pDataResult, dGapMin);
}

tuple<CDXFdata*, double> CDXFchain::GetSelectMakeGap
	(const CPointD& pt, BOOL bObject)
{
	CDXFdata*	pDataResult = NULL;
	double	dGapMin = HUGE_VAL;

	return make_tuple(pDataResult, dGapMin);
}

void CDXFchain::SetShapeSwitch(BOOL bSelect)
{
	for ( POSITION pos=GetHeadPosition(); pos; )
		GetNext(pos)->SetSelectFlg(bSelect);
}

void CDXFchain::RemoveObject(const CDXFdata* pData)
{
	POSITION	pos1=GetHeadPosition(), pos2;
	while ( (pos2=pos1) ) {
		if ( pData == GetNext(pos1) ) {
			RemoveAt(pos2);
			break;
		}
	}
}

void CDXFchain::DrawShape(CDC* pDC)
{
	CDXFdata*	pData;
	DWORD	dwSel, dwSelBak = 0;

	CBrush* pOldBrush = (CBrush *)pDC->SelectStockObject(NULL_BRUSH);
	CPen* pOldPen = pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenDXF(DXFPEN_CUTTER));
	pDC->SetROP2(R2_COPYPEN);
	for ( POSITION pos=GetHeadPosition(); pos; ) {
		pData = GetNext(pos);
		dwSel = pData->GetSelectFlg() & DXFSEL_SELECT;
		if ( dwSel != dwSelBak ) {
			dwSelBak = dwSel;
			pDC->SelectObject(pData->GetDrawPen());
		}
		pData->Draw(pDC);
	}
	pDC->SelectObject(pOldPen);
	pDC->SelectObject(pOldBrush);
}

/////////////////////////////////////////////////////////////////////////////
// ＤＸＦデータの形状集合クラス
/////////////////////////////////////////////////////////////////////////////
CDXFshape::CDXFshape()
{
	Constructor(DXFSHAPE_OUTLINE, NULL, 0);
}

CDXFshape::CDXFshape(DXFSHAPE_ASSEMBLE enAssemble, LPCTSTR lpszShape, DWORD dwFlags, CDXFchain* pChain)
{
	m_vShape = pChain;
	Constructor(enAssemble, lpszShape, dwFlags);
	// 座標ﾏｯﾌﾟのｵﾌﾞｼﾞｪｸﾄ最大矩形と所属ﾏｯﾌﾟの更新
	SetDetailInfo(pChain);
}

CDXFshape::CDXFshape(DXFSHAPE_ASSEMBLE enAssemble, LPCTSTR lpszShape, DWORD dwFlags, CDXFmap* pMap)
{
	m_vShape = pMap;
	Constructor(enAssemble, lpszShape, dwFlags);
	// 座標ﾏｯﾌﾟのｵﾌﾞｼﾞｪｸﾄ最大矩形と所属ﾏｯﾌﾟの更新
	SetDetailInfo(pMap);
}

void CDXFshape::Constructor(DXFSHAPE_ASSEMBLE enAssemble, LPCTSTR lpszShape, DWORD dwFlags)
{
	m_enAssemble = enAssemble;
	m_dwFlags = dwFlags;
	m_hTree = NULL;
	if ( lpszShape && lstrlen(lpszShape) > 0 )
		m_strShape = lpszShape;
	// ｵﾌﾞｼﾞｪｸﾄ矩形の初期化
	m_rcMax.left = m_rcMax.top = DBL_MAX;
	m_rcMax.right = m_rcMax.bottom = -DBL_MAX;
}

void CDXFshape::SetDetailInfo(CDXFchain* pChain)
{
	CDXFdata*	pData;
	for ( POSITION pos=pChain->GetHeadPosition(); pos; ) {
		pData = pChain->GetNext(pos);
		m_rcMax |= pData->GetMaxRect();
		pData->SetParentMap(this);
	}
}

void CDXFshape::SetDetailInfo(CDXFmap* pMap)
{
	CDXFarray*	pArray;
	CDXFdata*	pData;
	CPointD	pt;
	int		i;
	for ( POSITION pos=pMap->GetStartPosition(); pos; ) {
		pMap->GetNextAssoc(pos, pt, pArray);
		for ( i=0; i<pArray->GetSize(); i++ ) {
			pData = pArray->GetAt(i);
			m_rcMax |= pData->GetMaxRect();
			pData->SetParentMap(this);
		}
	}
}

CDXFshape::~CDXFshape()
{
	for ( POSITION pos=m_ltWork.GetHeadPosition(); pos; )
		delete	m_ltWork.GetNext(pos);
	if ( m_vShape.which() == 0 )
		delete	get<CDXFchain*>(m_vShape);
	else
		delete	get<CDXFmap*>(m_vShape);
}

void CDXFshape::Serialize(CArchive& ar)
{
	WORD	nType, nAssemble;

	if ( ar.IsStoring() ) {
		nType = m_vShape.which();
		nAssemble = m_enAssemble;
		ar << (m_dwFlags & ~DXFMAPFLG_SELECT) << nAssemble << m_strShape;	// 選択状態除く
		ar << m_rcMax.left << m_rcMax.top << m_rcMax.right << m_rcMax.bottom;
		ar << nType;
		if ( nType == 0 )
			get<CDXFchain*>(m_vShape)->Serialize(ar);
		else
			get<CDXFmap*>(m_vShape)->Serialize(ar);
	}
	else {
		CLayerData*	pLayer = (CLayerData *)(ar.m_pDocument);
		ar >> m_dwFlags >> nAssemble >> m_strShape;
		ar >> m_rcMax.left >> m_rcMax.top >> m_rcMax.right >> m_rcMax.bottom;
		ar >> nType;
		m_enAssemble = (DXFSHAPE_ASSEMBLE)nAssemble;
		if ( nType == 0 ) {
			CDXFchain* pChain = new CDXFchain;
			m_vShape = pChain;
			pChain->Serialize(ar);
			SetDetailInfo(pChain);
		}
		else {
			CDXFmap* pMap = new CDXFmap;
			m_vShape = pMap;
			pMap->Serialize(ar);
			SetDetailInfo(pMap);
		}
		// CDXFworkingｼﾘｱﾗｲｽﾞ情報用にCDXFshape*をCLayerData::m_pActiveMapに格納
		pLayer->SetActiveShape(this);
	}
	// 加工指示情報のｼﾘｱﾗｲｽﾞ
	m_ltWork.Serialize(ar);
}

BOOL CDXFshape::AddWorkingData(CDXFworking* pWork)
{
	// 例外は上位で行う
	m_ltWork.AddTail(pWork);
	// ﾏｯﾌﾟﾌﾗｸﾞとﾀｰｹﾞｯﾄｵﾌﾞｼﾞｪｸﾄのﾌﾗｸﾞを設定
	ENWORKINGTYPE nType = pWork->GetWorkingType();
	m_dwFlags |= g_dwMapFlag[nType];
	CDXFdata*	pData = pWork->GetTargetObject();
	if ( pData && nType==DIRECTION )
		pData->SetWorkingFlag(DXFSEL_DIRECTIONFIX);
	return TRUE;
}

BOOL CDXFshape::DelWorkingData(CDXFworking* pDelWork, CDXFshape* pShape/*=NULL*/)
{
	CDXFworking*	pWork;
	CDXFdata*		pData;
	ENWORKINGTYPE	nType;
	POSITION	pos1, pos2;
	BOOL	bResult = FALSE;

	for ( pos1=m_ltWork.GetHeadPosition(); (pos2=pos1); ) {
		pWork = m_ltWork.GetNext(pos1);
		if ( pWork == pDelWork ) {
			m_ltWork.RemoveAt(pos2);
			nType = pWork->GetWorkingType();
			// 各加工指示ごとにﾏｯﾌﾟﾌﾗｸﾞをｸﾘｱ
			m_dwFlags &= ~g_dwMapFlag[nType];
			if ( pShape ) {
				// 付け替え { LinkShape, CDXFView::OnLButtonUp_Sel() のみ }
				pShape->AddWorkingData(pWork);
			}
			else {
				// ｵﾌﾞｼﾞｪｸﾄの消去
				pData = pWork->GetTargetObject();
				if ( pData && nType==DIRECTION )
					pData->SetWorkingFlag(DXFSEL_DIRECTIONFIX, FALSE);
				delete	pWork;
			}
			bResult = TRUE;
			break;
		}
	}

	return bResult;
}

tuple<CDXFworking*, CDXFdata*> CDXFshape::GetDirectionObject(void) const
{
	CDXFworking*	pWork;
	CDXFdata*		pData = NULL;

	for ( POSITION pos=m_ltWork.GetHeadPosition(); pos; ) {
		pWork = m_ltWork.GetNext(pos);
		if ( pWork->GetWorkingType() == DIRECTION ) {
			pData = pWork->GetTargetObject();
			break;
		}
	}

	return make_tuple(pWork, pData);
}

void CDXFshape::RemoveExceptDirection(void)
{
	CDXFworking*	pWork;
	ENWORKINGTYPE	nType;
	POSITION	pos1=m_ltWork.GetHeadPosition(), pos2;
	while ( (pos2=pos1) ) {
		pWork = m_ltWork.GetNext(pos1);
		nType = pWork->GetWorkingType();
		if ( nType != DIRECTION ) {
			m_dwFlags &= ~g_dwMapFlag[nType];
			delete	pWork;
			m_ltWork.RemoveAt(pos2);
		}
	}
}

BOOL CDXFshape::LinkObject(void)
{
	CDXFmap*	pMap = GetShapeMap();

	// 集合検査
	if ( pMap ) {
		if ( pMap->GetShapeFlag() == 0 ) {
			// CDXFmapからCDXFchainに昇格
			if ( !ChangeCreate_MapToChain(pMap) )
				return FALSE;
			delete	pMap;
		}
	}
	else {
		// CDXFchainからは必ずCDXFmapに降格
		CDXFchain* pChain = GetShapeChain();
		if ( !ChangeCreate_ChainToMap(pChain) )
			return FALSE;
		delete	pChain;
	}

	return TRUE;
}

BOOL CDXFshape::ChangeCreate_MapToChain(CDXFmap* pMap)
{
	CDXFchain*	pChain = NULL;

	try {
		// 新たな形状集合を生成
		pChain = new CDXFchain;
		pMap->Copy_ToChain(pChain);
		// CDXFshape 情報更新
		m_enAssemble = DXFSHAPE_OUTLINE;
		m_vShape = pChain;
		m_dwFlags = 0;
		m_strShape += "(昇格)";
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		if ( pChain )
			delete	pChain;
		return FALSE;
	}

	return TRUE;
}

BOOL CDXFshape::ChangeCreate_ChainToMap(CDXFchain* pChain)
{
	CDXFmap*	pMap = NULL;

	try {
		// 新たな形状集合を生成
		pMap = new CDXFmap;
		pChain->Copy_ToMap(pMap);
		// CDXFshape 情報更新
		m_enAssemble = DXFSHAPE_LOCUS;
		m_vShape = pMap;
		m_dwFlags = pMap->GetShapeFlag();
		m_strShape += "(降格)";
		// 適さない加工指示(方向指示以外)を削除
		RemoveExceptDirection();
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		if ( pMap )
			delete	pMap;
		return FALSE;
	}

	return TRUE;
}

BOOL CDXFshape::LinkShape(CDXFshape* pShape)
{
	CDXFmap*	pMapTmp = NULL;
	CDXFmap*	pMap;
	CDXFchain*	pChainOrg;
	CDXFchain*	pChain;
	CPointD		pt;
	POSITION	pos;
	BOOL		bResult = FALSE;

	try {
		// 自分自身をCDXFmap一時領域へ
		pMapTmp = new CDXFmap;
		pChainOrg = GetShapeChain();
		if ( pChainOrg )
			pChainOrg->Copy_ToMap(pMapTmp);
		else
			pMapTmp->Append(GetShapeMap());
		// 結合出来るかどうか(座標検索)
		pChain = pShape->GetShapeChain();
		if ( pChain ) {
			for ( pos=pChain->GetHeadPosition(); pos; ) {
				pt = pChain->GetNext(pos)->GetNativePoint(1);
				if ( pMapTmp->PLookup(pt) ) {
					// １つでも見つかれば結合処理
					bResult = TRUE;
					pMapTmp->Append(pChain);
					break;
				}
			}
		}
		else {
			pMap = pShape->GetShapeMap();
			CDXFarray*	pDummy;
			for ( pos=pMap->GetStartPosition(); pos; ) {
				pMap->GetNextAssoc(pos, pt, pDummy);
				if ( pMapTmp->PLookup(pt) ) {
					bResult = TRUE;
					pMapTmp->Append(pMap);
					break;
				}
			}
		}
		if ( !bResult ) {
			delete	pMapTmp;
			return FALSE;
		}
		// 不要加工指示(方向指示以外)を削除
		RemoveExceptDirection();
		// 方向指示の統合
		CDXFworking*	pWork1;
		CDXFworking*	pWork2;
		CDXFdata*		pData1;
		CDXFdata*		pData2;
		tie(pWork1, pData1) = GetDirectionObject();
		tie(pWork2, pData2) = pShape->GetDirectionObject();
		if ( pData1 ) {
			// 自分自身に方向指示がある
			if ( pData2 ) {
				// 相手にも方向指示がある
				pShape->DelWorkingData(pWork2);
			}
		}
		else if ( pData2 ) {
			// 自分自身に方向指示がなく，相手にある
			pShape->DelWorkingData(pWork2, this);	// 方向指示の付け替え
		}
		// 集合検査
		DWORD dwFlag = pMapTmp->GetShapeFlag();
		if ( dwFlag == 0 ) {
			// 昇格
			pMap = pChainOrg ? NULL : GetShapeMap();
			if ( ChangeCreate_MapToChain(pMapTmp) ) {	// pMapTmpからCDXFchain生成
				// 元集合削除
				if ( pChainOrg )
					delete	pChainOrg;
				else
					delete	pMap;
			}
			else
				bResult = FALSE;
		}
		else {
			if ( pChainOrg ) {
				// 降格(CDXFchainからCDXFmapを生成しておいて)
				if ( ChangeCreate_ChainToMap(pChainOrg) ) {
					delete	pChainOrg;
					pMap = GetShapeMap();
					// 相手集合を追加
					pChain = pShape->GetShapeChain();
					if ( pChain )
						pMap->Append(pChain);
					else
						pMap->Append(pShape->GetShapeMap());
					// 形状ﾌﾗｸﾞの更新(ChangeCreate_ChainToMapでは出来ない)
					SetShapeFlag(dwFlag);
				}
				else
					bResult = FALSE;
			}
			else {
				// 現状維持(pMapTmpを上書きｺﾋﾟｰ)
				pMap = GetShapeMap();
				pMap->RemoveAll();
				pMap->Append(pMapTmp);
			}
		}
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		if ( pMapTmp )
			delete	pMapTmp;
		return FALSE;
	}

	if ( pMapTmp )
		delete	pMapTmp;

	return bResult;
}

void CDXFshape::AllChangeFactor(double dFactor)
{
	for ( POSITION pos=m_ltWork.GetHeadPosition(); pos; )
		m_ltWork.GetNext(pos)->DrawTuning(dFactor);
}

void CDXFshape::DrawWorking(CDC* pDC)
{
	CDXFworking*	pWork;
	int		i = 0, j;
	CPen*	pPen[] = {
		AfxGetNCVCMainWnd()->GetPenDXF(DXFPEN_WORKER),
		AfxGetNCVCMainWnd()->GetPenCom(COMPEN_SEL)
	};
	CPen*	pOldPen = pDC->SelectObject(pPen[i]);
	pDC->SetROP2(R2_COPYPEN);
	for ( POSITION pos = m_ltWork.GetHeadPosition(); pos; ) {
		pWork = m_ltWork.GetNext(pos);
		j = pWork->GetWorkingFlag() & DXFWORKFLG_SELECT ? 1 : 0;
		if ( i != j ) {
			i = j;
			pDC->SelectObject( pPen[i] );
		}
		pWork->Draw(pDC);
	}
	pDC->SelectObject(pOldPen);
}

int CDXFshape::GetObjectCount(void) const
{
	return apply_visitor( GetObjectCount_Visitor(), m_vShape );
}

tuple<CDXFdata*, double> CDXFshape::GetSelectViewGap
	(const CPointD& pt, const CRectD& rcView)
{
	if ( m_vShape.which() == 0 )
		return get<CDXFchain*>(m_vShape)->GetSelectViewGap(pt, rcView);
	else
		return get<CDXFmap*>(m_vShape)->GetSelectViewGap(pt, rcView);
}

tuple<CDXFdata*, double> CDXFshape::GetSelectMakeGap
	(const CPointD& pt, BOOL bObject)
{
	if ( m_vShape.which() == 0 )
		return get<CDXFchain*>(m_vShape)->GetSelectMakeGap(pt, bObject);
	else
		return get<CDXFmap*>(m_vShape)->GetSelectMakeGap(pt, bObject);
}

void CDXFshape::SetShapeSwitch(BOOL bSelect)
{
	if ( m_vShape.which() == 0 )
		get<CDXFchain*>(m_vShape)->SetShapeSwitch(bSelect);
	else
		get<CDXFmap*>(m_vShape)->SetShapeSwitch(bSelect);
}

void CDXFshape::RemoveObject(const CDXFdata* pData)
{
	if ( m_vShape.which() == 0 )
		get<CDXFchain*>(m_vShape)->RemoveObject(pData);
	else
		get<CDXFmap*>(m_vShape)->RemoveObject(pData);
}

void CDXFshape::DrawShape(CDC* pDC)
{
	if ( m_vShape.which() == 0 )
		get<CDXFchain*>(m_vShape)->DrawShape(pDC);
	else
		get<CDXFmap*>(m_vShape)->DrawShape(pDC);
}
