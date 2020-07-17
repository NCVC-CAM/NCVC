// NCMakeBase.cpp: CNCMakeBase クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NCVC.h"
#include "DXFdata.h"
#include "NCMakeOption.h"
#include "NCMakeMillOpt.h"
#include "NCMakeLatheOpt.h"
#include "NCMakeWireOpt.h"
#include "NCMakeBase.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

/////////////////////////////////////////////////////////////////////////////
// 静的変数の初期化
const	CNCMakeOption*	CNCMakeBase::ms_pMakeOpt = NULL;
double	CNCMakeBase::ms_xyz[] = {HUGE_VAL, HUGE_VAL, HUGE_VAL};
int		CNCMakeBase::NCAX = NCA_X;
int		CNCMakeBase::NCAY = NCA_Y;
int		CNCMakeBase::NCAI = NCA_I;
int		CNCMakeBase::NCAJ = NCA_J;
#undef	NCA_X	// 以降、NCA_X, NCA_Y は無効
#undef	NCA_Y	// NCA_I, NCA_J は、GetIJK() にのみ有効
int		CNCMakeBase::ms_nGcode = -1;
int		CNCMakeBase::ms_nSpindle = -1;
double	CNCMakeBase::ms_dFeed = -1.0;
int		CNCMakeBase::ms_nCnt = 1;
int		CNCMakeBase::ms_nMagni = 1;
int		CNCMakeBase::ms_nCircleCode = 2;
double	CNCMakeBase::ms_dEllipse = 0.5;
CString	CNCMakeBase::ms_strEOB;
PFNGETARGINT		CNCMakeBase::ms_pfnGetSpindle = &CNCMakeBase::GetSpindleString;
PFNGETARGDOUBLE		CNCMakeBase::ms_pfnGetFeed = &CNCMakeBase::GetFeedString_Integer;
PFNGETARGVOID		CNCMakeBase::ms_pfnGetLineNo = &CNCMakeBase::GetLineNoString;
PFNGETARGINT		CNCMakeBase::ms_pfnGetGString = &CNCMakeBase::GetGString;
PFNGETVALSTRING		CNCMakeBase::ms_pfnGetValString = NULL;		// 派生ｸﾗｽで決定
PFNGETARGDOUBLE		CNCMakeBase::ms_pfnGetValDetail = &CNCMakeBase::GetValString_Normal;
PFNMAKECIRCLESUB	CNCMakeBase::ms_pfnMakeCircleSub = &CNCMakeBase::MakeCircleSub_IJ;
PFNMAKECIRCLE		CNCMakeBase::ms_pfnMakeCircle = &CNCMakeBase::MakeCircle_IJ;
PFNMAKEHELICAL		CNCMakeBase::ms_pfnMakeHelical = &CNCMakeBase::MakeCircle_IJ_Helical;
PFNMAKEARC			CNCMakeBase::ms_pfnMakeArc = &CNCMakeBase::MakeArc_IJ;

//////////////////////////////////////////////////////////////////////
// ｺﾝｽﾄﾗｸﾀ

CNCMakeBase::CNCMakeBase()
{
}

// 任意の文字列ｺｰﾄﾞ
CNCMakeBase::CNCMakeBase(const CString& strGcode)
{
	extern	LPCTSTR	gg_szReturn;

	if ( strGcode.IsEmpty() )
		m_strGcode = gg_szReturn;
	else if ( strGcode[0] == '%' )
		m_strGcode = strGcode + gg_szReturn;
	else {
		CString	str(strGcode);
		str.Replace("\\n", "\n");	// 改行ｺｰﾄﾞの置換
		m_strGcode = (*ms_pfnGetLineNo)() + str + ms_strEOB;
	}
}

//////////////////////////////////////////////////////////////////////
// ﾒﾝﾊﾞ関数

void CNCMakeBase::InitialVariable(void)
{
	ms_nGcode = -1;
	ms_nSpindle = -1;
	ms_dFeed = -1.0;
	ms_nCnt = 1;
}

void CNCMakeBase::SetStaticOption(const CNCMakeOption* pNCMake)
{
	ms_pMakeOpt = pNCMake;
}

