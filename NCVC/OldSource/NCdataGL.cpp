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
extern	const	PENSTYLE	g_penStyle[];	// ViewOption.cpp
extern	double	_TABLECOS[ARCCOUNT],		// NCVC.cpp
				_TABLESIN[ARCCOUNT];

//////////////////////////////////////////////////////////////////////

inline void _SetEndmillCircle(double r, const CPoint3D& ptOrg, CPointD* pt)
{
	// 円座標をptc[]にｾｯﾄ
	for ( int i=0; i<ARCCOUNT; ++i ) {
		pt[i].x = r * _TABLECOS[i] + ptOrg.x;
		pt[i].y = r * _TABLESIN[i] + ptOrg.y;
	}
	// 最後の閉座標は描画時に設定
}

inline void _SetEndmillCircle(double r, const CPoint3D& ptOrg, VECPOINT3D& vpt)
{
	vpt.push_back(ptOrg);	// 中心座標
	CPoint3D	pt;
	pt.z = ptOrg.z;			// Z値変化なし
	for ( int i=0; i<ARCCOUNT; ++i ) {
		pt.x = r * _TABLECOS[i] + ptOrg.x;
		pt.y = r * _TABLESIN[i] + ptOrg.y;
		vpt.push_back(pt);
	}
	pt = vpt[0];
	vpt.push_back(pt);
}

inline void _SetEndmillSphere(double d, const CPoint3D& ptOrg,
							  CPoint3D ptResult[ARCCOUNT/4][ARCCOUNT])
{
	int		i, j, k = 0;
	double	x, z;
	CPointD	ptc[ARCCOUNT];

	// 半球座標、Z方向に輪切り
	// 90°-1回分をZ方向に繰り返す
	// --- 0度
	x = d;				// このZ位置の半径 cos(0)==1
	z = d + ptOrg.z;	//                 sin(0)==0
	_SetEndmillCircle(x, ptOrg, ptc);	// この位置にXY平面の円座標を登録
	for ( j=0; j<ARCCOUNT; ++j ) {
		ptResult[k][j].x = ptc[j].x;
		ptResult[k][j].y = ptc[j].y;
		ptResult[k][j].z = z;
	}
	// --- 残り270度手前まで
	for ( i=ARCCOUNT-1, ++k; i>ARCCOUNT-ARCCOUNT/4; --i, ++k ) {
		x = d * _TABLECOS[i];
		z = d * _TABLESIN[i] + d + ptOrg.z;
		_SetEndmillCircle(x, ptOrg, ptc);
		for ( j=0; j<ARCCOUNT; ++j ) {
			ptResult[k][j].x = ptc[j].x;
			ptResult[k][j].y = ptc[j].y;
			ptResult[k][j].z = z;
		}
	}
	// 270度頂点は別処理
}

inline void _SetEndmillSpherePath
	(double d, double qp, const CPoint3D& ptOrg, CPoint3D* ptResult)
{
	int		i, j = 0;
	double	x,
			cos_qp = cos(qp),
			sin_qp = sin(qp);
	CPoint3D	pt;

	// Z方向への半円掘り下げ
	// --- 0度
	// XZ平面で考える(y=0)
	x = pt.z = d;
	// XY平面での回転
	pt.x = x * cos_qp;	// - y * sin_qp;
	pt.y = x * sin_qp;	// + y * cos_qp;
	ptResult[j++] = pt + ptOrg;
	// --- 残り180度まで
	for ( i=ARCCOUNT-1; i>=ARCCOUNT/2; --i ) {
		x    = d * _TABLECOS[i];
		pt.z = d * _TABLESIN[i] + d;
		pt.x = x * cos_qp;
		pt.y = x * sin_qp;
		ptResult[j++] = pt + ptOrg;
	}
}

inline void _DrawBottomFaceCircle(const CPoint3D& ptOrg, const CPointD* ptc)
{
	::glBegin(GL_TRIANGLE_FAN);
	::glVertex3dv((const GLdouble *)&ptOrg);
	for ( int i=0; i<ARCCOUNT; ++i )
		::glVertex3d(ptc[i].x, ptc[i].y, ptOrg.z);
	::glVertex3d(ptc[0].x, ptc[0].y, ptOrg.z);
	::glEnd();
}

inline void _DrawBottomFaceSphere(const CPoint3D ptSphere[ARCCOUNT/4][ARCCOUNT], const CPoint3D& ptVtx)
{
	int		i, j;

	for ( i=0; i<ARCCOUNT/4-1; ++i ) {	// 半球頂点を除く
		::glBegin(GL_TRIANGLE_STRIP);
		for ( j=0; j<ARCCOUNT; ++j ) {
			::glVertex3dv((const GLdouble *)&ptSphere[i  ][j]);
			::glVertex3dv((const GLdouble *)&ptSphere[i+1][j]);
		}
		::glVertex3dv((const GLdouble *)&ptSphere[i  ][0]);
		::glVertex3dv((const GLdouble *)&ptSphere[i+1][0]);
		::glEnd();
	}
	::glBegin(GL_TRIANGLE_FAN);
	::glVertex3dv((const GLdouble *)&ptVtx);	// 半球頂点
	for ( j=0; j<ARCCOUNT; ++j )
		::glVertex3dv((const GLdouble *)&ptSphere[i][j]);
	::glVertex3dv((const GLdouble *)&ptSphere[i][0]);
	::glEnd();
}

inline void _AddBottomFaceSphere
	(const CPoint3D ptSphere[ARCCOUNT/4][ARCCOUNT], const CPoint3D& ptVtx,
		vector<VECPOINT3D>* pvStrip, vector<VECPOINT3D>* pvFan)
{
	int		i, j;
	VECPOINT3D	vStrip, vFan;
	vStrip.reserve(ARCCOUNT*2+3);
	vFan.reserve(ARCCOUNT+3);

	for ( i=0; i<ARCCOUNT/4-1; ++i ) {	// 半球頂点を除く
		vStrip.clear();
		for ( j=0; j<ARCCOUNT; ++j ) {
			vStrip.push_back(ptSphere[i  ][j]);
			vStrip.push_back(ptSphere[i+1][j]);
		}
		vStrip.push_back(ptSphere[i  ][0]);
		vStrip.push_back(ptSphere[i+1][0]);
		pvStrip->push_back(vStrip);
	}
	vFan.push_back(ptVtx);	// 半球頂点
	for ( j=0; j<ARCCOUNT; ++j )
		vFan.push_back(ptSphere[i][j]);
	vFan.push_back(ptSphere[i][0]);
	pvFan->push_back(vFan);
}

inline void _DrawEndmillPipe(const vector<VECPOINT3D>& vPipe)
{
	size_t		i, j,
				nTriangle = vPipe.size()-1,
				nVertex;

	for ( i=0; i<nTriangle; ++i ) {
		nVertex = vPipe[i].size();
		::glBegin(GL_TRIANGLE_STRIP);
		for ( j=0; j<nVertex; ++j ) {
			::glVertex3dv((const GLdouble *)&vPipe[i+1][j]);
			::glVertex3dv((const GLdouble *)&vPipe[i  ][j]);
		}
		::glVertex3dv((const GLdouble *)&vPipe[i+1][0]);
		::glVertex3dv((const GLdouble *)&vPipe[i  ][0]);
		::glEnd();
	}
}

