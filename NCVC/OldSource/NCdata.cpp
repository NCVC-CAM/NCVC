// NCdata.cpp: CNCdata �N���X�̃C���v�������e�[�V����
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NCVC.h"
#include "NCdata.h"
#include "ViewOption.h"

#include <math.h>
#include <float.h>

//#define	_DEBUGDRAW_NCD		// �`�揈����۸�
#include "MagaDbgMac.h"
#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

// CalcRoundPoint() ���
// --- �~���m�̓��O���a�v�Z
static	double	CalcRoundPoint_CircleInOut(const CPointD&, const CPointD&, int, int, double);

//////////////////////////////////////////////////////////////////////
// NC�ް��̊�b�ް��׽
//////////////////////////////////////////////////////////////////////

// ����o�^�p�ݽ�׸�
CNCdata::CNCdata(LPNCARGV lpArgv)
{
	int		i;
	Constracter(lpArgv);
	for ( i=0; i<VALUESIZE; i++ )
		m_nc.dValue[i] = lpArgv->nc.dValue[i];
	for ( i=0; i<NCXYZ; i++ ) {
		m_ptValS[i] = m_ptValE[i] = m_nc.dValue[i];
		m_dMove[i] = 0.0;
	}
	m_nc.dLength = 0.0;
	m_pt2D = 0.0;
	m_enType = NCDBASEDATA;
}

// �؍�(�`��)���ވȊO�̺ݽ�׸�
CNCdata::CNCdata(const CNCdata* pData, LPNCARGV lpArgv)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CNCdata", DBG_MAGENTA);
#endif
	// �w�肳�ꂽ�l���׸�
	extern	const	DWORD	g_dwSetValFlags[];
	int		i;

	Constracter(lpArgv);
	// ���W�w��̂Ȃ��ް��͑O����W����擾
	for ( i=0; i<NCXYZ; i++ ) {
		// �w�肳��Ă��镪�������
		if ( m_nc.dwValFlags & g_dwSetValFlags[i] ) {
			m_nc.dValue[i] = lpArgv->nc.dValue[i];
			m_ptValS[i] = m_ptValE[i] =
				m_nc.nGcode==92 ? lpArgv->nc.dValue[i] : pData->GetEndValue(i);
		}
		else
			m_ptValS[i] = m_ptValE[i] = m_nc.dValue[i] = pData->GetEndValue(i);
		m_dMove[i] = 0.0;
	}
	// ���W�l�ȊO�͂��̂܂܈����p��
	for ( ; i<VALUESIZE; i++ )
		m_nc.dValue[i] = pData->GetValue(i);
	m_nc.dLength = 0.0;
	// G92�Ȃ�V�K�v�Z�C�łȂ���ΑO��̌v�Z�l�������p��
	m_pt2D = m_nc.nGcode==92 ? m_ptValE.PointConvert() : pData->Get2DPoint();
	m_enType = NCDBASEDATA;

#ifdef _DEBUG
	DbgDump();
	Dbg_sep();
#endif
}

// �h���׽�p�ݽ�׸�
CNCdata::CNCdata(ENNCDTYPE enType, const CNCdata* pData, LPNCARGV lpArgv)
{
	extern	const	DWORD	g_dwSetValFlags[];
	int	i;

	Constracter(lpArgv);
	// ���W�w��̂Ȃ��ް��͑O����W����v�Z
	for ( i=0; i<NCXYZ; i++ ) {
		// �w�肳��Ă���Ƃ��������
		if ( m_nc.dwValFlags & g_dwSetValFlags[i] )
			m_nc.dValue[i] = lpArgv->bAbs ?		// ���W�̕␳
				lpArgv->nc.dValue[i] : (pData->GetEndValue(i)+lpArgv->nc.dValue[i]);
		else
			m_nc.dValue[i] = pData->GetEndValue(i);
	}
	for ( ; i<VALUESIZE; i++ )
		m_nc.dValue[i] = m_nc.dwValFlags & g_dwSetValFlags[i] ?
			lpArgv->nc.dValue[i] : 0.0;
/*
	m_ptValS, m_ptValE,
	m_nc.dLength, m_pt2D, m_rcMax �͔h���׽�ő��
*/
	m_enType = enType;
}

CPointD CNCdata::GetPlaneValue(const CPoint3D& ptVal)
{
	CPointD	pt;
	switch ( GetPlane() ) {
	case XY_PLANE:
		pt = ptVal.GetXY();
		break;
	case XZ_PLANE:
		pt = ptVal.GetXZ();
		break;
	case YZ_PLANE:
		pt = ptVal.GetYZ();
		break;
	default:
		pt = HUGE_VAL;
	}
	return pt;
}

CPointD	CNCdata::GetPlaneValueOrg(const CPoint3D& pt1, const CPoint3D& pt2)
{
	CPointD	pt;
	switch ( GetPlane() ) {
	case XY_PLANE:
		pt = pt2.GetXY() - pt1.GetXY();
		break;
	case XZ_PLANE:
		pt = pt2.GetXZ() - pt1.GetXZ();
		break;
	case YZ_PLANE:
		pt = pt2.GetYZ() - pt1.GetYZ();
		break;
	default:
		pt = HUGE_VAL;
	}
	return pt;
}

#ifdef _DEBUG
void CNCdata::DbgDump(void)
{
	CMagaDbg	dbg("CNCdata", DBG_MAGENTA);
	extern	const	DWORD	g_dwSetValFlags[];
	extern	LPCTSTR	g_szGtester[];
	extern	LPCTSTR	g_szNtester[];

	CString	strBuf, strTmp;
	if ( GetGtype()<0 || GetGtype()>GTYPESIZE )
		strBuf.Format("%s%d: ", "NO_TYPE:", GetGcode());
	else
		strBuf.Format("%s%d: ", g_szGtester[GetGtype()], GetGcode());
	for ( int i=0; i<VALUESIZE; i++ ) {
		if ( GetValFlags() & g_dwSetValFlags[i] ) {
			strTmp.Format("%s%.3f", g_szNtester[i], GetValue(i));
			strBuf += strTmp;
		}
	}
	dbg.printf("%s", strBuf);
}
#endif

//////////////////////////////////////////////////////////////////////
// CNCline �N���X
//////////////////////////////////////////////////////////////////////
CNCline::CNCline(const CNCdata* pData, LPNCARGV lpArgv) :
		CNCdata(NCDLINEDATA, pData, lpArgv)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CNCline", DBG_MAGENTA);
#endif
	// �`��n�_��O��̌v�Z�l����擾
	m_pts = pData->Get2DPoint();
	m_ptValS = pData->GetEndPoint();
	// �ŏI���W(==�w����W)���
	m_ptValE.SetPoint(GetValue(NCA_X), GetValue(NCA_Y), GetValue(NCA_Z));
	// �ړ����C�؍풷�̌v�Z
	CalcLength();
	// �`��I�_���v�Z���ۑ�
	m_pt2D = m_pte = m_ptValE.PointConvert();

	// ��Ԑ�L��`
	SetMaxRect();

#ifdef _DEBUG
	DbgDump();
	dbg.printf("Length = %f", m_nc.dLength);
	Dbg_sep();
#endif
}

void CNCline::DrawTuning(double f)
{
	m_ptDrawS = m_pts * f;
	m_ptDrawE = m_pte * f;
}

void CNCline::DrawTuningXY(double f)
{
	m_ptDrawS_XY = m_ptValS.GetXY() * f;
	m_ptDrawE_XY = m_ptValE.GetXY() * f;
}

void CNCline::DrawTuningXZ(double f)
{
	m_ptDrawS_XZ = m_ptValS.GetXZ() * f;
	m_ptDrawE_XZ = m_ptValE.GetXZ() * f;
}

void CNCline::DrawTuningYZ(double f)
{
	m_ptDrawS_YZ = m_ptValS.GetYZ() * f;
	m_ptDrawE_YZ = m_ptValE.GetYZ() * f;
}

void CNCline::Draw(CDC* pDC)
{
#ifdef _DEBUGDRAW_NCD
	CMagaDbg	dbg("CNCline::Draw()", DBG_RED);
	dbg.printf("Line=%d", GetStrLine());
#endif
	CPen* pOldPen = pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenNC(GetPenType()));
	pDC->MoveTo(m_ptDrawS);
	pDC->LineTo(m_ptDrawE);
	pDC->SelectObject(pOldPen);
}

void CNCline::DrawXY(CDC* pDC)
{
#ifdef _DEBUGDRAW_NCD
	CMagaDbg	dbg("CNCline::DrawXY()", DBG_RED);
	dbg.printf("Line=%d", GetStrLine());
#endif
	CPen* pOldPen = pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenNC(GetPenType()));
	pDC->MoveTo(m_ptDrawS_XY);
	pDC->LineTo(m_ptDrawE_XY);
	pDC->SelectObject(pOldPen);
}

void CNCline::DrawXZ(CDC* pDC)
{
#ifdef _DEBUGDRAW_NCD
	CMagaDbg	dbg("CNCline::DrawXZ()", DBG_RED);
	dbg.printf("Line=%d", GetStrLine());
#endif
	CPen* pOldPen = pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenNC(GetPenType()));
	pDC->MoveTo(m_ptDrawS_XZ);
	pDC->LineTo(m_ptDrawE_XZ);
	pDC->SelectObject(pOldPen);
}

void CNCline::DrawYZ(CDC* pDC)
{
#ifdef _DEBUGDRAW_NCD
	CMagaDbg	dbg("CNCline::DrawYZ()", DBG_RED);
	dbg.printf("Line=%d", GetStrLine());
#endif
	CPen* pOldPen = pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenNC(GetPenType()));
	pDC->MoveTo(m_ptDrawS_YZ);
	pDC->LineTo(m_ptDrawE_YZ);
	pDC->SelectObject(pOldPen);
}