void CNCMakeBase::MakeEllipse(const CDXFellipse* pEllipse, double dFeed)
{
	CString	strGcode;
	BOOL	bFeed = TRUE;
	CPointD	pt, ptMake;
	double	sq, eq,
			// 角度のｽﾃｯﾌﾟ数を求める -> (sq-eq) / (r*(sq-eq) / STEP)
			dStep = ms_dEllipse / pEllipse->GetR();

	// 楕円時の開始終了角度を再計算
	// -> XY反転など傾きを考慮しないと正しい開始位置が保持できないため
	if ( pEllipse->IsArc() ) {
		sq = pEllipse->GetStartAngle();
		eq = pEllipse->GetEndAngle();
	}
	else {
		pt = pEllipse->GetStartCutterPoint() - pEllipse->GetMakeCenter();
		sq = atan2(pt.y, pt.x) - pEllipse->GetMakeLean();	// 傾きを吸収
		if ( pEllipse->GetRound() )
			eq = sq + RAD(360.0);
		else
			eq = sq - RAD(360.0);
	}

	// 生成開始
	if ( pEllipse->GetRound() ) {
		for ( sq+=dStep; sq<eq; sq+=dStep ) {
			strGcode = MakeEllipse_Tolerance(pEllipse, sq);
			if ( bFeed && !strGcode.IsEmpty() ) {
				strGcode += GetFeedString(dFeed);
				bFeed = FALSE;
			}
			if ( !strGcode.IsEmpty() )
				m_strGarray.Add((*ms_pfnGetLineNo)() + strGcode + ms_strEOB);
		}
	}
	else {
		for ( sq-=dStep; sq>eq; sq-=dStep ) {
			strGcode = MakeEllipse_Tolerance(pEllipse, sq);
			if ( bFeed && !strGcode.IsEmpty() ) {
				strGcode += GetFeedString(dFeed);
				bFeed = FALSE;
			}
			if ( !strGcode.IsEmpty() )
				m_strGarray.Add((*ms_pfnGetLineNo)() + strGcode + ms_strEOB);
		}
	}
	strGcode = MakeEllipse_Tolerance(pEllipse, eq);
	if ( bFeed && !strGcode.IsEmpty() )
		strGcode += GetFeedString(dFeed);
	if ( !strGcode.IsEmpty() )
		m_strGarray.Add((*ms_pfnGetLineNo)() + strGcode + ms_strEOB);
}

CString	CNCMakeBase::MakeEllipse_Tolerance(const CDXFellipse* pEllipse, double q)
{
	CPointD	pt    ( pEllipse->GetLongLength()  * cos(q),
					pEllipse->GetShortLength() * sin(q) );
	CPointD	ptMake( pt.x * pEllipse->GetMakeLeanCos() - pt.y * pEllipse->GetMakeLeanSin(),
					pt.x * pEllipse->GetMakeLeanSin() + pt.y * pEllipse->GetMakeLeanCos() );
	ptMake += pEllipse->GetMakeCenter();
	pt = ptMake.RoundUp();
	return CString( (*ms_pfnGetGString)(1) +
		(*ms_pfnGetValString)(NCAX, pt.x, FALSE) + (*ms_pfnGetValString)(NCAY, pt.y, FALSE) );
}