inline void _AddEndmillPipe(const vector<VECPOINT3D>& vPipe, vector<VECPOINT3D>* pvStrip)
{
	size_t		i, j,
				nTriangle = vPipe.size()-1,
				nVertex;
	VECPOINT3D	vStrip;

	for ( i=0; i<nTriangle; ++i ) {
		nVertex = vPipe[i].size();
		vStrip.clear();
		vStrip.reserve(nVertex+2);
		for ( j=0; j<nVertex; ++j ) {
			vStrip.push_back(vPipe[i+1][j]);
			vStrip.push_back(vPipe[i  ][j]);
		}
		vStrip.push_back(vPipe[i+1][0]);
		vStrip.push_back(vPipe[i  ][0]);
		pvStrip->push_back(vStrip);
	}
}

//////////////////////////////////////////////////////////////////////
// NCﾃﾞｰﾀの基礎ﾃﾞｰﾀｸﾗｽ
//////////////////////////////////////////////////////////////////////

void CNCdata::DrawGLMillWire(void) const
{
	// 派生ｸﾗｽからの共通呼び出し
	for ( int i=0; i<m_obCdata.GetSize(); ++i )
		m_obCdata[i]->DrawGLMillWire();
}

void CNCdata::DrawGLWireWire(void) const
{
}

void CNCdata::DrawGLLatheFace(void) const
{
}

void CNCdata::DrawGLBottomFace(void) const
{
	for ( int i=0; i<m_obCdata.GetSize(); ++i )
		m_obCdata[i]->DrawGLBottomFace();
}

void CNCdata::AddBottomVertex(vector<VECPOINT3D>* pvStrip, vector<VECPOINT3D>* pvFan)
{
	for ( int i=0; i<m_obCdata.GetSize(); ++i )
		m_obCdata[i]->AddBottomVertex(pvStrip, pvFan);
}

int CNCdata::AddGLWireFirstVertex(vector<GLfloat>& vVertex, vector<GLfloat>& vNormal) const
{
	if ( !m_pWireObj )
		return -1;

	CPoint3D	pts(m_pWireObj->GetStartPoint());

	// ｵﾌﾞｼﾞｪｸﾄ始点を登録
	vVertex.push_back((GLfloat)m_ptValS.x);
	vVertex.push_back((GLfloat)m_ptValS.y);
	vVertex.push_back((GLfloat)m_ptValS.z);
	vVertex.push_back((GLfloat)pts.x);
	vVertex.push_back((GLfloat)pts.y);
	vVertex.push_back((GLfloat)pts.z);

	// 法線ﾍﾞｸﾄﾙ
	optional<CPointD> ptResult = CalcPerpendicularPoint(STARTPOINT, 1.0, 1);
	if ( ptResult ) {
		CPointD	pt( *ptResult );
		vNormal.push_back((GLfloat)pt.x);
		vNormal.push_back((GLfloat)pt.y);
		vNormal.push_back((GLfloat)m_ptValS.z);
		vNormal.push_back((GLfloat)pt.x);
		vNormal.push_back((GLfloat)pt.y);
		vNormal.push_back((GLfloat)pts.z);
	}
	else {
		// 保険
		vNormal.insert(vNormal.end(), NCXYZ*2, 1.0f);
	}

	return 2;
}

int CNCdata::AddGLWireVertex(vector<GLfloat>&, vector<GLfloat>&) const
{
	return 0;
}

int CNCdata::AddGLWireTexture(int, double&, double, GLfloat*) const
{
	return 0;
}

//////////////////////////////////////////////////////////////////////
// CNCline クラス
//////////////////////////////////////////////////////////////////////

void CNCline::DrawGLMillWire(void) const
{
	const CViewOption*	pOpt = AfxGetNCVCApp()->GetViewOption();

	if ( m_obCdata.IsEmpty() || pOpt->GetNCViewFlg(NCVIEWFLG_DRAWREVISE) ) {
		COLORREF col = pOpt->GetNcDrawColor(
			m_obCdata.IsEmpty() ? (GetPenType()+NCCOL_G0) : NCCOL_CORRECT);
		::glLineStipple(1, g_penStyle[pOpt->GetNcDrawType(GetLineType())].nGLpattern);
		::glBegin(GL_LINES);
			::glColor3ub( GetRValue(col), GetGValue(col), GetBValue(col) );
			::glVertex3dv((const GLdouble *)&m_ptValS);
			::glVertex3dv((const GLdouble *)&m_ptValE);
		::glEnd();
	}

	CNCdata::DrawGLMillWire();
}

void CNCline::DrawGLWireWire(void) const
{
	const CViewOption*	pOpt = AfxGetNCVCApp()->GetViewOption();
	COLORREF col = pOpt->GetNcDrawColor( GetPenType()+NCCOL_G0 );
	::glLineStipple(1, g_penStyle[pOpt->GetNcDrawType(GetLineType())].nGLpattern);
	// XY
	::glBegin(GL_LINES);
		::glColor3ub( GetRValue(col), GetGValue(col), GetBValue(col) );
		::glVertex3dv((const GLdouble *)&m_ptValS);
		::glVertex3dv((const GLdouble *)&m_ptValE);
		if ( m_pWireObj ) {
			// UV
			CPoint3D	pts(m_pWireObj->GetStartPoint()),
						pte(m_pWireObj->GetEndPoint());
			::glVertex3dv((const GLdouble *)&pts);
			::glVertex3dv((const GLdouble *)&pte);
			// XYとUVの接続
			::glVertex3dv((const GLdouble *)&m_ptValS);
			::glVertex3dv((const GLdouble *)&pts);
			::glVertex3dv((const GLdouble *)&m_ptValE);
			::glVertex3dv((const GLdouble *)&pte);
		}
	::glEnd();
}

void CNCline::DrawGLLatheFace(void) const
{
	if ( m_nc.nGcode != 1 )
		return;

	::glBegin(GL_LINES);
	::glVertex3dv((const GLdouble *)&m_ptValS);
	::glVertex3dv((const GLdouble *)&m_ptValE);
	::glEnd();
}

