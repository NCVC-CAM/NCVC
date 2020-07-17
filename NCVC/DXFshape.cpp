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
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
//#define	_DEBUGDRAW_DXF
#endif

using namespace boost;

IMPLEMENT_DYNAMIC(CDXFworking, CObject)
IMPLEMENT_SERIAL(CDXFworkingDirection, CDXFworking, NCVCSERIALVERSION|VERSIONABLE_SCHEMA)
IMPLEMENT_SERIAL(CDXFworkingStart, CDXFworking, NCVCSERIALVERSION|VERSIONABLE_SCHEMA)
IMPLEMENT_SERIAL(CDXFworkingOutline, CDXFworking, NCVCSERIALVERSION|VERSIONABLE_SCHEMA)
IMPLEMENT_SERIAL(CDXFworkingPocket, CDXFworking, NCVCSERIALVERSION|VERSIONABLE_SCHEMA)
IMPLEMENT_SERIAL(CDXFmap, CMapPointToDXFarray, NCVCSERIALVERSION|VERSIONABLE_SCHEMA)
IMPLEMENT_SERIAL(CDXFchain, CDXFlist, NCVCSERIALVERSION|VERSIONABLE_SCHEMA)
IMPLEMENT_SERIAL(CDXFshape, CObject, NCVCSERIALVERSION|VERSIONABLE_SCHEMA)

/////////////////////////////////////////////////////////////////////////////
// 静的変数の初期化
double		CDXFmap::ms_dTolerance = NCMIN;

extern	DWORD	g_dwCamVer;		// NCVC.cpp

static	DWORD	g_dwMapFlag[] = {
	DXFMAPFLG_DIRECTION, DXFMAPFLG_START,
	DXFMAPFLG_OUTLINE, DXFMAPFLG_POCKET
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
	int	nStart = strWork.Find('\n') + 1,				// '\n' の次から
		nCount = strWork.Find('(', nStart) - nStart;	// '('  まで
	m_strWorking += strWork.Mid(nStart, nCount);
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
		CLayerData*	pLayer = reinterpret_cast<CLayerData *>(ar.m_pDocument);
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
CDXFworkingDirection::CDXFworkingDirection
	(CDXFshape* pShape, CDXFdata* pData, CPointD pts, CPointD pte[]) :
		CDXFworking(WORK_DIRECTION, pShape, pData, 0)
{
	m_ptStart = pts;
	for ( int i=0; i<SIZEOF(m_ptArraw); i++ )
		m_ptArraw[i] = pte[i];
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
		ar << m_ptStart.x << m_ptStart.y;
		for ( i=0; i<SIZEOF(m_ptArraw); i++ )
			ar << m_ptArraw[i].x << m_ptArraw[i].y;
	}
	else {
		ar >> m_ptStart.x >> m_ptStart.y;
		for ( i=0; i<SIZEOF(m_ptArraw); i++ )
			ar >> m_ptArraw[i].x >> m_ptArraw[i].y;
	}
}

//////////////////////////////////////////////////////////////////////
// ＤＸＦデータの「開始位置」指示クラス
//////////////////////////////////////////////////////////////////////
CDXFworkingStart::CDXFworkingStart
	(CDXFshape* pShape, CDXFdata* pData, CPointD pts) :
		CDXFworking(WORK_START, pShape, pData, 0)
{
	m_ptStart = pts;
}

void CDXFworkingStart::DrawTuning(double f)
{
	CPointD	pt( m_ptStart * f ); 
	// 位置を表す丸印は常に2.5論理理位 (CDXFpoint準拠)
	m_rcDraw.TopLeft()		= pt - LOMETRICFACTOR*2.5;
	m_rcDraw.BottomRight()	= pt + LOMETRICFACTOR*2.5;
}

void CDXFworkingStart::Draw(CDC* pDC) const
{
	pDC->Ellipse(&m_rcDraw);
}

void CDXFworkingStart::Serialize(CArchive& ar)
{
	CDXFworking::Serialize(ar);
	if ( ar.IsStoring() )
		ar << m_ptStart.x << m_ptStart.y;
	else
		ar >> m_ptStart.x >> m_ptStart.y;
}

//////////////////////////////////////////////////////////////////////
// ＤＸＦデータの「輪郭」加工指示クラス
//////////////////////////////////////////////////////////////////////
CDXFworkingOutline::CDXFworkingOutline(CDXFshape* pShape, const CDXFchain* pOutline, DWORD dwFlags) :
	CDXFworking(WORK_OUTLINE, pShape, NULL, dwFlags)
{
	m_ltOutline.AddTail(const_cast<CDXFchain*>(pOutline));
}

CDXFworkingOutline::~CDXFworkingOutline()
{
	CDXFdata*	pData;
	for ( POSITION pos=m_ltOutline.GetHeadPosition(); pos; ) {
		pData = m_ltOutline.GetNext(pos);
		if ( pData )
			delete	pData;
	}
}

void CDXFworkingOutline::DrawTuning(double f)
{
	CDXFdata*	pData;
	for ( POSITION pos=m_ltOutline.GetHeadPosition(); pos; ) {
		pData = m_ltOutline.GetNext(pos);
		if ( pData )
			pData->DrawTuning(f);
	}
}

void CDXFworkingOutline::Draw(CDC* pDC) const
{
	CDXFdata*	pData;
	for ( POSITION pos=m_ltOutline.GetHeadPosition(); pos; ) {
		pData = m_ltOutline.GetNext(pos);
		if ( pData )
			pData->Draw(pDC);
//		else			// 分離集合ﾃﾞﾊﾞｯｸﾞ
//			break;
	}
}