BOOL CNCline::CalcRoundPoint
	(const CNCdata* pNext, double r, CPointD& pt, double& rr1, double& rr2)
{
	// �Q���̌�_(���g�̏I�_)�����_�ɂȂ�悤�ɕ␳
	CPointD	pts( GetPlaneValueOrg(m_ptValE, m_ptValS) );

	if ( pNext->GetGcode() > 1 ) {
		CNCcircle*	pCircle = (CNCcircle *)pNext;
		CPointD	pto( GetPlaneValueOrg(m_ptValE, pCircle->GetOrg()) );
		// �̾�ĕ����s�ړ���������_�����߂�
		double	rr, xa, ya, rn = fabs(pCircle->GetR());
		pt = ::CalcOffsetIntersectionPoint(pts, pto, rn, r,
					pCircle->GetG23()==0 ? 1 : -1, 0, 0);
		if ( pt == HUGE_VAL )
			return FALSE;
		// �ʎ��ɑ�������C�l�̌v�Z
		rr1 = sqrt(pt.x*pt.x + pt.y*pt.y - r*r);
		rr = rn + r;
		if ( r > 0 ) {
			// �����_
			xa = (pt.x*rn+pto.x*r) / rr;
			ya = (pt.y*rn+pto.y*r) / rr;
		}
		else {
			// �O���_
			r = fabs(r);
			xa = (pt.x*rn-pto.x*r) / rr;
			ya = (pt.y*rn-pto.y*r) / rr;
		}
		rr2 = sqrt(xa*xa + ya*ya);
	}
	else {
		CPointD	pte( GetPlaneValueOrg(m_ptValE, pNext->GetEndPoint()) );
		// �̾�ĕ����s�ړ���������_�����߂�
		pt = ::CalcOffsetIntersectionPoint(pts, pte, 0, 0, r);
		if ( pt == HUGE_VAL )
			return FALSE;
		// �ʎ��ɑ�������C�l�̌v�Z
		rr1 = rr2 = sqrt(pt.x*pt.x + pt.y*pt.y - r*r);
	}

	// ���_�␳
	pt += GetPlaneValue(m_ptValE);

	return TRUE;
}

BOOL CNCline::SetChamferingPoint(BOOL bStart, double c, CPointD& pt)
{
	// ����������Ȃ��Ƃ��ʹװ
	if ( c >= GetCutLength() )
		return FALSE;

	CPoint3D&	ptValS = bStart ? m_ptValS : m_ptValE,
				ptValE = bStart ? m_ptValE : m_ptValS;
	CPointD		pto, pte;
	switch ( GetPlane() ) {
	case XY_PLANE:
		pto = ptValS.GetXY();
		pte = ptValE.GetXY();
		break;
	case XZ_PLANE:
		pto = ptValS.GetXZ();
		pte = ptValE.GetXZ();
		break;
	case YZ_PLANE:
		pto = ptValS.GetYZ();
		pte = ptValE.GetYZ();
		break;
	default:
		return FALSE;
	}

	// pto �𒆐S�Ƃ����~�� pte �̌�_
	pt = ::CalcIntersectionPoint(pto, c, pte);

	// �������g�̓_���X�V
	switch ( GetPlane() ) {
	case XY_PLANE:
		ptValS.x = pt.x;
		ptValS.y = pt.y;
		break;
	case XZ_PLANE:
		ptValS.x = pt.x;
		ptValS.z = pt.y;
		break;
	case YZ_PLANE:
		ptValS.y = pt.x;
		ptValS.z = pt.y;
		break;
	}
	if ( bStart )
		m_pts = m_ptValS.PointConvert();
	else
		m_pt2D = m_pte = m_ptValE.PointConvert();
	// �ړ����C�؍풷�̌v�Z
	CalcLength();

	return TRUE;
}

double CNCline::CalcBetweenAngle(const CPoint3D& pts, const CNCdata* pNext)
{
	// �Q���̌�_(���g�̏I�_)�����_�ɂȂ�悤�ɕ␳
	CPoint3D	pto(GetValue(NCA_X), GetValue(NCA_Y), GetValue(NCA_Z));
	CPointD		pt1( GetPlaneValueOrg(pto, pts) ), pt2;

	if ( pNext->GetGcode() > 1 ) {
		CNCcircle*	pCircle = (CNCcircle *)pNext;
		// ���̵�޼ު�Ă��~�ʂȂ璆�S��Ă�
		CPointD	pt( GetPlaneValueOrg(pto, pCircle->GetOrg()) );
		// �ڐ��v�Z
		int k = pCircle->GetG23()==0 ? 1 : -1;	// G02
		pt2.x = -pt.y*k;	// G02:+90��
		pt2.y =  pt.x*k;	// G03:-90��
	}
	else {
		CPoint3D	pte(pNext->GetValue(NCA_X), pNext->GetValue(NCA_Y), pNext->GetValue(NCA_Z));
		pt2 = GetPlaneValueOrg(pto, pte);
	}
	
	// �Q��(�܂��͉~�ʂ̐ڐ�)���Ȃ��p�x�����߂�
	return ::CalcBetweenAngleTwoLines(pt1, pt2);
}

int CNCline::CalcOffsetSign(const CPoint3D& pts)
{
	// �n�_�����_�ɐi�s�����̊p�x���v�Z
	CPoint3D	pte(GetValue(NCA_X), GetValue(NCA_Y), GetValue(NCA_Z));
	CPointD		pt( GetPlaneValueOrg(pts, pte) );
	double	dAngle = atan2(pt.y, pt.x);
	if ( dAngle < 0.0 )
		dAngle += 360.0*RAD;

	// G41�̕␳���������߂�
	return 90.0*RAD<=dAngle && dAngle<270.0*RAD ? -1 : 1;
}

CPointD CNCline::CalcPerpendicularPoint
	(const CPoint3D& pts, const CPoint3D& pte, double r, int nSign)
{
	// ���̌X�����v�Z����90����]
	CPointD	pt( GetPlaneValueOrg(pts, pte) );
	double	q = atan2(pt.y, pt.x);
	CPointD	pt1(r*cos(q), r*sin(q));
	CPointD	pt2(-pt1.y*nSign, pt1.x*nSign);
	switch ( GetPlane() ) {
	case XY_PLANE:
		pt2 += pts.GetXY();
		break;
	case XZ_PLANE:
		pt2 += pts.GetXZ();
		break;
	case YZ_PLANE:
		pt2 += pts.GetYZ();
		break;
	}

	return pt2;
}

CPointD CNCline::CalcOffsetIntersectionPoint
	(const CPoint3D& pts, const CNCdata* pNext, double r, int k1, int k2)
{
	// �Q���̌�_(���g�̏I�_)�����_�ɂȂ�悤�ɕ␳
	CPoint3D	pto(GetValue(NCA_X), GetValue(NCA_Y), GetValue(NCA_Z));
	CPointD		pt, pt1( GetPlaneValueOrg(pto, pts) ), pt2;

	// �̾�ĕ����s�ړ���������_�����߂�
	if ( pNext->GetGcode() > 1 ) {
		CNCcircle*	pCircle = (CNCcircle *)pNext;
		pt2 = GetPlaneValueOrg(pto, pCircle->GetOrg());
		pt = ::CalcOffsetIntersectionPoint(pt1, pt2, fabs(pCircle->GetR()), r,
					pCircle->GetG23()==0 ? 1 : -1, k1, k2);
	}
	else {
		CPoint3D	pte(pNext->GetValue(NCA_X), pNext->GetValue(NCA_Y), pNext->GetValue(NCA_Z));
		pt2 = GetPlaneValueOrg(pto, pte);
		pt = ::CalcOffsetIntersectionPoint(pt1, pt2, k1, k2, r);
	}

	// ���_�␳
	if ( pt != HUGE_VAL )
		pt += GetPlaneValue(pto);
	return pt;
}

CPointD CNCline::CalcOffsetIntersectionPoint2
	(const CPoint3D& pts, const CNCdata* pNext, double r, int k1, int k2)
{
	// �Q���̌�_(���g�̏I�_)�����_�ɂȂ�悤�ɕ␳
	CPoint3D	pto(GetValue(NCA_X), GetValue(NCA_Y), GetValue(NCA_Z));
	CPointD		pt, pt1( GetPlaneValueOrg(pto, pts) ), pt2;

	// �̾�ĕ����s�ړ���������_�����߂�
	if ( pNext->GetGcode() > 1 ) {
		// �~�ʂ͐ڐ��Ƃ̵̾�Č�_�v�Z
		CNCcircle*	pCircle = (CNCcircle *)pNext;
		pt = GetPlaneValueOrg(pto, pCircle->GetOrg());
		int k = pCircle->GetG23()==0 ? 1 : -1;
		pt2.x = -pt.y * k;
		pt2.y =  pt.x * k;
		// �ڐ��p�ɕ������Čv�Z
		double	dAngle = atan2(pt2.y, pt2.x);
		if ( dAngle < 0.0 )
			dAngle += 360.0*RAD;
		k2 = 90.0*RAD<=dAngle && dAngle<270.0*RAD ? -1 : 1;
	}
	else {
		CPoint3D	pte(pNext->GetValue(NCA_X), pNext->GetValue(NCA_Y), pNext->GetValue(NCA_Z));
		pt2 = GetPlaneValueOrg(pto, pte);
	}

	// �������m�̵̾�Č�_�v�Z
	pt = ::CalcOffsetIntersectionPoint(pt1, pt2, k1, k2, r);
	// ���_�␳
	if ( pt != HUGE_VAL )
		pt += GetPlaneValue(pto);
	return pt;
}

BOOL CNCline::SetCorrectPoint(BOOL bStart, const CPointD& pt, double)
{
	CPoint3D&	ptVal = bStart ? m_ptValS : m_ptValE;
	switch ( GetPlane() ) {
	case XY_PLANE:
		ptVal.x = pt.x;
		ptVal.y = pt.y;
		break;
	case XZ_PLANE:
		ptVal.x = pt.x;
		ptVal.z = pt.y;
		break;
	case YZ_PLANE:
		ptVal.y = pt.x;
		ptVal.z = pt.y;
		break;
	default:
		return FALSE;
	}

	if ( bStart )
		m_pts = m_ptValS.PointConvert();
	else
		m_pt2D = m_pte = m_ptValE.PointConvert();
	CalcLength();

	return TRUE;
}

