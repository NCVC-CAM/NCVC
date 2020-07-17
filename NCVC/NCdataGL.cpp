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
extern	const	PENSTYLE	g_penStyle[];		// ViewOption.cpp

//////////////////////////////////////////////////////////////////////
//	OpenGL 描画共通
//////////////////////////////////////////////////////////////////////

void DrawCylinder(BOOL bCulling, const CPoint3D& pt, double z, double r)
{
	double	q;
	int		i;
	CPointD	vt[ARCCOUNT+1];

	// 円座標をｾｯﾄ（閉じた面を保証）
	vt[0].x = r + pt.x;		// cos(0) == 1
	vt[0].y = pt.y;			// sin(0) == 0
	for ( i=1, q=ARCSTEP; i<ARCCOUNT; i++, q+=ARCSTEP ) {
		vt[i].x = r * cos(q) + pt.x;
		vt[i].y = r * sin(q) + pt.y;
	}
	vt[i] = vt[0];

	if ( bCulling ) {
		// 遮蔽問合せのため底面描画のみ
		// かつ法線ﾍﾞｸﾄﾙは上向き
		::glBegin(GL_TRIANGLE_FAN);
		::glNormal3d(0.0, 0.0, 1.0);
		::glVertex3d(pt.x, pt.y, pt.z);		// 中心
		for	( i=0; i<=ARCCOUNT; i++ )
			::glVertex3d(vt[i].x, vt[i].y, pt.z);
		::glEnd();
	}
	else {
		// 上面
		::glBegin(GL_TRIANGLE_FAN);
		::glNormal3d(0.0, 0.0, 1.0);
		::glVertex3d(pt.x, pt.y, z);
		for	( i=0; i<=ARCCOUNT; i++ )
			::glVertex3d(vt[i].x, vt[i].y, z);
		::glEnd();
		// 底面
		::glBegin(GL_TRIANGLE_FAN);
		::glNormal3d(0.0, 0.0, -1.0);
		::glVertex3d(pt.x, pt.y, pt.z);
		for ( i=ARCCOUNT; i>=0; i-- )
			::glVertex3d(vt[i].x, vt[i].y, pt.z);
		::glEnd();
		// 側面
		::glBegin(GL_QUAD_STRIP);
		for ( i=0; i<=ARCCOUNT; i++ ) {
			::glNormal3d(vt[i].x, vt[i].y, 0.0);
			::glVertex3d(vt[i].x, vt[i].y, z);
			::glVertex3d(vt[i].x, vt[i].y, pt.z);
		}
		::glEnd();
	}
}

//////////////////////////////////////////////////////////////////////
// NCﾃﾞｰﾀの基礎ﾃﾞｰﾀｸﾗｽ
//////////////////////////////////////////////////////////////////////

void CNCdata::DrawGL(BOOL) const
{
}

void CNCdata::DrawMill(void) const
{
}

void CNCdata::CreateOcclusionCulling(const CNCdata*)
{
	m_glList = 0;
}

void CNCdata::CreateMillList(double)
{
}

//////////////////////////////////////////////////////////////////////
// CNCline クラス
//////////////////////////////////////////////////////////////////////

void CNCline::DrawGL(BOOL bG00) const
{
	if ( m_obCdata.GetCount() > 0 ) {
		for ( int i=0; i<m_obCdata.GetSize(); i++ )
			m_obCdata[i]->DrawGL(bG00);
		return;
	}
	if ( bG00 && m_nc.nGcode == 1 )
		return;
	const CViewOption*	pOpt = AfxGetNCVCApp()->GetViewOption();
	COLORREF	col = pOpt->GetNcDrawColor(GetPenType()+NCCOL_G0);
	::glLineStipple(1, g_penStyle[pOpt->GetNcDrawType(GetLineType())].nGLpattern);
	::glBegin(GL_LINES);
	::glColor3ub( GetRValue(col), GetGValue(col), GetBValue(col) );
	::glVertex3d(m_ptValS.x, m_ptValS.y, m_ptValS.z);
	::glVertex3d(m_ptValE.x, m_ptValE.y, m_ptValE.z);
	::glEnd();
}

