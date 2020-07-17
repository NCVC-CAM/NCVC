// 3Dto2D.cpp
// ３次元変換のｸﾞﾛｰﾊﾞﾙ関数
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "3Dto2D.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

using namespace boost;

//////////////////////////////////////////////////////////////////////
// 静的変数の初期化

//double	CPoint3D::ms_rx_cos = 0.0;
//double	CPoint3D::ms_rx_sin = 0.0;
//double	CPoint3D::ms_ry_cos = 0.0;
//double	CPoint3D::ms_ry_sin = 0.0;
//double	CPoint3D::ms_rz_cos = 0.0;
//double	CPoint3D::ms_rz_sin = 0.0;
float	CPoint3F::ms_rx_cos = 0.0f;
float	CPoint3F::ms_rx_sin = 0.0f;
float	CPoint3F::ms_ry_cos = 0.0f;
float	CPoint3F::ms_ry_sin = 0.0f;
float	CPoint3F::ms_rz_cos = 0.0f;
float	CPoint3F::ms_rz_sin = 0.0f;

//////////////////////////////////////////////////////////////////////
//	２線の交点を求める

optional<CPointF> CalcIntersectionPoint_LL
	(const CPointF& pts1, const CPointF& pte1, const CPointF& pts2, const CPointF& pte2,
		BOOL bRangeChk/*=TRUE*/)
{
	BOOL	bResult = FALSE;
	CPointF	pt;
	optional<CPointF>	pt1, pt2;
	float	minX1, minY1, maxX1, maxY1,
			minX2, minY2, maxX2, maxY2,
			xa = RoundUp(pte1.x - pts1.x),	ya = RoundUp(pte1.y - pts1.y),
			xb = RoundUp(pte2.x - pts2.x),	yb = RoundUp(pte2.y - pts2.y);

	// 計算前準備
	tie(minX1, maxX1) = minmax(pts1.x, pte1.x);		// boost/algorithm
	tie(minY1, maxY1) = minmax(pts1.y, pte1.y);
	tie(minX2, maxX2) = minmax(pts2.x, pte2.x);
	tie(minY2, maxY2) = minmax(pts2.y, pte2.y);

	// 交点計算
	if ( fabs(xa) < NCMIN ) {
		if ( fabs(xb) >= NCMIN ) {
			pt.x = pts1.x;
			pt.y = yb * (pts1.x - pts2.x) / xb + pts2.y;
			bResult = TRUE;
		}
		else if ( fabs(pts1.x-pts2.x) < NCMIN ) {
			if ( bRangeChk ) {
				if ( minY2<=pts1.y && pts1.y<=maxY2 )
					pt1 = pts1;
				else if ( minY2<=pte1.y && pte1.y<=maxY2 )
					pt1 = pte1;
				if ( minY1<=pts2.y && pts2.y<=maxY1 )
					pt2 = pts2;
				else if ( minY1<=pte2.y && pte2.y<=maxY1 )
					pt2 = pte2;
			}
			else {
				pt1 = pt2 = pt;		// ﾀﾞﾐｰで有効
				if ( maxY1 >= maxY2 ) {
					(*pt1).y = minY1;
					(*pt2).y = maxY2;
				}
				else {
					(*pt1).y = minY2;
					(*pt2).y = maxY1;
				}
			}
			if ( pt1 && pt2 ) {
				// 範囲内にある中点を交点とする
				ya = (*pt1).y;	yb = (*pt2).y;
				pt.x = pts1.x;
				pt.y = ( yb - ya ) / 2.0f + ya;
				return pt;	// 検算の必要なし
			}
			else {
				// 端点が近い場合など、なるべく答えを返す
				// RoundCt() を使用し <= NCMIN で条件緩和
				if ( RoundCt(fabs(pts2.y-pts1.y))<=NCMIN || RoundCt(fabs(pte2.y-pts1.y))<=NCMIN )
					return pts1;
				if ( RoundCt(fabs(pts2.y-pte1.y))<=NCMIN || RoundCt(fabs(pte2.y-pte1.y))<=NCMIN )
					return pte1;
			}
		}
		// 上記条件以外で両方とも垂直線では解なし
	}
	else {
		if ( fabs(xb) < NCMIN ) {
			pt.x = pts2.x;
			pt.y = ya * (pts2.x - pts1.x) / xa + pts1.y;
			bResult = TRUE;
		}
		else {
			float	yaxa = RoundUp(ya / xa),
					ybxb = RoundUp(yb / xb);
			if ( fabs(ya) < NCMIN ) {
				if ( fabs(yb) >= NCMIN ) {
					pt.x = (pts1.y - pts2.y) / ybxb + pts2.x;
					pt.y = pts1.y;
					bResult = TRUE;
				}
				else if ( fabs(pts1.y-pts2.y) < NCMIN ) {
					if ( bRangeChk ) {
						if ( minX2<=pts1.x && pts1.x<=maxX2 )
							pt1 = pts1;
						else if ( minX2<=pte1.x && pte1.x<=maxX2 )
							pt1 = pte1;
						if ( minX1<=pts2.x && pts2.x<=maxX1 )
							pt2 = pts2;
						else if ( minX1<=pte2.x && pte2.x<=maxX1 )
							pt2 = pte2;
					}
					else {
						pt1 = pt2 = pt;
						if ( maxX1 >= maxX2 ) {
							(*pt1).x = minX1;
							(*pt2).x = maxX2;
						}
						else {
							(*pt1).x = minX2;
							(*pt2).x = maxX1;
						}
					}
					if ( pt1 && pt2 ) {
						xa = (*pt1).x;	xb = (*pt2).x;
						pt.x = ( xb - xa ) / 2.0f + xa;
						pt.y = pts1.y;
						return pt;
					}
					else {
						if ( RoundCt(fabs(pts2.x-pts1.x))<=NCMIN || RoundCt(fabs(pte2.x-pts1.x))<=NCMIN )
							return pts1;
						if ( RoundCt(fabs(pts2.x-pte1.x))<=NCMIN || RoundCt(fabs(pte2.x-pte1.x))<=NCMIN )
							return pte1;
					}
				}
				// 上記条件以外で両方とも水平線では解なし
			}
			else {
				if ( fabs(yb) < NCMIN ) {
					pt.x = (pts2.y - pts1.y) / yaxa + pts1.x;
					pt.y = pts2.y;
					bResult = TRUE;
				}
				else {
					if ( fabs(yaxa-ybxb) < NCMIN ) {
						// 水平・垂直以外で傾きが同じ(ｾﾞﾛ除算防止)とき
						if ( bRangeChk ) {
							// 相手の座標範囲内にあるかどうか
							if ( minX2<=pts1.x && pts1.x<=maxX2 && minY2<=pts1.y && pts1.y<=maxY2 )
								pt1 = pts1;
							else if ( minX2<=pte1.x && pte1.x<=maxX2 && minY2<=pte1.y && pte1.y<=maxY2 )
								pt1 = pte1;
							if ( minX1<=pts2.x && pts2.x<=maxX1 && minY1<=pts2.y && pts2.y<=maxY1 )
								pt2 = pts2;
							else if ( minX1<=pte2.x && pte2.x<=maxX1 && minY1<=pte2.y && pte2.y<=maxY1 )
								pt2 = pte2;
						}
						else {
							pt1 = pt2 = pt;
							if ( maxX1 >= maxX2 ) {
								(*pt1).x = minX1;
								(*pt2).x = maxX2;
							}
							else {
								(*pt1).x = minX2;
								(*pt2).x = maxX1;
							}
							if ( maxY1 >= maxY2 ) {
								(*pt1).y = minY1;
								(*pt2).y = maxY2;
							}
							else {
								(*pt1).y = minY2;
								(*pt2).y = maxY1;
							}
						}
						if ( pt1 && pt2 ) {
							// 元の直線と傾きが同じかをﾁｪｯｸしてから
							xa = RoundUp((*pt2).x - (*pt1).x);
							ya = RoundUp((*pt2).y - (*pt1).y);
							if ( fabs(xa)>=NCMIN && RoundCt(fabs(ya/xa-yaxa))<=NCMIN ) {
								xa = (*pt1).x;	xb = (*pt2).x;
								ya = (*pt1).y;	yb = (*pt2).y;
								pt.x = ( xb - xa ) / 2.0f + xa;
								pt.y = ( yb - ya ) / 2.0f + ya;
								return pt;
							}
						}
						// else にすると「傾きが同じﾁｪｯｸ」の偽が引っかからない
						if ( RoundCt(sqrt(GAPCALC(pts2-pts1)))<=NCMIN || RoundCt(sqrt(GAPCALC(pte2-pts1)))<=NCMIN )
							return pts1;
						if ( RoundCt(sqrt(GAPCALC(pts2-pte1)))<=NCMIN || RoundCt(sqrt(GAPCALC(pte2-pte1)))<=NCMIN )
							return pte1;
					}
					else {
						pt.x = (pts2.y - pts1.y + pts1.x*yaxa - pts2.x*ybxb) / (yaxa - ybxb);
						pt.y = yaxa * (pt.x - pts1.x) + pts1.y;
						bResult = TRUE;
					}
				}
			}
		}
	}

	// 検算(範囲ﾁｪｯｸ)
	if ( bRangeChk ) {
		// 線１
		if ( bResult ) {
			if ( fabs(ya) < NCMIN ) {
				// 水平線では x の範囲ﾁｪｯｸのみ
				if ( pt.x<minX1 || maxX1<pt.x )
					bResult = FALSE;
			}
			else if ( fabs(xa) < NCMIN ) {
				// 垂直線では y の範囲ﾁｪｯｸのみ
				if ( pt.y<minY1 || maxY1<pt.y )
					bResult = FALSE;
			}
			else {
				if ( pt.x<minX1 || maxX1<pt.x || pt.y<minY1 || maxY1<pt.y )
					bResult = FALSE;
			}
		}
		// 線２	
		if ( bResult ) {
			if ( fabs(yb) < NCMIN ) {
				if ( pt.x<minX2 || maxX2<pt.x )
					bResult = FALSE;
			}
			else if ( fabs(xb) < NCMIN ) {
				if ( pt.y<minY2 || maxY2<pt.y )
					bResult = FALSE;
			}
			else {
				if ( pt.x<minX2 || maxX2<pt.x || pt.y<minY2 || maxY2<pt.y )
					bResult = FALSE;
			}
		}
	}

	return bResult ? pt : optional<CPointF>();
}

