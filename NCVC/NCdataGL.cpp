// NCdataGL.cpp: CNCdata �N���X�̃C���v�������e�[�V����
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

inline void _SetEndmillCircle(double d, const CPointD& ptOrg, CPointD* pt)
{
	// �~���W���
	int		i = 1;
	double	q = ARCSTEP;
	// -- �Q��ٰ�߂����炷
	pt[0].x = d + ptOrg.x;		// cos(0) == 1
	pt[0].y = ptOrg.y;			// sin(0) == 0
	for ( ; i<ARCCOUNT-1; i++, q+=ARCSTEP ) {
		pt[i].x = d * cos(q) + ptOrg.x;
		pt[i].y = d * sin(q) + ptOrg.y;
	}
	pt[i] = pt[0];
}

inline void _SetEndmillSphere(double d, const CPoint3D& ptOrg, vector<CVCircle>& vSphere)
{
	// �������W�AZ�����ɗ֐؂�
	int		i = 0, j;
	double	q = 0, x;
	CPointD	ptc[ARCCOUNT], ptOrgXY(ptOrg.GetXY());
	CPoint3D	pt;
	CVCircle	vc;

	vc.reserve(ARCCOUNT);

	// 90��-1�񕪂�Z�����ɌJ��Ԃ�
	for ( ; i<ARCCOUNT/4-1; i++, q-=ARCSTEP ) {
		x    = d * cos(q);		// ����Z�ʒu�̔��a
		pt.z = d * sin(q) + d + ptOrg.z;
		// ���̈ʒu��XY���ʂ̉~���W��o�^
		_SetEndmillCircle(x, ptOrgXY, ptc);
		for ( j=0; j<ARCCOUNT; j++ ) {
			pt.x = ptc[j].x;
			pt.y = ptc[j].y;
			vc.push_back(pt);	// pt.z �͕ێ�
		}
		vSphere.push_back(vc);
		vc.clear();
	}
	// �Ō�̔������_
	vc.push_back(ptOrg);
	vSphere.push_back(vc);
}

inline void _SetEndmillSpherePath
	(double d, double qp, const CPoint3D& ptOrg,
		CPoint3D* ptResult)
{
	int		i = 0;
	double	q = 0,
			cos_qp = cos(qp),
			sin_qp = sin(qp),
			x;
	CPoint3D	pt;

	// Z�����ւ̔��~�@�艺��
	for ( ; i<ARCCOUNT/2; i++, q-=ARCSTEP ) {
		// XZ���ʂōl����(y=0)
		x    = d * cos(q);
		pt.z = d * sin(q) + d;
		// XY���ʂł̉�]
		pt.x = x * cos_qp;	// - y * sin_qp;
		pt.y = x * sin_qp;	// + y * cos_qp;
		//
		ptResult[i] = pt + ptOrg;
	}
}

inline void _DrawBottomFaceCircle(const CPoint3D& ptOrg, const CPointD* ptc)
{
	::glBegin(GL_TRIANGLE_FAN);
	::glVertex3d(ptOrg.x, ptOrg.y, ptOrg.z);
	for ( int i=0; i<ARCCOUNT; i++ )
		::glVertex3d(ptc[i].x, ptc[i].y, ptOrg.z);
	::glVertex3d(ptc[0].x, ptc[0].y, ptOrg.z);
	::glEnd();
}

inline void _DrawBottomFaceSphere(const vector<CVCircle>& vSphere)
{
	size_t		i, j;
	CPoint3D	pt1, pt2;

	for ( i=0; i<vSphere.size()-2; i++ ) {	// �������_������
		::glBegin(GL_TRIANGLE_STRIP);
		for ( j=0; j<vSphere[i].size(); j++ ) {
			pt1 = vSphere[i][j];
			pt2 = vSphere[i+1][j];
			::glVertex3d(pt1.x, pt1.y, pt1.z);
			::glVertex3d(pt2.x, pt2.y, pt2.z);
		}
		pt1 = vSphere[i][0];
		pt2 = vSphere[i+1][0];
		::glVertex3d(pt1.x, pt1.y, pt1.z);
		::glVertex3d(pt2.x, pt2.y, pt2.z);
		::glEnd();
	}
	pt2 = vSphere.back()[0];	// �������_
	::glBegin(GL_TRIANGLE_FAN);
	::glVertex3d(pt2.x, pt2.y, pt2.z);
	for ( j=0; j<vSphere[i].size(); j++ ) {
		pt1 = vSphere[i][j];
		::glVertex3d(pt1.x, pt1.y, pt1.z);
	}
	pt1 = vSphere[i][0];
	::glVertex3d(pt1.x, pt1.y, pt1.z);
	::glEnd();
}