void CNCMakeBase::MakePolylineCut(const CDXFpolyline* pPoly, double dFeed)
{
	if ( pPoly->GetVertexCount() <= 1 )
		return;

	BOOL	bFeed = TRUE;
	CString	strGcode;
	CDXFdata*	pData;

	// SwapMakePt()で順序が入れ替わっても端点は必ず CDXFpoint
	POSITION pos = pPoly->GetFirstVertex();
	pPoly->GetNextVertex(pos);
	// ２点目からﾙｰﾌﾟ
	while ( pos ) {
		pData = pPoly->GetNextVertex(pos);
		switch ( pData->GetMakeType() ) {
		case DXFPOINTDATA:
			strGcode = (*ms_pfnGetGString)(1) +
					(*ms_pfnGetValString)(NCAX, pData->GetEndMakePoint().x, FALSE) +
					(*ms_pfnGetValString)(NCAY, pData->GetEndMakePoint().y, FALSE);
			// 最初だけ送り速度追加
			if ( bFeed && !strGcode.IsEmpty() ) {
				strGcode += GetFeedString(dFeed);
				bFeed = FALSE;
			}
			if ( !strGcode.IsEmpty() )
				m_strGarray.Add((*ms_pfnGetLineNo)() + strGcode + ms_strEOB);
			break;

		case DXFARCDATA:
			strGcode = (*ms_pfnMakeArc)(static_cast<CDXFarc*>(pData), dFeed);
			if ( !strGcode.IsEmpty() ) {
				m_strGarray.Add((*ms_pfnGetLineNo)() + strGcode);
				bFeed = FALSE;
			}
			// 終点分を飛ばす
			pPoly->GetNextVertex(pos);
			break;

		case DXFELLIPSEDATA:
			MakeEllipse(static_cast<CDXFellipse*>(pData), dFeed);
			bFeed = FALSE;
			// 終点分を飛ばす
			pPoly->GetNextVertex(pos);
			break;
		}
	}	// End of loop
}

CString CNCMakeBase::MakeCustomString
	(int nCode, DWORD dwValFlags/*=0*/, double* dValue/*=NULL*/, BOOL bReflect/*=TRUE*/)
{
	extern	LPCTSTR	g_szNdelimiter;		// "XYZUVWIJKRPLDH" from NCDoc.cpp
	extern	const	DWORD	g_dwSetValFlags[];

	// 座標値を前回値に反映させない場合(G92など)は直接 ms_pfnGetValDetail を呼び出す
	CString	strResult;
	if ( nCode >= 0 )
		strResult = bReflect ? (*ms_pfnGetGString)(nCode) : GetGString(nCode);
	if ( dValue ) {
		for ( int i=0; i<VALUESIZE; i++ ) {
			if ( dwValFlags & g_dwSetValFlags[i] )
				strResult += bReflect || i>=GVALSIZE ?	// i>=NCA_P
								(*ms_pfnGetValString)(i, dValue[i], FALSE) :
								(g_szNdelimiter[i]+(*ms_pfnGetValDetail)(dValue[i]));
		}
	}
	return strResult;
}

CString CNCMakeBase::MakeCustomString(int nCode, int nValFlag[], double dValue[])
{
	int		n;
	// nValFlagに指定された順に値を追加
	CString	strResult( (*ms_pfnGetGString)(nCode) );
	for ( int i=0; i<VALUESIZE && nValFlag[i]>0; i++ ) {
		n = nValFlag[i];
		strResult += (*ms_pfnGetValString)(n, dValue[n], FALSE);
	}
	return strResult;
}

//////////////////////////////////////////////////////////////////////

// 回転指示
CString	CNCMakeBase::GetSpindleString(int nSpindle)
{
	CString	strResult;
	if ( ms_nSpindle != nSpindle ) {
		strResult.Format("S%d", nSpindle);
		ms_nSpindle = nSpindle;
	}
	return strResult;
}

CString	CNCMakeBase::GetSpindleString_Clip(int)
{
	return CString();
}

// 送り速度
CString	CNCMakeBase::GetFeedString(double dFeed)
{
	CString		strResult;
	if ( dFeed!=0.0 && ms_dFeed!=dFeed ) {
		ms_dFeed = dFeed;
		strResult = "F" + (*ms_pfnGetFeed)(dFeed);
	}
	return strResult;
}

CString	CNCMakeBase::GetFeedString_Integer(double dFeed)
{
	CString	strResult;
	// 	GetFeedString()からの参照のため if() 不要
	strResult.Format("%d", (int)dFeed);
	return strResult;
}

// 行番号付加
CString	CNCMakeBase::GetLineNoString(void)
{
	ASSERT( MKNC_STR_LINEFORM == MKLA_STR_LINEFORM );	// 苦肉の策
	ASSERT( MKNC_STR_LINEFORM == MKWI_STR_LINEFORM );

	CString	strResult;
	strResult.Format(ms_pMakeOpt->GetStr(MKNC_STR_LINEFORM), ms_nCnt++ * ms_nMagni);
	return strResult;
}

