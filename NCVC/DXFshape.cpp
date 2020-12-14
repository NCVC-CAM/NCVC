// DXFshape.cpp: CDXFmap クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "DXFdata.h"
#include "DXFshape.h"
#include "Layer.h"
#include "DXFDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#define	_DEBUGOLD
//#define	_DEBUGDRAW_DXF
#endif

IMPLEMENT_DYNAMIC(CDXFworking, CObject)
IMPLEMENT_SERIAL(CDXFworkingDirection, CDXFworking, NCVCSERIALVERSION|VERSIONABLE_SCHEMA)
IMPLEMENT_SERIAL(CDXFworkingStart, CDXFworking, NCVCSERIALVERSION|VERSIONABLE_SCHEMA)
IMPLEMENT_SERIAL(CDXFworkingOutline, CDXFworking, NCVCSERIALVERSION|VERSIONABLE_SCHEMA)
IMPLEMENT_SERIAL(CDXFworkingPocket, CDXFworking, NCVCSERIALVERSION|VERSIONABLE_SCHEMA)
IMPLEMENT_SERIAL(CDXFmap, CObject, NCVCSERIALVERSION|VERSIONABLE_SCHEMA)
IMPLEMENT_SERIAL(CDXFchain, CObject, NCVCSERIALVERSION|VERSIONABLE_SCHEMA)
IMPLEMENT_SERIAL(CDXFshape, CObject, NCVCSERIALVERSION|VERSIONABLE_SCHEMA)

using std::vector;
using namespace boost;

extern	DWORD	g_dwCamVer;		// NCVC.cpp

/////////////////////////////////////////////////////////////////////////////
// 静的変数の初期化
float	CDXFmap::ms_dTolerance = NCMIN;

static	DWORD	g_dwMapFlag[] = {
	DXFMAPFLG_DIRECTION, DXFMAPFLG_START,
	DXFMAPFLG_OUTLINE, DXFMAPFLG_POCKET
};

/////////////////////////////////////////////////////////////////////////////
// ＤＸＦデータ加工指示のベースクラス
/////////////////////////////////////////////////////////////////////////////
CDXFworking::CDXFworking
	(ENWORKINGTYPE enType, CDXFshape* pShape, CDXFdata* pData, DWORD dwFlags)
{
	extern	LPCTSTR	gg_szReturn;	// "\n"
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
	int	nStart = strWork.Find(gg_szReturn) + 1,			// '\n' の次から
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
		CLayerData*	pLayer = static_cast<CDXFDoc *>(ar.m_pDocument)->GetSerializeLayer();
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
	(CDXFshape* pShape, CDXFdata* pData, CPointF pts, CPointF pte[]) :
		CDXFworking(WORK_DIRECTION, pShape, pData, 0)
{
	m_ptStart = pts;
	for ( int i=0; i<SIZEOF(m_ptArraw); i++ )
		m_ptArraw[i] = pte[i];
}

void CDXFworkingDirection::DrawTuning(float f)
{
	m_ptDraw[1] = m_ptArraw[1] * f;
	m_ptDraw[0] = m_ptArraw[0] + m_ptDraw[1];
	m_ptDraw[2] = m_ptArraw[2] + m_ptDraw[1];
}

void CDXFworkingDirection::Draw(CDC* pDC) const
{
#ifdef _DEBUGDRAW_DXF
	printf("CDXFworkingDirection::Draw() pt.x=%d pt.y=%d\n", m_ptDraw[1].x, m_ptDraw[1].y);
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
		if ( g_dwCamVer < NCVCSERIALVERSION_3620 ) {
			CPointD	pt;
			ar >> pt.x >> pt.y;
			m_ptStart = pt;
			for ( i=0; i<SIZEOF(m_ptArraw); i++ ) {
				ar >> pt.x >> pt.y;
				m_ptArraw[i] = pt;
			}
		}
		else {
			ar >> m_ptStart.x >> m_ptStart.y;
			for ( i=0; i<SIZEOF(m_ptArraw); i++ )
				ar >> m_ptArraw[i].x >> m_ptArraw[i].y;
		}
	}
}

//////////////////////////////////////////////////////////////////////
// ＤＸＦデータの「開始位置」指示クラス
//////////////////////////////////////////////////////////////////////
CDXFworkingStart::CDXFworkingStart
	(CDXFshape* pShape, CDXFdata* pData, CPointF pts) :
		CDXFworking(WORK_START, pShape, pData, 0)
{
	m_ptStart = pts;
}

void CDXFworkingStart::DrawTuning(float f)
{
	CPointF	pt( m_ptStart * f ); 
	// 位置を表す丸印は常に2.5論理理位 (CDXFpoint準拠)
	float	dFactor = LOMETRICFACTOR * 2.5;
	m_rcDraw.TopLeft()		= pt - dFactor;
	m_rcDraw.BottomRight()	= pt + dFactor;
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
	else {
		if ( g_dwCamVer < NCVCSERIALVERSION_3620 ) {
			CPointD	pt;
			ar >> pt.x >> pt.y;
			m_ptStart = pt;
		}
		else {
			ar >> m_ptStart.x >> m_ptStart.y;
		}
	}
}

//////////////////////////////////////////////////////////////////////
// ＤＸＦデータの「輪郭」加工指示クラス
//////////////////////////////////////////////////////////////////////
CDXFworkingOutline::CDXFworkingOutline
	(CDXFshape* pShape, const CDXFchain* pOutline, float dOffset, DWORD dwFlags) :
		CDXFworking(WORK_OUTLINE, pShape, NULL, dwFlags)
{
	m_obOutline.SetSize(0, 64);
	m_obMergeHandle.SetSize(0, 64);
	m_rcMax.SetRectMinimum();
	m_dOffset = dOffset;		// 輪郭が生成されたときのｵﾌｾｯﾄ値
	SeparateAdd_Construct(pOutline);
}

CDXFworkingOutline::~CDXFworkingOutline()
{
	for ( int i=0; i<m_obOutline.GetSize(); i++ ) {
		CDXFchain* pChain = m_obOutline[i];
		PLIST_FOREACH(CDXFdata* pData, pChain)
			delete	pData;
		END_FOREACH
		delete	pChain;
	}
	m_obOutline.RemoveAll();
	m_obMergeHandle.RemoveAll();
}

void CDXFworkingOutline::SeparateAdd_Construct(const CDXFchain* pOutline)
{
	CDXFchain*	pChain = NULL;
	;
	// NULL で分割された集合を分離して登録
	PLIST_FOREACH(CDXFdata* pData, pOutline)
		if ( pData ) {
			if ( !pChain )
				pChain = new CDXFchain;
			pChain->AddTail(pData);
			pChain->SetMaxRect(pData);
		}
		else if ( pChain ) {
			m_obOutline.Add(pChain);
			pChain = NULL;
		}
	END_FOREACH

	if ( pChain )
		m_obOutline.Add(pChain);
	m_rcMax = pOutline->GetMaxRect();

#ifdef _DEBUG
	printf("SeparateAdd_Construct() OutlineSize=%d\n", m_obOutline.GetSize());
	for ( int i=0; i<m_obOutline.GetSize(); i++ ) {
		pChain = m_obOutline[i];
		int	j = 0;
		PLIST_FOREACH(auto ref, pChain)
			if ( !ref )
				printf("??? NULL element %d\n", j);
			j++;
		END_FOREACH
	}
#endif
}

void CDXFworkingOutline::SeparateModify(void)
{
#ifdef _DEBUG
	printf("CDXFworkingOutline::SeparateModify() Start\n");
	CPointF		ptDbgS, ptDbgE;
#endif
	INT_PTR		i, j, nLoop,
				nMainLoop = m_obOutline.GetSize();	// 途中で追加されるので先に最大値取得
	POSITION	pos, pos1, pos2;
	optional<CPointF>	pts, pte;
	CDXFdata*	pData;
	CDXFchain*	pOutline;
	CDXFchain*	pChain;
	vector<POSITION>	vPos;

	// すでに登録されているｵﾌﾞｼﾞｪｸﾄに対して
	// NULL で分割された集合を分離して登録
	for ( i=0; i<nMainLoop; i++ ) {
		pOutline = m_obOutline[i];
		for ( pos1=pOutline->GetHeadPosition(); (pos2=pos1); ) {
			pData = pOutline->GetNext(pos1);
			if ( pData ) {
				if ( pData->GetDxfFlg() & DXFFLG_OFFSET_EXCLUDE ) {
					// 不要ｵﾌﾞｼﾞｪｸﾄの削除
					pOutline->RemoveAt(pos2);
					delete	pData;
				}
			}
			else
				vPos.push_back(pos2);	// NULLﾎﾟｼﾞｼｮﾝを記録
		}
		nLoop = vPos.size();
		// 分離処理
		if ( nLoop > 1 ) {
			// NULL間のｵﾌﾞｼﾞｪｸﾄを新しい集合へ
			for ( j=0; j<nLoop-1; j++ ) {
				pos = pos1 = vPos[j];
				pos2 = vPos[j+1];
				pOutline->GetNext(pos1);	// NULLﾃﾞｰﾀ(pos1++)
				pOutline->RemoveAt(pos);	// をﾘｽﾄから消去
				if ( pos == pOutline->GetHeadPosition() )
					continue;	// 先頭にNULLがある場合は、NULLだけ消す
				if ( pos1 != pos2 ) {		// 連続NULL実行防止
					pChain = new CDXFchain(DXFMAPFLG_SEPARATE);
					while ( (pos=pos1) && pos1!=pos2 ) {
						pData = pOutline->GetNext(pos1);
						pOutline->RemoveAt(pos);
						pChain->AddTail(pData);
						pChain->SetMaxRect(pData);
					}
					m_obOutline.Add(pChain);
				}
			}
		}
		if ( nLoop > 0 ) {
			pos2 = vPos.back();		// NULLのﾎﾟｼﾞｼｮﾝ情報
			pts.reset();
			pte.reset();
			// 先頭ﾃﾞｰﾀと末尾ﾃﾞｰﾀの始点終点からﾙｰﾌﾟ接続判断
			for ( pos=pOutline->GetHeadPosition(); pos; ) {
				pData = pOutline->GetNext(pos);
				if ( pData ) {
					pts = pData->GetNativePoint(0);
					break;
				}
			}
			for ( pos=pOutline->GetTailPosition(); pos; ) {
				pData = pOutline->GetPrev(pos);
				if ( pData ) {
					pte = pData->GetNativePoint(1);
					break;
				}
			}
			if ( pts && pte ) {
				if ( sqrt(GAPCALC(*pts - *pte)) < NCMIN ) {
					// 先頭ﾃﾞｰﾀの始点と末尾ﾃﾞｰﾀの座標が等しい場合は、
					// NULLﾎﾟｼﾞｼｮﾝを手掛かりに、端点が先頭に来るよう順序変更
					for ( pos1=pOutline->GetTailPosition(); pos1 && pos1!=pos2; ) {
						pData = pOutline->GetPrev(pos1);
						pOutline->RemoveTail();		// 最後から消去して
						pOutline->AddHead(pData);	// 先頭に移動
					}
				}
				else if ( pOutline->GetHead() && pOutline->GetTail() ) {	// 先頭も末尾もNULLでなければ
					// NULL以降を分離
					pChain = new CDXFchain(DXFMAPFLG_SEPARATE);
					for ( pos1=pOutline->GetTailPosition(); pos1 && pos1!=pos2; ) {
						pData = pOutline->GetPrev(pos1);
						pOutline->RemoveTail();
						pChain->AddHead(pData);
						pChain->SetMaxRect(pData);
					}
					m_obOutline.Add(pChain);
				}
			}
			pOutline->SetChainFlag(DXFMAPFLG_SEPARATE);
			// NULL消去
			pOutline->RemoveAt(pos2);
			// 次の処理へ
			vPos.clear();
		}
#ifdef _DEBUG
		pte.reset();
		printf("%s nLoop=%d\n", LPCTSTR(m_pShape->GetShapeName()), i);
		PLIST_FOREACH(pData, pOutline)
			ptDbgS = pData->GetNativePoint(0);
			ptDbgE = pData->GetNativePoint(1);
			printf("pt=(%.3f, %.3f) - (%.3f, %.3f) %s\n",
				ptDbgS.x, ptDbgS.y, ptDbgE.x, ptDbgE.y,
				pte && sqrt(GAPCALC(*pte-ptDbgS))>=NCMIN ? "X" : " ");
			pte = ptDbgE;
		END_FOREACH
#endif
	}

#ifdef _DEBUG
	if ( nMainLoop < m_obOutline.GetSize() ) {
		for ( i=nMainLoop; i<m_obOutline.GetSize(); i++ ) {
			pte.reset();
			printf("--- Separate Add %d ---\n", i);
			PLIST_FOREACH(pData, m_obOutline[i])
				ptDbgS = pData->GetNativePoint(0);
				ptDbgE = pData->GetNativePoint(1);
				printf("pt=(%.3f, %.3f) - (%.3f, %.3f) %s\n",
					ptDbgS.x, ptDbgS.y, ptDbgE.x, ptDbgE.y,
					pte && sqrt(GAPCALC(*pte-ptDbgS))>=NCMIN ? "X" : " ");
				pte = ptDbgE;
			END_FOREACH
		}
	}
#endif

	// 分離集合ﾁｪｯｸ
	while ( nMainLoop-- ) {		//	for ( i=nMainLoop-1; i>=0; i-- ) {
		pOutline = m_obOutline[nMainLoop];
		if ( pOutline->IsEmpty() ) {
#ifdef _DEBUG
			printf("--- Separete Delete %d ---\n", nMainLoop);
#endif
			delete	pOutline;
			m_obOutline.RemoveAt(nMainLoop);
		}
	}
}

