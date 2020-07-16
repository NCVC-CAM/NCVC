// 3Dto2D.cpp
// ‚RŸŒ³•ÏŠ·‚Ì¸ŞÛ°ÊŞÙŠÖ”
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
// Ã“I•Ï”‚Ì‰Šú‰»

double	CPoint3D::ms_rx_cos = 0.0;
double	CPoint3D::ms_rx_sin = 0.0;
double	CPoint3D::ms_ry_cos = 0.0;
double	CPoint3D::ms_ry_sin = 0.0;
double	CPoint3D::ms_rz_cos = 0.0;
double	CPoint3D::ms_rz_sin = 0.0;

//////////////////////////////////////////////////////////////////////
// ‚Qü‚ÌŒğ“_‚ğ‹‚ß‚é

optional<CPointD> CalcIntersectionPoint_LL
	(const CPointD& pts1, const CPointD& pte1, const CPointD& pts2, const CPointD& pte2)
{
	BOOL	bResult = FALSE;
	CPointD	pt;
	double	xa, ya, xb, yb;

	xa = pte1.x - pts1.x;
	ya = pte1.y - pts1.y;
	xb = pte2.x - pts2.x;
	yb = pte2.y - pts2.y;

	if ( fabs(xa) < EPS ) {
		if ( fabs(xb) >= EPS ) {
			pt.x = pts1.x;
			pt.y = yb / xb * (pt.x - pts2.x) + pts2.y;
			bResult = TRUE;
		}
	}
	else {
		ya /= xa;
		if ( fabs(xb) < EPS ) {
			pt.x = pts2.x;
			pt.y = ya * (pt.x - pts1.x) + pts1.y;
			bResult = TRUE;
		}
		else {
			yb /= xb;
			if ( fabs(ya) < EPS ) {
				if ( fabs(yb) >= EPS ) {
					pt.x = (pts1.y - pts2.y) / yb + pts2.x;
					pt.y = pts1.y;
					bResult = TRUE;
				}
			}
			else {
				if ( fabs(yb) < EPS ) {
					pt.x = (pts2.y - pts1.y) / ya + pts1.x;
					pt.y = pts2.y;
				}
				else {
					pt.x = (pts2.y + pts1.y + pts1.x*ya - pts2.x*yb) / (ya - yb);
					pt.y = ya * (pt.x - pts1.x) + pts1.y;
				}
				bResult = TRUE;
			}
		}
	}

	// ŒŸZ
	if ( bResult ) {	// ü‚P
		if ( fabs(ya) < EPS ) {
			if ( min(pts1.x,pte1.x)>pt.x+EPS || max(pts1.x,pte1.x)<pt.x-EPS )
				bResult = FALSE;
		}
		else {
			if ( fabs(xa) < EPS )
				ya = pt.y;
			else
				ya = ( pt.x*ya - pts1.x*pte1.y + pts1.y*pte1.x ) / xa;
			if ( min(pts1.y,pte1.y)>ya+EPS || max(pts1.y,pte1.y)<ya-EPS )
				bResult = FALSE;
		}
	}
	if ( bResult ) {	// ü‚Q
		if ( fabs(yb) < EPS ) {
			if ( min(pts2.x,pte2.x)>pt.x+EPS || max(pts2.x,pte2.x)<pt.x-EPS )
				bResult = FALSE;
		}
		else {
			if ( fabs(xb) < EPS )
				yb = pt.y;
			else
				yb = ( pt.x*yb - pts2.x*pte2.y + pts2.y*pte2.x ) / xb;
			if ( min(pts2.y,pte2.y)>yb+EPS || max(pts2.y,pte2.y)<yb-EPS )
				bResult = FALSE;
		}
	}

	return bResult ? pt : optional<CPointD>();
}

//////////////////////////////////////////////////////////////////////
//	’¼ü‚Æ‰~‚ÌŒğ“_‚ğ‹‚ß‚é