CString	CNCMakeBase::GetLineNoString_Clip(void)
{
	return CString();
}

// Gｺｰﾄﾞﾓｰﾀﾞﾙ
CString	CNCMakeBase::GetGString(int nCode)
{
	CString		strResult;
	strResult.Format("G%02d", nCode);
	return strResult;
}

CString	CNCMakeBase::GetGString_Clip(int nCode)
{
	CString		strResult;
	if ( ms_nGcode != nCode ) {
		strResult = GetGString(nCode);
		ms_nGcode = nCode;
	}
	return strResult;
}

// 座標値設定
CString	CNCMakeBase::GetValString_Normal(double dVal)
{
	CString		strResult;
	strResult.Format(IDS_MAKENCD_FORMAT, dVal);
	return strResult;
}

CString	CNCMakeBase::GetValString_UZeroCut(double dVal)
{
	CString		strResult;
	if ( dVal == 0.0 ) {
		strResult = "0";
		return strResult;
	}
	int			nCnt;
	strResult = GetValString_Normal(dVal);
	for ( nCnt=strResult.GetLength(); nCnt>0 ; nCnt-- ) {
		if ( strResult[nCnt-1]=='.' || strResult[nCnt-1]!='0' )
			break;
	}
	return	strResult.Left(nCnt);
}

CString	CNCMakeBase::GetValString_Multi1000(double dVal)
{
	CString		strResult;
/*	--- RoundUp() にて処理済みのため不要
	dVal *= 1000.0;
	dVal  = (dVal<0 ? floor(dVal) : ceil(dVal));
	strResult.Format("%ld", (long)dVal);
*/
	// 単純な1000倍では，丸め誤差が発生(??)
	strResult.Format("%d", (int)(dVal*1000.0+_copysign(0.0001, dVal)));
	return strResult;
}

// 円・円弧の生成補助
CString	CNCMakeBase::MakeCircleSub_R(int nCode, const CPointD& pt, const CPointD&, double r)
{
	return CString( (*ms_pfnGetGString)(nCode) +
		(*ms_pfnGetValString)(NCAX, pt.x, FALSE) +
		(*ms_pfnGetValString)(NCAY, pt.y, FALSE) +
		(*ms_pfnGetValString)(NCA_R, r,    FALSE) );
}

CString	CNCMakeBase::MakeCircleSub_IJ(int nCode, const CPointD& pt, const CPointD& ptij, double)
{
	return CString( (*ms_pfnGetGString)(nCode) +
		(*ms_pfnGetValString)(NCAX, pt.x,   FALSE) +
		(*ms_pfnGetValString)(NCAY, pt.y,   FALSE) +
		(*ms_pfnGetValString)(NCAI, ptij.x, FALSE) +
		(*ms_pfnGetValString)(NCAJ, ptij.y, FALSE) );
}

CString	CNCMakeBase::MakeCircleSub_Helical(int nCode, const CPoint3D& pt)
{
	return CString( (*ms_pfnGetLineNo)() + (*ms_pfnGetGString)(nCode) +
		(*ms_pfnGetValString)(NCAX,  pt.x, FALSE) +
		(*ms_pfnGetValString)(NCAY,  pt.y, FALSE) +
		(*ms_pfnGetValString)(NCA_Z, pt.z, FALSE) );	// ﾍﾘｶﾙはMILLのみ
}

// 円ﾃﾞｰﾀの生成
CString	CNCMakeBase::MakeCircle_R(const CDXFcircle* pCircle, double dFeed)
{
	int		a = pCircle->GetBaseAxis(), b = a & 0x01 ? (a-1) : (a+1),
			nCode = pCircle->IsRoundFixed() ? pCircle->GetG() : ms_nCircleCode;
	double	r = pCircle->GetMakeR();
	CPointD	pt;		// dummy
	// R指定，180゜ずつ分けて生成
	CString	strGcode;
	CString	strBuf1( MakeCircleSub_R(nCode, pCircle->GetMakePoint(b), pt, r) );
	CString	strBuf2( MakeCircleSub_R(nCode, pCircle->GetMakePoint(a), pt, r) );
	if ( !strBuf1.IsEmpty() && !strBuf2.IsEmpty() )
		strGcode = (*ms_pfnGetLineNo)() + strBuf1 + GetFeedString(dFeed) + ms_strEOB +
						(*ms_pfnGetLineNo)() + strBuf2 + ms_strEOB;
	return strGcode;
}