//////////////////////////////////////////////////////////////////////
//	直線と円の交点を求める

tuple<int, CPointF, CPointF> CalcIntersectionPoint_LC
	(const CPointF& pts, const CPointF& pte, const CPointF& ptc, float r,
		BOOL bRangeChk/*=TRUE*/)
{
	r = fabs(r);

	int		nResult = 0;
	CPointF	pr1, pr2,
			pt(pte-pts), pto(ptc-pts);	// 始点を中心に
	float	q = atan2(pt.y, pt.x);		// 線の傾き
	pto.RoundPoint(-q);					// 円の中心を線の傾き分逆回転
	float	y  = fabs(pto.y),			// 円の中心とX軸(直線)との距離
			yr = RoundUp(y-r);

	// 交点がない条件
	if ( yr > NCMIN )
		return make_tuple(nResult, pr1, pr2);

	pt.RoundPoint(-q);	// ptが必ず正方向に

	if ( fabs(yr) <= NCMIN ) {
		// 接する
		pr1.x = pto.x;
		// 範囲ﾁｪｯｸ
		nResult = ( bRangeChk && (pr1.x<0 || pt.x<pr1.x) ) ? 0 : 1;
		// 回転を元に戻す
		pr1.RoundPoint(q);
		pr2 = pr1;		// 接するﾏｰｸ
	}
	else {
		// ２交点
		nResult = 2;
		r = sqrt(r*r - y*y);
		pr1.x = pto.x + r;
		pr2.x = pto.x - r;
		// 範囲ﾁｪｯｸ
		if ( bRangeChk ) {
			if ( pr1.x<0 || pt.x<pr1.x )
				nResult--;
			if ( pr2.x<0 || pt.x<pr2.x )
				nResult--;
			else if ( nResult == 1 )
				swap(pr1, pr2);	// pr1の解を採用しない(代入では接線と区別付かない)
		}
		// 回転を元に戻す
		pr1.RoundPoint(q);
		pr2.RoundPoint(q);
	}

	// 平行移動
	pr1 += pts;
	pr2 += pts;

	return make_tuple(nResult, pr1, pr2);
}

