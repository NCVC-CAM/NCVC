// DXFdata.cpp: CDXFdata �N���X�̃C���v�������e�[�V����
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "DXFdata.h"
#include "DXFDoc.h"
#include "Layer.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
//#define	_DEBUGDRAW_DXF			// �`����W���
//#define	_DEBUGDRAW_DXF_EDGE_	// �[�_�`��
#endif

IMPLEMENT_SERIAL(CCAMHead, CObject, 1)
IMPLEMENT_DYNAMIC(CDXFdata, CObject)
IMPLEMENT_SERIAL(CDXFpoint, CDXFdata, NCVCSERIALVERSION|VERSIONABLE_SCHEMA)
IMPLEMENT_SERIAL(CDXFline, CDXFpoint, NCVCSERIALVERSION|VERSIONABLE_SCHEMA)
IMPLEMENT_SERIAL(CDXFcircle, CDXFline, NCVCSERIALVERSION|VERSIONABLE_SCHEMA)
IMPLEMENT_SERIAL(CDXFcircleEx, CDXFcircle, NCVCSERIALVERSION|VERSIONABLE_SCHEMA)
IMPLEMENT_SERIAL(CDXFarc, CDXFcircle, NCVCSERIALVERSION|VERSIONABLE_SCHEMA)
IMPLEMENT_SERIAL(CDXFellipse, CDXFarc, NCVCSERIALVERSION|VERSIONABLE_SCHEMA)
IMPLEMENT_SERIAL(CDXFpolyline, CDXFline, NCVCSERIALVERSION|VERSIONABLE_SCHEMA)
IMPLEMENT_SERIAL(CDXFtext, CDXFpoint, NCVCSERIALVERSION|VERSIONABLE_SCHEMA)

using namespace boost;

/////////////////////////////////////////////////////////////////////////////
// �ÓI�ϐ��̏�����
CPointD		CDXFdata::ms_ptOrg(HUGE_VAL, HUGE_VAL);
BOOL		CDXFdata::ms_fXRev = FALSE;
BOOL		CDXFdata::ms_fYRev = FALSE;
CDXFdata*	CDXFdata::ms_pData = NULL;
DWORD		CDXFdata::ms_nSerialSeq = 0;

PFNORGDRILLTUNING	CDXFpoint::ms_pfnOrgDrillTuning = &CDXFpoint::OrgTuning_Seq;

//////////////////////////////////////////////////////////////////////
// �b�`�l�f�[�^�̃w�b�_�[�N���X
//////////////////////////////////////////////////////////////////////
void CCAMHead::Serialize(CArchive& ar)
{
	extern	DWORD	g_dwCamVer;		// NCVC.cpp
	static	TCHAR	ss_szID[] = "NCVC_CAM_DATA...";
	char	szID[sizeof(ss_szID)-1];	// ���ʗp(�Ō�� \0 ������)
	CString	strComment;					// ���ĕ�����

	if ( ar.IsStoring() ) {
		// ���ʎq
		ar.Write(ss_szID, sizeof(ss_szID)-1);
		// �ް�ޮ�No.�C����
		ar << (DWORD)NCVCSERIALVERSION << strComment;
	}
	else {
		CString	strMsg;
		// ���ʎq
		ar.Read(szID, sizeof(szID));
		if ( memcmp(szID, ss_szID, sizeof(szID)) != 0 ) {
			strMsg.Format(IDS_ERR_CAMDATA, ar.GetFile()->GetFilePath());
			AfxMessageBox(strMsg, MB_OK|MB_ICONSTOP);
			AfxThrowUserException();
		}
		// �ް�ޮ�No.
		ar >> g_dwCamVer;
		if ( g_dwCamVer > NCVCSERIALVERSION ) {
			strMsg.Format(IDS_ERR_CAMVER, ar.GetFile()->GetFilePath());
			AfxMessageBox(strMsg, MB_OK|MB_ICONSTOP);
			AfxThrowUserException();
		}
		else if ( g_dwCamVer < NCVCSERIALVERSION_1503 ) {
			strMsg.Format(IDS_ERR_CAMOLD, ar.GetFile()->GetFilePath());
			AfxMessageBox(strMsg, MB_OK|MB_ICONSTOP);
			AfxThrowUserException();
		}
		// ����
		ar >> strComment;
	}
}

//////////////////////////////////////////////////////////////////////
// �c�w�e�f�[�^�̃x�[�X�N���X
//////////////////////////////////////////////////////////////////////
CDXFdata::CDXFdata(ENDXFTYPE enType, CLayerData* pLayer, int nPoint)
{
	m_enType	= m_enMakeType = enType;
	m_dwFlags	= 0;
	m_pParentLayer = pLayer;
	m_pParentMap   = NULL;
	m_dwSelect	= 0;
	m_nPoint	= nPoint;
	if ( nPoint > 0 ) {
		m_pt		= new CPointD[nPoint];
		m_ptTun		= new CPointD[nPoint];
		m_ptMake	= new CPointD[nPoint];
	}
	else {
		m_pt		= NULL;
		m_ptTun		= NULL;
		m_ptMake	= NULL;
	}
}

CDXFdata::~CDXFdata()
{
	if ( m_nPoint > 0 ) {
		delete[]	m_pt;
		delete[]	m_ptTun;
		delete[]	m_ptMake;
	}
}

void CDXFdata::Serialize(CArchive& ar)
{
	if ( ar.IsStoring() ) {
		ar << (m_dwSelect & ~DXFSEL_SELECT);	// �I����ԏ���
		m_nSerialSeq = ms_nSerialSeq++;
	}
	else {
		ar >> m_dwSelect;
		m_pParentLayer = reinterpret_cast<CLayerData *>(ar.m_pDocument);
	}
}

CPen* CDXFdata::GetDrawPen(void) const
{
	CPen*	pDrawPen[] = {
		AfxGetNCVCMainWnd()->GetPenDXF(DXFPEN_CUTTER),
		AfxGetNCVCMainWnd()->GetPenCom(COMPEN_SEL)
	};
	return pDrawPen[ m_dwSelect & DXFSEL_SELECT ? 1 : 0];
}

void CDXFdata::XRev(void)		// X���̕������]
{
	for ( int i=0; i<m_nPoint; i++ ) {
		m_ptTun[i].x  = -m_ptTun[i].x;
		m_ptMake[i].x = -m_ptMake[i].x;
	}
}

void CDXFdata::YRev(void)		// Y���̕������]
{
	for ( int i=0; i<m_nPoint; i++ ) {
		m_ptTun[i].y  = -m_ptTun[i].y;
		m_ptMake[i].y = -m_ptMake[i].y;
	}
}

void CDXFdata::OrgTuningBase(void)
{
	m_dwFlags = 0;			// �����C�����׸ނ̸ر
	for ( int i=0; i<m_nPoint; i++ )
		m_ptMake[i] = m_ptTun[i].RoundUp();
	if ( ms_fXRev ) XRev();		// �������]
	if ( ms_fYRev ) YRev();		// �������]
}

BOOL CDXFdata::GetDirectionArraw_Line(const CPointD pt[], CPointD ptResult[][3]) const
{
	double	lqs = atan2(pt[1].y - pt[0].y, pt[1].x - pt[0].x),
			lqe = lqs + 180.0*RAD;
	double	lq[][2] = { {lqs + ARRAWANGLE, lqs - ARRAWANGLE},
						{lqe + ARRAWANGLE, lqe - ARRAWANGLE} };
	for ( int i=0; i<2; i++ ) {
		ptResult[i][0].x = ARRAWLENGTH * cos(lq[i][0]);	// + pt[i] �͊g�嗦���f��
		ptResult[i][0].y = ARRAWLENGTH * sin(lq[i][0]);
		ptResult[i][1]   = pt[i];
		ptResult[i][2].x = ARRAWLENGTH * cos(lq[i][1]);
		ptResult[i][2].y = ARRAWLENGTH * sin(lq[i][1]);
	}
	return TRUE;
}

BOOL CDXFdata::GetDirectionArraw_Circle
	(BOOL bRound, const double q[], const CPointD pt[], CPointD ptResult[][3]) const
{
	double	lq[][2] = { {q[0] + ARRAWANGLE, q[0] - ARRAWANGLE},
						{q[1] + ARRAWANGLE, q[1] - ARRAWANGLE} };
	for ( int i=0; i<2; i++ ) {
		ptResult[i][0].x = ARRAWLENGTH * cos(lq[i][0]);
		ptResult[i][0].y = ARRAWLENGTH * sin(lq[i][0]);
		ptResult[i][1]   = pt[i];
		ptResult[i][2].x = ARRAWLENGTH * cos(lq[i][1]);
		ptResult[i][2].y = ARRAWLENGTH * sin(lq[i][1]);
	}

	return TRUE;
}

//////////////////////////////////////////////////////////////////////
// �c�w�e�f�[�^��Point�N���X
//////////////////////////////////////////////////////////////////////
CDXFpoint::CDXFpoint() : CDXFdata(DXFPOINTDATA, NULL, 1)
{
}

CDXFpoint::CDXFpoint(ENDXFTYPE enType, CLayerData* pLayer, int nPoint) :
	CDXFdata(enType, pLayer, nPoint)
{
}

CDXFpoint::CDXFpoint(LPDXFPARGV lpPoint) : CDXFdata(DXFPOINTDATA, lpPoint->pLayer, 1)
{
	m_pt[0] = lpPoint->c;
	SetMaxRect();
}

CDXFpoint::CDXFpoint(CLayerData* pLayer, const CDXFpoint* pData, LPDXFBLOCK lpBlock) :
	CDXFdata(DXFPOINTDATA, pLayer, 1)
{
	m_pt[0] = pData->GetNativePoint(0);
	if ( lpBlock->dwBlockFlg & DXFBLFLG_X )
		m_pt[0].x *= lpBlock->dMagni[NCA_X];
	if ( lpBlock->dwBlockFlg & DXFBLFLG_Y )
		m_pt[0].y *= lpBlock->dMagni[NCA_Y];
	if ( lpBlock->dwBlockFlg & DXFBLFLG_R )
		m_pt[0].RoundPoint(lpBlock->dRound*RAD);
	m_pt[0] += lpBlock->ptOrg;
	SetMaxRect();
}

void CDXFpoint::Serialize(CArchive& ar)
{
	CDXFdata::Serialize(ar);
	if ( ar.IsStoring() )
		ar << m_pt[0].x << m_pt[0].y;
	else {
		ar >> m_pt[0].x >> m_pt[0].y;
		SetMaxRect();
	}
}

void CDXFpoint::DrawTuning(double f)
{
	CPointD	pt( m_pt[0] * f);
	m_ptDraw = pt;
	// �ʒu��\���ۈ�͏��2.5�_������
	m_rcDraw.TopLeft()		= pt - LOMETRICFACTOR*2.5;
	m_rcDraw.BottomRight()	= pt + LOMETRICFACTOR*2.5;
}

void CDXFpoint::Draw(CDC* pDC) const
{
#ifdef _DEBUGDRAW_DXF
	CMagaDbg	dbg("CDXFpoint::Draw()", DBG_RED);
	dbg.printStruct((LPRECT)&m_rcDraw, "m_rcDraw");
#endif
	CPoint	pt(m_rcDraw.CenterPoint());
	pDC->MoveTo(m_rcDraw.right-1, pt.y);
	pDC->LineTo(m_rcDraw.left,  pt.y);
	pDC->LineTo(pt.x, m_rcDraw.top+1);	// top������
	pDC->LineTo(pt.x, m_rcDraw.bottom);
	pDC->LineTo(m_rcDraw.right-1, pt.y);
	pDC->Ellipse(&m_rcDraw);
}

double CDXFpoint::OrgTuning(BOOL bCalc/*=TRUE*/)
{
	ASSERT(ms_pData);
	m_ptTun[0] = m_pt[0] - ms_ptOrg;
	OrgTuningBase();
#ifdef _DEBUG
	CMagaDbg	dbg;
	dbg.printf("OrgTuning Point cx=%f cy=%f", m_ptTun[0].x, m_ptTun[0].y);
#endif
	return bCalc ? (*ms_pfnOrgDrillTuning)(this) : 0.0;	// ����ɂ��킹���ߐڍ��W : dummy
}

double CDXFpoint::OrgTuning_Seq(const CDXFpoint* pData)
{
	// �w���޼ު�Ă܂ł̋����v�Z
	return GAPCALC(pData->m_ptTun[0] - ms_pData->GetEndCutterPoint());
}

double CDXFpoint::OrgTuning_XY(const CDXFpoint*)
{
	return 0.0;		// ������Ƃɕ��בւ�����̂���а��Ԃ�
}

double CDXFpoint::GetSelectPointGap(const CPointD&)
{
	return HUGE_VAL;
}

BOOL CDXFpoint::GetDirectionArraw(const CPointD&, CPointD[][3]) const
{
	return FALSE;
}

void CDXFpoint::SetDirectionFixed(const CPointD&)
{
}