void CNCline::DrawGLBottomFace(void) const
{
	if ( m_nc.nGcode != 1 )
		return;
	if ( !m_obCdata.IsEmpty() ) {
		CNCdata::DrawGLBottomFace();
		return;
	}

	int		i;

	if ( GetEndmillType() == 0 ) {
		// ｽｸｳｪｱ座標計算
		CPointD	pts[ARCCOUNT];
		// 始点○の描画
		_SetEndmillCircle(m_dEndmill, m_ptValS, pts);
		_DrawBottomFaceCircle(m_ptValS, pts);
		if ( m_dMove[NCA_X]>0 || m_dMove[NCA_Y]>0 ) {
			CPointD	pte[ARCCOUNT];
			// 終点○の座標計算
			_SetEndmillCircle(m_dEndmill, m_ptValE, pte);
			if ( m_dMove[NCA_Z]>0 ) {
				// XZ, YZ もしくは 3軸移動のときは
				// pts と pte の円をﾊﾟｲﾌﾟ状につなぐ
				::glBegin(GL_TRIANGLE_STRIP);
				for ( i=0; i<ARCCOUNT; ++i ) {
					::glVertex3d(pts[i].x, pts[i].y, m_ptValS.z);
					::glVertex3d(pte[i].x, pte[i].y, m_ptValE.z);
				}
				::glVertex3d(pts[0].x, pts[0].y, m_ptValS.z);
				::glVertex3d(pte[0].x, pte[0].y, m_ptValE.z);
				::glEnd();
			}
			else {
				// XY平面の移動ﾊﾟｽは
				// 始点終点を矩形でつなぐ
				double	q = atan2(m_ptValE.y-m_ptValS.y, m_ptValE.x-m_ptValS.x)+RAD(90.0),
						cos_q = cos(q) * m_dEndmill,
						sin_q = sin(q) * m_dEndmill;
				::glBegin(GL_TRIANGLE_STRIP);
				::glVertex3d( cos_q+m_ptValS.x, sin_q+m_ptValS.y, m_ptValS.z);
				::glVertex3d(-cos_q+m_ptValS.x,-sin_q+m_ptValS.y, m_ptValS.z);
				::glVertex3d( cos_q+m_ptValE.x, sin_q+m_ptValE.y, m_ptValE.z);
				::glVertex3d(-cos_q+m_ptValE.x,-sin_q+m_ptValE.y, m_ptValE.z);
				::glEnd();
			}
			// 終点○の描画
			_DrawBottomFaceCircle(m_ptValE, pte);
		}
	}
	else {
		// ﾎﾞｰﾙｴﾝﾄﾞﾐﾙ座標計算
		CPoint3D	ptSphere[ARCCOUNT/4][ARCCOUNT];
		// 始点の半球座標計算
		_SetEndmillSphere(m_dEndmill, m_ptValS, ptSphere);
		_DrawBottomFaceSphere(ptSphere, m_ptValS);
		if ( m_dMove[NCA_X]>0 || m_dMove[NCA_Y]>0 ) {
			// ﾎﾞｰﾙｴﾝﾄﾞﾐﾙの移動ﾊﾟｽ座標計算
			CPoint3D	pts[ARCCOUNT/2+1], pte[ARCCOUNT/2+1];
			double	q = atan2(m_ptValE.y-m_ptValS.y, m_ptValE.x-m_ptValS.x) + RAD(90.0);
			_SetEndmillSpherePath(m_dEndmill, q, m_ptValS, pts);
			_SetEndmillSpherePath(m_dEndmill, q, m_ptValE, pte);
			::glBegin(GL_TRIANGLE_STRIP);
			for ( i=0; i<=ARCCOUNT/2; ++i ) {
				::glVertex3dv((const GLdouble *)&pts[i]);
				::glVertex3dv((const GLdouble *)&pte[i]);
			}
			::glEnd();
			// 終点の半球座標計算
			_SetEndmillSphere(m_dEndmill, m_ptValE, ptSphere);
			_DrawBottomFaceSphere(ptSphere, m_ptValE);
		}
	}
}

void CNCline::AddBottomVertex(vector<VECPOINT3D>* pvStrip, vector<VECPOINT3D>* pvFan)
{
	if ( m_nc.nGcode != 1 )
		return;
	if ( !m_obCdata.IsEmpty() ) {
		CNCdata::AddBottomVertex(pvStrip, pvFan);
		return;
	}

	int		i;

	if ( GetEndmillType() == 0 ) {
		VECPOINT3D	vFanS;
		vFanS.reserve(ARCCOUNT+3);
		// 始点○の座標ｾｯﾄ
		_SetEndmillCircle(m_dEndmill, m_ptValS, vFanS);
		pvFan->push_back(vFanS);
		if ( m_dMove[NCA_X]>0 || m_dMove[NCA_Y]>0 ) {
			VECPOINT3D	vFanE;
			vFanE.reserve(ARCCOUNT+3);
			// 終点○の座標ｾｯﾄ
			_SetEndmillCircle(m_dEndmill, m_ptValE, vFanE);
			pvFan->push_back(vFanE);
			if ( m_dMove[NCA_Z]>0 ) {
				VECPOINT3D	vStrip;
				vStrip.reserve(ARCCOUNT*2+3);
				// XZ, YZ もしくは 3軸移動のときは
				// pts と pte の円をﾊﾟｲﾌﾟ状につなぐ
				for ( i=1; i<ARCCOUNT+1; ++i ) {	// 0番目は中心
					vStrip.push_back(vFanS[i]);
					vStrip.push_back(vFanE[i]);
				}
				pvStrip->push_back(vStrip);
			}
			else {
				VECPOINT3D	vStrip;
				vStrip.reserve(5);
				// XY平面の移動ﾊﾟｽは
				// 始点終点を矩形でつなぐ
				double	q = atan2(m_ptValE.y-m_ptValS.y, m_ptValE.x-m_ptValS.x)+RAD(90.0),
						cos_q = cos(q) * m_dEndmill,
						sin_q = sin(q) * m_dEndmill;
				vStrip.push_back(CPoint3D( cos_q+m_ptValS.x, sin_q+m_ptValS.y, m_ptValS.z));
				vStrip.push_back(CPoint3D(-cos_q+m_ptValS.x,-sin_q+m_ptValS.y, m_ptValS.z));
				vStrip.push_back(CPoint3D( cos_q+m_ptValE.x, sin_q+m_ptValE.y, m_ptValE.z));
				vStrip.push_back(CPoint3D(-cos_q+m_ptValE.x,-sin_q+m_ptValE.y, m_ptValE.z));
				pvStrip->push_back(vStrip);
			}
		}
	}
	else {
		// ﾎﾞｰﾙｴﾝﾄﾞﾐﾙ座標計算
		CPoint3D	ptSphere[ARCCOUNT/4][ARCCOUNT];
		// 始点の半球座標計算
		_SetEndmillSphere(m_dEndmill, m_ptValS, ptSphere);
		_AddBottomFaceSphere(ptSphere, m_ptValS, pvStrip, pvFan);
		if ( m_dMove[NCA_X]>0 || m_dMove[NCA_Y]>0 ) {
			// ﾎﾞｰﾙｴﾝﾄﾞﾐﾙの移動ﾊﾟｽ座標計算
			CPoint3D	pts[ARCCOUNT/2+1], pte[ARCCOUNT/2+1];
			double	q = atan2(m_ptValE.y-m_ptValS.y, m_ptValE.x-m_ptValS.x) + RAD(90.0);
			_SetEndmillSpherePath(m_dEndmill, q, m_ptValS, pts);
			_SetEndmillSpherePath(m_dEndmill, q, m_ptValE, pte);
			VECPOINT3D	vStrip;
			vStrip.reserve(ARCCOUNT+3);
			for ( i=0; i<=ARCCOUNT/2; ++i ) {
				vStrip.push_back(pts[i]);
				vStrip.push_back(pte[i]);
			}
			pvStrip->push_back(vStrip);
			// 終点の半球座標計算
			_SetEndmillSphere(m_dEndmill, m_ptValE, ptSphere);
			_AddBottomFaceSphere(ptSphere, m_ptValE, pvStrip, pvFan);
		}
	}
}