void CNCline::DrawMill(void) const
{
//	if ( m_glList > 0 )
		::glCallList(m_glList);		// 無けりゃ無視なので if() 必要なし
}

void CNCline::CreateOcclusionCulling(const CNCdata* pDataNext)
{
	if ( m_nc.nGcode == 1 ) {
		// 遮蔽問い合わせのため、底面だけ描画
		m_glList = ::glGenLists(1);
		if ( m_glList > 0 ) {
			::glNewList(m_glList, GL_COMPILE);
			// 切削軌跡
			if ( m_dMove[NCA_X]>0 || m_dMove[NCA_Y]>0 )
				SolidPath(TRUE, 0.0);		// Z値はdummy
			// 終点のｴﾝﾄﾞﾐﾙ形状
			if ( !pDataNext->IsCutter() )
				DrawCylinder(TRUE, m_ptValE, 0.0, m_dEndmill);
			::glEndList();
		}
	}
	else
		m_glList = 0;
}

void CNCline::CreateMillList(double z)
{
	m_glList = ::glGenLists(1);
	if ( m_glList > 0 ) {
		::glNewList(m_glList, GL_COMPILE);
		if ( m_dMove[NCA_X]>0 || m_dMove[NCA_Y]>0 )
			SolidPath(FALSE, z);
		DrawCylinder(FALSE, m_ptValE, z, m_dEndmill);
		::glEndList();
	}
}

void CNCline::SolidPath(BOOL bCulling, double z) const
{
	int		i;
	double	qs = atan2(m_ptValE.y-m_ptValS.y, m_ptValE.x-m_ptValS.x) - 90.0*RAD,
			qe = atan2(m_ptValS.y-m_ptValE.y, m_ptValS.x-m_ptValE.x) + 90.0*RAD;
	CPointD	vt1[ARCCOUNT/2+1], vt2[ARCCOUNT/2+1];

	// 始点凸面，終点凹面の座標計算
	for ( i=0; i<=ARCCOUNT/2; i++, qs-=ARCSTEP, qe-=ARCSTEP ) {
		// 始点凸側は -90°から反時計回りに
		vt1[i].x = m_dEndmill * cos(qs) + m_ptValS.x;
		vt1[i].y = m_dEndmill * sin(qs) + m_ptValS.y;
		// 終点凹側は +90°から反時計回り
		vt2[i].x = m_dEndmill * cos(qe) + m_ptValE.x;
		vt2[i].y = m_dEndmill * sin(qe) + m_ptValE.y;
	}

	if ( bCulling ) {
		// 遮蔽問合せのため底面描画のみ
		// かつ法線ﾍﾞｸﾄﾙは上向き
		::glBegin(GL_QUAD_STRIP);
		::glNormal3d(0.0, 0.0, 1.0);
		for ( i=0; i<=ARCCOUNT/2; i++ ) {
			::glVertex3d(vt1[i].x, vt1[i].y, m_ptValS.z);
			::glVertex3d(vt2[i].x, vt2[i].y, m_ptValE.z);
		}
		::glEnd();
	}
	else {
		::glBegin(GL_QUADS);
		// 進行方向左側側面（反時計回りに繋げる）
		::glNormal3d(vt1[0].x, vt1[0].y, 0.0);
		::glVertex3d(vt1[0].x, vt1[0].y, m_ptValS.z);	// 左下
		::glVertex3d(vt2[0].x, vt2[0].y, m_ptValE.z);	// 右下
		::glVertex3d(vt2[0].x, vt2[0].y, z);			// 右上
		::glVertex3d(vt1[0].x, vt1[0].y, z);			// 左上
		// 進行方向右側側面（裏から見て反時計回り）
		::glNormal3d(vt2[ARCCOUNT/2].x, vt2[ARCCOUNT/2].y, 0.0);
		::glVertex3d(vt2[ARCCOUNT/2].x, vt2[ARCCOUNT/2].y, m_ptValE.z);	// 左下
		::glVertex3d(vt1[ARCCOUNT/2].x, vt1[ARCCOUNT/2].y, m_ptValS.z);	// 右下
		::glVertex3d(vt1[ARCCOUNT/2].x, vt1[ARCCOUNT/2].y, z);			// 右上
		::glVertex3d(vt2[ARCCOUNT/2].x, vt2[ARCCOUNT/2].y, z);			// 左上
		::glEnd();
		// 天井
		::glBegin(GL_QUAD_STRIP);
		::glNormal3d(0.0, 0.0, 1.0);
		for ( i=0; i<=ARCCOUNT/2; i++ ) {
			::glVertex3d(vt1[i].x, vt1[i].y, z);
			::glVertex3d(vt2[i].x, vt2[i].y, z);
		}
		::glEnd();
		// 底面
		::glBegin(GL_QUAD_STRIP);
		::glNormal3d(0.0, 0.0, -1.0);
		for ( i=ARCCOUNT/2; i>=0; i-- ) {
			::glVertex3d(vt1[i].x, vt1[i].y, m_ptValS.z);
			::glVertex3d(vt2[i].x, vt2[i].y, m_ptValE.z);
		}
		::glEnd();
		// 始点凸側面
		::glBegin(GL_QUAD_STRIP);
		for ( i=ARCCOUNT/2; i>=0; i-- ) {
			::glNormal3d(vt1[i].x, vt1[i].y, 0.0);
			::glVertex3d(vt1[i].x, vt1[i].y, z);
			::glVertex3d(vt1[i].x, vt1[i].y, m_ptValS.z);
		}
		::glEnd();
		// 終点凹側面
		::glBegin(GL_QUAD_STRIP);
		for ( i=0; i<=ARCCOUNT/2; i++ ) {
			::glNormal3d(vt2[i].x, vt2[i].y, 0.0);
			::glVertex3d(vt2[i].x, vt2[i].y, z);
			::glVertex3d(vt2[i].x, vt2[i].y, m_ptValE.z);
		}
		::glEnd();
	}
}

