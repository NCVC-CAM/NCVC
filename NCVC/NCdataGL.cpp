// NCdataGL.cpp: CNCdata クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NCVC.h"
#include "NCdata.h"
#include "ViewOption.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

using namespace std;
using namespace boost;
extern	const	PENSTYLE	g_penStyle[];		// ViewOption.cpp

//////////////////////////////////////////////////////////////////////
// NCﾃﾞｰﾀの基礎ﾃﾞｰﾀｸﾗｽ
//////////////////////////////////////////////////////////////////////

void CNCdata::DrawGL(void) const
{
}

void CNCdata::DrawBottomFace(void) const
{
}

//////////////////////////////////////////////////////////////////////
// CNCline クラス
//////////////////////////////////////////////////////////////////////

void CNCline::DrawGL(void) const
{
	if ( m_obCdata.GetCount() > 0 ) {
		for ( int i=0; i<m_obCdata.GetSize(); i++ )
			m_obCdata[i]->DrawGL();
		return;
	}
	const CViewOption*	pOpt = AfxGetNCVCApp()->GetViewOption();
	COLORREF	col = pOpt->GetNcDrawColor(GetPenType()+NCCOL_G0);
	::glLineStipple(1, g_penStyle[pOpt->GetNcDrawType(GetLineType())].nGLpattern);
	::glBegin(GL_LINES);
	::glColor3ub( GetRValue(col), GetGValue(col), GetBValue(col) );
	::glVertex3d(m_ptValS.x, m_ptValS.y, m_ptValS.z);
	::glVertex3d(m_ptValE.x, m_ptValE.y, m_ptValE.z);
	::glEnd();
}

void CNCline::SetEndmillPath(CPointD* pt1, CPointD* pt2, CPointD* pt3) const
{
	int		i;
	double	qs = atan2(m_ptValE.y-m_ptValS.y, m_ptValE.x-m_ptValS.x) + 90.0*RAD,
			qe = atan2(m_ptValS.y-m_ptValE.y, m_ptValS.x-m_ptValE.x) - 90.0*RAD;

	// 始点凸面，終点凹面の座標計算
	for ( i=0; i<=ARCCOUNT/2; i++, qs+=ARCSTEP, qe+=ARCSTEP ) {
		// 始点凸側は +90°から反時計回りに
		pt1[i].x = m_dEndmill * cos(qs) + m_ptValS.x;
		pt1[i].y = m_dEndmill * sin(qs) + m_ptValS.y;
		// 終点凹側は -90°から反時計回り
		pt2[i].x = m_dEndmill * cos(qe) + m_ptValE.x;
		pt2[i].y = m_dEndmill * sin(qe) + m_ptValE.y;
	}
	// 終点○面残りの半円座標計算
	for ( i=0; i<ARCCOUNT/2; i++, qe+=ARCSTEP ) {
		pt3[i].x = m_dEndmill * cos(qe) + m_ptValE.x;
		pt3[i].y = m_dEndmill * sin(qe) + m_ptValE.y;
	}
}

void CNCline::DrawBottomFace(void) const
{
	if ( m_nc.nGcode != 1 )
		return;

	int		i;
	CPointD	pt1[ARCCOUNT/2+1], pt2[ARCCOUNT/2+1], pt3[ARCCOUNT/2];

	// 座標計算
	SetEndmillPath(pt1, pt2, pt3);

	if ( m_dMove[NCA_X]>0 || m_dMove[NCA_Y]>0 ) {
		// 底面描画
		::glBegin(GL_TRIANGLE_STRIP);
		for ( i=0; i<=ARCCOUNT/2; i++ ) {
			::glVertex3d(pt2[i].x, pt2[i].y, m_ptValS.z);
			::glVertex3d(pt1[i].x, pt1[i].y, m_ptValE.z);
		}
		::glEnd();
	}

	// 終点○面の描画
	::glBegin(GL_TRIANGLE_FAN);
	::glVertex3d(m_ptValE.x, m_ptValE.y, m_ptValE.z);
	for ( i=0; i<=ARCCOUNT/2; i++ )
		::glVertex3d(pt2[i].x, pt2[i].y, m_ptValE.z);
	for ( i=0; i< ARCCOUNT/2; i++ )
		::glVertex3d(pt3[i].x, pt3[i].y, m_ptValE.z);
	::glVertex3d(pt2[0].x, pt2[0].y, m_ptValE.z);
	::glEnd();
}

//////////////////////////////////////////////////////////////////////
// CNCcycle クラス
//////////////////////////////////////////////////////////////////////

