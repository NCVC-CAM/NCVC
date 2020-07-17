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

double	CPoint3D::ms_rx_cos = 0.0;
double	CPoint3D::ms_rx_sin = 0.0;
double	CPoint3D::ms_ry_cos = 0.0;
double	CPoint3D::ms_ry_sin = 0.0;
double	CPoint3D::ms_rz_cos = 0.0;
double	CPoint3D::ms_rz_sin = 0.0;

//////////////////////////////////////////////////////////////////////
// ２線の交点を求める

optional<CPointD> CalcIntersectionPoint_LL
	(const CPointD& pts1, const CPointD& pte1, const CPointD& pts2, const CPointD& pte2,
		BOOL bRangeChk/*=TRUE*/)
{
	BOOL	bResult = FALSE;
	CPointD	pt;
	double	xa, ya, xb, yb,
			minX1, minY1, maxX1, maxY1,
			minX2, minY2, maxX2, maxY2;

	// 計算前準備
	xa = pte1.x - pts1.x;
	ya = pte1.y - pts1.y;
	xb = pte2.x - pts2.x;
	yb = pte2.y - pts2.y;
	if ( pts1.x < pte1.x ) {
		minX1 = pts1.x;		maxX1 = pte1.x;
	}
	else {
		minX1 = pte1.x;		maxX1 = pts1.x;
	}
	if ( pts1.y < pte1.y ) {
		minY1 = pts1.y;		maxY1 = pte1.y;
	}
	else {
		minY1 = pte1.y;		maxY1 = pts1.y;
	}
	if ( pts2.x < pte2.x ) {
		minX2 = pts2.x;		maxX2 = pte2.x;
	}
	else {
		minX2 = pte2.x;		maxX2 = pts2.x;
	}
	if ( pts2.y < pte2.y ) {
		minY2 = pts2.y;		maxY2 = pte2.y;
	}
	else {
		minY2 = pte2.y;		maxY2 = pts2.y;
	}

	// 交点計算
	if ( fabs(xa) < EPS ) {
		if ( fabs(xb) >= EPS ) {
			pt.x = pts1.x;
			pt.y = yb * (pts1.x - pts2.x) / xb + pts2.y;
			bResult = TRUE;
		}
		// 両方とも垂直線では解なし
	}
	else {
		if ( fabs(xb) < EPS ) {
			pt.x = pts2.x;
			pt.y = ya * (pts2.x - pts1.x) / xa + pts1.y;
			bResult = TRUE;
		}
		else {
			double	yaxa = ya / xa,
					ybxb = yb / xb;
			if ( fabs(ya) < EPS ) {
				if ( fabs(yb) >= EPS ) {
					pt.x = (pts1.y - pts2.y) / ybxb + pts2.x;
					pt.y = pts1.y;
					bResult = TRUE;
				}
				// 両方水平線では解なし
			}
			else {
				if ( fabs(yb) < EPS ) {
					pt.x = (pts2.y - pts1.y) / yaxa + pts1.x;
					pt.y = pts2.y;
				}
				else {
					if ( fabs(yaxa-ybxb) < EPS ) {
						optional<CPointD>	pt1, pt2;
						// 水平・垂直以外で傾きが同じ(ｾﾞﾛ除算防止)
						if ( minX2<=pts1.x && pts1.x<=maxX2 && minY2<=pts1.y && pts1.y<=maxY2 )
							pt1 = pts1;
						else if ( minX2<=pte1.x && pte1.x<=maxX2 && minY2<=pte1.y && pte1.y<=maxY2 )
							pt1 = pte1;
						if ( minX1<=pts2.x && pts2.x<=maxX1 && minY1<=pts2.y && pts2.y<=maxY1 )
							pt2 = pts2;
						else if ( minX1<=pte2.x && pte2.x<=maxX1 && minY1<=pte2.y && pte2.y<=maxY1 )
							pt2 = pte2;
						if ( pt1 && pt2 ) {
							// 範囲内にある中点を交点とする
							if ( (*pt1).x < (*pt2).x ) {
								minX1 = (*pt1).x;	maxX1 = (*pt2).x;
							}
							else {
								minX1 = (*pt2).x;	maxX1 = (*pt1).x;
							}
							if ( (*pt1).y < (*pt2).y ) {
								minY1 = (*pt1).y;	maxY1 = (*pt2).y;
							}
							else {
								minY1 = (*pt2).y;	maxY1 = (*pt1).y;
							}
							pt.x = ( maxX1 - minX1 ) / 2.0 + minX1;
							pt.y = ( maxY1 - minY1 ) / 2.0 + minY1;
							return pt;	// 検算の必要なし
						}
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
	if ( bRangeChk && bResult ) {	// 線１
		if ( fabs(ya) < EPS ) {
			// 水平線では x の範囲ﾁｪｯｸのみ
			if ( pt.x<minX1 || maxX1<pt.x )
				bResult = FALSE;
		}
		else if ( fabs(xa) < EPS ) {
			// 垂直線では y の範囲ﾁｪｯｸのみ
			if ( pt.y<minY1 || maxY1<pt.y )
				bResult = FALSE;
		}
		else {
			if ( pt.x<minX1 || maxX1<pt.x || pt.y<minY1 || maxY1<pt.y )
				bResult = FALSE;
		}
	}
	if ( bRangeChk && bResult ) {	// 線２
		if ( fabs(yb) < EPS ) {
			if ( pt.x<minX2 || maxX2<pt.x )
				bResult = FALSE;
		}
		else if ( fabs(xb) < EPS ) {
			if ( pt.y<minY2 || maxY2<pt.y )
				bResult = FALSE;
		}
		else {
			if ( pt.x<minX2 || maxX2<pt.x || pt.y<minY2 || maxY2<pt.y )
				bResult = FALSE;
		}
	}

	return bResult ? pt : optional<CPointD>();
}

//////////////////////////////////////////////////////////////////////
//	直線と円の交点を求める

tuple<int, CPointD, CPointD> CalcIntersectionPoint_LC
	(const CPointD& pts, const CPointD& pte, const CPointD& ptc, double r,
		BOOL bRangeChk/*=TRUE*/)
{
	int		nResult = 0;
	CPointD	pr1, pr2,
			pt(pte-pts), pto(ptc-pts);	// 始点を中心に
	double	q = atan2(pt.y, pt.x);		// 線の傾き
	pto.RoundPoint(-q);					// 逆回転
	r = fabs(r);
	double	y = fabs(pto.y);

	// 交点がない条件
	if ( y - r > EPS )
		return make_tuple(nResult, pr1, pr2);

	pt.RoundPoint(-q);	// ptが必ず正方向に

	if ( fabs(y-r) <= EPS ) {
		// 接する
		pr1.x = pto.x;
		// 範囲ﾁｪｯｸ
		nResult = ( bRangeChk && (pr1.x < 0 || pt.x < pr1.x) ) ? 0 : 1;
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
			if ( pr2.x < 0 || pt.x < pr2.x )
				nResult = 1;		// pr2の解を採用しない
			if ( pr1.x < 0 || pt.x < pr1.x ) {
				nResult--;
				if ( nResult > 0 )
					std::swap(pr1, pr2);	// pr1の解を採用しない(代入では接線と区別付かない)
			}
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
// ２つの円の交点を求める

tuple<int, CPointD, CPointD> CalcIntersectionPoint_CC
	(const CPointD& pts, const CPointD& pte, double r1, double r2)
{
	int		nResult = 0;
	CPointD	pr1, pr2,
			pt(pte-pts);			// 始点Ｓが原点となるよう平行移動
	double	l,
			q = atan2(pt.y, pt.x);	// 回転角度
	r1 = fabs(r1);
	r2 = fabs(r2);

	// 円の方程式より，(s.x, s.y)，(e.x, e.y) を中心とした
	// ２つの円の交点を求める

	// 逆回転したものとして考えると、これでOKだが遅い(と思う)
//	l = pt.hypot();
	// 他方(pte)の中心座標がx軸上になるよう逆回転
	pt.RoundPoint(-q);
	l = fabs(pt.x);		// ２円の中心の距離

	// 交点がない条件(同心円{距離ｾﾞﾛ}, ２円の半径より距離が大きい, 小さい
	if ( l < EPS || l > (r1+r2) || l < fabs(r1-r2) )
		return make_tuple(nResult, pr1, pr2);

	// 交点計算
	if ( fabs(l-(r1+r2)) <= EPS || fabs(r1-(r2-l)) <= EPS || fabs(r1-(r2+l)) <= EPS ) {
		// 接する条件(y=0)
		pr1.x = _copysign(r1, r1-r2);	// r2の方が大きいとﾏｲﾅｽ
		// 回転を復元
		pr1.RoundPoint(q);
		pr2 = pr1;
		// 解の数
		nResult = 1;
	}
	else {
		// 交点２つ
		pr1.x = pr2.x = (r1*r1 - r2*r2 + l*l) / (2.0*l);
		pr1.y = sqrt( r1*r1 - pr1.x*pr1.x );
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
// 線と線の端点を中心とした円との交点を求める

CPointD	CalcIntersectionPoint_TC
	(const CPointD& ptOrg, double r, const CPointD& ptSrc)
{
	CPointD	pt;

	// ptOrg を中心とした円と ptSrc の交点
	if ( fabs( ptSrc.x - ptOrg.x ) < EPS ) {
		pt.x = ptOrg.x;
		pt.y = ptOrg.y + _copysign(r, ptSrc.y - ptOrg.y);
	}
	else if ( fabs( ptSrc.y - ptOrg.y ) < EPS ) {
		pt.x = ptOrg.x + _copysign(r, ptSrc.x - ptOrg.x);
		pt.y = ptOrg.y;
	}
	else {
		double a = (ptSrc.y - ptOrg.y) / (ptSrc.x - ptOrg.x);
		pt.x = r * sqrt( 1 / (1+a*a) );
		pt.y = (ptOrg.x < ptSrc.x ? a : -a) * pt.x;
		pt.x = pt.y / a + ptOrg.x;	// xを再計算(yとaの符号を考慮)
		pt.y += ptOrg.y;			// 原点補正
	}

	return pt;
}

//////////////////////////////////////////////////////////////////////
// ２線がなす角度を求める(交点を原点ｾﾞﾛ扱い)

double CalcBetweenAngle_LL(const CPointD& pts, const CPointD& pte)
{
	// pto(２線の共通点)を原点に
	CPointD	pt(pte);
	// ptsの角度でpteをｱﾌｨﾝ変換
	pt.RoundPoint(-atan2(pts.y, pts.x));
	// ptsが0度扱いなのでpt2の角度が２線間の角度(-180〜180°で返す)
	return atan2(pt.y, pt.x);
}

//////////////////////////////////////////////////////////////////////
// ｵﾌｾｯﾄ分平行移動させた線分同士の交点を求める
//		k=[1|-1]:ｵﾌｾｯﾄ方向指示係数, 0:相手座標から自動計算

inline CPointD CalcOffsetIntersectionPoint_V
	(const CPointD& pt1, const CPointD& pt2, int k1, int k2, double r)
{
	CPointD	pt;
	// pt1が垂直
	pt.x = _copysign(r, k1==0?pt2.x:k1);		// pt1の左右(±)
	if ( fabs(pt2.y) < EPS )
		pt.y = _copysign(r, k2==0?pt1.y:k2);	// pt2の上下(±)
	else {
		double a = pt2.y / pt2.x;
		pt.y = a * pt.x + _copysign(r*sqrt(1+a*a), k2==0?pt1.y:k2);
	}

	return pt;
}

inline CPointD CalcOffsetIntersectionPoint_H
	(const CPointD& pt1, const CPointD& pt2, int k1, int k2, double r)
{
	CPointD	pt;
	// pt1が水平
	pt.y = _copysign(r, k1==0?pt2.y:k1);		// pt1の上下(±)
	if ( fabs(pt2.x) < EPS )
		pt.x = _copysign(r, k2==0?pt1.x:k2);	// pt2の左右(±)
	else {
		double a = pt2.y / pt2.x;
		pt.x = ( pt.y - _copysign(r*sqrt(1+a*a), k2==0?-a*pt1.x:k2) ) / a;
	}

	return pt;
}

optional<CPointD> CalcOffsetIntersectionPoint_LL
	(const CPointD& pts, const CPointD& pte, int k1, int k2, double r)
{
	BOOL	bResult = TRUE;
	CPointD	pt;
	double	sx = fabs(pts.x), sy = fabs(pts.y), ex = fabs(pte.x), ey = fabs(pte.y);

	// ２線の接円計算から求める
	if ( sx < EPS || ex < EPS ) {
		if ( sx < EPS && ex < EPS ) {		// 両方垂直線？
			if ( k1!=0 && k1==k2 ) {		// 符号が等しいときだけ
				pt.x = _copysign(r, k1);
				pt.y = 0;
			}
			else
				bResult = FALSE;
		}
		else {
			pt = sx < EPS ?
				CalcOffsetIntersectionPoint_V(pts, pte, k1, k2, r) :
				CalcOffsetIntersectionPoint_V(pte, pts, k2, k1, r);
		}
	}
	else if ( sy < EPS || ey < EPS ) {
		if ( sy < EPS && ey < EPS ) {		// 両方水平線？
			if ( k1!=0 && k1==k2 ) {
				pt.x = 0;
				pt.y = _copysign(r, k1);
			}
			else
				bResult = FALSE;
		}
		else {
			pt = sy < EPS ?
				CalcOffsetIntersectionPoint_H(pts, pte, k1, k2, r) :
				CalcOffsetIntersectionPoint_H(pte, pts, k2, k1, r);
		}
	}
	else {
		double a1 = pts.y / pts.x;
		double a2 = pte.y / pte.x;
		if ( fabs(a1-a2) > EPS ) {	// 傾き(ｾﾞﾛ除算)ﾁｪｯｸ
			double b1 = _copysign(r*sqrt(1+a1*a1), k1==0?(a2-a1)*pte.x:k1);
			double b2 = _copysign(r*sqrt(1+a2*a2), k2==0?(a1-a2)*pts.x:k2);
			pt.x = (b2 - b1) / (a1 - a2);
			pt.y = a1 * pt.x + b1;
		}
		else if ( k1 != 0 ) {
			double	q  = atan2(pts.y, pts.x),
					qq = RoundUp(q*DEG);	// 角度の判断用
			if ( qq>=90.0 || qq<-90.0 )
				k1 = -k1;
			q += 90.0*RAD * k1;		// +90 or -90
			pt.x = r * cos(q);
			pt.y = r * sin(q);
		}
		else
			bResult = FALSE;
	}

	return bResult ? pt : optional<CPointD>();
}

//////////////////////////////////////////////////////////////////////
// ｵﾌｾｯﾄ分平行移動させた線と円弧の交点を求める
//		k=[1|-1]:ｵﾌｾｯﾄ方向指示係数, 0:相手座標から自動計算
//		nRound: CW=1, CCW=-1
//	rrの符号と交点を返す

tuple<BOOL, CPointD, double> CalcOffsetIntersectionPoint_LC
	(const CPointD& pts, const CPointD& pto, double ro, double rr, int nRound, int k1, int k2)
{
	CPointD	pt, p1, p2;
	double	a, b, o1, o2, z;
	int		nResult;
	BOOL	bResult = TRUE;

	if ( fabs(pts.x) < EPS ) {
		a = 0;
		o1 = pto.y;	o2 = pto.x;
		if ( fabs(pto.y) < EPS ) {
			b  = _copysign(rr, k1==0?pto.x:k1);
			rr = _copysign(rr, k2==0?pts.y*pto.x*nRound:k2);
			ro += rr;
		}
		else {
			b  = _copysign(rr, k1==0?-pto.y*nRound:k1);
			rr = _copysign(rr, k2==0?-pts.y*pto.y:k2);
			ro += rr;
		}
	}
	else if ( fabs(pts.y) < EPS ) {
		a = 0;
		o1 = pto.x;	o2 = pto.y;
		if ( fabs(pto.x) < EPS ) {
			b  = _copysign(rr, k1==0?pto.y:k1);
			rr = _copysign(rr, k2==0?-pts.x*pto.y*nRound:k2);
			ro += rr;
		}
		else {
			b  = _copysign(rr, k1==0?pto.x*nRound:k1);
			rr = _copysign(rr, k2==0?-pts.x*pto.x:k2);
			ro += rr;
		}
	}
	else {
		a  = pts.y / pts.x;
		o1 = pto.x;	o2 = pto.y;
		b  = _copysign(rr*sqrt(1+a*a), k1==0?(pto.x+a*pto.y)*nRound:k1);
		rr = _copysign(rr, k2==0?-(pts.x*pto.x+pts.y*pto.y):k2);
		ro += rr;
	}

	z = b - o2;
	tie(nResult, p1.x, p2.x) = GetKon(1.0+a*a, 2.0*(a*z-o1), o1*o1+z*z-ro*ro);
	switch ( nResult ) {
	case 1:
		if ( fabs(pts.x) < EPS ) {
			pt.y = p1.x;
			pt.x = b;
		}
		else {
			pt.x = p1.x;
			pt.y = a * pt.x + b;
		}
		break;
	case 2:
		if ( fabs(pts.x) < EPS ) {
			p1.y = p1.x;	p2.y = p2.x;
			p1.x = p2.x = b;
		}
		else {
			p1.y = a * p1.x + b;
			p2.y = a * p2.x + b;
		}
		pt = GAPCALC(p1) < GAPCALC(p2) ? p1 : p2;
		break;
	default:
		bResult = FALSE;
		break;
	}

	return make_tuple(bResult, pt, rr);
}

//////////////////////////////////////////////////////////////////////
// ２次方程式の解を求める
// 戻り値：int    -> 0:解なし, 1:重根, 2:２根
//         double -> 解
tuple<int, double, double>	GetKon(double a, double b, double c)
{
	int		nResult;
	double	x1 = 0.0, x2 = 0.0;

	if ( fabs(a) < EPS ) {
		if ( fabs(b) < EPS )
			nResult = 0;
		else {
			x1 = x2 = -c / b;		// bx + c = 0
			nResult = 1;
		}
	}
	else {
		b /= a;		c /= a;
		double d = b*b - 4.0*c;
		if ( d > EPS ) {
			d = sqrt(d);
			x1 = (b > 0.0 ? (-b - d) : (-b + d)) / 2.0;
			x2 = c / x1;
			nResult = 2;
		}
		else if ( fabs(d) < EPS ) {
			x1 = x2 = -b / 2.0;
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