int CNCline::AddGLWireVertex(vector<GLfloat>& vVertex, vector<GLfloat>& vNormal) const
{
	if ( m_nc.nGcode!=1 || !m_pWireObj )
		return -1;		// 移動ｺｰﾄﾞ => ﾌﾞﾚｲｸ

	CPoint3D	pte(m_pWireObj->GetEndPoint());

	// ｵﾌﾞｼﾞｪｸﾄ終点を登録
	vVertex.push_back((GLfloat)m_ptValE.x);
	vVertex.push_back((GLfloat)m_ptValE.y);
	vVertex.push_back((GLfloat)m_ptValE.z);
	vVertex.push_back((GLfloat)pte.x);
	vVertex.push_back((GLfloat)pte.y);
	vVertex.push_back((GLfloat)pte.z);

	// 法線ﾍﾞｸﾄﾙ
	optional<CPointD> ptResult = CalcPerpendicularPoint(ENDPOINT, 1.0, 1);
	if ( ptResult ) {
		CPointD	pt( *ptResult );
		vNormal.push_back((GLfloat)pt.x);
		vNormal.push_back((GLfloat)pt.y);
		vNormal.push_back((GLfloat)m_ptValE.z);
		vNormal.push_back((GLfloat)pt.x);
		vNormal.push_back((GLfloat)pt.y);
		vNormal.push_back((GLfloat)pte.z);
	}
	else {
		// 保険
		vNormal.insert(vNormal.end(), NCXYZ*2, 1.0f);
	}

	return 2;	// 頂点数
}

int CNCline::AddGLWireTexture(int n, double& dAccuLength, double dAllLength, GLfloat* pfTEX) const
{
	if ( m_nc.nGcode!=1 || !m_pWireObj )
		return -1;

	dAccuLength += m_nc.dLength;
	GLfloat	f = (GLfloat)(dAccuLength / dAllLength);

	pfTEX[n++] = f;
	pfTEX[n++] = 0.0;
	pfTEX[n++] = f;
	pfTEX[n++] = 1.0;

	return 4;
}

//////////////////////////////////////////////////////////////////////
// CNCcycle クラス
//////////////////////////////////////////////////////////////////////

void CNCcycle::DrawGLMillWire(void) const
{
	const CViewOption*	pOpt = AfxGetNCVCApp()->GetViewOption();
	COLORREF	colG0 = pOpt->GetNcDrawColor( NCCOL_G0 ),
				colCY = pOpt->GetNcDrawColor( NCCOL_CYCLE );
	BYTE	bG0r = GetRValue(colG0), bG0g = GetGValue(colG0), bG0b = GetBValue(colG0),
			bCYr = GetRValue(colCY), bCYg = GetGValue(colCY), bCYb = GetBValue(colCY);
	::glLineStipple(1, g_penStyle[pOpt->GetNcDrawType(NCCOLLINE_G0)].nGLpattern);
	::glBegin(GL_LINE_STRIP);
	::glColor3ub( bG0r, bG0g, bG0b );
	::glVertex3dv((const GLdouble *)&m_ptValS);
	::glVertex3dv((const GLdouble *)&m_ptValI);
	::glVertex3dv((const GLdouble *)&m_ptValR);
	::glVertex3dv((const GLdouble *)&m_ptValE);
	::glEnd();
	::glBegin(GL_LINES);
	for ( int i=0; i<m_nDrawCnt; ++i ) {
		::glLineStipple(1, g_penStyle[pOpt->GetNcDrawType(NCCOLLINE_G0)].nGLpattern);
		::glColor3ub( bG0r, bG0g, bG0b );
		::glVertex3dv((const GLdouble *)&m_Cycle3D[i].ptI);
		::glVertex3dv((const GLdouble *)&m_Cycle3D[i].ptR);
		::glLineStipple(1, g_penStyle[pOpt->GetNcDrawType(NCCOLLINE_CYCLE)].nGLpattern);
		::glColor3ub( bCYr, bCYg, bCYb );
		::glVertex3dv((const GLdouble *)&m_Cycle3D[i].ptR);
		::glVertex3dv((const GLdouble *)&m_Cycle3D[i].ptC);
	}
	::glEnd();
}

void CNCcycle::DrawGLWireWire(void) const
{
}

void CNCcycle::DrawGLLatheFace(void) const
{
}

void CNCcycle::DrawGLBottomFace(void) const
{
	if ( GetPlane() != XY_PLANE )
		return;

	int		i;

	if ( GetEndmillType() == 0 ) {
		CPointD	ptc[ARCCOUNT];
		for ( i=0; i<m_nDrawCnt; ++i ) {
			// 座標計算と描画
			_SetEndmillCircle(m_dEndmill, m_Cycle3D[i].ptC, ptc);
			_DrawBottomFaceCircle(m_Cycle3D[i].ptC, ptc);
		}
	}
	else {
		CPoint3D	ptSphere[ARCCOUNT/4][ARCCOUNT];
		for ( i=0; i<m_nDrawCnt; ++i ) {
			_SetEndmillSphere(m_dEndmill, m_Cycle3D[i].ptC, ptSphere);
			_DrawBottomFaceSphere(ptSphere, m_Cycle3D[i].ptC);
		}
	}
}

void CNCcycle::AddBottomVertex(vector<VECPOINT3D>* pvStrip, vector<VECPOINT3D>* pvFan)
{
	if ( GetPlane() != XY_PLANE )
		return;

	int		i;

	if ( GetEndmillType() == 0 ) {
		VECPOINT3D	vFan;
		vFan.reserve(ARCCOUNT+3);
		for ( i=0; i<m_nDrawCnt; ++i ) {
			vFan.clear();
			// 座標計算と描画
			_SetEndmillCircle(m_dEndmill, m_Cycle3D[i].ptC, vFan);
			pvFan->push_back(vFan);
		}
	}
	else {
		CPoint3D	ptSphere[ARCCOUNT/4][ARCCOUNT];
		for ( i=0; i<m_nDrawCnt; ++i ) {
			_SetEndmillSphere(m_dEndmill, m_Cycle3D[i].ptC, ptSphere);
			_AddBottomFaceSphere(ptSphere, m_Cycle3D[i].ptC, pvStrip, pvFan);
		}
	}
}

int CNCcycle::AddGLWireVertex(vector<GLfloat>&, vector<GLfloat>&) const
{
	return 0;
}

int CNCcycle::AddGLWireTexture(int, double&, double, GLfloat*) const
{
	return 0;
}

//////////////////////////////////////////////////////////////////////
// CNCcircle クラス
//////////////////////////////////////////////////////////////////////

void CNCcircle::DrawGLMillWire(void) const
{
	const CViewOption*	pOpt = AfxGetNCVCApp()->GetViewOption();

	if ( m_obCdata.IsEmpty() || pOpt->GetNCViewFlg(NCVIEWFLG_DRAWREVISE) ) {
		COLORREF	col = pOpt->GetNcDrawColor(
			m_obCdata.IsEmpty() ? NCCOL_G1 : NCCOL_CORRECT);
		::glLineStipple(1, g_penStyle[pOpt->GetNcDrawType(NCCOLLINE_G1)].nGLpattern);
		::glBegin(GL_LINE_STRIP);
			::glColor3ub( GetRValue(col), GetGValue(col), GetBValue(col) );
			DrawGLWire();	// 座標指示
		::glEnd();
	}

	CNCdata::DrawGLMillWire();
}