int CDXFpoint::GetIntersectionPoint(const CDXFdata*, CPointD[], BOOL) const
{
	return 0;
}

optional<CPointD>
CDXFpoint::CalcOffsetIntersectionPoint(const CDXFdata*, double, BOOL) const
{
	return optional<CPointD>();
}

int CDXFpoint::CheckIntersectionCircle(const CPointD&, double) const
{
	return 0;
}

optional<CPointD>
CDXFpoint::CalcExpandPoint(const CDXFdata*) const
{
	return optional<CPointD>();
}

//////////////////////////////////////////////////////////////////////
// �c�w�e�f�[�^��Line�N���X
//////////////////////////////////////////////////////////////////////
CDXFline::CDXFline() : CDXFpoint(DXFLINEDATA, NULL, 2)
{
}

CDXFline::CDXFline(ENDXFTYPE enType, CLayerData* pLayer, int nPoint) :
	CDXFpoint(enType, pLayer, nPoint)
{
}

CDXFline::CDXFline(LPDXFLARGV lpLine) : CDXFpoint(DXFLINEDATA, lpLine->pLayer, 2)
{
	m_pt[0] = lpLine->s;
	m_pt[1] = lpLine->e;
	SetMaxRect();
}

CDXFline::CDXFline(CLayerData* pLayer, const CDXFline* pData, LPDXFBLOCK lpBlock) :
	CDXFpoint(DXFLINEDATA, pLayer, 2)
{
	m_pt[0] = pData->GetNativePoint(0);
	m_pt[1] = pData->GetNativePoint(1);
	if ( lpBlock->dwBlockFlg & DXFBLFLG_X ) {
		m_pt[0].x *= lpBlock->dMagni[NCA_X];
		m_pt[1].x *= lpBlock->dMagni[NCA_X];
	}
	if ( lpBlock->dwBlockFlg & DXFBLFLG_Y ) {
		m_pt[0].y *= lpBlock->dMagni[NCA_Y];
		m_pt[1].y *= lpBlock->dMagni[NCA_Y];
	}
	if ( lpBlock->dwBlockFlg & DXFBLFLG_R ) {
		double	dRound = lpBlock->dRound*RAD;
		m_pt[0].RoundPoint(dRound);
		m_pt[1].RoundPoint(dRound);
	}
	m_pt[0] += lpBlock->ptOrg;
	m_pt[1] += lpBlock->ptOrg;
	SetMaxRect();
}

void CDXFline::Serialize(CArchive& ar)
{
	CDXFdata::Serialize(ar);
	if ( ar.IsStoring() )
		ar << m_pt[0].x << m_pt[0].y << m_pt[1].x << m_pt[1].y;
	else {
		ar >> m_pt[0].x >> m_pt[0].y >> m_pt[1].x >> m_pt[1].y;
		SetMaxRect();
	}
}

void CDXFline::DrawTuning(double f)
{
	m_ptDrawS = m_pt[0] * f;
	m_ptDrawE = m_pt[1] * f;
}

void CDXFline::Draw(CDC* pDC) const
{
#ifdef _DEBUGDRAW_DXF
	CMagaDbg	dbg("CDXFline::Draw()", DBG_RED);
	dbg.printf("pts.x=%d pts.y=%d pte.x=%d pte.y=%d",
		m_ptDrawS.x, m_ptDrawS.y, m_ptDrawE.x, m_ptDrawE.y);
#endif
	pDC->MoveTo(m_ptDrawS);
	pDC->LineTo(m_ptDrawE);
#ifdef _DEBUGDRAW_DXF_EDGE_
	CRect	rc(m_ptDrawE.x-10, m_ptDrawE.y-10, m_ptDrawE.x+10, m_ptDrawE.y+10);
	pDC->Ellipse(&rc);
#endif
}

double CDXFline::OrgTuning(BOOL bCalc/*=TRUE*/)
{
	ASSERT(ms_pData);
	m_ptTun[0] = m_pt[0] - ms_ptOrg;
	m_ptTun[1] = m_pt[1] - ms_ptOrg;
	OrgTuningBase();
#ifdef _DEBUG
	CMagaDbg	dbg;
	dbg.printf("OrgTuning Line sx=%f sy=%f ex=%f ey=%f",
					m_ptTun[0].x, m_ptTun[0].y, m_ptTun[1].x, m_ptTun[1].y);
#endif
	return bCalc ? GetEdgeGap(ms_pData->GetEndCutterPoint()) : 0.0;
}

double CDXFline::GetSelectPointGap(const CPointD& pt)
{
	return GetSelectPointGap_Line(m_rcMax, m_pt[0], m_pt[1], pt);
}

double CDXFline::GetSelectPointGap_Line
	(const CRectD& rcMax, const CPointD& pts, const CPointD& pte, const CPointD& pt) const
{
	double	dResult = HUGE_VAL;
	CPointD	pt1, pt2;

	// ���������̏ꍇ�CPtInRect() �m�f
	if ( fabs(pts.x-pte.x) < EPS ) {
		if ( min(pts.y, pte.y)<=pt.y && pt.y<=max(pts.y, pte.y) )
			dResult = fabs(pt.x - pts.x);
	}
	else if ( fabs(pts.y-pte.y) < EPS ) {
		if ( min(pts.x, pte.x)<=pt.x && pt.x<=max(pts.x, pte.x) )
			dResult = fabs(pt.y - pts.y);
	}
	else if ( rcMax.PtInRect(pt) ) {
		// �͈͓��̏ꍇ
		pt1 = pt  - pts;	// �n�_�����_��
		pt2 = pte - pts;
		double	l  = pt1.hypot(),
				qp = atan2(pt1.y, pt1.x),
				qs = atan2(pt2.y, pt2.x);
		dResult = l * fabs( sin(qp - qs) );
	}

	if ( dResult == HUGE_VAL ) {
		// �͈͊O�̏ꍇ�C�[�_����̋����v�Z
		double	d1 = GAPCALC(pts-pt), d2 = GAPCALC(pte-pt);
		dResult = sqrt(d1<d2 ? d1 : d2);	// �߂����̋�����Ԃ�
	}

	return dResult;
}

BOOL CDXFline::GetDirectionArraw(const CPointD&, CPointD pt[][3]) const
{
	return GetDirectionArraw_Line(m_pt, pt);
}

void CDXFline::SetDirectionFixed(const CPointD& pts)
{
	// �ŗL���W���߂����ɓ���ւ�
	if ( GAPCALC(m_pt[0]-pts) > GAPCALC(m_pt[1]-pts) )
		std::swap(m_pt[0], m_pt[1]);
}

int CDXFline::GetIntersectionPoint(const CDXFdata* pData, CPointD pt[], BOOL bEdge/*=TRUE*/) const
{
	int		nResult = 0;
	CPointD	pt1(pData->GetNativePoint(0)), pt2(pData->GetNativePoint(1));
	const CDXFcircle*	pCircle;
	const CDXFellipse*	pEllipse;
	optional<CPointD>	ptResult;

	// bEdge==TRUE : �[�_�������ꍇ�͌�_�Ȃ��Ƃ���
	switch ( pData->GetType() ) {
	case DXFLINEDATA:
		if ( bEdge &&
			(sqrt(GAPCALC(pt1-m_pt[0])) < EPS || sqrt(GAPCALC(pt1-m_pt[1])) < EPS ||
			 sqrt(GAPCALC(pt2-m_pt[0])) < EPS || sqrt(GAPCALC(pt2-m_pt[1])) < EPS) ) {
			break;
		}
		ptResult = ::CalcIntersectionPoint_LL(m_pt[0], m_pt[1], pt1, pt2);
		if ( ptResult ) {
			pt[0] = *ptResult;
			nResult = 1;
		}
		break;
	case DXFARCDATA:
		if ( bEdge &&
			(sqrt(GAPCALC(pt1-m_pt[0])) < EPS || sqrt(GAPCALC(pt1-m_pt[1])) < EPS ||
			 sqrt(GAPCALC(pt2-m_pt[0])) < EPS || sqrt(GAPCALC(pt2-m_pt[1])) < EPS) ) {
			break;
		}
		// through
	case DXFCIRCLEDATA:
		pCircle = static_cast<const CDXFcircle*>(pData);
		tie(nResult, pt1, pt2) = ::CalcIntersectionPoint_LC(m_pt[0], m_pt[1],
				pCircle->GetCenter(), pCircle->GetR());
		if ( pData->GetType() == DXFARCDATA ) {
			if ( nResult > 1 ) {
				if ( !pCircle->IsRangeAngle(pt2) )
					nResult--;
			}
			if ( nResult > 0 ) {
				if ( !pCircle->IsRangeAngle(pt1) ) {
					nResult--;
					if ( nResult > 0 )
						std::swap(pt1, pt2);
				}
			}
		}
		if ( nResult > 1 )
			pt[1] = pt2;
		if ( nResult > 0 )
			pt[0] = pt1;
		break;
	case DXFELLIPSEDATA:
		pEllipse = static_cast<const CDXFellipse*>(pData);
		if ( bEdge && pEllipse->IsArc() &&
			(sqrt(GAPCALC(pt1-m_pt[0])) < EPS || sqrt(GAPCALC(pt1-m_pt[1])) < EPS ||
			 sqrt(GAPCALC(pt2-m_pt[0])) < EPS || sqrt(GAPCALC(pt2-m_pt[1])) < EPS) ) {
			break;
		}
		tie(nResult, pt1, pt2) = ::CalcIntersectionPoint_LE(m_pt[0], m_pt[1],
				pEllipse->GetCenter(), pEllipse->GetLongLength(), pEllipse->GetShortLength(), pEllipse->GetLean());
		if ( pEllipse->IsArc() ) {
			if ( nResult > 1 ) {
				if ( !pEllipse->IsRangeAngle(pt2) )
					nResult--;
			}
			if ( nResult > 0 ) {
				if ( !pEllipse->IsRangeAngle(pt1) ) {
					nResult--;
					if ( nResult > 0 )
						std::swap(pt1, pt2);
				}
			}
		}
		if ( nResult > 1 )
			pt[1] = pt2;
		if ( nResult > 0 )
			pt[0] = pt1;
		break;
	}

	return nResult;
}

optional<CPointD>
CDXFline::CalcOffsetIntersectionPoint
	(const CDXFdata* pNext, double r, BOOL bLeft) const
{
	CPointD	pto( GetNativePoint(1) ), p1, p2, pts( GetNativePoint(0) ), pt;
	int		k1, k2, nRound;
	BOOL	bResult;
	const CDXFarc*		pArc;
	const CDXFellipse*	pEllipse;
	optional<CPointD>	ptResult;

	// ��_�����_��
	p1 = pts - pto;

	// �i�s�����ւ̕���
	pt = pto - pts;
	k1 = ::CalcOffsetSign(pt);
	if ( !bLeft )
		k1 = -k1;

	switch ( pNext->GetMakeType() ) {
	case DXFLINEDATA:
		p2 = pNext->GetNativePoint(1) - pto;
		k2 = ::CalcOffsetSign(p2);
		if ( !bLeft )
			k2 = -k2;
		ptResult = ::CalcOffsetIntersectionPoint_LL(p1, p2, k1, k2, r);
		break;
	case DXFARCDATA:
		pArc = static_cast<const CDXFarc*>(pNext);
		p2 = pArc->GetCenter() - pto;
		k2 = nRound = pArc->GetRoundOrig() ? -1 : 1;	// �����v��ϲŽ����
		if ( !bLeft )
			k2 = -k2;
		tie(bResult, pt, r) = ::CalcOffsetIntersectionPoint_LC(p1, p2,
				pArc->GetR(), r, nRound, k1, k2);
		if ( bResult )
			ptResult = pt;
		break;
	case DXFELLIPSEDATA:
		pEllipse = static_cast<const CDXFellipse*>(pNext);
		p2 = pEllipse->GetCenter() - pto;
		ptResult = ::CalcOffsetIntersectionPoint_LE(p1, p2,
			pEllipse->GetLongLength(), pEllipse->GetShortLength(), pEllipse->GetLean(), r,
			pEllipse->GetRoundOrig(), bLeft);
		break;
	}

	if ( ptResult ) {
		pt = *ptResult + pto;
		return pt;
	}
	return ptResult;
}

int CDXFline::CheckIntersectionCircle(const CPointD& ptc, double r) const
{
	int	nResult;
	CPointD	pt1, pt2;
	tie(nResult, pt1, pt2) = ::CalcIntersectionPoint_LC(m_pt[0], m_pt[1], ptc, r);
	// �u�ڂ���v�ꍇ�̌�������
	if ( nResult==1 && pt1!=pt2 )
		nResult = 2;	// ��_����ɉ���ύX
	return nResult;
}

