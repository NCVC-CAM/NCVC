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
extern	const	PENSTYLE	g_penStyle[];		// ViewOption.cpp

//////////////////////////////////////////////////////////////////////
// NC�ް��̊�b�ް��׽
//////////////////////////////////////////////////////////////////////

void CNCdata::DrawGL(BOOL) const
{
}

void CNCdata::DrawSideFace(double) const
{
}

void CNCdata::DrawBottomFace(void) const
{
}

//////////////////////////////////////////////////////////////////////
// CNCline �N���X
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

void CNCline::SetEndmillPath(CPointD* pt1, CPointD* pt2, CPointD* pt3) const
{
	int		i;
	double	qs = atan2(m_ptValE.y-m_ptValS.y, m_ptValE.x-m_ptValS.x) + 90.0*RAD,
			qe = atan2(m_ptValS.y-m_ptValE.y, m_ptValS.x-m_ptValE.x) - 90.0*RAD;

	// �n�_�ʖʁC�I�_���ʂ̍��W�v�Z
	for ( i=0; i<=ARCCOUNT/2; i++, qs+=ARCSTEP, qe+=ARCSTEP ) {
		// �n�_�ʑ��� +90�����甽���v����
		pt1[i].x = m_dEndmill * cos(qs) + m_ptValS.x;
		pt1[i].y = m_dEndmill * sin(qs) + m_ptValS.y;
		// �I�_������ -90�����甽���v���
		pt2[i].x = m_dEndmill * cos(qe) + m_ptValE.x;
		pt2[i].y = m_dEndmill * sin(qe) + m_ptValE.y;
	}
	// �I�_���ʎc��̔��~���W�v�Z
	for ( i=0; i<ARCCOUNT/2; i++, qe+=ARCSTEP ) {
		pt3[i].x = m_dEndmill * cos(qe) + m_ptValE.x;
		pt3[i].y = m_dEndmill * sin(qe) + m_ptValE.y;
	}
}

void CNCline::DrawSideFace(double z) const
{
	if ( m_nc.nGcode != 1 )
		return;

	int		i;
	CPointD	pt1[ARCCOUNT/2+1], pt2[ARCCOUNT/2+1], pt3[ARCCOUNT/2];

	// ���W�v�Z
	SetEndmillPath(pt1, pt2, pt3);

	if ( m_dMove[NCA_X]>0 || m_dMove[NCA_Y]>0 ) {
		// ���ʕ`��(�����Ėʂ��؍��߽�̓���)
		::glBegin(GL_QUADS);
		// �i�s�����������ʁi�����v���Ɍq����j
		::glNormal3d(pt1[0].x, pt1[0].y, 0.0);
		::glVertex3d(pt1[0].x, pt1[0].y, m_ptValS.z);	// ����
		::glVertex3d(pt2[0].x, pt2[0].y, m_ptValE.z);	// �E��
		::glVertex3d(pt2[0].x, pt2[0].y, z);			// �E��
		::glVertex3d(pt1[0].x, pt1[0].y, z);			// ����
		// �i�s�����E�����ʁi�����猩�Ĕ����v���j
		::glNormal3d(pt2[ARCCOUNT/2].x, pt2[ARCCOUNT/2].y, 0.0);
		::glVertex3d(pt2[ARCCOUNT/2].x, pt2[ARCCOUNT/2].y, m_ptValE.z);	// ����
		::glVertex3d(pt1[ARCCOUNT/2].x, pt1[ARCCOUNT/2].y, m_ptValS.z);	// �E��
		::glVertex3d(pt1[ARCCOUNT/2].x, pt1[ARCCOUNT/2].y, z);			// �E��
		::glVertex3d(pt2[ARCCOUNT/2].x, pt2[ARCCOUNT/2].y, z);			// ����
		::glEnd();
	}

	// �n�_�ʑ���
//	::glBegin(GL_QUAD_STRIP);
	::glBegin(GL_TRIANGLE_STRIP);		// ������̕�������
	for ( i=ARCCOUNT/2; i>=0; i-- ) {
		::glNormal3d(pt1[i].x, pt1[i].y, 0.0);
		::glVertex3d(pt1[i].x, pt1[i].y, z);
		::glVertex3d(pt1[i].x, pt1[i].y, m_ptValS.z);
	}
	::glEnd();
	// �I�_������
//	::glBegin(GL_QUAD_STRIP);
	::glBegin(GL_TRIANGLE_STRIP);
	for ( i=0; i<=ARCCOUNT/2; i++ ) {
		::glNormal3d(pt2[i].x, pt2[i].y, 0.0);
		::glVertex3d(pt2[i].x, pt2[i].y, z);
		::glVertex3d(pt2[i].x, pt2[i].y, m_ptValE.z);
	}
	::glEnd();
}