tuple<int, CPointD, CPointD> CalcIntersectionPoint_LC
	(const CPointD& pts, const CPointD& pte, const CPointD& ptc, double r)
{
	int		nResult = 0;
	CPointD	pr1, pr2,
			pt(pte-pts), pto(ptc-pts);	// n“_‚ğ’†S‚É
	double	q = atan2(pt.y, pt.x);		// ü‚ÌŒX‚«
	pto.RoundPoint(-q);					// ‹t‰ñ“]
	r = fabs(r);
	double	y = fabs(pto.y);

	// Œğ“_‚ª‚È‚¢ğŒ
	if ( y - r > EPS )
		return make_tuple(0, pr1, pr2);

	pt.RoundPoint(-q);

	if ( fabs(y-r) <= EPS ) {
		// Ú‚·‚é
		nResult = 1;
		pr1.x = pto.x;
		// ”ÍˆÍÁª¯¸(”ÍˆÍŠO‚Å‚à“š‚¦‚Í•Ô‚·)
		if ( pr1.x <= -EPS || pt.x+EPS <= pr1.x )
			nResult--;
		pr1.RoundPoint(q);
		pr2 = pr1;
	}
	else {
		// ‚QŒğ“_
		nResult = 2;
		r = sqrt(r*r - y*y);
		pr1.x = pto.x + r;
		pr2.x = pto.x - r;
		// ”ÍˆÍÁª¯¸
		if ( pr2.x <= -EPS || pt.x+EPS <= pr2.x )
			nResult--;
		if ( pr1.x <= -EPS || pt.x+EPS <= pr1.x ) {
			nResult--;
			if ( nResult > 0 )
				pr1 = pr2;
		}
		// ‰ñ“]‚ğŒ³‚É–ß‚·
		pr1.RoundPoint(q);
		pr2.RoundPoint(q);
	}

	// •½sˆÚ“®
	pr1 += pts;
	pr2 += pts;

	return make_tuple(nResult, pr1, pr2);
}

//////////////////////////////////////////////////////////////////////
// ‚Q‚Â‚Ì‰~‚ÌŒğ“_‚ğ‹‚ß‚é

tuple<int, CPointD, CPointD> CalcIntersectionPoint_CC
	(const CPointD& pts, const CPointD& pte, double r1, double r2)
{
	int		nResult;
	CPointD	pr1, pr2,
			pt(pte-pts);			// n“_‚r‚ªŒ´“_‚Æ‚È‚é‚æ‚¤•½sˆÚ“®
	double	l,
			q = atan2(pt.y, pt.x);	// ‰ñ“]Šp“x
	r1 = fabs(r1);
	r2 = fabs(r2);

	// ‰~‚Ì•û’ö®‚æ‚èC(s.x, s.y)C(e.x, e.y) ‚ğ’†S‚Æ‚µ‚½
	// ‚Q‚Â‚Ì‰~‚ÌŒğ“_‚ğ‹‚ß‚é

	// ‹t‰ñ“]‚µ‚½‚à‚Ì‚Æ‚µ‚Äl‚¦‚é‚ÆA‚±‚ê‚ÅOK‚¾‚ª’x‚¢(‚Æv‚¤)
//	l = pt.hypot();
	// yÀ•W‚ªx²ã‚É‚È‚é‚æ‚¤‹t‰ñ“]
	pt.RoundPoint(-q);
	l = fabs(pt.x);		// ‚Q‰~‚Ì’†S‚Ì‹——£

	// Œğ“_‚ª‚È‚¢ğŒ(“¯S‰~{‹——£¾ŞÛ}, ‚Q‰~‚Ì”¼Œa‚æ‚è‹——£‚ª‘å‚«‚¢, ¬‚³‚¢
	if ( l < EPS || l-(r1+r2) > EPS || l < fabs(r1-r2) )
		return make_tuple(0, pr1, pr2);

	// Œğ“_ŒvZ
	if ( fabs(l-(r1+r2)) <= EPS ) {	// ‚Q‰~‚Ì”¼Œa‚Æ’†S‹——£‚ª“™‚µ‚¢
		// Ú‚·‚éğŒ(y=0)
		pr1.x = r1;
		// ‰ñ“]‚ğ•œŒ³
		pr1.RoundPoint(q);
		pr2 = pr1;
		// ‰ğ‚Ì”
		nResult = 1;
	}
	else {
		// Œğ“_‚Q‚Â
		pr1.x = pr2.x = (r1*r1 - r2*r2 + l*l) / (2.0*l);
		pr1.y = sqrt( r1*r1 - pr1.x*pr1.x );
		pr2.y = -pr1.y;
		// ‰ñ“]‚ğ•œŒ³
		pr1.RoundPoint(q);
		pr2.RoundPoint(q);
		// ‰ğ‚Ì”
		nResult = 2;
	}

	// •½sˆÚ“®•œŒ³
	pr1 += pts;
	pr2 += pts;

	return make_tuple(nResult, pr1, pr2);
}