void CDXFworkingOutline::Serialize(CArchive& ar)
{
	CDXFworking::Serialize(ar);

	DWORD		dwCnt = 0;	// ﾃﾞｰﾀの切れ目回数
	CDWordArray	obGap;		// ﾃﾞｰﾀの切れ目一覧

	// 輪郭ｵﾌﾞｼﾞｪｸﾄのｼﾘｱﾗｲｽﾞ
	// m_ltOutline.Serialize(ar); では CDXFchain を呼び，
	// ｵﾌﾞｼﾞｪｸﾄへの SeqNo. で処理される．ゆえに自力ｺｰﾃﾞｨﾝｸﾞ
	if ( ar.IsStoring() ) {
		POSITION	pos;
		CDXFdata*	pData;
		// ﾃﾞｰﾀの切れ目(NULL値)の記録
		for ( pos=m_ltOutline.GetHeadPosition(); pos; dwCnt++ ) {
			pData = m_ltOutline.GetNext(pos);
			if ( !pData )
				obGap.Add(dwCnt);
		}
		obGap.Serialize(ar);
		// ﾃﾞｰﾀ本体
		ar << m_ltOutline.GetCount();
		for ( pos=m_ltOutline.GetHeadPosition(); pos; ) {
			pData = m_ltOutline.GetNext(pos);
			if ( pData )
				ar << pData;
		}
	}
	else {
		INT_PTR		i, n = 0, nLoop;
		CObject*	pNewData;	// CDXFdata*
		// ﾃﾞｰﾀの切れ目(NULL値)の読み込み
		if ( g_dwCamVer > NCVCSERIALVERSION_1503 )	// Ver1.10〜
			obGap.Serialize(ar);
		dwCnt = obGap.IsEmpty() ? -1 : obGap[0];
		// ﾃﾞｰﾀ本体
		ar >> nLoop;
		for ( i=0; i<nLoop; i++ ) {
			if ( i == dwCnt ) {
				m_ltOutline.AddTail( (CDXFdata *)NULL );
				dwCnt = n < obGap.GetUpperBound() ? obGap[++n] : -1;
			}
			else {
				ar >> pNewData;
				m_ltOutline.AddTail( (CDXFdata *)pNewData );
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////
// ＤＸＦデータの「ポケット」加工指示クラス
//////////////////////////////////////////////////////////////////////
CDXFworkingPocket::CDXFworkingPocket(CDXFshape* pShape, DWORD dwFlags) :
	CDXFworking(WORK_POCKET, pShape, NULL, dwFlags)
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
	int			i, nDataCnt;
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
		CLayerData*	pLayer = reinterpret_cast<CLayerData *>(ar.m_pDocument);
		ar >> nMapLoop >> nHash;
		InitHashTable(nHash);
		while ( nMapLoop-- ) {
			ar >> pt.x >> pt.y >> nDataCnt;
			if ( nDataCnt > 0 ) {
				pArray = new CDXFarray;
				pArray->SetSize(0, nDataCnt);
				while ( nDataCnt-- ) {
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

void CDXFmap::SetMakePointMap(CDXFdata* pData)
{
	CDXFarray*	pArray;
	CPointD	pt;

	// 各ｵﾌﾞｼﾞｪｸﾄ頂点の座標登録
	for ( int i=0; i<pData->GetPointNumber(); i++ ) {
		pt = pData->GetTunPoint(i);		// 原点調整座標
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
	if ( nOddCnt == 0 ) {		// 奇数がゼロ
		ASSERT( pStartArray2 );
		bEuler = TRUE;		// 一筆書き要件を満たしている
		// 奇数座標がない場合は偶数座標の最も近いｵﾌﾞｼﾞｪｸﾄ配列を使う
		pStartArray = pStartArray2;
		ptStart = ptStart2;
	}
	else if ( nOddCnt == 2 ) {	// 奇点が２個
		ASSERT( pStartArray );
		bEuler = TRUE;		// 一筆書き要件を満たしている
	}

	return make_tuple(bEuler, pStartArray, ptStart);
}

DWORD CDXFmap::GetMapTypeFlag(void) const
{
	// １つの座標に３つ以上のｵﾌﾞｼﾞｪｸﾄがあれば
	//		DXFMAPFLG_CANNOTWORKING -> 加工指示できない
	// 交点や端点があれば
	//		DXFMAPFLG_CANNOTOUTLINE -> 方向指示は可能，輪郭・ﾎﾟｹｯﾄ加工ができない

	// 所属ｵﾌﾞｼﾞｪｸﾄが１つ かつ それが円なら
	if ( GetObjectCount()==1 && GetFirstObject()->GetType()==DXFCIRCLEDATA )
		return 0;

	int			i, j, nLoop;
	DWORD		dwFlags = 0;
	CDXFarray	obWorkArray;
	CDXFarray*	pArray;
	CDXFdata*	pData;
#ifdef _DEBUG
	CDXFdata*	pDataDbg;
#endif
	CPointD		pt, ptChk[4];

	obWorkArray.SetSize(0, GetSize());
	AllMapObject_ClearSearchFlg(FALSE);

	try {
		for ( POSITION pos=GetStartPosition(); pos; ) {
			GetNextAssoc(pos, pt, pArray);
			nLoop = pArray->GetSize();
			if ( nLoop == 1 )			// 端点
				dwFlags |= DXFMAPFLG_CANNOTOUTLINE;
			else if ( nLoop != 2 ) {	// ３つ以上のｵﾌﾞｼﾞｪｸﾄ
				dwFlags |= DXFMAPFLG_CANNOTWORKING;
				break;	// これ以上調査の必要なし
			}
			// 交点ﾁｪｯｸのためにｵﾌﾞｼﾞｪｸﾄをｺﾋﾟｰ
			if ( !dwFlags ) {
				for ( i=0; i<nLoop; i++ ) {
					pData = pArray->GetAt(i);
					if ( !pData->IsSearchFlg() ) {
						obWorkArray.Add(pData);		// ﾜｰｸ配列に登録
						pData->SetSearchFlg();
						pData->ClearMakeFlg();
					}
				}
			}
		}
		if ( !dwFlags ) {
			// 交点ﾁｪｯｸ
			nLoop = obWorkArray.GetSize();
			for ( i=0; i<nLoop; i++ ) {
				pData = obWorkArray[i];
				for ( j=i+1; j<nLoop; j++ ) {
#ifdef _DEBUG
					pDataDbg = obWorkArray[j];	// obWorkArray[j]値確認用
#endif
					if ( pData->GetIntersectionPoint(obWorkArray[j], ptChk) > 0 ) {
						dwFlags |= DXFMAPFLG_CANNOTOUTLINE;
						i = nLoop;	// 外側ﾙｰﾌﾟも終了
						break;
					}
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
		for ( i=0; i<pArray->GetSize(); i++ ) {
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

void CDXFmap::CopyToChain(CDXFchain* pChain)
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
				pData->SetDirectionFixed(pt);
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
	CDXFdata*	pData;
	for ( POSITION pos=pChain->GetHeadPosition(); pos; ) {
		pData = pChain->GetNext(pos);
		if ( pData )
			SetPointMap(pData);
	}
}

int CDXFmap::GetObjectCount(void) const
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

double CDXFmap::GetSelectObjectFromShape
	(const CPointD& pt, const CRectD* rcView/*=NULL*/, CDXFdata** pDataResult/*=NULL*/)
{
	int		i;
	CPointD	ptKey;
	double	dGap, dGapMin = HUGE_VAL;
	CDXFarray*	pArray;
	CDXFarray*	pArrayMin = NULL;
	CDXFdata*	pData;

	if ( pDataResult )
		*pDataResult = NULL;

	// 指定座標に一番近いｵﾌﾞｼﾞｪｸﾄを検索
	// --- 「一番近い座標ｷｰ」では端点の検索になってしまう ---
	for ( POSITION pos=GetStartPosition(); pos; ) {
		GetNextAssoc(pos, ptKey, pArray);
		for ( i=0; i<pArray->GetSize(); i++ ) {
			pData = pArray->GetAt(i);
			if ( rcView && !rcView->PtInRectpt(pData->GetMaxRect()) )
				continue;
			dGap = pData->GetSelectPointGap(pt);
			if ( dGap < dGapMin ) {
				dGapMin = dGap;
				if ( pDataResult )
					*pDataResult = pData;
			}
		}
	}

	return dGapMin;
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
		if ( Lookup(pt, pArray) ) {
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

void CDXFmap::DrawShape(CDC* pDC) const
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

void CDXFmap::OrgTuning(void)
{
	CDXFarray*	pArray;
	CPointD	pt;
	int		i;

	for ( POSITION pos=GetStartPosition(); pos; ) {
		GetNextAssoc(pos, pt, pArray);
		for ( i=0; i<pArray->GetSize(); i++ )
			pArray->GetAt(i)->OrgTuning(FALSE);
	}
}

/////////////////////////////////////////////////////////////////////////////
// ＤＸＦデータの輪郭集団クラス
/////////////////////////////////////////////////////////////////////////////
CDXFchain::CDXFchain()
{
	m_rcMax.SetRectMinimum();
	m_dwFlags = 0;
}

CDXFchain::~CDXFchain()
{
}

void CDXFchain::Serialize(CArchive& ar)
{
	CDXFdata*	pData;

	if ( ar.IsStoring() ) {
		ar << GetCount();
		for ( POSITION pos=GetHeadPosition(); pos; ) {
			pData = GetNext(pos);
			if ( pData )
				ar << pData->GetSerializeSeq();
			else
				ar << (DWORD)-1;
		}
	}
	else {
		INT_PTR		nListLoop;
		DWORD		nIndex;
		CLayerData*	pLayer = reinterpret_cast<CLayerData *>(ar.m_pDocument);
		ar >> nListLoop;
		while ( nListLoop-- ) {
			ar >> nIndex;
			if ( nIndex >= 0 ) {
				pData = pLayer->GetDxfData(nIndex);
				AddTail(pData);
			}
			else
				AddTail((CDXFdata *)NULL);
		}
	}
}

void CDXFchain::ReversPoint(void)
{
	// 全ｵﾌﾞｼﾞｪｸﾄの座標反転
	CDXFdata*	pData;
	for ( POSITION pos=GetHeadPosition(); pos; ) {
		pData = GetNext(pos);
		if ( pData )
			pData->ReversePt();
	}
}

void CDXFchain::CopyToMap(CDXFmap* pMap)
{
	CDXFdata*	pData;
	for ( POSITION pos=GetHeadPosition(); pos; ) {
		pData = GetNext(pos);
		if ( pData )
			pMap->SetPointMap(pData);
	}
}

int CDXFchain::GetObjectCount(void) const
{
	return GetCount();
}

double CDXFchain::GetSelectObjectFromShape
	(const CPointD& pt, const CRectD* rcView/*=NULL*/, CDXFdata** pDataResult/*=NULL*/)
{
	double	dGap, dGapMin = HUGE_VAL;
	CDXFdata*	pData;

	if ( pDataResult )
		*pDataResult = NULL;

	// 指定座標に一番近いｵﾌﾞｼﾞｪｸﾄを検索
	for ( POSITION pos=GetHeadPosition(); pos; ) {
		pData = GetNext(pos);
		if ( !pData )	// 分離ﾏｰｸ
			continue;
		if ( rcView && !rcView->PtInRectpt(pData->GetMaxRect()) )
			continue;
		dGap = pData->GetSelectPointGap(pt);
		if ( dGap < dGapMin ) {
			dGapMin = dGap;
			if ( pDataResult )
				*pDataResult = pData;
		}
	}

	return dGapMin;
}

void CDXFchain::SetShapeSwitch(BOOL bSelect)
{
	CDXFdata*	pData;
	for ( POSITION pos=GetHeadPosition(); pos; ) {
		pData = GetNext(pos);
		if ( pData )
			pData->SetSelectFlg(bSelect);
	}
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

void CDXFchain::DrawShape(CDC* pDC) const
{
	CDXFdata*	pData;
	DWORD	dwSel, dwSelBak = 0;

	CBrush* pOldBrush = (CBrush *)pDC->SelectStockObject(NULL_BRUSH);
	CPen* pOldPen = pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenDXF(DXFPEN_CUTTER));
	pDC->SetROP2(R2_COPYPEN);
	for ( POSITION pos=GetHeadPosition(); pos; ) {
		pData = GetNext(pos);
		if ( !pData )	// 分離ﾏｰｸ
			continue;
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

void CDXFchain::OrgTuning(void)
{
	CDXFdata*	pData;
	for ( POSITION pos=GetHeadPosition(); pos; ) {
		pData = GetNext(pos);
		if ( pData )
			pData->OrgTuning(FALSE);
	}
	// 自分自身のﾌﾗｸﾞも初期化
	m_dwFlags = 0;
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
	m_dOffset = 1.0;	// ﾃﾞﾌｫﾙﾄｵﾌｾｯﾄ値
	m_nInOut  = -1;		// ﾃﾞﾌｫﾙﾄ輪郭方向無し
	m_bAcute  = TRUE;
	m_hTree   = NULL;
	m_nSerialSeq = 0;
	if ( lpszShape && lstrlen(lpszShape) > 0 )
		m_strShape = lpszShape;
	// ｵﾌﾞｼﾞｪｸﾄ矩形の初期化
	m_rcMax.SetRectMinimum();
}

void CDXFshape::SetDetailInfo(CDXFchain* pChain)
{
	CDXFdata*	pData;
	for ( POSITION pos=pChain->GetHeadPosition(); pos; ) {
		pData = pChain->GetNext(pos);
		if ( pData ) {
			m_rcMax |= pData->GetMaxRect();
			pData->SetParentMap(this);
		}
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
		ar << m_dOffset << m_nInOut << m_bAcute;
		ar << nType;
		if ( nType == 0 )
			get<CDXFchain*>(m_vShape)->Serialize(ar);
		else
			get<CDXFmap*>(m_vShape)->Serialize(ar);
	}
	else {
		CLayerData*	pLayer = reinterpret_cast<CLayerData *>(ar.m_pDocument);
		ar >> m_dwFlags >> nAssemble >> m_strShape;
		ar >> m_dOffset >> m_nInOut;
		if ( g_dwCamVer > NCVCSERIALVERSION_1503 )	// Ver1.10〜
			ar >> m_bAcute;
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

BOOL CDXFshape::AddWorkingData(CDXFworking* pWork, int nInOut/*=-1*/)
{
	// 例外は上位で行う
	m_ltWork.AddTail(pWork);
	// ﾏｯﾌﾟﾌﾗｸﾞとﾀｰｹﾞｯﾄｵﾌﾞｼﾞｪｸﾄのﾌﾗｸﾞを設定
	ENWORKINGTYPE nType = pWork->GetWorkingType();
	m_dwFlags |= g_dwMapFlag[nType];
	m_nInOut   = nInOut;
	CDXFdata*	pData = pWork->GetTargetObject();
	if ( pData && nType==WORK_DIRECTION )
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
				if ( pData && nType==WORK_DIRECTION )
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
		if ( pWork->GetWorkingType() == WORK_DIRECTION ) {
			pData = pWork->GetTargetObject();
			break;
		}
	}

	return make_tuple(pWork, pData);
}

tuple<CDXFworking*, CDXFdata*> CDXFshape::GetStartObject(void) const
{
	CDXFworking*	pWork;
	CDXFdata*		pData = NULL;

	for ( POSITION pos=m_ltWork.GetHeadPosition(); pos; ) {
		pWork = m_ltWork.GetNext(pos);
		if ( pWork->GetWorkingType() == WORK_START ) {
			pData = pWork->GetTargetObject();
			break;
		}
	}

	return make_tuple(pWork, pData);
}

CDXFchain*	CDXFshape::GetOutlineObject(void) const
{
	CDXFworking*	pWork;
	CDXFchain*		pChain = NULL;

	for ( POSITION pos=m_ltWork.GetHeadPosition(); pos; ) {
		pWork = m_ltWork.GetNext(pos);
		if ( pWork->GetWorkingType() == WORK_OUTLINE ) {
			pChain = static_cast<CDXFworkingOutline*>(pWork)->GetOutlineChain();
			break;
		}
	}

	return pChain;
}

void CDXFshape::RemoveExceptDirection(void)
{
	CDXFworking*	pWork;
	ENWORKINGTYPE	nType;
	POSITION	pos1=m_ltWork.GetHeadPosition(), pos2;
	while ( (pos2=pos1) ) {
		pWork = m_ltWork.GetNext(pos1);
		nType = pWork->GetWorkingType();
		if ( nType > WORK_START ) {		// WORK_OUTLINE, WORK_POCKET
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
		if ( pMap->GetMapTypeFlag() == 0 ) {
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
		pMap->CopyToChain(pChain);
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
		pChain->CopyToMap(pMap);
		// CDXFshape 情報更新
		m_enAssemble = DXFSHAPE_LOCUS;
		m_vShape = pMap;
		m_dwFlags = pMap->GetMapTypeFlag();
		m_strShape += "(降格)";
		// 適さない加工指示(輪郭指示など)を削除
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
			pChainOrg->CopyToMap(pMapTmp);
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
		// 不要加工指示(輪郭指示など)を削除
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
		// 開始位置の統合
		tie(pWork1, pData1) = GetStartObject();
		tie(pWork2, pData2) = pShape->GetStartObject();
		if ( pData1 ) {
			if ( pData2 ) {
				pShape->DelWorkingData(pWork2);
			}
		}
		else if ( pData2 ) {
			pShape->DelWorkingData(pWork2, this);
		}

		// 集合検査
		DWORD dwFlag = pMapTmp->GetMapTypeFlag();
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

BOOL CDXFshape::CreateOutlineTempObject(BOOL bLeft, CDXFchain* pResult)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CreateOutlineTempObject()", DBG_MAGENTA);
	CPointD		ptDbg1, ptDbg2, ptDbg3;
#endif
	// 占有矩形領域の初期化
	pResult->ClearMaxRect();

	const CDXFchain*	pChain = GetShapeChain();
	if ( !pChain || pChain->IsEmpty() )
		return FALSE;

	CTypedPtrArrayEx<CPtrArray, CDXFlist*>	obSepArray;

	int			k = bLeft ? -1 : 1;
	BOOL		bResult = TRUE;
	CDXFdata*	pData;
	CPointD		pt, pte;
	optional<CPointD>	ptResult, pts;

	// 唯一のｵﾌﾞｼﾞｪｸﾄが円ﾃﾞｰﾀなら
	if ( pChain->GetCount() == 1 ) {
		pData = pChain->GetHead();
		if ( pData->GetType() == DXFCIRCLEDATA ) {
			// ｵﾌｾｯﾄ半径のﾏｲﾅｽﾁｪｯｸ
			if ( static_cast<CDXFcircle*>(pData)->GetR()+m_dOffset*k < EPS )
				return TRUE;	// ｵﾌｾｯﾄｵﾌﾞｼﾞｪｸﾄを生成しない
			pData = CreateOutlineTempObject_new(pData, pte, pte, k);
			if ( pData ) {
				pResult->AddTail(pData);
				pResult->SetMaxRect(pData);
				return TRUE;
			}
		}
		return FALSE;	// 円ﾃﾞｰﾀ以外はｴﾗｰ
	}

	// 輪郭ｵﾌﾞｼﾞｪｸﾄﾙｰﾌﾟ準備
	POSITION	pos = pChain->GetHeadPosition();
	CDXFdata*	pData1 = pChain->GetNext(pos);	// 最初のｵﾌﾞｼﾞｪｸﾄ
	CDXFdata*	pData2;

	// CDXFchainは，始点終点の調整が行われている(SetDirectionFixed)ので
	// 単純ﾙｰﾌﾟで良い
	while ( pos ) {		// ２回目以降からﾙｰﾌﾟ
		pData2 = pChain->GetNext(pos);
#ifdef _DEBUG
		ptDbg1 = pData1->GetNativePoint(0);
		ptDbg2 = pData1->GetNativePoint(1);
		ptDbg3 = pData2->GetNativePoint(1);
		dbg.printf("p1=(%.3f, %.3f) p2=(%.3f, %.3f) p3=(%.3f, %.3f)",
			ptDbg1.x, ptDbg1.y, ptDbg2.x, ptDbg2.y, ptDbg3.x, ptDbg3.y);
#endif
		// ｵﾌｾｯﾄ座標計算
		ptResult = pData1->CalcOffsetIntersectionPoint(pData2, m_dOffset, bLeft);
		if ( !ptResult ) {
			bResult = FALSE;
			break;
		}
		pt = *ptResult;
#ifdef _DEBUG
		dbg.printf("Offset=(%.3f, %.3f)", pt.x, pt.y);
#endif
		// 輪郭ｵﾌﾞｼﾞｪｸﾄ生成(初回は無視)
		if ( pts ) {
			pData = CreateOutlineTempObject_new(pData1, *pts, pt, k);
			if ( pData ) {
				pResult->AddTail(pData);
				pResult->SetMaxRect(pData);
				// ｵﾌｾｯﾄｵﾌﾞｼﾞｪｸﾄの交点検査
				if ( !SeparateOutlineIntersection(pResult, obSepArray) ) {
					bResult = FALSE;
					break;
				}
			}
			else {
				bResult = FALSE;
				break;
			}
		}
		else
			pte = pt;		// 最初の輪郭ｵﾌﾞｼﾞｪｸﾄの終点
		pts = pt;
		pData1 = pData2;
	}
	if ( !bResult ) {
		// ｴﾗｰﾘｶﾊﾞﾘ
		for ( k=0; k<obSepArray.GetSize(); k++ ) {
			for ( pos=obSepArray[k]->GetHeadPosition(); pos; )
				delete	obSepArray[k]->GetNext(pos);
			delete	obSepArray[k];
		}
		return FALSE;
	}

	// 残りの輪郭ｵﾌﾞｼﾞｪｸﾄ生成(最後のｵﾌﾞｼﾞｪｸﾄと先頭のｵﾌﾞｼﾞｪｸﾄ)
	pData2 = pChain->GetHead();
#ifdef _DEBUG
	ptDbg1 = pData1->GetNativePoint(0);
	ptDbg2 = pData1->GetNativePoint(1);
	ptDbg3 = pData2->GetNativePoint(1);
	dbg.printf("p1=(%.3f, %.3f) p2=(%.3f, %.3f) p3=(%.3f, %.3f)",
		ptDbg1.x, ptDbg1.y, ptDbg2.x, ptDbg2.y, ptDbg3.x, ptDbg3.y);
#endif
	ptResult = pData1->CalcOffsetIntersectionPoint(pData2, m_dOffset, bLeft);
	if ( ptResult ) {
		pt = *ptResult;
#ifdef _DEBUG
		dbg.printf("Offset=(%.3f, %.3f)", pt.x, pt.y);
#endif
		pData = CreateOutlineTempObject_new(pData1, *pts, pt, k);
		if ( pData ) {
			pResult->AddTail(pData);
			pResult->SetMaxRect(pData);
			if ( !SeparateOutlineIntersection(pResult, obSepArray) )
				bResult = FALSE;
		}
		else
			bResult = FALSE;
	}
	else
		bResult = FALSE;
	if ( !bResult ) {
		for ( k=0; k<obSepArray.GetSize(); k++ ) {
			for ( pos=obSepArray[k]->GetHeadPosition(); pos; )
				delete	obSepArray[k]->GetNext(pos);
			delete	obSepArray[k];
		}
		return FALSE;
	}

	// 最初のｵﾌﾞｼﾞｪｸﾄ(の終点で)
	pData = CreateOutlineTempObject_new(pData2, pt, pte, k);
	if ( pData ) {
		pResult->AddTail(pData);
		pResult->SetMaxRect(pData);
		if ( !SeparateOutlineIntersection(pResult, obSepArray) )
			bResult = FALSE;
	}
	else
		bResult = FALSE;
	if ( !bResult ) {
		for ( k=0; k<obSepArray.GetSize(); k++ ) {
			for ( pos=obSepArray[k]->GetHeadPosition(); pos; )
				delete	obSepArray[k]->GetNext(pos);
			delete	obSepArray[k];
		}
		return FALSE;
	}

	// 本集合の検査
	CheckSeparateChain(pResult);

	// 分離集合の検査
	for ( k=0; k<obSepArray.GetSize(); k++ ) {
		CheckSeparateChain(obSepArray[k]);
		// 分離集合を末尾に結合
		if ( !obSepArray[k]->IsEmpty() ) {
			if ( !pResult->IsEmpty() )
				pResult->AddTail((CDXFdata *)NULL);		// 分離ﾏｰｸ
			pResult->AddTail(obSepArray[k]);
		}
		delete	obSepArray[k];
	}

	return TRUE;
}

CDXFdata* CDXFshape::CreateOutlineTempObject_new
	(const CDXFdata* pDataSrc, const CPointD& pts, const CPointD& pte, int k) const
{
	CDXFdata*	pData = NULL;
	DXFLARGV	dxfLine;
	DXFCARGV	dxfCircle;
	DXFAARGV	dxfArc;

	switch ( pDataSrc->GetType() ) {
	case DXFLINEDATA:
		dxfLine.pLayer = pDataSrc->GetParentLayer();
		dxfLine.s = pts;
		dxfLine.e = pte;
		pData = new CDXFline(&dxfLine);
		break;
	case DXFCIRCLEDATA:
		pData = const_cast<CDXFdata*>(pDataSrc);
		dxfCircle.c = static_cast<CDXFcircle*>(pData)->GetCenter();
		dxfCircle.r = static_cast<CDXFcircle*>(pData)->GetR() + m_dOffset * k;
		pData = new CDXFcircle(&dxfCircle);
		break;
	case DXFARCDATA:
		{
			const CDXFarc* pArc = static_cast<const CDXFarc*>(pDataSrc);
			BOOL	bRound = pArc->GetRound();
			if ( !bRound && k!=0 )	// 左方向を基準
				k = -k;
			dxfArc.c = pArc->GetCenter();
			dxfArc.r = pArc->GetR() + m_dOffset * k;
			if ( (dxfArc.sq=atan2(pts.y - dxfArc.c.y, pts.x - dxfArc.c.x)) < 0.0 )
				dxfArc.sq += 360.0*RAD;
			if ( (dxfArc.eq=atan2(pte.y - dxfArc.c.y, pte.x - dxfArc.c.x)) < 0.0 )
				dxfArc.eq += 360.0*RAD;
			// ｵﾘｼﾞﾅﾙ円弧とｵﾌｾｯﾄ円弧の回転方向ﾁｪｯｸ
			if ( k != 0 ) {
				optional<CPointD> ptResult = ::CalcIntersectionPoint_LL(
						pts, pArc->GetNativePoint(0),
						pte, pArc->GetNativePoint(1) );
				if ( ptResult )		// ｵﾘｼﾞﾅﾙ円弧とｵﾌｾｯﾄ円弧の回転方向が違う
					bRound = !bRound;
			}
			if ( bRound ) {
				// for CDXFarc::AngleTuning()
				while ( ::RoundUp(dxfArc.sq*DEG) > ::RoundUp(dxfArc.eq*DEG) )
					dxfArc.eq += 360.0*RAD;
			}
			else {
				while ( ::RoundUp(dxfArc.eq*DEG) > ::RoundUp(dxfArc.sq*DEG) )
					dxfArc.sq += 360.0*RAD;
			}
			pData = new CDXFarc(&dxfArc, bRound, pts, pte);
		}
		break;
	}

	return pData;
}

BOOL CDXFshape::SeparateOutlineIntersection
	(CDXFchain* pOffset, CTypedPtrArrayEx<CPtrArray, CDXFlist*>& obSepList)
{
	POSITION	pos1 = pOffset->GetTailPosition(), pos2, pos;
	CDXFdata*	pData;
	CDXFdata*	pData1;
	CDXFdata*	pData2;
	CDXFlist*	pSepList = NULL;
	CPointD		pt[4];	// 最大４交点
	int			nCnt, nLoop = 0;
	BOOL		bResult = TRUE;

	if ( pos1 )
		pData1 = pOffset->GetPrev(pos1);	// 検索対象ﾃﾞｰﾀ
	else
		return TRUE;

	for ( ; (pos2=pos1); nLoop++ ) {
		pData2 = pOffset->GetPrev(pos1);		// さかのぼって検索
		// ｵﾌｾｯﾄｵﾌﾞｼﾞｪｸﾄ同士の交点ﾁｪｯｸ(端点含む)
		nCnt = pData1->GetIntersectionPoint(pData2, pt, nLoop==0);	// 最初のﾙｰﾌﾟだけTRUE
		if ( nCnt <= 0 )
			continue;
		// --- 交点あり！！
		pSepList = new CDXFlist;
		// 1) 交点から pData2 の終点までｵﾌﾞｼﾞｪｸﾄ生成
		pData = CreateOutlineTempObject_new(pData2, pt[0], pData2->GetNativePoint(1), 0);
		if ( !pData ) {
			bResult = FALSE;
			break;
		}
		pSepList->AddTail(pData);
		// 2) pData2 の次から pData1 の手前まで分離集合に移動
		pos = pos2;
		pOffset->GetNext(pos);		// １つ進める
		while ( (pos2=pos) ) {
			pData = pOffset->GetNext(pos);
			if ( pData1 == pData )
				break;
			pSepList->AddTail(pData);
			pOffset->RemoveAt(pos2);
		}
		// 3) pData1 の始点から交点までｵﾌﾞｼﾞｪｸﾄ生成(pData1上書き禁止)
		pData = CreateOutlineTempObject_new(pData1, pData1->GetNativePoint(0), pt[0], 0);
		if ( !pData ) {
			bResult = FALSE;
			break;
		}
		pSepList->AddTail(pData);
		// 4) pData1/pData2 の始点/終点を更新
		pData1->SetNativePoint(0, pt[0]);
		pData2->SetNativePoint(1, pt[0]);
		// 5) 分離集合に追加
		obSepList.Add(pSepList);
		pSepList = NULL;
	}

	if ( !bResult && pSepList ) {
		// ｺﾞﾐ掃除
		for ( pos=pSepList->GetHeadPosition(); pos; )
			delete	pSepList->GetNext(pos);
	}

	return TRUE;
}

BOOL CDXFshape::CheckSeparateChain(CDXFlist* pResult)
{
	BOOL	bDelete  = TRUE;
	int		nCnt = 0;
	POSITION	pos1, pos2, pos;
	CDXFdata*	pData;
	CMapPtrToPtr	mp, mpDel;		// NG点のﾎﾟｼﾞｼｮﾝｷｰ

	// NG点検査
	for ( pos1=pResult->GetHeadPosition(); (pos2=pos1); ) {
		pData = pResult->GetNext(pos1);
		if ( CheckIntersectionCircle(pData->GetNativePoint(1)) )
			mp.SetAt(pos2, pData);		// １つでもNG点含む
		else {
			bDelete = FALSE;	// 全部NG点でない=>削除対象ではない
			nCnt++;				// OK点ｶｳﾝﾄ
		}
	}

	// 分離集合の削除対象検査
	if ( nCnt<=2 || bDelete ) {	// OK点2個以下もｱｳﾄ
		for ( pos1=pResult->GetHeadPosition(); pos1; )
			delete	pResult->GetNext(pos1);
		pResult->RemoveAll();
		return TRUE;
	}

	// --- NG点の除去
	LPVOID		pKey, pVoid;		// dummy

	// 連続NG点のｵﾌﾞｼﾞｪｸﾄを除去
	for ( pos1=mp.GetStartPosition(); pos1; ) {
		mp.GetNextAssoc(pos1, pKey, pVoid);
		pos2 = (POSITION)pKey;
		if ( mpDel.Lookup(pos2, pVoid) )
			continue;	// 削除処理済み
		pResult->GetNext(pos2);
		if ( !pos2 )
			pos2 = pResult->GetHeadPosition();
		while ( mp.Lookup(pos2, pVoid) ) {
			mpDel.SetAt(pos2, pVoid);	// ここでは mp.RemoveKey() できない
			pos = pos2;
			delete	pResult->GetNext(pos2);
			pResult->RemoveAt(pos);
			if ( !pos2 )
				pos2 = pResult->GetHeadPosition();
		}
	}
	for ( pos1=mpDel.GetStartPosition(); pos1; ) {	// 後片付け
		mpDel.GetNextAssoc(pos1, pKey, pVoid);
		mp.RemoveKey(pKey);
	}

	// NG点の枝刈り(終点でNG点を持つｵﾌﾞｼﾞｪｸﾄ)
	CDXFdata*	pData1;
	CDXFdata*	pData2;
	optional<CPointD>	ptResult;
	for ( pos1=mp.GetStartPosition(); pos1; ) {
		mp.GetNextAssoc(pos1, pKey, pVoid);
		pos2 = (POSITION)pKey;
		pData1 = pResult->GetNext(pos2);
		if ( !pos2 )
			pos2 = pResult->GetHeadPosition();
		pData2 = pResult->GetNext(pos2);
		// NG点を持つｵﾌﾞｼﾞｪｸﾄの長さﾁｪｯｸ
		if ( pData1->GetLength() <= pData2->GetLength() ) {
			// pData1の前とpData2の交点を計算
			pos2 = (POSITION)pKey;
			pData = pResult->GetPrev(pos2);		// pData1
			if ( !pos2 )
				pos2 = pResult->GetTailPosition();
			pData = pResult->GetPrev(pos2);
			ptResult = pData2->CalcExpandPoint(pData);
		}
		else {
			// pData2の次とpData1の交点を計算
			if ( !pos2 )
				pos2 = pResult->GetHeadPosition();
			pData = pResult->GetNext(pos2);
			ptResult = pData1->CalcExpandPoint(pData);
		}
		if ( ptResult ) {
			pData1->SetNativePoint(1, *ptResult);
			pData2->SetNativePoint(0, *ptResult);
		}
	}

	return TRUE;
}

BOOL CDXFshape::CheckIntersectionCircle(const CPointD& ptc)
{
	const CDXFchain*	pChain = GetShapeChain();
	ASSERT( pChain );

	// ｵﾘｼﾞﾅﾙｵﾌﾞｼﾞｪｸﾄとの交点ﾁｪｯｸ
	for ( POSITION pos = pChain->GetHeadPosition(); pos; ) {
		// 交点あり(==2)なら TRUE、「接する」(==1)はOK!
		if ( pChain->GetNext(pos)->CheckIntersectionCircle(ptc, m_dOffset) > 1 )
			return TRUE;
	}

	return FALSE;
}

void CDXFshape::AllChangeFactor(double dFactor) const
{
	for ( POSITION pos=m_ltWork.GetHeadPosition(); pos; )
		m_ltWork.GetNext(pos)->DrawTuning(dFactor);
}

void CDXFshape::DrawWorking(CDC* pDC) const
{
	CDXFworking*	pWork;
	int		i = 0, j,
			a = 0, b;
	CPen*	pPen[] = {
		AfxGetNCVCMainWnd()->GetPenDXF(DXFPEN_WORKER),
		AfxGetNCVCMainWnd()->GetPenCom(COMPEN_SEL)
	};
	CPen*	pOldPen   = pDC->SelectObject(pPen[i]);
	CBrush* pOldBrush = (CBrush *)pDC->SelectStockObject(NULL_BRUSH);
	pDC->SetROP2(R2_COPYPEN);
	for ( POSITION pos = m_ltWork.GetHeadPosition(); pos; ) {
		pWork = m_ltWork.GetNext(pos);
		j = pWork->GetWorkingFlag() & DXFWORKFLG_SELECT ? 1 : 0;
		if ( i != j ) {
			i = j;
			pDC->SelectObject( pPen[i] );
		}
		b = pWork->GetWorkingType() == WORK_START ? 1 : 0;
		if ( a != b ) {
			a = b;
			if ( a == 0 )
				pDC->SelectStockObject(NULL_BRUSH);
			else
				pDC->SelectObject(AfxGetNCVCMainWnd()->GetBrushDXF(DXFBRUSH_START));
		}
		pWork->Draw(pDC);
	}
	pDC->SelectObject(pOldBrush);
	pDC->SelectObject(pOldPen);
}

int CDXFshape::GetObjectCount(void) const
{
	return apply_visitor( GetObjectCount_Visitor(), m_vShape );
}

double CDXFshape::GetSelectObjectFromShape
	(const CPointD& pt, const CRectD* rcView/*=NULL*/, CDXFdata** pDataResult/*=NULL*/)
{
	if ( m_vShape.which() == 0 )
		return get<CDXFchain*>(m_vShape)->GetSelectObjectFromShape(pt, rcView, pDataResult);
	else
		return get<CDXFmap*>(m_vShape)->GetSelectObjectFromShape(pt, rcView, pDataResult);
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

void CDXFshape::DrawShape(CDC* pDC) const
{
	if ( m_vShape.which() == 0 )
		get<CDXFchain*>(m_vShape)->DrawShape(pDC);
	else
		get<CDXFmap*>(m_vShape)->DrawShape(pDC);
}

void CDXFshape::OrgTuning(void)
{
	apply_visitor( OrgTuning_Visitor(), m_vShape );
}