void CNCline::DrawBottomFace(void) const
{
	if ( m_nc.nGcode != 1 )
		return;

	int		i;
	CPointD	pt1[ARCCOUNT/2+1], pt2[ARCCOUNT/2+1], pt3[ARCCOUNT/2];

	// ���W�v�Z
	SetEndmillPath(pt1, pt2, pt3);

	if ( m_dMove[NCA_X]>0 || m_dMove[NCA_Y]>0 ) {
		// ��ʕ`��(�@���޸�ق͏����)
//		::glBegin(GL_QUAD_STRIP);
		::glBegin(GL_TRIANGLE_STRIP);
		::glNormal3d(0.0, 0.0, 1.0);
		for ( i=0; i<=ARCCOUNT/2; i++ ) {
			::glVertex3d(pt2[i].x, pt2[i].y, m_ptValS.z);
			::glVertex3d(pt1[i].x, pt1[i].y, m_ptValE.z);
		}
		::glEnd();
	}

	// �I�_���ʂ̕`��
	::glBegin(GL_TRIANGLE_FAN);
	::glNormal3d(0.0, 0.0, 1.0);
	::glVertex3d(m_ptValE.x, m_ptValE.y, m_ptValE.z);
	for ( i=0; i<=ARCCOUNT/2; i++ )
		::glVertex3d(pt2[i].x, pt2[i].y, m_ptValE.z);
	for ( i=0; i< ARCCOUNT/2; i++ )
		::glVertex3d(pt3[i].x, pt3[i].y, m_ptValE.z);
	::glVertex3d(pt2[0].x, pt2[0].y, m_ptValE.z);
	::glEnd();
}

//////////////////////////////////////////////////////////////////////
// CNCcycle �N���X
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

void CNCcycle::SetEndmillPath(const CPointD& ptOrg, CPointD* pt) const
{
	int		i;
	double	q;

	// �~���W��āi�����ʂ�ۏ؁j
	pt[0].x = m_dEndmill + ptOrg.x;		// cos(0) == 1
	pt[0].y = ptOrg.y;					// sin(0) == 0
	for ( i=1, q=ARCSTEP; i<ARCCOUNT; i++, q+=ARCSTEP ) {
		pt[i].x = m_dEndmill * cos(q) + ptOrg.x;
		pt[i].y = m_dEndmill * sin(q) + ptOrg.y;
	}
	pt[i] = pt[0];
}

void CNCcycle::DrawSideFace(double z) const
{
	int		i, j;
	CPointD	pt[ARCCOUNT+1];

	for ( i=0; i<m_nDrawCnt; i++ ) {
		// ���W�v�Z
		SetEndmillPath(m_Cycle3D[i].ptC, pt);
		// ���ʁi�����������Ėʂɂ��邽�߂ɔ���ٰ�߁j
//		::glBegin(GL_QUAD_STRIP);
		::glBegin(GL_TRIANGLE_STRIP);
		for ( j=ARCCOUNT; j>=0; j-- ) {
			::glNormal3d(pt[j].x, pt[j].y, 0.0);
			::glVertex3d(pt[j].x, pt[j].y, z);
			::glVertex3d(pt[j].x, pt[j].y, m_Cycle3D[i].ptC.z);
		}
		::glEnd();
	}
}

void CNCcycle::DrawBottomFace(void) const
{
	int		i, j;
	CPointD	pt[ARCCOUNT+1];

	for ( i=0; i<m_nDrawCnt; i++ ) {
		// ���W�v�Z
		SetEndmillPath(m_Cycle3D[i].ptC, pt);
		// ��ʕ`��
		::glBegin(GL_TRIANGLE_FAN);
		::glNormal3d(0.0, 0.0, 1.0);
		::glVertex3d(m_Cycle3D[i].ptC.x, m_Cycle3D[i].ptC.y, m_Cycle3D[i].ptC.z);	// ���S
		for	( j=0; j<=ARCCOUNT; j++ )
			::glVertex3d(pt[j].x, pt[j].y, m_Cycle3D[i].ptC.z);
		::glEnd();
	}
}

//////////////////////////////////////////////////////////////////////
// CNCcircle �N���X
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
		pt.z = m_ptOrg.z;	// �ضيJ�n���W
		// ARCSTEP �Â��א����ŕ`��
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
		// �[�����`��
		pt.x = r * cos(eq) + m_ptOrg.x;
		pt.y = r * sin(eq) + m_ptOrg.y;
		pt.z = m_ptValE.z;		// �ضُI�����W
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