//////////////////////////////////////////////////////////////////////
// CNCcycle �N���X
//////////////////////////////////////////////////////////////////////
CNCcycle::CNCcycle(const CNCdata* pData, LPNCARGV lpArgv) :
		CNCline(NCDCYCLEDATA, pData, lpArgv)
{
/*
	Z, R, P �l�́CTH_NCRead.cpp �ł���Ԃ��Ă��邱�Ƃɒ���
*/
#ifdef _DEBUG
	CMagaDbg	dbg("CNCcycle", DBG_MAGENTA);
#endif
	double		dx, dy,		// XY�e���̈ړ�����
				dR, dI,		// R�_���W, �Ƽ�ٍ��W
				dRLength, dZLength;		// �ړ����C�؍풷
	int			i, nH, nV;	// �c���̌J��Ԃ���

	m_Cycle = NULL;
	m_CycleXY = NULL;
	m_CycleXZ = NULL;
	m_CycleYZ = NULL;

	// �`��n�_��O��̌v�Z�l����擾
	m_pts = pData->Get2DPoint();
	m_ptValS = pData->GetEndPoint();
	// �c�J��Ԃ����擾
	if ( GetValFlags() & NCD_K )
		nV = max(0, (int)GetValue(NCA_K));
	else if ( GetValFlags() & NCD_L )
		nV = max(0, (int)GetValue(NCA_L));
	else
		nV = 1;
	// ���A���W(�O��̵�޼ު�Ă��Œ軲�ق��ǂ���)
	m_dInitial = pData->GetType()!=NCDCYCLEDATA ? m_ptValS.z :
							((CNCcycle *)pData)->GetInitialValue();
	// �ݸ����ٕ␳(R���W���ް��׽�ō��W�␳�̑ΏۊO)
	if ( lpArgv->bAbs ) {
		dR = GetValFlags() & NCD_R ? GetValue(NCA_R) : m_ptValS.z;
		m_nDrawCnt = nH = min(1, nV);	// ��޿ح�ĂȂ牡�ւ�(0 or 1)��̂�
	}
	else {
		dR = GetValFlags() & NCD_R ? m_dInitial + GetValue(NCA_R) : m_dInitial;
		// !!! Z�l��R�_����̲ݸ���Ăɕ␳ !!!
		if ( GetValFlags() & NCD_Z )
			m_nc.dValue[NCA_Z] = dR + lpArgv->nc.dValue[NCA_Z];
		m_nDrawCnt = nH = nV;	// �ݸ����قȂ牡�ւ��J��Ԃ�
	}
	dI = lpArgv->bInitial ? m_dInitial : dR;

	// �J��Ԃ�����ۂȂ�ȍ~�̌v�Z�͕s�v
	if ( m_nDrawCnt <= 0 ) {
		for ( i=0; i<NCXYZ; m_dMove[i++]=0.0 );
		m_dDwell = 0.0;
		m_nc.dLength = m_dCycleMove = 0.0;
		m_ptValE = m_ptValS;
		m_pt2D = m_pte = m_pts;
		return;
	}

	// �e�����Ƃ̈ړ����v�Z
	dx = GetValue(NCA_X) - m_ptValS.x;
	dy = GetValue(NCA_Y) - m_ptValS.y;
	dRLength = fabs(dI - dR);
	dZLength = fabs(dR - GetValue(NCA_Z));
	m_dMove[NCA_X] = fabs(dx) * nH;
	m_dMove[NCA_Y] = fabs(dy) * nH;
	m_dMove[NCA_Z] = fabs(m_ptValS.z - dR);	// ���񉺍~��
	m_dMove[NCA_Z] += dRLength * (nV-1);
	// �ړ����v�Z
	m_dCycleMove = sqrt(
			m_dMove[NCA_X]*m_dMove[NCA_X] +
			m_dMove[NCA_Y]*m_dMove[NCA_Y] );
	m_dCycleMove += m_dMove[NCA_Z];
	// �؍풷
	m_nc.dLength = dZLength * nV;
	// �e���W�l�̐ݒ�
	m_ptValE.SetPoint(m_ptValS.x + dx * nH, m_ptValS.y + dy * nH, dI);
	m_ptValI.SetPoint(GetValue(NCA_X), GetValue(NCA_Y), m_ptValS.z);	// �`��p
	m_ptValR.SetPoint(GetValue(NCA_X), GetValue(NCA_Y), dI);
	m_pte = m_ptValE.PointConvert();
	m_pt2D = m_pte;

	// Z���w�肳��Ă��Ȃ���δװ(��Ԃ�TH_NCRead.cpp�ɂ�)
	if ( !(GetValFlags() & NCD_Z) ) {
		SetNCFlags(NCF_ERROR);
		m_nDrawCnt = 0;
		m_dMove[NCA_Z] = 0.0;
		m_dDwell = 0.0;
		return;
	}

#ifdef _DEBUG
	dbg.printf("StartPoint x=%.3f y=%.3f z=%.3f",
		m_ptValS.x, m_ptValS.y, m_ptValS.z);
	dbg.printf("           R-Point=%.3f Z-Point=%.3f", dR, GetValue(NCA_Z));
	dbg.printf("FinalPoint x=%.3f y=%.3f z=%.3f",
		m_ptValE[NCA_X], m_ptValE[NCA_Y], m_ptValE[NCA_Z]);
	dbg.printf("m_nDrawCnt=%d", m_nDrawCnt);
#endif

	// ���W�i�[�̈�m��
	m_Cycle = new PTCYCLE[m_nDrawCnt];
	m_CycleXY = new PTCYCLE_XY[m_nDrawCnt];
	m_CycleXZ = new PTCYCLE[m_nDrawCnt];
	m_CycleYZ = new PTCYCLE[m_nDrawCnt];

	CPoint3D	pt(m_ptValS);	// ���݈ʒu�ŏ�����
	for ( i=0; i<m_nDrawCnt; i++ ) {
		pt.x += dx;		pt.y += dy;
#ifdef _DEBUG
		dbg.printf("           No.%d x=%.3f y=%.3f", i+1, pt.x, pt.y);
#endif
		// XYZ, XZ, YZ
		pt.z = dI;
		m_Cycle[i].ptI = pt.PointConvert();
		m_CycleXZ[i].ptI = pt.GetXZ();
		m_CycleYZ[i].ptI = pt.GetYZ();
		pt.z = dR;
		m_Cycle[i].ptR = pt.PointConvert();
		m_CycleXZ[i].ptR = pt.GetXZ();
		m_CycleYZ[i].ptR = pt.GetYZ();
		pt.z = GetValue(NCA_Z);
		m_Cycle[i].ptZ = pt.PointConvert();
		m_CycleXZ[i].ptZ = pt.GetXZ();
		m_CycleYZ[i].ptZ = pt.GetYZ();
		// XY
		m_CycleXY[i].pt = pt;	// Z����
	}
	
	// �㏸���̈ړ��E�؍풷�v�Z
	double	dResult;
	switch ( GetGcode() ) {
	case 84:	// R�_�܂Ő؍한�A�C�Ƽ�ٓ_�܂ő����蕜�A
	case 85:
	case 87:
	case 88:
	case 89:
		if ( lpArgv->bInitial ) {
			dResult = dRLength * nV;
			m_nc.dLength += dZLength * nV;
			m_dMove[NCA_Z] += dResult;
			m_dCycleMove += dResult;
		}
		else
			m_nc.dLength += dZLength * nV;
		break;
	default:	// ����ȊO�͑����蕜�A
		dResult = (dRLength + dZLength) * nV;
		m_dMove[NCA_Z] += dResult;
		m_dCycleMove += dResult;
		break;
	}

	// �޳�َ���
	if ( GetValFlags() & NCD_P &&
		(GetGcode()==82 || GetGcode()==88 || GetGcode()==89) )
		m_dDwell = GetValue(NCA_P) * nV;
	else
		m_dDwell = 0.0;

#ifdef _DEBUG
	DbgDump();
	dbg.printf("Move=%f Cut=%f", m_dCycleMove, m_nc.dLength);
	Dbg_sep();
#endif

	// ��Ԑ�L��`
	SetMaxRect();
}

CNCcycle::~CNCcycle()
{
	if ( m_Cycle )
		delete[] m_Cycle;
	if ( m_CycleXY )
		delete[] m_CycleXY;
	if ( m_CycleXZ )
		delete[]	m_CycleXZ;
	if ( m_CycleYZ )
		delete[] m_CycleYZ;
}

void CNCcycle::DrawTuning(double f)
{
	if ( m_nDrawCnt > 0 ) {
		CNCline::DrawTuning(f);
		m_ptDrawI = m_ptValI.PointConvert() * f;
		m_ptDrawR = m_ptValR.PointConvert() * f;
		for ( int i=0; i<m_nDrawCnt; i++ )
			m_Cycle[i].DrawTuning(f);
	}
}

void CNCcycle::DrawTuningXY(double f)
{
	if ( m_nDrawCnt > 0 ) {
		CNCline::DrawTuningXY(f);	// Z���W�͖��֌W
		for ( int i=0; i<m_nDrawCnt; i++ )
			m_CycleXY[i].DrawTuning(f);
	}
}

void CNCcycle::DrawTuningXZ(double f)
{
	if ( m_nDrawCnt > 0 ) {
		CNCline::DrawTuningXZ(f);
		m_ptDrawI_XZ = m_ptValI.GetXZ() * f;
		m_ptDrawR_XZ = m_ptValR.GetXZ() * f;
		for ( int i=0; i<m_nDrawCnt; i++ )
			m_CycleXZ[i].DrawTuning(f);
	}
}