optional<CPointD>
CDXFline::CalcExpandPoint(const CDXFdata* pData) const
{
	// �L�k�v�Z������i�͈͊O�̌�_�v�Z�j
	// ��{�I�ɂ� GetIntersectionPoint() �Ɠ���
	optional<CPointD>	ptResult;

	switch ( pData->GetType() ) {
	case DXFLINEDATA:
		ptResult = ::CalcIntersectionPoint_LL(m_pt[0], m_pt[1],
						pData->GetNativePoint(0), pData->GetNativePoint(1), FALSE);
		break;
	case DXFARCDATA:
		{
			int		nResult;
			CPointD	pt1, pt2;
			const CDXFarc* pArc = static_cast<const CDXFarc*>(pData);
			tie(nResult, pt1, pt2) = ::CalcIntersectionPoint_LC(m_pt[0], m_pt[1],
						pArc->GetCenter(), pArc->GetR(), FALSE);
			if ( nResult > 1 ) {
				// �I�_�ɋ߂�����I��
				ptResult = GAPCALC(pt1-m_pt[1]) < GAPCALC(pt2-m_pt[1]) ? pt1 : pt2;
			}
			else if ( nResult > 0 ) {
				ptResult = pt1;
			}
		}
		break;
	}

	return ptResult;
}

//////////////////////////////////////////////////////////////////////
// �c�w�e�f�[�^��Circle�N���X
//////////////////////////////////////////////////////////////////////
CDXFcircle::CDXFcircle() : CDXFline(DXFCIRCLEDATA, NULL, 4)
{
	m_nArrayExt = 0;
	m_bRound = FALSE;	// �~�ް��̉�]�����͕����w���̂�
	m_bRoundFixed = FALSE;
}

CDXFcircle::CDXFcircle(ENDXFTYPE enType, CLayerData* pLayer,
	const CPointD& c, double r, BOOL bRound, int nPoint) :
		CDXFline(enType, pLayer, nPoint)
{
	m_nArrayExt = 0;
	m_ct	= c;
	m_r		= r;
	m_rMake	= ::RoundUp(m_r);
	m_bRound = bRound;
	m_bRoundFixed = FALSE;
}

CDXFcircle::CDXFcircle(LPDXFCARGV lpCircle) :
	CDXFline(DXFCIRCLEDATA, lpCircle->pLayer, 4)
{
	m_nArrayExt = 0;
	m_ct	= lpCircle->c;
	m_r		= lpCircle->r;
	m_rMake	= ::RoundUp(m_r);
	m_bRound = FALSE;
	m_bRoundFixed = FALSE;
	SetCirclePoint();
	SetMaxRect();
}

CDXFcircle::CDXFcircle(CLayerData* pLayer, const CDXFcircle* pData, LPDXFBLOCK lpBlock) :
	CDXFline(DXFCIRCLEDATA, pLayer, 4)
{
	m_nArrayExt = 0;
	m_ct	= pData->GetCenter();
	m_r		= pData->GetR();
	// �~�̊g�嗦��X�������� ->
	// �@�e���Ŋg��k������Ƒȉ~�ɂȂ邪�C��ۯ�����̐�������(DXFDoc2.cpp)�Œ���
	if ( lpBlock->dwBlockFlg & DXFBLFLG_X ) {
		m_ct *= lpBlock->dMagni[NCA_X];
		m_r  *= lpBlock->dMagni[NCA_X];
	}
	if ( lpBlock->dwBlockFlg & DXFBLFLG_R )
		m_ct.RoundPoint(lpBlock->dRound*RAD);
	m_ct += lpBlock->ptOrg;
	m_rMake = pData->GetMakeR();
	m_bRound = pData->GetRound();
	m_bRoundFixed = pData->IsRoundFixed();
	SetCirclePoint();
	SetMaxRect();
}

void CDXFcircle::Serialize(CArchive& ar)
{
	CDXFdata::Serialize(ar);
	if ( ar.IsStoring() )
		ar << m_r << m_ct.x << m_ct.y;
	else {
		ar >> m_r >> m_ct.x >> m_ct.y;
		m_rMake	= ::RoundUp(m_r);
		SetCirclePoint();
		SetMaxRect();
	}
}

void CDXFcircle::XRev(void)
{
	if ( GetMakeType() == DXFPOINTDATA ) {
		m_ptTun[0].x  = -m_ptTun[0].x;
		m_ptMake[0].x = -m_ptMake[0].x;
	}
	else {
		CDXFdata::XRev();
		m_ctTun.x = -m_ctTun.x;
	}
}

void CDXFcircle::YRev(void)
{
	if ( GetMakeType() == DXFPOINTDATA ) {
		m_ptTun[0].y  = -m_ptTun[0].y;
		m_ptMake[0].y = -m_ptMake[0].y;
	}
	else {
		CDXFdata::YRev();
		m_ctTun.y = -m_ctTun.y;
	}
}

BOOL CDXFcircle::IsRangeAngle(const CPointD&) const
{
	return TRUE;
}

void CDXFcircle::DrawTuning(double f)
{
	m_rcDraw.TopLeft()		= (m_ct - m_r) * f;
	m_rcDraw.BottomRight()	= (m_ct + m_r) * f;
}

void CDXFcircle::Draw(CDC* pDC) const
{
#ifdef _DEBUGDRAW_DXF
	CMagaDbg	dbg("CDXFcircle::Draw()", DBG_RED);
	dbg.printStruct((LPRECT)&m_rcDraw, "m_rcDraw");
#endif
	pDC->Ellipse(m_rcDraw);
}

double CDXFcircle::OrgTuning(BOOL bCalc/*=TRUE*/)
{
	ASSERT(ms_pData);
	if ( GetMakeType() == DXFPOINTDATA ) {
		m_ptTun[0] = m_ct - ms_ptOrg;
		for ( int i=1; i<SIZEOF(m_ptTun); i++ )
			m_ptTun[i] = m_ptTun[0];
	}
	else {
		m_ctTun = m_ct - ms_ptOrg;
		// 0�`1 -> X��
		m_ptTun[0].x = m_ctTun.x + m_r;
		m_ptTun[1].x = m_ctTun.x - m_r;
		m_ptTun[0].y = m_ptTun[1].y = m_ctTun.y;
		// 2�`3 -> Y��
		m_ptTun[2].y = m_ctTun.y + m_r;
		m_ptTun[3].y = m_ctTun.y - m_r;
		m_ptTun[2].x = m_ptTun[3].x = m_ctTun.x;
		//
		m_nArrayExt = 0;
	}
	m_bRoundFixed = FALSE;	// �����w���̉���
	OrgTuningBase();
#ifdef _DEBUG
	CMagaDbg	dbg;
	dbg.printf("OrgTuning Circle s1=(%f, %f) s2=(%f, %f)", 
					m_ptTun[0].x, m_ptTun[0].y, m_ptTun[1].x, m_ptTun[1].y);
	dbg.printf("                 s3=(%f, %f) s4=(%f, %f)", 
					m_ptTun[2].x, m_ptTun[2].y, m_ptTun[3].x, m_ptTun[3].y);
#endif
	if ( GetMakeType() == DXFPOINTDATA )
		return bCalc ? (*ms_pfnOrgDrillTuning)(this) : 0.0;	// CDXFpoint::OrgTuning()
	else
		return bCalc ? GetEdgeGap(ms_pData->GetEndCutterPoint()) : 0.0;
}

double CDXFcircle::GetSelectPointGap(const CPointD& pt)
{
	return GetSelectPointGap_Circle(pt, 0.0, 360.0*RAD);
}

BOOL CDXFcircle::GetDirectionArraw(const CPointD& ptClick, CPointD ptResult[][3]) const
{
	CPointD	pt[2];
	// 1/4�n�_�I�_���擾
	GetQuarterPoint(ptClick, pt);
	// �ڐ��̌X�������߂� -> �_pt�ɒ���������̌X��
	double	q[] = {
		-atan2(pt[0].x - m_ct.x, pt[0].y - m_ct.y)+180.0*RAD,
		-atan2(pt[1].x - m_ct.x, pt[1].y - m_ct.y)
	};
	// �ڐ���������W�̌v�Z
	return GetDirectionArraw_Circle(TRUE, q, pt, ptResult);
}

void CDXFcircle::SetDirectionFixed(const CPointD&)
{
	// ���W����ւ��s�v
}

void CDXFcircle::SetRoundFixed(const CPointD& pts, const CPointD& pte)
{
	int	nLoop = GetPointNumber() - 1;
	// ��]�����̔���
	for ( int i=0; i<nLoop; i++ ) {
		if ( m_pt[i] == pts ) {
			m_bRound = m_pt[i+1] == pte ? TRUE : FALSE;
			return;
		}
	}
	if ( m_pt[i] == pts )	// 270������
		m_bRound = m_pt[i-1] == pte ? FALSE : TRUE;
	// ��]�����̌Œ�
	m_bRoundFixed = TRUE;
}

int CDXFcircle::GetIntersectionPoint(const CDXFdata* pData, CPointD pt[], BOOL) const
{
	int		nResult = 0;
	CPointD	pt1, pt2;
	const CDXFcircle*	pCircle;

	switch ( pData->GetType() ) {
	case DXFLINEDATA:
		tie(nResult, pt1, pt2) = ::CalcIntersectionPoint_LC(
				pData->GetNativePoint(0), pData->GetNativePoint(1), m_ct, m_r);
		break;
	case DXFCIRCLEDATA:
	case DXFARCDATA:
		pCircle = static_cast<const CDXFcircle*>(pData);
		tie(nResult, pt1, pt2) = ::CalcIntersectionPoint_CC(
				m_ct, pCircle->GetCenter(), m_r, pCircle->GetR());
		if ( pData->GetType() == DXFARCDATA ) {
			if ( nResult > 1 ) {
				if ( !pCircle->IsRangeAngle(pt2) )
					nResult--;
			}
			if ( nResult > 0 ) {
				if ( !pCircle->IsRangeAngle(pt1) ) {
					nResult--;
					if ( nResult > 0 )
						std::swap(pt1, pt2);
				}
			}
		}
		break;
	case DXFELLIPSEDATA:
		// ���l�v�Z������A�ĺ��ިݸ�
		pt[0] = pt[1] = 0;
		return 2;	// �Ƃ肠������_�����邱�Ƃɂ��Ă���
	}

	if ( nResult > 1 )
		pt[1] = pt2;
	if ( nResult > 0 )
		pt[0] = pt1;

	return nResult;
}

optional<CPointD>
CDXFcircle::CalcOffsetIntersectionPoint(const CDXFdata*, double, BOOL) const
{
	return optional<CPointD>();
}

int CDXFcircle::CheckIntersectionCircle(const CPointD&, double) const
{
	return 0;
}

optional<CPointD>
CDXFcircle::CalcExpandPoint(const CDXFdata*) const
{
	return optional<CPointD>();
}

//////////////////////////////////////////////////////////////////////
// �c�w�e�f�[�^��CircleEx�N���X
//////////////////////////////////////////////////////////////////////
CDXFcircleEx::CDXFcircleEx() : CDXFcircle(DXFCIRCLEDATA, NULL, CPointD(), 0.0, FALSE, 0)
{
}

CDXFcircleEx::CDXFcircleEx(ENDXFTYPE2 enType2, CLayerData* pLayer, const CPointD& pt, double r) :
	CDXFcircle(DXFCIRCLEDATA, pLayer, pt, r, FALSE, 0)
{
	m_enType2 = enType2;
	SetMaxRect();
}

void CDXFcircleEx::Serialize(CArchive& ar)
{
	int		nType;

	CDXFdata::Serialize(ar);
	if ( ar.IsStoring() ) {
		nType = m_enType2;
		ar << nType;
		ar << m_r << m_ct.x << m_ct.y;
	}
	else {
		ar >> nType;
		ar >> m_r >> m_ct.x >> m_ct.y;
		if ( nType < 0 || nType > 1 )	// DXFORGDATA �` DXFSTADATA
			AfxThrowUserException();
		m_rMake	= ::RoundUp(m_r);
		m_enType2 = (ENDXFTYPE2)nType;
		SetMaxRect();
	}
}

void CDXFcircleEx::XRev(void)
{
	m_ctTun.x  = -m_ctTun.x;
	m_ctMake.x = -m_ctMake.x;
}

void CDXFcircleEx::YRev(void)
{
	m_ctTun.y  = -m_ctTun.y;
	m_ctMake.y = -m_ctMake.y;
}

void CDXFcircleEx::Draw(CDC* pDC) const
{
#ifdef _DEBUGDRAW_DXF
	CMagaDbg	dbg("CDXFcircleEx::Draw()", DBG_RED);
	dbg.printStruct((LPRECT)&m_rcDraw, "m_rcDraw");
#endif
	if ( m_enType2 == DXFSTADATA ) {
		// ���H�J�n�ʒu��\���~
		pDC->Ellipse(&m_rcDraw);
	}
	// ���_��\���\��
	CPoint	pt(m_rcDraw.CenterPoint());
	pDC->MoveTo(m_rcDraw.left,  pt.y);
	pDC->LineTo(m_rcDraw.right, pt.y);
	pDC->MoveTo(pt.x, m_rcDraw.top);
	pDC->LineTo(pt.x, m_rcDraw.bottom);
}