void CNCcircle::SetEndmillPath(vector<CPointD>& vt1, vector<CPointD>& vt2, CPointD* pt3) const
{
	int		i;
	double	sq, eq, qs, cos_q, sin_q, r1, r2, rr = fabs(m_r);
	CPointD	pt1, pt2;

	// �~�ʕ�Ԑ؍��߽���W
	if ( m_nG23 == 0 ) {
		sq = qs = m_eq;
		eq = m_sq;
		r1 = rr + m_dEndmill;	// �i�s��������
		r2 = rr - m_dEndmill;	// �i�s�����E��
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
		sq = qs = m_sq;
		eq = m_eq;
		r1 = rr - m_dEndmill;	// �i�s��������
		r2 = rr + m_dEndmill;	// �i�s�����E��
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
	// �n�_�ʑ�
	sq = qs;
	if ( m_nG23 == 1 )
		sq += 180.0*RAD;
	pt3[0] = vt1.front();
	for ( i=1; i<ARCCOUNT/2; i++, sq+=ARCSTEP ) {
		pt3[i].x = m_dEndmill * cos(sq) + m_ptValS.x;
		pt3[i].y = m_dEndmill * sin(sq) + m_ptValS.y;
	}
	pt3[i] = vt2.front();
}

void CNCcircle::DrawSideFace(double z) const
{
	int		i;
	double	sz;
	vector<CPointD>	vt1, vt2;
	CPointD	pt, pt3[ARCCOUNT/2+1];

	// ���W�v�Z
	SetEndmillPath(vt1, vt2, pt3);

	// ���ʍ���
//	::glBegin(GL_QUAD_STRIP);
	::glBegin(GL_TRIANGLE_STRIP);
	::glNormal3d(0.0, 0.0, 1.0);
	for ( i=0, sz=m_ptOrg.z; i<(int)vt1.size(); i++, sz+=m_dHelicalStep ) {
		pt = vt1[i];
		::glVertex3d(pt.x, pt.y,  z);
		::glVertex3d(pt.x, pt.y, sz);
	}
	::glEnd();
	// ���ʉE��
//	::glBegin(GL_QUAD_STRIP);
	::glBegin(GL_TRIANGLE_STRIP);
	::glNormal3d(0.0, 0.0, 1.0);
	for ( i=0, sz=m_ptOrg.z; i<(int)vt2.size(); i++, sz+=m_dHelicalStep ) {
		pt = vt2[i];
		::glVertex3d(pt.x, pt.y, sz);
		::glVertex3d(pt.x, pt.y,  z);
	}
	::glEnd();
	// �n�_�ʑ���
//	::glBegin(GL_QUAD_STRIP);
	::glBegin(GL_TRIANGLE_STRIP);
	for ( i=ARCCOUNT/2; i>=0; i-- ) {
		::glNormal3d(pt3[i].x, pt3[i].y, 0.0);
		::glVertex3d(pt3[i].x, pt3[i].y, z);
		::glVertex3d(pt3[i].x, pt3[i].y, m_ptOrg.z);
	}
	::glEnd();
}

void CNCcircle::DrawBottomFace(void) const
{
	size_t	i;
	double	sz;
	vector<CPointD>	vt1, vt2;
	CPointD	pt1, pt2, pt3[ARCCOUNT/2+1];

	// ���W�v�Z
	SetEndmillPath(vt1, vt2, pt3);

	// ��ʂ̎n�_�ʑ�
	::glBegin(GL_TRIANGLE_FAN);
	::glNormal3d(0.0, 0.0, 1.0);
	::glVertex3d(m_ptValS.x, m_ptValS.y, m_ptOrg.z);
	for ( i=0; i<=ARCCOUNT/2; i++ )
		::glVertex3d(pt3[i].x, pt3[i].y, m_ptOrg.z);
	::glEnd();
	// ���
//	::glBegin(GL_QUAD_STRIP);
	::glBegin(GL_TRIANGLE_STRIP);
	::glNormal3d(0.0, 0.0, 1.0);
	for ( i=0, sz=m_ptOrg.z; i<vt1.size() && i<vt2.size(); i++, sz+=m_dHelicalStep ) {
		pt1 = vt1[i];
		pt2 = vt2[i];
		::glVertex3d(pt1.x, pt1.y, sz);
		::glVertex3d(pt2.x, pt2.y, sz);
	}
	::glEnd();
}
