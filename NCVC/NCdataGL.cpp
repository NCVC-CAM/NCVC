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
extern	const	PENSTYLE	g_penStyle[];	// ViewOption.cpp
extern	double	_TABLECOS[ARCCOUNT],		// NCVC.cpp
				_TABLESIN[ARCCOUNT];

//////////////////////////////////////////////////////////////////////

inline void _SetEndmillCircle(double r, const CPointD& ptOrg, CPointD* pt)
{
	// �~���W��ptc[]�ɾ��
	for ( int i=0; i<ARCCOUNT; i++ ) {
		pt[i].x = r * _TABLECOS[i] + ptOrg.x;
		pt[i].y = r * _TABLESIN[i] + ptOrg.y;
	}
	// �Ō�̕��W�͕`�掞�ɐݒ�
}

inline void _SetEndmillSphere(double d, const CPoint3D& ptOrg,
							  CPoint3D ptResult[ARCCOUNT/4][ARCCOUNT])
{
	int		i, j, k = 0;
	double	x, z;
	CPointD	ptc[ARCCOUNT], ptOrgXY(ptOrg.GetXY());

	// �������W�AZ�����ɗ֐؂�
	// 90��-1�񕪂�Z�����ɌJ��Ԃ�
	// --- 0�x
	x = d;				// ����Z�ʒu�̔��a cos(0)==1
	z = d + ptOrg.z;	//                 sin(0)==0
	_SetEndmillCircle(x, ptOrgXY, ptc);	// ���̈ʒu��XY���ʂ̉~���W��o�^
	for ( j=0; j<ARCCOUNT; j++ ) {
		ptResult[k][j].x = ptc[j].x;
		ptResult[k][j].y = ptc[j].y;
		ptResult[k][j].z = z;
	}
	// --- �c��270�x��O�܂�
	for ( i=ARCCOUNT-1, k++; i>ARCCOUNT-ARCCOUNT/4; i--, k++ ) {
		x = d * _TABLECOS[i];
		z = d * _TABLESIN[i] + d + ptOrg.z;
		_SetEndmillCircle(x, ptOrgXY, ptc);
		for ( j=0; j<ARCCOUNT; j++ ) {
			ptResult[k][j].x = ptc[j].x;
			ptResult[k][j].y = ptc[j].y;
			ptResult[k][j].z = z;
		}
	}
	// 270�x���_�͕ʏ���
}