void CNCcycle::DrawTuningYZ(double f)
{
	if ( m_nDrawCnt > 0 ) {
		CNCline::DrawTuningYZ(f);
		m_ptDrawI_YZ = m_ptValI.GetYZ() * f;
		m_ptDrawR_YZ = m_ptValR.GetYZ() * f;
		for ( int i=0; i<m_nDrawCnt; i++ )
			m_CycleYZ[i].DrawTuning(f);
	}
}

void CNCcycle::Draw(CDC* pDC)
{
#ifdef _DEBUGDRAW_NCD
	CMagaDbg	dbg("CNCcycle::Draw()", DBG_RED);
#endif
	if ( m_nDrawCnt > 0 ) {
#ifdef _DEBUGDRAW_NCD
		dbg.printf("Line=%d", GetStrLine());
#endif
		CPen* pOldPen = pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenNC(NCPEN_G0));
		// �O��ʒu����P�_�ڂ̌����H�܂ł̈ړ�
		pDC->MoveTo(m_ptDrawS);
		pDC->LineTo(m_ptDrawI);		// ���݈ʒu����P�_�ڂ̲Ƽ�ٓ_
		pDC->LineTo(m_ptDrawR);		// �Ƽ�ٓ_����R�_
		pDC->LineTo(m_ptDrawE);		// R�_����Ō�̌����H
		// Z�����̐؍�
		for ( int i=0; i<m_nDrawCnt; i++ ) {
			pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenNC(NCPEN_G0));
			pDC->MoveTo(m_Cycle[i].ptDrawI);
			pDC->LineTo(m_Cycle[i].ptDrawR);
			pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenNC(NCPEN_CYCLE));
			pDC->LineTo(m_Cycle[i].ptDrawZ);
		}
		pDC->SelectObject(pOldPen);
	}
#ifdef _DEBUGDRAW_NCD
	else
		dbg.printf("Line=%d NoDraw!", GetStrLine());
#endif
}

void CNCcycle::DrawXY(CDC* pDC)
{
#ifdef _DEBUGDRAW_NCD
	CMagaDbg	dbg("CNCcycle::DrawXY()", DBG_RED);
#endif
	if ( m_nDrawCnt > 0 ) {
#ifdef _DEBUGDRAW_NCD
		dbg.printf("Line=%d", GetStrLine());
#endif
		CNCline::DrawXY(pDC);
		CPen* pOldPen = (CPen *)(pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenNC(NCPEN_CYCLE)));
		CBrush* pOldBrush = (CBrush *)(pDC->SelectObject(AfxGetNCVCMainWnd()->GetBrushNC(NCBRUSH_CYCLEXY)));
		for ( int i=0; i<m_nDrawCnt; i++ )
			pDC->Ellipse(&m_CycleXY[i].rcDraw);
		pDC->SelectObject(pOldBrush);
		pDC->SelectObject(pOldPen);
	}
#ifdef _DEBUGDRAW_NCD
	else
		dbg.printf("Line=%d NoDraw!", GetStrLine());
#endif
}

void CNCcycle::DrawXZ(CDC* pDC)
{
#ifdef _DEBUGDRAW_NCD
	CMagaDbg	dbg("CNCcycle::DrawXZ()", DBG_RED);
#endif
	if ( m_nDrawCnt > 0 ) {
#ifdef _DEBUGDRAW_NCD
		dbg.printf("Line=%d", GetStrLine());
#endif
		CPen* pOldPen = pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenNC(NCPEN_G0));
		pDC->MoveTo(m_ptDrawS_XZ);
		pDC->LineTo(m_ptDrawI_XZ);
		pDC->LineTo(m_ptDrawR_XZ);
		pDC->LineTo(m_ptDrawE_XZ);
		for ( int i=0; i<m_nDrawCnt; i++ ) {
			pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenNC(NCPEN_G0));
			pDC->MoveTo(m_CycleXZ[i].ptDrawI);
			pDC->LineTo(m_CycleXZ[i].ptDrawR);
			pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenNC(NCPEN_CYCLE));
			pDC->LineTo(m_CycleXZ[i].ptDrawZ);
		}
		pDC->SelectObject(pOldPen);
	}
#ifdef _DEBUGDRAW_NCD
	else
		dbg.printf("Line=%d NoDraw!", GetStrLine());
#endif
}

void CNCcycle::DrawYZ(CDC* pDC)
{
#ifdef _DEBUGDRAW_NCD
	CMagaDbg	dbg("CNCcycle::DrawYZ()", DBG_RED);
#endif
	if ( m_nDrawCnt > 0 ) {
#ifdef _DEBUGDRAW_NCD
		dbg.printf("Line=%d", GetStrLine());
#endif
		CPen* pOldPen = pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenNC(NCPEN_G0));
		pDC->MoveTo(m_ptDrawS_YZ);
		pDC->LineTo(m_ptDrawI_YZ);
		pDC->LineTo(m_ptDrawR_YZ);
		pDC->LineTo(m_ptDrawE_YZ);
		for ( int i=0; i<m_nDrawCnt; i++ ) {
			pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenNC(NCPEN_G0));
			pDC->MoveTo(m_CycleYZ[i].ptDrawI);
			pDC->LineTo(m_CycleYZ[i].ptDrawR);
			pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenNC(NCPEN_CYCLE));
			pDC->LineTo(m_CycleYZ[i].ptDrawZ);
		}
		pDC->SelectObject(pOldPen);
	}
#ifdef _DEBUGDRAW_NCD
	else
		dbg.printf("Line=%d NoDraw!", GetStrLine());
#endif
}

//////////////////////////////////////////////////////////////////////
// CNCcircle �N���X
//////////////////////////////////////////////////////////////////////
CNCcircle::CNCcircle(const CNCdata* pData, LPNCARGV lpArgv) :
		CNCdata(NCDARCDATA, pData, lpArgv)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CNCcircle", DBG_MAGENTA);
#endif
	BOOL		fError;

	m_nG23 = GetGcode() - 2;	// G2=0, G3=1
/*
	XZ���ʂ�(Z->X, X->Y)�Ȃ̂ł��̂܂܌v�Z����� -90����]������K�v������
	�ȒP�ɑΉ�����ɂ͕����𔽑΂ɂ���΂悢 -> ����������ƃ}�V�ȑΉ����I
*/
	if ( GetPlane() == XZ_PLANE )
		m_nG23 = 1 - m_nG23;	// 0->1 , 1->0;

	m_ptValS = pData->GetEndPoint();
	m_ptValE.SetPoint(GetValue(NCA_X), GetValue(NCA_Y), GetValue(NCA_Z));
	CPointD	pts( m_ptValS ), pte( m_ptValE ), pto;

	// ���a�ƒ��S���W�̌v�Z(R�D��)
	if ( GetValFlags() & NCD_R ) {
		m_r = GetValue(NCA_R);
		fError = CalcCenter(pts, pte);
	}
	else if ( GetValFlags() & (NCD_I|NCD_J|NCD_K) ) {
		m_ptOrg = m_ptValS;
		fError = FALSE;
		switch ( GetPlane() ) {
		case XY_PLANE:
			m_r = hypot(GetValue(NCA_I), GetValue(NCA_J));
			m_ptOrg.x += GetValue(NCA_I);
			m_ptOrg.y += GetValue(NCA_J);
			pto = m_ptOrg.GetXY();
			break;
		case XZ_PLANE:
			m_r = hypot(GetValue(NCA_I), GetValue(NCA_K));
			m_ptOrg.x += GetValue(NCA_I);
			m_ptOrg.z += GetValue(NCA_K);
			pto = m_ptOrg.GetXZ();
			break;
		case YZ_PLANE:
			m_r = hypot(GetValue(NCA_J), GetValue(NCA_K));
			m_ptOrg.y += GetValue(NCA_J);
			m_ptOrg.z += GetValue(NCA_K);
			pto = m_ptOrg.GetYZ();
			break;
		default:
			fError = TRUE;
		}
		if ( !fError ) {
			pts -= pto;
			pte -= pto;
			AngleTuning(pts, pte);
		}
	}
	else {
		fError = TRUE;
	}

	// �`��֐��̌�����ضوړ��ʂ̌v�Z
	switch ( GetPlane() ) {
	case XY_PLANE:
		m_pfnCircleDraw = Draw_G17;		// �u&�v�͕s�v
		m_dHelicalStep = GetValFlags() & NCD_Z ?
			(m_ptValE.z - m_ptValS.z) / ((m_eq-m_sq)/ARCSTEP) : 0.0;
		break;
	case XZ_PLANE:
		m_pfnCircleDraw = Draw_G18;
		m_dHelicalStep = GetValFlags() & NCD_Y ?
			(m_ptValE.y - m_ptValS.y) / ((m_eq-m_sq)/ARCSTEP) : 0.0;
		break;
	case YZ_PLANE:
		m_pfnCircleDraw = Draw_G19;
		m_dHelicalStep = GetValFlags() & NCD_X ?
			(m_ptValE.x - m_ptValS.x) / ((m_eq-m_sq)/ARCSTEP) : 0.0;
		break;
	}

	if ( m_nG23 == 0 )
		m_dHelicalStep *= -1.0;		// �������]

	// �`��I�_���v�Z���ۑ�
	m_pt2D = m_ptValE.PointConvert();

#ifdef _DEBUG
	dbg.printf("gcode=%d", m_nG23);
	dbg.printf("sx=%.3f sy=%.3f sz=%.3f ex=%.3f ey=%.3f ez=%.3f r=%.3f",
		m_ptValS.x, m_ptValS.y, m_ptValS.z,
		m_ptValE.x, m_ptValE.y, m_ptValE.z, m_r);
	dbg.printf("px=%.3f py=%.3f pz=%.3f sq=%f eq=%f",
		m_ptOrg.x, m_ptOrg.y, m_ptOrg.z, m_sq*DEG, m_eq*DEG);
#endif
	// �؍풷�v�Z
	CalcLength();
	// ��Ԑ�L��`
	SetMaxRect();

	if ( fError )
		SetNCFlags(NCF_ERROR);
#ifdef _DEBUG
	DbgDump();
	dbg.printf("Length = %f", m_nc.dLength);
	Dbg_sep();