double CDXFcircleEx::OrgTuning(BOOL/*=TRUE*/)
{
	m_ctTun = m_ct - ms_ptOrg;
	m_ctMake = m_ctTun.RoundUp();
	OrgTuningBase();
	return 0.0;		// dummy
}

//////////////////////////////////////////////////////////////////////
// �c�w�e�f�[�^��Arc�N���X
//////////////////////////////////////////////////////////////////////
CDXFarc::CDXFarc() : CDXFcircle(DXFARCDATA, NULL, CPointD(), 0.0, TRUE, 2)
{
}

CDXFarc::CDXFarc(ENDXFTYPE enType, CLayerData* pLayer,
	const CPointD& c, double r, double sq, double eq, BOOL bRound, int nPoint) :
		CDXFcircle(enType, pLayer, c, r, bRound, nPoint)
{
	m_bRoundOrig = m_bRound;
	m_sq = sq;
	m_eq = eq;
	AngleTuning();
	m_sqDraw = m_sq;
	m_eqDraw = m_eq;
}

CDXFarc::CDXFarc(LPDXFAARGV lpArc) :
	CDXFcircle(DXFARCDATA, lpArc->pLayer, lpArc->c, lpArc->r, TRUE, 2)
{
	m_bRoundOrig = m_bRound;
	m_sq = lpArc->sq * RAD;
	m_eq = lpArc->eq * RAD;
	AngleTuning();
	m_sqDraw = m_sq;
	m_eqDraw = m_eq;
	m_pt[0].SetPoint(m_r * cos(m_sq), m_r * sin(m_sq));
	m_pt[1].SetPoint(m_r * cos(m_eq), m_r * sin(m_eq));
	m_pt[0] += m_ct;
	m_pt[1] += m_ct;
#ifdef _DEBUG
	CMagaDbg	dbg("CDXFarc::CDXFarc()", DBG_RED);
	dbg.printf("sx=%f sy=%f", m_pt[0].x, m_pt[0].y);
	dbg.printf("ex=%f ey=%f", m_pt[1].x, m_pt[1].y);
#endif
	SetMaxRect();
	SetRsign();
}

CDXFarc::CDXFarc(LPDXFAARGV lpArc, BOOL bRound, const CPointD& pts, const CPointD& pte) :
	CDXFcircle(DXFARCDATA, lpArc->pLayer, lpArc->c, lpArc->r, bRound, 2)
{
	m_bRoundOrig = m_bRound;
	m_sqDraw = m_sq = lpArc->sq;		// ����׼ޱݒP�ʁC�����ς�
	m_eqDraw = m_eq = lpArc->eq;
	m_pt[0]  = pts;
	m_pt[1]  = pte;
	SetMaxRect();
	SetRsign();
}

CDXFarc::CDXFarc(CLayerData* pLayer, const CDXFarc* pData, LPDXFBLOCK lpBlock) :
	CDXFcircle(DXFARCDATA, pLayer, pData->GetCenter(), pData->GetR(), pData->GetRoundOrig(), 2)
{
	m_bRoundOrig = m_bRound;
	m_sq	= pData->GetStartAngle();
	m_eq	= pData->GetEndAngle();
	m_pt[0]	= pData->GetNativePoint(0);
	m_pt[1]	= pData->GetNativePoint(1);
	// �~�ʂ̊g�嗦��X�������� ->
	// �@�e���Ŋg��k������Ƒȉ~�ʂɂȂ邪�C��ۯ�����̐�������(DXFDoc2.cpp)�Œ���
	if ( lpBlock->dwBlockFlg & DXFBLFLG_X ) {
		m_ct	*= lpBlock->dMagni[NCA_X];
		m_pt[0]	*= lpBlock->dMagni[NCA_X];
		m_pt[1]	*= lpBlock->dMagni[NCA_X];
		m_r		*= lpBlock->dMagni[NCA_X];
		m_rMake = ::RoundUp(m_r);
	}
	if ( lpBlock->dwBlockFlg & DXFBLFLG_R ) {
		double	dRound = lpBlock->dRound*RAD;
		m_ct.RoundPoint(dRound);
		m_sq += dRound;		m_eq += dRound;
		m_pt[0].RoundPoint(dRound);
		m_pt[1].RoundPoint(dRound);
	}
	m_sqDraw = m_sq;
	m_eqDraw = m_eq;
	SetRsign();
	m_ct	+= lpBlock->ptOrg;
	m_pt[0]	+= lpBlock->ptOrg;
	m_pt[1]	+= lpBlock->ptOrg;
	// �~�ʂ� SetMaxRect() �͏������d���̂Ŏړx�����w�肳�ꂽ�Ƃ������Čv�Z
	if ( lpBlock->dwBlockFlg&DXFBLFLG_X || lpBlock->dwBlockFlg&DXFBLFLG_R )
		SetMaxRect();
	else {
		m_rcMax	= pData->GetMaxRect();
		m_rcMax.OffsetRect(lpBlock->ptOrg);
	}
}

void CDXFarc::Serialize(CArchive& ar)
{
	CDXFdata::Serialize(ar);
	if ( ar.IsStoring() )
		ar << m_r << m_ct.x << m_ct.y << m_sqDraw << m_eqDraw << m_bRoundOrig;
	else {
		ar >> m_r >> m_ct.x >> m_ct.y >> m_sqDraw >> m_eqDraw >> m_bRoundOrig;
		m_rMake	= ::RoundUp(m_r);
		m_sq = m_sqDraw;
		m_eq = m_eqDraw;
		SetRsign();
		m_pt[0].SetPoint(m_r * cos(m_sq), m_r * sin(m_sq));
		m_pt[1].SetPoint(m_r * cos(m_eq), m_r * sin(m_eq));
		m_pt[0] += m_ct;
		m_pt[1] += m_ct;
		m_bRound = m_bRoundOrig;
		SetMaxRect();
	}
}

void CDXFarc::SetMaxRect(void)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CDXFarc::SetMaxRect()", DBG_RED);
#endif
	CPointD	pts, pte, ptInit[4];
	double	sq, eq;

	// �n�_�I�_�Ɗp�x�̒���
	if ( m_bRoundOrig ) {
		pts = m_pt[0] - m_ct;
		pte = m_pt[1] - m_ct;
		sq = m_sqDraw;	eq = m_eqDraw;
	}
	else {
		pts = m_pt[1] - m_ct;
		pte = m_pt[0] - m_ct;
		sq = m_eqDraw;	eq = m_sqDraw;
	}

	// �e�ی��̎��ő�l
	ptInit[0].SetPoint(  m_r,    0 );
	ptInit[1].SetPoint(    0,  m_r );
	ptInit[2].SetPoint( -m_r,    0 );
	ptInit[3].SetPoint(    0, -m_r );

	// �Q�_�̋�`�͕K���ʂ�̂ŁC
	// �����l�Ƃ��čő�l�E�ŏ��l����
	// �޶�č��W�Ȃ̂ŁAtop��bottom�͋t
	m_rcMax.left	= min(pts.x, pte.x);
	m_rcMax.top		= max(pts.y, pte.y);
	m_rcMax.right	= max(pts.x, pte.x);
	m_rcMax.bottom	= min(pts.y, pte.y);

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

	// �ی��ʉ߂��ƂɎ��ő�l(r)����(top��bottom�͋t)
	int	a;
	for  ( j=1; j<=nCnt; j++ ) {
		a = ( i + j ) % 4;
		if ( m_rcMax.left > ptInit[a].x )
			m_rcMax.left = ptInit[a].x;
		if ( m_rcMax.top < ptInit[a].y )
			m_rcMax.top = ptInit[a].y;
		if ( m_rcMax.right < ptInit[a].x )
			m_rcMax.right = ptInit[a].x;
		if ( m_rcMax.bottom > ptInit[a].y )
			m_rcMax.bottom = ptInit[a].y;
	}

	// ���S���W�␳
	m_rcMax.OffsetRect(m_ct);
	m_rcMax.NormalizeRect();
#ifdef _DEBUG
	dbg.printf("l=%.3f t=%.3f r=%.3f b=%.3f",
		m_rcMax.left, m_rcMax.top, m_rcMax.right, m_rcMax.bottom);
#endif
}

void CDXFarc::XRev(void)
{
	CDXFdata::XRev();
	m_ctTun.x = -m_ctTun.x;
	SwapRound();	// �~�ʂ̏ꍇ�͉�]�������ύX
}

void CDXFarc::YRev(void)
{
	CDXFdata::YRev();
	m_ctTun.y = -m_ctTun.y;
	SwapRound();
}

BOOL CDXFarc::IsRangeAngle(const CPointD& pt) const
{
	CPointD	ptr( pt - m_ct );
	double	q = atan2(ptr.y, ptr.x);
	if ( q < 0 )
		q += 360.0*RAD;

	if ( m_bRoundOrig ) {
		if ( m_sq <= q && q <= m_eq )
			return TRUE;
	}
	else {
		if ( m_sq >= q && q >= m_eq )
			return TRUE;
	}

	return FALSE;
}

void CDXFarc::DrawTuning(double f)
{
	m_rDraw  = m_r * f;
	m_ptDraw = m_ct * f;
}

void CDXFarc::Draw(CDC* pDC) const
{
	// CDC::Arc() ���g���Ƃǂ����Ă��\�����Y����D���א����ɂ��ߎ�
#ifdef _DEBUGDRAW_DXF
	CMagaDbg	dbg("CDXFarc::Draw()", DBG_RED);
#endif
	double	sq, eq;
	if ( m_bRoundOrig ) {
		sq = m_sqDraw;	eq = m_eqDraw;
	}
	else {
		sq = m_eqDraw;	eq = m_sqDraw;
	}
	CPointD	pt(m_rDraw * cos(sq) + m_ptDraw.x, m_rDraw * sin(sq) + m_ptDraw.y);
	CPointD	ptBak(pt);
	pDC->MoveTo(pt);
#ifdef _DEBUGDRAW_DXF
	dbg.printf("pts.x=%d pts.y=%d", (int)pt.x, (int)pt.y);
#endif
	for ( sq+=ARCSTEP; sq<eq; sq+=ARCSTEP ) {
		pt.SetPoint(m_rDraw * cos(sq), m_rDraw * sin(sq));
		pt += m_ptDraw;
		pDC->LineTo(pt);
	}
	pt.SetPoint(m_rDraw * cos(eq), m_rDraw * sin(eq));
	pt += m_ptDraw;
	pDC->LineTo(pt);
#ifdef _DEBUGDRAW_DXF
	dbg.printf("pte.x=%d pte.y=%d", (int)pt.x, (int)pt.y);
#endif
#ifdef _DEBUGDRAW_DXF_EDGE_
	pt.SetPoint(m_rDraw * cos(m_eqDraw), m_rDraw * sin(m_eqDraw));
	pt += m_ptDraw;
	CRect	rc(pt.x-10, pt.y-10, pt.x+10, pt.y+10);
	pDC->Ellipse(&rc);
#endif
}

double CDXFarc::OrgTuning(BOOL bCalc/*=TRUE*/)
{
	m_bRound = m_bRoundOrig;	// ��]������������Ԃɖ߂�
	m_sq = m_sqDraw;
	m_eq = m_eqDraw;
	m_ctTun = m_ct - ms_ptOrg;
	// �ȉ� CDXFline �Ɠ���
#ifdef _DEBUG
	double	dResult = CDXFline::OrgTuning(bCalc);
	CMagaDbg	dbg;
	dbg.printf("   Round=%s cx=%f cy=%f", m_bRound ? "CCW" : "CW", m_ctTun.x, m_ctTun.y);
	dbg.printf("   sq=%f eq=%f", m_sq*DEG, m_eq*DEG);
	return dResult;
#else
	return CDXFline::OrgTuning(bCalc);
#endif
}

double CDXFarc::GetSelectPointGap(const CPointD& pt)
{
	double	sq, eq;
	if ( m_bRoundOrig ) {
		sq = m_sqDraw;	eq = m_eqDraw;
	}
	else {
		sq = m_eqDraw;	eq = m_sqDraw;
	}
	return GetSelectPointGap_Circle(pt, sq, eq);
}

BOOL CDXFarc::GetDirectionArraw(const CPointD&, CPointD pt[][3]) const
{
	// �ڐ��̌X�������߂� -> �_pt�ɒ���������̌X��
	double	q[] = {
		-atan2(m_pt[0].x - m_ct.x, m_pt[0].y - m_ct.y)+180.0*RAD,
		-atan2(m_pt[1].x - m_ct.x, m_pt[1].y - m_ct.y)
	};
	// �ڐ���������W�̌v�Z
	return GetDirectionArraw_Circle(m_bRoundOrig, q, m_pt, pt);
}

void CDXFarc::SetDirectionFixed(const CPointD& pts)
{
	// �ŗL���W�̓���ւ��Ɖ�]�����̔���
	if ( GAPCALC(m_pt[0]-pts) > GAPCALC(m_pt[1]-pts) ) {
		std::swap(m_pt[0], m_pt[1]);
		// ��]����
		m_bRound = m_bRoundOrig = !m_bRoundOrig;
		std::swap(m_sq, m_eq);
		std::swap(m_sqDraw, m_eqDraw);
	}
}