inline void _SetEndmillSpherePath
	(double d, double qp, const CPoint3D& ptOrg, CPoint3D* ptResult)
{
	int		i, j = 0;
	double	x,
			cos_qp = cos(qp),
			sin_qp = sin(qp);
	CPoint3D	pt;

	// Z�����ւ̔��~�@�艺��
	// --- 0�x
	// XZ���ʂōl����(y=0)
	x = pt.z = d;
	// XY���ʂł̉�]
	pt.x = x * cos_qp;	// - y * sin_qp;
	pt.y = x * sin_qp;	// + y * cos_qp;
	ptResult[j++] = pt + ptOrg;
	// --- �c��180�x�܂�
	for ( i=ARCCOUNT-1; i>=ARCCOUNT/2; i-- ) {
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
	::glVertex3d(ptOrg.x, ptOrg.y, ptOrg.z);
	for ( int i=0; i<ARCCOUNT; i++ )
		::glVertex3d(ptc[i].x, ptc[i].y, ptOrg.z);
	::glVertex3d(ptc[0].x, ptc[0].y, ptOrg.z);
	::glEnd();
}

inline void _DrawBottomFaceSphere(const CPoint3D ptSphere[ARCCOUNT/4][ARCCOUNT], const CPoint3D& ptVtx)
{
	int		i, j;

	for ( i=0; i<ARCCOUNT/4-1; i++ ) {	// �������_������
		::glBegin(GL_TRIANGLE_STRIP);
		for ( j=0; j<ARCCOUNT; j++ ) {
			::glVertex3d(ptSphere[i  ][j].x, ptSphere[i  ][j].y, ptSphere[i  ][j].z);
			::glVertex3d(ptSphere[i+1][j].x, ptSphere[i+1][j].y, ptSphere[i+1][j].z);
		}
		::glVertex3d(ptSphere[i  ][0].x, ptSphere[i  ][0].y, ptSphere[i  ][0].z);
		::glVertex3d(ptSphere[i+1][0].x, ptSphere[i+1][0].y, ptSphere[i+1][0].z);
		::glEnd();
	}
	::glBegin(GL_TRIANGLE_FAN);
	::glVertex3d(ptVtx.x, ptVtx.y, ptVtx.z);	// �������_
	for ( j=0; j<ARCCOUNT; j++ )
		::glVertex3d(ptSphere[i][j].x, ptSphere[i][j].y, ptSphere[i][j].z);
	::glVertex3d(ptSphere[i][0].x, ptSphere[i][0].y, ptSphere[i][0].z);
	::glEnd();
}

inline void _DrawEndmillPipe(const vector<CVCircle>& vPipe)
{
	size_t		i, j;
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

void CNCdata::DrawGLMillWire(void) const
{
	// �h���׽����̋��ʌĂяo��
	for ( int i=0; i<m_obCdata.GetSize(); i++ )
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
	for ( int i=0; i<m_obCdata.GetSize(); i++ )
		m_obCdata[i]->DrawGLBottomFace();
}

int CNCdata::AddGLWireFirstVertex(vector<GLfloat>& vVertex, vector<GLfloat>& vNormal) const
{
	if ( !m_pWireObj )
		return -1;

	CPoint3D	pts(m_pWireObj->GetStartPoint());

	// ��޼ު�Ďn�_��o�^
	vVertex.push_back((GLfloat)m_ptValS.x);
	vVertex.push_back((GLfloat)m_ptValS.y);
	vVertex.push_back((GLfloat)m_ptValS.z);
	vVertex.push_back((GLfloat)pts.x);
	vVertex.push_back((GLfloat)pts.y);
	vVertex.push_back((GLfloat)pts.z);

	// �@���޸��
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
		// �ی�
		vNormal.push_back(1.0f);
		vNormal.push_back(1.0f);
		vNormal.push_back(1.0f);
		vNormal.push_back(1.0f);
		vNormal.push_back(1.0f);
		vNormal.push_back(1.0f);
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
// CNCline �N���X
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
			::glVertex3d(m_ptValS.x, m_ptValS.y, m_ptValS.z);
			::glVertex3d(m_ptValE.x, m_ptValE.y, m_ptValE.z);
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
		::glVertex3d(m_ptValS.x, m_ptValS.y, m_ptValS.z);
		::glVertex3d(m_ptValE.x, m_ptValE.y, m_ptValE.z);
		if ( m_pWireObj ) {
			// UV
			CPoint3D	pts(m_pWireObj->GetStartPoint()),
						pte(m_pWireObj->GetEndPoint());
			::glVertex3d(pts.x,      pts.y,      pts.z);
			::glVertex3d(pte.x,      pte.y,      pte.z);
			// XY��UV�̐ڑ�
			::glVertex3d(m_ptValS.x, m_ptValS.y, m_ptValS.z);
			::glVertex3d(pts.x,      pts.y,      pts.z);
			::glVertex3d(m_ptValE.x, m_ptValE.y, m_ptValE.z);
			::glVertex3d(pte.x,      pte.y,      pte.z);
		}
	::glEnd();
}

void CNCline::DrawGLLatheFace(void) const
{
	if ( m_nc.nGcode != 1 )
		return;

	::glBegin(GL_LINES);
	::glVertex3d(m_ptValS.x, m_ptValS.y, m_ptValS.z);
	::glVertex3d(m_ptValE.x, m_ptValE.y, m_ptValE.z);
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
		}
		// �I�_���̕`��
		_DrawBottomFaceCircle(m_ptValE, pte);
	}
	else {
		// �ްٴ����ٍ��W�v�Z
		CPoint3D	ptSphere[ARCCOUNT/4][ARCCOUNT];
		// �n�_�̔������W�v�Z
		_SetEndmillSphere(m_dEndmill, m_ptValS, ptSphere);
		_DrawBottomFaceSphere(ptSphere, m_ptValS);
		if ( m_dMove[NCA_X]>0 || m_dMove[NCA_Y]>0 ) {
			// �ްٴ����ق̈ړ��߽���W�v�Z
			CPoint3D	pts[ARCCOUNT/2+1], pte[ARCCOUNT/2+1];
			double	q = atan2(m_ptValE.y-m_ptValS.y, m_ptValE.x-m_ptValS.x) + RAD(90.0);
			_SetEndmillSpherePath(m_dEndmill, q, m_ptValS, pts);
			_SetEndmillSpherePath(m_dEndmill, q, m_ptValE, pte);
			::glBegin(GL_TRIANGLE_STRIP);
			for ( i=0; i<=ARCCOUNT/2; i++ ) {
				::glVertex3d(pts[i].x, pts[i].y, pts[i].z);
				::glVertex3d(pte[i].x, pte[i].y, pte[i].z);
			}
			::glEnd();
			// �I�_�̔������W�v�Z
			_SetEndmillSphere(m_dEndmill, m_ptValE, ptSphere);
			_DrawBottomFaceSphere(ptSphere, m_ptValE);
		}
	}
}