inline void _DrawEndmillPipe(const vector<CVCircle>& vPipe)
{
	size_t	i, j;
	CPoint3D	pt1, pt2;

	for ( i=0; i<vPipe.size()-1; i++ ) {
		::glBegin(GL_TRIANGLE_STRIP);
		for ( j=0; j<vPipe[i].size(); j++ ) {
			pt1 = vPipe[i+1][j];
			pt2 = vPipe[i][j];
			::glVertex3d(pt1.x, pt1.y, pt1.z);
			::glVertex3d(pt2.x, pt2.y, pt2.z);
		}
		pt1 = vPipe[i+1][0];
		pt2 = vPipe[i][0];
		::glVertex3d(pt1.x, pt1.y, pt1.z);
		::glVertex3d(pt2.x, pt2.y, pt2.z);
		::glEnd();
	}
}

//////////////////////////////////////////////////////////////////////
// NC�ް��̊�b�ް��׽
//////////////////////////////////////////////////////////////////////

void CNCdata::DrawGL(void) const
{
	for ( int i=0; i<m_obCdata.GetSize(); i++ )
		m_obCdata[i]->DrawGL();
}

void CNCdata::DrawBottomFace(void) const
{
}

//////////////////////////////////////////////////////////////////////
// CNCline �N���X
//////////////////////////////////////////////////////////////////////

void CNCline::DrawGL(void) const
{
	const CViewOption*	pOpt = AfxGetNCVCApp()->GetViewOption();

	if ( m_obCdata.IsEmpty() || pOpt->GetNCViewFlg(NCVIEWFLG_DRAWREVISE) ) {
		COLORREF col = pOpt->GetNcDrawColor(
			m_obCdata.IsEmpty() ? (GetPenType()+NCCOL_G0) : NCCOL_CORRECT);
		::glLineStipple(1, g_penStyle[pOpt->GetNcDrawType(GetLineType())].nGLpattern);
		::glBegin(GL_LINES);
		::glColor3ub( GetRValue(col), GetGValue(col), GetBValue(col) );
		::glVertex3d(m_ptValS.x, m_ptValS.y, m_ptValS.z);
		::glVertex3d(m_ptValE.x, m_ptValE.y, m_ptValE.z);
		::glEnd();
	}

	CNCdata::DrawGL();
}