#endif
}

BOOL CNCcircle::CalcCenter(const CPointD& pts, const CPointD& pte)
{
#ifdef _DEBUG
	CMagaDbg	dbg("Center()", DBG_RED);
#endif
	// R �w��Ŏn�_�I�_�������ꍇ�̓G���[
	if ( pts == pte )
		return TRUE;

	// �~�̕��������C(s.x, s.y)�C(e.x, e.y) �𒆐S�Ƃ���
	// �Q�̉~�̌�_�����߂�
	CPointD	pt1, pt2;
	int		nResult;
	if ( (nResult=::CalcIntersectionPoint(pts, pte, m_r, m_r, &pt1, &pt2)) <= 0 )
		return TRUE;

	// �ǂ���̉����̗p���邩
	AngleTuning(pts-pt1, pte-pt1);	// �܂�����̒��S���W����p�x�����߂�
	if ( nResult==1 ||
		(m_r>0.0 && fabs(m_sq - m_eq)<180.0*RAD) ||
		(m_r<0.0 && fabs(m_sq - m_eq)>180.0*RAD) ) {
		SetCenter(pt1);
	}
	else {
		// ������ϯ����Ȃ������̂ő����̉����̗p
		AngleTuning(pts-pt2, pte-pt2);
		SetCenter(pt2);
	}
	return FALSE;
}

void CNCcircle::SetCenter(const CPointD& pt)
{
	switch ( GetPlane() ) {
	case XY_PLANE:
		m_ptOrg.x = pt.x;
		m_ptOrg.y = pt.y;
		m_ptOrg.z = GetValue(NCA_Z);
		break;
	case XZ_PLANE:
		m_ptOrg.x = pt.x;
		m_ptOrg.y = GetValue(NCA_Y);
		m_ptOrg.z = pt.y;
		break;
	case YZ_PLANE:
		m_ptOrg.x = GetValue(NCA_X);
		m_ptOrg.y = pt.x;
		m_ptOrg.z = pt.y;
		break;
	}
}

void CNCcircle::AngleTuning(const CPointD& pts, const CPointD& pte)
{
	if ( (m_sq=atan2(pts.y, pts.x)) < 0.0 )
		m_sq += 360.0*RAD;
	if ( (m_eq=atan2(pte.y, pte.x)) < 0.0 )
		m_eq += 360.0*RAD;
	// ��� s<e (�����v���) �Ƃ��� -> �`��ɕ����͊֌W�Ȃ�
	if ( m_nG23 == 0 ) {	// G02 �Ȃ�J�n�p�x�ƏI���p�x�����ւ�
		double	dTmp = m_sq;
		m_sq = m_eq;
		m_eq = dTmp;
	}
	while ( m_sq >= m_eq )
		m_eq += 360.0*RAD;
}

void CNCcircle::DrawTuning(double f)
{
	m_dFactor = f;
}

void CNCcircle::DrawTuningXY(double f)
{
	m_dFactorXY = f;
}

void CNCcircle::DrawTuningXZ(double f)
{
	m_dFactorXZ = f;
}

void CNCcircle::DrawTuningYZ(double f)
{
	m_dFactorYZ = f;
}

void CNCcircle::Draw(CDC* pDC)
{
#ifdef _DEBUGDRAW_NCD
	CMagaDbg	dbg("CNCcircle::Draw()", DBG_RED);
	dbg.printf("Line=%d", GetStrLine());
#endif
	// ���ʂ��Ƃ̕`��֐�
	CPen* pOldPen = pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenNC(NCPEN_G1));
	(this->*m_pfnCircleDraw)(NCCIRCLEDRAW_XYZ, pDC);
	pDC->SelectObject(pOldPen);
	// ���S���W(���F)
	CViewOption*	pOpt = AfxGetNCVCApp()->GetViewOption();
	if ( pOpt->IsDrawCircleCenter() )
		pDC->SetPixelV(m_ptOrg.PointConvert()*m_dFactor,
			pOpt->GetNcDrawColor(NCCOL_CENTERCIRCLE));
}

void CNCcircle::DrawXY(CDC* pDC)
{
#ifdef _DEBUGDRAW_NCD
	CMagaDbg	dbg("CNCcircle::DrawXY()", DBG_RED);
	dbg.printf("Line=%d", GetStrLine());
#endif
	CPen* pOldPen = pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenNC(NCPEN_G1));
	(this->*m_pfnCircleDraw)(NCCIRCLEDRAW_XY, pDC);
	pDC->SelectObject(pOldPen);
	CViewOption*	pOpt = AfxGetNCVCApp()->GetViewOption();
	if ( pOpt->IsDrawCircleCenter() )
		pDC->SetPixelV(m_ptOrg.GetXY()*m_dFactorXY,
			pOpt->GetNcDrawColor(NCCOL_CENTERCIRCLE));
}

void CNCcircle::DrawXZ(CDC* pDC)
{
#ifdef _DEBUGDRAW_NCD
	CMagaDbg	dbg("CNCcircle::DrawXZ()", DBG_RED);
	dbg.printf("Line=%d", GetStrLine());
#endif
	CPen* pOldPen = pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenNC(NCPEN_G1));
	(this->*m_pfnCircleDraw)(NCCIRCLEDRAW_XZ, pDC);
	pDC->SelectObject(pOldPen);
	CViewOption*	pOpt = AfxGetNCVCApp()->GetViewOption();
	if ( pOpt->IsDrawCircleCenter() )
		pDC->SetPixelV(m_ptOrg.GetXZ()*m_dFactorXZ,
			pOpt->GetNcDrawColor(NCCOL_CENTERCIRCLE));
}

void CNCcircle::DrawYZ(CDC* pDC)
{
#ifdef _DEBUGDRAW_NCD
	CMagaDbg	dbg("CNCcircle::DrawYZ()", DBG_RED);
	dbg.printf("Line=%d", GetStrLine());
#endif
	CPen* pOldPen = pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenNC(NCPEN_G1));
	(this->*m_pfnCircleDraw)(NCCIRCLEDRAW_YZ, pDC);
	pDC->SelectObject(pOldPen);
	CViewOption*	pOpt = AfxGetNCVCApp()->GetViewOption();
	if ( pOpt->IsDrawCircleCenter() )
		pDC->SetPixelV(m_ptOrg.GetYZ()*m_dFactorYZ,
			pOpt->GetNcDrawColor(NCCOL_CENTERCIRCLE));
}

// CDC::Arc() ���g���Ƃǂ����Ă��\�����Y����D
// ���ꕽ�ʂł����Ă����א����ɂ��ߎ����s��
// �Ⴄ���ʂ̂Ƃ��́C��Ԑ�L��` m_rcMax �𗘗p���Đ����o��
void CNCcircle::Draw_G17(EN_NCCIRCLEDRAW enType, CDC* pDC)	// XY_PLANE
{
	double		q = m_sq, r = fabs(m_r), dFinalVal = m_ptValE.z;
	CPoint3D	pt3D, ptDrawOrg(m_ptOrg);
	CPointD		ptDraw, ptDraw2;

	switch ( enType ) {
	case NCCIRCLEDRAW_XYZ:
		r *= m_dFactor;
		ptDrawOrg *= m_dFactor;
		dFinalVal *= m_dFactor;
		pt3D.x = r * cos(q) + ptDrawOrg.x;
		pt3D.y = r * sin(q) + ptDrawOrg.y;
		pt3D.z = m_nG23==0 ? dFinalVal : ptDrawOrg.z;	// �ضيJ�n���W
		ptDraw = pt3D.PointConvert();
		pDC->MoveTo(ptDraw);	// �J�n�_�ֈړ�
		// ARCSTEP �Â��א����ŕ`��
		for ( q+=ARCSTEP; q<m_eq; q+=ARCSTEP ) {
			pt3D.x = r * cos(q) + ptDrawOrg.x;
			pt3D.y = r * sin(q) + ptDrawOrg.y;
			pt3D.z += m_dHelicalStep;
			ptDraw = pt3D.PointConvert();
			pDC->LineTo(ptDraw);
		}
		// �[�����`��
		pt3D.x = r * cos(m_eq) + ptDrawOrg.x;
		pt3D.y = r * sin(m_eq) + ptDrawOrg.y;
		pt3D.z = m_nG23==0 ? ptDrawOrg.z : dFinalVal;	// �ضُI�����W
		ptDraw = pt3D.PointConvert();
		pDC->LineTo(ptDraw);
		break;

	case NCCIRCLEDRAW_XY:	// Don't ARC()
		r *= m_dFactorXY;
		ptDrawOrg *= m_dFactorXY;
		ptDraw.x = r * cos(q) + ptDrawOrg.x;
		ptDraw.y = r * sin(q) + ptDrawOrg.y;
		pDC->MoveTo(ptDraw);
		for ( q+=ARCSTEP; q<m_eq; q+=ARCSTEP ) {
			ptDraw.x = r * cos(q) + ptDrawOrg.x;
			ptDraw.y = r * sin(q) + ptDrawOrg.y;
			pDC->LineTo(ptDraw);
		}
		ptDraw.x = r * cos(m_eq) + ptDrawOrg.x;
		ptDraw.y = r * sin(m_eq) + ptDrawOrg.y;
		pDC->LineTo(ptDraw);
		break;

	case NCCIRCLEDRAW_XZ:
		ptDraw.x  = m_rcMax.left;	ptDraw.y  = m_rcMax.low;
		ptDraw2.x = m_rcMax.right;	ptDraw2.y = m_rcMax.high;
		ptDraw *= m_dFactorXZ;		ptDraw2 *= m_dFactorXZ;
		pDC->MoveTo(ptDraw);
		pDC->LineTo(ptDraw2);
		break;

	case NCCIRCLEDRAW_YZ:
		ptDraw.x  = m_rcMax.top;	ptDraw.y  = m_rcMax.low;
		ptDraw2.x = m_rcMax.bottom;	ptDraw2.y = m_rcMax.high;
		ptDraw *= m_dFactorYZ;		ptDraw2 *= m_dFactorYZ;
		pDC->MoveTo(ptDraw);
		pDC->LineTo(ptDraw2);
		break;
	}
}