//////////////////////////////////////////////////////////////////////
//	２つの円の交点を求める

tuple<int, CPointF, CPointF> CalcIntersectionPoint_CC
	(const CPointF& pts, const CPointF& pte, float r1, float r2)
{
	r1 = fabs(r1);
	r2 = fabs(r2);

	int		nResult = 0;
	CPointF	pr1, pr2,
			pt(pte-pts);			// 始点Ｓが原点となるよう平行移動
	float	l,
			r1r2p = r1 + r2,		// 使用頻度の高い計算
			r1r2n = fabs(r1 - r2),
			q = atan2(pt.y, pt.x);	// 回転角度

	// 他方(pte)の中心座標がx軸上になるよう逆回転
	pt.RoundPoint(-q);
	l = RoundUp(fabs(pt.x));	// ２円の中心の距離

	// 交点がない条件(同心円{距離ｾﾞﾛ}, ２円の半径より距離が大きい, 小さい
	if ( l < NCMIN || l > RoundUp(r1r2p)+NCMIN || l < RoundUp(r1r2n)-NCMIN )
		return make_tuple(nResult, pr1, pr2);

	// 交点計算
	if ( RoundCt(fabs(l-r1r2p)) <= NCMIN || RoundCt(fabs(l-r1r2n)) <= NCMIN ) {
		// 接する条件(y=0)とﾏｲﾅｽ条件
		pr1.x = ( r1 < r2 && l-r2 < 0 ) ? -r1 : r1;
		// 回転を復元
		pr1.RoundPoint(q);
		pr2 = pr1;
		// 解の数
		nResult = 1;
	}
	else {
		// 交点２つ
		r1 *= r1;	// r1*r1を２回使うので先に計算
		pr1.x = pr2.x = (r1 - r2*r2 + l*l) / (2.0f*l);
		pr1.y = sqrt(r1 - pr1.x*pr1.x);
		pr2.y = -pr1.y;
		// 回転を復元
		pr1.RoundPoint(q);
		pr2.RoundPoint(q);
		// 解の数
		nResult = 2;
	}

	// 平行移動復元
	pr1 += pts;
	pr2 += pts;

	return make_tuple(nResult, pr1, pr2);
}