//////////////////////////////////////////////////////////////////////
// ü‚Æü‚Ì’[“_‚ğ’†S‚Æ‚µ‚½‰~‚Æ‚ÌŒğ“_‚ğ‹‚ß‚é

CPointD	CalcIntersectionPoint_TC
	(const CPointD& ptOrg, double r, const CPointD& ptSrc)
{
	CPointD	pt;

	// ptOrg ‚ğ’†S‚Æ‚µ‚½‰~‚Æ ptSrc ‚ÌŒğ“_
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
		pt.x = pt.y / a + ptOrg.x;	// x‚ğÄŒvZ(y‚Æa‚Ì•„†‚ğl—¶)
		pt.y += ptOrg.y;			// Œ´“_•â³
	}

	return pt;
}

//////////////////////////////////////////////////////////////////////
// ‚Qü‚ª‚È‚·Šp“x‚ğ‹‚ß‚é(Œğ“_‚ğŒ´“_¾ŞÛˆµ‚¢)

double CalcBetweenAngle_LL(const CPointD& pts, const CPointD& pte)
{
	// pto(‚Qü‚Ì‹¤’Ê“_)‚ğŒ´“_‚É
	CPointD	pt(pte);
	// pts‚ÌŠp“x‚Åpte‚ğ±Ì¨İ•ÏŠ·
	pt.RoundPoint(-atan2(pts.y, pts.x));
	// pts‚ª0“xˆµ‚¢‚È‚Ì‚Åpt2‚ÌŠp“x‚ª‚QüŠÔ‚ÌŠp“x(-180`180‹‚Å•Ô‚·)
	return atan2(pt.y, pt.x);
}

//////////////////////////////////////////////////////////////////////
// µÌ¾¯Ä•ª•½sˆÚ“®‚³‚¹‚½ü•ª“¯m‚ÌŒğ“_‚ğ‹‚ß‚é
//		k=[1|-1]:µÌ¾¯Ä•ûŒüw¦ŒW”, 0:‘ŠèÀ•W‚©‚ç©“®ŒvZ

inline CPointD CalcOffsetIntersectionPoint_X
	(const CPointD& pt1, const CPointD& pt2, int k1, int k2, double r)
{
	CPointD	pt;

	pt.x = _copysign(r, k1==0?pt2.x:k1);		// pt1‚Ì¶‰E(})
	if ( fabs(pt2.y) < EPS )
		pt.y = _copysign(r, k2==0?pt1.y:k2);	// pt2‚Ìã‰º(})
	else {
		double a = pt2.y / pt2.x;
		pt.y = a * pt.x + _copysign(r*sqrt(1+a*a), k2==0?pt1.y:k2);
	}

	return pt;
}

