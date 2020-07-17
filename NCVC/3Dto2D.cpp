// 3Dto2D.cpp
// �R�����ϊ��̸�۰��ي֐�
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
// �ÓI�ϐ��̏�����

double	CPoint3D::ms_rx_cos = 0.0;
double	CPoint3D::ms_rx_sin = 0.0;
double	CPoint3D::ms_ry_cos = 0.0;
double	CPoint3D::ms_ry_sin = 0.0;
double	CPoint3D::ms_rz_cos = 0.0;
double	CPoint3D::ms_rz_sin = 0.0;

//////////////////////////////////////////////////////////////////////
//	�Q���̌�_�����߂�

optional<CPointD> CalcIntersectionPoint_LL
	(const CPointD& pts1, const CPointD& pte1, const CPointD& pts2, const CPointD& pte2,
		BOOL bRangeChk/*=TRUE*/)
{
	BOOL	bResult = FALSE;
	CPointD	pt;
	double	xa, ya, xb, yb,
			minX1, minY1, maxX1, maxY1,
			minX2, minY2, maxX2, maxY2;

	// �v�Z�O����
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

	// ��_�v�Z
	if ( fabs(xa) < NCMIN ) {
		if ( fabs(xb) >= NCMIN ) {
			pt.x = pts1.x;
			pt.y = yb * (pts1.x - pts2.x) / xb + pts2.y;
			bResult = TRUE;
		}
		// �����Ƃ��������ł͉��Ȃ�
	}
	else {
		if ( fabs(xb) < NCMIN ) {
			pt.x = pts2.x;
			pt.y = ya * (pts2.x - pts1.x) / xa + pts1.y;
			bResult = TRUE;
		}
		else {
			double	yaxa = ya / xa,
					ybxb = yb / xb;
			if ( fabs(ya) < NCMIN ) {
				if ( fabs(yb) >= NCMIN ) {
					pt.x = (pts1.y - pts2.y) / ybxb + pts2.x;
					pt.y = pts1.y;
					bResult = TRUE;
				}
				// �����������ł͉��Ȃ�
			}
			else {
				if ( fabs(yb) < NCMIN ) {
					pt.x = (pts2.y - pts1.y) / yaxa + pts1.x;
					pt.y = pts2.y;
				}
				else {
					if ( fabs(yaxa-ybxb) < NCMIN ) {
						optional<CPointD>	pt1, pt2;
						// �����E�����ȊO�ŌX��������(��ۏ��Z�h�~)
						if ( minX2<=pts1.x && pts1.x<=maxX2 && minY2<=pts1.y && pts1.y<=maxY2 )
							pt1 = pts1;
						else if ( minX2<=pte1.x && pte1.x<=maxX2 && minY2<=pte1.y && pte1.y<=maxY2 )
							pt1 = pte1;
						if ( minX1<=pts2.x && pts2.x<=maxX1 && minY1<=pts2.y && pts2.y<=maxY1 )
							pt2 = pts2;
						else if ( minX1<=pte2.x && pte2.x<=maxX1 && minY1<=pte2.y && pte2.y<=maxY1 )
							pt2 = pte2;
						if ( pt1 && pt2 ) {
							// �͈͓��ɂ��钆�_����_�Ƃ���
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
							return pt;	// ���Z�̕K�v�Ȃ�
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

	// ���Z(�͈�����)
	if ( bRangeChk && bResult ) {	// ���P
		if ( fabs(ya) < NCMIN ) {
			// �������ł� x �͈̔������̂�
			if ( pt.x<minX1 || maxX1<pt.x )
				bResult = FALSE;
		}
		else if ( fabs(xa) < NCMIN ) {
			// �������ł� y �͈̔������̂�
			if ( pt.y<minY1 || maxY1<pt.y )
				bResult = FALSE;
		}
		else {
			if ( pt.x<minX1 || maxX1<pt.x || pt.y<minY1 || maxY1<pt.y )
				bResult = FALSE;
		}
	}
	if ( bRangeChk && bResult ) {	// ���Q
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

	return bResult ? pt : optional<CPointD>();
}

//////////////////////////////////////////////////////////////////////
//	�����Ɖ~�̌�_�����߂�

tuple<int, CPointD, CPointD> CalcIntersectionPoint_LC
	(const CPointD& pts, const CPointD& pte, const CPointD& ptc, double r,
		BOOL bRangeChk/*=TRUE*/)
{
	int		nResult = 0;
	CPointD	pr1, pr2,
			pt(pte-pts), pto(ptc-pts);	// �n�_�𒆐S��
	double	q = atan2(pt.y, pt.x);		// ���̌X��
	pto.RoundPoint(-q);					// �t��]
	r = fabs(r);
	double	y = fabs(pto.y);

	// ��_���Ȃ�����
	if ( y - r > NCMIN )
		return make_tuple(nResult, pr1, pr2);

	pt.RoundPoint(-q);	// pt���K����������

	if ( fabs(y-r) <= NCMIN ) {
		// �ڂ���
		pr1.x = pto.x;
		// �͈�����
		nResult = ( bRangeChk && (pr1.x < 0 || pt.x < pr1.x) ) ? 0 : 1;
		// ��]�����ɖ߂�
		pr1.RoundPoint(q);
		pr2 = pr1;		// �ڂ���ϰ�
	}
	else {
		// �Q��_
		nResult = 2;
		r = sqrt(r*r - y*y);
		pr1.x = pto.x + r;
		pr2.x = pto.x - r;
		// �͈�����
		if ( bRangeChk ) {
			if ( pr1.x<0 || pt.x<pr1.x )
				nResult--;
			if ( pr2.x<0 || pt.x<pr2.x )
				nResult--;
			else if ( nResult == 1 )
				std::swap(pr1, pr2);	// pr1�̉����̗p���Ȃ�(����ł͐ڐ��Ƌ�ʕt���Ȃ�)
		}
		// ��]�����ɖ߂�
		pr1.RoundPoint(q);
		pr2.RoundPoint(q);
	}

	// ���s�ړ�
	pr1 += pts;
	pr2 += pts;

	return make_tuple(nResult, pr1, pr2);
}

//////////////////////////////////////////////////////////////////////
//	�Q�̉~�̌�_�����߂�

tuple<int, CPointD, CPointD> CalcIntersectionPoint_CC
	(const CPointD& pts, const CPointD& pte, double r1, double r2)
{
	r1 = fabs(r1);
	r2 = fabs(r2);

	int		nResult = 0;
	CPointD	pr1, pr2,
			pt(pte-pts);			// �n�_�r�����_�ƂȂ�悤���s�ړ�
	double	l,
			r1r2 = r1 + r2,			// �g�p�p�x�̍����v�Z
			q = atan2(pt.y, pt.x);	// ��]�p�x

	// �~�̕��������C(s.x, s.y)�C(e.x, e.y) �𒆐S�Ƃ���
	// �Q�̉~�̌�_�����߂�

	// ����(pte)�̒��S���W��x����ɂȂ�悤�t��]
	pt.RoundPoint(-q);
	l = RoundUp(fabs(pt.x));	// �Q�~�̒��S�̋���

	// ��_���Ȃ�����(���S�~{�������}, �Q�~�̔��a��苗�����傫��, ������
	if ( l < NCMIN || l > RoundUp(r1r2)+NCMIN || l < RoundUp(fabs(r1-r2))-NCMIN )
		return make_tuple(nResult, pr1, pr2);

	// ��_�v�Z
	if ( RoundUp(fabs(l-r1r2)) <= NCMIN || RoundUp(fabs(l-fabs(r1-r2))) <= NCMIN ) {
		// �ڂ������(y=0)��ϲŽ����
		pr1.x = ( r1 < r2 && l-r2 < 0 ) ? -r1 : r1;
		// ��]�𕜌�
		pr1.RoundPoint(q);
		pr2 = pr1;
		// ���̐�
		nResult = 1;
	}
	else {
		// ��_�Q��
		r1 *= r1;	// r1*r1���Q��g���̂Ő�Ɍv�Z
		pr1.x = pr2.x = (r1 - r2*r2 + l*l) / (2.0*l);
		pr1.y = sqrt(r1 - pr1.x*pr1.x);
		pr2.y = -pr1.y;
		// ��]�𕜌�
		pr1.RoundPoint(q);
		pr2.RoundPoint(q);
		// ���̐�
		nResult = 2;
	}

	// ���s�ړ�����
	pr1 += pts;
	pr2 += pts;

	return make_tuple(nResult, pr1, pr2);
}

//////////////////////////////////////////////////////////////////////
//	�����Ƒȉ~�̌�_�����߂�

tuple<int, CPointD, CPointD> CalcIntersectionPoint_LE
	(const CPointD& pts, const CPointD& pte,
		const CPointD& pto, double a, double b, double q,
		BOOL bRangeChk/*=TRUE*/)
{
	int		nResult = 0;
	CPointD	pt1(pts-pto), pt2(pte-pto), pr1, pr2;
	// �ȉ~�̌X���𑊎E
	pt1.RoundPoint(-q);
	pt2.RoundPoint(-q);
	// �v�Z�O����
	double	xa = pt2.x - pt1.x,
			ya = pt2.y - pt1.y,
			minX, minY, maxX, maxY;
	if ( pt1.x < pt2.x ) {
		minX = pt1.x;	maxX = pt2.x;
	}
	else {
		minX = pt2.x;	maxX = pt1.x;
	}
	if ( pt1.y < pt2.y ) {
		minY = pt1.y;	maxY = pt2.y;
	}
	else {
		minY = pt2.y;	maxY = pt1.y;
	}
	// ��_�v�Z
	if ( fabs(xa) < NCMIN ) {
		// ������
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
			else if ( nResult == 1 )	// pr1��NG��pr2��OK
				std::swap(pr1, pr2);
		}
	}
	else if ( fabs(ya) < NCMIN ) {
		// ������
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
				std::swap(pr1, pr2);
		}
	}
	else {
		// �Q������������x�l���v�Z
		tie(nResult, pr1.x, pr2.x) = GetKon(
			ya*ya + xa*xa*b*b/(a*a),
			2.0*ya*(xa*pt1.y - pt1.x*ya),
			(pt1.y*pt1.y-b*b)*xa*xa + (pt1.x*ya-2*pt1.y*xa)*pt1.x*ya );
		// y�l���v�Z
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
						std::swap(pr1, pr2);
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
//	���Ɛ��̒[�_�𒆐S�Ƃ����~�Ƃ̌�_�����߂�

CPointD	CalcIntersectionPoint_TC
	(const CPointD& ptOrg, double r, const CPointD& ptSrc)
{
	CPointD	pt;

	// ptOrg �𒆐S�Ƃ����~�� ptSrc �̌�_
	if ( fabs( ptSrc.x - ptOrg.x ) < NCMIN ) {
		pt.x = ptOrg.x;
		pt.y = ptOrg.y + _copysign(r, ptSrc.y - ptOrg.y);
	}
	else if ( fabs( ptSrc.y - ptOrg.y ) < NCMIN ) {
		pt.x = ptOrg.x + _copysign(r, ptSrc.x - ptOrg.x);
		pt.y = ptOrg.y;
	}
	else {
		double a = (ptSrc.y - ptOrg.y) / (ptSrc.x - ptOrg.x);
		pt.x = r * sqrt( 1 / (1+a*a) );
		pt.y = (ptOrg.x < ptSrc.x ? a : -a) * pt.x;
		pt.x = pt.y / a + ptOrg.x;	// x���Čv�Z(y��a�̕������l��)
		pt.y += ptOrg.y;			// ���_�␳
	}

	return pt;
}

//////////////////////////////////////////////////////////////////////
//	�����̵̾�č��W

tuple<CPointD, CPointD> CalcOffsetLine
	(const CPointD& pts, const CPointD& pte, double r, BOOL bLeft)
{
	CPointD	pt1(pte-pts),	// �n�_�����_��
			pt2(pts-pte);	// �I�_�����_��
	int		k = bLeft ? 1 : -1;	// ��:+, �E:- 90��
	double	q1 = atan2(pt1.y, pt1.x) + 90.0*RAD*k,
			q2 = atan2(pt2.y, pt2.x) - 90.0*RAD*k;

	// �n�_���̵̾���߲��
	pt1.x = r * cos(q1) + pts.x;
	pt1.y = r * sin(q1) + pts.y;
	// �I�_���̵̾���߲��
	pt2.x = r * cos(q2) + pte.x;
	pt2.y = r * sin(q2) + pte.y;

	return make_tuple(pt1, pt2);
}

//////////////////////////////////////////////////////////////////////
//	�Q�����Ȃ��p�x�����߂�(��_�����_��ۈ���)

double CalcBetweenAngle(const CPointD& pts, const CPointD& pte)
{
	// pto(�Q���̋��ʓ_)�����_��
	CPointD	pt(pte);
	// pts�̊p�x��pte��̨ݕϊ�
	pt.RoundPoint(-atan2(pts.y, pts.x));
	// pts��0�x�����Ȃ̂�pt2�̊p�x���Q���Ԃ̊p�x(-180�`180���ŕԂ�)
	return atan2(pt.y, pt.x);
}

//////////////////////////////////////////////////////////////////////
//	�̾�ĕ����s�ړ��������������m�̌�_�����߂�
//		k=[1|-1]:�̾�ĕ����w���W��, 0:������W���玩���v�Z

inline CPointD CalcOffsetIntersectionPoint_V
	(const CPointD& pt1, const CPointD& pt2, int k1, int k2, double r)
{
	CPointD	pt;
	// pt1������
	pt.x = _copysign(r, k1==0?pt2.x:k1);		// pt1�̍��E(�})
	if ( fabs(pt2.y) < NCMIN )
		pt.y = _copysign(r, k2==0?pt1.y:k2);	// pt2�̏㉺(�})
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
	// pt1������
	pt.y = _copysign(r, k1==0?pt2.y:k1);		// pt1�̏㉺(�})
	if ( fabs(pt2.x) < NCMIN )
		pt.x = _copysign(r, k2==0?pt1.x:k2);	// pt2�̍��E(�})
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

	// �Q���̐ډ~�v�Z���狁�߂�
	if ( sx < NCMIN || ex < NCMIN ) {
		if ( sx < NCMIN && ex < NCMIN ) {		// �����������H
			if ( k1!=0 && k1==k2 ) {			// �������������Ƃ�����
				pt.x = _copysign(r, k1);
				pt.y = 0;
			}
			else
				bResult = FALSE;
		}
		else {
			pt = sx < NCMIN ?
				CalcOffsetIntersectionPoint_V(pts, pte, k1, k2, r) :
				CalcOffsetIntersectionPoint_V(pte, pts, k2, k1, r);
		}
	}
	else if ( sy < NCMIN || ey < NCMIN ) {
		if ( sy < NCMIN && ey < NCMIN ) {		// �����������H
			if ( k1!=0 && k1==k2 ) {
				pt.x = 0;
				pt.y = _copysign(r, k1);
			}
			else
				bResult = FALSE;
		}
		else {
			pt = sy < NCMIN ?
				CalcOffsetIntersectionPoint_H(pts, pte, k1, k2, r) :
				CalcOffsetIntersectionPoint_H(pte, pts, k2, k1, r);
		}
	}
	else {
		double a1 = pts.y / pts.x;
		double a2 = pte.y / pte.x;
		if ( fabs(a1-a2) > NCMIN ) {	// �X��(��ۏ��Z)����
			double b1 = _copysign(r*sqrt(1+a1*a1), k1==0?(a2-a1)*pte.x:k1);
			double b2 = _copysign(r*sqrt(1+a2*a2), k2==0?(a1-a2)*pts.x:k2);
			pt.x = (b2 - b1) / (a1 - a2);
			pt.y = a1 * pt.x + b1;
		}
		else if ( k1 != 0 ) {
			double	q  = atan2(pts.y, pts.x),
					qq = RoundUp(q*DEG);	// �p�x�̔��f�p
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
//	�̾�ĕ����s�ړ����������Ɖ~�ʂ̌�_�����߂�
//		k=[1|-1]:�̾�ĕ����w���W��, 0:������W���玩���v�Z
//		nRound: CW=1, CCW=-1
//	rr�̕����ƌ�_��Ԃ�

tuple<BOOL, CPointD, double> CalcOffsetIntersectionPoint_LC
	(const CPointD& pts, const CPointD& pto, double ro, double rr, int nRound, int k1, int k2)
{
	CPointD	pt, p1, p2;
	double	a, b, o1, o2, z;
	int		nResult;
	BOOL	bResult = TRUE;

	if ( fabs(pts.x) < NCMIN ) {
		a = 0;
		o1 = pto.y;	o2 = pto.x;
		if ( fabs(pto.y) < NCMIN ) {
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
	else if ( fabs(pts.y) < NCMIN ) {
		a = 0;
		o1 = pto.x;	o2 = pto.y;
		if ( fabs(pto.x) < NCMIN ) {
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
		if ( fabs(pts.x) < NCMIN ) {
			pt.y = p1.x;
			pt.x = b;
		}
		else {
			pt.x = p1.x;
			pt.y = a * pt.x + b;
		}
		break;
	case 2:
		if ( fabs(pts.x) < NCMIN ) {
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
//	�̾�ĕ����s�ړ����������Ƒȉ~�ʂ̌�_�����߂�
//		k=[1|-1]:�̾�ĕ����w���W��, 0:�w��s��
//		nRound: CW=1, CCW=-1
//	�������ł͌v�Z������̂ŁA�����Ƒȉ~�̵̾�Ă��Ɍv�Z���Ă����_�v�Z
//	k1,k2 �������ݒ�ł���
//	�����̵̾�Čv�Z�����̕����ȒP�Ǝv�����ǁA��������������������̂ł��̂܂�

optional<CPointD> CalcOffsetIntersectionPoint_LE
	(const CPointD& pts, const CPointD& pto, double a, double b, double q, double rr,
		BOOL bRound, BOOL bLeft)
{
	int		nResult = 0, k;
	CPointD	pt1, pt2, pr1, pr2;

	// �����̵̾�Ă��v�Z
	tie(pt1, pt2) = CalcOffsetLine(pts, pt1, rr, bLeft);	// pt1=(0,0)
	// �ȉ~�̵̾�Ă��v�Z
	k = bRound ? -1 : 1;	// �����v���Ȃ�ϲŽ�̾��(�����)
	if ( !bLeft )
		k = -k;
	a += rr * k;
	b += rr * k;
	if ( a>0 && b>0 ) {
		// �̾�Č�̒����Ƒȉ~�̌�_
		tie(nResult, pr1, pr2) = CalcIntersectionPoint_LE(pt1, pt2, pto, a, b, q, FALSE);
		if ( nResult>1 && GAPCALC(pr1)>GAPCALC(pr2) )
			pr1 = pr2;
	}

	return nResult > 0 ? pr1 : optional<CPointD>();
}

//////////////////////////////////////////////////////////////////////
//	�Q���������̉������߂�
//	�߂�l�Fint    -> 0:���Ȃ�, 1:�d��, 2:�Q��
//	        double -> ��

tuple<int, double, double>	GetKon(double a, double b, double c)
{
	int		nResult;
	double	x1 = 0.0, x2 = 0.0;

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
		double d = b*b - 4.0*c;
		if ( d > NCMIN ) {
			d = sqrt(d);
			x1 = (b > 0.0 ? (-b - d) : (-b + d)) / 2.0;
			x2 = c / x1;
			nResult = 2;
		}
		else if ( fabs(d) < NCMIN ) {
			x1 = x2 = -b / 2.0;
			nResult = 1;
		}
		else
			nResult = 0;
	}
	return make_tuple(nResult, x1, x2);
}

//////////////////////////////////////////////////////////////////////
// �������z����f����Ԃ�

UINT GetPrimeNumber(UINT nNum)
{
	UINT	i = 0;		// �����l
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