int CDXFarc::GetIntersectionPoint(const CDXFdata* pData, CPointD pt[], BOOL bEdge/*=TRUE*/) const
{
	int		nResult = 0;
	CPointD	pt1(pData->GetNativePoint(0)), pt2(pData->GetNativePoint(1));
	const CDXFcircle*	pCircle;
	const CDXFellipse*	pEllipse;

	// bEdge==TRUE : �[�_�������ꍇ�͌�_�Ȃ��Ƃ���
	switch ( pData->GetType() ) {
	case DXFLINEDATA:
		if ( bEdge &&
			(sqrt(GAPCALC(pt1-m_pt[0])) < EPS || sqrt(GAPCALC(pt1-m_pt[1])) < EPS ||
			 sqrt(GAPCALC(pt2-m_pt[0])) < EPS || sqrt(GAPCALC(pt2-m_pt[1])) < EPS) ) {
			break;
		}
		tie(nResult, pt1, pt2) = ::CalcIntersectionPoint_LC(pt1, pt2, m_ct, m_r);
		break;
	case DXFARCDATA:
		if ( bEdge &&
			(sqrt(GAPCALC(pt1-m_pt[0])) < EPS || sqrt(GAPCALC(pt1-m_pt[1])) < EPS ||
			 sqrt(GAPCALC(pt2-m_pt[0])) < EPS || sqrt(GAPCALC(pt2-m_pt[1])) < EPS) ) {
			break;
		}
		// through
	case DXFCIRCLEDATA:
		pCircle = static_cast<const CDXFcircle*>(pData);
		tie(nResult, pt1, pt2) = ::CalcIntersectionPoint_CC(m_ct, pCircle->GetCenter(), m_r, pCircle->GetR());
		break;
	case DXFELLIPSEDATA:
		pEllipse = static_cast<const CDXFellipse*>(pData);
		if ( bEdge && pEllipse->IsArc() &&
			(sqrt(GAPCALC(pt1-m_pt[0])) < EPS || sqrt(GAPCALC(pt1-m_pt[1])) < EPS ||
			 sqrt(GAPCALC(pt2-m_pt[0])) < EPS || sqrt(GAPCALC(pt2-m_pt[1])) < EPS) ) {
			break;
		}
		// ���l�v�Z������A�ĺ��ިݸ�
		pt[0] = pt[1] = 0;
		return 2;	// �Ƃ肠������_�����邱�Ƃɂ��Ă���
	}

	if ( nResult > 1 ) {
		if ( !IsRangeAngle(pt2) ||
				(pData->GetType()==DXFARCDATA && !pCircle->IsRangeAngle(pt2)) )
			nResult--;
	}
	if ( nResult > 0 ) {
		if ( !IsRangeAngle(pt1) ||
				(pData->GetType()==DXFARCDATA && !pCircle->IsRangeAngle(pt1)) ) {
			nResult--;
			if ( nResult > 0 )
				std::swap(pt1, pt2);
		}
	}

	if ( nResult > 1 )
		pt[1] = pt2;
	if ( nResult > 0 )
		pt[0] = pt1;

	return nResult;
}

optional<CPointD>
CDXFarc::CalcOffsetIntersectionPoint
	(const CDXFdata* pNext, double r, BOOL bLeft) const
{
	CPointD	pto( GetNativePoint(1) ), p1, p2, ptResult;
	int		k1, k2, nResult, nRound;
	BOOL	bResult = FALSE;
	const CDXFarc*	pArc;

	switch ( pNext->GetMakeType() ) {
	case DXFLINEDATA:
		// �������Ăōl���邽�߁A��]�����Ȃǂ𔽑΂ɂ���
		p1 = pNext->GetNativePoint(1) - pto;
		p2 = m_ct - pto;
		k1 = -(::CalcOffsetSign(p1));			// �n�_�I�_���t�Ȃ̂�ϲŽ����
		k2 = nRound = m_bRoundOrig ? 1 : -1;	// �����v����׽����
		if ( bLeft ) {		// �i�s�������t
			k1 = -k1;
			k2 = -k2;
		}
		tie(bResult, ptResult, r) = ::CalcOffsetIntersectionPoint_LC(p1, p2, m_r, r,
				nRound, k1, k2);
		if ( bResult )
			ptResult += pto;
		break;
	case DXFARCDATA:
		pArc = static_cast<const CDXFarc*>(pNext);
		k1 = m_bRoundOrig ? -1 : 1;
		k2 = pArc->GetRoundOrig() ? -1 : 1;
		if ( !bLeft ) {
			k1 = -k1;
			k2 = -k2;
		}
		tie(nResult, p1, p2) = ::CalcIntersectionPoint_CC(m_ct, pArc->GetCenter(),
				m_r+r*k1, pArc->GetR()+r*k2);
		if ( nResult > 0 ) {
			ptResult = GAPCALC(p1-pto) < GAPCALC(p2-pto) ? p1 : p2;
			bResult = TRUE;
		}
		else
			bResult = FALSE;
		break;
	}

	return bResult ? ptResult : optional<CPointD>();
}

int CDXFarc::CheckIntersectionCircle(const CPointD& ptc, double r) const
{
	int	nResult;
	CPointD	pt1, pt2;
	tie(nResult, pt1, pt2) = ::CalcIntersectionPoint_CC(m_ct, ptc, m_r, r);
	// ��_�������ɋ��߂�킯�ł͂Ȃ�
	if ( nResult > 1 && !IsRangeAngle(pt2) )
		nResult--;
	if ( nResult > 0 && !IsRangeAngle(pt1) )
		nResult--;

	return nResult;
}

optional<CPointD>
CDXFarc::CalcExpandPoint(const CDXFdata* pData) const
{
	int		nResult;
	CPointD	pt1, pt2;
	optional<CPointD>	ptResult;

	switch ( pData->GetType() ) {
	case DXFLINEDATA:
		tie(nResult, pt1, pt2) = ::CalcIntersectionPoint_LC(
				pData->GetNativePoint(0), pData->GetNativePoint(1),
				m_ct, m_r, FALSE);
		break;
	case DXFARCDATA:
		tie(nResult, pt1, pt2) = ::CalcIntersectionPoint_CC(
				m_ct, static_cast<const CDXFarc*>(pData)->GetCenter(),
				m_r,  static_cast<const CDXFarc*>(pData)->GetR());
		break;
	}

	if ( nResult > 1 ) {
		// �I�_�ɋ߂�����I��
		ptResult = GAPCALC(pt1-m_pt[1]) < GAPCALC(pt2-m_pt[1]) ? pt1 : pt2;
	}
	else if ( nResult > 0 ) {
		ptResult = pt1;
	}

	return ptResult;
}

//////////////////////////////////////////////////////////////////////
// �c�w�e�f�[�^��Ellipse�N���X
//////////////////////////////////////////////////////////////////////
CDXFellipse::CDXFellipse() : CDXFarc(DXFELLIPSEDATA, NULL, CPointD(), 0, 0, 0, TRUE, 4)
{
}

CDXFellipse::CDXFellipse(LPDXFEARGV lpEllipse) :
	CDXFarc(DXFELLIPSEDATA, lpEllipse->pLayer,
		lpEllipse->c, 0.0, lpEllipse->sq, lpEllipse->eq, lpEllipse->bRound, 4)
{
	m_ptLong = lpEllipse->l;
	m_dShort = lpEllipse->s;
	// �ȉ~�̌X�����v�Z
	m_lqMake = m_lq = atan2(m_ptLong.y, m_ptLong.x);
	// ����������
	Construct();
	// �e��v�Z
	EllipseCalc();
}

CDXFellipse::CDXFellipse(CLayerData* pLayer, const CDXFellipse* pData, LPDXFBLOCK lpBlock) :
	CDXFarc(DXFELLIPSEDATA, pLayer, pData->GetCenter(), pData->GetR(),
		pData->GetStartAngle(), pData->GetEndAngle(), pData->GetRoundOrig(), 4 )
{
	m_bArc   = pData->IsArc();
	m_ptLong = pData->GetLongPoint();
	m_dShort = pData->GetShortMagni();
	if ( lpBlock->dwBlockFlg & DXFBLFLG_X ) {
		m_ct.x *= lpBlock->dMagni[NCA_X];
		m_ptLong.x *= lpBlock->dMagni[NCA_X];
	}
	if ( lpBlock->dwBlockFlg & DXFBLFLG_Y ) {
		m_ct.y *= lpBlock->dMagni[NCA_Y];
		m_ptLong.y *= lpBlock->dMagni[NCA_Y];
	}
	if ( lpBlock->dwBlockFlg & DXFBLFLG_R ) {
		double	dRound = lpBlock->dRound*RAD;
		m_ct.RoundPoint(dRound);
		// �ȉ~�̉�]�́C�X��(m_lq)�Ŷ�ް
		m_ptLong.RoundPoint(dRound);
	}
	m_ct += lpBlock->ptOrg;
	if ( lpBlock->dwBlockFlg&DXFBLFLG_X || lpBlock->dwBlockFlg&DXFBLFLG_Y || lpBlock->dwBlockFlg&DXFBLFLG_R ) {
		m_dLongLength = m_ptLong.hypot();
		double	len = m_dLongLength * m_dShort;
		m_r = max(m_dLongLength, len);
		m_rMake = ::RoundUp(m_r);
		m_lqMake = m_lq = atan2(m_ptLong.y, m_ptLong.x);
		EllipseCalc();
	}
	else {
		m_dLongLength	= pData->GetLongLength();
		m_lqMake = m_lq	= pData->GetLean();
		m_lqDrawCos = m_lqMakeCos = pData->GetLeanCos();
		m_lqDrawSin = m_lqMakeSin = pData->GetLeanSin();
		CPointD	pt;
		m_nPoint = pData->GetPointNumber();
		for ( int i=0; i<m_nPoint; i++ )
			m_pt[i] = pData->GetNativePoint(i) + lpBlock->ptOrg;
		m_rcMax = pData->GetMaxRect();
		m_rcMax.OffsetRect(lpBlock->ptOrg);
	}
}

void CDXFellipse::Serialize(CArchive& ar)
{
	CDXFdata::Serialize(ar);
	if ( ar.IsStoring() ) {
		ar << m_ct.x << m_ct.y << m_sqDraw << m_eqDraw << m_bRoundOrig;
		ar << m_ptLong.x << m_ptLong.y << m_dShort << m_lq;
	}
	else {
		ar >> m_ct.x >> m_ct.y >> m_sqDraw >> m_eqDraw >> m_bRoundOrig;
		ar >> m_ptLong.x >> m_ptLong.y >> m_dShort >> m_lq;
		m_sq = m_sqDraw;
		m_eq = m_eqDraw;
		m_bRound = m_bRoundOrig;
		Construct();
		EllipseCalc();
	}
}

void CDXFellipse::Construct(void)
{
	m_dLongLength = m_ptLong.hypot();
	// �ݽ�׸��ł� m_r=0.0 ��n�������C
	// �֋X�㒷���Z���̒������𔼌a�ƌ��Ȃ�
	double	len = m_dLongLength * m_dShort;
	m_r = max(m_dLongLength, len);
	m_rMake = ::RoundUp(m_r);
	// �ȉ~���ȉ~�ʂ�
	m_bArc = fabs(m_eq-m_sq)*DEG+NCMIN < 360.0 ? TRUE : FALSE;
	if ( !m_bArc ) {
		m_sqDraw = m_sq = 0;
		m_eqDraw = m_eq = 360.0*RAD;
		AngleTuning();
	}
}

void CDXFellipse::EllipseCalc(void)
{
	int		i;
	double	dShort = m_dLongLength * m_dShort;

	// �X���v�Z
	m_lqDrawCos = m_lqMakeCos = cos(m_lq);
	m_lqDrawSin = m_lqMakeSin = sin(m_lq);
	// �ȉ~���ȉ~�ʂ��Őݒ肷��l���Ⴄ
	if ( m_bArc ) {
		// �~�ʂƓ����Ɏn�_�E�I�_�̌v�Z
		CPointD	pt[2];
		pt[0].SetPoint(m_dLongLength * cos(m_sq), dShort * sin(m_sq));
		pt[1].SetPoint(m_dLongLength * cos(m_eq), dShort * sin(m_eq));
		for ( i=0; i<SIZEOF(pt); i++ ) {
			m_pt[i].x = pt[i].x * m_lqDrawCos - pt[i].y * m_lqDrawSin + m_ct.x;
			m_pt[i].y = pt[i].x * m_lqDrawSin + pt[i].y * m_lqDrawCos + m_ct.y;
		}
		// �g��Ȃ����W�͕���
		m_nPoint = 2;
	}
	else {
		// �~�Ɠ���(�e���S�_�g�p)
		m_pt[0].SetPoint( m_dLongLength,     0.0 );	//   0��
		m_pt[1].SetPoint(           0.0,  dShort );	//  90��
		m_pt[2].SetPoint(-m_dLongLength,     0.0 );	// 180��
		m_pt[3].SetPoint(           0.0, -dShort );	// 270��
		CPointD	pt;
		for ( i=0; i<m_nPoint; i++ ) {
			pt = m_pt[i];
			m_pt[i].x = pt.x * m_lqDrawCos - pt.y * m_lqDrawSin + m_ct.x;
			m_pt[i].y = pt.x * m_lqDrawSin + pt.y * m_lqDrawCos + m_ct.y;
		}
	}
	// ��`�̌v�Z
	SetMaxRect();
}