void CNCcycle::DrawGL(void) const
{
	const CViewOption*	pOpt = AfxGetNCVCApp()->GetViewOption();
	COLORREF	colG0 = pOpt->GetNcDrawColor( NCCOL_G0 ),
				colCY = pOpt->GetNcDrawColor( NCCOL_CYCLE );
	BYTE	bG0r = GetRValue(colG0), bG0g = GetGValue(colG0), bG0b = GetBValue(colG0),
			bCYr = GetRValue(colCY), bCYg = GetGValue(colCY), bCYb = GetBValue(colCY);
	::glLineStipple(1, g_penStyle[pOpt->GetNcDrawType(NCCOLLINE_G0)].nGLpattern);
	::glBegin(GL_LINE_STRIP);
	::glColor3ub( bG0r, bG0g, bG0b );
	::glVertex3d(m_ptValS.x, m_ptValS.y, m_ptValS.z);
	::glVertex3d(m_ptValI.x, m_ptValI.y, m_ptValI.z);
	::glVertex3d(m_ptValR.x, m_ptValR.y, m_ptValR.z);
	::glVertex3d(m_ptValE.x, m_ptValE.y, m_ptValE.z);
	::glEnd();
	::glBegin(GL_LINES);
	for ( int i=0; i<m_nDrawCnt; i++ ) {
		::glLineStipple(1, g_penStyle[pOpt->GetNcDrawType(NCCOLLINE_G0)].nGLpattern);
		::glColor3ub( bG0r, bG0g, bG0b );
		::glVertex3d(m_Cycle3D[i].ptI.x, m_Cycle3D[i].ptI.y, m_Cycle3D[i].ptI.z);
		::glVertex3d(m_Cycle3D[i].ptR.x, m_Cycle3D[i].ptR.y, m_Cycle3D[i].ptR.z);
		::glLineStipple(1, g_penStyle[pOpt->GetNcDrawType(NCCOLLINE_CYCLE)].nGLpattern);
		::glColor3ub( bCYr, bCYg, bCYb );
		::glVertex3d(m_Cycle3D[i].ptR.x, m_Cycle3D[i].ptR.y, m_Cycle3D[i].ptR.z);
		::glVertex3d(m_Cycle3D[i].ptC.x, m_Cycle3D[i].ptC.y, m_Cycle3D[i].ptC.z);
	}
	::glEnd();
}

void CNCcycle::SetEndmillPath(const CPointD& ptOrg, CPointD* pt) const
{
	int		i;
	double	q;

	// 円座標をｾｯﾄ（閉じた面を保証）
	pt[0].x = m_dEndmill + ptOrg.x;		// cos(0) == 1
	pt[0].y = ptOrg.y;					// sin(0) == 0
	for ( i=1, q=ARCSTEP; i<ARCCOUNT; i++, q+=ARCSTEP ) {
		pt[i].x = m_dEndmill * cos(q) + ptOrg.x;
		pt[i].y = m_dEndmill * sin(q) + ptOrg.y;
	}
	pt[i] = pt[0];
}

void CNCcycle::DrawBottomFace(void) const
{
	if ( GetPlane() != XY_PLANE )
		return;

	int		i, j;
	CPointD	pt[ARCCOUNT+1];

	for ( i=0; i<m_nDrawCnt; i++ ) {
		// 座標計算
		SetEndmillPath(m_Cycle3D[i].ptC, pt);
		// 底面描画
		::glBegin(GL_TRIANGLE_FAN);
		::glVertex3d(m_Cycle3D[i].ptC.x, m_Cycle3D[i].ptC.y, m_Cycle3D[i].ptC.z);	// 中心
		for	( j=0; j<=ARCCOUNT; j++ )
			::glVertex3d(pt[j].x, pt[j].y, m_Cycle3D[i].ptC.z);
		::glEnd();
	}
}

//////////////////////////////////////////////////////////////////////
// CNCcircle クラス
//////////////////////////////////////////////////////////////////////