//////////////////////////////////////////////////////////////////////
//	直線と楕円の交点を求める

tuple<int, CPointF, CPointF> CalcIntersectionPoint_LE
	(const CPointF& pts, const CPointF& pte,
		const CPointF& pto, float a, float b, float q,
		BOOL bRangeChk/*=TRUE*/)
{
	int		nResult = 0;
	CPointF	pt1(pts-pto), pt2(pte-pto), pr1, pr2;
	// 楕円の傾きを相殺
	pt1.RoundPoint(-q);
	pt2.RoundPoint(-q);
	// 計算前準備
	float	xa = pt2.x - pt1.x,		ya = pt2.y - pt1.y,
			minX, minY, maxX, maxY;
	tie(minX, maxX) = minmax(pt1.x, pt2.x);
	tie(minY, maxY) = minmax(pt1.y, pt2.y);

	// 交点計算
	if ( fabs(xa) < NCMIN ) {
		// 垂直線
		if ( fabs(ya) < NCMIN )
			return make_tuple(nResult, pr1, pr2);
		pr1.x = pr2.x = pt1.x;
		pr1.y = b * sqrt(1 - (pt1.x*pt1.x)/(a*a));
		pr2.y = -pr1.y;
		nResult = 2;
		if ( bRangeChk ) {
			if ( pr1.y<minY || maxY<pr1.y )
				nResult--;
			if ( pr2.y<minY || maxY<pr2.y )
				nResult--;
			else if ( nResult == 1 )	// pr1がNGでpr2がOK
				swap(pr1, pr2);
		}
	}
	else if ( fabs(ya) < NCMIN ) {
		// 水平線
		pr1.y = pr2.y = pt1.y;
		pr1.x = a * sqrt(1 - (pt1.y*pt1.y)/(b*b));
		pr2.x = -pr1.x;
		nResult = 2;
		if ( bRangeChk ) {
			if ( pr1.x<minX || maxX<pr1.x )
				nResult--;
			if ( pr2.x<minX || maxX<pr2.x )
				nResult--;
			else if ( nResult == 1 )
				swap(pr1, pr2);
		}
	}
	else {
		// ２次方程式からx値を計算
		tie(nResult, pr1.x, pr2.x) = GetKon(
				ya*ya + xa*xa*b*b/(a*a),
				2.0f*ya*(xa*pt1.y - pt1.x*ya),
				(pt1.y*pt1.y-b*b)*xa*xa + (pt1.x*ya-2*pt1.y*xa)*pt1.x*ya );
		// y値を計算
		if ( nResult > 1 )
			pr2.y = ya*(pr2.x-pt1.x)/xa + pt1.y;
		if ( nResult > 0 )
			pr1.y = ya*(pr1.x-pt1.x)/xa + pt1.y;
		else
			return make_tuple(nResult, pr1, pr2);
		if ( bRangeChk ) {
			if ( nResult > 1 ) {
				if ( pr2.x<minX || maxX<pr2.x || pr2.y<minY || pr2.y<maxY )
					nResult--;
			}
			if ( nResult > 0 ) {
				if ( pr1.x<minX || maxX<pr1.x || pr1.y<minY || pr1.y<maxY ) {
					nResult--;
					if ( nResult > 0 )
						swap(pr1, pr2);
				}
			}
		}
	}

	if ( nResult > 0 ) {
		pr1.RoundPoint(q);
		pr2.RoundPoint(q);
		pr1 += pto;
		pr2 += pto;
	}

	return make_tuple(nResult, pr1, pr2);
}