void CDXFellipse::SetMaxRect(void)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CDXFellipse::SetMaxRect()", DBG_RED);
#endif
	CPointD	pt, pts, pte, ptc[4];
	double	sq, eq, dShort = m_dLongLength * m_dShort;

	// �p�x����
	if ( m_bRoundOrig ) {
		sq = m_sqDraw;	eq = m_eqDraw;
	}
	else {
		sq = m_eqDraw;	eq = m_sqDraw;
	}

	// �n�_�I�_�v�Z(m_pt�ɂ͌X���������Ă���̂ōČv�Z)
	if ( m_bArc ) {
		pts.SetPoint( m_dLongLength * cos(sq), dShort * sin(sq) );
		pte.SetPoint( m_dLongLength * cos(eq), dShort * sin(eq) );
	}
	else {
		pte.x = pts.x = m_dLongLength;
//		pte.y = pts.y = 0.0;
	}

	// �e�ی��̎��ő�l
	ptc[0].SetPoint(  m_dLongLength,       0 );
	ptc[1].SetPoint(              0,  dShort );
	ptc[2].SetPoint( -m_dLongLength,       0 );
	ptc[3].SetPoint(              0, -dShort );

	// �Q�_�̋�`�͕K���ʂ�̂ŁC
	// �����l�Ƃ��čő�l�E�ŏ��l����
	// �޶�č��W�Ȃ̂ŁAtop��bottom�͋t
	CRectD	rcMax( min(pts.x, pte.x), max(pts.y, pte.y),
				   max(pts.x, pte.x), min(pts.y, pte.y) );

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

	// �ی��ʉ߂��ƂɎ��ő�l(r)����(top��bottom�͋t)
	int	a;
	for  ( j=1; j<=nCnt; j++ ) {
		a = ( i + j ) % 4;
		if ( rcMax.left > ptc[a].x )
			rcMax.left = ptc[a].x;
		if ( rcMax.top < ptc[a].y )
			rcMax.top = ptc[a].y;
		if ( rcMax.right < ptc[a].x )
			rcMax.right = ptc[a].x;
		if ( rcMax.bottom > ptc[a].y )
			rcMax.bottom = ptc[a].y;
	}
	// �O�ڂ���l�p�`�̂S�p���W
	ptc[0].SetPoint( rcMax.right, rcMax.top );
	ptc[1].SetPoint( rcMax.left,  rcMax.top );
	ptc[2].SetPoint( rcMax.left,  rcMax.bottom );
	ptc[3].SetPoint( rcMax.right, rcMax.bottom );

	// �O�ڂ���l�p�`����]�����Đ�L��`���v�Z
	m_rcMax.SetRectMinimum();
	for ( i=0; i<SIZEOF(ptc); i++ ) {
		pt.x = ptc[i].x * m_lqDrawCos - ptc[i].y * m_lqDrawSin;
		pt.y = ptc[i].x * m_lqDrawSin + ptc[i].y * m_lqDrawCos;
		if ( m_rcMax.left   > pt.x )	m_rcMax.left = pt.x;
		if ( m_rcMax.top    > pt.y )	m_rcMax.top = pt.y;
		if ( m_rcMax.right  < pt.x )	m_rcMax.right = pt.x;
		if ( m_rcMax.bottom < pt.y )	m_rcMax.bottom = pt.y;
	}

	// ���S���W�␳
	m_rcMax.NormalizeRect();
	m_rcMax.OffsetRect(m_ct);
#ifdef _DEBUG
	dbg.printf("l=%.3f t=%.3f r=%.3f b=%.3f",
		m_rcMax.left, m_rcMax.top, m_rcMax.right, m_rcMax.bottom);
#endif
}

void CDXFellipse::XRev(void)
{
	if ( GetMakeType()==DXFPOINTDATA || GetMakeType()==DXFCIRCLEDATA )
		CDXFcircle::XRev();
	else {
		// �X���̂Ȃ��n�_�I�_���W�C���Ε����̖����u�~�v�ōČv�Z
		CPointD	pts(m_dLongLength * cos(m_sq), m_dLongLength * sin(m_sq)),
				pte(m_dLongLength * cos(m_eq), m_dLongLength * sin(m_eq));
		pts.x = -pts.x;
		pte.x = -pte.x;
		m_ctTun.x = -m_ctTun.x;
		XYRev(pts, pte);
	}
}

void CDXFellipse::YRev(void)
{
	if ( GetMakeType()==DXFPOINTDATA || GetMakeType()==DXFCIRCLEDATA )
		CDXFcircle::YRev();
	else {
		CPointD	pts(m_dLongLength * cos(m_sq), m_dLongLength * sin(m_sq)),
				pte(m_dLongLength * cos(m_eq), m_dLongLength * sin(m_eq));
		pts.y = -pts.y;
		pte.y = -pte.y;
		m_ctTun.y = -m_ctTun.y;
		XYRev(pts, pte);
	}
}

void CDXFellipse::XYRev(const CPointD& pts, const CPointD& pte)
{
	int		i;

	// ��]�����̔��]
	SwapRound();
	// �p�x�̍Čv�Z
	m_sq = atan2(pts.y, pts.x);
	m_eq = atan2(pte.y, pte.x);
	AngleTuning();
	// �X���̍Čv�Z(�����]�Ȃ̂ŕ������])
	m_lqMake = -m_lqMake;
	m_lqMakeCos = cos(m_lqMake);
	m_lqMakeSin = sin(m_lqMake);
	if ( m_bArc ) {
		// (�p�x�Čv�Z��)�Ε����̔��f
		CPointD	pt1[2], pt;
		pt1[0].SetPoint(pts.x, pts.y * m_dShort);
		pt1[1].SetPoint(pte.x, pte.y * m_dShort);
		for ( i=0; i<SIZEOF(pt1); i++ ) {		// �Q�_�̂�
			pt = pt1[i];
			// �����ް��֔��f
			m_ptTun[i].x = pt.x * m_lqMakeCos - pt.y * m_lqMakeSin + m_ctTun.x;
			m_ptTun[i].y = pt.x * m_lqMakeSin + pt.y * m_lqMakeCos + m_ctTun.y;
			m_ptMake[i] = m_ptTun[i].RoundUp();
		}
	}
	else {
		SetEllipseTunPoint();
		for ( i=0; i<4; i++ )		// �S�_
			m_ptMake[i] = m_ptTun[i].RoundUp();
	}
}

void CDXFellipse::SwapPt(int n)
{
	if ( !m_bArc || GetMakeType()==DXFPOINTDATA || GetMakeType()==DXFCIRCLEDATA ) {
		// ���W�̓���ւ��͕K�v�Ȃ�
//		CDXFdata::SwapPt(n);	// CDXFcircle::SwapPt()
		if ( !m_bArc ) {
			// �ȉ~�̏ꍇ�A�J�n�E�I���p�x�𒲐�
			switch ( m_nArrayExt ) {
			case 1:		// 180��
				m_sq = 180.0*RAD;
				break;
			case 2:		// 90��
				m_sq = 90.0*RAD;
				break;
			case 3:		// 270��
				m_sq = 270.0*RAD;
				break;
			default:	// 0��
				m_sq = 0.0;
				break;
			}
			m_eq = m_sq + 360.0*RAD;
			AngleTuning();
		}
	}
	else
		CDXFarc::SwapPt(n);
}

void CDXFellipse::SetEllipseTunPoint(void)
{
	double	dShort = m_dLongLength * m_dShort;
	// 0�`1 -> X��
	m_ptTun[0].x =  m_dLongLength;
	m_ptTun[1].x = -m_dLongLength;
	m_ptTun[0].y = m_ptTun[1].y = 0;
	// 2�`3 -> Y��
	m_ptTun[2].y =  dShort;
	m_ptTun[3].y = -dShort;
	m_ptTun[2].x = m_ptTun[3].x = 0;
	//
	m_nArrayExt = 0;
	// ��]
	CPointD	pt;
	for ( int i=0; i<GetPointNumber(); i++ ) {
		pt = m_ptTun[i];
		m_ptTun[i].x = pt.x * m_lqMakeCos - pt.y * m_lqMakeSin + m_ctTun.x;
		m_ptTun[i].y = pt.x * m_lqMakeSin + pt.y * m_lqMakeCos + m_ctTun.y;
	}
}

void CDXFellipse::DrawTuning(double f)
{
	m_dDrawLongLength = m_dLongLength * f;
	// ���S���W�̒���
	CDXFarc::DrawTuning(f);
}

void CDXFellipse::Draw(CDC* pDC) const
{
#ifdef _DEBUGDRAW_DXF
	CMagaDbg	dbg("CDXFellipse::Draw()", DBG_RED);
#endif
	double	sq, eq,
			dShort = m_dDrawLongLength * m_dShort;
	if ( m_bRoundOrig ) {
		sq = m_sqDraw;	eq = m_eqDraw;
	}
	else {
		sq = m_eqDraw;	eq = m_sqDraw;
	}
	CPointD	pt(m_dDrawLongLength * cos(sq), dShort * sin(sq));
	CPointD	ptDraw(pt.x * m_lqDrawCos - pt.y * m_lqDrawSin + m_ptDraw.x,
					pt.x * m_lqDrawSin + pt.y * m_lqDrawCos + m_ptDraw.y );
	CPointD	ptBak(ptDraw);
	pDC->MoveTo(ptDraw);
#ifdef _DEBUGDRAW_DXF
	dbg.printf("pts.x=%d pts.y=%d", (int)pt.x, (int)pt.y);
#endif
	for ( sq+=ARCSTEP; sq<eq; sq+=ARCSTEP ) {
		pt.SetPoint(m_dDrawLongLength * cos(sq), dShort * sin(sq));
		ptDraw.SetPoint(pt.x * m_lqDrawCos - pt.y * m_lqDrawSin,
						pt.x * m_lqDrawSin + pt.y * m_lqDrawCos);
		ptDraw += m_ptDraw;
		pDC->LineTo(ptDraw);
	}
	pt.SetPoint(m_dDrawLongLength * cos(eq), dShort * sin(eq));
	ptDraw.SetPoint(pt.x * m_lqDrawCos - pt.y * m_lqDrawSin,
					pt.x * m_lqDrawSin + pt.y * m_lqDrawCos);
	ptDraw += m_ptDraw;
	pDC->LineTo(ptDraw);
#ifdef _DEBUGDRAW_DXF
	dbg.printf("pte.x=%d pte.y=%d", (int)pt.x, (int)pt.y);
#endif
}

double CDXFellipse::OrgTuning(BOOL bCalc/*=TRUE*/)
{
	m_lqMake = m_lq;
	m_lqMakeCos = m_lqDrawCos;
	m_lqMakeSin = m_lqDrawSin;
	double	dResult;
	if ( GetMakeType()==DXFPOINTDATA || GetMakeType()==DXFCIRCLEDATA )
		dResult = CDXFcircle::OrgTuning(bCalc);
	else if ( m_bArc )
		dResult = CDXFarc::OrgTuning(bCalc);
	else {
		m_bRound = m_bRoundOrig;
		m_sq = m_sqDraw;
		m_eq = m_eqDraw;
		m_ctTun = m_ct - ms_ptOrg;
		SetEllipseTunPoint();
		OrgTuningBase();
		dResult = bCalc ? GetEdgeGap(ms_pData->GetEndCutterPoint()) : 0.0;
	}
#ifdef _DEBUG
	CMagaDbg	dbg;
	dbg.printf("   lx=%f ly=%f LongLen=%f Short=%f",
			m_ptLong.x, m_ptLong.y, m_dLongLength, m_dShort);
	dbg.printf("   lq=%f", m_lq*DEG);
#endif
	return dResult;
}

double CDXFellipse::GetSelectPointGap(const CPointD& pt)
{
	double	sq, eq, q1, q2;
	if ( m_bRoundOrig ) {
		sq = m_sqDraw;	eq = m_eqDraw;
	}
	else {
		sq = m_eqDraw;	eq = m_sqDraw;
	}
	CPointD	pt1(pt - m_ct);
	// �X����������ԂŔ��肷��悤�د��߲�Ă�␳
	if ( fabs(m_lq) > 0.0 )
		pt1.RoundPoint(-m_lq);
	// �Ε����̕␳��C�p�x�v�Z
	pt1.y /= m_dShort;
	if ( (q1=atan2(pt1.y, pt1.x)) < 0.0 )
		q1 += 360.0*RAD;
	q2 = q1 + 360.0*RAD;
	// �د��߲�Ă��p�x�͈͓̔��ɂ��邩
	return ( !m_bArc || (sq <= q1 && q1 <= eq) || (sq <= q2 && q2 <= eq) ) ?
		fabs(m_dLongLength - pt1.hypot()) : HUGE_VAL;
}