CString	CNCMakeBase::MakeCircle_IJ(const CDXFcircle* pCircle, double dFeed)
{
	CString	strGcode, strBuf( pCircle->GetBaseAxis() > 1 ?	// X軸かY軸か
						(*ms_pfnGetValString)(NCAJ, pCircle->GetIJK(NCA_J), FALSE) :
						(*ms_pfnGetValString)(NCAI, pCircle->GetIJK(NCA_I), FALSE) ); 
	int		nCode = pCircle->IsRoundFixed() ? pCircle->GetG() : ms_nCircleCode;
	if ( !strBuf.IsEmpty() )
		strGcode = (*ms_pfnGetLineNo)() + (*ms_pfnGetGString)(nCode) +
					strBuf + GetFeedString(dFeed) + ms_strEOB;
	return strGcode;
}

CString	CNCMakeBase::MakeCircle_IJHALF(const CDXFcircle* pCircle, double dFeed)
{
	int		a = pCircle->GetBaseAxis(), b = a & 0x01 ? (a-1) : (a+1),
			nCode = pCircle->IsRoundFixed() ? pCircle->GetG() : ms_nCircleCode;
	CPointD	ij;
	// 基準軸の決定
	if ( a > 1 ) 	// Y軸ﾍﾞｰｽ
		ij.y = pCircle->GetIJK(NCA_J);
	else 			// X軸ﾍﾞｰｽ
		ij.x = pCircle->GetIJK(NCA_I);
	CString	strGcode;
	CString	strBuf1( MakeCircleSub_IJ(nCode, pCircle->GetMakePoint(b), ij, 0.0) );
	ij *= -1.0;		// ij = -ij;
	CString	strBuf2( MakeCircleSub_IJ(nCode, pCircle->GetMakePoint(a), ij, 0.0) );
	if ( !strBuf1.IsEmpty() && !strBuf2.IsEmpty() )
		strGcode = (*ms_pfnGetLineNo)() + strBuf1 + GetFeedString(dFeed) + ms_strEOB +
						(*ms_pfnGetLineNo)() + strBuf2 + ms_strEOB;
	return strGcode;
}

CString	CNCMakeBase::MakeCircle_R_Helical(const CDXFcircle* pCircle, double dFeed, double dHelical)
{
	int		a = pCircle->GetBaseAxis(), b = a & 0x01 ? (a-1) : (a+1),
			nCode = pCircle->IsRoundFixed() ? pCircle->GetG() : ms_nCircleCode;
	double	r = pCircle->GetMakeR(),
			s = ms_pMakeOpt->GetDbl(MKNC_DBL_ZSTEP),	// ﾍﾘｶﾙはMILLのみ
			z = dHelical - s + s/2.0;
	CPoint3D	pt1(pCircle->GetMakePoint(b).x, pCircle->GetMakePoint(b).y, z),
				pt2(pCircle->GetMakePoint(a).x, pCircle->GetMakePoint(a).y, dHelical);
	CString	strGcode, strBuf( (*ms_pfnGetValString)(NCA_R, r, FALSE) );
	if ( !strBuf.IsEmpty() ) {
		// 計算順序の関係で１行にできない
		strGcode  = MakeCircleSub_Helical(nCode, pt1) + strBuf + GetFeedString(dFeed) + ms_strEOB;
		strGcode += MakeCircleSub_Helical(nCode, pt2) + strBuf + ms_strEOB;
	}
	return strGcode;
}