//////////////////////////////////////////////////////////////////////
// CNCcycle クラス
//////////////////////////////////////////////////////////////////////

void CNCcycle::DrawGL(BOOL bG00) const
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
		if ( !bG00 ) {
			::glLineStipple(1, g_penStyle[pOpt->GetNcDrawType(NCCOLLINE_CYCLE)].nGLpattern);
			::glColor3ub( bCYr, bCYg, bCYb );
			::glVertex3d(m_Cycle3D[i].ptR.x, m_Cycle3D[i].ptR.y, m_Cycle3D[i].ptR.z);
			::glVertex3d(m_Cycle3D[i].ptC.x, m_Cycle3D[i].ptC.y, m_Cycle3D[i].ptC.z);
		}
	}
	::glEnd();
}

void CNCcycle::DrawMill(void) const
{
//	if ( m_glList > 0 )
		::glCallList(m_glList);
}

void CNCcycle::CreateOcclusionCulling(const CNCdata*)
{
	m_glList = ::glGenLists(1);
	if ( m_glList > 0 ) {
		::glNewList(m_glList, GL_COMPILE);
		for ( int i=0; i<m_nDrawCnt; i++ )
			DrawCylinder(TRUE, m_Cycle3D[i].ptC, 0.0, m_dEndmill);
		::glEndList();
	}
}

void CNCcycle::CreateMillList(double z)
{
	m_glList = ::glGenLists(1);
	if ( m_glList > 0 ) {
		::glNewList(m_glList, GL_COMPILE);
		for ( int i=0; i<m_nDrawCnt; i++ )
			DrawCylinder(FALSE, m_Cycle3D[i].ptC, z, m_dEndmill);
		::glEndList();
	}
}

//////////////////////////////////////////////////////////////////////
// CNCcircle クラス
//////////////////////////////////////////////////////////////////////