void CNCline::DrawBottomFace(void) const
{
	int		i;

	if ( m_nc.nGcode != 1 )
		return;
	if ( !m_obCdata.IsEmpty() ) {
		for ( i=0; i<m_obCdata.GetSize(); i++ )
			m_obCdata[i]->DrawBottomFace();
		return;
	}

	if ( GetEndmillType() == 0 ) {
		// ��������W�v�Z
		CPointD	pte[ARCCOUNT];
		_SetEndmillCircle(m_dEndmill, m_ptValE, pte);
		if ( m_dMove[NCA_X]>0 || m_dMove[NCA_Y]>0 ) {
			// �n�_���̕`��
			CPointD	pts[ARCCOUNT];
			_SetEndmillCircle(m_dEndmill, m_ptValS, pts);
			_DrawBottomFaceCircle(m_ptValS, pts);
			if ( m_dMove[NCA_Z]>0 ) {
				// XZ, YZ �������� 3���ړ��̂Ƃ���
				// pts �� pte �̉~���߲�ߏ�ɂȂ�
				::glBegin(GL_TRIANGLE_STRIP);
				for ( i=0; i<ARCCOUNT; i++ ) {
					::glVertex3d(pts[i].x, pts[i].y, m_ptValS.z);
					::glVertex3d(pte[i].x, pte[i].y, m_ptValE.z);
				}
				::glVertex3d(pts[0].x, pts[0].y, m_ptValS.z);
				::glVertex3d(pte[0].x, pte[0].y, m_ptValE.z);
				::glEnd();
			}
			else {
				// XY���ʂ̈ړ��߽��
				// �n�_�I�_����`�łȂ�
				double	q = atan2(m_ptValE.y-m_ptValS.y, m_ptValE.x-m_ptValS.x)+90.0*RAD,
						cos_q = cos(q) * m_dEndmill,
						sin_q = sin(q) * m_dEndmill;
				::glBegin(GL_TRIANGLE_STRIP);
				::glVertex3d( cos_q+m_ptValS.x, sin_q+m_ptValS.y, m_ptValS.z);
				::glVertex3d(-cos_q+m_ptValS.x,-sin_q+m_ptValS.y, m_ptValS.z);
				::glVertex3d( cos_q+m_ptValE.x, sin_q+m_ptValE.y, m_ptValE.z);
				::glVertex3d(-cos_q+m_ptValE.x,-sin_q+m_ptValE.y, m_ptValE.z);
				::glEnd();
			}
		}
		// �I�_���̕`��
		_DrawBottomFaceCircle(m_ptValE, pte);
	}
	else {
		// �ްٴ����ٍ��W�v�Z
		vector<CVCircle>	vSphere;
		vSphere.reserve(ARCCOUNT/4);
		// �n�_�̔������W�v�Z
		_SetEndmillSphere(m_dEndmill, m_ptValS, vSphere);
		_DrawBottomFaceSphere(vSphere);
		if ( m_dMove[NCA_X]>0 || m_dMove[NCA_Y]>0 ) {
			// �ްٴ����ق̈ړ��߽���W�v�Z
			CPoint3D	pts[ARCCOUNT/2], pte[ARCCOUNT/2];
			double	q = atan2(m_ptValE.y-m_ptValS.y, m_ptValE.x-m_ptValS.x) + 90.0*RAD;
			_SetEndmillSpherePath(m_dEndmill, q, m_ptValS, pts);
			_SetEndmillSpherePath(m_dEndmill, q, m_ptValE, pte);
			::glBegin(GL_TRIANGLE_STRIP);
			for ( i=0; i<ARCCOUNT/2; i++ ) {
				::glVertex3d(pts[i].x, pts[i].y, pts[i].z);
				::glVertex3d(pte[i].x, pte[i].y, pte[i].z);
			}
			::glEnd();
			// �I�_�̔������W�v�Z
			vSphere.clear();
			_SetEndmillSphere(m_dEndmill, m_ptValE, vSphere);
			_DrawBottomFaceSphere(vSphere);
		}
	}
}

//////////////////////////////////////////////////////////////////////
// CNCcycle �N���X
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

void CNCcycle::DrawBottomFace(void) const
{
	if ( GetPlane() != XY_PLANE )
		return;

	int		i;

	if ( GetEndmillType() == 0 ) {
		CPointD	ptc[ARCCOUNT];
		for ( i=0; i<m_nDrawCnt; i++ ) {
			// ���W�v�Z�ƕ`��
			_SetEndmillCircle(m_dEndmill, m_Cycle3D[i].ptC.GetXY(), ptc);
			_DrawBottomFaceCircle(m_Cycle3D[i].ptC, ptc);
		}
	}
	else {
		vector<CVCircle>	vSphere;
		vSphere.reserve(ARCCOUNT/4);
		for ( i=0; i<m_nDrawCnt; i++ ) {
			_SetEndmillSphere(m_dEndmill, m_Cycle3D[i].ptC, vSphere);
			_DrawBottomFaceSphere(vSphere);
			vSphere.clear();
		}
	}
}

//////////////////////////////////////////////////////////////////////
// CNCcircle �N���X
//////////////////////////////////////////////////////////////////////