void CNCcircle::Draw_G18(EN_NCCIRCLEDRAW enType, CDC* pDC)	// XZ_PLANE
{
	double		q = m_sq, r = fabs(m_r), dFinalVal = m_ptValE.y;
	CPoint3D	pt3D, ptDrawOrg(m_ptOrg);
	CPointD		ptDraw, ptDraw2;

	switch ( enType ) {
	case NCCIRCLEDRAW_XYZ:
		r *= m_dFactor;
		ptDrawOrg *= m_dFactor;
		dFinalVal *= m_dFactor;
		pt3D.x = r * cos(q) + ptDrawOrg.x;
		pt3D.y = m_nG23==0 ? dFinalVal : ptDrawOrg.y;
		pt3D.z = r * sin(q) + ptDrawOrg.z;
		ptDraw = pt3D.PointConvert();
		pDC->MoveTo(ptDraw);
		for ( q+=ARCSTEP; q<m_eq; q+=ARCSTEP ) {
			pt3D.x = r * cos(q) + ptDrawOrg.x;
			pt3D.y += m_dHelicalStep;
			pt3D.z = r * sin(q) + ptDrawOrg.z;
			ptDraw = pt3D.PointConvert();
			pDC->LineTo(ptDraw);
		}
		pt3D.x = r * cos(m_eq) + ptDrawOrg.x;
		pt3D.y = m_nG23==0 ? ptDrawOrg.y : dFinalVal;
		pt3D.z = r * sin(m_eq) + ptDrawOrg.z;
		ptDraw = pt3D.PointConvert();
		pDC->LineTo(ptDraw);
		break;

	case NCCIRCLEDRAW_XY:
		ptDraw  = m_rcMax.TopLeft() * m_dFactorXY;
		ptDraw2 = m_rcMax.BottomRight() * m_dFactorXY;
		pDC->MoveTo(ptDraw);
		pDC->LineTo(ptDraw2);
		break;

	case NCCIRCLEDRAW_XZ:	// Don't ARC()
		r *= m_dFactorXZ;
		ptDrawOrg *= m_dFactorXZ;
		ptDraw.x = r * cos(q) + ptDrawOrg.x;
		ptDraw.y = r * sin(q) + ptDrawOrg.z;
		pDC->MoveTo(ptDraw);
		for ( q+=ARCSTEP; q<m_eq; q+=ARCSTEP ) {
			ptDraw.x = r * cos(q) + ptDrawOrg.x;
			ptDraw.y = r * sin(q) + ptDrawOrg.z;
			pDC->LineTo(ptDraw);
		}
		ptDraw.x = r * cos(m_eq) + ptDrawOrg.x;
		ptDraw.y = r * sin(m_eq) + ptDrawOrg.z;
		pDC->LineTo(ptDraw);
		break;

	case NCCIRCLEDRAW_YZ:
		ptDraw.x  = m_rcMax.top;	ptDraw.y  = m_rcMax.low;
		ptDraw2.x = m_rcMax.bottom;	ptDraw2.y = m_rcMax.high;
		ptDraw *= m_dFactorYZ;		ptDraw2 *= m_dFactorYZ;
		pDC->MoveTo(ptDraw);
		pDC->LineTo(ptDraw2);
		break;
	}
}

void CNCcircle::Draw_G19(EN_NCCIRCLEDRAW enType, CDC* pDC)	// YZ_PLANE
{
	double		q = m_sq, r = fabs(m_r), dFinalVal = m_ptValE.x;
	CPoint3D	pt3D, ptDrawOrg(m_ptOrg);
	CPointD		ptDraw, ptDraw2;

	switch ( enType ) {
	case NCCIRCLEDRAW_XYZ:
		r *= m_dFactor;
		ptDrawOrg *= m_dFactor;
		dFinalVal *= m_dFactor;
		pt3D.x = m_nG23==0 ? dFinalVal : ptDrawOrg.x;
		pt3D.y = r * cos(q) + ptDrawOrg.y;
		pt3D.z = r * sin(q) + ptDrawOrg.z;
		ptDraw = pt3D.PointConvert();
		pDC->MoveTo(ptDraw);
		for ( q+=ARCSTEP; q<m_eq; q+=ARCSTEP ) {
			pt3D.x += m_dHelicalStep;
			pt3D.y = r * cos(q) + ptDrawOrg.y;
			pt3D.z = r * sin(q) + ptDrawOrg.z;
			ptDraw = pt3D.PointConvert();
			pDC->LineTo(ptDraw);
		}
		pt3D.x = m_nG23==0 ? ptDrawOrg.x : dFinalVal;
		pt3D.y = r * cos(m_eq) + ptDrawOrg.y;
		pt3D.z = r * sin(m_eq) + ptDrawOrg.z;
		ptDraw = pt3D.PointConvert();
		pDC->LineTo(ptDraw);
		break;

	case NCCIRCLEDRAW_XY:
		ptDraw  = m_rcMax.TopLeft() * m_dFactorXY;
		ptDraw2 = m_rcMax.BottomRight() * m_dFactorXY;
		pDC->MoveTo(ptDraw);
		pDC->LineTo(ptDraw2);
		break;

	case NCCIRCLEDRAW_XZ:
		ptDraw.x  = m_rcMax.left;	ptDraw.y  = m_rcMax.low;
		ptDraw2.x = m_rcMax.right;	ptDraw2.y = m_rcMax.high;
		ptDraw *= m_dFactorXZ;		ptDraw2 *= m_dFactorXZ;
		pDC->MoveTo(ptDraw);
		pDC->LineTo(ptDraw2);
		break;

	case NCCIRCLEDRAW_YZ:	// Don't ARC()
		r *= m_dFactorYZ;
		ptDrawOrg *= m_dFactorYZ;
		ptDraw.x = r * cos(q) + ptDrawOrg.y;
		ptDraw.y = r * sin(q) + ptDrawOrg.z;
		pDC->MoveTo(ptDraw);
		for ( q+=ARCSTEP; q<m_eq; q+=ARCSTEP ) {
			ptDraw.x = r * cos(q) + ptDrawOrg.y;
			ptDraw.y = r * sin(q) + ptDrawOrg.z;
			pDC->LineTo(ptDraw);
		}
		ptDraw.x = r * cos(m_eq) + ptDrawOrg.y;
		ptDraw.y = r * sin(m_eq) + ptDrawOrg.z;
		pDC->LineTo(ptDraw);
		break;
	}
}

void CNCcircle::SetMaxRect(void)
{
#ifdef _DEBUG
	CMagaDbg	dbg("SetMaxRect()", DBG_RED);
#endif
	// �O�ڂ���l�p�`(SetMaxRect)
	// �ٺ�ؽ�т� CDXFarc::Constracter() �Ɠ���
	double	r = fabs(m_r), sq = m_sq, eq = m_eq;
	CRectD	rcMax, rcInit(r, r, -r, -r);

	// �n�_�E�I�_�̊J�n�ʒu
	// m_ptValS, m_ptValE ���g���ƁC���ʂ��Ƃ̏������K�v�Ȃ̂�
	// m_ptOrg �����_(0,0) �Ƃ����n�_�I�_���v�Z
	CPointD	pts(r*cos(sq), r*sin(sq));
	CPointD	pte(r*cos(eq), r*sin(eq));
	// �Q�_�̋�`�͕K���ʂ�̂ŁC
	// �����l�Ƃ��čő�l�E�ŏ��l����
	rcMax.left		= max(pts.x, pte.x);
	rcMax.top		= max(pts.y, pte.y);
	rcMax.right		= min(pts.x, pte.x);
	rcMax.bottom	= min(pts.y, pte.y);

	// �p�x�̒����ƊJ�n�I���ی�(i,j)�̐ݒ�
	int	i = 0, j = 0;
	while ( sq >= 90.0*RAD ) {
		sq -= 90.0*RAD;
		i++;
	}
	while ( eq >= 90.0*RAD ) {
		eq -= 90.0*RAD;
		j++;
	}
	// i ���猩�� j �����ی���ɂ��邩
	int	nCnt = ( j - i ) % 4;
	if ( nCnt==0 && sq>=eq )
		nCnt = 4;

	// �ی��ʉ߂��ƂɎ��ő�l(r)����
	int	a;
	for  ( j=1; j<=nCnt; j++ ) {
		a = ( i + j ) % 4;
		rcMax[a] = rcInit[a];
	}
	rcMax.NormalizeRect();

	// ��Ԑ�L��`���W�ݒ�
	switch ( GetPlane() ) {
	case XY_PLANE:
		rcMax.OffsetRect(m_ptOrg.GetXY());
		m_rcMax.left	= rcMax.left;
		m_rcMax.top		= rcMax.top;
		m_rcMax.right	= rcMax.right;
		m_rcMax.bottom	= rcMax.bottom;
		m_rcMax.low		= GetValue(NCA_Z);
		m_rcMax.high	= m_ptOrg.z;
		break;
	case XZ_PLANE:
		rcMax.OffsetRect(m_ptOrg.GetXZ());
		m_rcMax.left	= rcMax.left;
		m_rcMax.top		= GetValue(NCA_Y);
		m_rcMax.right	= rcMax.right;
		m_rcMax.bottom	= m_ptOrg.y;
		m_rcMax.low		= rcMax.top;
		m_rcMax.high	= rcMax.bottom;
		break;
	case YZ_PLANE:
		rcMax.OffsetRect(m_ptOrg.GetYZ());
		m_rcMax.left	= GetValue(NCA_X);
		m_rcMax.top		= rcMax.left;
		m_rcMax.right	= m_ptOrg.x;
		m_rcMax.bottom	= rcMax.right;
		m_rcMax.low		= rcMax.top;
		m_rcMax.high	= rcMax.bottom;
		break;
	default:
		m_rcMax.SetRectEmpty();
		break;
	}
	m_rcMax.NormalizeRect();
#ifdef _DEBUG
	dbg.printf("m_rcMax(left, top   )=(%f, %f)", m_rcMax.left, m_rcMax.top);
	dbg.printf("m_rcMax(right,bottom)=(%f, %f)", m_rcMax.right, m_rcMax.bottom);
	dbg.printf("m_rcMax(high, low   )=(%f, %f)", m_rcMax.high, m_rcMax.low);
#endif
}