void CNCcircle::DrawGL(void) const
{
	if ( m_obCdata.GetCount() > 0 ) {
		for ( int i=0; i<m_obCdata.GetSize(); i++ )
			m_obCdata[i]->DrawGL();
		return;
	}

	double		sq, eq, r = fabs(m_r);
	CPoint3D	pt;

	if ( m_nG23 == 0 ) {
		sq = m_eq;
		eq = m_sq;
	}
	else {
		sq = m_sq;
		eq = m_eq;
	}

	const CViewOption*	pOpt = AfxGetNCVCApp()->GetViewOption();
	COLORREF	col = pOpt->GetNcDrawColor(NCCOL_G1);
	::glLineStipple(1, g_penStyle[pOpt->GetNcDrawType(NCCOLLINE_G1)].nGLpattern);
	::glBegin(GL_LINE_STRIP);
	::glColor3ub( GetRValue(col), GetGValue(col), GetBValue(col) );

	switch ( GetPlane() ) {
	case XY_PLANE:
		pt.z = m_ptValS.z;	// ﾍﾘｶﾙ開始座標
		// ARCSTEP づつ微細線分で描画
		if ( m_nG23 == 0 ) {
			for ( ; sq>eq; sq-=ARCSTEP, pt.z+=m_dHelicalStep ) {
				pt.x = r * cos(sq) + m_ptOrg.x;
				pt.y = r * sin(sq) + m_ptOrg.y;
				::glVertex3d(pt.x, pt.y, pt.z);
			}
		}
		else {
			for ( ; sq<eq; sq+=ARCSTEP, pt.z+=m_dHelicalStep ) {
				pt.x = r * cos(sq) + m_ptOrg.x;
				pt.y = r * sin(sq) + m_ptOrg.y;
				::glVertex3d(pt.x, pt.y, pt.z);
			}
		}
		// 端数分描画
		pt.x = r * cos(eq) + m_ptOrg.x;
		pt.y = r * sin(eq) + m_ptOrg.y;
		pt.z = m_ptValE.z;		// ﾍﾘｶﾙ終了座標
		::glVertex3d(pt.x, pt.y, pt.z);
		break;

	case XZ_PLANE:
		pt.y = m_ptValS.y;
		if ( m_nG23 == 0 ) {
			for ( ; sq>eq; sq-=ARCSTEP, pt.y+=m_dHelicalStep ) {
				pt.x = r * cos(sq) + m_ptOrg.x;
				pt.z = r * sin(sq) + m_ptOrg.z;
				::glVertex3d(pt.x, pt.y, pt.z);
			}
		}
		else {
			for ( ; sq<eq; sq+=ARCSTEP, pt.y+=m_dHelicalStep ) {
				pt.x = r * cos(sq) + m_ptOrg.x;
				pt.z = r * sin(sq) + m_ptOrg.z;
				::glVertex3d(pt.x, pt.y, pt.z);
			}
		}
		pt.x = r * cos(eq) + m_ptOrg.x;
		pt.y = m_ptValE.y;
		pt.z = r * sin(eq) + m_ptOrg.z;
		::glVertex3d(pt.x, pt.y, pt.z);
		break;

	case YZ_PLANE:
		pt.x = m_ptValS.x;
		if ( m_nG23 == 0 ) {
			for ( ; sq>eq; sq-=ARCSTEP, pt.x+=m_dHelicalStep ) {
				pt.y = r * cos(sq) + m_ptOrg.y;
				pt.z = r * sin(sq) + m_ptOrg.z;
				::glVertex3d(pt.x, pt.y, pt.z);
			}
		}
		else {
			for ( ; sq<eq; sq+=ARCSTEP, pt.x+=m_dHelicalStep ) {
				pt.y = r * cos(sq) + m_ptOrg.y;
				pt.z = r * sin(sq) + m_ptOrg.z;
				::glVertex3d(pt.x, pt.y, pt.z);
			}
		}
		pt.x = m_ptValE.x;
		pt.y = r * cos(eq) + m_ptOrg.y;
		pt.z = r * sin(eq) + m_ptOrg.z;
		::glVertex3d(pt.x, pt.y, pt.z);
		break;
	}

	::glEnd();
}

inline void SetEndmillPath_XY_Path
	(const CPointD& pt, double q, double h, double r1, double r2,
		vector<CPoint3D>& vt1, vector<CPoint3D>& vt2)
{
	double	cos_q = cos(q);
	double	sin_q = sin(q);
	vt1.push_back( CPoint3D(
		r1 * cos_q + pt.x,
		r1 * sin_q + pt.y,
		h )
	);
	vt2.push_back( CPoint3D(
		r2 * cos_q + pt.x,
		r2 * sin_q + pt.y,
		h )
	);
}

inline void SetEndmillPath_XZ_Path
	(const CPointD& pt, double q, double rr, double r1, double r2, double d,
		vector<CPoint3D>& vt1, vector<CPoint3D>& vt2)
{
	CPoint3D	pt1, pt2;
	double	cos_q = cos(q);
	double	sin_q = sin(q);
	pt1.x = rr * cos_q;
	if ( cos_q > 0 )
		pt1.x = max(pt1.x-d, 0) + pt.x;
	else
		pt1.x = min(pt1.x+d, 0) + pt.x;
	pt1.y = r1;		// h + r1
	pt1.z = rr * sin_q + pt.y;
	pt2.x = pt1.x;
	pt2.y = r2;		// h + r2
	pt2.z = pt1.z;
	vt1.push_back(pt1);
	vt2.push_back(pt2);
}