//////////////////////////////////////////////////////////////////////
//	線と線の端点を中心とした円との交点を求める

CPointF	CalcIntersectionPoint_TC
	(const CPointF& ptOrg, float r, const CPointF& ptSrc)
{
	CPointF	pt;

	// ptOrg を中心とした円と ptSrc の交点
	if ( fabs( ptSrc.x - ptOrg.x ) < NCMIN ) {
		pt.x = ptOrg.x;
		pt.y = ptOrg.y + copysign(r, ptSrc.y - ptOrg.y);
	}
	else if ( fabs( ptSrc.y - ptOrg.y ) < NCMIN ) {
		pt.x = ptOrg.x + copysign(r, ptSrc.x - ptOrg.x);
		pt.y = ptOrg.y;
	}
	else {
		float a = (ptSrc.y - ptOrg.y) / (ptSrc.x - ptOrg.x);
		pt.x = r * sqrt( 1 / (1+a*a) );
		pt.y = (ptOrg.x < ptSrc.x ? a : -a) * pt.x;
		pt.x = pt.y / a + ptOrg.x;	// xを再計算(yとaの符号を考慮)
		pt.y += ptOrg.y;			// 原点補正
	}

	return pt;
}

//////////////////////////////////////////////////////////////////////
//	直線のｵﾌｾｯﾄ座標

tuple<CPointF, CPointF> CalcOffsetLine
	(const CPointF& pts, const CPointF& pte, float r, BOOL bLeft)
{
	float	k = bLeft ? 1.0f : -1.0f;	// 左:+, 右:- 90°
	// 始点を原点にした傾き+90°
	float	q = atan2(pte.y-pts.y, pte.x-pts.x) + copysign(RAD(90.0f), k),
			cos_q = r * cos(q),
			sin_q = r * sin(q);

	return make_tuple(
		CPointF(cos_q+pts.x, sin_q+pts.y),	// 始点側のｵﾌｾｯﾄﾎﾟｲﾝﾄ
		CPointF(cos_q+pte.x, sin_q+pte.y)	// 終点側のｵﾌｾｯﾄﾎﾟｲﾝﾄ
	);
}