void CDXFworkingOutline::SetMergeHandle(const CString& strHandle)
{
	int		i;
	for ( i=0; i<m_obMergeHandle.GetSize(); i++ ) {
		if ( m_obMergeHandle[i] == strHandle )
			break;
	}
	if ( i >= m_obMergeHandle.GetSize() )
		m_obMergeHandle.Add(strHandle);
}

void CDXFworkingOutline::DrawTuning(float f)
{
	for ( int i=0; i<m_obOutline.GetSize(); i++ ) {
		PLIST_FOREACH(CDXFdata* pData, m_obOutline[i])
			pData->DrawTuning(f);
		END_FOREACH
	}
}

void CDXFworkingOutline::Draw(CDC* pDC) const
{
	for ( int i=0; i<m_obOutline.GetSize(); i++ ) {
		PLIST_FOREACH(CDXFdata* pData, m_obOutline[i])
			pData->Draw(pDC);
		END_FOREACH
	}
}

void CDXFworkingOutline::Serialize(CArchive& ar)
{
	CDXFworking::Serialize(ar);

	CObject*	pNewData;	// CDXFdata*
	CDXFdata*	pData;
	CDXFchain*	pChain;
	int			i, nLoop1, nLoop2;

	// 輪郭ｵﾌﾞｼﾞｪｸﾄのｼﾘｱﾗｲｽﾞ
	// m_obOutline.Serialize(ar); では CDXFchain を呼び，
	// ｵﾌﾞｼﾞｪｸﾄへの SeqNo. で処理される．ゆえに自力ｺｰﾃﾞｨﾝｸﾞ
	if ( ar.IsStoring() ) {
		// 輪郭ｵﾌｾｯﾄ値
		ar << m_dOffset;
		// 輪郭ｵﾌﾞｼﾞｪｸﾄ数
		nLoop1 = (int)m_obOutline.GetSize();
		ar << nLoop1;
		// ﾃﾞｰﾀ本体
		for ( i=0; i<nLoop1; i++ ) {
			pChain = m_obOutline[i];
			nLoop2 =  (int)pChain->GetSize();
			ar << nLoop2 << pChain->GetChainFlag();
			PLIST_FOREACH(pData, pChain)
				ar << pData;
			END_FOREACH
		}
		// 併合情報
		m_obMergeHandle.Serialize(ar);
		return;
	}

	if ( g_dwCamVer > NCVCSERIALVERSION_1507 ) {		// Ver1.60～
		if ( g_dwCamVer > NCVCSERIALVERSION_1600 ) {	// Ver1.70～
			if ( g_dwCamVer < NCVCSERIALVERSION_3620 ) {
				double	d;
				ar >> d;
				m_dOffset = (float)d;
			}
			else {
				ar >> m_dOffset;
			}
		}
		else
			m_dOffset = m_pShape->GetOffset();
		DWORD	dwFlags;
		ar >> nLoop1;
		while ( nLoop1-- ) {
			ar >> i >> dwFlags;
			pChain = new CDXFchain(dwFlags);
			while ( i-- ) {
				ar >> pNewData;
				pData = static_cast<CDXFdata *>(pNewData);
				pChain->AddTail(pData);
				pChain->SetMaxRect(pData);
			}
			m_obOutline.Add(pChain);
			m_rcMax |= pChain->GetMaxRect();
		}
		m_obMergeHandle.Serialize(ar);
	}
	else {
		CDXFchain	ltOutline;
		DWORD		dwCnt = 0;	// ﾃﾞｰﾀの切れ目回数
		CDWordArray	obGap;		// ﾃﾞｰﾀの切れ目一覧
		INT_PTR		n = 0;
		// ﾃﾞｰﾀの切れ目(NULL値)の読み込み
		if ( g_dwCamVer > NCVCSERIALVERSION_1503 )	// Ver1.10～
			obGap.Serialize(ar);
		dwCnt = obGap.IsEmpty() ? -1 : obGap[0];
		// ﾃﾞｰﾀ本体
		ar >> nLoop1;
		for ( i=0; i<nLoop1; i++ ) {
			if ( i == dwCnt ) {
				ltOutline.AddTail( (CDXFdata *)NULL );
				dwCnt = n < obGap.GetUpperBound() ? obGap[++n] : -1;
			}
			else {
				ar >> pNewData;
				pData = static_cast<CDXFdata *>(pNewData);
				ltOutline.AddTail(pData);
			}
		}
		// 新しいﾃﾞｰﾀ格納方法に変換
		SeparateAdd_Construct(&ltOutline);
	}
}

//////////////////////////////////////////////////////////////////////
// ＤＸＦデータの「ポケット」加工指示クラス
//////////////////////////////////////////////////////////////////////
CDXFworkingPocket::CDXFworkingPocket(CDXFshape* pShape, DWORD dwFlags) :
	CDXFworking(WORK_POCKET, pShape, NULL, dwFlags)
{
	m_obPocket.SetSize(0, 64);
}

CDXFworkingPocket::~CDXFworkingPocket()
{
	for ( int i=0; i<m_obPocket.GetSize(); i++ )
		delete	m_obPocket[i];
	m_obPocket.RemoveAll();
}

void CDXFworkingPocket::DrawTuning(float f)
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

#ifdef _DEBUG
void CDXFmap::DbgDump(void) const
{
	CPointF		pt;
	CDXFarray*	pArray;

	printf("CDXFmap::DbgDump() Cnt=%d\n", GetCount());
	PMAP_FOREACH(pt, pArray, this)
		printf("Key=(%f, %f)\n", pt.x, pt.y);
		printf("DataCnt=%d\n", pArray->GetCount());
	END_FOREACH
}
#endif