BOOL CDXFellipse::GetDirectionArraw(const CPointD& ptClick, CPointD ptResult[][3]) const
{
	CPointD	pt[2], pt1;
	double	q[2];

	if ( m_bArc ) {
		// �Ε����ƌX���p���l��
		for ( int i=0; i<SIZEOF(q); i++ ) {
			pt1 = pt[i] = m_pt[i];
			pt1 -= m_ct;
			if ( fabs(m_lq) > 0.0 )
				pt1.RoundPoint(-m_lq);
			q[i] = -atan2(pt1.x, pt1.y);
			pt1.x = cos(q[i]);
			pt1.y = sin(q[i]);
			q[i] = atan2(pt1.y*m_dShort, pt1.x) + m_lq;
		}
		q[0] += 180.0*RAD;
	}
	else {
		// �د��߲�Ă�␳
		pt1 = ptClick;
		if ( fabs(m_lq) > 0.0 ) {
			pt1 -= m_ct;
			pt1.RoundPoint(-m_lq);
			pt1 += m_ct;
		}
		GetQuarterPoint(pt1, pt);
		q[0] = -atan2(pt[0].x - m_ct.x, pt[0].y - m_ct.y)+180.0*RAD;
		q[1] = -atan2(pt[1].x - m_ct.x, pt[1].y - m_ct.y);
	}

	return GetDirectionArraw_Circle(m_bRoundOrig, q, pt, ptResult);
}

void CDXFellipse::SetDirectionFixed(const CPointD& pts)
{
	if ( m_bArc )	// �ȉ~�ʂ̏ꍇ����
		CDXFarc::SetDirectionFixed(pts);
	// �ȉ~�̏ꍇ�͉~���l�A���W����ւ��s�v
}

int CDXFellipse::GetIntersectionPoint(const CDXFdata* pData, CPointD pt[], BOOL bEdge/*=TRUE*/) const
{
	int		nResult = 0;
	CPointD	pt1(pData->GetNativePoint(0)), pt2(pData->GetNativePoint(1));
	const CDXFcircle*	pCircle;

	// bEdge==TRUE : �[�_�������ꍇ�͌�_�Ȃ��Ƃ���
	switch ( pData->GetType() ) {
	case DXFLINEDATA:
		if ( bEdge && m_bArc &&
			(sqrt(GAPCALC(pt1-m_pt[0])) < EPS || sqrt(GAPCALC(pt1-m_pt[1])) < EPS ||
			 sqrt(GAPCALC(pt2-m_pt[0])) < EPS || sqrt(GAPCALC(pt2-m_pt[1])) < EPS) ) {
			break;
		}
		tie(nResult, pt1, pt2) = ::CalcIntersectionPoint_LE(pt1, pt2,
				m_ct, m_dLongLength, m_dLongLength*m_dShort, m_lq);
		break;
	case DXFARCDATA:
		if ( bEdge && m_bArc &&
			(sqrt(GAPCALC(pt1-m_pt[0])) < EPS || sqrt(GAPCALC(pt1-m_pt[1])) < EPS ||
			 sqrt(GAPCALC(pt2-m_pt[0])) < EPS || sqrt(GAPCALC(pt2-m_pt[1])) < EPS) ) {
			break;
		}
		// through
	case DXFCIRCLEDATA:
		pCircle = static_cast<const CDXFcircle*>(pData);
		// ���l�v�Z������A�ĺ��ިݸ�
		pt[0] = pt[1] = 0;
		return 2;	// �Ƃ肠������_�����邱�Ƃɂ��Ă���
	}

	if ( m_bArc ) {
		if ( nResult > 1 ) {
			if ( !IsRangeAngle(pt2) ||
					(pData->GetType()==DXFARCDATA && !pCircle->IsRangeAngle(pt2)) )
				nResult--;
		}
		if ( nResult > 0 ) {
			if ( !IsRangeAngle(pt1) ||
					(pData->GetType()==DXFARCDATA && !pCircle->IsRangeAngle(pt1)) ) {
				nResult--;
				if ( nResult > 0 )
					std::swap(pt1, pt2);
			}
		}
	}

	if ( nResult > 1 )
		pt[1] = pt2;
	if ( nResult > 0 )
		pt[0] = pt1;

	return nResult;
}

optional<CPointD>
CDXFellipse::CalcOffsetIntersectionPoint
	(const CDXFdata* pNext, double r, BOOL bLeft) const
{
	CPointD	pto( GetNativePoint(1) ), p1, p2, pt;
	optional<CPointD>	ptResult;

	switch ( pNext->GetMakeType() ) {
	case DXFLINEDATA:
		p1 = pNext->GetNativePoint(1) - pto;
		p2 = m_ct - pto;
		ptResult = ::CalcOffsetIntersectionPoint_LE(p1, p2,
			m_dLongLength, m_dLongLength*m_dShort, m_lq, r,
			!m_bRoundOrig, !bLeft);
		break;
	}

	if ( ptResult ) {
		pt = *ptResult + pto;
		return pt;
	}
	return ptResult;
}

int CDXFellipse::CheckIntersectionCircle(const CPointD&, double) const
{
	return 0;
}

optional<CPointD>
CDXFellipse::CalcExpandPoint(const CDXFdata*) const
{
	return optional<CPointD>();
}

//////////////////////////////////////////////////////////////////////
// �c�w�e�f�[�^��Polyline�N���X
//////////////////////////////////////////////////////////////////////
CDXFpolyline::CDXFpolyline() : CDXFline(DXFPOLYDATA, NULL, 2)
{
	m_nPolyFlag = 0;
	m_bSeq = m_bSeqBak = TRUE;		// Default
	m_posSel = NULL;
	for ( int i=0; i<SIZEOF(m_nObjCnt); i++ )
		m_nObjCnt[i] = 0;
}

CDXFpolyline::CDXFpolyline(CLayerData* pLayer, const CDXFpolyline* pPoly, LPDXFBLOCK lpBlock) :
	CDXFline(DXFPOLYDATA, pLayer, 2)
{
	m_nPolyFlag = pPoly->GetPolyFlag();
	m_bSeq = m_bSeqBak = pPoly->GetSequence();
	m_posSel = NULL;
	for ( int i=0; i<SIZEOF(m_nObjCnt); i++ )
		m_nObjCnt[i] = 0;
	// ��O�۰�͏�ʂŷ���
	CDXFdata*		pDataSrc;
	CDXFdata*		pData;
	DXFEARGV		dxfEllipse;
	for ( POSITION pos=pPoly->m_ltVertex.GetHeadPosition(); pos; ) {
		pDataSrc = pPoly->m_ltVertex.GetNext(pos);
		switch ( pDataSrc->GetType() ) {
		case DXFPOINTDATA:
			pData = new CDXFpoint(NULL, static_cast<CDXFpoint*>(pDataSrc), lpBlock);
			ASSERT(pData);
			m_ltVertex.AddTail(pData);
			break;
		case DXFARCDATA:
			// �e���Ǝ��̊g�嗦�� CDXFarc -> CDXFellipse
			if ( lpBlock->dMagni[NCA_X] != lpBlock->dMagni[NCA_Y] ) {
				(static_cast<CDXFarc*>(pDataSrc))->SetEllipseArgv(lpBlock, &dxfEllipse);
				pData = new CDXFellipse(&dxfEllipse); 
				ASSERT(pData);
				m_ltVertex.AddTail(pData);
			}
			else {
				pData = new CDXFarc(NULL, static_cast<CDXFarc*>(pDataSrc), lpBlock);
				ASSERT(pData);
				m_ltVertex.AddTail(pData);
			}
			break;
		}
	}
	EndSeq();
}

CDXFpolyline::~CDXFpolyline()
{
	for ( POSITION pos = m_ltVertex.GetHeadPosition(); pos; )
		delete	m_ltVertex.GetNext(pos);
}

void CDXFpolyline::Serialize(CArchive& ar)
{
	CDXFdata::Serialize(ar);
	if ( ar.IsStoring() )
		ar << m_nPolyFlag << m_bSeqBak;
	else {
		ar >> m_nPolyFlag >> m_bSeqBak;
		m_bSeq = m_bSeqBak;
	}
	m_ltVertex.Serialize(ar);
	if ( !ar.IsStoring() )
		EndSeq();
}

void CDXFpolyline::XRev(void)
{
	// �e��޼ު�Ă� OrgTuning() ������ɌĂ΂�Ă���
	CDXFdata::XRev();
}

void CDXFpolyline::YRev(void)
{
	// �e��޼ު�Ă� OrgTuning() ������ɌĂ΂�Ă���
	CDXFdata::YRev();
}

void CDXFpolyline::SwapPt(int)
{
	// �������g�̍��W����ւ�
	m_bSeq = !m_bSeq;
	CDXFdata::SwapPt(0);

	// �\����޼ު�Ă̍��W����ւ�
	CDXFdata* pData;
	for ( POSITION pos=m_ltVertex.GetHeadPosition(); pos; ) {
		// CDXFpoint�ȊO�͉�]��������ݼ�
		pData = m_ltVertex.GetNext(pos);
		if ( pData->GetType() != DXFPOINTDATA )
			pData->ReversePt();		// SwapPt() �� protected�錾
	}
}

BOOL CDXFpolyline::SetVertex(LPDXFPARGV lpArgv)
{
	lpArgv->pLayer = NULL;
	CDXFpoint*	pPoint = new CDXFpoint(lpArgv);
	ASSERT(pPoint);
	m_ltVertex.AddTail(pPoint);
	return TRUE;
}

BOOL CDXFpolyline::SetVertex(LPDXFPARGV lpArgv, double dBow, const CPointD& pts)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CDXFpolyline::SetVertex()", DBG_RED);
#endif

	double	q = fabs(4 * atan(dBow));
	double	d = _hypot(lpArgv->c.x - pts.x, lpArgv->c.y - pts.y);
	// �Q�_���������ꍇ�͵�޼ު�Ă𐶐����Ȃ�
	if ( d < NCMIN )
		return TRUE;
	double	r = fabs( (d/2) / sin(q/2) );
	// �~�̕��������C(s.x, s.y)�C(e.x, e.y) �𒆐S�Ƃ���
	// �Q�̉~�̌�_�����߂�
	CPointD	pt1, pt2;
	int		nResult;
	tie(nResult, pt1, pt2) = ::CalcIntersectionPoint_CC(pts, lpArgv->c, r, r);
	if ( nResult < 1 )
		return FALSE;
	// CDXFarc�o�^����
	DXFAARGV	dxfArc;
	dxfArc.r		= r;
	dxfArc.pLayer	= NULL;
	// �ǂ���̉����̗p���邩
	BOOL	bRound;
	double	sq1, eq1, sq2, eq2;
	if ( (sq1=atan2(pts.y - pt1.y, pts.x - pt1.x)) < 0.0 )
		sq1 += 360.0*RAD;
	if ( (eq1=atan2(lpArgv->c.y - pt1.y, lpArgv->c.x - pt1.x)) < 0.0 )
		eq1 += 360.0*RAD;
	if ( nResult == 1 ) {	// �d��
		sq2 = sq1;	eq2 = eq1;
	}
	else {
		if ( (sq2=atan2(pts.y - pt2.y, pts.x - pt2.x)) < 0.0 )
			sq2 += 360.0*RAD;
		if ( (eq2=atan2(lpArgv->c.y - pt2.y, lpArgv->c.x - pt2.x)) < 0.0 )
			eq2 += 360.0*RAD;
	}
#ifdef _DEBUG
	dbg.printf("pts x=%f y=%f pte x=%f y=%f", pts.x, pts.y, lpArgv->c.x, lpArgv->c.y);
	dbg.printf("q=%f d=%f r=%f", q*DEG, d/2, r);
	dbg.printf("ptc1 x=%f y=%f sq1=%f eq1=%f", pt1.x, pt1.y, sq1*DEG, eq1*DEG);
	dbg.printf("ptc2 x=%f y=%f sq2=%f eq2=%f", pt2.x, pt2.y, sq2*DEG, eq2*DEG);
