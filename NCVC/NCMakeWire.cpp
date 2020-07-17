// NCMakeWire.cpp: CNCMakeWire クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NCVC.h"
#include "DXFdata.h"
#include "NCMakeWireOpt.h"
#include "NCMakeWire.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

// よく使う変数や呼び出しの簡略置換
#define	GetFlg		ms_pMakeOpt->GetFlag
#define	GetNum		ms_pMakeOpt->GetNum
#define	GetDbl		ms_pMakeOpt->GetDbl
#define	GetStr		ms_pMakeOpt->GetStr

//////////////////////////////////////////////////////////////////////
// CNCMakeWire 構築/消滅
//////////////////////////////////////////////////////////////////////

// 切削ﾃﾞｰﾀ
CNCMakeWire::CNCMakeWire(const CDXFdata* pData, float dFeed) : CNCMakeMill(pData, dFeed)
{
	// CNCMakeMillと同じ
}

// 上下異形状の切削ﾃﾞｰﾀ
CNCMakeWire::CNCMakeWire(const CDXFdata* pDataXY, const CDXFdata* pDataUV, float dFeed)
{
	// ここで生成されるのは、pDataXYとpDataUVが同じ生成ﾀｲﾌﾟのみ
	ASSERT( pDataXY->GetMakeType() == pDataUV->GetMakeType() );

	CString	strGcode;
	CPointF	ptxy, ptuv;

	switch ( pDataXY->GetMakeType() ) {
	case DXFLINEDATA:
		ptxy = pDataXY->GetEndMakePoint();
		ptuv = pDataUV->GetEndMakePoint() - ptxy;	// 偏差
		strGcode = GetValString(NCA_X, ptxy.x, FALSE) +
				   GetValString(NCA_Y, ptxy.y, FALSE) +
				   GetValString(NCA_U, ::RoundUp(ptuv.x), FALSE) +
				   GetValString(NCA_V, ::RoundUp(ptuv.y), FALSE);
		if ( !strGcode.IsEmpty() )
			m_strGcode = (*ms_pfnGetLineNo)() + (*ms_pfnGetGString)(1) +
						strGcode + GetFeedString(dFeed) + ms_strEOB;
		break;

	case DXFARCDATA:
		ptxy = pDataXY->GetEndMakePoint();
		ptuv = pDataUV->GetEndMakePoint() - ptxy;
		strGcode = GetValString(NCA_X, ptxy.x, FALSE) +
				   GetValString(NCA_Y, ptxy.y, FALSE) +
				   GetValString(NCA_U, ::RoundUp(ptuv.x), FALSE) +
				   GetValString(NCA_V, ::RoundUp(ptuv.y), FALSE) +
				   GetValString(NCA_I, static_cast<const CDXFarc*>(pDataXY)->GetIJK(NCA_I), FALSE) +
				   GetValString(NCA_J, static_cast<const CDXFarc*>(pDataXY)->GetIJK(NCA_J), FALSE);
		// through
	case DXFCIRCLEDATA:
		// CNCMakeBase::MakeCircle_IJ() 参考
	{
		int		nCode;
		const CDXFcircle*	pCircleXY = static_cast<const CDXFcircle*>(pDataXY);
		const CDXFcircle*	pCircleUV = static_cast<const CDXFcircle*>(pDataUV);
		ASSERT( pCircleXY->GetG() == pCircleUV->GetG() );	// 回転方向が同じ
		if ( pDataXY->GetMakeType() == DXFCIRCLEDATA ) {
			strGcode = pCircleXY->GetBaseAxis() > 1 ?	// X軸かY軸か
						   GetValString(NCA_J, pCircleXY->GetIJK(NCA_J), FALSE) :
						   GetValString(NCA_I, pCircleXY->GetIJK(NCA_I), FALSE);
			nCode = pCircleXY->IsRoundFixed() ?
						pCircleXY->GetG() : ms_nCircleCode;
		}
		else
			nCode = pCircleXY->GetG();

		CPointF	ptOrgUV(pCircleUV->GetMakeCenter() - pCircleXY->GetMakeCenter());
		CString	strGcodeKL(GetValString(NCA_K, ::RoundUp(ptOrgUV.x), TRUE) +	// bSpecial==TRUE
						   GetValString(NCA_L, ::RoundUp(ptOrgUV.y), TRUE));
		strGcode += strGcodeKL;
		if ( !strGcode.IsEmpty() )
			m_strGcode = (*ms_pfnGetLineNo)() + (*ms_pfnGetGString)(nCode) +
						strGcode + GetFeedString(dFeed) + ms_strEOB;
	}
		break;
	}
}