void CNCcircle::DrawGLWireWire(void) const
{
	const CViewOption*	pOpt = AfxGetNCVCApp()->GetViewOption();

	COLORREF	col = pOpt->GetNcDrawColor(NCCOL_G1);
	::glLineStipple(1, g_penStyle[pOpt->GetNcDrawType(NCCOLLINE_G1)].nGLpattern);
	// XY
	::glBegin(GL_LINE_STRIP);
		::glColor3ub( GetRValue(col), GetGValue(col), GetBValue(col) );
		DrawGLWire();
	::glEnd();
	// UV
	if ( m_pWireObj ) {
		::glBegin(GL_LINE_STRIP);
			::glColor3ub( GetRValue(col), GetGValue(col), GetBValue(col) );
			static_cast<CNCcircle*>(m_pWireObj)->DrawGLWire();
		::glEnd();
		// XYとUVの接続
		CPoint3D	pts(m_pWireObj->GetStartPoint()),
					pte(m_pWireObj->GetEndPoint());
		::glBegin(GL_LINES);
			::glColor3ub( GetRValue(col), GetGValue(col), GetBValue(col) );
			::glVertex3dv((const GLdouble *)&m_ptValS);
			::glVertex3dv((const GLdouble *)&pts);
			::glVertex3dv((const GLdouble *)&m_ptValE);
			::glVertex3dv((const GLdouble *)&pte);
		::glEnd();
	}
}

void CNCcircle::DrawGLLatheFace(void) const
{
	::glBegin(GL_LINE_STRIP);
		DrawGLWire();
	::glEnd();
}

void CNCcircle::DrawGLWire(void) const
{
	double		sq, eq, r = fabs(m_r);
	tie(sq, eq) = GetSqEq();
	CPoint3D	pt;

	switch ( GetPlane() ) {
	case XY_PLANE:
		// ARCSTEP づつ微細線分で描画
		if ( m_nG23 == 0 ) {
			for ( pt.z=m_ptValS.z; sq>eq; sq-=ARCSTEP, pt.z+=m_dHelicalStep ) {
				pt.x = r * cos(sq) + m_ptOrg.x;
				pt.y = r * sin(sq) + m_ptOrg.y;
				::glVertex3dv((const GLdouble *)&pt);
			}
		}
		else {
			for ( pt.z=m_ptValS.z; sq<eq; sq+=ARCSTEP, pt.z+=m_dHelicalStep ) {
				pt.x = r * cos(sq) + m_ptOrg.x;
				pt.y = r * sin(sq) + m_ptOrg.y;
				::glVertex3dv((const GLdouble *)&pt);
			}
		}
		// 端数分描画
		pt.x = r * cos(eq) + m_ptOrg.x;
		pt.y = r * sin(eq) + m_ptOrg.y;
		pt.z = m_ptValE.z;		// ﾍﾘｶﾙ終了座標
		::glVertex3dv((const GLdouble *)&pt);
		break;

	case XZ_PLANE:
		if ( m_nG23 == 0 ) {
			for ( pt.y=m_ptValS.y; sq>eq; sq-=ARCSTEP, pt.y+=m_dHelicalStep ) {
				pt.x = r * cos(sq) + m_ptOrg.x;
				pt.z = r * sin(sq) + m_ptOrg.z;
				::glVertex3dv((const GLdouble *)&pt);
			}
		}
		else {
			for ( pt.y=m_ptValS.y; sq<eq; sq+=ARCSTEP, pt.y+=m_dHelicalStep ) {
				pt.x = r * cos(sq) + m_ptOrg.x;
				pt.z = r * sin(sq) + m_ptOrg.z;
				::glVertex3dv((const GLdouble *)&pt);
			}
		}
		pt.x = r * cos(eq) + m_ptOrg.x;
		pt.y = m_ptValE.y;
		pt.z = r * sin(eq) + m_ptOrg.z;
		::glVertex3dv((const GLdouble *)&pt);
		break;

	case YZ_PLANE:
		if ( m_nG23 == 0 ) {
			for ( pt.x=m_ptValS.x; sq>eq; sq-=ARCSTEP, pt.x+=m_dHelicalStep ) {
				pt.y = r * cos(sq) + m_ptOrg.y;
				pt.z = r * sin(sq) + m_ptOrg.z;
				::glVertex3dv((const GLdouble *)&pt);
			}
		}
		else {
			for ( pt.x=m_ptValS.x; sq<eq; sq+=ARCSTEP, pt.x+=m_dHelicalStep ) {
				pt.y = r * cos(sq) + m_ptOrg.y;
				pt.z = r * sin(sq) + m_ptOrg.z;
				::glVertex3dv((const GLdouble *)&pt);
			}
		}
		pt.x = m_ptValE.x;
		pt.y = r * cos(eq) + m_ptOrg.y;
		pt.z = r * sin(eq) + m_ptOrg.z;
		::glVertex3dv((const GLdouble *)&pt);
		break;
	}
}

//	--- CNCcircle::DrawGLBottomFace() サブ
inline void _SetEndmillPathXY
	(const CPointD& pt, double q, double h, double r1, double r2,
		VECPOINT3D& vStrip)
{
	double	cos_q = cos(q);
	double	sin_q = sin(q);
	vStrip.push_back( CPoint3D(
		r1 * cos_q + pt.x,
		r1 * sin_q + pt.y,
		h)
	);
	vStrip.push_back( CPoint3D(
		r2 * cos_q + pt.x,
		r2 * sin_q + pt.y,
		h)
	);
}

inline void _SetEndmillPathXY_Pipe
	(const CPointD& ptOrg, double q, double rr, double h, double d,
		VECPOINT3D& vc)
{
	CPoint3D	pt(
		rr * cos(q) + ptOrg.x,
		rr * sin(q) + ptOrg.y,
		h
	);
	// この位置にXY平面の円を座標登録
	CPointD		ptc[ARCCOUNT];
	_SetEndmillCircle(d, pt, ptc);
	for ( int i=0; i<ARCCOUNT; ++i ) {
		pt.x = ptc[i].x;
		pt.y = ptc[i].y;
		vc.push_back(pt);	// Z座標は変化なし
	}
}

inline void _SetEndmillPathXZ_Pipe
	(const CPointD& ptOrg, double q, double rr, double h, double d,
		VECPOINT3D& vc)
{
	CPoint3D	pt(
		rr * cos(q) + ptOrg.x,
		h,
		rr * sin(q) + ptOrg.y
	);
	CPointD		ptc[ARCCOUNT];
	_SetEndmillCircle(d, pt, ptc);
	for ( int i=0; i<ARCCOUNT; ++i ) {
		pt.x = ptc[i].x;
		pt.y = ptc[i].y;
		vc.push_back(pt);
	}
}

inline void _SetEndmillPathXZ_Sphere
	(double d, double qp, const CPoint3D& ptOrg,
		CPoint3D* ptResult)
{
	double	x,
			cos_qp = cos(qp),
			sin_qp = sin(qp);
	CPoint3D	pt;

	for ( int i=0; i<ARCCOUNT; ++i ) {
		// XY平面の円
		x    = d * _TABLECOS[i];
		pt.y = d * _TABLESIN[i];
		// Zでの回転
		pt.x = x * cos_qp;	// - z * sin_qp;
		pt.z = x * sin_qp;	// + z * cos_qp;
		//
		ptResult[i] = pt + ptOrg;
	}
}

inline void _SetEndmillPathYZ_Pipe
	(const CPointD& ptOrg, double q, double rr, double h, double d,
		VECPOINT3D& vc)
{
	CPoint3D	pt(
		h,
		rr * cos(q) + ptOrg.x,
		rr * sin(q) + ptOrg.y
	);
	CPointD		ptc[ARCCOUNT];
	_SetEndmillCircle(d, pt, ptc);
	for ( int i=0; i<ARCCOUNT; ++i ) {
		pt.x = ptc[i].x;
		pt.y = ptc[i].y;
		vc.push_back(pt);
	}
}