void CDXFmap::Serialize(CArchive& ar)
{
	UINT		i, nMapCnt, nDataCnt;
	CPointF		pt;
	CDXFarray*	pArray;

	if ( ar.IsStoring() ) {
		nMapCnt = (UINT)GetCount();
		ar << nMapCnt << GetHashTableSize();
		PMAP_FOREACH(pt, pArray, this)
			nDataCnt = (UINT)pArray->GetSize();
			ar << pt.x << pt.y << nDataCnt;
			for ( i=0; i<nDataCnt; i++ )
				ar << pArray->GetAt(i)->GetSerializeSeq();
		END_FOREACH
	}
	else {
		UINT		nHash;
		DWORD		nIndex;
		CDXFdata*	pData;
		CLayerData*	pLayer = static_cast<CDXFDoc *>(ar.m_pDocument)->GetSerializeLayer();
		ar >> nMapCnt >> nHash;
		InitHashTable(nHash);
		while ( nMapCnt-- ) {
			if ( g_dwCamVer < NCVCSERIALVERSION_3620 ) {
				CPointD	ptD;
				ar >> ptD.x >> ptD.y >> nDataCnt;
				pt = ptD;
			}
			else {
				ar >> pt.x >> pt.y >> nDataCnt;
			}
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
	CPointF	pt;
	CDXFarray*	pArray;

	PMAP_FOREACH(pt, pArray, this)
		if ( pArray )
			delete	pArray;
	END_FOREACH
	CMapPointToDXFarray::RemoveAll();
}

void CDXFmap::SetPointMap(CDXFdata* pData)
{
	CDXFarray*	pArray;
	CPointF	pt;
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
	CPointF	pt;

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

tuple<BOOL, CDXFarray*, CPointF>
CDXFmap::IsEulerRequirement(const CPointF& ptKey) const
{
	int			i, nObCnt, nOddCnt = 0;
	BOOL		bEuler = FALSE,	// 一筆書き要件を満たしているか
				bOne = FALSE;	// 端点
	float		dGap, dGapMin = FLT_MAX, dGapMin2 = FLT_MAX;
	CPointF		pt, ptStart, ptStart2;
	CDXFdata*	pData;
	CDXFarray*	pArray;
	CDXFarray*	pStartArray = NULL;
	CDXFarray*	pStartArray2;

	// ｵﾌﾞｼﾞｪｸﾄ登録数の奇数を探す + ｻｰﾁﾌﾗｸﾞのｸﾘｱ
	PMAP_FOREACH(pt, pArray, this)
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
			// 奇数を優先的に検索
			nOddCnt++;
			if ( nObCnt == 1 ) {
				// 端点(1)をさらに優先
				if ( bOne ) {
					if ( dGap < dGapMin ) {
						dGapMin = dGap;
						pStartArray = pArray;
						ptStart = pt;
					}
				}
				else {
					bOne = TRUE;
					dGapMin = dGap;
					pStartArray = pArray;
					ptStart = pt;
				}
			}
			else if ( !bOne ) {
				// 端点がない時だけ近い座標検索
				if ( dGap < dGapMin ) {
					dGapMin = dGap;
					pStartArray = pArray;
					ptStart = pt;
				}
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
	END_FOREACH
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
	//		DXFMAPFLG_EDGE|INTERSEC -> 方向指示は可能，輪郭・ﾎﾟｹｯﾄ加工ができない
	INT_PTR		i, j, nLoop;
	DWORD		dwFlags = 0;
	CDXFarray	obWorkArray;
	CDXFarray*	pArray;
	CPointF		pt, ptChk[4];
	CDXFdata*	pData;

	// 所属ｵﾌﾞｼﾞｪｸﾄが１つ かつ 閉ﾙｰﾌﾟなら
	if ( GetObjectCount() == 1 ) {
		POSITION pos = GetStartPosition();
		GetNextAssoc(pos, pt, pArray);
		pData = pArray->GetAt(0);
		if ( !pData->IsStartEqEnd() )
			dwFlags |= DXFMAPFLG_EDGE;
		if ( pData->GetType()==DXFPOLYDATA &&
						static_cast<CDXFpolyline*>(pData)->IsIntersection() )
			dwFlags |= DXFMAPFLG_INTERSEC;
		return dwFlags;
	}

	obWorkArray.SetSize(0, GetSize());
	AllMapObject_ClearSearchFlg(FALSE);

	try {
		PMAP_FOREACH(pt, pArray, this)
			nLoop = pArray->GetSize();
			if ( nLoop == 1 )			// 端点
				dwFlags |= DXFMAPFLG_EDGE;
			else if ( nLoop != 2 )		// ３つ以上のｵﾌﾞｼﾞｪｸﾄ
				return DXFMAPFLG_CANNOTWORKING;	// これ以上調査の必要なし
			// 交点ﾁｪｯｸのためにｵﾌﾞｼﾞｪｸﾄをｺﾋﾟｰ
			for ( i=0; i<nLoop; i++ ) {
				pData = pArray->GetAt(i);
				if ( pData->GetType()==DXFPOLYDATA &&
						static_cast<CDXFpolyline*>(pData)->IsIntersection() ) {
					dwFlags |= DXFMAPFLG_INTERSEC;
					break;
				}
				if ( !pData->IsSearchFlg() ) {
					obWorkArray.Add(pData);		// ﾜｰｸ配列に登録
					pData->SetSearchFlg();
					pData->ClearMakeFlg();
				}
			}
		END_FOREACH
		// 交点ﾁｪｯｸ
		if ( !(dwFlags & DXFMAPFLG_INTERSEC) ) {
			nLoop = obWorkArray.GetSize();
			for ( i=0; i<nLoop; i++ ) {
				pData = obWorkArray[i];
				for ( j=i+1; j<nLoop; j++ ) {
					if ( pData->GetIntersectionPoint(obWorkArray[j], ptChk) > 0 ) {
#ifdef _DEBUG
						printf("Intersection !!! i=%d j=%d\n", i, j);
						pData->DbgDump();
						obWorkArray[j]->DbgDump();
#endif
						dwFlags |= DXFMAPFLG_INTERSEC;
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
	CPointF		pt;
	CDXFarray*	pArray;

	PMAP_FOREACH(pt, pArray, this)
		for ( i=0; i<pArray->GetSize(); i++ ) {
			if ( !pArray->GetAt(i)->IsSearchFlg() ) {
				pos = NULL;
				bResult = FALSE;
				break;
			}
		}
	END_FOREACH

	return bResult;
}

BOOL CDXFmap::IsAllMakeFlg(void) const
{
	int			i;
	BOOL		bResult = TRUE;
	CPointF		pt;
	CDXFarray*	pArray;

	PMAP_FOREACH(pt, pArray, this)
		for ( i=0; i<pArray->GetSize(); i++ ) {
			if ( !pArray->GetAt(i)->IsMakeFlg() ) {
				pos = NULL;
				bResult = FALSE;
				break;
			}
		}
	END_FOREACH

	return bResult;
}

void CDXFmap::AllMapObject_ClearSearchFlg(BOOL bMake/*=TRUE*/) const
{
	int			i;
	CPointF		pt;
	CDXFdata*	pData;
	CDXFarray*	pArray;

	PMAP_FOREACH(pt, pArray, this)
		for ( i=0; i<pArray->GetSize(); i++ ) {
			pData = pArray->GetAt(i);
			if ( !bMake || (bMake && !pData->IsMakeFlg()) )
				pData->ClearSearchFlg();
		}
	END_FOREACH
}

void CDXFmap::AllMapObject_ClearMakeFlg(void) const
{
	int			i;
	CPointF		pt;
	CDXFarray*	pArray;

	PMAP_FOREACH(pt, pArray, this)
		for ( i=0; i<pArray->GetSize(); i++ )
			pArray->GetAt(i)->ClearMakeFlg();
	END_FOREACH
}

BOOL CDXFmap::CopyToChain(CDXFchain* pChain)
{
	INT_PTR		i, nLoop;
	CPointF		pt, pts, pte;
	CDXFarray*	pArray;
	CDXFdata*	pData = NULL;

	// 始点の端点検索
	PMAP_FOREACH(pt, pArray, this)
		if ( pArray->GetSize() == 1 ) {
			pData = pArray->GetAt(0);
			break;
		}
		if ( !pData && pArray->GetSize()>0 )
			pData = pArray->GetAt(0);
	END_FOREACH
	if ( !pData )
		return FALSE;

	pts = pData->GetNativePoint(0);
	pte = pData->GetNativePoint(1);
	if ( GAPCALC(pts-pt) > GAPCALC(pte-pt) )
		pData->SwapNativePt();
	pt = pData->GetNativePoint(1);	// 終点==次の座標ｷｰ

	AllMapObject_ClearMakeFlg();
	pChain->AddTail(pData);
	pChain->SetMaxRect(pData);
	pData->SetMakeFlg();

	while ( Lookup(pt, pArray) ) {
		nLoop = pArray->GetSize();
		for ( i=0; i<nLoop; i++ ) {
			pData = pArray->GetAt(i);
			if ( !pData->IsMakeFlg() ) {
				pts = pData->GetNativePoint(0);
				pte = pData->GetNativePoint(1);
				if ( GAPCALC(pts-pt) > GAPCALC(pte-pt) )
					pData->SwapNativePt();
				pChain->AddTail(pData);
				pChain->SetMaxRect(pData);
				pData->SetMakeFlg();
				pt = pData->GetNativePoint(1);
				break;
			}
		}
		if ( i >= nLoop )
			break;
	}

	return TRUE;
}

void CDXFmap::Append(const CDXFmap* pMap)
{
	CPointF		pt;
	CDXFarray*	pArraySrc;
	CDXFarray*	pArrayDst;

	PMAP_FOREACH(pt, pArraySrc, pMap)
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
	END_FOREACH
}

void CDXFmap::Append(const CDXFchain* pChain)
{
	PLIST_FOREACH(CDXFdata* pData, pChain)
		SetPointMap(pData);
	END_FOREACH
}

INT_PTR CDXFmap::GetObjectCount(void) const
{
	INT_PTR		i, nCnt = 0;
	CPointF		pt;
	CDXFdata*	pData;
	CDXFarray*	pArray;

	AllMapObject_ClearSearchFlg(FALSE);

	PMAP_FOREACH(pt, pArray, this)
		for ( i=0; i<pArray->GetSize(); i++ ) {
			pData = pArray->GetAt(i);
			if ( !pData->IsSearchFlg() ) {
				nCnt++;
				pData->SetSearchFlg();
			}
		}
	END_FOREACH

	return nCnt;
}

float CDXFmap::GetSelectObjectFromShape
	(const CPointF& pt, const CRectF* rcView/*=NULL*/, CDXFdata** pDataResult/*=NULL*/)
{
	int		i;
	CPointF	ptKey;
	CRectF	rc, _rcView;
	float	dGap, dGapMin = FLT_MAX;
	CDXFarray*	pArray;
	CDXFarray*	pArrayMin = NULL;
	CDXFdata*	pData;

	if ( pDataResult )
		*pDataResult = NULL;
	if ( rcView )
		_rcView.SetRect(rcView);

	// 指定座標に一番近いｵﾌﾞｼﾞｪｸﾄを検索
	// --- 「一番近い座標ｷｰ」では端点の検索になってしまう ---
	PMAP_FOREACH(ptKey, pArray, this)
		for ( i=0; i<pArray->GetSize(); i++ ) {
			pData = pArray->GetAt(i);
			if ( rcView && !rc.CrossRect(_rcView, pData->GetMaxRect()) )
				continue;
			dGap = pData->GetSelectPointGap(pt);
			if ( dGap < dGapMin ) {
				dGapMin = dGap;
				if ( pDataResult )
					*pDataResult = pData;
			}
		}
	END_FOREACH

	return dGapMin;
}

void CDXFmap::SetShapeSwitch(BOOL bSelect)
{
	CDXFarray*	pArray;
	CPointF	pt;

	PMAP_FOREACH(pt, pArray, this)
		for ( int i=0; i<pArray->GetSize(); i++ )
			pArray->GetAt(i)->SetDxfFlg(DXFFLG_SELECT, bSelect);
	END_FOREACH
}

void CDXFmap::RemoveObject(const CDXFdata* pData)
{
	CDXFarray*	pArray;
	CPointF	pt;
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
	CPointF	pt;
	int		i;
	DWORD	dwSel, dwSelBak = 0;

	CBrush* pOldBrush = (CBrush *)pDC->SelectStockObject(NULL_BRUSH);
	CPen* pOldPen = pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenDXF(DXFPEN_CUTTER));
	pDC->SetROP2(R2_COPYPEN);
	PMAP_FOREACH(pt, pArray, this)
		for ( i=0; i<pArray->GetSize(); i++ ) {
			pData = pArray->GetAt(i);
			dwSel = pData->GetDxfFlg() & DXFFLG_SELECT;
			if ( dwSel != dwSelBak ) {
				dwSelBak = dwSel;
				pDC->SelectObject(pData->GetDrawPen());
			}
			pData->Draw(pDC);
		}
	END_FOREACH
	pDC->SelectObject(pOldPen);
	pDC->SelectObject(pOldBrush);
}

void CDXFmap::OrgTuning(void)
{
	CDXFarray*	pArray;
	CPointF	pt;

	PMAP_FOREACH(pt, pArray, this)
		for ( int i=0; i<pArray->GetSize(); i++ )
			pArray->GetAt(i)->OrgTuning(FALSE);
	END_FOREACH
}

/////////////////////////////////////////////////////////////////////////////
// ＤＸＦデータの輪郭集団クラス
/////////////////////////////////////////////////////////////////////////////
CDXFchain::CDXFchain(DWORD dwFlag/*=0*/)
{
	m_rcMax.SetRectMinimum();
	m_dwFlags = dwFlag;
}

CDXFchain::~CDXFchain()
{
}

void CDXFchain::Serialize(CArchive& ar)
{
	UINT		nLstCnt;
	CDXFdata*	pData;

	if ( ar.IsStoring() ) {
		nLstCnt = (UINT)GetCount();
		ar << nLstCnt << m_dwFlags;
		PLIST_FOREACH(CDXFdata* pData, this)
			ar << pData->GetSerializeSeq();
		END_FOREACH
	}
	else {
		DWORD		nIndex;
		CLayerData*	pLayer = static_cast<CDXFDoc *>(ar.m_pDocument)->GetSerializeLayer();
		ar >> nLstCnt >> m_dwFlags;
		while ( nLstCnt-- ) {
			ar >> nIndex;
			if ( nIndex >= 0 ) {
				pData = pLayer->GetDxfData(nIndex);
				SetMaxRect(pData);
				AddTail(pData);
			}
		}
	}
}

void CDXFchain::SetMakeFlags(void)
{
	PLIST_FOREACH(CDXFdata* pData, this)
		pData->SetMakeFlg();
	END_FOREACH
}

void CDXFchain::ReverseMakePt(void)
{
	// 全ｵﾌﾞｼﾞｪｸﾄの座標反転
	PLIST_FOREACH(CDXFdata* pData, this)
		pData->SwapMakePt(0);
	END_FOREACH
}

void CDXFchain::ReverseNativePt(void)
{
	PLIST_FOREACH(CDXFdata* pData, this)
		pData->SwapNativePt();
	END_FOREACH
}

void CDXFchain::CopyToMap(CDXFmap* pMap)
{
	PLIST_FOREACH(CDXFdata* pData, this)
		pMap->SetPointMap(pData);
	END_FOREACH
}

BOOL CDXFchain::IsLoop(void) const
{
	if ( IsEmpty() )
		return FALSE;
	if ( GetCount()==1 && GetHead()->IsStartEqEnd() )
		return TRUE;

	CPointF	pts( GetHead()->GetNativePoint(0) ),
			pte( GetTail()->GetNativePoint(1) );
	return sqrt(GAPCALC(pts-pte)) < NCMIN;
}

BOOL CDXFchain::IsPointInPolygon(const CPointF& ptTarget) const
{
	CDXFdata*	pData;
	CPointF		pt;
	CVPointF	vpt;

	if ( GetCount() == 1 ) {
		pData = GetHead();
		if ( pData && pData->GetType()==DXFCIRCLEDATA ) {
			CDXFcircle* pCircle = static_cast<CDXFcircle*>(pData);
			pt = ptTarget - pCircle->GetCenter();
			return pt.hypot() < pCircle->GetR();
		}
	}

	BOOL	bNext = TRUE;
	// 各ｵﾌﾞｼﾞｪｸﾄの始点を配列に
	PLIST_FOREACH(pData, this)
		if ( pData ) {
			switch ( pData->GetType() ) {
			case DXFLINEDATA:
				if ( bNext )
					pData->SetVectorPoint(vpt);
				bNext = TRUE;
				break;
			case DXFARCDATA:
			case DXFELLIPSEDATA:
			case DXFPOLYDATA:
				pData->SetVectorPoint(vpt);
				bNext = FALSE;	// 終点まで登録するから次のDXFLINEDATAの始点は無視
				break;
			default:
				bNext = TRUE;
			}
		}
	END_FOREACH

	if ( bNext ) {
		// 最終ｵﾌﾞｼﾞｪｸﾄの終点をｾｯﾄ
		for ( POSITION pos=GetTailPosition(); pos; ) {
			pData = GetPrev(pos);
			if ( pData ) {
				vpt.push_back(pData->GetNativePoint(1));
				break;
			}
		}
	}
/*
	// なるべく閉ﾙｰﾌﾟになるように
	CPointF	pts(vpt.front()), pte(vpt.back());
	if ( sqrt(GAPCALC(pts-pte)) >= NCMIN )
		vpt.push_back(pts);
*/
	return ::IsPointInPolygon(ptTarget, vpt);
}

POSITION CDXFchain::SetLoopFunc(const CDXFdata* pData, BOOL bReverse, BOOL bNext)
{
	POSITION	pos1, pos2;

	if ( bReverse ) {
		ReverseMakePt();	// 座標ﾃﾞｰﾀ反転
		m_pfnGetFirstPos	= &CDXFchain::GetTailPosition;
		m_pfnGetFirstData	= &CDXFchain::GetTail;
		m_pfnGetData		= &CDXFchain::GetPrev;
	}
	else {
		m_pfnGetFirstPos	= &CDXFchain::GetHeadPosition;
		m_pfnGetFirstData	= &CDXFchain::GetHead;
		m_pfnGetData		= &CDXFchain::GetNext;
	}

	// 開始ｵﾌﾞｼﾞｪｸﾄの検索
	if ( pData && IsLoop() ) {
		for ( pos1=(this->*m_pfnGetFirstPos)(); (pos2=pos1); ) {
			if ( pData == (this->*m_pfnGetData)(pos1) )
				break;
		}
		pos1 = pos2;	// pData のﾎﾟｼﾞｼｮﾝ
		// 開始ｵﾌﾞｼﾞｪｸﾄの最終調整
		if ( bNext ) {
			for ( int i=0; i<2; i++ ) {	// 自分自身と次のｵﾌﾞｼﾞｪｸﾄのﾎﾟｼﾞｼｮﾝ
				pos2 = pos1;
				(this->*m_pfnGetData)(pos1);
				if ( !pos1 )
					pos1 = (this->*m_pfnGetFirstPos)();
			}
			pos1 = pos2;
		}
	}
	else {
		pos1 = pos2 = (this->*m_pfnGetFirstPos)();
	}

	ASSERT( pos1 );

#ifdef _DEBUG
	optional<CPointF>	ptDbg;
	CPointF		ptDbg1, ptDbg2;
	CDXFdata*	pDataDbg;
	POSITION	posDbg;
	// CDXFchainﾘｽﾄ構造が順序良く並んでいるか
	printf("--- pChain Fin\n");
	ptDbg.reset();

	for ( posDbg=pos1; posDbg; ) {
		pDataDbg = (this->*m_pfnGetData)(posDbg);
		ptDbg1 = pDataDbg->GetTunPoint(0);
		ptDbg2 = pDataDbg->GetTunPoint(1);
		printf("pt=(%.3f, %.3f) - (%.3f, %.3f) %s\n",
			ptDbg1.x, ptDbg1.y, ptDbg2.x, ptDbg2.y,
			ptDbg && sqrt(GAPCALC(*ptDbg-ptDbg1))>=NCMIN ? "X" : " ");
		ptDbg = ptDbg2;
	}
	for ( posDbg=(this->*m_pfnGetFirstPos)(); posDbg!=pos2; ) {
		pDataDbg = (this->*m_pfnGetData)(posDbg);
		ptDbg1 = pDataDbg->GetTunPoint(0);
		ptDbg2 = pDataDbg->GetTunPoint(1);
		printf("pt=(%.3f, %.3f) - (%.3f, %.3f) %s\n",
			ptDbg1.x, ptDbg1.y, ptDbg2.x, ptDbg2.y,
			ptDbg && sqrt(GAPCALC(*ptDbg-ptDbg1))>=NCMIN ? "X" : " ");
		ptDbg = ptDbg2;
	}
#endif

	return pos1;
}

INT_PTR CDXFchain::GetObjectCount(void) const
{
	return GetCount();
}

float CDXFchain::GetLength(void) const
{
	float	dLength = 0;
	PLIST_FOREACH(CDXFdata* pData, this)
		dLength += pData->GetLength();
	END_FOREACH
	return dLength;
}

void CDXFchain::AllChainObject_ClearSearchFlg(void)
{
	PLIST_FOREACH(CDXFdata* pData, this)
		pData->ClearSearchFlg();
	END_FOREACH
}

float CDXFchain::GetSelectObjectFromShape
	(const CPointF& pt, const CRectF* rcView/*=NULL*/, CDXFdata** pDataResult/*=NULL*/)
{
	CRectF	rc, _rcView;
	float	dGap, dGapMin = FLT_MAX;

	if ( pDataResult )
		*pDataResult = NULL;
	if ( rcView )
		_rcView.SetRect(rcView);

	// 指定座標に一番近いｵﾌﾞｼﾞｪｸﾄを検索
	PLIST_FOREACH(CDXFdata* pData, this)
		// DXFViewからの呼び出しで pData が NULL の可能性アリ
		if ( !pData )
			continue;
		if ( rcView && !rc.CrossRect(_rcView, pData->GetMaxRect()) )
			continue;
		dGap = pData->GetSelectPointGap(pt);
		if ( dGap < dGapMin ) {
			dGapMin = dGap;
			if ( pDataResult )
				*pDataResult = pData;
		}
	END_FOREACH

	return dGapMin;
}

void CDXFchain::SetVectorPoint(POSITION pos1, CVPointF& vpt, float k)
{
	POSITION	pos2 = pos1;
	CDXFdata*	pData;

	do {
		pData = GetSeqData(pos1);
		if ( !pos1 )
			pos1 = GetFirstPosition();
		pData->SetVectorPoint(vpt, k);
	} while ( pos1 != pos2 );
}

void CDXFchain::SetVectorPoint(POSITION pos1, CVPointF& vpt, size_t n)
{
	POSITION	pos2 = pos1;
	CDXFdata*	pData;
	size_t		nCnt, nTotal = 0;
	vector<size_t>	vCnt;
	vector<size_t>::iterator	it;

	float		L = GetLength();	// 総長さ

	// 各ｵﾌﾞｼﾞｪｸﾄの長さから分割する割合を計算
	do {
		pData = GetSeqData(pos1);
		if ( !pos1 )
			pos1 = GetFirstPosition();
		nCnt = (size_t)(n * pData->GetLength() / L + 0.5);	// 四捨五入
		nTotal += nCnt;
		vCnt.push_back(nCnt);
	} while ( pos1 != pos2 );

	// 四捨五入するので過不足を調整
	if ( nTotal > n ) {
		do {
			it = max_element(vCnt.begin(), vCnt.end());
			(*it)--;
			nTotal--;
		} while ( nTotal > n );
	}
	else if ( nTotal < n ) {
		do {
			it = min_element(vCnt.begin(), vCnt.end());
			(*it)++;
			nTotal++;
		} while ( nTotal < n );
	}

	// 調整後の分割数で各ｵﾌﾞｼﾞｪｸﾄを分割
	it = vCnt.begin();
	do {
		pData = GetSeqData(pos1);
		if ( !pos1 )
			pos1 = GetFirstPosition();
		pData->SetVectorPoint(vpt, *it++);
	} while ( pos1 != pos2 );
}

void CDXFchain::SetShapeSwitch(BOOL bSelect)
{
	PLIST_FOREACH(CDXFdata* pData, this)
		pData->SetDxfFlg(DXFFLG_SELECT, bSelect);
	END_FOREACH
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
	DWORD	dwSel, dwSelBak = 0;

	CBrush* pOldBrush = (CBrush *)pDC->SelectStockObject(NULL_BRUSH);
	CPen* pOldPen = pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenDXF(DXFPEN_CUTTER));
	pDC->SetROP2(R2_COPYPEN);
	PLIST_FOREACH(CDXFdata* pData, this)
		dwSel = pData->GetDxfFlg() & DXFFLG_SELECT;
		if ( dwSel != dwSelBak ) {
			dwSelBak = dwSel;
			pDC->SelectObject(pData->GetDrawPen());
		}
		pData->Draw(pDC);
	END_FOREACH
	pDC->SelectObject(pOldPen);
	pDC->SelectObject(pOldBrush);
}

void CDXFchain::OrgTuning(void)
{
	PLIST_FOREACH(CDXFdata* pData, this)
		pData->OrgTuning(FALSE);
	END_FOREACH
	// 自分自身のﾌﾗｸﾞも初期化
	m_dwFlags &= ~(DXFMAPFLG_MAKE|DXFMAPFLG_SEARCH);
}

/////////////////////////////////////////////////////////////////////////////
// ＤＸＦデータの形状集合クラス
/////////////////////////////////////////////////////////////////////////////
CDXFshape::CDXFshape()
{
	Constructor(DXFSHAPE_OUTLINE, NULL, 0, NULL);
}

CDXFshape::CDXFshape(DXFSHAPE_ASSEMBLE enAssemble, LPCTSTR lpszShape, DWORD dwFlags, CLayerData* pLayer, CDXFchain* pChain)
{
	m_vShape = pChain;
	Constructor(enAssemble, lpszShape, dwFlags, pLayer);
	// 座標ﾏｯﾌﾟのｵﾌﾞｼﾞｪｸﾄ最大矩形と所属ﾏｯﾌﾟの更新
	SetDetailInfo(pChain);
}

CDXFshape::CDXFshape(DXFSHAPE_ASSEMBLE enAssemble, LPCTSTR lpszShape, DWORD dwFlags, CLayerData* pLayer, CDXFmap* pMap)
{
	m_vShape = pMap;
	Constructor(enAssemble, lpszShape, dwFlags, pLayer);
	// 座標ﾏｯﾌﾟのｵﾌﾞｼﾞｪｸﾄ最大矩形と所属ﾏｯﾌﾟの更新
	SetDetailInfo(pMap);
}

void CDXFshape::Constructor(DXFSHAPE_ASSEMBLE enAssemble, LPCTSTR lpszShape, DWORD dwFlags, CLayerData* pLayer)
{
	m_enAssemble	= enAssemble;
	m_pParentLayer	= pLayer;
	m_dwFlags	= dwFlags;
	m_dOffset	= 1.0f;		// ﾃﾞﾌｫﾙﾄｵﾌｾｯﾄ値
	m_nInOut	= -1;		// ﾃﾞﾌｫﾙﾄ輪郭方向無し
	m_bAcute	= TRUE;
	m_hTree		= NULL;
	m_nSerialSeq= 0;
	if ( lpszShape && lstrlen(lpszShape) > 0 )
		m_strShapeHandle = m_strShape = lpszShape;
	// ｵﾌﾞｼﾞｪｸﾄ矩形の初期化
	m_rcMax.SetRectMinimum();
}

void CDXFshape::SetDetailInfo(CDXFchain* pChain)
{
	PLIST_FOREACH(CDXFdata* pData, pChain)
		m_rcMax |= pData->GetMaxRect();
		pData->SetParentMap(this);
	END_FOREACH
}

void CDXFshape::SetDetailInfo(CDXFmap* pMap)
{
	CDXFarray*	pArray;
	CDXFdata*	pData;
	CPointF	pt;
	int		i;
	PMAP_FOREACH(pt, pArray, pMap)
		for ( i=0; i<pArray->GetSize(); i++ ) {
			pData = pArray->GetAt(i);
			m_rcMax |= pData->GetMaxRect();
			pData->SetParentMap(this);
		}
	END_FOREACH
}

CDXFshape::~CDXFshape()
{
	PLIST_FOREACH(auto ref, &m_ltWork)
		delete	ref;
	END_FOREACH
	PLIST_FOREACH(auto ref, &m_ltOutline)
		delete	ref;
	END_FOREACH
	m_ltWork.RemoveAll();
	m_ltOutline.RemoveAll();
	CrearScanLine_Lathe();		// Clear m_obLathe

	// m_vShape内のｵﾌﾞｼﾞｪｸﾄはCDXFshapeで作られたﾃﾞｰﾀではないので
	// ここでdeleteしてはいけない
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
		ar << (m_dwFlags & ~DXFMAPFLG_SELECT) << nAssemble <<	// 選択状態除く
			m_strShape << m_strShapeHandle <<
			m_dOffset << m_nInOut << m_bAcute <<
			nType;
		if ( nType == 0 )
			get<CDXFchain*>(m_vShape)->Serialize(ar);
		else
			get<CDXFmap*>(m_vShape)->Serialize(ar);
	}
	else {
		m_pParentLayer = static_cast<CDXFDoc *>(ar.m_pDocument)->GetSerializeLayer();
		ar >> m_dwFlags >> nAssemble >> m_strShape;
		if ( g_dwCamVer > NCVCSERIALVERSION_1507 )	// Ver1.60～
			ar >> m_strShapeHandle;
		if ( g_dwCamVer < NCVCSERIALVERSION_3620 ) {
			double	d;
			ar >> d;
			m_dOffset = (float)d;
		}
		else {
			ar >> m_dOffset;
		}
		ar >> m_nInOut;
		if ( g_dwCamVer > NCVCSERIALVERSION_1503 )	// Ver1.10～
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
		m_pParentLayer->SetActiveShape(this);
	}
	// 加工指示情報のｼﾘｱﾗｲｽﾞ
	m_ltWork.Serialize(ar);
	if ( ar.IsStoring() )
		m_ltOutline.Serialize(ar);
	else {
		if ( g_dwCamVer > NCVCSERIALVERSION_1600 )		// Ver1.70～
			m_ltOutline.Serialize(ar);
		else {
			// m_ltWork にある CDXFworkingOutline を m_ltOutline へ
			CDXFworkingOutline*	pWork;
			POSITION		pos1, pos2;
			for ( pos1=m_ltWork.GetHeadPosition(); (pos2=pos1); ) {
				pWork = static_cast<CDXFworkingOutline*>(m_ltWork.GetNext(pos1));
				if ( pWork->GetWorkingType() == WORK_OUTLINE ) {
					m_ltOutline.AddTail(pWork);
					m_ltWork.RemoveAt(pos2);
				}
			}
		}
	}
}

BOOL CDXFshape::AddWorkingData(CDXFworking* pWork)
{
	// 例外は上位で行う
	m_ltWork.AddTail(pWork);
	// ﾏｯﾌﾟﾌﾗｸﾞとﾀｰｹﾞｯﾄｵﾌﾞｼﾞｪｸﾄのﾌﾗｸﾞを設定
	ENWORKINGTYPE nType = pWork->GetWorkingType();
	m_dwFlags |= g_dwMapFlag[nType];
	CDXFdata*	pData = pWork->GetTargetObject();
	if ( pData && nType==WORK_DIRECTION )
		pData->SetDxfFlg(DXFFLG_DIRECTIONFIX);
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
					pData->SetDxfFlg(DXFFLG_DIRECTIONFIX, FALSE);
				delete	pWork;
			}
			bResult = TRUE;
			break;
		}
	}

	return bResult;
}

BOOL CDXFshape::AddOutlineData(CDXFworkingOutline* pOutline, int nInOut)
{
	m_ltOutline.AddTail(pOutline);
	m_dwFlags |= DXFMAPFLG_OUTLINE;
	m_nInOut   = nInOut;
	return TRUE;
}

BOOL CDXFshape::DelOutlineData(CDXFworkingOutline* pWork/*=NULL*/)
{
	POSITION	pos1, pos2;
	CDXFworkingOutline*	pOutline;
	for ( pos1=m_ltOutline.GetHeadPosition(); (pos2=pos1); ) {
		pOutline = m_ltOutline.GetNext(pos1);
		if ( !pWork ) {
			m_ltOutline.RemoveAt(pos2);
			delete	pOutline;
		}
		else if ( pWork && pWork==pOutline ) {
			m_ltOutline.RemoveAt(pos2);
			delete	pOutline;
			break;
		}
	}
	if ( m_ltOutline.IsEmpty() )
		m_dwFlags &= ~DXFMAPFLG_OUTLINE;

	return TRUE;
}

tuple<CDXFworking*, CDXFdata*> CDXFshape::GetDirectionObject(void) const
{
	CDXFworking*	pWork;
	CDXFdata*		pData = NULL;

	PLIST_FOREACH(pWork, &m_ltWork)
		if ( pWork->GetWorkingType() == WORK_DIRECTION ) {
			pData = pWork->GetTargetObject();
			break;
		}
	END_FOREACH

	return make_tuple(pWork, pData);
}

tuple<CDXFworking*, CDXFdata*> CDXFshape::GetStartObject(void) const
{
	CDXFworking*	pWork;
	CDXFdata*		pData = NULL;

	PLIST_FOREACH(pWork, &m_ltWork)
		if ( pWork->GetWorkingType() == WORK_START ) {
			pData = pWork->GetTargetObject();
			break;
		}
	END_FOREACH

	return make_tuple(pWork, pData);
}

POSITION CDXFshape::GetFirstChainPosition(void)
{
	float			dGap1, dGap2;
	const CPointF	ptOrg( CDXFdata::ms_ptOrg );
	CPointF			ptNow;
	CDXFworking*	pWork;
	CDXFdata*		pData;
	CDXFdata*		pDataFix;
	CDXFchain*		pChain = GetShapeChain();

	m_dwFlags &= ~(DXFMAPFLG_REVERSE|DXFMAPFLG_NEXTPOS);

	// 開始位置指示
	tie(pWork, pData) = GetStartObject();
	if ( pData ) {
		// 先頭ｵﾌﾞｼﾞｪｸﾄと現在位置を更新
		pDataFix = pData;
		ptNow = static_cast<CDXFworkingStart*>(pWork)->GetStartPoint() - ptOrg;	// 原点調整兼ねる
	}
	else {
		ptNow = CDXFdata::ms_pData->GetEndCutterPoint();
		// 開始ｵﾌﾞｼﾞｪｸﾄの検索 (pDataに近接ｵﾌﾞｼﾞｪｸﾄをｾｯﾄ)
		GetSelectObjectFromShape(ptNow+ptOrg, NULL, &pDataFix);
	}
	ASSERT(pDataFix);

	// 方向指示および生成順のﾁｪｯｸ
	tie(pWork, pData) = GetDirectionObject();
	if ( pChain->GetCount()==1 && pDataFix->IsStartEqEnd() ) {
		if ( pData ) {
			CPointF	pts( static_cast<CDXFworkingDirection*>(pWork)->GetStartPoint() ),
					pte( static_cast<CDXFworkingDirection*>(pWork)->GetArrowPoint() );
			// 回転設定
			pData->IsDirectionPoint(pts, pte);
		}
		pDataFix->GetEdgeGap(ptNow);	// 輪郭ｵﾌﾞｼﾞｪｸﾄの近接座標計算
	}
	else {
		dGap1 = GAPCALC(pDataFix->GetStartCutterPoint() - ptNow);
		dGap2 = GAPCALC(pDataFix->GetEndCutterPoint()   - ptNow);
		if ( pData ) {
			CPointF	ptFix( static_cast<CDXFworkingDirection*>(pWork)->GetArrowPoint() - ptOrg );
			if ( pData->GetEndCutterPoint().IsMatchPoint(&ptFix) ) {
				// 開始ｵﾌﾞｼﾞｪｸﾄの終点の方が近い場合は、
				// 次のｵﾌﾞｼﾞｪｸﾄから開始
				if ( dGap1 > dGap2 )
					m_dwFlags |= DXFMAPFLG_NEXTPOS;
			}
			else {
				m_dwFlags |= DXFMAPFLG_REVERSE;
				// 開始ｵﾌﾞｼﾞｪｸﾄの終点は bReverse なので始点で判断
				if ( dGap1 < dGap2 )
					m_dwFlags |= DXFMAPFLG_NEXTPOS;
			}
		}
		else {
			if ( dGap1 > dGap2 )
				m_dwFlags |= DXFMAPFLG_REVERSE;
		}
	}

	return pChain->SetLoopFunc(pDataFix, m_dwFlags&DXFMAPFLG_REVERSE, m_dwFlags&DXFMAPFLG_NEXTPOS);
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
		CDXFchain* pChain = GetShapeChain();	// 移行後にdelete
		if ( !ChangeCreate_ChainToMap(pChain) )
			return FALSE;
		delete	pChain;
	}

	return TRUE;
}

BOOL CDXFshape::ChangeCreate_MapToChain(CDXFmap* pMap/*=NULL*/)
{
	CDXFchain*	pChain = NULL;
	if ( !pMap )
		pMap = GetShapeMap();

	try {
		// 新たな形状集合を生成
		pChain = new CDXFchain;
		if ( pMap->CopyToChain(pChain) ) {
			// CDXFshape 情報更新
			m_enAssemble = DXFSHAPE_OUTLINE;
			m_vShape = pChain;
			m_dwFlags = 0;
			m_strShape += "(昇格)";
		}
		else {
			delete pChain;
			return FALSE;
		}
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

BOOL CDXFshape::ChangeCreate_ChainToMap(CDXFchain* pChain/*=NULL*/)
{
	CDXFmap*	pMap = NULL;
	if ( !pChain )
		pChain = GetShapeChain();

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
		DelOutlineData();
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
	CPointF		pt;
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
			PLIST_FOREACH(CDXFdata* pData, pChain)
				pt = pData->GetNativePoint(1);
				if ( pMapTmp->PLookup(pt) ) {
					// １つでも見つかれば結合処理
					bResult = TRUE;
					pMapTmp->Append(pChain);
					break;
				}
			END_FOREACH
		}
		else {
			pMap = pShape->GetShapeMap();
			CDXFarray*	pDummy;
			PMAP_FOREACH(pt, pDummy, pMap)
				if ( pMapTmp->PLookup(pt) ) {
					bResult = TRUE;
					pMapTmp->Append(pMap);
					break;
				}
			END_FOREACH
		}
		if ( !bResult ) {
			delete	pMapTmp;
			return FALSE;
		}
		// 不要加工指示(輪郭指示など)を削除
		DelOutlineData();
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

BOOL CDXFshape::CreateOutlineTempObject(BOOL bLeft, CDXFchain* pResult, float dOffset/*=0.0f*/)
{
#ifdef _DEBUG
	printf("CreateOutlineTempObject() Start\n");
	CPointF		ptDbg1, ptDbg2, ptDbg3;
#endif
	if ( dOffset <= 0.0f )
		dOffset = m_dOffset;	// ﾃﾞﾌｫﾙﾄｵﾌｾｯﾄ値

	// 占有矩形領域の初期化
	pResult->ClearMaxRect();

	const CDXFchain*	pChain = GetShapeChain();
	if ( !pChain || pChain->IsEmpty() )
		return FALSE;

	CTypedPtrArrayEx<CPtrArray, CDXFlist*>	obSepArray;
	obSepArray.SetSize(0, 32);

	int			k = bLeft ? -1 : 1;
	INT_PTR		nLoop;
	BOOL		bResult = TRUE;
	CDXFdata*	pData;
	CPointF		pt, pte;
	optional<CPointF>	ptResult, pts;
	POSITION	pos;

	// 唯一のｵﾌﾞｼﾞｪｸﾄが閉ﾙｰﾌﾟなら
	if ( pChain->GetCount() == 1 ) {
		pData = pChain->GetHead();
		CDXFcircle*		pCircle;
		CDXFellipse*	pEllipse;
		CDXFpolyline*	pPolyline;
		switch ( pData->GetType() ) {
		case DXFCIRCLEDATA:
			pCircle = static_cast<CDXFcircle*>(pData);
			// ｵﾌｾｯﾄ半径のﾏｲﾅｽﾁｪｯｸ
			pData = pCircle->GetR()+dOffset*k > NCMIN ?
				::CreateDxfOffsetObject(pData, pte, pte, k, dOffset) :	// pte is dummy
				NULL;
			break;
		case DXFELLIPSEDATA:
			pEllipse = static_cast<CDXFellipse*>(pData);
			if ( pEllipse->IsArc() )
				return FALSE;
			pData = (pEllipse->GetLongLength() +dOffset*k > NCMIN &&
						pEllipse->GetShortLength()+dOffset*k > NCMIN) ?
				::CreateDxfOffsetObject(pData, pte, pte, k, dOffset) :
				NULL;
			break;
		case DXFPOLYDATA:
			pPolyline = static_cast<CDXFpolyline*>(pData);
			if ( !pPolyline->IsStartEqEnd() || pPolyline->IsIntersection() )
				return FALSE;
			bResult = CreateOutlineTempObject_polyline(pPolyline, bLeft, dOffset, pResult, obSepArray);
			nLoop = obSepArray.GetSize();
			if ( bResult ) {
				CheckSeparateChain(pResult, dOffset);
				for ( k=0; k<nLoop; k++ ) {
					CheckSeparateChain(obSepArray[k], dOffset);
					if ( !obSepArray[k]->IsEmpty() ) {
						if ( !pResult->IsEmpty() )
							pResult->AddTail((CDXFdata *)NULL);
						pResult->AddTail(obSepArray[k]);
					}
					delete	obSepArray[k];
				}
			}
			else {
				for ( k=0; k<nLoop; k++ ) {
					PLIST_FOREACH(pData, obSepArray[k])
						delete	pData;
					END_FOREACH
					delete	obSepArray[k];
				}
			}
			return bResult;
		default:
			return FALSE;	// 円・楕円・ﾎﾟﾘﾗｲﾝの閉ﾙｰﾌﾟ以外はｴﾗｰ
		}
		if ( pData ) {
			pResult->AddTail(pData);
			pResult->SetMaxRect(pData);
		}
		return TRUE;
	}

	// 輪郭ｵﾌﾞｼﾞｪｸﾄﾙｰﾌﾟ準備
	pos = pChain->GetHeadPosition();
	CDXFdata*	pData1 = pChain->GetNext(pos);	// 最初のｵﾌﾞｼﾞｪｸﾄ
	CDXFdata*	pData2;

	// CDXFchainは，始点終点の調整が行われているので
	// 単純ﾙｰﾌﾟで良い
	while ( pos ) {		// ２回目以降からﾙｰﾌﾟ
		pData2 = pChain->GetNext(pos);
#ifdef _DEBUG
		ptDbg1 = pData1->GetNativePoint(0);
		ptDbg2 = pData1->GetNativePoint(1);
		ptDbg3 = pData2->GetNativePoint(1);
		printf("p1=(%.3f, %.3f) p2=(%.3f, %.3f) p3=(%.3f, %.3f)\n",
			ptDbg1.x, ptDbg1.y, ptDbg2.x, ptDbg2.y, ptDbg3.x, ptDbg3.y);
#endif
		// ｵﾌｾｯﾄ座標計算
		ptResult = pData1->CalcOffsetIntersectionPoint(pData2, dOffset, bLeft);
		if ( !ptResult ) {
			bResult = FALSE;
			break;
		}
		pt = *ptResult;
#ifdef _DEBUG
		printf("Offset=(%.3f, %.3f)\n", pt.x, pt.y);
#endif
		// 輪郭ｵﾌﾞｼﾞｪｸﾄ生成(初回は無視)
		if ( pts ) {
			pData = ::CreateDxfOffsetObject(pData1, *pts, pt, k, dOffset);
			if ( pData ) {
				pResult->AddTail(pData);
				pResult->SetMaxRect(pData);
				// ｵﾌｾｯﾄｵﾌﾞｼﾞｪｸﾄの交点検査
				if ( !SeparateOutlineIntersection(pResult, obSepArray) ) {
					bResult = FALSE;
					break;
				}
			}
		}
		else
			pte = pt;		// 最初の輪郭ｵﾌﾞｼﾞｪｸﾄの終点
		pts = pt;
		pData1 = pData2;
	}

	if ( bResult ) {
		// 残りの輪郭ｵﾌﾞｼﾞｪｸﾄ生成(最後のｵﾌﾞｼﾞｪｸﾄと先頭のｵﾌﾞｼﾞｪｸﾄ)
		pData2 = pChain->GetHead();
#ifdef _DEBUG
		ptDbg1 = pData1->GetNativePoint(0);
		ptDbg2 = pData1->GetNativePoint(1);
		ptDbg3 = pData2->GetNativePoint(1);
		printf("p1=(%.3f, %.3f) p2=(%.3f, %.3f) p3=(%.3f, %.3f)\n",
			ptDbg1.x, ptDbg1.y, ptDbg2.x, ptDbg2.y, ptDbg3.x, ptDbg3.y);
#endif
		ptResult = pData1->CalcOffsetIntersectionPoint(pData2, dOffset, bLeft);
		if ( ptResult ) {
			pt = *ptResult;
#ifdef _DEBUG
			printf("Offset=(%.3f, %.3f)\n", pt.x, pt.y);
#endif
			pData = ::CreateDxfOffsetObject(pData1, *pts, pt, k, dOffset);
			if ( pData ) {
				pResult->AddTail(pData);
				pResult->SetMaxRect(pData);
				if ( !SeparateOutlineIntersection(pResult, obSepArray) )
					bResult = FALSE;
			}
			// 最初のｵﾌﾞｼﾞｪｸﾄ(の終点で)
			if ( bResult ) {
				pData = ::CreateDxfOffsetObject(pData2, pt, pte, k, dOffset);
				if ( pData ) {
					pResult->AddTail(pData);
					pResult->SetMaxRect(pData);
					if ( !SeparateOutlineIntersection(pResult, obSepArray, TRUE) )
						bResult = FALSE;
				}
			}
		}
		else
			bResult = FALSE;
	}

	nLoop = obSepArray.GetSize();
	if ( !bResult ) {
		// ｴﾗｰﾘｶﾊﾞﾘ
		for ( k=0; k<nLoop; k++ ) {
			PLIST_FOREACH(pData, obSepArray[k])
				delete	pData;
			END_FOREACH
			delete	obSepArray[k];
		}
		return FALSE;
	}

#ifdef _DEBUG
	printf("Result member = %d\n", pResult->GetCount());
#endif
	// 本集合の検査
	CheckSeparateChain(pResult, dOffset);
#ifdef _DEBUG
	printf("CheckSeparateChain() -> Result member = %d\n", pResult->GetCount());
	printf("obSepArray.GetSize()=%d\n", nLoop);
#endif

	// 分離集合の検査
	for ( k=0; k<nLoop; k++ ) {
#ifdef _DEBUG
		printf("Separate member = %d\n", obSepArray[k]->GetCount()); 
#endif
		CheckSeparateChain(obSepArray[k], dOffset);
#ifdef _DEBUG
		printf("CheckSeparateChain() -> Separate member = %d\n", obSepArray[k]->GetCount()); 
#endif
		// 分離集合を末尾に結合
		if ( !obSepArray[k]->IsEmpty() ) {
			if ( !pResult->IsEmpty() )
				pResult->AddTail((CDXFdata *)NULL);		// 分離ﾏｰｸ
			pResult->AddTail(obSepArray[k]);
		}
		delete	obSepArray[k];
	}

	return !pResult->IsEmpty();
}

BOOL CDXFshape::CreateOutlineTempObject_polyline
	(const CDXFpolyline* pPolyline, BOOL bLeft, float dOffset,
		CDXFchain* pResult, CTypedPtrArrayEx<CPtrArray, CDXFlist*>& obSepArray)
{
	int		k = bLeft ? -1 : 1;
	BOOL	bResult = TRUE;
	CDXFdata*	pData;
	CDXFdata*	pData1 = NULL;
	CDXFdata*	pData2 = NULL;
	CDXFdata*	pDataFirst = NULL;
	CDXFarray	obTemp;		// 一時ｵﾌﾞｼﾞｪｸﾄ格納
	optional<CPointF>	pt1, pr1, ptResult;
	CPointF				pt2, pte, pt;
	DXFLARGV	dxfLine;
	dxfLine.pLayer = pPolyline->GetParentLayer();
	obTemp.SetSize(0, pPolyline->GetVertexCount());

	for ( POSITION pos=pPolyline->GetFirstVertex(); pos; ) {
		pData = pPolyline->GetNextVertex(pos);
		if ( pData->GetType() == DXFPOINTDATA ) {
			pt2 = pData->GetNativePoint(0);
			if ( pt1 ) {
				dxfLine.s = *pt1;
				dxfLine.e = pt2;
				pData2 = new CDXFline(&dxfLine);
				obTemp.Add(pData2);
			}
			else
				pData2 = NULL;
			pt1 = pt2;
		}
		else {
			pData2 = pData;
			pt1.reset();
		}
		//
		if ( pData1 && pData2 ) {
			ptResult = pData1->CalcOffsetIntersectionPoint(pData2, dOffset, bLeft);
			if ( !ptResult ) {
				bResult = FALSE;
				break;
			}
			pt = *ptResult;
			if ( pr1 ) {
				pData = ::CreateDxfOffsetObject(pData1, *pr1, pt, k, dOffset);
				if ( pData ) {
					pResult->AddTail(pData);
					pResult->SetMaxRect(pData);
					if ( !SeparateOutlineIntersection(pResult, obSepArray) ) {
						bResult = FALSE;
						break;
					}
				}
			}
			else
				pte = pt;
			pr1 = pt;
		}
		if ( pData2 )
			pData1 = pData2;
		if ( !pDataFirst && pData1 )
			pDataFirst = pData1;
	}

	if ( bResult && pDataFirst ) {
		ptResult = pData1->CalcOffsetIntersectionPoint(pDataFirst, dOffset, bLeft);
		if ( ptResult ) {
			pt = *ptResult;
			pData = ::CreateDxfOffsetObject(pData1, *pr1, pt, k, dOffset);
			if ( pData ) {
				pResult->AddTail(pData);
				pResult->SetMaxRect(pData);
				if ( !SeparateOutlineIntersection(pResult, obSepArray) )
					bResult = FALSE;
			}
			if ( bResult ) {
				pData = ::CreateDxfOffsetObject(pDataFirst, pt, pte, k, dOffset);
				if ( pData ) {
					pResult->AddTail(pData);
					pResult->SetMaxRect(pData);
					if ( !SeparateOutlineIntersection(pResult, obSepArray, TRUE) )
						bResult = FALSE;
				}
			}
		}
		else
			bResult = FALSE;
	}

	for ( k=0; k<obTemp.GetSize(); k++ )
		delete	obTemp[k];

	return bResult;
}

BOOL CDXFshape::CreateScanLine_X(CDXFchain* pResult)
{
	CDXFdata*	pData;
	DXFLARGV	dxfLine, dxfLine2;
	INT_PTR		nLoop1 = m_ltOutline.GetCount(), nLoop2;
	if ( nLoop1 == 0 )
		nLoop1 = nLoop2 = 1;
	else
		nLoop2 = nLoop1 + 1;		// 外周が既に存在する

	dxfLine.pLayer = dxfLine2.pLayer = NULL;
	dxfLine.s = dxfLine.e = m_rcMax.TopLeft();
	dxfLine.s.x += m_dOffset*nLoop1;
	dxfLine.s.y += m_dOffset*nLoop2;
	dxfLine.e.x = m_rcMax.right - m_dOffset*nLoop1;
	dxfLine.e.y = dxfLine.s.y;
	float	ptEnd = ::RoundUp(m_rcMax.bottom - m_dOffset*nLoop1);

	while ( TRUE ) {
		pData = new CDXFline(&dxfLine);
		pResult->AddTail(pData);
		pResult->SetMaxRect(pData);
		dxfLine2 = dxfLine;
		dxfLine.s.y += m_dOffset;
		if ( ::RoundUp(dxfLine.s.y) <= ptEnd ) {
			dxfLine2.s = dxfLine.e;
			dxfLine2.e.x = dxfLine.e.x;
			dxfLine2.e.y = dxfLine.s.y;
			pData = new CDXFline(&dxfLine2);
			pResult->AddTail(pData);
			dxfLine.e.y = dxfLine.s.y;
			swap(dxfLine.s, dxfLine.e);
		}
		else
			break;	// ﾙｰﾌﾟ終了条件
	}

	return TRUE;
}

BOOL CDXFshape::CreateScanLine_Y(CDXFchain* pResult)
{
	CDXFdata*	pData;
	DXFLARGV	dxfLine, dxfLine2;
	INT_PTR		nLoop1 = m_ltOutline.GetCount(), nLoop2;
	if ( nLoop1 == 0 )
		nLoop1 = nLoop2 = 1;
	else
		nLoop2 = nLoop1 + 1;

	dxfLine.pLayer = dxfLine2.pLayer = NULL;
	dxfLine.s = dxfLine.e = m_rcMax.TopLeft();
	dxfLine.s.x += m_dOffset*nLoop2;
	dxfLine.s.y += m_dOffset*nLoop1;
	dxfLine.e.x = dxfLine.s.x;
	dxfLine.e.y = m_rcMax.bottom - m_dOffset*nLoop1;
	float	ptEnd = ::RoundUp(m_rcMax.right - m_dOffset*nLoop1);

	while ( TRUE ) {
		pData = new CDXFline(&dxfLine);
		pResult->AddTail(pData);
		pResult->SetMaxRect(pData);
		dxfLine.s.x += m_dOffset;
		if ( ::RoundUp(dxfLine.s.x) <= ptEnd ) {
			dxfLine2.s = dxfLine.e;
			dxfLine2.e.x = dxfLine.s.x;
			dxfLine2.e.y = dxfLine.e.y;
			pData = new CDXFline(&dxfLine2);
			pResult->AddTail(pData);
			dxfLine.e.x = dxfLine.s.x;
			swap(dxfLine.s, dxfLine.e);
		}
		else
			break;	// ﾙｰﾌﾟ終了条件
	}

	return TRUE;
}

BOOL CDXFshape::CreateScanLine_Outline(CDXFchain* pResult)
{
	INT_PTR		n = m_ltOutline.GetCount() + 1;
	CDXFchain	ltOutline;

	while ( CreateOutlineTempObject(m_nInOut, &ltOutline, m_dOffset*n++) ) {
		if ( ltOutline.IsEmpty() )
			break;
		pResult->AddTail(&ltOutline);
		pResult->AddTail((CDXFdata *)NULL);
		ltOutline.RemoveAll();
	}

	return TRUE;
}

BOOL CDXFshape::CreateScanLine_ScrollCircle(CDXFchain* pResult)
{
	DXFAARGV	dxfArc;
	CDXFdata*	pData;
	const CDXFcircle*	pCircleOrg = static_cast<const CDXFcircle *>(GetShapeChain()->GetHead());
	int			nCnt = 1;
	float		r = m_dOffset / 2.0f,
				dMax = pCircleOrg->GetR() - m_dOffset * m_ltOutline.GetCount();
	CPointF		pto(pCircleOrg->GetCenter()),
				pts(pto), pte(pts.x+m_dOffset, pts.y);

	dxfArc.pLayer = NULL;
	dxfArc.r	= r;
	dxfArc.c.x	= pto.x + r;
	dxfArc.c.y	= pto.y;
	dxfArc.sq	= PI;	// RAD(180.0)
	dxfArc.eq	= 0.0f;

	while ( dMax - fabs(pte.x-pto.x) > NCMIN ) {
		pData = new CDXFarc(&dxfArc, FALSE, pts, pte);
		pResult->AddTail(pData);

		dxfArc.r += r;
		pts.x = pte.x;
		if ( nCnt++ & 0x01 ) {
			dxfArc.c.x = pto.x;
			pte.x = dxfArc.c.x - dxfArc.r;
			dxfArc.sq = 0.0f;
			dxfArc.eq = -PI;
		}
		else {
			dxfArc.c.x = pto.x + r;
			pte.x = dxfArc.c.x + dxfArc.r;
			dxfArc.sq = PI;
			dxfArc.eq = 0.0f;
		}
	}

	// 残りの螺旋を生成
	pte.x = pto.x + dMax;				// 外周円の右接終点
	dxfArc.r = (pte.x - pts.x) / 2.0f;	// 終点から逆算した半径と
	dxfArc.c.x = pte.x - dxfArc.r;		// 中心
	pData = new CDXFarc(&dxfArc, FALSE, pts, pte);
	pResult->AddTail(pData);

	return TRUE;
}

void CDXFshape::CreateScanLine_Lathe(int n, float d)
{
	CDXFchain*	pChainOrig = GetShapeChain();
	if ( !pChainOrig )
		return;

	CDXFchain*	pChain;
	CDXFdata*	pData;

	while ( n > 0 ) {
		pChain = new CDXFchain;
		PLIST_FOREACH(CDXFdata* pDataOrig, pChainOrig)
			pData = CreateDxfLatheObject(pDataOrig, n*d);
			if ( pData ) {
				pChain->AddTail(pData);
				pChain->SetMaxRect(pData);
			}
		END_FOREACH
		n--;
		m_obLathe.Add(pChain);
	}
}

void CDXFshape::CrearScanLine_Lathe(void)
{
	for ( int i=0; i<m_obLathe.GetSize(); i++ ) {
		CDXFchain* pChain = m_obLathe[i];
		PLIST_FOREACH(CDXFdata* pData, pChain)
			delete	pData;
		END_FOREACH
		delete	pChain;
	}
	m_obLathe.RemoveAll();
}

BOOL CDXFshape::SeparateOutlineIntersection
	(CDXFchain* pOffset, CTypedPtrArrayEx<CPtrArray, CDXFlist*>& obSepList, BOOL bFinish/*=FALSE*/)
{
#ifdef _DEBUG
	printf("SeparateOutlineIntersection() Start\n");
	CPointF		ptDbg1, ptDbg2, ptDbg3;
#endif
	POSITION	pos1 = pOffset->GetTailPosition(), pos2, pos;
	CDXFdata*	pData;
	CDXFdata*	pData1;
	CDXFdata*	pData2;
	CDXFlist*	pSepList = NULL;
	CPointF		pt[4];	// 交点は最大４つ
	int			nCnt, nLoop = 0;
	BOOL		bResult = TRUE, bUpdate;

	if ( pos1 )
		pData1 = pOffset->GetPrev(pos1);	// 検索対象ﾃﾞｰﾀ
	else
		return TRUE;

	for ( ; (pos2=pos1); nLoop++ ) {
		pData2 = pOffset->GetPrev(pos1);		// さかのぼって検索
		if ( nLoop == 0 ) {
			// １つ目は必ず pData1 の始点と pData2 の終点が等しい
			continue;
		}
		// ｵﾌｾｯﾄｵﾌﾞｼﾞｪｸﾄ同士の交点ﾁｪｯｸ(端点含む)
		nCnt = pData1->GetIntersectionPoint(pData2, pt, FALSE);
		ASSERT( nCnt <= 1 );	// 交点２個以上あればどうする？
		if ( nCnt <= 0 )
			continue;
		else if ( bFinish && !pos1 ) {
			// 最後のﾙｰﾌﾟは必ず pData1 の終点と pData2 の始点が等しい
			continue;
		}
		// --- 交点あり！！
#ifdef _DEBUG
		printf("pData1 type=%d pData2 type=%d\n", pData1->GetType(), pData2->GetType());
		printf("pt=(%.3f, %.3f)\n", pt[0].x, pt[0].y);
#endif
		pSepList = new CDXFlist;
		// 1) 交点から pData2 の終点までｵﾌﾞｼﾞｪｸﾄ生成
		if ( pData2->GetNativePoint(1).IsMatchPoint(&pt[0]) ) {
			// pData2 の終点と交点が等しい場合、
			// ｵﾌﾞｼﾞｪｸﾄ作らないが、次(next)のﾃﾞｰﾀから pSepList へ移動
			bUpdate = TRUE;
		}
		else if ( pData2->GetNativePoint(0).IsMatchPoint(&pt[0]) ) {
			// pData2 の始点と交点が等しい場合、
			// ｵﾌﾞｼﾞｪｸﾄ作らない代わりに pData2 自身が pSepList へ移動
			bUpdate = FALSE;
		}
		else {
			pData = ::CreateDxfOffsetObject(pData2, pt[0], pData2->GetNativePoint(1));
			if ( pData ) {
				pSepList->AddTail(pData);
				// 1-2) pData2 の終点を更新
				pData2->SetNativePoint(1, pt[0]);
			}
			bUpdate = TRUE;
		}
		// 2) pData2 または その次から pData1 の手前まで分離集合に移動
		pos = pos2;
		if ( bUpdate )
			pOffset->GetNext(pos);		// posを１つ進める
		while ( (pos2=pos) ) {
			pData = pOffset->GetNext(pos);
			if ( !pos )		// pData1 == pData
				break;
			pSepList->AddTail(pData);
			pOffset->RemoveAt(pos2);
		}
		// 3) pData1 の始点から交点までｵﾌﾞｼﾞｪｸﾄ生成
		if ( pData1->GetNativePoint(1).IsMatchPoint(&pt[0]) ) {
			// pData1の終点と交点が等しい場合
			bUpdate = FALSE;
			// pSepList へ移動
			pSepList->AddTail(pData1);
			pOffset->RemoveTail();		// RemoveAt(pos2);
		}
		// pData1の始点と交点が等しい場合は何もしなくて良い
		else if ( !pData1->GetNativePoint(0).IsMatchPoint(&pt[0]) ) {
			pData = ::CreateDxfOffsetObject(pData1, pData1->GetNativePoint(0), pt[0]);
			if ( pData ) {
				pSepList->AddTail(pData);
				// 3-1) pData1 の始点を更新
				pData1->SetNativePoint(0, pt[0]);
			}
		}
		// 4) 分離集合に追加
		if ( pSepList->IsEmpty() )
			delete	pSepList;
		else
			obSepList.Add(pSepList);
		pSepList = NULL;
	}

	if ( !bResult && pSepList ) {
		// ｺﾞﾐ掃除
		PLIST_FOREACH(pData, pSepList)
			delete	pData;
		END_FOREACH
	}

	return TRUE;
}

BOOL CDXFshape::CheckSeparateChain(CDXFlist* pResult, const float dOffset)
{
	int				nCnt = 0;
	POSITION		pos1, pos2, pos;
	CDXFdata*		pData;
	CMapPtrToPtr	mp, mpDel;		// NG点のﾎﾟｼﾞｼｮﾝｷｰ

	// NG点検査(終点で検索)
	for ( pos1=pResult->GetHeadPosition(); (pos2=pos1); ) {
		pData = pResult->GetNext(pos1);
		if ( CheckIntersectionCircle(pData->GetNativePoint(1), dOffset) )
			mp.SetAt(pos2, pData);		// １つでもNG点含む
		else
			nCnt++;						// OK点ｶｳﾝﾄ
	}

	// 分離集合の削除対象検査
	if ( nCnt <= 2 ) {		// OK点2個以下もｱｳﾄ
		for ( pos=pResult->GetHeadPosition(); pos; )
			delete	pResult->GetNext(pos);
		pResult->RemoveAll();
		return TRUE;
	}

	if ( mp.IsEmpty() )
		return TRUE;

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
	for ( pos=mpDel.GetStartPosition(); pos; ) {	// 後片付け
		mpDel.GetNextAssoc(pos, pKey, pVoid);
		mp.RemoveKey(pKey);
	}

	// NG点の枝刈り(終点でNG点を持つｵﾌﾞｼﾞｪｸﾄ)
	CDXFdata*	pData1;
	CDXFdata*	pData2;
	for ( pos=mp.GetStartPosition(); pos; ) {
		mp.GetNextAssoc(pos, pKey, pVoid);
		pos1 = (POSITION)pKey;
		pData1 = pResult->GetNext(pos1);
		if ( !pos1 )
			pos1 = pResult->GetHeadPosition();
		pData2 = pResult->GetNext(pos1);
		// ｵﾌﾞｼﾞｪｸﾄの終点変更
		pData1->SetNativePoint(1, pData2->GetNativePoint(1));
		pData2->SetDxfFlg(DXFFLG_OFFSET_EXCLUDE);	// 削除ﾏｰｸ
	}

	// ﾃﾞｰﾀ整理
	for ( pos1=pResult->GetHeadPosition(); (pos=pos1); ) {
		pData = pResult->GetNext(pos1);
		if ( pData->GetDxfFlg() & DXFFLG_OFFSET_EXCLUDE ) {
			pResult->RemoveAt(pos);
			delete	pData;
		}
	}

	return TRUE;
}

BOOL CDXFshape::CheckIntersectionCircle(const CPointF& ptc, float dOffset)
{
	const CDXFchain*	pChain = GetShapeChain();
	ASSERT( pChain );

	// ｵﾘｼﾞﾅﾙｵﾌﾞｼﾞｪｸﾄとの交点ﾁｪｯｸ
	PLIST_FOREACH(CDXFdata* pData, pChain)
		// 交点あり(==2)なら TRUE、「接する」(==1)はOK!
		if ( pData->CheckIntersectionCircle(ptc, dOffset) > 1 )
			return TRUE;
	END_FOREACH

	return FALSE;
}

void CDXFshape::AllChangeFactor(float dFactor) const
{
	PLIST_FOREACH(auto ref, &m_ltWork)
		ref->DrawTuning(dFactor);
	END_FOREACH
	PLIST_FOREACH(auto ref, &m_ltOutline)
		ref->DrawTuning(dFactor);
	END_FOREACH
}

void CDXFshape::DrawWorking(CDC* pDC) const
{
	CDXFworking*		pWork;
	CDXFworkingOutline*	pOutline;
	int		i = 0, j,
			a = 0, b;
	CPen*	pPen[] = {
		AfxGetNCVCMainWnd()->GetPenDXF(DXFPEN_OUTLINE),
		AfxGetNCVCMainWnd()->GetPenCom(COMPEN_SEL)
	};
	CPen*	pOldPen   = pDC->SelectObject(pPen[i]);
	CBrush* pOldBrush = (CBrush *)pDC->SelectStockObject(NULL_BRUSH);
	pDC->SetROP2(R2_COPYPEN);
	PLIST_FOREACH(pWork, &m_ltWork)
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
	END_FOREACH
	pDC->SelectStockObject(NULL_BRUSH);
	PLIST_FOREACH(pOutline, &m_ltOutline)
		j = pOutline->GetWorkingFlag() & DXFWORKFLG_SELECT ? 1 : 0;
		if ( i != j ) {
			i = j;
			pDC->SelectObject( pPen[i] );
		}
		pOutline->Draw(pDC);
	END_FOREACH

	pDC->SelectObject(pOldBrush);
	pDC->SelectObject(pOldPen);
}

INT_PTR CDXFshape::GetObjectCount(void) const
{
	return apply_visitor( GetObjectCount_Visitor(), m_vShape );
}

float CDXFshape::GetSelectObjectFromShape
	(const CPointF& pt, const CRectF* rcView/*=NULL*/, CDXFdata** pDataResult/*=NULL*/)
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

/////////////////////////////////////////////////////////////////////////////

CDXFdata* CreateDxfOffsetObject
	(const CDXFdata* pData, const CPointF& pts, const CPointF& pte,
		int k/*=0*/, float dOffset/*=0*/)
{
	CDXFdata*	pDataResult = NULL;

	switch ( pData->GetType() ) {
	case DXFLINEDATA:
		{
			if ( k!=0 && pts.IsMatchPoint(&pte) )
				break;
			DXFLARGV	dxfLine;
			dxfLine.pLayer = pData->GetParentLayer();
			dxfLine.s = pts;
			dxfLine.e = pte;
			pDataResult = new CDXFline(&dxfLine);
		}
		break;
	case DXFCIRCLEDATA:
		{
			DXFCARGV	dxfCircle;
			const CDXFcircle* pCircle = static_cast<const CDXFcircle*>(pData);
			dxfCircle.pLayer = pCircle->GetParentLayer();
			dxfCircle.c = pCircle->GetCenter();
			dxfCircle.r = pCircle->GetR() + dOffset * k;
			pDataResult = new CDXFcircle(&dxfCircle);
		}
		break;
	case DXFARCDATA:
		{
			DXFAARGV	dxfArc;
			const CDXFarc* pArc = static_cast<const CDXFarc*>(pData);
			BOOL	bRound = pArc->GetRoundOrig(), bCreate = TRUE;
			if ( !bRound && k!=0 )	// 左方向を基準
				k = -k;
			dxfArc.pLayer = pArc->GetParentLayer();
			dxfArc.c = pArc->GetCenter();
			dxfArc.r = pArc->GetR() + dOffset * k;
			if ( (dxfArc.sq=dxfArc.c.arctan(pts)) < 0.0f )
				dxfArc.sq += PI2;
			if ( (dxfArc.eq=dxfArc.c.arctan(pte)) < 0.0f )
				dxfArc.eq += PI2;
			// ｵﾘｼﾞﾅﾙ円弧とｵﾌｾｯﾄ円弧の回転方向ﾁｪｯｸ
			if ( k != 0 ) {
				optional<CPointF> ptResult = ::CalcIntersectionPoint_LL(
						pts, pArc->GetNativePoint(0),
						pte, pArc->GetNativePoint(1) );
				if ( ptResult )		// ｵﾘｼﾞﾅﾙ円弧とｵﾌｾｯﾄ円弧の回転方向が違う
					bRound = !bRound;
			}
			float	d;
			if ( bRound ) {
				// for CDXFarc::AngleTuning()
				d = ::RoundUp(DEG(dxfArc.sq));
				while ( d > ::RoundUp(DEG(dxfArc.eq)) )
					dxfArc.eq += PI2;
				// 円周の長さが既定値未満なら
				if ( k!=0 && dxfArc.r * ( dxfArc.eq - dxfArc.sq ) < NCMIN )
					bCreate = FALSE;
			}
			else {
				d = ::RoundUp(DEG(dxfArc.eq));
				while ( d > ::RoundUp(DEG(dxfArc.sq)) )
					dxfArc.sq += PI2;
				if ( k!=0 && dxfArc.r * ( dxfArc.sq - dxfArc.eq ) < NCMIN )
					bCreate = FALSE;
			}
			if ( bCreate )
				pDataResult = new CDXFarc(&dxfArc, bRound, pts, pte);
		}
		break;
	case DXFELLIPSEDATA:
		{
			DXFEARGV	dxfEllipse;
			const CDXFellipse* pEllipse = static_cast<const CDXFellipse*>(pData);
			if ( !pEllipse->GetRoundOrig() && k!=0 )	// 左方向を基準
				k = -k;
			dxfEllipse.pLayer = pEllipse->GetParentLayer();
			dxfEllipse.c	= pEllipse->GetCenter();
			float	l		= pEllipse->GetLongLength() + dOffset * k,
					lq		= pEllipse->GetLean();
			dxfEllipse.l.x	= l * cos(lq);
			dxfEllipse.l.y	= l * sin(lq);
			dxfEllipse.s	=(pEllipse->GetShortLength() + dOffset * k) / l;
			dxfEllipse.bRound = pEllipse->GetRoundOrig();
			if ( pEllipse->IsArc() ) {
				// 角度計算は長軸の傾きを考慮して計算
				CPointF	pt1(pts - dxfEllipse.c), pt2(pte - dxfEllipse.c);
				pt1.RoundPoint(-lq);
				pt2.RoundPoint(-lq);
				// 楕円の角度は atan2() ではない
				float	q = pt1.x / l;
				if ( q < -1.0f || 1.0f < q )
					q = copysign(1.0f, q);	// -1.0 or 1.0
				dxfEllipse.sq = copysign(acos(q), pt1.y);
				if ( dxfEllipse.sq < 0.0f )
					dxfEllipse.sq += PI2;
				q = pt2.x / l;
				if ( q < -1.0f || 1.0f < q )
					q = copysign(1.0f, q);
				dxfEllipse.eq = copysign(acos(q), pt2.y);
				if ( dxfEllipse.eq < 0.0f )
					dxfEllipse.eq += PI2;
				if ( k != 0 ) {
					optional<CPointF> ptResult = ::CalcIntersectionPoint_LL(
							pts, pEllipse->GetNativePoint(0),
							pte, pEllipse->GetNativePoint(1) );
					if ( ptResult ) {
						dxfEllipse.bRound = !dxfEllipse.bRound;
						swap(dxfEllipse.sq, dxfEllipse.eq);
					}
				}
				if ( dxfEllipse.bRound ) {
					q = ::RoundUp(DEG(dxfEllipse.sq));
					while ( q > ::RoundUp(DEG(dxfEllipse.eq)) )
						dxfEllipse.eq += PI2;
				}
				else {
					q = ::RoundUp(DEG(dxfEllipse.eq));
					while ( q > ::RoundUp(DEG(dxfEllipse.sq)) )
						dxfEllipse.sq += PI2;
				}
			}
			else {
				dxfEllipse.sq = pEllipse->GetStartAngle();
				dxfEllipse.eq = pEllipse->GetEndAngle();
			}
			pDataResult = new CDXFellipse(&dxfEllipse);
		}
		break;
	}

	return pDataResult;
}

CDXFdata*	CreateDxfLatheObject(const CDXFdata* pData, float yd)
{
	CDXFdata*	pDataResult = NULL;

	// 始点終点や中心点のY値だけをﾌﾟﾗｽしてｵﾌﾞｼﾞｪｸﾄ生成
	switch ( pData->GetType() ) {
	case DXFLINEDATA:
		{
			DXFLARGV	dxfLine;
			dxfLine.pLayer = pData->GetParentLayer();
			dxfLine.s = pData->GetNativePoint(0);
			dxfLine.e = pData->GetNativePoint(1);
			dxfLine.s.y += yd;
			dxfLine.e.y += yd;
			pDataResult = new CDXFline(&dxfLine);
		}
		break;
	case DXFARCDATA:
		{
			DXFAARGV	dxfArc;
			const CDXFarc* pArc = static_cast<const CDXFarc*>(pData);
			dxfArc.pLayer = pArc->GetParentLayer();
			dxfArc.c  = pArc->GetCenter();
			dxfArc.r  = pArc->GetR();
			dxfArc.sq = pArc->GetStartAngle();	// RADでOK
			dxfArc.eq = pArc->GetEndAngle();
			dxfArc.c.y += yd;
			CPointF	pts(pArc->GetNativePoint(0)), pte(pArc->GetNativePoint(1));
			pts.y += yd;
			pte.y += yd;
			pDataResult = new CDXFarc(&dxfArc, pArc->GetRoundOrig(), pts, pte);
		}
		break;
	case DXFELLIPSEDATA:
		{
			DXFEARGV	dxfEllipse;
			const CDXFellipse* pEllipse = static_cast<const CDXFellipse*>(pData);
			dxfEllipse.pLayer = pEllipse->GetParentLayer();
			dxfEllipse.c	= pEllipse->GetCenter();
			dxfEllipse.l	= pEllipse->GetLongPoint();
			dxfEllipse.s	= pEllipse->GetShortMagni();
			dxfEllipse.sq	= DEG(pEllipse->GetStartAngle());
			dxfEllipse.eq	= DEG(pEllipse->GetEndAngle());
			dxfEllipse.bRound = pEllipse->GetRoundOrig();
			dxfEllipse.c.y += yd;
			pDataResult = new CDXFellipse(&dxfEllipse);
		}
		break;
	}
	
	return pDataResult;
}