int CNCline::AddGLWireVertex(vector<GLfloat>& vVertex, vector<GLfloat>& vNormal) const
{
	if ( m_nc.nGcode!=1 || !m_pWireObj )
		return -1;		// �ړ����� => ��ڲ�

	CPoint3D	pte(m_pWireObj->GetEndPoint());

	// ��޼ު�ďI�_��o�^
	vVertex.push_back((GLfloat)m_ptValE.x);
	vVertex.push_back((GLfloat)m_ptValE.y);
	vVertex.push_back((GLfloat)m_ptValE.z);
	vVertex.push_back((GLfloat)pte.x);
	vVertex.push_back((GLfloat)pte.y);
	vVertex.push_back((GLfloat)pte.z);

	// �@���޸��
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
		vNormal.push_back(1.0f);	// �ی�
		vNormal.push_back(1.0f);
		vNormal.push_back(1.0f);
		vNormal.push_back(1.0f);
		vNormal.push_back(1.0f);
		vNormal.push_back(1.0f);
	}

	return 2;	// ���_��
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
// CNCcycle �N���X
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
		for ( i=0; i<m_nDrawCnt; i++ ) {
			// ���W�v�Z�ƕ`��
			_SetEndmillCircle(m_dEndmill, m_Cycle3D[i].ptC.GetXY(), ptc);
			_DrawBottomFaceCircle(m_Cycle3D[i].ptC, ptc);
		}
	}
	else {
		CPoint3D	ptSphere[ARCCOUNT/4][ARCCOUNT];
		for ( i=0; i<m_nDrawCnt; i++ ) {
			_SetEndmillSphere(m_dEndmill, m_Cycle3D[i].ptC, ptSphere);
			_DrawBottomFaceSphere(ptSphere, m_Cycle3D[i].ptC);
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
// CNCcircle �N���X
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
			DrawGLWire();	// ���W�w��
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
		// XY��UV�̐ڑ�
		CPoint3D	pts(m_pWireObj->GetStartPoint()),
					pte(m_pWireObj->GetEndPoint());
		::glBegin(GL_LINES);
			::glColor3ub( GetRValue(col), GetGValue(col), GetBValue(col) );
			::glVertex3d(m_ptValS.x, m_ptValS.y, m_ptValS.z);
			::glVertex3d(pts.x,      pts.y,      pts.z);
			::glVertex3d(m_ptValE.x, m_ptValE.y, m_ptValE.z);
			::glVertex3d(pte.x,      pte.y,      pte.z);
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
}

//	--- CNCcircle::DrawGLBottomFace() �T�u
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
	double	x,
			cos_qp = cos(qp),
			sin_qp = sin(qp);
	CPoint3D	pt;

	for ( int i=0; i<ARCCOUNT; i++ ) {
		// XY���ʂ̉~
		x    = d * _TABLECOS[i];
		pt.y = d * _TABLESIN[i];
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
	double	y,
			cos_qp = cos(qp),
			sin_qp = sin(qp);
	CPoint3D	pt;

	for ( int i=0; i<ARCCOUNT; i++ ) {
		// XY���ʂ̉~
		pt.x = d * _TABLECOS[i];
		y    = d * _TABLESIN[i];
		// Z�ł̉�]
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
		CPoint3D	ptSphere[ARCCOUNT/4][ARCCOUNT];
		// �n�_�E�I�_���ްٴ����ً�
		_SetEndmillSphere(m_dEndmill, m_ptValS, ptSphere);
		_DrawBottomFaceSphere(ptSphere, m_ptValS);
		_SetEndmillSphere(m_dEndmill, m_ptValE, ptSphere);
		_DrawBottomFaceSphere(ptSphere, m_ptValE);
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
				for ( i=0; i<=ARCCOUNT/2; i++ )	// ���~�������g�p
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

	// ���W�l���~����ɓo�^
	// �@���޸�ق� r - 1.0
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

	return nCnt+2;		// ���_��
}

int CNCcircle::AddGLWireTexture(int n, double& dAccuLength, double dAllLength, GLfloat* pfTEX) const
{
	if ( !m_pWireObj )
		return -1;

	int		nCnt = 0;
	GLfloat	f;
	double	sq = m_sq, r = fabs(m_r);

	// ø������W�̓o�^�́A��]�����͊֌W�Ȃ��A�����̊��������ŗǂ�
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