//////////////////////////////////////////////////////////////////////
//	２線がなす角度を求める(交点を原点ｾﾞﾛ扱い)

float CalcBetweenAngle(const CPointF& pts, const CPointF& pte)
{
	// pto(２線の共通点)を原点に
	CPointF	pt(pte);
	// ptsの角度でpteをｱﾌｨﾝ変換
	pt.RoundPoint(-atan2(pts.y, pts.x));
	// ptsが0度扱いなのでptの角度が２線間の角度(-180～180°で返す)
	return atan2(pt.y, pt.x);
}

//////////////////////////////////////////////////////////////////////
//	ｵﾌｾｯﾄ分平行移動させた線分同士の交点を求める

optional<CPointF> CalcOffsetIntersectionPoint_LL
	(const CPointF& pts, const CPointF& pte, float t1, float t2, BOOL bLeft)
{
	CPointF	pt1s, pt1e, pt2s, pt2e, pt;

	tie(pt1s, pt1e) = CalcOffsetLine(pts, pt, t1, bLeft);
	tie(pt2s, pt2e) = CalcOffsetLine(pt, pte, t2, bLeft);

	return CalcIntersectionPoint_LL(pt1s, pt1e, pt2s, pt2e, FALSE);
}

//////////////////////////////////////////////////////////////////////
//	ｵﾌｾｯﾄ分平行移動させた線と円弧の交点を求める
//		bRound: CW=FALSE, CCW=TRUE

optional<CPointF> CalcOffsetIntersectionPoint_LC
	(const CPointF& pts, const CPointF& pto, float ro, float t1, float t2, BOOL bRound, BOOL bLeft)
{
	int		nResult = 0, k;
	CPointF	pt1, pt2, pr1, pr2;

	// 直線のｵﾌｾｯﾄを計算
	tie(pt1, pt2) = CalcOffsetLine(pts, CPointF(), t1, bLeft);
	// 円のｵﾌｾｯﾄを計算
	k = bRound ? -1 : 1;	// 反時計回りならﾏｲﾅｽｵﾌｾｯﾄ(左側基準)
	if ( !bLeft )
		k = -k;
	ro += t2 * k;
	if ( ro > 0 ) {
		// ｵﾌｾｯﾄ後の直線と円の交点
		tie(nResult, pr1, pr2) = CalcIntersectionPoint_LC(pt1, pt2, pto, ro, FALSE);
		if ( nResult>1 && GAPCALC(pr1)>GAPCALC(pr2) )
			pr1 = pr2;
	}

	return nResult > 0 ? pr1 : optional<CPointF>();
}

//////////////////////////////////////////////////////////////////////
//	ｵﾌｾｯﾄ分平行移動させた線と楕円弧の交点を求める
//		bRound: CW=FALSE, CCW=TRUE

optional<CPointF> CalcOffsetIntersectionPoint_LE
	(const CPointF& pts, const CPointF& pto, float a, float b, float q, float rr,
		BOOL bRound, BOOL bLeft)
{
	int		nResult = 0, k;
	CPointF	pt1, pt2, pr1, pr2;

	// 直線のｵﾌｾｯﾄを計算
	tie(pt1, pt2) = CalcOffsetLine(pts, CPointF(), rr, bLeft);
	// 楕円のｵﾌｾｯﾄを計算
	k = bRound ? -1 : 1;	// 反時計回りならﾏｲﾅｽｵﾌｾｯﾄ(左側基準)
	if ( !bLeft )
		k = -k;
	a += rr * k;
	b += rr * k;
	if ( a>0 && b>0 ) {
		// ｵﾌｾｯﾄ後の直線と楕円の交点
		tie(nResult, pr1, pr2) = CalcIntersectionPoint_LE(pt1, pt2, pto, a, b, q, FALSE);
		if ( nResult>1 && GAPCALC(pr1)>GAPCALC(pr2) )
			pr1 = pr2;
	}

	return nResult > 0 ? pr1 : optional<CPointF>();
}