inline void _SetEndmillPathYZ_Sphere
	(double d, double qp, const CPoint3D& ptOrg,
		CPoint3D* ptResult)
{
	double	y,
			cos_qp = cos(qp),
			sin_qp = sin(qp);
	CPoint3D	pt;

	for ( int i=0; i<ARCCOUNT; ++i ) {
		// XY平面の円
		pt.x = d * _TABLECOS[i];
		y    = d * _TABLESIN[i];
		// Zでの回転
		pt.y = y * cos_qp;	// - z * sin_qp;
		pt.z = y * sin_qp;	// + z * cos_qp;
		//
		ptResult[i] = pt + ptOrg;
	}
}
//	---
void CNCcircle::DrawGLBottomFace(void) const
{
	if ( !m_obCdata.IsEmpty() ) {
		CNCdata::DrawGLBottomFace();
		return;
	}

	if ( GetEndmillType() == 0 ) {
		CPointD	pts[ARCCOUNT], pte[ARCCOUNT];
		// 始点・終点のｴﾝﾄﾞﾐﾙ円
		_SetEndmillCircle(m_dEndmill, m_ptValS, pts);
		_SetEndmillCircle(m_dEndmill, m_ptValE, pte);
		_DrawBottomFaceCircle(m_ptValS, pts);
		_DrawBottomFaceCircle(m_ptValE, pte);
		// 軌跡ﾊﾟｽの座標計算
		if ( GetPlane()==XY_PLANE && m_dHelicalStep==0 ) {
			// XY単純矩形座標計算
			VECPOINT3D	vStrip;		// 円弧座標情報（可変長）
			SetEndmillXYPath(vStrip);
			// 描画（ｴﾝﾄﾞﾐﾙ中心から長方形ﾊﾟｽ）
			size_t	i, nVertex = vStrip.size();
			::glBegin(GL_TRIANGLE_STRIP);
			for ( i=0; i<nVertex; ++i )
				::glVertex3dv((const GLdouble *)&vStrip[i]);
			::glEnd();
		}
		else {
			// 軌跡上の円をﾊﾟｲﾌﾟ状につなぐ
			SetEndmillPipe(NULL);		// 描画
		}
	}
	else {
		CPoint3D	ptSphere[ARCCOUNT/4][ARCCOUNT];
		// 始点・終点のﾎﾞｰﾙｴﾝﾄﾞﾐﾙ球
		_SetEndmillSphere(m_dEndmill, m_ptValS, ptSphere);
		_DrawBottomFaceSphere(ptSphere, m_ptValS);
		_SetEndmillSphere(m_dEndmill, m_ptValE, ptSphere);
		_DrawBottomFaceSphere(ptSphere, m_ptValE);
		// 軌跡ﾊﾟｽの座標計算
		SetEndmillBall(NULL);			// 描画
	}
}

void CNCcircle::AddBottomVertex(vector<VECPOINT3D>* pvStrip, vector<VECPOINT3D>* pvFan)
{
	if ( !m_obCdata.IsEmpty() ) {
		CNCdata::AddBottomVertex(pvStrip, pvFan);
		return;
	}

	if ( GetEndmillType() == 0 ) {
		VECPOINT3D	vFanS, vFanE;
		vFanS.reserve(ARCCOUNT+3);
		vFanE.reserve(ARCCOUNT+3);
		// 始点・終点のｴﾝﾄﾞﾐﾙ円
		_SetEndmillCircle(m_dEndmill, m_ptValS, vFanS);
		_SetEndmillCircle(m_dEndmill, m_ptValE, vFanE);
		pvFan->push_back(vFanS);
		pvFan->push_back(vFanE);
		// 軌跡ﾊﾟｽの座標計算
		if ( GetPlane()==XY_PLANE && m_dHelicalStep==0 ) {
			// XY単純矩形座標計算
			VECPOINT3D	vStrip;
			SetEndmillXYPath(vStrip);
			pvStrip->push_back(vStrip);
		}
		else {
			// 軌跡上の円をﾊﾟｲﾌﾟ状につなぐ
			SetEndmillPipe(pvStrip);
		}
	}
	else {
		CPoint3D	ptSphere[ARCCOUNT/4][ARCCOUNT];
		// 始点・終点のﾎﾞｰﾙｴﾝﾄﾞﾐﾙ球
		_SetEndmillSphere(m_dEndmill, m_ptValS, ptSphere);
		_AddBottomFaceSphere(ptSphere, m_ptValS, pvStrip, pvFan);
		_SetEndmillSphere(m_dEndmill, m_ptValE, ptSphere);
		_AddBottomFaceSphere(ptSphere, m_ptValE, pvStrip, pvFan);
		// 軌跡ﾊﾟｽの座標計算
		SetEndmillBall(pvStrip);
	}
}

void CNCcircle::SetEndmillXYPath(VECPOINT3D& vStrip) const
{
	double		sq, eq, h, r1, r2, rr = fabs(m_r);
	CPointD		ptOrg(m_ptOrg.GetXY());

	// 円弧座標情報（可変長）
	vStrip.reserve(ARCCOUNT*2+2);

	tie(sq, eq) = GetSqEq();

	// 円弧補間切削ﾊﾟｽ座標
	if ( m_nG23 == 0 ) {
		r1 = rr + m_dEndmill;	// 進行方向左側
		r2 = rr - m_dEndmill;	// 進行方向右側
		for ( h=m_ptValS.z; sq>eq; sq-=ARCSTEP, h+=m_dHelicalStep )
			_SetEndmillPathXY(ptOrg, sq, h, r1, r2, vStrip);
	}
	else {
		r1 = rr - m_dEndmill;
		r2 = rr + m_dEndmill;
		for ( h=m_ptValS.z; sq<eq; sq+=ARCSTEP, h+=m_dHelicalStep )
			_SetEndmillPathXY(ptOrg, sq, h, r1, r2, vStrip);
	}
	// 端数分
	_SetEndmillPathXY(ptOrg, eq, m_ptValE.z, r1, r2, vStrip);
}