inline CPointD CalcOffsetIntersectionPoint_Y
	(const CPointD& pt1, const CPointD& pt2, int k1, int k2, double r)
{
	CPointD	pt;

	pt.y = _copysign(r, k1==0?pt2.y:k1);		// pt1‚Ìã‰º(})
	if ( fabs(pt2.x) < EPS )
		pt.x = _copysign(r, k2==0?pt1.x:k2);	// pt2‚Ì¶‰E(})
	else {
		double a = pt2.y / pt2.x;
		pt.x = ( pt.y - _copysign(r*sqrt(1+a*a), k2==0?pt1.x:k2) ) / a;
	}

	return pt;
}

optional<CPointD> CalcOffsetIntersectionPoint_LL
	(const CPointD& pts, const CPointD& pte, int k1, int k2, double r)
{
	BOOL	bResult = TRUE;
	CPointD	pt;
	double	sx = fabs(pts.x), sy = fabs(pts.y), ex = fabs(pte.x), ey = fabs(pte.y);

	// ‚Qü‚ÌÚ‰~ŒvZ‚©‚ç‹‚ß‚é
	if ( sx < EPS || ex < EPS ) {
		if ( sx < EPS && ex < EPS ) {		// —¼•û‚’¼üH
			if ( k1!=0 && k1==k2 ) {		// •„†‚ª“™‚µ‚¢‚Æ‚«‚¾‚¯
				pt.x = _copysign(r, k1);
				pt.y = 0;
			}
			else
				bResult = FALSE;
		}
		else {
			pt = sx < EPS ?
				CalcOffsetIntersectionPoint_X(pts, pte, k1, k2, r) :
				CalcOffsetIntersectionPoint_X(pte, pts, k2, k1, r);
		}
	}
	else if ( sy < EPS || ey < EPS ) {
		if ( sy < EPS && ey < EPS ) {		// —¼•û…•½üH
			if ( k1!=0 && k1==k2 ) {
				pt.x = 0;
				pt.y = _copysign(r, k1);
			}
			else
				bResult = FALSE;
		}
		else {
			pt = sy < EPS ?
				CalcOffsetIntersectionPoint_Y(pts, pte, k1, k2, r) :
				CalcOffsetIntersectionPoint_Y(pte, pts, k2, k1, r);
		}
	}
	else {
		double a1 = pts.y / pts.x;
		double a2 = pte.y / pte.x;
		if ( fabs(a1-a2) > EPS ) {	// ŒX‚«(¾ŞÛœZ)Áª¯¸
			double b1 = _copysign(r*sqrt(1+a1*a1), k1==0?(a2-a1)*pte.x:k1);
			double b2 = _copysign(r*sqrt(1+a2*a2), k2==0?(a1-a2)*pts.x:k2);
			pt.x = (b2 - b1) / (a1 - a2);
			pt.y = a1 * pt.x + b1;
		}
		else if ( k1 != 0 ) {
			double	q  = atan2(pts.y, pts.x),
					qq = RoundUp(q*DEG);	// Šp“x‚Ì”»’f—p
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
// µÌ¾¯Ä•ª•½sˆÚ“®‚³‚¹‚½ü‚Æ‰~ŒÊ‚ÌŒğ“_‚ğ‹‚ß‚é
//		k=[1|-1]:µÌ¾¯Ä•ûŒüw¦ŒW”, 0:‘ŠèÀ•W‚©‚ç©“®ŒvZ
//		nRound: CW=1, CCW=-1
//	rr‚Ì•„†‚ÆŒğ“_‚ğ•Ô‚·

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
// ‚QŸ•û’ö®‚Ì‰ğ‚ğ‹‚ß‚é
// –ß‚è’lFint    -> 0:‰ğ‚È‚µ, 1:dª, 2:‚Qª
//         double -> ‰ğ
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
// ˆø”‚ğ‰z‚¦‚é‘f”‚ğ•Ô‚·

UINT GetPrimeNumber(UINT nNum)
{
	UINT	i = 0;		// ‰Šú’l
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