BOOL CNCcircle::CalcRoundPoint
	(const CNCdata* pNext, double r, CPointD& pt, double& rr1, double& rr2)
{
	// �Q���̌�_(���g�̏I�_)�����_�ɂȂ�悤�ɕ␳
	CPointD	pts( GetPlaneValueOrg(m_ptValE, m_ptOrg) ), pte;
	double	xa, ya, r0 = fabs(m_r);

	if ( pNext->GetGcode() > 1 ) {
		CNCcircle*	pCircle = (CNCcircle *)pNext;
		int	nG23next = 1 - pCircle->GetG23();	// ��_�ւ̐i���͔��Ή�]
		double	r1, r2, rn = fabs(pCircle->GetR());
		pte = GetPlaneValueOrg(m_ptValE, pCircle->GetOrg());
		CPointD	p1, p2, pts_abs(fabs(pts.x), fabs(pts.y)), pte_abs(fabs(pte.x), fabs(pte.y));
		// ���g�̉~�Ƒ����̐ڐ��Ŏ��g�́}r�������v�Z
		if ( (r1=CalcRoundPoint_CircleInOut(pts, pte, m_nG23, nG23next, r)) == HUGE_VAL )
			return FALSE;
		// �����̉~�Ǝ��g�̐ڐ��ő����́}r�������v�Z
		if ( (r2=CalcRoundPoint_CircleInOut(pte, pts, nG23next, m_nG23, r)) == HUGE_VAL )
			return FALSE;
		// �Q�̉~�̌�_�����߂� -> �����Q�Ȃ��Ɩʎ��o���Ȃ��Ɣ��f����
		rr1 = r0 + r1;		rr2 = rn + r2;
		if ( ::CalcIntersectionPoint(pts, pte, rr1, rr2, &p1, &p2) != 2 )
			return FALSE;
		// ���̑I��
		if ( (pts_abs.x<EPS && pte_abs.x<EPS) || (pts_abs.y<EPS && pte_abs.y<EPS) ||
				(pts_abs.x>EPS && pte_abs.x>EPS && fabs(pts.y/pts.x - pte.y/pte.x)<EPS) ) {
			// ���S���������ɂ���Ƃ��C�ڐ��ƕ�������������I��
			if ( pts_abs.y < EPS )
				pt = (m_nG23==0 ? -pts.x : pts.x) * p1.y > 0 ? p1 : p2;
			else
				pt = (m_nG23==0 ? pts.y : -pts.y) * p1.x > 0 ? p1 : p2;
		}
		else
			pt = p1.x*p1.x+p1.y*p1.y < p2.x*p2.x+p2.y*p2.y ? p1 : p2;
		// �ʎ��ɑ�������C�l�̌v�Z
		if ( r1 > 0 ) {
			xa = (pt.x*r0+pts.x*r1) / rr1;
			ya = (pt.y*r0+pts.y*r1) / rr1;
		}
		else {
			r1 = fabs(r1);
			xa = (pt.x*r0-pts.x*r1) / rr1;
			ya = (pt.y*r0-pts.y*r1) / rr1;
		}
		rr1 = sqrt(xa*xa + ya*ya);
		if ( r2 > 0 ) {
			xa = (pt.x*rn+pte.x*r2) / rr2;
			ya = (pt.y*rn+pte.y*r2) / rr2;
		}
		else {
			r2 = fabs(r2);
			xa = (pt.x*rn-pte.x*r2) / rr2;
			ya = (pt.y*rn-pte.y*r2) / rr2;
		}
		rr2 = sqrt(xa*xa + ya*ya);
	}
	else {
		pte = GetPlaneValueOrg(m_ptValE, pNext->GetEndPoint());
		// �̾�ĕ����s�ړ���������_�����߂�
		pt = ::CalcOffsetIntersectionPoint(pte, pts,	// ��������̱��۰���
					r0, r, m_nG23==0 ? -1 : 1, 0, 0);	// ��]�����𔽓]
		if ( pt == HUGE_VAL )
			return FALSE;
		// �ʎ��ɑ�������C�l�̌v�Z
		rr1 = r0 + r;
		if ( r > 0 ) {
			// �����_
			xa = (pt.x*r0+pts.x*r) / rr1;
			ya = (pt.y*r0+pts.y*r) / rr1;
		}
		else {
			// �O���_
			r = fabs(r);
			xa = (pt.x*r0-pts.x*r) / rr1;
			ya = (pt.y*r0-pts.y*r) / rr1;
		}
		rr1 = sqrt(xa*xa + ya*ya);
		rr2 = sqrt(pt.x*pt.x + pt.y*pt.y - r*r);
	}

	// ���_�␳
	pt += GetPlaneValue(m_ptValE);

	return TRUE;
}

BOOL CNCcircle::SetChamferingPoint(BOOL bStart, double c, CPointD& pt)
{
	CPoint3D	ptOrg3D( bStart ? m_ptValS : m_ptValE );
	CPointD		ptOrg1, ptOrg2, pt1, pt2;
	double		pa, pb, ps;

	switch ( GetPlane() ) {
	case XY_PLANE:
		ptOrg1 = m_ptOrg.GetXY();
		ptOrg2 = ptOrg3D.GetXY();
		pt1 = m_ptValS.GetXY();
		pt2 = m_ptValE.GetXY();
		break;
	case XZ_PLANE:
		ptOrg1 = m_ptOrg.GetXZ();
		ptOrg2 = ptOrg3D.GetXZ();
		pt1 = m_ptValS.GetXZ();
		pt2 = m_ptValE.GetXZ();
		break;
	case YZ_PLANE:
		ptOrg1 = m_ptOrg.GetYZ();
		ptOrg2 = ptOrg3D.GetYZ();
		pt1 = m_ptValS.GetYZ();
		pt2 = m_ptValE.GetYZ();
		break;
	default:
		return FALSE;
	}

	// ����������Ȃ��Ƃ��ʹװ
	if ( m_eq - m_sq > 180.0*RAD ) {
		// 180���𒴂���Ƃ��͒��a�Ɣ�r
		if ( c >= fabs(m_r)*2 )
			return FALSE;
	}
	else {
		// 180�������̏ꍇ�͌��̒����Ɣ�r
		if ( c >= hypot(pt1.x-pt2.x, pt1.y-pt2.y) )
			return FALSE;
	}

	// �Q�̉~�̌�_�����߂� -> �����Q�Ȃ��Ɩʎ��o���Ȃ��Ɣ��f����
	if ( ::CalcIntersectionPoint(ptOrg1, ptOrg2, fabs(m_r), c, &pt1, &pt2) != 2 )
		return FALSE;

	// ���v���̏ꍇ�C�n�p�ƏI�p������ւ���Ă���̂ňꎞ�I�Ɍ��ɖ߂�
	if ( m_nG23 == 0 ) {
		ps = m_sq;
		m_sq = m_eq;
		m_eq = ps;
	}

	// ���̑I��
	ps = bStart ? m_sq : m_eq;
	if ( (pa=atan2(pt1.y-ptOrg1.y, pt1.x-ptOrg1.x)) < 0.0 )
		pa += 360.0*RAD;
	if ( (pb=atan2(pt2.y-ptOrg1.y, pt2.x-ptOrg1.x)) < 0.0 )
		pb += 360.0*RAD;
	// 180�x�ȏ�̍��͕␳
	if ( fabs(ps-pa) > 180.0*RAD ) {
		if ( ps > pa )
			ps -= 360.0*RAD;
		else
			pa -= 360.0*RAD;
	}

	// �n�p�E�I�p�ɋ߂����̑I���Ǝ������g�̓_���X�V
	if ( bStart ) {
		if ( m_nG23 == 0 ) {	// ���v���̎��́C���p��������������
			if ( ps > pa )
				pt = pt1;
			else {
				pt = pt2;
				pa = pb;
			}
		}
		else {					// �����v���̎��́C�傫������I��
			if ( ps < pa )
				pt = pt1;
			else {
				pt = pt2;
				pa = pb;
			}
		}
		switch ( GetPlane() ) {
		case XY_PLANE:
			m_ptValS.x = pt.x;
			m_ptValS.y = pt.y;
			break;
		case XZ_PLANE:
			m_ptValS.x = pt.x;
			m_ptValS.z = pt.y;
			break;
		case YZ_PLANE:
			m_ptValS.y = pt.x;
			m_ptValS.z = pt.y;
			break;
		}
		m_sq = pa;
	}
	else {
		if ( m_nG23 == 0 ) {
			if ( ps < pa )
				pt = pt1;
			else {
				pt = pt2;
				pa = pb;
			}
		}
		else {
			if ( ps > pa )
				pt = pt1;
			else {
				pt = pt2;
				pa = pb;
			}
		}
		switch ( GetPlane() ) {
		case XY_PLANE:
			m_ptValE.x = pt.x;
			m_ptValE.y = pt.y;
			break;
		case XZ_PLANE:
			m_ptValE.x = pt.x;
			m_ptValE.z = pt.y;
			break;
		case YZ_PLANE:
			m_ptValE.y = pt.x;
			m_ptValE.z = pt.y;
			break;
		}
		m_eq = pa;
		m_pt2D = m_ptValE.PointConvert();
	}

	// �p�x�␳
	if ( m_nG23 == 0 ) {
		ps = m_sq;
		m_sq = m_eq;
		m_eq = ps;
	}
	while ( m_sq >= m_eq )
		m_eq += 360.0*RAD;

	// �؍풷�v�Z
	CalcLength();

	return TRUE;
}