inline void SetEndmillPath_YZ_Path
	(const CPointD& pt, double q, double rr, double r1, double r2, double d,
		vector<CPoint3D>& vt1, vector<CPoint3D>& vt2)
{
	CPoint3D	pt1, pt2;
	double	cos_q = cos(q);
	double	sin_q = sin(q);
	pt1.x = r1;		// h + r1
	pt1.y = rr * cos_q;
	if ( cos_q > 0 )
		pt1.y = max(pt1.y-d, 0) + pt.x;
	else
		pt1.y = min(pt1.y+d, 0) + pt.x;
	pt1.z = rr * sin_q + pt.y;
	pt2.x = r2;		// h + r2
	pt2.y = pt1.y;
	pt2.z = pt1.z;
	vt1.push_back(pt1);
	vt2.push_back(pt2);
}

inline double SetEndmillPath_XZYZ_Start
	(const CPoint3D& pts, const CPoint3D& pte, double d,
		vector<CPoint3D>& vt1, vector<CPoint3D>& vt2)
{
	double	h = atan2(pte.y-pte.y, pte.x-pts.x) + 90.0*RAD,	// ﾍﾘｶﾙの傾き
			hResult = h,
			cos_q = d * cos(h),
			sin_q = d * sin(h);
	CPoint3D	pt1, pt2;
	pt1.x = cos_q + pts.x;
	pt1.y = sin_q + pts.y;
	pt1.z = pts.z;
	pt2.x = -cos_q + pts.x;	// 180°反転
	pt2.y = -sin_q + pts.y;
	pt2.z = pts.z;
	vt1.push_back(pt1);
	vt2.push_back(pt2);
	h -= 90.0*RAD;
	cos_q = d * cos(h);
	sin_q = d * sin(h);
	pt1.x += cos_q;
	pt1.y += sin_q;
	pt2.x += cos_q;
	pt2.y += sin_q;
	vt1.push_back(pt1);
	vt2.push_back(pt2);

	return hResult;
}