//////////////////////////////////////////////////////////////////////
//	点が多角形の内側か否か

BOOL IsPointInPolygon(const CPointF& ptTarget, const CVPointF& pt)
{
	int		iCountCrossing = 0;
	size_t	ui, uiCountPoint = pt.size();
	CPointF	pt0(pt[0]), pt1;
	BOOL	bFlag0x = (ptTarget.x <= pt0.x), bFlag1x,
			bFlag0y = (ptTarget.y <= pt0.y), bFlag1y;

	// レイの方向は、Ｘプラス方向
	for( ui=1; ui<uiCountPoint+1; ui++ ) {
		pt1 = pt[ui%uiCountPoint];	// 最後は始点が入る（多角形データの始点と終点が一致していないデータ対応）
		bFlag1x = (ptTarget.x <= pt1.x);
		bFlag1y = (ptTarget.y <= pt1.y);
		if( bFlag0y != bFlag1y ) {
			// 線分はレイを横切る可能性あり。
			if( bFlag0x == bFlag1x ) {
				// 線分の２端点は対象点に対して両方右か両方左にある
				if( bFlag0x ) {
					// 完全に右。⇒線分はレイを横切る
					iCountCrossing += (bFlag0y ? -1 : 1);	// 上から下にレイを横切るときには、交差回数を１引く、下から上は１足す。
				}
			}
			else {
				// レイと交差するかどうか、対象点と同じ高さで、対象点の右で交差するか、左で交差するかを求める。
				if( ptTarget.x <= ( pt0.x + (pt1.x - pt0.x) * (ptTarget.y - pt0.y ) / (pt1.y - pt0.y) ) ) {
					// 線分は、対象点と同じ高さで、対象点の右で交差する。⇒線分はレイを横切る
					iCountCrossing += (bFlag0y ? -1 : 1);	// 上から下にレイを横切るときには、交差回数を１引く、下から上は１足す。
				}
			}
		}
		// 次の判定のために、
		pt0 = pt1;
		bFlag0x = bFlag1x;
		bFlag0y = bFlag1y;
	}

	// クロスカウントがゼロのとき外、ゼロ以外のとき内。
	return (0 != iCountCrossing);
}

//////////////////////////////////////////////////////////////////////
//	点と直線の距離を求める

float CalcLineDistancePt(const CPointF& pts, const CPointF& pte, const CPointF& ptc)
{
	CPointF	pt(pte-pts), pto(ptc-pts);
	float	q = atan2(pt.y, pt.x);
	pto.RoundPoint(-q);
	return fabs(pto.y);
}

//////////////////////////////////////////////////////////////////////
//	２次方程式の解を求める
//	戻り値：int    -> 0:解なし, 1:重根, 2:２根
//	        float -> 解

tuple<int, float, float>	GetKon(float a, float b, float c)
{
	int		nResult;
	float	x1 = 0.0f, x2 = 0.0f;

	if ( fabs(a) < NCMIN ) {
		if ( fabs(b) < NCMIN )
			nResult = 0;
		else {
			x1 = x2 = -c / b;		// bx + c = 0
			nResult = 1;
		}
	}
	else {
		b /= a;		c /= a;
		float d = b*b - 4.0f*c;
		if ( d > NCMIN ) {
			d = sqrt(d);
			x1 = (b > 0.0 ? (-b - d) : (-b + d)) / 2.0f;
			x2 = c / x1;
			nResult = 2;
		}
		else if ( fabs(d) < NCMIN ) {
			x1 = x2 = -b / 2.0f;
			nResult = 1;
		}
		else
			nResult = 0;
	}
	return make_tuple(nResult, x1, x2);
}

//////////////////////////////////////////////////////////////////////
// 引数を越える素数を返す

UINT GetPrimeNumber(UINT nNum)
{
	UINT	i = 0;		// 初期値
	UINT	nLoop = 1;

	while ( i < nLoop ) {
		nLoop = (++nNum >> 1) + 1;
		for ( i=2; i<nLoop; i++ ) {
			if ( nNum % i == 0 )
				break;
		}
	}
	return nNum;
}