double CNCcircle::CalcBetweenAngle(const CPoint3D&, const CNCdata* pNext)
{
	// �Q���̌�_(���g�̏I�_)�����_�ɂȂ�悤�ɕ␳
	CPoint3D	pto(GetValue(NCA_X), GetValue(NCA_Y), GetValue(NCA_Z));
	CPointD		pt( GetPlaneValueOrg(pto, m_ptOrg) ), pt1, pt2;

	// �ڐ��v�Z
	int k = m_nG23==0 ? -1 : 1;	// �I�_�ł͉�]�����𔽓]
	pt1.x = -pt.y*k;	// G02:-90��
	pt1.y =  pt.x*k;	// G03:+90��

	if ( pNext->GetGcode() > 1 ) {
		CNCcircle*	pCircle = (CNCcircle *)pNext;
		// ���̵�޼ު�Ă��~�ʂȂ璆�S��Ă�
		pt = GetPlaneValueOrg(pto, pCircle->GetOrg());
		// �ڐ��v�Z
		k = pCircle->GetG23()==0 ? 1 : -1;
		pt2.x = -pt.y*k;
		pt2.y =  pt.x*k;
	}
	else {
		CPoint3D	pte(pNext->GetValue(NCA_X), pNext->GetValue(NCA_Y), pNext->GetValue(NCA_Z));
		pt2 = GetPlaneValueOrg(pto, pte);
	}

	// �Q��(�܂��͉~�ʂ̐ڐ�)���Ȃ��p�x�����߂�
	return ::CalcBetweenAngleTwoLines(pt1, pt2);
}

int CNCcircle::CalcOffsetSign(const CPoint3D&)
{
	// ��]��������G41�̕␳���������߂�
	return m_nG23==0 ? 1 : -1;
}

CPointD CNCcircle::CalcPerpendicularPoint
	(const CPoint3D& pts, const CPoint3D&, double r, int)
{
	// �n�_�I�_�֌W�Ȃ�CalcOffsetSign()����
	// pts�ƒ��S�̌X�����v�Z���Ĕ��a�}r
	CPointD	pt( GetPlaneValueOrg(m_ptOrg, pts) );
	double	q = atan2(pt.y, pt.x), rr = fabs(m_r) + r * CalcOffsetSign(CPoint3D());
	CPointD	pt1(rr*cos(q), rr*sin(q));
	pt1 += GetPlaneValue(m_ptOrg);

	return pt1;
}

CPointD CNCcircle::CalcOffsetIntersectionPoint
	(const CPoint3D& pts, const CNCdata* pNext, double r, int k1, int k2)
{
	// �Q���̌�_(���g�̏I�_)�����_�ɂȂ�悤�ɕ␳
	CPoint3D	pto(GetValue(NCA_X), GetValue(NCA_Y), GetValue(NCA_Z));
	CPointD		pt;

	if ( pNext->GetGcode() > 1 ) {
		CNCcircle*	pCircle = (CNCcircle *)pNext;
		CPointD	pto1( GetPlaneValueOrg(pto, m_ptOrg) ),
				pto2( GetPlaneValueOrg(pto, pCircle->GetOrg()) );
		// �̾�ĕ����a�𒲐߂��ĂQ�̉~�̌�_�����߂�
		CPointD	p1, p2;
		int	n = ::CalcIntersectionPoint(pto1, pto2,
					fabs(m_r)+r*k1, fabs(pCircle->GetR())+r*k2, &p1, &p2);
		// ���̑I��
		pt = p1.x*p1.x+p1.y*p1.y < p2.x*p2.x+p2.y*p2.y ? p1 : p2;
	}
	else {
		CPoint3D	pte(pNext->GetValue(NCA_X), pNext->GetValue(NCA_Y), pNext->GetValue(NCA_Z));
		// �̾�ĕ����s�ړ���������_�����߂�
		CPointD	pt1( GetPlaneValueOrg(pto, pte) ),
				pt2( GetPlaneValueOrg(pto, m_ptOrg) );
		pt = ::CalcOffsetIntersectionPoint(pt1, pt2,			// ������̱��۰���
					fabs(m_r), r, m_nG23==0 ? -1 : 1, k2, k1);	// ��]�����𔽓]
	}

	// ���_�␳
	if ( pt != HUGE_VAL )
		pt += pto;

	return pt;
}

CPointD CNCcircle::CalcOffsetIntersectionPoint2
	(const CPoint3D& pts, const CNCdata* pNext, double r, int k1, int k2)
{
	int		k;
	double	dAngle;
	// �Q���̌�_(���g�̏I�_)�����_�ɂȂ�悤�ɕ␳
	CPoint3D	pto(GetValue(NCA_X), GetValue(NCA_Y), GetValue(NCA_Z));
	CPointD		pt, pt1, pt2;

	// ���g�̐ڐ����W
	pt = GetPlaneValueOrg(pto, m_ptOrg);
	k = m_nG23==0 ? -1 : 1;
	pt1.x = -pt.y * k;
	pt1.y =  pt.x * k;
	// �ڐ��p�ɕ������Čv�Z
	if ( (dAngle=atan2(pt1.y, pt1.x)) < 0.0 )
		dAngle += 360.0*RAD;
	k1 = 90.0*RAD<=dAngle && dAngle<270.0*RAD ? 1 : -1;		// �������]

	// �����̍��W�v�Z
	if ( pNext->GetGcode() > 1 ) {
		CNCcircle*	pCircle = (CNCcircle *)pNext;
		pt = GetPlaneValueOrg(pto, pCircle->GetOrg());
		int k = pCircle->GetG23()==0 ? 1 : -1;
		pt2.x = -pt.y * k;
		pt2.y =  pt.x * k;
		// �ڐ��p�ɕ������Čv�Z
		if ( (dAngle=atan2(pt2.y, pt2.x)) < 0.0 )
			dAngle += 360.0*RAD;
		k2 = 90.0*RAD<=dAngle && dAngle<270.0*RAD ? -1 : 1;
	}
	else {
		CPoint3D	pte(pNext->GetValue(NCA_X), pNext->GetValue(NCA_Y), pNext->GetValue(NCA_Z));
		pt2 = GetPlaneValueOrg(pto, pte);
	}

	// �������m�̵̾�Č�_�v�Z
	pt = ::CalcOffsetIntersectionPoint(pt1, pt2, k1, k2, r);
	// ���_�␳
	if ( pt != HUGE_VAL )
		pt += pto;

	return pt;
}

BOOL CNCcircle::SetCorrectPoint(BOOL bStart, const CPointD& ptSrc, double rr)
{
	CPoint3D&	ptVal = bStart ? m_ptValS : m_ptValE;
	CPointD		pt, ptOrg;
	switch ( GetPlane() ) {
	case XY_PLANE:
		ptVal.x = ptSrc.x;
		ptVal.y = ptSrc.y;
		pt = ptVal.GetXY();
		ptOrg = m_ptOrg.GetXY();
		break;
	case XZ_PLANE:
		ptVal.x = ptSrc.x;
		ptVal.z = ptSrc.y;
		pt = ptVal.GetXZ();
		ptOrg = m_ptOrg.GetXZ();
		break;
	case YZ_PLANE:
		ptVal.y = ptSrc.x;
		ptVal.z = ptSrc.y;
		pt = ptVal.GetYZ();
		ptOrg = m_ptOrg.GetYZ();
		break;
	default:
		return FALSE;
	}

	if ( bStart ) {
		double&	q = m_nG23==0 ? m_eq : m_sq;
		if ( (q=atan2(pt.y-ptOrg.y, pt.x-ptOrg.x)) < 0.0 )
			q += 360.0*RAD;
	}
	else {
		m_r += rr;	// �I�_�̎��������a�␳
		double&	q = m_nG23==0 ? m_sq : m_eq;
		if ( (q=atan2(pt.y-ptOrg.y, pt.x-ptOrg.x)) < 0.0 )
			q += 360.0*RAD;
		m_pt2D = m_ptValE.PointConvert();
	}
	while ( m_sq >= m_eq )
		m_eq += 360.0*RAD;
	CalcLength();

	return TRUE;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

// �~���m�̓��O���a�v�Z
double CalcRoundPoint_CircleInOut
	(const CPointD& pts, const CPointD& pte, int nG23, int nG23next, double r)
{
	CPointD	pto, pts_abs(fabs(pts.x), fabs(pts.y)), pte_abs(fabs(pte.x), fabs(pte.y));
	double	rr;
	int		k1 = nG23==0 ? -1 : 1, k2 = nG23next==0 ? -1 : 1;

	// ������̔��f
	if ( (pts_abs.x<EPS && pte_abs.x<EPS && pts.y*pte.y>0) ||
			(pts_abs.y<EPS && pte_abs.y<EPS && pts.x*pte.x>0) ||
			(pts_abs.x>EPS && pte_abs.x>EPS && fabs(pts.y/pts.x - pte.y/pte.x)<EPS && pts.x*pte.x>0) ) {
		// ���S���������ɂ���C���Cx,y �̕����������Ƃ�
		double	l1 = pts.x*pts.x + pts.y*pts.y;
		double	l2 = pte.x*pte.x + pte.y*pte.y;
		if ( fabs(l1 - l2) < EPS )	// ������������==���O�Չ~
			return HUGE_VAL;
		else if ( l1 > l2 )
			return -r;
		else
			return r;
	}
	// ���g�̉~�Ƒ����̐ڐ��Ŏ��g�́}r�������v�Z
	pto.x = -pte.y*k2;	// G02:-90��
	pto.y =  pte.x*k2;	// G03:+90��

	// ���Ɖ~�ʂ̏ꍇ�ƍl����(�������@)�͓���
	if ( fabs(pto.x) < EPS && pts_abs.y < EPS )
		rr = _copysign(r, pto.y*pts.x*k1);
	else if ( fabs(pto.y) < EPS && pts_abs.x < EPS )
		rr = _copysign(r, -pto.x*pts.y*k1);
	else
		rr = _copysign(r, -(pto.x*pts.x + pto.y*pts.y));

	return rr;
}