void CNCcircle::DrawGL(BOOL bG00) const
{
	if ( m_obCdata.GetCount() > 0 ) {
		for ( int i=0; i<m_obCdata.GetSize(); i++ )
			m_obCdata[i]->DrawGL(bG00);
		return;
	}
	if ( bG00 )
		return;

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
		pt.z = m_ptOrg.z;	// ﾍﾘｶﾙ開始座標
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
		pt.y = m_ptOrg.y;
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
		pt.x = m_ptOrg.x;
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

void CNCcircle::DrawMill(void) const
{
//	if ( m_glList > 0 )
		::glCallList(m_glList);
}

void CNCcircle::CreateOcclusionCulling(const CNCdata*)
{
	m_glList = ::glGenLists(1);
	if ( m_glList > 0 ) {
		::glNewList(m_glList, GL_COMPILE);
		SolidPath(TRUE, 0.0);
		DrawCylinder(TRUE, m_ptValE, 0.0, m_dEndmill);
		::glEndList();
	}
}

void CNCcircle::CreateMillList(double z)
{
	m_glList = ::glGenLists(1);
	if ( m_glList > 0 ) {
		::glNewList(m_glList, GL_COMPILE);
		SolidPath(FALSE, z);
		DrawCylinder(FALSE, m_ptValE, z, m_dEndmill);
		::glEndList();
	}
}

void CNCcircle::SolidPath(BOOL bCulling, double z) const
{
	int			i;
	double		sq, eq, cos_q, sin_q, sz, r1, r2, rr = fabs(m_r),
				qs = atan2(m_ptValS.y-m_ptOrg.y, m_ptValS.x-m_ptOrg.x),
				qe = atan2(m_ptValE.y-m_ptOrg.y, m_ptValE.x-m_ptOrg.x);
	CPointD		pt1, pt2;
	vector<CPointD>		vt1, vt2;		// 側面座標
	CPointD		vt3[ARCCOUNT/2+1];		// 始点凸座標

	// 円弧補間切削ﾊﾟｽ座標
	if ( m_nG23 == 0 ) {
		sq = m_eq;
		eq = m_sq;
		r1 = rr + m_dEndmill;	// 進行方向左側
		r2 = rr - m_dEndmill;	// 進行方向右側
		for ( ; sq>eq; sq-=ARCSTEP ) {
			cos_q = cos(sq);
			sin_q = sin(sq);
			pt1.x = r1 * cos_q + m_ptOrg.x;
			pt1.y = r1 * sin_q + m_ptOrg.y;
			pt2.x = r2 * cos_q + m_ptOrg.x;
			pt2.y = r2 * sin_q + m_ptOrg.y;
			vt1.push_back(pt1);
			vt2.push_back(pt2);
		}
	}
	else {
		sq = m_sq;
		eq = m_eq;
		r1 = rr - m_dEndmill;	// 進行方向左側
		r2 = rr + m_dEndmill;	// 進行方向右側
		for ( ; sq<eq; sq+=ARCSTEP ) {
			cos_q = cos(sq);
			sin_q = sin(sq);
			pt1.x = r1 * cos_q + m_ptOrg.x;
			pt1.y = r1 * sin_q + m_ptOrg.y;
			pt2.x = r2 * cos_q + m_ptOrg.x;
			pt2.y = r2 * sin_q + m_ptOrg.y;
			vt1.push_back(pt1);
			vt2.push_back(pt2);
		}
	}
	cos_q = cos(eq);
	sin_q = sin(eq);
	pt1.x = r1 * cos_q + m_ptOrg.x;
	pt1.y = r1 * sin_q + m_ptOrg.y;
	pt2.x = r2 * cos_q + m_ptOrg.x;
	pt2.y = r2 * sin_q + m_ptOrg.y;
	vt1.push_back(pt1);
	vt2.push_back(pt2);
	// 始点凸側
	sq = qs;
	if ( m_nG23 == 1 )
		sq += 180.0*RAD;
	vt3[0] = vt1.front();
	for ( i=1; i<ARCCOUNT/2; i++, sq+=ARCSTEP ) {
		vt3[i].x = m_dEndmill * cos(sq) + m_ptValS.x;
		vt3[i].y = m_dEndmill * sin(sq) + m_ptValS.y;
	}
	vt3[i] = vt2.front();

	if ( bCulling ) {
		// 遮蔽問合せのため底面描画のみ
		// かつ法線ﾍﾞｸﾄﾙは上向き
		// 底面の始点凸側
		::glBegin(GL_TRIANGLE_FAN);
		::glNormal3d(0.0, 0.0, 1.0);
		::glVertex3d(m_ptValS.x, m_ptValS.y, m_ptOrg.z);
		for ( i=ARCCOUNT/2; i>=0; i-- )	// unsigned ではﾙｰﾌﾟ抜けない
			::glVertex3d(vt3[i].x, vt3[i].y, m_ptOrg.z);
		::glEnd();
		// 底面
		sz = m_ptOrg.z;
		::glBegin(GL_QUAD_STRIP);
		::glNormal3d(0.0, 0.0, 1.0);
		for ( i=0; (size_t)i<vt1.size() && (size_t)i<vt2.size(); i++, sz+=m_dHelicalStep ) {
			pt1 = vt1[i];
			pt2 = vt2[i];
			::glVertex3d(pt2.x, pt2.y, sz);
			::glVertex3d(pt1.x, pt1.y, sz);
		}
		::glEnd();
	}
	else {
		// 進行方向左側側面
		sz = m_ptOrg.z;
		::glBegin(GL_QUAD_STRIP);
		for ( i=0; (size_t)i<vt1.size(); i++, sz+=m_dHelicalStep ) {
			pt1 = vt1[i];
			::glNormal3d(pt1.x, pt1.y, 0.0);
			::glVertex3d(pt1.x, pt1.y, sz);
			::glVertex3d(pt1.x, pt1.y, z);
		}
		::glEnd();
		// 進行方向右側側面
		sz = m_ptOrg.z;
		::glBegin(GL_QUAD_STRIP);
		for ( i=0; (size_t)i<vt2.size(); i++, sz+=m_dHelicalStep ) {
			pt2 = vt2[i];
			::glNormal3d(pt2.x, pt2.y, 0.0);
			::glVertex3d(pt2.x, pt2.y, z);
			::glVertex3d(pt2.x, pt2.y, sz);
		}
		::glEnd();
		// 天井の始点凸側
		::glBegin(GL_TRIANGLE_FAN);
		::glNormal3d(0.0, 0.0, 1.0);
		::glVertex3d(m_ptValS.x, m_ptValS.y, z);
		for ( i=0; i<=ARCCOUNT/2; i++ )
			::glVertex3d(vt3[i].x, vt3[i].y, z);
		::glEnd();
		// 天井
		::glBegin(GL_QUAD_STRIP);
		::glNormal3d(0.0, 0.0, 1.0);
		for ( i=0; (size_t)i<vt1.size() && (size_t)i<vt2.size(); i++ ) {
			pt1 = vt1[i];
			pt2 = vt2[i];
			::glVertex3d(pt1.x, pt1.y, z);
			::glVertex3d(pt2.x, pt2.y, z);
		}
		::glEnd();
		// 底面の始点凸側
		::glBegin(GL_TRIANGLE_FAN);
		::glNormal3d(0.0, 0.0, -1.0);
		::glVertex3d(m_ptValS.x, m_ptValS.y, m_ptOrg.z);
		for ( i=ARCCOUNT/2; i>=0; i-- )	// unsigned ではﾙｰﾌﾟ抜けない
			::glVertex3d(vt3[i].x, vt3[i].y, m_ptOrg.z);
		::glEnd();
		// 底面
		sz = m_ptOrg.z;
		::glBegin(GL_QUAD_STRIP);
		::glNormal3d(0.0, 0.0, -1.0);
		for ( i=0; (size_t)i<vt1.size() && (size_t)i<vt2.size(); i++, sz+=m_dHelicalStep ) {
			pt1 = vt1[i];
			pt2 = vt2[i];
			::glVertex3d(pt2.x, pt2.y, sz);
			::glVertex3d(pt1.x, pt1.y, sz);
		}
		::glEnd();
		// 始点凸側面
		::glBegin(GL_QUAD_STRIP);
		for ( i=0; i<=ARCCOUNT/2; i++ ) {
			::glNormal3d(vt3[i].x, vt3[i].y, 0.0);
			::glVertex3d(vt3[i].x, vt3[i].y, z);
			::glVertex3d(vt3[i].x, vt3[i].y, m_ptOrg.z);
		}
		::glEnd();
	}
}