void CNCcircle::SetEndmillPipe(vector<VECPOINT3D>* pvStrip) const
{
	double		sq, eq, h, rr = fabs(m_r);
	CPointD		ptOrg(GetPlaneValue(m_ptOrg));
	VECPOINT3D	vc;
	vector<VECPOINT3D>	vPath;

	vc.reserve(ARCCOUNT+1);
	vPath.reserve(ARCCOUNT+1);

	tie(sq, eq) = GetSqEq();

	// 円弧補間切削ﾊﾟｽ座標
	switch ( GetPlane() ) {
	case XY_PLANE:
		if ( m_nG23 == 0 ) {
			for ( h=m_ptValS.z; sq>eq; sq-=ARCSTEP, h+=m_dHelicalStep ) {
				_SetEndmillPathXY_Pipe(ptOrg, sq, rr, h, m_dEndmill, vc);
				vPath.push_back(vc);
				vc.clear();
			}
		}
		else {
			for ( h=m_ptValS.z; sq<eq; sq+=ARCSTEP, h+=m_dHelicalStep ) {
				_SetEndmillPathXY_Pipe(ptOrg, sq, rr, h, m_dEndmill, vc);
				vPath.push_back(vc);
				vc.clear();
			}
		}
		_SetEndmillPathXY_Pipe(ptOrg, eq, rr, h, m_dEndmill, vc);
		vPath.push_back(vc);
		break;

	case XZ_PLANE:
		if ( m_nG23 == 0 ) {
			for ( h=m_ptValS.y; sq>eq; sq-=ARCSTEP, h+=m_dHelicalStep ) {
				_SetEndmillPathXZ_Pipe(ptOrg, sq, rr, h, m_dEndmill, vc);
				vPath.push_back(vc);
				vc.clear();
			}
		}
		else {
			for ( h=m_ptValS.y; sq<eq; sq+=ARCSTEP, h+=m_dHelicalStep ) {
				_SetEndmillPathXZ_Pipe(ptOrg, sq, rr, h, m_dEndmill, vc);
				vPath.push_back(vc);
				vc.clear();
			}
		}
		_SetEndmillPathXZ_Pipe(ptOrg, eq, rr, h, m_dEndmill, vc);
		vPath.push_back(vc);
		break;

	case YZ_PLANE:
		if ( m_nG23 == 0 ) {
			for ( h=m_ptValS.x; sq>eq; sq-=ARCSTEP, h+=m_dHelicalStep ) {
				_SetEndmillPathYZ_Pipe(ptOrg, sq, rr, h, m_dEndmill, vc);
				vPath.push_back(vc);
				vc.clear();
			}
		}
		else {
			for ( h=m_ptValS.x; sq<eq; sq+=ARCSTEP, h+=m_dHelicalStep ) {
				_SetEndmillPathYZ_Pipe(ptOrg, sq, rr, h, m_dEndmill, vc);
				vPath.push_back(vc);
				vc.clear();
			}
		}
		_SetEndmillPathYZ_Pipe(ptOrg, eq, rr, h, m_dEndmill, vc);
		vPath.push_back(vc);
		break;
	}

	// 軌跡上に並ぶ円をﾊﾟｲﾌﾟ状につなぐ
	if ( pvStrip )
		_AddEndmillPipe(vPath, pvStrip);
	else
		_DrawEndmillPipe(vPath);
}

void CNCcircle::SetEndmillBall(vector<VECPOINT3D>* pvStrip) const
{
	int			i;
	double		sq, eq, qp, h, rr = fabs(m_r);
	CPointD		ptOrg(GetPlaneValue(m_ptOrg));
	CPoint3D	pt, ptc[ARCCOUNT];
	VECPOINT3D	vc;
	vector<VECPOINT3D>	vPath;

	vc.reserve(ARCCOUNT+1);
	vPath.reserve(ARCCOUNT+1);

	tie(sq, eq) = GetSqEq();

	switch ( GetPlane() ) {
	case XY_PLANE:
		// 円弧軌跡上にZ方向の半円座標を計算（直線補間と同じ描画でＯＫ）
		if ( m_nG23 == 0 ) {
			for ( h=m_ptValS.z; sq>eq; sq-=ARCSTEP, h+=m_dHelicalStep ) {
				pt.x = rr * cos(sq) + ptOrg.x;
				pt.y = rr * sin(sq) + ptOrg.y;
				pt.z = h;
				_SetEndmillSpherePath(m_dEndmill, sq, pt, ptc);
				for ( i=0; i<=ARCCOUNT/2; i++ )	// 半円分だけ使用
					vc.push_back(ptc[i]);
				vPath.push_back(vc);
				vc.clear();
			}
		}
		else {
			for ( h=m_ptValS.z; sq<eq; sq+=ARCSTEP, h+=m_dHelicalStep ) {
				pt.x = rr * cos(sq) + ptOrg.x;
				pt.y = rr * sin(sq) + ptOrg.y;
				pt.z = h;
				_SetEndmillSpherePath(m_dEndmill, sq, pt, ptc);
				for ( i=0; i<=ARCCOUNT/2; i++ )
					vc.push_back(ptc[i]);
				vPath.push_back(vc);
				vc.clear();
			}
		}
		pt.x = rr * cos(eq) + ptOrg.x;
		pt.y = rr * sin(eq) + ptOrg.y;
		pt.z = h;
		_SetEndmillSpherePath(m_dEndmill, eq, pt, ptc);
		for ( i=0; i<=ARCCOUNT/2; i++ )
			vc.push_back(ptc[i]);
		vPath.push_back(vc);
		break;

	case XZ_PLANE:
		// 円弧軌跡上に円弧軌跡の中心に傾いた円座標を計算
		if ( m_nG23 == 0 ) {
			for ( h=m_ptValS.y; sq>eq; sq-=ARCSTEP, h+=m_dHelicalStep ) {
				pt.x = rr * cos(sq) + ptOrg.x;
				pt.y = h;
				pt.z = rr * sin(sq) + ptOrg.y + m_dEndmill;
				qp = atan2(m_ptOrg.z-pt.z, m_ptOrg.x-pt.x);	// 円がXY平面に対して傾く角度
				_SetEndmillPathXZ_Sphere(m_dEndmill, qp, pt, ptc);
				for ( i=0; i<ARCCOUNT; i++ )
					vc.push_back(ptc[i]);
				vPath.push_back(vc);
				vc.clear();
			}
		}
		else {
			for ( h=m_ptValS.y; sq<eq; sq+=ARCSTEP, h+=m_dHelicalStep ) {
				pt.x = rr * cos(sq) + ptOrg.x;
				pt.y = h;
				pt.z = rr * sin(sq) + ptOrg.y + m_dEndmill;
				qp = atan2(m_ptOrg.z-pt.z, m_ptOrg.x-pt.x);
				_SetEndmillPathXZ_Sphere(m_dEndmill, qp, pt, ptc);
				for ( i=0; i<ARCCOUNT; i++ )
					vc.push_back(ptc[i]);
				vPath.push_back(vc);
				vc.clear();
			}
		}
		pt.x = rr * cos(eq) + ptOrg.x;
		pt.y = h;
		pt.z = rr * sin(eq) + ptOrg.y + m_dEndmill;
		qp = atan2(m_ptOrg.z-pt.z, m_ptOrg.x-pt.x);
		_SetEndmillPathXZ_Sphere(m_dEndmill, qp, pt, ptc);
		for ( i=0; i<ARCCOUNT; i++ )
			vc.push_back(ptc[i]);
		vPath.push_back(vc);
		break;

	case YZ_PLANE:
		if ( m_nG23 == 0 ) {
			for ( h=m_ptValS.x; sq>eq; sq-=ARCSTEP, h+=m_dHelicalStep ) {
				pt.x = h;
				pt.y = rr * cos(sq) + ptOrg.x;
				pt.z = rr * sin(sq) + ptOrg.y + m_dEndmill;
				qp = atan2(m_ptOrg.z-pt.z, m_ptOrg.y-pt.y);
				_SetEndmillPathYZ_Sphere(m_dEndmill, qp, pt, ptc);
				for ( i=0; i<ARCCOUNT; i++ )
					vc.push_back(ptc[i]);
				vPath.push_back(vc);
				vc.clear();
			}
		}
		else {
			for ( h=m_ptValS.x; sq<eq; sq+=ARCSTEP, h+=m_dHelicalStep ) {
				pt.x = h;
				pt.y = rr * cos(sq) + ptOrg.x;
				pt.z = rr * sin(sq) + ptOrg.y + m_dEndmill;
				qp = atan2(m_ptOrg.z-pt.z, m_ptOrg.y-pt.y);
				_SetEndmillPathYZ_Sphere(m_dEndmill, qp, pt, ptc);
				for ( i=0; i<ARCCOUNT; i++ )
					vc.push_back(ptc[i]);
				vPath.push_back(vc);
				vc.clear();
			}
		}
		pt.x = h;
		pt.y = rr * cos(eq) + ptOrg.x;
		pt.z = rr * sin(eq) + ptOrg.y + m_dEndmill;
		qp = atan2(m_ptOrg.z-pt.z, m_ptOrg.y-pt.y);
		_SetEndmillPathYZ_Sphere(m_dEndmill, qp, pt, ptc);
		for ( i=0; i<ARCCOUNT; i++ )
			vc.push_back(ptc[i]);
		vPath.push_back(vc);
		break;
	}

	// 軌跡上に並ぶ半円または円をﾊﾟｲﾌﾟ状につなぐ
	if ( pvStrip )
		_AddEndmillPipe(vPath, pvStrip);
	else
		_DrawEndmillPipe(vPath);
}