#endif

	if ( dBow > 0 ) {	// �����v���w��
		bRound = TRUE;
		while ( sq1 > eq1 )
			eq1 += 360.0*RAD;
		while ( sq2 > eq2 )
			eq2 += 360.0*RAD;
		if ( fabs(eq1 - sq1 - q) < fabs(eq2 - sq2 - q) ) {
			dxfArc.c	= pt1;
			dxfArc.sq	= sq1;
			dxfArc.eq	= eq1;
#ifdef _DEBUG
			dbg.printf("CCW pt1");
#endif
		}
		else {
			dxfArc.c	= pt2;
			dxfArc.sq	= sq2;
			dxfArc.eq	= eq2;
#ifdef _DEBUG
			dbg.printf("CCW pt2");
#endif
		}
	}
	else {
		bRound = FALSE;
		while ( sq1 < eq1 )
			sq1 += 360.0*RAD;
		while ( sq2 < eq2 )
			sq2 += 360.0*RAD;
		if ( fabs(sq1 - eq1 - q) < fabs(sq2 - eq2 - q) ) {
			dxfArc.c	= pt1;
			dxfArc.sq	= sq1;
			dxfArc.eq	= eq1;
#ifdef _DEBUG
			dbg.printf("CW pt1");
#endif
		}
		else {
			dxfArc.c	= pt2;
			dxfArc.sq	= sq2;
			dxfArc.eq	= eq2;
#ifdef _DEBUG
			dbg.printf("CW pt2");
#endif
		}
	}
	CDXFarc*	pArc = new CDXFarc(&dxfArc, bRound, pts, lpArgv->c);
	ASSERT(pArc);
	m_ltVertex.AddTail(pArc);

	// NC���ނ𐶐����₷���悤��CDXFpoint��޼ު�Ă�����
	DXFPARGV	dxfPoint;
	dxfPoint.c = lpArgv->c;
	CDXFpoint*	pPoint = new CDXFpoint(&dxfPoint);
	ASSERT(pPoint);
	m_ltVertex.AddTail(pPoint);

	return TRUE;
}

void CDXFpolyline::EndSeq(void)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CDXFpolyline::EndSeq()", DBG_RED);
#endif
	ASSERT( !m_ltVertex.IsEmpty() );
	ASSERT( m_ltVertex.GetHead()->GetType() == DXFPOINTDATA );
	CDXFdata*	pData;
	// �ŏ��ƍŌ�̓_�͕K��CDXFpoint
	m_pt[0] = m_ltVertex.GetHead()->GetNativePoint(0);
	m_pt[1] = m_nPolyFlag & 1 ? m_pt[0] : m_ltVertex.GetTail()->GetNativePoint(0);
	
	// SetMaxRect() and DataCount
	m_rcMax.TopLeft()     = m_pt[0];
	m_rcMax.BottomRight() = m_pt[0];
	CPointD	pt;
	for ( POSITION pos=m_ltVertex.GetHeadPosition(); pos; ) {
		pData = m_ltVertex.GetNext(pos);
		switch ( pData->GetType() ) {
		case DXFPOINTDATA:
			pt = pData->GetNativePoint(0);
			if ( m_rcMax.left > pt.x )
				m_rcMax.left = pt.x;
			if ( m_rcMax.top > pt.y )
				m_rcMax.top = pt.y;
			if ( m_rcMax.right < pt.x )
				m_rcMax.right = pt.x;
			if ( m_rcMax.bottom < pt.y )
				m_rcMax.bottom = pt.y;
			m_nObjCnt[0]++;
			break;
		case DXFARCDATA:
			m_rcMax |= pData->GetMaxRect();
			m_nObjCnt[1]++;
			m_nObjCnt[0]--;		// �I�_�̕������Z
			break;
		case DXFELLIPSEDATA:
			m_rcMax |= pData->GetMaxRect();
			m_nObjCnt[2]++;
			m_nObjCnt[0]--;
			break;
		}
	}
	// �n�_���܂܂�邽�߁u-1�v�Ő��̐�
	m_nObjCnt[0]--;
#ifdef _DEBUG
	dbg.printf("LineCnt=%d ArcCnt=%d Ellipse=%d",
			m_nObjCnt[0], m_nObjCnt[1], m_nObjCnt[2]);
	dbg.printf("l=%.3f t=%.3f r=%.3f b=%.3f",
		m_rcMax.left, m_rcMax.top, m_rcMax.right, m_rcMax.bottom);
#endif
}

void CDXFpolyline::DrawTuning(double f)
{
	for ( POSITION pos=m_ltVertex.GetHeadPosition(); pos; )
		m_ltVertex.GetNext(pos)->DrawTuning(f);
}

void CDXFpolyline::Draw(CDC* pDC) const
{
#ifdef _DEBUGDRAW_DXF
	CMagaDbg	dbg("CDXFpolyline::Draw()", DBG_RED);
	int		nDbgCnt;
	CPoint	ptDbg;
#endif
	CDXFdata*	pData;
	POSITION pos = m_ltVertex.GetHeadPosition();
	// �P�_�ڂ͕K��CDXFpoint�D��ٰ�߂̂��߂̍��W���擾
	CPoint	pt( static_cast<CDXFpoint*>(m_ltVertex.GetNext(pos))->GetDrawPoint() );
	pDC->MoveTo(pt);

	// �Q�_�ڂ���ٰ��
#ifdef _DEBUGDRAW_DXF
	for ( nDbgCnt=0; pos; nDbgCnt++ ) {
#else
	while ( pos ) {
#endif
		pData = m_ltVertex.GetNext(pos);
		if ( pData->GetType() == DXFPOINTDATA ) {
#ifdef _DEBUGDRAW_DXF
			ptDbg = static_cast<CDXFpoint*>(pData)->GetDrawPoint();
			dbg.printf("No.%03d: x=%d y=%d", nDbgCnt, ptDbg.x, ptDbg.y);
#endif
			pDC->LineTo( static_cast<CDXFpoint*>(pData)->GetDrawPoint() );
		}
		else {
			pData->Draw(pDC);
			m_ltVertex.GetNext(pos);	// �I�_�����΂�
		}
	}
	// �������ײ݂Ȃ�
	if ( m_nPolyFlag & 1 )
		pDC->LineTo(pt);	// �J�n���W�܂ō�}
}

double CDXFpolyline::OrgTuning(BOOL)
{
	m_bSeq = m_bSeqBak;		// ������������Ԃɖ߂�
	// �e�v�f�͒l�̓���ւ��⌴�_����̋����͕K�v�Ȃ��̂� OrgTuning(FALSE) �ŌĂ�
	for ( POSITION pos=m_ltVertex.GetHeadPosition(); pos; )
		m_ltVertex.GetNext(pos)->OrgTuning(FALSE);
	// �ȉ� CDXFline �Ɠ���
	return CDXFline::OrgTuning();
}

double CDXFpolyline::GetSelectPointGap(const CPointD& pt)
{
	CDXFdata*	pData;
	CPointD		ptf, pts, pte;
	CRectD		rc;
	double		dGap, dGapMin = HUGE_VAL;
	POSITION	pos1 = m_ltVertex.GetHeadPosition(), pos2;
	// �P�_�ڂ͕K��CDXFpoint�D��ٰ�߂̂��߂̍��W���擾
	ptf = pts = m_ltVertex.GetNext(pos1)->GetNativePoint(0);
	rc.TopLeft() = pts;

	// �Q�_�ڂ���ٰ��
	while ( pos2 = pos1 ) {
		pData = m_ltVertex.GetNext(pos1);
		if ( pData->GetType() == DXFPOINTDATA ) {
			pte = pData->GetNativePoint(0);
			rc.BottomRight() = pte;
			rc.NormalizeRect();
			dGap = GetSelectPointGap_Line(rc, pts, pte, pt);
		}
		else {
			dGap = pData->GetSelectPointGap(pt);
			pte = pData->GetNativePoint(1);
			m_ltVertex.GetNext(pos1);	// �I�_�����΂�
		}
		if ( dGap < dGapMin ) {
			dGapMin = dGap;
			m_posSel = pos2;
		}
		rc.TopLeft() = pts = pte;
	}

	// �������ײ݂Ȃ�
	if ( m_nPolyFlag & 1 ) {
		rc.BottomRight() = ptf;
		rc.NormalizeRect();
		dGap = GetSelectPointGap_Line(rc, pts, ptf, pt);
		if ( dGap < dGapMin ) {
			dGapMin = dGap;
			m_posSel = m_ltVertex.GetHeadPosition();
		}
	}

	return dGapMin;
}

BOOL CDXFpolyline::GetDirectionArraw(const CPointD& ptClick, CPointD ptResult[][3]) const
{
	// ���ײݑS�̂̎n�_�I�_�ł́u�������ײ݁v�̖�󂪏����Ȃ��̂�
	// �Y���v�f(�د��_)�ɋ߂���޼ު�Ă̖����W�����߂�
	ASSERT( m_posSel );	// GetSelectPointGap() �ž�Ă���Ă���ʽ�

	POSITION	pos = m_posSel;
	BOOL	bResult;
	CPointD	pt[2];
	CDXFdata*	pData = m_ltVertex.GetPrev(pos);

	if ( pData->GetType() == DXFPOINTDATA ) {
		if ( pos ) {
			pt[1] = pData->GetNativePoint(0);	// �I�_
			pData = m_ltVertex.GetPrev(pos);
			pt[0] = pData->GetNativePoint(0);	// �n�_
		}
		else {
			// �������ײ݂ōŌ�̵�޼ު�Ă�˯�
			pt[0] = pData->GetNativePoint(0);					// �n�_
			pt[1] = m_ltVertex.GetTail()->GetNativePoint(0);	// �I�_
		}
		bResult = GetDirectionArraw_Line(pt, ptResult);
	}
	else
		bResult = pData->GetDirectionArraw(ptClick, ptResult);

	return bResult;
}

void CDXFpolyline::SetDirectionFixed(const CPointD& pts)
{
	// �ŗL���W�̓���ւ��Ə����ύX
	if ( GAPCALC(m_pt[0]-pts) > GAPCALC(m_pt[1]-pts) ) {
		std::swap(m_pt[0], m_pt[1]);
		// �����ύX
		m_bSeq = m_bSeqBak = !m_bSeqBak;
		CDXFdata*	pData;
		CPointD		pt(m_pt[0]);
		for ( POSITION pos=GetFirstVertex(); pos; ) {
			// CDXFpoint�ȊO�͌ŗL���W�����ւ�
			pData = GetNextVertex(pos);
			if ( pData->GetType() != DXFPOINTDATA ) {
				pData->SetDirectionFixed(pt);
				pt = pData->GetNativePoint(1);
			}
			else
				pt = pData->GetNativePoint(0);
		}
	}
}

int CDXFpolyline::GetIntersectionPoint(const CDXFdata*, CPointD[], BOOL) const
{
	return 0;
}

optional<CPointD>
CDXFpolyline::CalcOffsetIntersectionPoint(const CDXFdata*, double, BOOL) const
{
	return optional<CPointD>();
}

int CDXFpolyline::CheckIntersectionCircle(const CPointD&, double) const
{
	return 0;
}

optional<CPointD>
CDXFpolyline::CalcExpandPoint(const CDXFdata*) const
{
	return optional<CPointD>();
}

//////////////////////////////////////////////////////////////////////
// �c�w�e�f�[�^��Text�N���X
//////////////////////////////////////////////////////////////////////
CDXFtext::CDXFtext() : CDXFpoint(DXFTEXTDATA, NULL, 1)
{
}

CDXFtext::CDXFtext(LPDXFTARGV lpText) : CDXFpoint(DXFTEXTDATA, lpText->pLayer, 1)
{
	m_pt[0] = lpText->c;
	m_strValue = lpText->strValue;
	SetMaxRect();
}

CDXFtext::CDXFtext(CLayerData* pLayer, const CDXFtext* pData, LPDXFBLOCK lpBlock) :
	CDXFpoint(DXFTEXTDATA, pLayer, 1)
{
	m_pt[0] = pData->GetNativePoint(0);
	if ( lpBlock->dwBlockFlg & DXFBLFLG_X )
		m_pt[0].x *= lpBlock->dMagni[NCA_X];
	if ( lpBlock->dwBlockFlg & DXFBLFLG_Y )
		m_pt[0].y *= lpBlock->dMagni[NCA_Y];
	if ( lpBlock->dwBlockFlg & DXFBLFLG_R )
		m_pt[0].RoundPoint(lpBlock->dRound*RAD);
	m_pt[0] += lpBlock->ptOrg;
	m_strValue = pData->GetStrValue();
	SetMaxRect();
}

void CDXFtext::Serialize(CArchive& ar)
{
	CDXFdata::Serialize(ar);
	if ( ar.IsStoring() )
		ar << m_pt[0].x << m_pt[0].y << m_strValue;
	else {
		ar >> m_pt[0].x >> m_pt[0].y >> m_strValue;
		SetMaxRect();
	}
}

void CDXFtext::Draw(CDC* pDC) const
{
#ifdef _DEBUGDRAW_DXF
	CMagaDbg	dbg("CDXFtext::Draw()", DBG_RED);
	dbg.printStruct((LPRECT)&m_ptDraw, "m_ptDraw");
#endif
	pDC->TextOut(m_ptDraw.x, m_ptDraw.y, m_strValue);
	pDC->Ellipse(&m_rcDraw);
}

double CDXFtext::OrgTuning(BOOL/*=TRUE*/)
{
	CDXFpoint::OrgTuning(FALSE);	// �����v�Z�̕K�v�Ȃ�
	return HUGE_VAL;
}