// 上下異形状を微細線分で生成
CNCMakeWire::CNCMakeWire(const CVPointF& vptXY, const CVPointF& vptUV, float dFeed)
{
	ASSERT( vptXY.size() == vptUV.size() );

	CString	strGcode;
	CPointF	pt;

	// 最初だけ送り速度を追加
	pt = vptUV[0] - vptXY[0];
	strGcode = GetValString(NCA_X, vptXY[0].x, FALSE) +
			   GetValString(NCA_Y, vptXY[0].y, FALSE) +
			   GetValString(NCA_U, ::RoundUp(pt.x), FALSE) +
			   GetValString(NCA_V, ::RoundUp(pt.y), FALSE);
	if ( !strGcode.IsEmpty() )
		m_strGcode = (*ms_pfnGetLineNo)() + (*ms_pfnGetGString)(1) +
					strGcode + GetFeedString(dFeed) + ms_strEOB;

	for ( size_t i=1; i<vptXY.size(); i++ ) {
		pt = vptUV[i] - vptXY[i];
		strGcode = GetValString(NCA_X, vptXY[i].x, FALSE) +
				   GetValString(NCA_Y, vptXY[i].y, FALSE) +
				   GetValString(NCA_U, ::RoundUp(pt.x), FALSE) +
				   GetValString(NCA_V, ::RoundUp(pt.y), FALSE);
		if ( !strGcode.IsEmpty() )
			m_strGarray.Add( (*ms_pfnGetLineNo)() + (*ms_pfnGetGString)(1) +
						strGcode + ms_strEOB );
	}
}

// XYのG[0|1]移動
CNCMakeWire::CNCMakeWire(int nCode, const CPointF& pt, float dFeed, float dTaper)
{
	// ﾃｰﾊﾟ指示ｺｰﾄﾞ
	CString	strTaper;
	if ( dTaper != 0.0 ) {
		if ( nCode == 0 )
			strTaper = GetGString(50) + "T0";
		else
			strTaper = GetGString(dTaper>0 ? 51:52) + "T" + (*ms_pfnGetValDetail)(fabs(dTaper));
	}
	// 移動ｺｰﾄﾞ
	CString	strGcode(GetValString(NCA_X, pt.x, FALSE) +
					 GetValString(NCA_Y, pt.y, FALSE));
	if ( !strGcode.IsEmpty() ) {
		if ( nCode != 0 )	// G00以外
			strGcode += GetFeedString(dFeed);
	}
	// 最終整形
	if ( !strTaper.IsEmpty() || !strGcode.IsEmpty() ) {
		m_strGcode = (*ms_pfnGetLineNo)() + strTaper;
		if ( !strGcode.IsEmpty() )
			m_strGcode += (*ms_pfnGetGString)(nCode) + strGcode;
		m_strGcode += ms_strEOB;
	}
}