int CNCcircle::AddGLWireVertex(vector<GLfloat>& vVertex, vector<GLfloat>& vNormal) const
{
	if ( !m_pWireObj )
		return -1;

	int			nCnt = 0;
	CNCcircle*	pCircleUV = static_cast<CNCcircle*>(m_pWireObj);
	double		sqxy, eqxy, squv, equv,
				cxy, sxy, cuv, suv,
				rxy = fabs(m_r), ruv = fabs(pCircleUV->GetR());
	CPointD		ptOrgXY( m_ptOrg.GetXY() ),
				ptOrgUV( pCircleUV->GetOrg().GetXY() );
	CPoint3D	pt1, pt2;

	tie(sqxy, eqxy) = GetSqEq();
	tie(squv, equv) = pCircleUV->GetSqEq();
	pt1.z = m_ptValS.z;
	pt2.z = pCircleUV->GetStartPoint().z;

	// 座標値を円筒状に登録
	// 法線ﾍﾞｸﾄﾙは r - 1.0
	if ( m_nG23 == 0 ) {
		for ( ; sqxy>eqxy; sqxy-=ARCSTEP, squv-=ARCSTEP, nCnt+=2 ) {
			cxy = cos(sqxy);	sxy = sin(sqxy);
			cuv = cos(squv);	suv = sin(squv);
			pt1.x = rxy * cxy + ptOrgXY.x;
			pt1.y = rxy * sxy + ptOrgXY.y;
			pt2.x = ruv * cuv + ptOrgUV.x;
			pt2.y = ruv * suv + ptOrgUV.y;
			vVertex.push_back((GLfloat)pt1.x);
			vVertex.push_back((GLfloat)pt1.y);
			vVertex.push_back((GLfloat)pt1.z);
			vVertex.push_back((GLfloat)pt2.x);
			vVertex.push_back((GLfloat)pt2.y);
			vVertex.push_back((GLfloat)pt2.z);
			pt1.x = (rxy-1.0) * cxy + ptOrgXY.x;
			pt1.y = (rxy-1.0) * sxy + ptOrgXY.y;
			pt2.x = (ruv-1.0) * cuv + ptOrgUV.x;
			pt2.y = (ruv-1.0) * suv + ptOrgUV.y;
			vNormal.push_back((GLfloat)pt1.x);
			vNormal.push_back((GLfloat)pt1.y);
			vNormal.push_back((GLfloat)pt1.z);
			vNormal.push_back((GLfloat)pt2.x);
			vNormal.push_back((GLfloat)pt2.y);
			vNormal.push_back((GLfloat)pt2.z);
		}
	}
	else {
		for ( ; sqxy<eqxy; sqxy+=ARCSTEP, squv+=ARCSTEP, nCnt+=2 ) {
			cxy = cos(sqxy);	sxy = sin(sqxy);
			cuv = cos(squv);	suv = sin(squv);
			pt1.x = rxy * cxy + ptOrgXY.x;
			pt1.y = rxy * sxy + ptOrgXY.y;
			pt2.x = ruv * cuv + ptOrgUV.x;
			pt2.y = ruv * suv + ptOrgUV.y;
			vVertex.push_back((GLfloat)pt1.x);
			vVertex.push_back((GLfloat)pt1.y);
			vVertex.push_back((GLfloat)pt1.z);
			vVertex.push_back((GLfloat)pt2.x);
			vVertex.push_back((GLfloat)pt2.y);
			vVertex.push_back((GLfloat)pt2.z);
			pt1.x = (rxy-1.0) * cxy + ptOrgXY.x;
			pt1.y = (rxy-1.0) * sxy + ptOrgXY.y;
			pt2.x = (ruv-1.0) * cuv + ptOrgUV.x;
			pt2.y = (ruv-1.0) * suv + ptOrgUV.y;
			vNormal.push_back((GLfloat)pt1.x);
			vNormal.push_back((GLfloat)pt1.y);
			vNormal.push_back((GLfloat)pt1.z);
			vNormal.push_back((GLfloat)pt2.x);
			vNormal.push_back((GLfloat)pt2.y);
			vNormal.push_back((GLfloat)pt2.z);
		}
	}
	cxy = cos(eqxy);	sxy = sin(eqxy);
	cuv = cos(equv);	suv = sin(equv);
	pt1.x = rxy * cxy + ptOrgXY.x;
	pt1.y = rxy * sxy + ptOrgXY.y;
	pt2.x = ruv * cuv + ptOrgUV.x;
	pt2.y = ruv * suv + ptOrgUV.y;
	vVertex.push_back((GLfloat)pt1.x);
	vVertex.push_back((GLfloat)pt1.y);
	vVertex.push_back((GLfloat)pt1.z);
	vVertex.push_back((GLfloat)pt2.x);
	vVertex.push_back((GLfloat)pt2.y);
	vVertex.push_back((GLfloat)pt2.z);
	pt1.x = (rxy-1.0) * cxy + ptOrgXY.x;
	pt1.y = (rxy-1.0) * sxy + ptOrgXY.y;
	pt2.x = (ruv-1.0) * cuv + ptOrgUV.x;
	pt2.y = (ruv-1.0) * suv + ptOrgUV.y;
	vNormal.push_back((GLfloat)pt1.x);
	vNormal.push_back((GLfloat)pt1.y);
	vNormal.push_back((GLfloat)pt1.z);
	vNormal.push_back((GLfloat)pt2.x);
	vNormal.push_back((GLfloat)pt2.y);
	vNormal.push_back((GLfloat)pt2.z);

	return nCnt+2;		// 頂点数
}

int CNCcircle::AddGLWireTexture(int n, double& dAccuLength, double dAllLength, GLfloat* pfTEX) const
{
	if ( !m_pWireObj )
		return -1;

	int		nCnt = 0;
	GLfloat	f;
	double	sq = m_sq, r = fabs(m_r);

	// ﾃｸｽﾁｬ座標の登録は、回転方向は関係なく、長さの割合だけで良い
	for ( ; sq<m_eq; sq+=ARCSTEP, nCnt+=4, dAccuLength+=r*ARCSTEP ) {
		f = (GLfloat)(dAccuLength / dAllLength);
		pfTEX[n++] = f;
		pfTEX[n++] = 0.0;
		pfTEX[n++] = f;
		pfTEX[n++] = 1.0;
	}
	sq -= ARCSTEP;
	dAccuLength -= r*ARCSTEP;
	dAccuLength += r*(m_eq-sq);
	f = (GLfloat)(dAccuLength / dAllLength);
	pfTEX[n++] = f;
	pfTEX[n++] = 0.0;
	pfTEX[n++] = f;
	pfTEX[n++] = 1.0;

	return nCnt+4;
}