void CNCcircle::SetEndmillPath
	(vector<CPoint3D>& vt1, vector<CPoint3D>& vt2, CPointD* pts, CPointD* pte) const
{
	int			i;
	double		sq, eq, h, hs, cos_q, sin_q, r1, r2, rr = fabs(m_r);
	CPointD		ptOrg(GetPlaneValue(m_ptOrg));
	CPoint3D	pt1, pt2;

	if ( m_nG23 == 0 ) {
		sq = m_eq;
		eq = m_sq;
	}
	else {
		sq = m_sq;
		eq = m_eq;
	}

	// 円弧補間切削ﾊﾟｽ座標
	switch ( GetPlane() ) {
	case XY_PLANE:
		if ( m_nG23 == 0 ) {
			r1 = rr + m_dEndmill;	// 進行方向左側
			r2 = rr - m_dEndmill;	// 進行方向右側
			for ( h=m_ptValS.z; sq>eq; sq-=ARCSTEP, h+=m_dHelicalStep )
				SetEndmillPath_XY_Path(ptOrg, sq, h, r1, r2, vt1, vt2);
		}
		else {
			r1 = rr - m_dEndmill;
			r2 = rr + m_dEndmill;
			for ( h=m_ptValS.z; sq<eq; sq+=ARCSTEP, h+=m_dHelicalStep )
				SetEndmillPath_XY_Path(ptOrg, sq, h, r1, r2, vt1, vt2);
		}
		// 端数分
		SetEndmillPath_XY_Path(ptOrg, eq, m_ptValE.z, r1, r2, vt1, vt2);
		break;

	case XZ_PLANE:
		if ( m_nG23 == 0 ) {
			r1 = m_dEndmill;
			r2 = m_dEndmill * -1.0;
		}
		else {
			r1 = m_dEndmill * -1.0;
			r2 = m_dEndmill;
		}
		// 始点○とかぶる部分の矩形
		hs = SetEndmillPath_XZYZ_Start(m_ptValS, m_ptValE, m_dEndmill, vt1, vt2);
		// 移動ﾊﾟｽ
		if ( m_nG23 == 0 ) {
			for ( h=m_ptValS.y+m_dHelicalStep, sq-=ARCSTEP; sq>eq; sq-=ARCSTEP, h+=m_dHelicalStep )
				SetEndmillPath_XZ_Path(ptOrg, sq, rr, h+r1, h+r2, m_dEndmill, vt1, vt2);
		}
		else {
			for ( h=m_ptValS.y+m_dHelicalStep, sq+=ARCSTEP; sq<eq; sq+=ARCSTEP, h+=m_dHelicalStep )
				SetEndmillPath_XZ_Path(ptOrg, sq, rr, h+r1, h+r2, m_dEndmill, vt1, vt2);
		}
		SetEndmillPath_XZ_Path(ptOrg, eq, rr, m_ptValE.y+r1, m_ptValE.y+r2, m_dEndmill, vt1, vt2);
		break;
	
	case YZ_PLANE:
		if ( m_nG23 == 0 ) {
			r1 = m_dEndmill * -1.0;
			r2 = m_dEndmill;
		}
		else {
			r1 = m_dEndmill;
			r2 = m_dEndmill * -1.0;
		}
		hs = SetEndmillPath_XZYZ_Start(m_ptValS, m_ptValE, m_dEndmill, vt1, vt2);
		if ( m_nG23 == 0 ) {
			for ( h=m_ptValS.x+m_dHelicalStep, sq-=ARCSTEP; sq>eq; sq-=ARCSTEP, h+=m_dHelicalStep )
				SetEndmillPath_YZ_Path(ptOrg, sq, rr, h+r1, h+r2, m_dEndmill, vt1, vt2);
		}
		else {
			for ( h=m_ptValS.x+m_dHelicalStep, sq+=ARCSTEP; sq<eq; sq+=ARCSTEP, h+=m_dHelicalStep )
				SetEndmillPath_YZ_Path(ptOrg, sq, rr, h+r1, h+r2, m_dEndmill, vt1, vt2);
		}
		SetEndmillPath_YZ_Path(ptOrg, eq, rr, m_ptValE.x+r1, m_ptValE.x+r2, m_dEndmill, vt1, vt2);
		break;
	}

	// 終点の○とかぶる部分の矩形(XZ_PLANE, YZ_PLANE)
	if ( GetPlane() != XY_PLANE ) {
		cos_q = m_dEndmill * cos(hs);
		sin_q = m_dEndmill * sin(hs);
		pt1.x = cos_q + m_ptValE.x;
		pt1.y = sin_q + m_ptValE.y;
		pt1.z = m_ptValE.z;
		pt2.x = -cos_q + m_ptValE.x;	// 180°反転
		pt2.y = -sin_q + m_ptValE.y;
		pt2.z = m_ptValE.z;
	}
	vt1.push_back(pt1);
	vt2.push_back(pt2);

	// 始点側○
	for ( i=0, sq=0; i<ARCCOUNT; i++, sq+=ARCSTEP ) {
		pts[i].x = m_dEndmill * cos(sq) + m_ptValS.x;
		pts[i].y = m_dEndmill * sin(sq) + m_ptValS.y;
	}
	pts[i] = pts[0];
	// 終点側○
	for ( i=0, eq=0; i<ARCCOUNT; i++, eq+=ARCSTEP ) {
		pte[i].x = m_dEndmill * cos(eq) + m_ptValE.x;
		pte[i].y = m_dEndmill * sin(eq) + m_ptValE.y;
	}
	pte[i] = pte[0];
}

void CNCcircle::DrawBottomFace(void) const
{
	size_t	i;
	vector<CPoint3D>	vt1, vt2;
	CPointD	pts[ARCCOUNT+1], pte[ARCCOUNT+1];

	// 座標計算
	SetEndmillPath(vt1, vt2, pts, pte);

	// 始点・終点のｴﾝﾄﾞﾐﾙ円
	// --始点側
	::glBegin(GL_TRIANGLE_FAN);
	::glVertex3d(m_ptValS.x, m_ptValS.y, m_ptOrg.z);
	for ( i=0; i<=ARCCOUNT; i++ )
		::glVertex3d(pts[i].x, pts[i].y, m_ptOrg.z);
	::glEnd();
	// --終点側
	::glBegin(GL_TRIANGLE_FAN);
	::glVertex3d(m_ptValE.x, m_ptValE.y, m_ptValE.z);
	for ( i=0; i<=ARCCOUNT; i++ )
		::glVertex3d(pte[i].x, pte[i].y, m_ptValE.z);
	::glEnd();

	// 底面（ｴﾝﾄﾞﾐﾙ中心から長方形ﾊﾟｽ）
	::glBegin(GL_TRIANGLE_STRIP);
	for ( i=0; i<vt1.size(); i++ ) {
		::glVertex3d(vt1[i].x, vt1[i].y, vt1[i].z);
		::glVertex3d(vt2[i].x, vt2[i].y, vt2[i].z);
	}
	::glEnd();
}