// XY/UVのG[0|1]移動
CNCMakeWire::CNCMakeWire(const CPointF& ptxy, const CPointF& ptuv, float dFeed)
{
	CPointF	pt(ptuv - ptxy);
	CString	strGcode(GetValString(NCA_X, ptxy.x, FALSE) +
					 GetValString(NCA_Y, ptxy.y, FALSE) +
					 GetValString(NCA_U, ::RoundUp(pt.x), FALSE) +
					 GetValString(NCA_V, ::RoundUp(pt.y), FALSE));
	if ( !strGcode.IsEmpty() )
		m_strGcode = (*ms_pfnGetLineNo)() + (*ms_pfnGetGString)(1) +
					strGcode + GetFeedString(dFeed) + ms_strEOB;
}

// 任意の文字列ｺｰﾄﾞ
CNCMakeWire::CNCMakeWire(const CString& strGcode) : CNCMakeMill(strGcode)
{
}

//////////////////////////////////////////////////////////////////////
// ﾒﾝﾊﾞ関数

void CNCMakeWire::SetStaticOption(const CNCMakeWireOpt* pNCMake)
{
	extern	LPCTSTR	gg_szReturn;	// "\n"
	static	const	int		nLineMulti[] = {	// 行番号増加分
		1, 5, 10, 100
	};

	CNCMakeBase::SetStaticOption(pNCMake);

	// --- 軸指示
	NCAX = NCA_X;	NCAY = NCA_Y;
	NCAI = NCA_I;	NCAJ = NCA_J;
	// --- 送り指示
	switch ( GetNum(MKWI_NUM_FDOT) ) {
	case 0:		// 小数点表記
		ms_pfnGetFeed = GetFlg(MKWI_FLG_ZEROCUT) ?
			&GetValString_UZeroCut : &GetValString_Normal;
		break;
	case 1:		// 1/1000表記
		ms_pfnGetFeed = &GetValString_Multi1000;
		break;
	default:	// 整数表記
		ms_pfnGetFeed = &GetFeedString_Integer;
		break;
	}
	// --- 行番号付加
	ms_pfnGetLineNo = GetFlg(MKWI_FLG_LINEADD) && !(GetStr(MKWI_STR_LINEFORM).IsEmpty()) ?
		&GetLineNoString : &GetLineNoString_Clip;
	// --- Gｺｰﾄﾞﾓｰﾀﾞﾙ
	ms_pfnGetGString =  GetFlg(MKWI_FLG_GCLIP) ? &GetGString_Clip : &GetGString;
	// --- 座標表記
	ms_pfnGetValString = &GetValString;	// ﾍﾞｰｽｸﾗｽからの呼出用
	ms_pfnGetValDetail = GetNum(MKWI_NUM_DOT) == 0 ?
		(GetFlg(MKWI_FLG_ZEROCUT) ?
			&GetValString_UZeroCut : &GetValString_Normal) : &GetValString_Multi1000;
	// --- 行番号増加分
	ms_nMagni = GetNum(MKWI_NUM_LINEADD)<0 ||
					 GetNum(MKWI_NUM_LINEADD)>=SIZEOF(nLineMulti) ?
		nLineMulti[0] : nLineMulti[GetNum(MKWI_NUM_LINEADD)];
	// --- EOB
	ms_strEOB = GetStr(MKWI_STR_EOB).IsEmpty() ? 
		gg_szReturn : GetStr(MKWI_STR_EOB) + gg_szReturn;
	// --- 円ﾃﾞｰﾀの切削指示
	ms_nCircleCode = GetNum(MKWI_NUM_CIRCLECODE) == 0 ? 2 : 3;
	// --- 円,円弧ﾃﾞｰﾀの生成
	ms_pfnMakeCircle	= &MakeCircle_IJ;
	ms_pfnMakeCircleSub	= &MakeCircleSub_IJ;
	ms_pfnMakeHelical	= &MakeCircle_IJ_Helical;	// 保険(ﾜｲﾔ加工には無い)
	ms_pfnMakeArc		= &MakeArc_IJ;
	ms_bIJValue			= FALSE;
	// --- 楕円公差
	ms_dEllipse = GetDbl(MKWI_DBL_ELLIPSE);
}
