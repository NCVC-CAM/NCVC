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

using namespace std;
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
	optional<CPointD>	pt1, pt2;
	double	minX1, minY1, maxX1, maxY1,
			minX2, minY2, maxX2, maxY2,
			xa = RoundUp(pte1.x - pts1.x),	ya = RoundUp(pte1.y - pts1.y),
			xb = RoundUp(pte2.x - pts2.x),	yb = RoundUp(pte2.y - pts2.y);

	// �v�Z�O����
	tie(minX1, maxX1) = minmax(pts1.x, pte1.x);		// boost/algorithm
	tie(minY1, maxY1) = minmax(pts1.y, pte1.y);
	tie(minX2, maxX2) = minmax(pts2.x, pte2.x);
	tie(minY2, maxY2) = minmax(pts2.y, pte2.y);

	// ��_�v�Z
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
				pt1 = pt2 = pt;		// ��а�ŗL��
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
				// �͈͓��ɂ��钆�_����_�Ƃ���
				ya = (*pt1).y;	yb = (*pt2).y;
				pt.x = pts1.x;
				pt.y = ( yb - ya ) / 2.0 + ya;
				return pt;	// ���Z�̕K�v�Ȃ�
			}
			else {
				// �[�_���߂��ꍇ�ȂǁA�Ȃ�ׂ�������Ԃ�
				// RoundCt() ���g�p�� <= NCMIN �ŏ����ɘa
				if ( RoundCt(fabs(pts2.y-pts1.y))<=NCMIN || RoundCt(fabs(pte2.y-pts1.y))<=NCMIN )
					return pts1;
				if ( RoundCt(fabs(pts2.y-pte1.y))<=NCMIN || RoundCt(fabs(pte2.y-pte1.y))<=NCMIN )
					return pte1;
			}
		}
		// ��L�����ȊO�ŗ����Ƃ��������ł͉��Ȃ�
	}
	else {
		if ( fabs(xb) < NCMIN ) {
			pt.x = pts2.x;
			pt.y = ya * (pts2.x - pts1.x) / xa + pts1.y;
			bResult = TRUE;
		}
		else {
			double	yaxa = RoundUp(ya / xa),
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
						pt.x = ( xb - xa ) / 2.0 + xa;
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
				// ��L�����ȊO�ŗ����Ƃ��������ł͉��Ȃ�
			}
			else {
				if ( fabs(yb) < NCMIN ) {
					pt.x = (pts2.y - pts1.y) / yaxa + pts1.x;
					pt.y = pts2.y;
					bResult = TRUE;
				}
				else {
					if ( fabs(yaxa-ybxb) < NCMIN ) {
						// �����E�����ȊO�ŌX��������(��ۏ��Z�h�~)�Ƃ�
						if ( bRangeChk ) {
							// ����̍��W�͈͓��ɂ��邩�ǂ���
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
							// ���̒����ƌX�������������������Ă���
							xa = RoundUp((*pt2).x - (*pt1).x);
							ya = RoundUp((*pt2).y - (*pt1).y);
							if ( fabs(xa)>=NCMIN && RoundCt(fabs(ya/xa-yaxa))<=NCMIN ) {
								xa = (*pt1).x;	xb = (*pt2).x;
								ya = (*pt1).y;	yb = (*pt2).y;
								pt.x = ( xb - xa ) / 2.0 + xa;
								pt.y = ( yb - ya ) / 2.0 + ya;
								return pt;
							}
						}
						// else �ɂ���Ɓu�X�������������v�̋U������������Ȃ�
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

	// ���Z(�͈�����)
	if ( bRangeChk ) {
		// ���P
		if ( bResult ) {
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
		// ���Q	
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

	return bResult ? pt : optional<CPointD>();
}

//////////////////////////////////////////////////////////////////////
//	�����Ɖ~�̌�_�����߂�

tuple<int, CPointD, CPointD> CalcIntersectionPoint_LC
	(const CPointD& pts, const CPointD& pte, const CPointD& ptc, double r,
		BOOL bRangeChk/*=TRUE*/)
{
	r = fabs(r);

	int		nResult = 0;
	CPointD	pr1, pr2,
			pt(pte-pts), pto(ptc-pts);	// �n�_�𒆐S��
	double	q = atan2(pt.y, pt.x);		// ���̌X��
	pto.RoundPoint(-q);					// �~�̒��S����̌X�����t��]
	double	y  = fabs(pto.y),			// �~�̒��S��X��(����)�Ƃ̋���
			yr = RoundUp(y-r);

	// ��_���Ȃ�����
	if ( yr > NCMIN )
		return make_tuple(nResult, pr1, pr2);

	pt.RoundPoint(-q);	// pt���K����������

	if ( fabs(yr) <= NCMIN ) {
		// �ڂ���
		pr1.x = pto.x;
		// �͈�����
		nResult = ( bRangeChk && (pr1.x<0 || pt.x<pr1.x) ) ? 0 : 1;
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
				swap(pr1, pr2);	// pr1�̉����̗p���Ȃ�(����ł͐ڐ��Ƌ�ʕt���Ȃ�)
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
			r1r2p = r1 + r2,		// �g�p�p�x�̍����v�Z
			r1r2n = fabs(r1 - r2),
			q = atan2(pt.y, pt.x);	// ��]�p�x

	// ����(pte)�̒��S���W��x����ɂȂ�悤�t��]
	pt.RoundPoint(-q);
	l = RoundUp(fabs(pt.x));	// �Q�~�̒��S�̋���

	// ��_���Ȃ�����(���S�~{�������}, �Q�~�̔��a��苗�����傫��, ������
	if ( l < NCMIN || l > RoundUp(r1r2p)+NCMIN || l < RoundUp(r1r2n)-NCMIN )
		return make_tuple(nResult, pr1, pr2);

	// ��_�v�Z
	if ( RoundCt(fabs(l-r1r2p)) <= NCMIN || RoundCt(fabs(l-r1r2n)) <= NCMIN ) {
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
	double	xa = pt2.x - pt1.x,		ya = pt2.y - pt1.y,
			minX, minY, maxX, maxY;
	tie(minX, maxX) = minmax(pt1.x, pt2.x);
	tie(minY, maxY) = minmax(pt1.y, pt2.y);

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
				swap(pr1, pr2);
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
				swap(pr1, pr2);
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
	int		k = bLeft ? 1 : -1;	// ��:+, �E:- 90��
	// �n�_�����_�ɂ����X��+90��
	double	q = atan2(pte.y-pts.y, pte.x-pts.x) + RAD(90.0) * k,
			cos_q = r * cos(q),
			sin_q = r * sin(q);

	return make_tuple(
		CPointD(cos_q+pts.x, sin_q+pts.y),	// �n�_���̵̾���߲��
		CPointD(cos_q+pte.x, sin_q+pte.y)	// �I�_���̵̾���߲��
	);
}

//////////////////////////////////////////////////////////////////////
//	�Q�����Ȃ��p�x�����߂�(��_�����_��ۈ���)

double CalcBetweenAngle(const CPointD& pts, const CPointD& pte)
{
	// pto(�Q���̋��ʓ_)�����_��
	CPointD	pt(pte);
	// pts�̊p�x��pte��̨ݕϊ�
	pt.RoundPoint(-atan2(pts.y, pts.x));
	// pts��0�x�����Ȃ̂�pt�̊p�x���Q���Ԃ̊p�x(-180�`180���ŕԂ�)
	return atan2(pt.y, pt.x);
}

//////////////////////////////////////////////////////////////////////
//	�̾�ĕ����s�ړ��������������m�̌�_�����߂�

optional<CPointD> CalcOffsetIntersectionPoint_LL
	(const CPointD& pts, const CPointD& pte, double t1, double t2, BOOL bLeft)
{
	CPointD	pt1s, pt1e, pt2s, pt2e, pt;

	tie(pt1s, pt1e) = CalcOffsetLine(pts, pt, t1, bLeft);
	tie(pt2s, pt2e) = CalcOffsetLine(pt, pte, t2, bLeft);

	return CalcIntersectionPoint_LL(pt1s, pt1e, pt2s, pt2e, FALSE);
}

//////////////////////////////////////////////////////////////////////
//	�̾�ĕ����s�ړ����������Ɖ~�ʂ̌�_�����߂�
//		bRound: CW=FALSE, CCW=TRUE

optional<CPointD> CalcOffsetIntersectionPoint_LC
	(const CPointD& pts, const CPointD& pto, double ro, double t1, double t2, BOOL bRound, BOOL bLeft)
{
	int		nResult = 0, k;
	CPointD	pt1, pt2, pr1, pr2;

	// �����̵̾�Ă��v�Z
	tie(pt1, pt2) = CalcOffsetLine(pts, CPointD(), t1, bLeft);
	// �~�̵̾�Ă��v�Z
	k = bRound ? -1 : 1;	// �����v���Ȃ�ϲŽ�̾��(�����)
	if ( !bLeft )
		k = -k;
	ro += t2 * k;
	if ( ro > 0 ) {
		// �̾�Č�̒����Ɖ~�̌�_
		tie(nResult, pr1, pr2) = CalcIntersectionPoint_LC(pt1, pt2, pto, ro, FALSE);
		if ( nResult>1 && GAPCALC(pr1)>GAPCALC(pr2) )
			pr1 = pr2;
	}

	return nResult > 0 ? pr1 : optional<CPointD>();
}

//////////////////////////////////////////////////////////////////////
//	�̾�ĕ����s�ړ����������Ƒȉ~�ʂ̌�_�����߂�
//		bRound: CW=FALSE, CCW=TRUE

optional<CPointD> CalcOffsetIntersectionPoint_LE
	(const CPointD& pts, const CPointD& pto, double a, double b, double q, double rr,
		BOOL bRound, BOOL bLeft)
{
	int		nResult = 0, k;
	CPointD	pt1, pt2, pr1, pr2;

	// �����̵̾�Ă��v�Z
	tie(pt1, pt2) = CalcOffsetLine(pts, CPointD(), rr, bLeft);
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
//	�_�����p�`�̓������ۂ�

BOOL IsPointInPolygon(const CPointD& ptTarget, const vector<CPointD>& pt)
{
	int		iCountCrossing = 0;
	UINT	ui, uiCountPoint = pt.size();
	CPointD	pt0(pt[0]), pt1;
	BOOL	bFlag0x = (ptTarget.x <= pt0.x), bFlag1x,
			bFlag0y = (ptTarget.y <= pt0.y), bFlag1y;

	// ���C�̕����́A�w�v���X����
	for( ui=1; ui<uiCountPoint+1; ui++ ) {
		pt1 = pt[ui%uiCountPoint];	// �Ō�͎n�_������i���p�`�f�[�^�̎n�_�ƏI�_����v���Ă��Ȃ��f�[�^�Ή��j
		bFlag1x = (ptTarget.x <= pt1.x);
		bFlag1y = (ptTarget.y <= pt1.y);
		if( bFlag0y != bFlag1y ) {
			// �����̓��C�����؂�\������B
			if( bFlag0x == bFlag1x ) {
				// �����̂Q�[�_�͑Ώۓ_�ɑ΂��ė����E���������ɂ���
				if( bFlag0x ) {
					// ���S�ɉE�B�ː����̓��C�����؂�
					iCountCrossing += (bFlag0y ? -1 : 1);	// �ォ�牺�Ƀ��C�����؂�Ƃ��ɂ́A�����񐔂��P�����A�������͂P�����B
				}
			}
			else {
				// ���C�ƌ������邩�ǂ����A�Ώۓ_�Ɠ��������ŁA�Ώۓ_�̉E�Ō������邩�A���Ō������邩�����߂�B
				if( ptTarget.x <= ( pt0.x + (pt1.x - pt0.x) * (ptTarget.y - pt0.y ) / (pt1.y - pt0.y) ) ) {
					// �����́A�Ώۓ_�Ɠ��������ŁA�Ώۓ_�̉E�Ō�������B�ː����̓��C�����؂�
					iCountCrossing += (bFlag0y ? -1 : 1);	// �ォ�牺�Ƀ��C�����؂�Ƃ��ɂ́A�����񐔂��P�����A�������͂P�����B
				}
			}
		}
		// ���̔���̂��߂ɁA
		pt0 = pt1;
		bFlag0x = bFlag1x;
		bFlag0y = bFlag1y;
	}

	// �N���X�J�E���g���[���̂Ƃ��O�A�[���ȊO�̂Ƃ����B
	return (0 != iCountCrossing);
}

//////////////////////////////////////////////////////////////////////
//	�_�ƒ����̋��������߂�

double CalcLineDistancePt(const CPointD& pts, const CPointD& pte, const CPointD& ptc)
{
	CPointD	pt(pte-pts), pto(ptc-pts);
	double	q = atan2(pt.y, pt.x);
	pto.RoundPoint(-q);
	return fabs(pto.y);
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