void CNCcircle::DrawGL(void) const
{
	const CViewOption*	pOpt = AfxGetNCVCApp()->GetViewOption();

	if ( m_obCdata.IsEmpty() || pOpt->GetNCViewFlg(NCVIEWFLG_DRAWREVISE) ) {
		double		sq, eq, r = fabs(m_r);
		CPoint3D	pt;
		tie(sq, eq) = GetSqEq();
		COLORREF	col = pOpt->GetNcDrawColor(
			m_obCdata.IsEmpty() ? NCCOL_G1 : NCCOL_CORRECT);
		::glLineStipple(1, g_penStyle[pOpt->GetNcDrawType(NCCOLLINE_G1)].nGLpattern);
		::glBegin(GL_LINE_STRIP);
		::glColor3ub( GetRValue(col), GetGValue(col), GetBValue(col) );

		switch ( GetPlane() ) {
		case XY_PLANE:
			// ARCSTEP �Â��א����ŕ`��
			if ( m_nG23 == 0 ) {
				for ( pt.z=m_ptValS.z; sq>eq; sq-=ARCSTEP, pt.z+=m_dHelicalStep ) {
					pt.x = r * cos(sq) + m_ptOrg.x;
					pt.y = r * sin(sq) + m_ptOrg.y;
					::glVertex3d(pt.x, pt.y, pt.z);
				}
			}
			else {
				for ( pt.z=m_ptValS.z; sq<eq; sq+=ARCSTEP, pt.z+=m_dHelicalStep ) {
					pt.x = r * cos(sq) + m_ptOrg.x;
					pt.y = r * sin(sq) + m_ptOrg.y;
					::glVertex3d(pt.x, pt.y, pt.z);
				}
			}
			// �[�����`��
			pt.x = r * cos(eq) + m_ptOrg.x;
			pt.y = r * sin(eq) + m_ptOrg.y;
			pt.z = m_ptValE.z;		// �ضُI�����W
			::glVertex3d(pt.x, pt.y, pt.z);
			break;

		case XZ_PLANE:
			if ( m_nG23 == 0 ) {
				for ( pt.y=m_ptValS.y; sq>eq; sq-=ARCSTEP, pt.y+=m_dHelicalStep ) {
					pt.x = r * cos(sq) + m_ptOrg.x;
					pt.z = r * sin(sq) + m_ptOrg.z;
					::glVertex3d(pt.x, pt.y, pt.z);
				}
			}
			else {
				for ( pt.y=m_ptValS.y; sq<eq; sq+=ARCSTEP, pt.y+=m_dHelicalStep ) {
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
			if ( m_nG23 == 0 ) {
				for ( pt.x=m_ptValS.x; sq>eq; sq-=ARCSTEP, pt.x+=m_dHelicalStep ) {
					pt.y = r * cos(sq) + m_ptOrg.y;
					pt.z = r * sin(sq) + m_ptOrg.z;
					::glVertex3d(pt.x, pt.y, pt.z);
				}
			}
			else {
				for ( pt.x=m_ptValS.x; sq<eq; sq+=ARCSTEP, pt.x+=m_dHelicalStep ) {
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

	CNCdata::DrawGL();
}
//	--- CNCcircle::DrawBottomFace() �T�u
inline void _SetEndmillPathXY
	(const CPointD& pt, double q, double h, double r1, double r2,
		vector<CPoint3D>& vt1, vector<CPoint3D>& vt2)
{
	double	cos_q = cos(q);
	double	sin_q = sin(q);
	vt1.push_back( CPoint3D(
		r1 * cos_q + pt.x,
		r1 * sin_q + pt.y,
		h)
	);
	vt2.push_back( CPoint3D(
		r2 * cos_q + pt.x,
		r2 * sin_q + pt.y,
		h)
	);
}

inline void _SetEndmillPathXY_Pipe
	(const CPointD& ptOrg, double q, double rr, double h, double d,
		CVCircle& vc)
{
	CPoint3D	pt(
		rr * cos(q) + ptOrg.x,
		rr * sin(q) + ptOrg.y,
		h
	);
	// ���̈ʒu��XY���ʂ̉~�����W�o�^
	CPointD		ptc[ARCCOUNT];
	_SetEndmillCircle(d, pt.GetXY(), ptc);
	for ( int i=0; i<ARCCOUNT; i++ ) {
		pt.x = ptc[i].x;
		pt.y = ptc[i].y;
		vc.push_back(pt);	// Z���W�͕ω��Ȃ�
	}
}

inline void _SetEndmillPathXZ_Pipe
	(const CPointD& ptOrg, double q, double rr, double h, double d,
		CVCircle& vc)
{
	CPoint3D	pt(
		rr * cos(q) + ptOrg.x,
		h,
		rr * sin(q) + ptOrg.y
	);
	CPointD		ptc[ARCCOUNT];
	_SetEndmillCircle(d, pt.GetXY(), ptc);
	for ( int i=0; i<ARCCOUNT; i++ ) {
		pt.x = ptc[i].x;
		pt.y = ptc[i].y;
		vc.push_back(pt);
	}
}

inline void _SetEndmillPathXZ_Sphere
	(double d, double qp, const CPoint3D& ptOrg,
		CPoint3D* ptResult)
{
	int		i = 0;
	double	q = 0,
			cos_qp = cos(qp),
			sin_qp = sin(qp),
			x;
	CPoint3D	pt;

	for ( ; i<ARCCOUNT; i++, q+=ARCSTEP ) {
		// XY���ʂ̉~
		x    = d * cos(q);
		pt.y = d * sin(q);
		// Z�ł̉�]
		pt.x = x * cos_qp;	// - z * sin_qp;
		pt.z = x * sin_qp;	// + z * cos_qp;
		//
		ptResult[i] = pt + ptOrg;
	}
}

inline void _SetEndmillPathYZ_Pipe
	(const CPointD& ptOrg, double q, double rr, double h, double d,
		CVCircle& vc)
{
	CPoint3D	pt(
		h,
		rr * cos(q) + ptOrg.x,
		rr * sin(q) + ptOrg.y
	);
	CPointD		ptc[ARCCOUNT];
	_SetEndmillCircle(d, pt.GetXY(), ptc);
	for ( int i=0; i<ARCCOUNT; i++ ) {
		pt.x = ptc[i].x;
		pt.y = ptc[i].y;
		vc.push_back(pt);
	}
}

inline void _SetEndmillPathYZ_Sphere
	(double d, double qp, const CPoint3D& ptOrg,
		CPoint3D* ptResult)
{
	int		i = 0;
	double	q = 0,
			cos_qp = cos(qp),
			sin_qp = sin(qp),
			y;
	CPoint3D	pt;

	for ( ; i<ARCCOUNT; i++, q+=ARCSTEP ) {
		// XY���ʂ̉~
		pt.x = d * cos(q);
		y    = d * sin(q);
		// Z�ł̉�]
		pt.y = y * cos_qp;	// - z * sin_qp;
		pt.z = y * sin_qp;	// + z * cos_qp;
		//
		ptResult[i] = pt + ptOrg;
	}
}
//	---
void CNCcircle::DrawBottomFace(void) const
{
	if ( !m_obCdata.IsEmpty() ) {
		for ( int i=0; i<m_obCdata.GetSize(); i++ )
			m_obCdata[i]->DrawBottomFace();
		return;
	}

	if ( GetEndmillType() == 0 ) {
		CPointD	pts[ARCCOUNT], pte[ARCCOUNT];
		// �n�_�E�I�_�̴����ى~
		_SetEndmillCircle(m_dEndmill, m_ptValS.GetXY(), pts);
		_SetEndmillCircle(m_dEndmill, m_ptValE.GetXY(), pte);
		_DrawBottomFaceCircle(m_ptValS, pts);
		_DrawBottomFaceCircle(m_ptValE, pte);
		// �O���߽�̍��W�v�Z
		if ( GetPlane()==XY_PLANE && m_dHelicalStep==0 ) {
			// XY�P����`���W�v�Z
			DrawEndmillXYPath();
		}
		else {
			// �O�Տ�̉~���߲�ߏ�ɂȂ�
			DrawEndmillPipe();
		}
	}
	else {
		vector<CVCircle>	vSphere;
		vSphere.reserve(ARCCOUNT/4);
		// �n�_�E�I�_���ްٴ����ً�
		_SetEndmillSphere(m_dEndmill, m_ptValS, vSphere);
		_DrawBottomFaceSphere(vSphere);
		vSphere.clear();
		_SetEndmillSphere(m_dEndmill, m_ptValE, vSphere);
		_DrawBottomFaceSphere(vSphere);
		vSphere.clear();
		// �O���߽�̍��W�v�Z
		DrawEndmillBall();
	}
}

void CNCcircle::DrawEndmillXYPath(void) const
{
	double	sq, eq, h, r1, r2, rr = fabs(m_r);
	CPointD	ptOrg(m_ptOrg.GetXY());
	vector<CPoint3D>	vt1, vt2;

	vt1.reserve(ARCCOUNT);
	vt2.reserve(ARCCOUNT);
	tie(sq, eq) = GetSqEq();

	// �~�ʕ�Ԑ؍��߽���W
	if ( m_nG23 == 0 ) {
		r1 = rr + m_dEndmill;	// �i�s��������
		r2 = rr - m_dEndmill;	// �i�s�����E��
		for ( h=m_ptValS.z; sq>eq; sq-=ARCSTEP, h+=m_dHelicalStep )
			_SetEndmillPathXY(ptOrg, sq, h, r1, r2, vt1, vt2);
	}
	else {
		r1 = rr - m_dEndmill;
		r2 = rr + m_dEndmill;
		for ( h=m_ptValS.z; sq<eq; sq+=ARCSTEP, h+=m_dHelicalStep )
			_SetEndmillPathXY(ptOrg, sq, h, r1, r2, vt1, vt2);
	}
	// �[����
	_SetEndmillPathXY(ptOrg, eq, m_ptValE.z, r1, r2, vt1, vt2);

	// �`��i�����ْ��S���璷���`�߽�j
	::glBegin(GL_TRIANGLE_STRIP);
	for ( size_t i=0; i<vt1.size(); i++ ) {
		::glVertex3d(vt1[i].x, vt1[i].y, vt1[i].z);
		::glVertex3d(vt2[i].x, vt2[i].y, vt2[i].z);
	}
	::glEnd();
}

void CNCcircle::DrawEndmillPipe(void) const
{
	double		sq, eq, h, rr = fabs(m_r);
	CPointD		ptOrg(GetPlaneValue(m_ptOrg));
	CVCircle	vc;
	vector<CVCircle>	vPath;

	vc.reserve(ARCCOUNT);
	vPath.reserve(ARCCOUNT);
	tie(sq, eq) = GetSqEq();

	// �~�ʕ�Ԑ؍��߽���W
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

	// �O�Տ�ɕ��ԉ~���߲�ߏ�ɂȂ�
	_DrawEndmillPipe(vPath);
}

void CNCcircle::DrawEndmillBall(void) const
{
	int			i;
	double		sq, eq, qp, h, rr = fabs(m_r);
	CPointD		ptOrg(GetPlaneValue(m_ptOrg));
	CPoint3D	pt, ptc[ARCCOUNT];
	CVCircle	vc;
	vector<CVCircle>	vPath;

	vc.reserve(ARCCOUNT);
	vPath.reserve(ARCCOUNT);
	tie(sq, eq) = GetSqEq();

	switch ( GetPlane() ) {
	case XY_PLANE:
		// �~�ʋO�Տ��Z�����̔��~���W���v�Z�i������ԂƓ����`��łn�j�j
		if ( m_nG23 == 0 ) {
			for ( h=m_ptValS.z; sq>eq; sq-=ARCSTEP, h+=m_dHelicalStep ) {
				pt.x = rr * cos(sq) + ptOrg.x;
				pt.y = rr * sin(sq) + ptOrg.y;
				pt.z = h;
				_SetEndmillSpherePath(m_dEndmill, sq, pt, ptc);
				for ( i=0; i<ARCCOUNT/2; i++ )	// ���~�������g�p
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
				for ( i=0; i<ARCCOUNT/2; i++ )
					vc.push_back(ptc[i]);
				vPath.push_back(vc);
				vc.clear();
			}
		}
		pt.x = rr * cos(eq) + ptOrg.x;
		pt.y = rr * sin(eq) + ptOrg.y;
		pt.z = h;
		_SetEndmillSpherePath(m_dEndmill, eq, pt, ptc);
		for ( i=0; i<ARCCOUNT/2; i++ )
			vc.push_back(ptc[i]);
		vPath.push_back(vc);
		break;

	case XZ_PLANE:
		// �~�ʋO�Տ�ɉ~�ʋO�Ղ̒��S�ɌX�����~���W���v�Z
		if ( m_nG23 == 0 ) {
			for ( h=m_ptValS.y; sq>eq; sq-=ARCSTEP, h+=m_dHelicalStep ) {
				pt.x = rr * cos(sq) + ptOrg.x;
				pt.y = h;
				pt.z = rr * sin(sq) + ptOrg.y + m_dEndmill;
				qp = atan2(m_ptOrg.z-pt.z, m_ptOrg.x-pt.x);	// �~��XY���ʂɑ΂��ČX���p�x
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

	// �O�Տ�ɕ��Ԕ��~�܂��͉~���߲�ߏ�ɂȂ�
	_DrawEndmillPipe(vPath);
}