CString	CNCMakeBase::MakeCircle_IJ_Helical(const CDXFcircle* pCircle, double dFeed, double dHelical)
{
	CString	strGcode, strBuf( pCircle->GetBaseAxis() > 1 ?
						(*ms_pfnGetValString)(NCAJ, pCircle->GetIJK(NCA_J), FALSE) :
						(*ms_pfnGetValString)(NCAI, pCircle->GetIJK(NCA_I), FALSE) ); 
	int		nCode = pCircle->IsRoundFixed() ? pCircle->GetG() : ms_nCircleCode;
	if ( !strBuf.IsEmpty() )
		strGcode = (*ms_pfnGetLineNo)() + (*ms_pfnGetGString)(nCode) +
					(*ms_pfnGetValString)(NCA_Z, dHelical, FALSE) +
					strBuf + GetFeedString(dFeed) + ms_strEOB;
	return strGcode;
}

CString	CNCMakeBase::MakeCircle_IJHALF_Helical(const CDXFcircle* pCircle, double dFeed, double dHelical)
{
	int		a = pCircle->GetBaseAxis(), b = a & 0x01 ? (a-1) : (a+1),
			nCode = pCircle->IsRoundFixed() ? pCircle->GetG() : ms_nCircleCode;
	double	s = ms_pMakeOpt->GetDbl(MKNC_DBL_ZSTEP),	// ﾍﾘｶﾙはMILLのみ
			z = dHelical - s + s/2.0;
	CPointD		ij;
	CPoint3D	pt1(pCircle->GetMakePoint(b).x, pCircle->GetMakePoint(b).y, z),
				pt2(pCircle->GetMakePoint(a).x, pCircle->GetMakePoint(a).y, dHelical);
	if ( a > 1 )
		ij.y = pCircle->GetIJK(NCA_J);
	else
		ij.x = pCircle->GetIJK(NCA_I);
	CString	strGcode;
	CString	strBuf1( (*ms_pfnGetValString)(NCAI, ij.x, FALSE) + (*ms_pfnGetValString)(NCAJ, ij.y, FALSE) );
	ij *= -1.0;		// ij = -ij;
	CString	strBuf2( (*ms_pfnGetValString)(NCAI, ij.x, FALSE) + (*ms_pfnGetValString)(NCAJ, ij.y, FALSE) );
	if ( !strBuf1.IsEmpty() && !strBuf2.IsEmpty() ) {
		// 計算順序の関係で１行にできない
		strGcode  = MakeCircleSub_Helical(nCode, pt1) + strBuf1 + GetFeedString(dFeed) + ms_strEOB;
		strGcode += MakeCircleSub_Helical(nCode, pt2) + strBuf2 + ms_strEOB;
	}
	return strGcode;
}

// 円弧ﾃﾞｰﾀの生成
CString	CNCMakeBase::MakeArc_R(const CDXFarc* pArc, double dFeed)
{
	CString	strGcode,
			strBuf( MakeCircleSub_R(pArc->GetG(), pArc->GetEndMakePoint(), CPointD(), pArc->GetMakeR()) );
	if ( !strBuf.IsEmpty() )
		strGcode = (*ms_pfnGetLineNo)() + strBuf + GetFeedString(dFeed) + ms_strEOB;
	return strGcode;
}

CString	CNCMakeBase::MakeArc_IJ(const CDXFarc* pArc, double dFeed)
{
	CPointD	ij(pArc->GetIJK(NCA_I), pArc->GetIJK(NCA_J));
	CString	strGcode,
			strBuf( MakeCircleSub_IJ(pArc->GetG(), pArc->GetEndMakePoint(), ij, 0.0) );
	if ( !strBuf.IsEmpty() )
		strGcode = (*ms_pfnGetLineNo)() + strBuf + GetFeedString(dFeed) + ms_strEOB;
	return strGcode;
}

//////////////////////////////////////////////////////////////////////

// Gｺｰﾄﾞ出力
void CNCMakeBase::WriteGcode(CStdioFile& fp)
{
	if ( !m_strGcode.IsEmpty() )
		fp.WriteString( m_strGcode );
	for ( int i=0; i<m_strGarray.GetSize(); i++ )
		fp.WriteString( m_strGarray[i] );
}
