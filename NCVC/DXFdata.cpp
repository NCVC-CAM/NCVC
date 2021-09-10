// DXFdata.cpp: CDXFdata �N���X�̃C���v�������e�[�V����
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "DXFdata.h"
#include "DXFDoc.h"
#include "Layer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
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
IMPLEMENT_SERIAL(CDXFspline, CDXFline, NCVCSERIALVERSION|VERSIONABLE_SCHEMA)
IMPLEMENT_SERIAL(CDXFtext, CDXFpoint, NCVCSERIALVERSION|VERSIONABLE_SCHEMA)

using namespace boost;
extern	DWORD	g_dwCamVer;		// NCVC.cpp

/////////////////////////////////////////////////////////////////////////////
// �ÓI�ϐ��̏�����
CPointF		CDXFdata::ms_ptOrg(HUGE_VALF, HUGE_VALF);
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

CDXFdata::CDXFdata(ENDXFTYPE enType, CLayerData* pLayer, int nPoint, DWORD dwFlags)
{
	m_enType	= m_enMakeType = enType;
	m_dwFlags	= dwFlags;
	m_pParentLayer = pLayer;
	m_pParentMap   = NULL;
	m_nPoint	= nPoint;
	if ( nPoint > 0 ) {
		m_pt		= new CPointF[nPoint];
		m_ptTun		= new CPointF[nPoint];
		m_ptMake	= new CPointF[nPoint];
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

#ifdef _DEBUG
void CDXFdata::DbgDump(void)
{
	for ( int i=0; i<m_nPoint; i++ ) {
		printf("No.%d x=%.3f y=%.3f\n", i+1, m_pt[i].x, m_pt[i].y);
	}
}
#endif

void CDXFdata::Serialize(CArchive& ar)
{
	if ( ar.IsStoring() ) {
		ar << (m_dwFlags & ~DXFFLG_SELECT);	// �I����ԏ���
		if ( !(m_dwFlags & DXFFLG_POLYCHILD) )
			m_nSerialSeq = ms_nSerialSeq++;
	}
	else {
		ar >> m_dwFlags;
		m_pParentLayer = static_cast<CDXFDoc *>(ar.m_pDocument)->GetSerializeLayer();
	}
}

CPen* CDXFdata::GetDrawPen(void) const
{
	CPen*	pDrawPen[] = {
		AfxGetNCVCMainWnd()->GetPenDXF(DXFPEN_CUTTER),
		AfxGetNCVCMainWnd()->GetPenCom(COMPEN_SEL)
	};
	return pDrawPen[ m_dwFlags & DXFFLG_SELECT ? 1 : 0 ];
}

void CDXFdata::SwapMakePt(int n)	// m_ptTun �̓���ւ�
{
	if ( m_nPoint > n+1 ) {	// �O�̂�������
#ifdef _DEBUGOLD
		printf("SwapMakePt() calling ok\n");
#endif
		swap(m_ptTun[n],  m_ptTun[n+1]);
		swap(m_ptMake[n], m_ptMake[n+1]);
	}
#ifdef _DEBUGOLD
	else
		printf("SwapMakePt() Missing call! m_nPoint=%d n=%d\n", m_nPoint, n);
#endif
}

void CDXFdata::SwapNativePt(void)	// �ŗL���W�l�̓���ւ�
{
	if ( m_nPoint > 1 ) {
#ifdef _DEBUGOLD
		printf("SwapNativePoint() calling ok\n");
#endif
		swap(m_pt[0], m_pt[1]);
	}
#ifdef _DEBUGOLD
	else
		printf("SwapNativePoint() Missing call! m_nPoint=%d\n", m_nPoint);
#endif
}

void CDXFdata::RoundObjPoint(const CPointF& ptOrg, float dRound)
{
	for ( int i=0; i<m_nPoint; i++ ) {
		m_pt[i] -= ptOrg;
		m_pt[i].RoundPoint(dRound);
		m_pt[i] += ptOrg;
	}
	SetMaxRect();
}
	
BOOL CDXFdata::IsDirectionPoint(const CPointF&, const CPointF& pte)
{
	// �I�_�Ɩ��_(pte)���������Ȃ���� TRUE(Reverse) ��Ԃ�
	return GetEndCutterPoint() != pte;
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
	m_dwFlags &= ~DXFFLG_CLRWORK;	// �����C�����C�G�b�W�t���O�̃N���A
	for ( int i=0; i<m_nPoint; i++ )
		m_ptMake[i] = m_ptTun[i].RoundUp();
	if ( ms_fXRev ) XRev();		// �������](virtual)
	if ( ms_fYRev ) YRev();
}

//////////////////////////////////////////////////////////////////////
// �c�w�e�f�[�^��Point�N���X
//////////////////////////////////////////////////////////////////////

CDXFpoint::CDXFpoint() : CDXFdata(DXFPOINTDATA, NULL, 1, 0)
{
}

CDXFpoint::CDXFpoint(ENDXFTYPE enType, CLayerData* pLayer, int nPoint, DWORD dwFlags) :
	CDXFdata(enType, pLayer, nPoint, dwFlags)
{
}

CDXFpoint::CDXFpoint(LPCDXFPARGV lpPoint, DWORD dwFlags) :
	CDXFdata(DXFPOINTDATA, lpPoint->pLayer, 1, dwFlags)
{
	m_pt[0] = lpPoint->c;
	SetMaxRect();
}

CDXFpoint::CDXFpoint(CLayerData* pLayer, const CDXFpoint* pData, LPCDXFBLOCK lpBlock, DWORD dwFlags) :
	CDXFdata(DXFPOINTDATA, pLayer, 1, dwFlags)
{
	m_pt[0] = pData->GetNativePoint(0);
	if ( lpBlock->dwBlockFlg & DXFBLFLG_X )
		m_pt[0].x *= lpBlock->dMagni[NCA_X];
	if ( lpBlock->dwBlockFlg & DXFBLFLG_Y )
		m_pt[0].y *= lpBlock->dMagni[NCA_Y];
	if ( lpBlock->dwBlockFlg & DXFBLFLG_R )
		m_pt[0].RoundPoint(RAD(lpBlock->dRound));
	m_pt[0] += lpBlock->ptOrg;
	SetMaxRect();
}

#ifdef _DEBUG
void CDXFpoint::DbgDump(void)
{
	printf("[POINT]\n");
	CDXFdata::DbgDump();
}
#endif

void CDXFpoint::Serialize(CArchive& ar)
{
	CDXFdata::Serialize(ar);
	if ( ar.IsStoring() )
		ar << m_pt[0].x << m_pt[0].y;
	else {
		if ( g_dwCamVer < NCVCSERIALVERSION_3620 ) {
			CPointD	pt;
			ar >> pt.x >> pt.y;
			m_pt[0] = pt;
		}
		else {
			ar >> m_pt[0].x >> m_pt[0].y;
		}
		SetMaxRect();
	}
}

void CDXFpoint::SetMaxRect(void)
{
	m_rcMax.TopLeft()     = m_pt[0];
	m_rcMax.BottomRight() = m_pt[0];
	m_rcMax.NormalizeRect();
#ifdef _DEBUG_MAXRECT
	printf("CDXFpoint::SetMaxRect() l=%.3f t=%.3f\n", m_rcMax.left, m_rcMax.top);
#endif
}

void CDXFpoint::DrawTuning(float f)
{
	CPointF	pt(m_pt[0] * f);
	m_ptDraw = pt;
	// �ʒu��\���ۈ�͏��2.5�_������
	float	dFactor = LOMETRICFACTOR * 2.5f;
	m_rcDraw.TopLeft()		= pt - dFactor;
	m_rcDraw.BottomRight()	= pt + dFactor;
}

void CDXFpoint::Draw(CDC* pDC) const
{
#ifdef _DEBUGDRAW_DXF
	printf("CDXFpoint::Draw() (%d, %d)-(%d, %d)\n", m_rcDraw.left, m_rcDraw.top, m_rcDraw.right, m_rcDraw.bottom);
#endif
	CPoint	pt(m_rcDraw.CenterPoint());
	pDC->MoveTo(m_rcDraw.right-1, pt.y);
	pDC->LineTo(m_rcDraw.left,  pt.y);
	pDC->LineTo(pt.x, m_rcDraw.top+1);	// top������
	pDC->LineTo(pt.x, m_rcDraw.bottom);
	pDC->LineTo(m_rcDraw.right-1, pt.y);
	pDC->Ellipse(&m_rcDraw);
}

float CDXFpoint::OrgTuning(BOOL bCalc/*=TRUE*/)
{
	ASSERT(ms_pData);
	m_ptTun[0] = m_pt[0] - ms_ptOrg;
	OrgTuningBase();
#ifdef _DEBUG
	printf("OrgTuning Point cx=%f cy=%f\n", m_ptTun[0].x, m_ptTun[0].y);
#endif
	return bCalc ? (*ms_pfnOrgDrillTuning)(this) : 0.0f;	// ����ɂ��킹���ߐڍ��W : dummy
}

float CDXFpoint::OrgTuning_Seq(const CDXFpoint* pData)
{
	// �w���޼ު�Ă܂ł̋����v�Z
	return GAPCALC(pData->m_ptTun[0] - ms_pData->GetEndCutterPoint());
}

float CDXFpoint::OrgTuning_XY(const CDXFpoint*)
{
	return 0.0f;		// ������Ƃɕ��בւ�����̂���а��Ԃ�
}

float CDXFpoint::GetSelectPointGap(const CPointF&)
{
	return HUGE_VALF;
}

BOOL CDXFpoint::GetDirectionArraw(const CPointF&, CPointF[][3])
{
	return FALSE;
}

int CDXFpoint::GetIntersectionPoint(const CDXFdata*, CPointF[], BOOL) const
{
	return 0;
}

int CDXFpoint::GetIntersectionPoint(const CPointF&, const CPointF&, CPointF[], BOOL) const
{
	return 0;
}

optional<CPointF>
CDXFpoint::CalcOffsetIntersectionPoint(const CDXFdata*, float, BOOL) const
{
	return optional<CPointF>();
}

int CDXFpoint::CheckIntersectionCircle(const CPointF&, float) const
{
	return 0;
}

size_t CDXFpoint::SetVectorPoint(CVPointF& vpt, float) const
{
	vpt.push_back(GetNativePoint(0));
	return 1;
}

void CDXFpoint::SetVectorPoint(CVPointF& vpt, size_t) const
{
}

void CDXFpoint::SetWireHeteroData(const CDXFdata*, CVPointF&, CVPointF&, float) const
{
}

//////////////////////////////////////////////////////////////////////
// �c�w�e�f�[�^��Line�N���X
//////////////////////////////////////////////////////////////////////

CDXFline::CDXFline() : CDXFpoint(DXFLINEDATA, NULL, 2, 0)
{
}

CDXFline::CDXFline(ENDXFTYPE enType, CLayerData* pLayer, int nPoint, DWORD dwFlags) :
	CDXFpoint(enType, pLayer, nPoint, dwFlags)
{
}

CDXFline::CDXFline(LPCDXFLARGV lpLine, DWORD dwFlags) :
	CDXFpoint(DXFLINEDATA, lpLine->pLayer, 2, dwFlags)
{
	m_pt[0] = lpLine->s;
	m_pt[1] = lpLine->e;
	SetMaxRect();
}

CDXFline::CDXFline(CLayerData* pLayer, const CDXFline* pData, LPCDXFBLOCK lpBlock, DWORD dwFlags) :
	CDXFpoint(DXFLINEDATA, pLayer, 2, dwFlags)
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
		float	dRound = RAD(lpBlock->dRound);
		m_pt[0].RoundPoint(dRound);
		m_pt[1].RoundPoint(dRound);
	}
	m_pt[0] += lpBlock->ptOrg;
	m_pt[1] += lpBlock->ptOrg;
	SetMaxRect();
}

#ifdef _DEBUG
void CDXFline::DbgDump(void)
{
	printf("[LINE]\n");
	CDXFdata::DbgDump();
}
#endif

void CDXFline::Serialize(CArchive& ar)
{
	CDXFdata::Serialize(ar);
	if ( ar.IsStoring() )
		ar << m_pt[0].x << m_pt[0].y << m_pt[1].x << m_pt[1].y;
	else {
		if ( g_dwCamVer < NCVCSERIALVERSION_3620 ) {
			CPointD	pts, pte;
			ar >> pts.x >> pts.y >> pte.x >> pte.y;
			m_pt[0] = pts;
			m_pt[1] = pte;
		}
		else {
			ar >> m_pt[0].x >> m_pt[0].y >> m_pt[1].x >> m_pt[1].y;
		}
		SetMaxRect();
	}
}

void CDXFline::SetMaxRect(void)
{
	m_rcMax.TopLeft()     = m_pt[0];
	m_rcMax.BottomRight() = m_pt[1];
	m_rcMax.NormalizeRect();
#ifdef _DEBUG_MAXRECT
	printf("CDXFline::SetMaxRect() l=%.3f t=%.3f r=%.3f b=%.3f\n",
		m_rcMax.left, m_rcMax.top, m_rcMax.right, m_rcMax.bottom);
#endif
}

void CDXFline::DrawTuning(float f)
{
	m_ptDrawS = m_pt[0] * f;
	m_ptDrawE = m_pt[1] * f;
}

void CDXFline::Draw(CDC* pDC) const
{
#ifdef _DEBUGDRAW_DXF
	printf("CDXFline::Draw() pts.x=%d pts.y=%d pte.x=%d pte.y=%d\n",
		m_ptDrawS.x, m_ptDrawS.y, m_ptDrawE.x, m_ptDrawE.y);
#endif
	pDC->MoveTo(m_ptDrawS);
	pDC->LineTo(m_ptDrawE);
#ifdef _DEBUGDRAW_DXF_EDGE_
	CRect	rc(m_ptDrawE.x-10, m_ptDrawE.y-10, m_ptDrawE.x+10, m_ptDrawE.y+10);
	pDC->Ellipse(&rc);
#endif
}

float CDXFline::OrgTuning(BOOL bCalc/*=TRUE*/)
{
	ASSERT(ms_pData);
	m_ptTun[0] = m_pt[0] - ms_ptOrg;
	m_ptTun[1] = m_pt[1] - ms_ptOrg;
	OrgTuningBase();
#ifdef _DEBUG
	printf("OrgTuning Line sx=%f sy=%f ex=%f ey=%f\n",
					m_ptTun[0].x, m_ptTun[0].y, m_ptTun[1].x, m_ptTun[1].y);
#endif
	return bCalc ? GetEdgeGap(ms_pData->GetEndCutterPoint()) : 0.0f;
}

float CDXFline::GetSelectPointGap(const CPointF& pt)
{
	float	dResult = HUGE_VALF;

	// ���������̏ꍇ�CPtInRect() �m�f
	if ( fabs(m_pt[0].x-m_pt[1].x) < NCMIN ) {
		float	minY, maxY;
		tie(minY, maxY) = minmax(m_pt[0].y, m_pt[1].y);
		if ( minY<=pt.y && pt.y<=maxY )
			dResult = fabs(pt.x - m_pt[0].x);
	}
	else if ( fabs(m_pt[0].y-m_pt[1].y) < NCMIN ) {
		float	minX, maxX;
		tie(minX, maxX) = minmax(m_pt[0].x, m_pt[1].x);
		if ( minX<=pt.x && pt.x<=maxX )
			dResult = fabs(pt.y - m_pt[0].y);
	}
	else if ( m_rcMax.PtInRect(pt) ) {
		// �͈͓��̏ꍇ
		CPointF	pt1 = pt      - m_pt[0],	// �n�_�����_��
				pt2 = m_pt[1] - m_pt[0];
		float	l  = pt1.hypot(),
				qp = pt1.arctan(),
				qs = pt2.arctan();
		dResult = l * fabs( sin(qp - qs) );
	}

	if ( dResult == HUGE_VALF ) {
		// �͈͊O�̏ꍇ�C�[�_����̋����v�Z
		float	d1 = GAPCALC(m_pt[0]-pt), d2 = GAPCALC(m_pt[1]-pt);
		dResult = sqrt( min(d1, d2) );	// �߂����̋�����Ԃ�
	}

	return dResult;
}

BOOL CDXFline::GetDirectionArraw(const CPointF&, CPointF pt[][3])
{
	float	lqs = m_pt[0].arctan(m_pt[1]),
			lqe = lqs + PI;		// + RAD(180.0)
	float	lq[][2] = { {lqs + ARRAWANGLE, lqs - ARRAWANGLE},
						{lqe + ARRAWANGLE, lqe - ARRAWANGLE} };
	for ( int i=0; i<m_nPoint; i++ ) {
		pt[i][0].x = ARRAWLENGTH * cos(lq[i][0]);	// + pt[i] �͊g�嗦���f��
		pt[i][0].y = ARRAWLENGTH * sin(lq[i][0]);
		pt[i][1]   = m_pt[i];
		pt[i][2].x = ARRAWLENGTH * cos(lq[i][1]);
		pt[i][2].y = ARRAWLENGTH * sin(lq[i][1]);
	}
	return TRUE;
}

int CDXFline::GetIntersectionPoint(const CDXFdata* pData, CPointF pt[], BOOL bEdge/*=TRUE*/) const
{
	int		nResult = 0;
	CPointF	pt1(pData->GetNativePoint(0)), pt2(pData->GetNativePoint(1));
	const CDXFcircle*	pCircle;
	const CDXFellipse*	pEllipse;

	// bEdge==TRUE : �[�_�������ꍇ�͌�_�Ȃ��Ƃ���
	switch ( pData->GetType() ) {
	case DXFLINEDATA:
		return GetIntersectionPoint(pt1, pt2, pt, bEdge);
	case DXFARCDATA:
		if ( bEdge && (IsMatchPoint(pt1) || IsMatchPoint(pt2)) )
			break;
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
						swap(pt1, pt2);
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
		if ( bEdge && pEllipse->IsArc() && (IsMatchPoint(pt1) || IsMatchPoint(pt2)) )
			break;
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
						swap(pt1, pt2);
				}
			}
		}
		if ( nResult > 1 )
			pt[1] = pt2;
		if ( nResult > 0 )
			pt[0] = pt1;
		break;
	case DXFPOLYDATA:
		nResult = pData->GetIntersectionPoint(this, pt, bEdge);
		break;
	}

	return nResult;
}

int CDXFline::GetIntersectionPoint(const CPointF& pts, const CPointF& pte, CPointF pt[], BOOL bEdge/*=TRUE*/) const
{
	if ( bEdge && (IsMatchPoint(pts) || IsMatchPoint(pte)) )
		return 0;

	int		nResult = 0;
	CPointF	pto(m_pt[1]-m_pt[0]), ptc;
	float	q = -pto.arctan();
	pto.RoundPoint(q);
	optional<CPointF>	ptResult;

	// ��������
	ptc = pts - m_pt[0];
	ptc.RoundPoint(q);
	if ( 0<=ptc.x+NCMIN && ptc.x-NCMIN<=pto.x && fabs(ptc.y)<=NCMIN ) {
		// ���͐���ɂ���
		pt[0] = pts;
		return 1;
	}
	ptc = pte - m_pt[0];
	ptc.RoundPoint(q);
	if ( 0<=ptc.x+NCMIN && ptc.x-NCMIN<=pto.x && fabs(ptc.y)<=NCMIN ) {
		pt[0] = pte;
		return 1;
	}
	// ��_�v�Z
	ptResult = ::CalcIntersectionPoint_LL(m_pt[0], m_pt[1], pts, pte);
	if ( ptResult ) {
		pt[0] = *ptResult;
		nResult = 1;
	}

	return nResult;
}

optional<CPointF>
CDXFline::CalcOffsetIntersectionPoint
	(const CDXFdata* pNext, float r, BOOL bLeft) const
{
	CPointF	pto( GetNativePoint(1) ), pts( GetNativePoint(0) ),
			pt1(pts - pto), pt2, pt;
	CDXFdata*	pData;
	const CDXFarc*		pArc;
	const CDXFellipse*	pEllipse;
	const CDXFpolyline*	pPolyline;
	optional<CPointF>	ptResult;

	switch ( pNext->GetType() ) {
	case DXFLINEDATA:
		pt2 = pNext->GetNativePoint(1) - pto;
		ptResult = ::CalcOffsetIntersectionPoint_LL(pt1, pt2, r, r, bLeft);
		break;
	case DXFARCDATA:
		pArc = static_cast<const CDXFarc*>(pNext);
		pt2 = pArc->GetCenter() - pto;
		ptResult = ::CalcOffsetIntersectionPoint_LC(pt1, pt2,
				pArc->GetR(), r, r, pArc->GetRoundOrig(), bLeft);
		break;
	case DXFELLIPSEDATA:
		pEllipse = static_cast<const CDXFellipse*>(pNext);
		pt2 = pEllipse->GetCenter() - pto;
		ptResult = ::CalcOffsetIntersectionPoint_LE(pt1, pt2,
			pEllipse->GetLongLength(), pEllipse->GetShortLength(), pEllipse->GetLean(), r,
			pEllipse->GetRoundOrig(), bLeft);
		break;
	case DXFPOLYDATA:
		pPolyline = static_cast<const CDXFpolyline*>(pNext);
		pData = pPolyline->GetFirstObject();
		if ( pData ) {
			ptResult = this->CalcOffsetIntersectionPoint(pData, r, bLeft);
			if ( pData->GetType() == DXFLINEDATA )
				delete	pData;
			return ptResult;
		}
		break;
	}

	if ( ptResult ) {
		pt = *ptResult + pto;
		return pt;
	}
	return ptResult;
}

int CDXFline::CheckIntersectionCircle(const CPointF& ptc, float r) const
{
	int	nResult;
	CPointF	pt1, pt2;
	tie(nResult, pt1, pt2) = ::CalcIntersectionPoint_LC(m_pt[0], m_pt[1], ptc, r);

	switch ( nResult ) {
	case 0:
		// ��_�������Ă��[�_���~�̓����Ȃ�NG
		if ( ::RoundUp(sqrt(GAPCALC(m_pt[0]-ptc)))<r || ::RoundUp(sqrt(GAPCALC(m_pt[1]-ptc)))<r )
			nResult = 2;
		break;
	case 1:
		// �u�ڂ���v�ꍇ����������
		if ( pt1 != pt2 )
			nResult = 2;	// ��_����ɉ���ύX
		break;
	}

	return nResult;
}

size_t CDXFline::SetVectorPoint(CVPointF& vpt, float k) const
{
	if ( k == 0.0f )
		return CDXFpoint::SetVectorPoint(vpt);

	vpt.push_back(GetEndMakePoint());
	return 1;
}

void CDXFline::SetVectorPoint(CVPointF& vpt, size_t n) const
{
	CPointF	pts(GetStartMakePoint()), pte(GetEndMakePoint()),
			pt;
	float	q = pts.arctan(pte),
			s = GetLength() / n,	// ���݉񐔂��獏�ݕ������߂�
			r = s;
	for ( size_t i=0; i<n-1; i++, r+=s ) {	// �I�_��
		pt.x = r * cos(q) + pts.x;
		pt.y = r * sin(q) + pts.y;
		vpt.push_back(pt);
	}
	vpt.push_back(pte);
}

void CDXFline::SetWireHeteroData(const CDXFdata* pDataUV, CVPointF& vptXY, CVPointF& vptUV, float k) const
{
	if ( pDataUV->GetMakeType() == DXFARCDATA ) {
		// �~�ʂ����ݕ��ō��W�o�^��
		pDataUV->SetVectorPoint(vptUV, k);
		// ���̍��ݐ��Ő������W�o�^
		SetVectorPoint(vptXY, vptUV.size());
	}
}

//////////////////////////////////////////////////////////////////////
// �c�w�e�f�[�^��Circle�N���X
//////////////////////////////////////////////////////////////////////

CDXFcircle::CDXFcircle() : CDXFline(DXFCIRCLEDATA, NULL, 4, 0)
{
	m_nArrayExt = 0;
	m_bRound = FALSE;	// �~�ް��̉�]�����͕����w���̂�
	m_bRoundFixed = FALSE;
}

CDXFcircle::CDXFcircle(ENDXFTYPE enType, CLayerData* pLayer,
	const CPointF& c, float r, BOOL bRound, int nPoint, DWORD dwFlags) :
		CDXFline(enType, pLayer, nPoint, dwFlags)
{
	m_nArrayExt = 0;
	m_ct	= c;
	m_r		= r;
	m_rMake	= fabs(::RoundUp(m_r));
	m_bRound = bRound;
	m_bRoundFixed = FALSE;
}

CDXFcircle::CDXFcircle(LPCDXFCARGV lpCircle, DWORD dwFlags) :
	CDXFline(DXFCIRCLEDATA, lpCircle->pLayer, 4, dwFlags)
{
	m_nArrayExt = 0;
	m_ct	= lpCircle->c;
	m_r		= lpCircle->r;
	m_rMake	= fabs(::RoundUp(m_r));
	m_bRound = FALSE;
	m_bRoundFixed = FALSE;
	SetCirclePoint();
	SetMaxRect();
}

CDXFcircle::CDXFcircle(CLayerData* pLayer, const CDXFcircle* pData, LPCDXFBLOCK lpBlock, DWORD dwFlags) :
	CDXFline(DXFCIRCLEDATA, pLayer, 4, dwFlags)
{
	m_nArrayExt = 0;
	m_ct	= pData->GetCenter();
	m_r		= pData->GetR();
	// �~�̊g�嗦��X�������� ->
	// �@�e���Ŋg��k������Ƒȉ~�ɂȂ邪�C��ۯ�����̐�������(ReadDXF.cpp)�Œ���
	if ( lpBlock->dwBlockFlg & DXFBLFLG_X ) {
		m_ct *= lpBlock->dMagni[NCA_X];
		m_r  *= lpBlock->dMagni[NCA_X];
	}
	if ( lpBlock->dwBlockFlg & DXFBLFLG_R )
		m_ct.RoundPoint(RAD(lpBlock->dRound));
	m_ct += lpBlock->ptOrg;
	m_rMake = pData->GetMakeR();
	m_bRound = pData->GetRound();
	m_bRoundFixed = pData->IsRoundFixed();
	SetCirclePoint();
	SetMaxRect();
}

#ifdef _DEBUG
void CDXFcircle::DbgDump(void)
{
	printf("[CIRCLE]\n");
	CDXFdata::DbgDump();
	printf("r=%.3f cx=%.3f cy=%.3f %s\n", m_r, m_ct.x, m_ct.y,
		m_bRound ? "CCW" : "CW");
}
#endif

void CDXFcircle::Serialize(CArchive& ar)
{
	CDXFdata::Serialize(ar);
	if ( ar.IsStoring() )
		ar << m_r << m_ct.x << m_ct.y;
	else {
		if ( g_dwCamVer < NCVCSERIALVERSION_3620 ) {
			double	r;
			CPointD ct;
			ar >> r >> ct.x >> ct.y;
			m_r  = (float)r;
			m_ct = ct;
		}
		else {
			ar >> m_r >> m_ct.x >> m_ct.y;
		}
		m_rMake	= ::RoundUp(m_r);
		SetCirclePoint();
		SetMaxRect();
	}
}

void CDXFcircle::SwapMakePt(int n)
{
	if ( m_nArrayExt != n )
		m_nArrayExt = n;
	// ���W�̓���ւ��͕K�v�Ȃ�
	// ���Ă䂤�� m_nArrayExt �ŊJ�n�_���R���g���[�����Ă���̂�
	// ���W����ւ�������_���I
//	CDXFdata::SwapMakePt( n & 0xfe );	// 0 or 2 (����1�ޯ�Ͻ�)
}

void CDXFcircle::SwapNativePt(void)
{
	// �������Ȃ�
}

void CDXFcircle::RoundObjPoint(const CPointF& ptOrg, float dRound)
{
	m_ct -= ptOrg;
	m_ct.RoundPoint(dRound);
	m_ct += ptOrg;
	SetCirclePoint();
	SetMaxRect();
}

void CDXFcircle::SetCirclePoint(void)
{
	m_pt[0].x = m_ct.x + m_r;	// 0��
	m_pt[0].y = m_ct.y;
	m_pt[1].x = m_ct.x;			// 90��
	m_pt[1].y = m_ct.y + m_r;
	m_pt[2].x = m_ct.x - m_r;	// 180��
	m_pt[2].y = m_ct.y;
	m_pt[3].x = m_ct.x;			// 270��
	m_pt[3].y = m_ct.y - m_r;
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

void CDXFcircle::SetMaxRect(void)
{
	m_rcMax.TopLeft()     = m_ct - m_r;
	m_rcMax.BottomRight() = m_ct + m_r;
	m_rcMax.NormalizeRect();
#ifdef _DEBUG_MAXRECT
	printf("CDXFcircle::SetMaxRect() l=%.3f t=%.3f r=%.3f b=%.3f\n",
		m_rcMax.left, m_rcMax.top, m_rcMax.right, m_rcMax.bottom);
#endif
}

BOOL CDXFcircle::IsRangeAngle(const CPointF&) const
{
	return TRUE;
}

void CDXFcircle::DrawTuning(const float f)
{
	m_rcDraw.TopLeft()		= (m_ct - m_r) * f;
	m_rcDraw.BottomRight()	= (m_ct + m_r) * f;
}

void CDXFcircle::Draw(CDC* pDC) const
{
#ifdef _DEBUGDRAW_DXF
	printf("CDXFcircle::Draw() (%d, %d)-(%d, %d)\n", m_rcDraw.left, m_rcDraw.top, m_rcDraw.right, m_rcDraw.bottom);
#endif
	pDC->Ellipse(m_rcDraw);
}

float CDXFcircle::OrgTuning(BOOL bCalc/*=TRUE*/)
{
	ASSERT(ms_pData);
	if ( GetMakeType() == DXFPOINTDATA ) {
		m_ptTun[0] = m_ct - ms_ptOrg;
		for ( int i=1; i<m_nPoint; i++ )
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
	printf("OrgTuning Circle s1=(%f, %f) s2=(%f, %f)\n", 
					m_ptTun[0].x, m_ptTun[0].y, m_ptTun[1].x, m_ptTun[1].y);
	printf("                 s3=(%f, %f) s4=(%f, %f)\n", 
					m_ptTun[2].x, m_ptTun[2].y, m_ptTun[3].x, m_ptTun[3].y);
#endif
	if ( GetMakeType() == DXFPOINTDATA )
		return bCalc ? (*ms_pfnOrgDrillTuning)(this) : 0.0f;	// CDXFpoint::OrgTuning()
	else
		return bCalc ? GetEdgeGap(ms_pData->GetEndCutterPoint()) : 0.0f;
}

float CDXFcircle::GetSelectPointGap(const CPointF& pt)
{
	return GetSelectPointGap_Circle(pt, 0.0f, PI2);
}

BOOL CDXFcircle::GetDirectionArraw_Circle
	(const float q[], const CPointF pt[], CPointF ptResult[][3])
{
	float	lq[][2] = { {q[0] + ARRAWANGLE, q[0] - ARRAWANGLE},
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

BOOL CDXFcircle::GetDirectionArraw(const CPointF& ptClick, CPointF ptResult[][3])
{
	CPointF	pt[2];
	// 1/4�n�_�I�_���擾
	GetQuarterPoint(ptClick, pt);
	// �ڐ��̌X�������߂� -> �_pt�ɒ���������̌X��
	float	q[] = {
		-atan2(pt[0].x - m_ct.x, pt[0].y - m_ct.y)+PI,		// X��Y���t�H
		-atan2(pt[1].x - m_ct.x, pt[1].y - m_ct.y)
	};
	// �ڐ���������W�̌v�Z
	return GetDirectionArraw_Circle(q, pt, ptResult);
}

BOOL CDXFcircle::IsDirectionPoint(const CPointF& pts, const CPointF& pte)
{
	const int	nLoop = m_nPoint - 1;
	int			i;

	// ��]�����̔���
	for ( i=0; i<nLoop; i++ ) {
		if ( m_ptTun[i] == pts ) {
			m_bRound = m_ptTun[i+1] == pte ? TRUE : FALSE;
			return FALSE;
		}
	}
	if ( m_ptTun[i] == pts )	// 270������
		m_bRound = m_ptTun[i-1] == pte ? FALSE : TRUE;

	// ��]�����̌Œ�
	m_bRoundFixed = TRUE;

	return FALSE;
}

int CDXFcircle::GetIntersectionPoint(const CDXFdata* pData, CPointF pt[], BOOL) const
{
	int		nResult = 0;
	CPointF	pt1, pt2;
	const CDXFcircle*	pCircle;

	switch ( pData->GetType() ) {
	case DXFLINEDATA:
		return GetIntersectionPoint(pData->GetNativePoint(0), pData->GetNativePoint(1), pt);
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
						swap(pt1, pt2);
				}
			}
		}
		break;
	case DXFELLIPSEDATA:
		// ���l�v�Z������A�ĺ��ިݸ�
		pt[0] = pt[1] = 0;
		return 2;	// �Ƃ肠������_�����邱�Ƃɂ��Ă���
	case DXFPOLYDATA:
		nResult = pData->GetIntersectionPoint(this, pt);
		break;
	}

	if ( nResult > 1 )
		pt[1] = pt2;
	if ( nResult > 0 )
		pt[0] = pt1;

	return nResult;
}

int CDXFcircle::GetIntersectionPoint(const CPointF& pts, const CPointF& pte, CPointF pt[], BOOL) const
{
	int		nResult;
	CPointF	pt1, pt2;

	tie(nResult, pt1, pt2) = ::CalcIntersectionPoint_LC(pts, pte, m_ct, m_r);
	if ( nResult > 1 )
		pt[1] = pt2;
	if ( nResult > 0 )
		pt[0] = pt1;

	return nResult;
}

optional<CPointF>
CDXFcircle::CalcOffsetIntersectionPoint(const CDXFdata*, float, BOOL) const
{
	return optional<CPointF>();
}

int CDXFcircle::CheckIntersectionCircle(const CPointF&, float) const
{
	return 0;
}

size_t CDXFcircle::SetVectorPoint(CVPointF& vpt, float k) const
{
	float	sq, eq;
	switch (m_nArrayExt) {
	case 1:		sq = RAD(180.0f);	break;
	case 2:		sq = RAD(90.0f);	break;
	case 3:		sq = RAD(270.0f);	break;
	default:	sq = 0;
	}
	if ( m_bRound )
		eq = sq + PI2;	// RAD(360.0)
	else
		eq = sq - PI2;

	return SetVectorPointSub(m_bRound, sq, eq, k/m_r, m_ctTun, vpt);
}

void CDXFcircle::SetVectorPoint(CVPointF& vpt, size_t n) const
{
	float	sq, eq;
	switch (m_nArrayExt) {
	case 1:		sq = RAD(180.0f);	break;
	case 2:		sq = RAD(90.0f);	break;
	case 3:		sq = RAD(270.0f);	break;
	default:	sq = 0;
	}
	if ( m_bRound )
		eq = sq + PI2;
	else
		eq = sq - PI2;

	size_t m = SetVectorPointSub(m_bRound, sq, eq, PI2/n, m_ctTun, vpt);
	while ( n < m-- )
		vpt.pop_back();
}

size_t CDXFcircle::SetVectorPointSub(BOOL bRound, float s, float e, float k, const CPointF& ptOrg, CVPointF& vpt) const
{
	size_t	nCnt = 0;
	CPointF	pt;

	// ��]�����ɂČ����ɉ�
	if ( bRound ) {
		for ( s+=k; s<e; s+=k, nCnt++ ) {
			pt.SetPoint(m_r * cos(s) + ptOrg.x, m_r * sin(s) + ptOrg.y);
			vpt.push_back(pt);
		}
	}
	else {
		for ( s-=k; s>e; s-=k, nCnt++ ) {
			pt.SetPoint(m_r * cos(s) + ptOrg.x, m_r * sin(s) + ptOrg.y);
			vpt.push_back(pt);
		}
	}
	pt.SetPoint(m_r * cos(e) + ptOrg.x, m_r * sin(e) + ptOrg.y);
	vpt.push_back(pt);

	return nCnt+1;
}

void CDXFcircle::SetWireHeteroData(const CDXFdata*, CVPointF&, CVPointF&, float) const
{
}

//////////////////////////////////////////////////////////////////////
// �c�w�e�f�[�^��CircleEx�N���X
//////////////////////////////////////////////////////////////////////

CDXFcircleEx::CDXFcircleEx() : CDXFcircle(DXFCIRCLEDATA, NULL, CPointF(), 0.0f, FALSE, 0, 0)
{
}

CDXFcircleEx::CDXFcircleEx(ENDXFTYPE2 enType2, CLayerData* pLayer, const CPointF& pt, float r) :
	CDXFcircle(DXFCIRCLEDATA, pLayer, pt, r, FALSE, 0, 0)
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
		if ( g_dwCamVer < NCVCSERIALVERSION_3620 ) {
			double	r;
			CPointD ct;
			ar >> r >> ct.x >> ct.y;
			m_r  = (float)r;
			m_ct = ct;
		}
		else {
			ar >> m_r >> m_ct.x >> m_ct.y;
		}
		if ( nType < 0 || nType > 1 )	// DXFORGDATA �` DXFSTADATA
			AfxThrowUserException();
		m_rMake	= ::RoundUp(m_r);
		m_enType2 = (ENDXFTYPE2)nType;
		SetMaxRect();
	}
}

void CDXFcircleEx::SetCirclePoint(void)
{
	// �������Ȃ�
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
	printf("CDXFcircleEx::Draw() (%d, %d)-(%d, %d)\n", m_rcDraw.left, m_rcDraw.top, m_rcDraw.right, m_rcDraw.bottom);
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

float CDXFcircleEx::OrgTuning(BOOL/*=TRUE*/)
{
	m_ctTun = m_ct - ms_ptOrg;
	m_ctMake = m_ctTun.RoundUp();
	OrgTuningBase();
	return 0.0f;	// dummy
}

//////////////////////////////////////////////////////////////////////
// �c�w�e�f�[�^��Arc�N���X
//////////////////////////////////////////////////////////////////////

CDXFarc::CDXFarc() : CDXFcircle(DXFARCDATA, NULL, CPointF(), 0.0f, TRUE, 2, 0)
{
}

CDXFarc::CDXFarc(ENDXFTYPE enType, CLayerData* pLayer,
	const CPointF& c, float r, float sq, float eq, BOOL bRound, int nPoint, DWORD dwFlags) :
		CDXFcircle(enType, pLayer, c, r, bRound, nPoint, dwFlags)
{
	m_bRoundOrig = m_bRound;
	m_sq = sq;
	m_eq = eq;
	AngleTuning();
	m_sqDraw = m_sq;
	m_eqDraw = m_eq;
}

CDXFarc::CDXFarc(LPCDXFAARGV lpArc, DWORD dwFlags) :
	CDXFcircle(DXFARCDATA, lpArc->pLayer, lpArc->c, lpArc->r, TRUE, 2, dwFlags)
{
	m_bRoundOrig = m_bRound;
	m_sq = RAD(lpArc->sq);
	m_eq = RAD(lpArc->eq);
	AngleTuning();
	m_sqDraw = m_sq;
	m_eqDraw = m_eq;
	m_pt[0].SetPoint(m_r * cos(m_sq), m_r * sin(m_sq));
	m_pt[1].SetPoint(m_r * cos(m_eq), m_r * sin(m_eq));
	m_pt[0] += m_ct;
	m_pt[1] += m_ct;
#ifdef _DEBUG
	printf("CDXFarc::CDXFarc() sx=%f sy=%f\n", m_pt[0].x, m_pt[0].y);
	printf("                   ex=%f ey=%f\n", m_pt[1].x, m_pt[1].y);
#endif
	SetMaxRect();
	SetRsign();
}

CDXFarc::CDXFarc(LPCDXFAARGV lpArc, BOOL bRound, const CPointF& pts, const CPointF& pte, DWORD dwFlags) :
	CDXFcircle(DXFARCDATA, lpArc->pLayer, lpArc->c, lpArc->r, bRound, 2, dwFlags)
{
	m_bRoundOrig = m_bRound;
	m_sqDraw = m_sq = lpArc->sq;		// ����׼ޱݒP�ʁC�����ς�
	m_eqDraw = m_eq = lpArc->eq;
	m_pt[0]  = pts;
	m_pt[1]  = pte;
	SetMaxRect();
	SetRsign();
}

CDXFarc::CDXFarc(CLayerData* pLayer, const CDXFarc* pData, LPCDXFBLOCK lpBlock, DWORD dwFlags) :
	CDXFcircle(DXFARCDATA, pLayer, pData->GetCenter(), pData->GetR(), pData->GetRoundOrig(), 2, dwFlags)
{
	m_bRoundOrig = m_bRound;
	m_sq	= pData->GetStartAngle();
	m_eq	= pData->GetEndAngle();
	m_pt[0]	= pData->GetNativePoint(0);
	m_pt[1]	= pData->GetNativePoint(1);
	// �~�ʂ̊g�嗦��X�������� ->
	// �@�e���Ŋg��k������Ƒȉ~�ʂɂȂ邪�C��ۯ�����̐�������(ReadDXF.cpp)�Œ���
	if ( lpBlock->dwBlockFlg & DXFBLFLG_X ) {
		m_ct	*= lpBlock->dMagni[NCA_X];
		m_pt[0]	*= lpBlock->dMagni[NCA_X];
		m_pt[1]	*= lpBlock->dMagni[NCA_X];
		m_r		*= lpBlock->dMagni[NCA_X];
		m_rMake = ::RoundUp(m_r);
	}
	if ( lpBlock->dwBlockFlg & DXFBLFLG_R ) {
		float	dRound = RAD(lpBlock->dRound);
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

#ifdef _DEBUG
void CDXFarc::DbgDump(void)
{
	printf("[ARC]\n");
	CDXFdata::DbgDump();
	printf("r=%.3f cx=%.3f cy=%.3f %s\n", m_r, m_ct.x, m_ct.y,
		m_bRound ? "CCW" : "CW");
}
#endif

void CDXFarc::Serialize(CArchive& ar)
{
	CDXFdata::Serialize(ar);
	if ( ar.IsStoring() )
		ar << m_r << m_ct.x << m_ct.y << m_sqDraw << m_eqDraw << m_bRoundOrig;
	else {
		if ( g_dwCamVer < NCVCSERIALVERSION_3620 ) {
			double	r, sq, eq;
			CPointD ct;
			ar >> r >> ct.x >> ct.y >> sq >> eq >> m_bRoundOrig;
			m_r  = (float)r;
			m_ct = ct;
			m_sqDraw = (float)sq;
			m_eqDraw = (float)eq;
		}
		else {
			ar >> m_r >> m_ct.x >> m_ct.y >> m_sqDraw >> m_eqDraw >> m_bRoundOrig;
		}
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
	CPointF	pts, pte, ptInit[4];
	float	sq, eq;

	// �n�_�I�_�Ɗp�x�̒���(s<e �Ƃ��ď���)
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
	tie(m_rcMax.left,   m_rcMax.right) = minmax(pts.x, pte.x);
	tie(m_rcMax.bottom, m_rcMax.top)   = minmax(pts.y, pte.y);

	// �p�x�̒����ƊJ�n�I���ی�(i,j)�̐ݒ�
	int	i = 0, j = 0;
	while ( sq >= RAD(90.0f) ) {
		sq -= RAD(90.0f);
		i++;
	}
	while ( eq >= RAD(90.0f) ) {
		eq -= RAD(90.0f);
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
	printf("CDXFarc::SetMaxRect() l=%.3f t=%.3f r=%.3f b=%.3f\n",
		m_rcMax.left, m_rcMax.top, m_rcMax.right, m_rcMax.bottom);
#endif
}

void CDXFarc::SwapMakePt(int)	// �߲�Ă̓���ւ�(+��]�����̔��])
{
	// �v�Z�l�̓���ւ�
	CDXFdata::SwapMakePt(0);
	// ��]�����̔��]
	SwapRound();
	// �p�x�̓���ւ�
	SwapAngle();
}

void CDXFarc::SwapNativePt(void)
{
	// ���W�l�̓���ւ�
	CDXFdata::SwapNativePt();
	// ��]�����̔��]
	m_bRound = m_bRoundOrig = !m_bRoundOrig;
	// �p�x�̓���ւ�
	swap(m_sq, m_eq);
	swap(m_sqDraw, m_eqDraw);
}

void CDXFarc::RoundObjPoint(const CPointF& ptOrg, float dRound)
{
	m_ct -= ptOrg;
	m_ct.RoundPoint(dRound);
	m_ct += ptOrg;
	m_sq += dRound;		m_eq += dRound;
	AngleTuning();
	m_sqDraw = m_sq;	m_eqDraw = m_eq;
	CDXFdata::RoundObjPoint(ptOrg, dRound);
}

size_t CDXFarc::SetVectorPoint(CVPointF& vpt, float k) const
{
	BOOL	bRound;
	float	sq, eq, dStep;
	CPointF	ptOrg;

	// k ���w�肳��Ă���Ƃ��͋������Ⴄ
	if ( k == 0.0f ) {
		bRound	= m_bRoundOrig;
		sq		= m_sqDraw;
		eq		= m_eqDraw;
		dStep	= ARCSTEP;
		ptOrg	= m_ct;
	}
	else {
		bRound	= m_bRound;
		sq		= m_sq;
		eq		= m_eq;
		dStep	= k / m_r;	// �ï�ߊp�x
		ptOrg	= m_ctTun;
	}

	// �~�ʔ��א����� vector<> �ɓo�^
	if ( k == 0.0f ) {
		// �n�_�o�^�� k==0.0 �̂Ƃ��̂�
		CPointF	pt(m_r * cos(sq) + ptOrg.x, m_r * sin(sq) + ptOrg.y);
		vpt.push_back(pt);
	}
	// ���ʏ���
	return SetVectorPointSub(bRound, sq, eq, dStep, ptOrg, vpt);
}

void CDXFarc::SetVectorPoint(CVPointF& vpt, size_t n) const
{
	// �p�x�͈͂� n�� �o�^
	float	sq, eq, dStep;
	if ( m_bRound ) {
		sq = m_sq;	eq = m_eq;
	}
	else {
		sq = m_eq;	eq = m_sq;
	}
	dStep = (eq - sq) / n;

	// ���̊p�x�ï�߂� vector<> �ɓo�^
	size_t m = SetVectorPointSub(m_bRound, m_sq, m_eq, dStep, m_ctTun, vpt);
	while ( n < m-- )
		vpt.pop_back();
}

void CDXFarc::SetWireHeteroData(const CDXFdata* pDataUV, CVPointF& vptXY, CVPointF& vptUV, float k) const
{
	if ( pDataUV->GetMakeType() == DXFLINEDATA ) {
		// ���g�̍��ݕ������
		SetVectorPoint(vptXY, k);
		// ��������
		pDataUV->SetVectorPoint(vptUV, vptXY.size());
	}
	else if ( pDataUV->GetMakeType()==DXFARCDATA &&
						GetRound() != static_cast<const CDXFarc*>(pDataUV)->GetRound() ) {
		float	dLen1 = GetLength(),
				dLen2 = pDataUV->GetLength();
		// �����������ݕ��ō��W�o�^���A�Z�����͂��̉񐔂ō��W�o�^
		if ( dLen1 > dLen2 ) {
			SetVectorPoint(vptXY, k);
			pDataUV->SetVectorPoint(vptUV, vptXY.size());
		}
		else {
			pDataUV->SetVectorPoint(vptUV, k);
			SetVectorPoint(vptXY, vptUV.size());
		}
	}
}

void CDXFarc::XRev(void)
{
	CDXFcircle::XRev();
	// ��]�����̔��]
	SwapRound();
	// �p�x�̒���(SwapAngle�ł͂Ȃ�)
	m_sq = PI - m_sq;
	m_eq = PI - m_eq;
	AngleTuning();
}

void CDXFarc::YRev(void)
{
	CDXFcircle::YRev();
	SwapRound();
	m_sq = PI2 - m_sq;
	m_eq = PI2 - m_eq;
	AngleTuning();
}

void CDXFarc::AngleTuning(void)
{
	if ( m_sq<0.0f || m_eq<0.0f ) {
		m_sq += PI2;
		m_eq += PI2;
	}
	// �����Ȍ덷�̋z��(=>���׉~�ʂ��傫�ȉ~�ɕς���Ă��܂�)�̂���
	// �x(deg)�Ŕ��f
	float	d;
	if ( m_bRound ) {
		d = ::RoundUp(DEG(m_sq));
		while ( d > ::RoundUp(DEG(m_eq)) )
			m_eq += PI2;
	}
	else {
		d = ::RoundUp(DEG(m_eq));
		while ( d > ::RoundUp(DEG(m_sq)) )
			m_sq += PI2;
	}
	// ����360���𒴂��Ȃ��悤��(�ی�)
	if ( DEG(fabs(m_eq-m_sq)) - 360.0 > NCMIN ) {
		float&	q = m_bRound ? m_eq : m_sq;		// �Q�ƌ^
		q -= PI2;
	}
}

BOOL CDXFarc::IsRangeAngle(const CPointF& pt) const
{
	float	q = m_ct.arctan(pt);
	if ( q < 0 )
		q += PI2;
	q = ::RoundUp(DEG(q));	// �x�Ŕ��f

	// ��ۓx���E�ɒ���
	if ( m_bRoundOrig ) {
		float	eq = DEG(m_eq);
		if ( ::RoundUp(DEG(m_sq))<=q && q<=::RoundUp(eq) )
			return TRUE;
		if ( eq>360.0f && q<=::RoundUp(eq-360.0f) )
			return TRUE;
	}
	else {
		float	sq = DEG(m_sq);
		if ( ::RoundUp(sq)>=q && q>=::RoundUp(DEG(m_eq)) )
			return TRUE;
		if ( sq>360.0f && q<=::RoundUp(sq-360.0f) )
			return TRUE;
	}

	return FALSE;
}

void CDXFarc::DrawTuning(float f)
{
	m_rDraw  = m_r * f;
	m_ptDraw = m_ct * f;
}

void CDXFarc::Draw(CDC* pDC) const
{
	// CDC::Arc() ���g���Ƃǂ����Ă��\�����Y����D���א����ɂ��ߎ�
	float	sq = m_sqDraw, st;

	CPointF	pt(m_rDraw * cos(sq) + m_ptDraw.x, m_rDraw * sin(sq) + m_ptDraw.y);
	pDC->MoveTo(pt);
#ifdef _DEBUGDRAW_DXF
	printf("CDXFarc::Draw() pts.x=%d pts.y=%d\n", (int)pt.x, (int)pt.y);
#endif
	if ( m_bRoundOrig ) {
		st = (m_eqDraw - sq) / ARCCOUNT;
		for ( sq+=st; sq<m_eqDraw; sq+=st ) {
			pt.SetPoint(m_rDraw * cos(sq) + m_ptDraw.x, m_rDraw * sin(sq) + m_ptDraw.y);
			pDC->LineTo(pt);
		}
	}
	else {
		st = (sq - m_eqDraw) / ARCCOUNT;
		for ( sq-=st; sq>m_eqDraw; sq-=st ) {
			pt.SetPoint(m_rDraw * cos(sq) + m_ptDraw.x, m_rDraw * sin(sq) + m_ptDraw.y);
			pDC->LineTo(pt);
		}
	}
	pt.SetPoint(m_rDraw * cos(m_eqDraw) + m_ptDraw.x, m_rDraw * sin(m_eqDraw) + m_ptDraw.y);
	pDC->LineTo(pt);
#ifdef _DEBUGDRAW_DXF
	printf("                pte.x=%d pte.y=%d\n", (int)pt.x, (int)pt.y);
#endif
#ifdef _DEBUGDRAW_DXF_EDGE_
	pt.SetPoint(m_rDraw * cos(m_eqDraw), m_rDraw * sin(m_eqDraw));
	pt += m_ptDraw;
	CRect	rc(pt.x-10, pt.y-10, pt.x+10, pt.y+10);
	pDC->Ellipse(&rc);
#endif
}

float CDXFarc::OrgTuning(BOOL bCalc/*=TRUE*/)
{
	m_bRound = m_bRoundOrig;	// ��]������������Ԃɖ߂�
	m_sq = m_sqDraw;
	m_eq = m_eqDraw;
	m_ctTun = m_ct - ms_ptOrg;
	// �ȉ� CDXFline �Ɠ���
#ifdef _DEBUG
	float	dResult = CDXFline::OrgTuning(bCalc);
	printf("   Round=%s cx=%f cy=%f\n", m_bRound ? "CCW" : "CW", m_ctTun.x, m_ctTun.y);
	printf("   sq=%f eq=%f\n", DEG(m_sq), DEG(m_eq));
	return dResult;
#else
	return CDXFline::OrgTuning(bCalc);
#endif
}

float CDXFarc::GetSelectPointGap(const CPointF& pt)
{
	float	sq, eq, dResult;
	if ( m_bRoundOrig ) {
		sq = m_sqDraw;	eq = m_eqDraw;
	}
	else {
		sq = m_eqDraw;	eq = m_sqDraw;
	}
	dResult = GetSelectPointGap_Circle(pt, sq, eq);
	if ( dResult == HUGE_VALF ) {
		// �͈͊O�̏ꍇ�C�[�_����̋����v�Z
		sq = GAPCALC(m_pt[0] - pt);
		eq = GAPCALC(m_pt[1] - pt);
		dResult = sqrt( min(sq, eq) );	// �߂����̋�����Ԃ�
	}
	return dResult;
}

BOOL CDXFarc::GetDirectionArraw(const CPointF&, CPointF pt[][3])
{
	// �ڐ��̌X�������߂� -> �_pt�ɒ���������̌X��
	float	q[] = {
		-atan2(m_pt[0].x - m_ct.x, m_pt[0].y - m_ct.y),		// X��Y���t�H
		-atan2(m_pt[1].x - m_ct.x, m_pt[1].y - m_ct.y)
	};
	// ��󂪏�ɉ~�ʂ̓����ɗ���悤�p�x�̕␳
	q[m_bRoundOrig ? 0 : 1] += PI;
	// �ڐ���������W�̌v�Z
	return GetDirectionArraw_Circle(q, m_pt, pt);
}

int CDXFarc::GetIntersectionPoint(const CDXFdata* pData, CPointF pt[], BOOL bEdge/*=TRUE*/) const
{
	int		nResult = 0;
	CPointF	pt1(pData->GetNativePoint(0)), pt2(pData->GetNativePoint(1));
	const CDXFcircle*	pCircle;
	const CDXFellipse*	pEllipse;

	// bEdge==TRUE : �[�_�������ꍇ�͌�_�Ȃ��Ƃ���
	switch ( pData->GetType() ) {
	case DXFLINEDATA:
		nResult = GetIntersectionPoint(pt1, pt2, pt, bEdge);
		break;
	case DXFARCDATA:
		if ( bEdge && (IsMatchPoint(pt1) || IsMatchPoint(pt2)) )
			break;
		// through
	case DXFCIRCLEDATA:
		pCircle = static_cast<const CDXFcircle*>(pData);
		tie(nResult, pt1, pt2) = ::CalcIntersectionPoint_CC(m_ct, pCircle->GetCenter(), m_r, pCircle->GetR());
		break;
	case DXFELLIPSEDATA:
		pEllipse = static_cast<const CDXFellipse*>(pData);
		if ( bEdge && pEllipse->IsArc() && (IsMatchPoint(pt1) || IsMatchPoint(pt2)) )
			break;
		// ���l�v�Z������A�ĺ��ިݸ�
		pt[0] = pt[1] = 0;
		return 2;	// �Ƃ肠������_�����邱�Ƃɂ��Ă���
	case DXFPOLYDATA:
		nResult = pData->GetIntersectionPoint(this, pt, bEdge);
		break;
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
				swap(pt1, pt2);
		}
	}

	if ( nResult > 1 )
		pt[1] = pt2;
	if ( nResult > 0 )
		pt[0] = pt1;

	return nResult;
}

int CDXFarc::GetIntersectionPoint(const CPointF& pts, const CPointF& pte, CPointF pt[], BOOL bEdge/*=TRUE*/) const
{
	if ( bEdge && (IsMatchPoint(pts) || IsMatchPoint(pte)) )
		return 0;

	int		nResult;
	CPointF	pt1, pt2;

	tie(nResult, pt1, pt2) = ::CalcIntersectionPoint_LC(pts, pte, m_ct, m_r);

	if ( nResult > 1 )
		pt[1] = pt2;
	if ( nResult > 0 )
		pt[0] = pt1;

	return nResult;
}

optional<CPointF>
CDXFarc::CalcOffsetIntersectionPoint
	(const CDXFdata* pNext, float r, BOOL bLeft) const
{
	CPointF	pto( GetNativePoint(1) ), pt1, pt2, ptResult;
	int		k1, k2, nResult;
	BOOL	bResult = FALSE;
	CDXFdata*	pData;
	const CDXFarc*		pArc;
	const CDXFpolyline*	pPolyline;

	switch ( pNext->GetType() ) {
	case DXFLINEDATA:
		// �������Ăōl���邽�߁A��]�����Ȃǂ𔽑΂ɂ���
		pt1 = pNext->GetNativePoint(1) - pto;
		pt2 = m_ct - pto;
		{
			optional<CPointF> ptr = ::CalcOffsetIntersectionPoint_LC(pt1, pt2, m_r, r, r,
						!m_bRoundOrig, !bLeft);
			if ( ptr ) {
				ptResult = *ptr + pto;
				bResult = TRUE;
			}
		}
		break;
	case DXFARCDATA:
		pArc = static_cast<const CDXFarc*>(pNext);
		k1 = m_bRoundOrig ? -1 : 1;
		k2 = pArc->GetRoundOrig() ? -1 : 1;
		if ( !bLeft ) {
			k1 = -k1;
			k2 = -k2;
		}
		tie(nResult, pt1, pt2) = ::CalcIntersectionPoint_CC(m_ct, pArc->GetCenter(),
				m_r+r*k1, pArc->GetR()+r*k2);
		if ( nResult > 1 ) {
			ptResult = GAPCALC(pt1-pto) < GAPCALC(pt2-pto) ? pt1 : pt2;
			bResult = TRUE;
		}
		else if ( nResult > 0 ) {
			ptResult = pt1;
			bResult = TRUE;
		}
		else
			bResult = FALSE;
		break;
	case DXFELLIPSEDATA:
		break;
	case DXFPOLYDATA:
		pPolyline = static_cast<const CDXFpolyline*>(pNext);
		pData = pPolyline->GetFirstObject();
		if ( pData ) {
			optional<CPointF> ptr = this->CalcOffsetIntersectionPoint(pData, r, bLeft);
			if ( pData->GetType() == DXFLINEDATA )
				delete	pData;
			return ptr;
		}
		break;
	}

	return bResult ? ptResult : optional<CPointF>();
}

int CDXFarc::CheckIntersectionCircle(const CPointF& ptc, float r) const
{
	int	nResult;
	CPointF	pt1, pt2;

	tie(nResult, pt1, pt2) = ::CalcIntersectionPoint_CC(m_ct, ptc, m_r, r);
	// ���m�Ɍ�_������ꍇ�����p�x�͈̔�����
	if ( nResult>1 && !IsRangeAngle(pt1) && !IsRangeAngle(pt2) )
		nResult = 0;
	// ��_�������Ă�(�ڂ��Ă��Ă�)�[�_���~�̓����Ȃ�NG
	if ( nResult<2 && (::RoundUp(sqrt(GAPCALC(m_pt[0]-ptc)))<r || ::RoundUp(sqrt(GAPCALC(m_pt[1]-ptc)))<r) )
		nResult = 2;

	return nResult;
}

//////////////////////////////////////////////////////////////////////
// �c�w�e�f�[�^��Ellipse�N���X
//////////////////////////////////////////////////////////////////////

CDXFellipse::CDXFellipse() : CDXFarc(DXFELLIPSEDATA, NULL, CPointF(), 0, 0, 0, TRUE, 4, 0)
{
}

CDXFellipse::CDXFellipse(LPCDXFEARGV lpEllipse, DWORD dwFlags) :
	CDXFarc(DXFELLIPSEDATA, lpEllipse->pLayer,
		lpEllipse->c, 0.0f, lpEllipse->sq, lpEllipse->eq, lpEllipse->bRound, 4, dwFlags)
{
	m_ptLong = lpEllipse->l;
	m_dShort = lpEllipse->s;
	// �ȉ~�̌X�����v�Z
	m_lqMake = m_lq = m_ptLong.arctan();
	// ����������
	Construct();
	// �e��v�Z
	EllipseCalc();
}

CDXFellipse::CDXFellipse(CLayerData* pLayer, const CDXFellipse* pData, LPCDXFBLOCK lpBlock, DWORD dwFlags) :
	CDXFarc(DXFELLIPSEDATA, pLayer, pData->GetCenter(), pData->GetR(),
		pData->GetStartAngle(), pData->GetEndAngle(), pData->GetRoundOrig(), 4, dwFlags)
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
		float	dRound = RAD(lpBlock->dRound);
		m_ct.RoundPoint(dRound);
		// �ȉ~�̉�]�́C�X��(m_lq)�Ŷ�ް
		m_ptLong.RoundPoint(dRound);
	}
	m_ct += lpBlock->ptOrg;
	if ( lpBlock->dwBlockFlg&DXFBLFLG_X || lpBlock->dwBlockFlg&DXFBLFLG_Y || lpBlock->dwBlockFlg&DXFBLFLG_R ) {
		m_dLongLength = m_ptLong.hypot();
		float	len = m_dLongLength * m_dShort;
		m_r = max(m_dLongLength, len);
		m_rMake = ::RoundUp(m_r);
		m_lqMake = m_lq = m_ptLong.arctan();
		EllipseCalc();
	}
	else {
		m_dLongLength	= pData->GetLongLength();
		m_lqMake = m_lq	= pData->GetLean();
		m_lqDrawCos = m_lqMakeCos = cos(m_lq);
		m_lqDrawSin = m_lqMakeSin = sin(m_lq);
		CPointF	pt;
		m_nPoint = pData->m_nPoint;
		for ( int i=0; i<m_nPoint; i++ )
			m_pt[i] = pData->GetNativePoint(i) + lpBlock->ptOrg;
		m_rcMax = pData->GetMaxRect();
		m_rcMax.OffsetRect(lpBlock->ptOrg);
	}
}

#ifdef _DEBUG
void CDXFellipse::DbgDump(void)
{
	printf("[ELLIPSE]\n");
	CDXFdata::DbgDump();
	printf("r=%.3f cx=%.3f cy=%.3f %s\n", m_r, m_ct.x, m_ct.y,
		m_bRound ? "CCW" : "CW");
}
#endif

void CDXFellipse::Serialize(CArchive& ar)
{
	CDXFdata::Serialize(ar);
	if ( ar.IsStoring() ) {
		ar << m_ct.x << m_ct.y << m_sqDraw << m_eqDraw << m_bRoundOrig;
		ar << m_ptLong.x << m_ptLong.y << m_dShort << m_lq;
	}
	else {
		if ( g_dwCamVer < NCVCSERIALVERSION_3620 ) {
			double	sq, eq, sh, lq;
			CPointD ct, pt;
			ar >> ct.x >> ct.y >> sq >> eq >> m_bRoundOrig;
			ar >> pt.x >> pt.y >> sh >> lq;
			m_ct     = ct;
			m_sqDraw = (float)sq;
			m_eqDraw = (float)eq;
			m_ptLong = pt;
			m_dShort = (float)sh;
			m_lq     = (float)lq;
		}
		else {
			ar >> m_ct.x >> m_ct.y >> m_sqDraw >> m_eqDraw >> m_bRoundOrig;
			ar >> m_ptLong.x >> m_ptLong.y >> m_dShort >> m_lq;
		}
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
	float	len = m_dLongLength * m_dShort;
	m_r = max(m_dLongLength, len);
	m_rMake = ::RoundUp(m_r);
	// �ȉ~���ȉ~�ʂ�
	m_bArc = DEG(fabs(m_eq-m_sq))+NCMIN < 360.0 ? TRUE : FALSE;
	if ( !m_bArc ) {
		m_sqDraw = m_sq = 0;
		m_eqDraw = m_eq = PI2;	// RAD(360.0)
	}
}

void CDXFellipse::EllipseCalc(void)
{
	int		i;
	float	dShort = m_dLongLength * m_dShort;

	// �X���v�Z
	m_lqDrawCos = m_lqMakeCos = cos(m_lq);
	m_lqDrawSin = m_lqMakeSin = sin(m_lq);
	// �ȉ~���ȉ~�ʂ��Őݒ肷��l���Ⴄ
	if ( m_bArc ) {
		// �~�ʂƓ����Ɏn�_�E�I�_�̌v�Z
		CPointF	pt[2];
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
		m_pt[0].SetPoint( m_dLongLength,    0.0f );	//   0��
		m_pt[1].SetPoint(          0.0f,  dShort );	//  90��
		m_pt[2].SetPoint(-m_dLongLength,    0.0f );	// 180��
		m_pt[3].SetPoint(          0.0f, -dShort );	// 270��
		CPointF	pt;
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
	CPointF	pt, pts, pte, ptc[4];
	float	sq, eq, dShort = m_dLongLength * m_dShort;

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
		pte.y = pts.y = 0.0f;
	}

	// �e�ی��̎��ő�l
	ptc[0].SetPoint(  m_dLongLength,    0.0f );
	ptc[1].SetPoint(           0.0f,  dShort );
	ptc[2].SetPoint( -m_dLongLength,    0.0f );
	ptc[3].SetPoint(           0.0f, -dShort );

	// �Q�_�̋�`�͕K���ʂ�̂ŁC
	// �����l�Ƃ��čő�l�E�ŏ��l����
	// �޶�č��W�Ȃ̂ŁAtop��bottom�͋t
	CRectF	rcMax;
	tie(rcMax.left,   rcMax.right) = minmax(pts.x, pte.x);
	tie(rcMax.bottom, rcMax.top)   = minmax(pts.y, pte.y);

	// �p�x�̒����ƊJ�n�I���ی�(i,j)�̐ݒ�
	int	i = 0, j = 0;
	while ( sq >= RAD(90.0f) ) {
		sq -= RAD(90.0f);
		i++;
	}
	while ( eq >= RAD(90.0f) ) {
		eq -= RAD(90.0f);
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
	printf("CDXFellipse::SetMaxRect() l=%.3f t=%.3f r=%.3f b=%.3f\n",
		m_rcMax.left, m_rcMax.top, m_rcMax.right, m_rcMax.bottom);
#endif
}

void CDXFellipse::SwapMakePt(int n)
{
	if ( !m_bArc || GetMakeType()==DXFPOINTDATA || GetMakeType()==DXFCIRCLEDATA ) {
		// ��������GetStartCutterPoint()����
		// �i�X�����l�������j�J�n�p�x���Čv�Z����K�v�����邽��
		// �~�Ɠ����ł悢
		CDXFcircle::SwapMakePt(n);
	}
	else if ( m_bArc ) {
		// ���W�l�̓���ւ��Ɖ�]�����̔��]
		CDXFarc::SwapMakePt(0);
	}
}


void CDXFellipse::SwapNativePt(void)
{
	if ( m_bArc )
		CDXFarc::SwapNativePt();
}

void CDXFellipse::RoundObjPoint(const CPointF& ptOrg, float dRound)
{
	m_lqMake = m_lq += dRound;
	m_lqDrawCos = cos(m_lq);
	m_lqDrawSin = sin(m_lq);
//	CDXFarc::RoundObjPoint(ptOrg, dRound);	// �J�n�E�I���p�x�̉�]�͕K�v�Ȃ�
	m_ct -= ptOrg;
	m_ct.RoundPoint(dRound);
	m_ct += ptOrg;
	CDXFdata::RoundObjPoint(ptOrg, dRound);
}

size_t CDXFellipse::SetVectorPoint(CVPointF& vpt, float) const
{
	float	sq = m_sqDraw,
			dShort = m_dLongLength * m_dShort;

	CPointF	pt1(m_dLongLength * cos(sq), dShort * sin(sq)),
			pt2(pt1.x * m_lqDrawCos - pt1.y * m_lqDrawSin + m_ct.x,
				pt1.x * m_lqDrawSin + pt1.y * m_lqDrawCos + m_ct.y);
	vpt.push_back(pt2);
	if ( m_bRoundOrig ) {
		for ( sq+=ARCSTEP; sq<m_eqDraw; sq+=ARCSTEP ) {
			pt1.SetPoint(m_dLongLength * cos(sq), dShort * sin(sq));
			pt2.SetPoint(pt1.x * m_lqDrawCos - pt1.y * m_lqDrawSin + m_ct.x,
						 pt1.x * m_lqDrawSin + pt1.y * m_lqDrawCos + m_ct.y);
			vpt.push_back(pt2);
		}
	}
	else {
		for ( sq-=ARCSTEP; sq>m_eqDraw; sq-=ARCSTEP ) {
			pt1.SetPoint(m_dLongLength * cos(sq), dShort * sin(sq));
			pt2.SetPoint(pt1.x * m_lqDrawCos - pt1.y * m_lqDrawSin + m_ct.x,
						 pt1.x * m_lqDrawSin + pt1.y * m_lqDrawCos + m_ct.y);
			vpt.push_back(pt2);
		}
	}
	pt1.SetPoint(m_dLongLength * cos(m_eqDraw), dShort * sin(m_eqDraw));
	pt2.SetPoint(pt1.x * m_lqDrawCos - pt1.y * m_lqDrawSin + m_ct.x,
				 pt1.x * m_lqDrawSin + pt1.y * m_lqDrawCos + m_ct.y);
	vpt.push_back(pt2);

	return 0;
}

void CDXFellipse::SetVectorPoint(CVPointF& vpt, size_t n) const
{
}

void CDXFellipse::SetWireHeteroData(const CDXFdata*, CVPointF&, CVPointF&, float) const
{
}

void CDXFellipse::XRev(void)
{
	if ( GetMakeType()==DXFPOINTDATA || GetMakeType()==DXFCIRCLEDATA ) {
		CDXFcircle::XRev();
	}
	else if ( m_bArc ) {
		// �P�������ł́A�X�����}�C�i�X�ɂ��邱�ƂŊJ�n�I���p�x��180����]�ɂȂ�
		// �X���̂Ȃ��n�_�I�_���W�C���Ε����̖����u�~�v�ōČv�Z
		CPointF	pts(-m_dLongLength * cos(m_sq), m_dLongLength * sin(m_sq)),
				pte(-m_dLongLength * cos(m_eq), m_dLongLength * sin(m_eq));
		m_ctTun.x = -m_ctTun.x;
		XYRev(pts, pte);
	}
	else {
		CDXFcircle::XRev();
		m_lqMake = -m_lqMake;
		m_lqMakeCos = cos(m_lqMake);
		m_lqMakeSin = sin(m_lqMake);
	}
}

void CDXFellipse::YRev(void)
{
	if ( GetMakeType()==DXFPOINTDATA || GetMakeType()==DXFCIRCLEDATA ) {
		CDXFcircle::YRev();
	}
	else if ( m_bArc ) {
		CPointF	pts(m_dLongLength * cos(m_sq), -m_dLongLength * sin(m_sq)),
				pte(m_dLongLength * cos(m_eq), -m_dLongLength * sin(m_eq));
		m_ctTun.y = -m_ctTun.y;
		XYRev(pts, pte);
	}
	else {
		CDXFcircle::YRev();
		m_lqMake = -m_lqMake;
		m_lqMakeCos = cos(m_lqMake);
		m_lqMakeSin = sin(m_lqMake);
	}
}

void CDXFellipse::XYRev(const CPointF& pts, const CPointF& pte)
{
	int		i;

	// �X���̍Čv�Z(�����]�Ȃ̂ŕ������])
	m_lqMake = -m_lqMake;
	m_lqMakeCos = cos(m_lqMake);
	m_lqMakeSin = sin(m_lqMake);

	// ��]�����̔��]
	SwapRound();
	// �p�x�̍Čv�Z
	m_sq = pts.arctan();
	m_eq = pte.arctan();
	AngleTuning();
	// (�p�x�Čv�Z��)�Ε����̔��f
	CPointF	pt1[2], pt;
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

void CDXFellipse::SetEllipseTunPoint(void)
{
	float	dShort = m_dLongLength * m_dShort;
	// 0�`1 -> X��
	m_ptTun[0].x =  m_dLongLength;
	m_ptTun[1].x = -m_dLongLength;
	m_ptTun[0].y = m_ptTun[1].y = 0;
	// 2�`3 -> Y��
	m_ptTun[2].y =  dShort;
	m_ptTun[3].y = -dShort;
	m_ptTun[2].x = m_ptTun[3].x = 0;
	//
	m_nArrayExt = -1;	// ���̋ߐڌ����Ŋm���ɓ�����Ԃ���悤��
	// ��]
	CPointF	pt;
	for ( int i=0; i<m_nPoint; i++ ) {
		pt = m_ptTun[i];
		m_ptTun[i].x = pt.x * m_lqMakeCos - pt.y * m_lqMakeSin + m_ctTun.x;
		m_ptTun[i].y = pt.x * m_lqMakeSin + pt.y * m_lqMakeCos + m_ctTun.y;
	}
}

void CDXFellipse::DrawTuning(float f)
{
	m_dDrawLongLength = m_dLongLength * f;
	// ���S���W�̒���
	CDXFarc::DrawTuning(f);
}

void CDXFellipse::Draw(CDC* pDC) const
{
	float	sq = m_sqDraw, st,
			dShort = m_dDrawLongLength * m_dShort;

	CPointF	pt(m_dDrawLongLength * cos(sq), dShort * sin(sq));
	CPointF	ptDraw(pt.x * m_lqDrawCos - pt.y * m_lqDrawSin + m_ptDraw.x,
				   pt.x * m_lqDrawSin + pt.y * m_lqDrawCos + m_ptDraw.y);
	pDC->MoveTo(ptDraw);
#ifdef _DEBUGDRAW_DXF
	printf("CDXFellipse::Draw() pts.x=%d pts.y=%d\n", (int)pt.x, (int)pt.y);
#endif
	if ( m_bRoundOrig ) {
		st = (m_eqDraw - sq) / ARCCOUNT;
		for ( sq+=st; sq<m_eqDraw; sq+=st ) {
			pt.SetPoint(m_dDrawLongLength * cos(sq), dShort * sin(sq));
			ptDraw.SetPoint(pt.x * m_lqDrawCos - pt.y * m_lqDrawSin + m_ptDraw.x,
							pt.x * m_lqDrawSin + pt.y * m_lqDrawCos + m_ptDraw.y);
			pDC->LineTo(ptDraw);
		}
	}
	else {
		st = (sq - m_eqDraw) / ARCCOUNT;
		for ( sq-=st; sq>m_eqDraw; sq-=st ) {
			pt.SetPoint(m_dDrawLongLength * cos(sq), dShort * sin(sq));
			ptDraw.SetPoint(pt.x * m_lqDrawCos - pt.y * m_lqDrawSin + m_ptDraw.x,
							pt.x * m_lqDrawSin + pt.y * m_lqDrawCos + m_ptDraw.y);
			pDC->LineTo(ptDraw);
		}
	}
	pt.SetPoint(m_dDrawLongLength * cos(m_eqDraw), dShort * sin(m_eqDraw));
	ptDraw.SetPoint(pt.x * m_lqDrawCos - pt.y * m_lqDrawSin + m_ptDraw.x,
					pt.x * m_lqDrawSin + pt.y * m_lqDrawCos + m_ptDraw.y);
	pDC->LineTo(ptDraw);
#ifdef _DEBUGDRAW_DXF
	printf("                    pte.x=%d pte.y=%d\n", (int)pt.x, (int)pt.y);
#endif
}

float CDXFellipse::OrgTuning(BOOL bCalc/*=TRUE*/)
{
	float	dResult;

	m_lqMake = m_lq;
	m_lqMakeCos = m_lqDrawCos;
	m_lqMakeSin = m_lqDrawSin;
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
		dResult = bCalc ? GetEdgeGap(ms_pData->GetEndCutterPoint()) : 0.0f;
	}
#ifdef _DEBUG
	printf("   lx=%f ly=%f LongLen=%f Short=%f\n",
			m_ptLong.x, m_ptLong.y, m_dLongLength, m_dShort);
	printf("   lq=%f\n", DEG(m_lq));
#endif

	return dResult;
}

float CDXFellipse::GetSelectPointGap(const CPointF& pt)
{
	float	sq, eq, q1, q2, dResult;
	if ( m_bRoundOrig ) {
		sq = m_sqDraw;	eq = m_eqDraw;
	}
	else {
		sq = m_eqDraw;	eq = m_sqDraw;
	}
	CPointF	pt1(pt - m_ct);
	// �X����������ԂŔ��肷��悤�د��߲�Ă�␳
	if ( fabs(m_lq) > 0.0f )
		pt1.RoundPoint(-m_lq);
	// �Ε����̕␳��C�p�x�v�Z
	pt1.y /= m_dShort;
	if ( (q1=pt1.arctan()) < 0.0f )
		q1 += PI2;
	q2 = q1 + PI2;
	// �د��߲�Ă��p�x�͈͓̔��ɂ��邩
	if ( !m_bArc || (sq <= q1 && q1 <= eq) || (sq <= q2 && q2 <= eq) ) {
		dResult = fabs(m_dLongLength - pt1.hypot());
	}
	else {
		// �͈͊O�̏ꍇ�C�[�_����̋����v�Z
		sq = GAPCALC(m_pt[0] - pt);
		eq = GAPCALC(m_pt[1] - pt);
		dResult = sqrt( min(sq, eq) );	// �߂����̋�����Ԃ�
	}
	return dResult;
}

BOOL CDXFellipse::GetDirectionArraw(const CPointF& ptClick, CPointF ptResult[][3])
{
	CPointF	pt[2], pt1;
	float	q[2];

	if ( m_bArc ) {
		// �Ε����ƌX���p���l��
		for ( int i=0; i<SIZEOF(q); i++ ) {
			pt1 = pt[i] = m_pt[i];
			pt1 -= m_ct;
			if ( fabs(m_lq) > 0.0f )
				pt1.RoundPoint(-m_lq);
			q[i] = -atan2(pt1.x, pt1.y);		// X��Y���t�H
			pt1.x = cos(q[i]);
			pt1.y = sin(q[i]);
			q[i] = atan2(pt1.y*m_dShort, pt1.x) + m_lq;
		}
		// ��󂪏�ɉ~�ʂ̓����ɗ���悤�p�x�̕␳
		q[m_bRoundOrig ? 0 : 1] += PI;
	}
	else {
		// �د��߲�Ă�␳
		pt1 = ptClick;
		if ( fabs(m_lq) > 0.0f ) {
			pt1 -= m_ct;
			pt1.RoundPoint(-m_lq);
			pt1 += m_ct;
		}
		GetQuarterPoint(pt1, pt);
		q[0] = -atan2(pt[0].x - m_ct.x, pt[0].y - m_ct.y)+PI;	// GetQuarterPoint()�Ŕ����v
		q[1] = -atan2(pt[1].x - m_ct.x, pt[1].y - m_ct.y);
	}

	return GetDirectionArraw_Circle(q, pt, ptResult);
}

BOOL CDXFellipse::IsDirectionPoint(const CPointF& pts, const CPointF& pte)
{
	BOOL	bRound = m_bRound, bResult = FALSE;
	CDXFcircle::IsDirectionPoint(pts, pte);
	return ( m_bRound != bRound );
}

void CDXFellipse::SetRoundFixed(BOOL bRound)
{
	if ( m_bRound != bRound ) {
		m_bRound = bRound;
		SwapAngle();
	}
}

int CDXFellipse::GetIntersectionPoint(const CDXFdata* pData, CPointF pt[], BOOL bEdge/*=TRUE*/) const
{
	int		nResult = 0;
	CPointF	pt1(pData->GetNativePoint(0)), pt2(pData->GetNativePoint(1));
	const CDXFcircle*	pCircle = NULL;

	// bEdge==TRUE : �[�_�������ꍇ�͌�_�Ȃ��Ƃ���
	switch ( pData->GetType() ) {
	case DXFLINEDATA:
		return GetIntersectionPoint(pt1, pt2, pt, bEdge);
	case DXFARCDATA:
		if ( bEdge && m_bArc && (IsMatchPoint(pt1) || IsMatchPoint(pt2)) )
			break;
		// through
	case DXFCIRCLEDATA:
		pCircle = static_cast<const CDXFcircle*>(pData);
		// ���l�v�Z������A�ĺ��ިݸ�
		pt[0] = pt[1] = 0;
		return 2;	// �Ƃ肠������_�����邱�Ƃɂ��Ă���
	case DXFELLIPSEDATA:
		pt[0] = pt[1] = 0;
		return 2;
	case DXFPOLYDATA:
		nResult = pData->GetIntersectionPoint(this, pt, bEdge);
		break;
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
					swap(pt1, pt2);
			}
		}
	}

	if ( nResult > 1 )
		pt[1] = pt2;
	if ( nResult > 0 )
		pt[0] = pt1;

	return nResult;
}

int CDXFellipse::GetIntersectionPoint(const CPointF& pts, const CPointF& pte, CPointF pt[], BOOL bEdge/*=TRUE*/) const
{
	if ( bEdge && m_bArc && (IsMatchPoint(pts) || IsMatchPoint(pte)) )
		return 0;

	int		nResult;
	CPointF	pt1, pt2;

	tie(nResult, pt1, pt2) = ::CalcIntersectionPoint_LE(pt1, pt2,
			m_ct, m_dLongLength, m_dLongLength*m_dShort, m_lq);

	if ( nResult > 1 )
		pt[1] = pt2;
	if ( nResult > 0 )
		pt[0] = pt1;

	return nResult;
}

optional<CPointF>
CDXFellipse::CalcOffsetIntersectionPoint
	(const CDXFdata* pNext, float r, BOOL bLeft) const
{
	CPointF	pto( GetNativePoint(1) ), p1, p2, pt;
	optional<CPointF>	ptResult;
	CDXFdata*	pData;
	const CDXFpolyline*	pPolyline;

	switch ( pNext->GetType() ) {
	case DXFLINEDATA:
		p1 = pNext->GetNativePoint(1) - pto;
		p2 = m_ct - pto;
		ptResult = ::CalcOffsetIntersectionPoint_LE(p1, p2,
			m_dLongLength, m_dLongLength*m_dShort, m_lq, r,
			!m_bRoundOrig, !bLeft);
		break;
	case DXFARCDATA:
		break;
	case DXFELLIPSEDATA:
		break;
	case DXFPOLYDATA:
		pPolyline = static_cast<const CDXFpolyline*>(pNext);
		pData = pPolyline->GetFirstObject();
		if ( pData ) {
			ptResult = this->CalcOffsetIntersectionPoint(pData, r, bLeft);
			if ( pData->GetType() == DXFLINEDATA )
				delete	pData;
			return ptResult;
		}
		break;
	}

	if ( ptResult ) {
		pt = *ptResult + pto;
		return pt;
	}
	return ptResult;
}

int CDXFellipse::CheckIntersectionCircle(const CPointF&, float) const
{
	return 0;
}

//////////////////////////////////////////////////////////////////////
// �c�w�e�f�[�^��Polyline�N���X
//////////////////////////////////////////////////////////////////////

CDXFpolyline::CDXFpolyline() : CDXFline(DXFPOLYDATA, NULL, 2, 0)
{
	m_dwPolyFlags = DXFPOLY_SEQ|DXFPOLY_SEQBAK;
	m_posSel = NULL;
	m_pArrawLine = NULL;
	ZEROCLR(m_nObjCnt);		// m_nObjCnt[i++]=0
}

CDXFpolyline::CDXFpolyline(CLayerData* pLayer, const CDXFpolyline* pPoly, LPCDXFBLOCK lpBlock, DWORD dwFlags) :
	CDXFline(DXFPOLYDATA, pLayer, 2, dwFlags)
{
	m_dwPolyFlags = pPoly->GetPolyFlag();
	m_posSel = NULL;
	m_pArrawLine = NULL;
	ZEROCLR(m_nObjCnt);
	// ��O�۰�͏�ʂŷ���
	CDXFdata*		pData;

	PLIST_FOREACH(CDXFdata* pDataSrc, &pPoly->m_ltVertex)
		switch ( pDataSrc->GetType() ) {
		case DXFPOINTDATA:
			pData = new CDXFpoint(pLayer, static_cast<CDXFpoint*>(pDataSrc), lpBlock, DXFFLG_POLYCHILD);
			ASSERT(pData);
			m_ltVertex.AddTail(pData);
			break;
		case DXFARCDATA:
			// �e���Ǝ��̊g�嗦�� CDXFarc -> CDXFellipse
			if ( lpBlock->dMagni[NCA_X] != lpBlock->dMagni[NCA_Y] ) {
				DXFEARGV	dxfEllipse;
				(static_cast<CDXFarc*>(pDataSrc))->SetEllipseArgv(lpBlock, &dxfEllipse);
				pData = new CDXFellipse(&dxfEllipse, DXFFLG_POLYCHILD); 
				ASSERT(pData);
				m_ltVertex.AddTail(pData);
				m_dwPolyFlags |= DXFPOLY_ELLIPSE;
			}
			else {
				pData = new CDXFarc(pLayer, static_cast<CDXFarc*>(pDataSrc), lpBlock, DXFFLG_POLYCHILD);
				ASSERT(pData);
				m_ltVertex.AddTail(pData);
			}
			break;
		}
	END_FOREACH

	EndSeq();
}

CDXFpolyline::~CDXFpolyline()
{
	if ( m_pArrawLine )
		delete	m_pArrawLine;
	PLIST_FOREACH(CDXFdata* pData, &m_ltVertex)
		delete pData;
	END_FOREACH
}

#ifdef _DEBUG
void CDXFpolyline::DbgDump(void)
{
}
#endif

void CDXFpolyline::Serialize(CArchive& ar)
{
	BOOL	bDummy = TRUE;

	CDXFdata::Serialize(ar);
	if ( ar.IsStoring() ) {
		ar << m_dwPolyFlags << bDummy;
		if ( m_pArrawLine ) {
			ar << (BYTE)1;
			ar.WriteObject(m_pArrawLine);
		}
		else {
			ar << (BYTE)0;
		}
	}
	else {
		BYTE	nByte;
		ar >> m_dwPolyFlags >> bDummy;
		if ( g_dwCamVer >= NCVCSERIALVERSION_3602 ) {
			ar >> nByte;
			if ( nByte > 0 )
				m_pArrawLine = static_cast<CDXFline*>(ar.ReadObject(RUNTIME_CLASS(CDXFline)));
		}
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

void CDXFpolyline::SetMaxRect(void)
{
	CPointF	pt;

	m_rcMax.TopLeft()     = m_pt[0];	// ��L��`������
	m_rcMax.BottomRight() = m_pt[0];

	PLIST_FOREACH(CDXFdata*	pData, &m_ltVertex)
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
			break;
		case DXFARCDATA:
			m_rcMax |= pData->GetMaxRect();
			break;
		case DXFELLIPSEDATA:
			m_rcMax |= pData->GetMaxRect();
			break;
		}
	END_FOREACH
}

void CDXFpolyline::SwapMakePt(int)
{
	// �������g�̍��W����ւ�
	m_dwPolyFlags ^= DXFPOLY_SEQ;	// XOR DXFPOLY_SEQ => �ޯĔ��]
	CDXFdata::SwapMakePt(0);

	// �\����޼ު�Ă̍��W����ւ�
	PLIST_FOREACH(CDXFdata* pData, &m_ltVertex)
		// CDXFpoint�ȊO�͉�]��������ݼ�
		if ( pData->GetType() != DXFPOINTDATA )
			pData->SwapMakePt(0);
	END_FOREACH
}

void CDXFpolyline::SwapNativePt(void)
{
	m_dwPolyFlags ^= (DXFPOLY_SEQ|DXFPOLY_SEQBAK);
	CDXFdata::SwapNativePt();

	PLIST_FOREACH(CDXFdata* pData, &m_ltVertex)
		if ( pData->GetType() != DXFPOINTDATA )
			pData->SwapNativePt();
	END_FOREACH
}

void CDXFpolyline::RoundObjPoint(const CPointF& ptOrg, float dRound)
{
	PLIST_FOREACH(CDXFdata* pData, &m_ltVertex)
		pData->RoundObjPoint(ptOrg, dRound);
	END_FOREACH
	CDXFdata::RoundObjPoint(ptOrg, dRound);
}

BOOL CDXFpolyline::IsDirectionPoint(const CPointF&, const CPointF& pte)
{
#ifdef _DEBUG
	ASSERT( m_pArrawLine );
#else
	if ( !m_pArrawLine )	// ���S��
		return FALSE;
#endif
	m_pArrawLine->OrgTuning(FALSE);
	// ���_��pte�ɾ�Ă���Ă���
	float	dGapS = GAPCALC( m_pArrawLine->GetStartCutterPoint() - pte ),
			dGapE = GAPCALC( m_pArrawLine->GetEndCutterPoint()   - pte );

	return  dGapS < dGapE ? TRUE : FALSE;
}

size_t CDXFpolyline::SetVectorPoint(CVPointF& vpt, float k) const
{
	CDXFdata*	pData;

	for ( POSITION pos=GetFirstVertex(); pos; ) {
		pData = GetNextVertex(pos);
		// �����Ɍ����̂́ADXFPOINTDATA, DXFARCDATA, DXFELLIPSEDATA �̂�
		pData->SetVectorPoint(vpt, k);
	}

	return 0;
}

void CDXFpolyline::SetVectorPoint(CVPointF& vpt, size_t) const
{
}

void CDXFpolyline::SetWireHeteroData(const CDXFdata*, CVPointF&, CVPointF&, float) const
{
}

BOOL CDXFpolyline::SetVertex(LPCDXFPARGV lpArgv)
{
	CDXFpoint*	pPoint = new CDXFpoint(lpArgv, DXFFLG_POLYCHILD);
	ASSERT(pPoint);
	m_ltVertex.AddTail(pPoint);
	return TRUE;
}

BOOL CDXFpolyline::SetVertex(LPCDXFPARGV lpArgv, float dBow, const CPointF& pts)
{
	float	q = fabs(4 * atan(dBow));
	float	d = lpArgv->c.hypot(&pts);
	// �Q�_���������ꍇ�͵�޼ު�Ă𐶐����Ȃ�
	if ( d < NCMIN )
		return TRUE;
	float	r = fabs( (d/2) / sin(q/2) );
	// �Q�̉~�̌�_�����߂�
	CPointF	pt1, pt2;
	int		nResult;
	tie(nResult, pt1, pt2) = ::CalcIntersectionPoint_CC(pts, lpArgv->c, r, r);
	if ( nResult < 1 )
		return FALSE;
	// CDXFarc�o�^����
	DXFAARGV	dxfArc;
	dxfArc.pLayer	= lpArgv->pLayer;
	dxfArc.r		= r;
	// �ǂ���̉����̗p���邩
	BOOL	bRound;
	float	sq1, eq1, sq2, eq2;
	if ( (sq1=pt1.arctan(pts)) < 0.0f )
		sq1 += PI2;
	if ( (eq1=pt1.arctan(lpArgv->c)) < 0.0f )
		eq1 += PI2;
	if ( nResult == 1 ) {	// �d��
		sq2 = sq1;	eq2 = eq1;
	}
	else {
		if ( (sq2=pt2.arctan(pts)) < 0.0f )
			sq2 += PI2;
		if ( (eq2=pt2.arctan(lpArgv->c)) < 0.0f )
			eq2 += PI2;
	}
#ifdef _DEBUG
	printf("CDXFpolyline::SetVertex() pts x=%f y=%f pte x=%f y=%f\n", pts.x, pts.y, lpArgv->c.x, lpArgv->c.y);
	printf("q=%f d=%f r=%f\n", DEG(q), d/2, r);
	printf("ptc1 x=%f y=%f sq1=%f eq1=%f\n", pt1.x, pt1.y, DEG(sq1), DEG(eq1));
	printf("ptc2 x=%f y=%f sq2=%f eq2=%f\n", pt2.x, pt2.y, DEG(sq2), DEG(eq2));
#endif

	if ( dBow > 0 ) {	// �����v���w��
		bRound = TRUE;
		while ( sq1 > eq1 )
			eq1 += PI2;
		while ( sq2 > eq2 )
			eq2 += PI2;
		if ( fabs(eq1 - sq1 - q) < fabs(eq2 - sq2 - q) ) {
			dxfArc.c	= pt1;
			dxfArc.sq	= sq1;
			dxfArc.eq	= eq1;
#ifdef _DEBUG
			printf("CCW pt1\n");
#endif
		}
		else {
			dxfArc.c	= pt2;
			dxfArc.sq	= sq2;
			dxfArc.eq	= eq2;
#ifdef _DEBUG
			printf("CCW pt2\n");
#endif
		}
	}
	else {
		bRound = FALSE;
		while ( sq1 < eq1 )
			sq1 += PI2;
		while ( sq2 < eq2 )
			sq2 += PI2;
		if ( fabs(sq1 - eq1 - q) < fabs(sq2 - eq2 - q) ) {
			dxfArc.c	= pt1;
			dxfArc.sq	= sq1;
			dxfArc.eq	= eq1;
#ifdef _DEBUG
			printf("CW pt1\n");
#endif
		}
		else {
			dxfArc.c	= pt2;
			dxfArc.sq	= sq2;
			dxfArc.eq	= eq2;
#ifdef _DEBUG
			printf("CW pt2\n");
#endif
		}
	}
	CDXFarc*	pArc = new CDXFarc(&dxfArc, bRound, pts, lpArgv->c, DXFFLG_POLYCHILD);
	ASSERT(pArc);
	m_ltVertex.AddTail(pArc);

	// NC���ނ𐶐����₷���悤��CDXFpoint��޼ު�Ă�����
	DXFPARGV	dxfPoint;
	dxfPoint.pLayer	= lpArgv->pLayer;
	dxfPoint.c		= lpArgv->c;
	CDXFpoint*	pPoint = new CDXFpoint(&dxfPoint, DXFFLG_POLYCHILD);
	ASSERT(pPoint);
	m_ltVertex.AddTail(pPoint);

	return TRUE;
}

void CDXFpolyline::EndSeq(void)
{
	ASSERT( !m_ltVertex.IsEmpty() );
	ASSERT( m_ltVertex.GetHead()->GetType() == DXFPOINTDATA );
	// �ŏ��ƍŌ�̓_�͕K��CDXFpoint
	m_pt[0] = m_ltVertex.GetHead()->GetNativePoint(0);
	m_pt[1] = m_ltVertex.GetTail()->GetNativePoint(0);
	if ( m_pt[0] == m_pt[1] )
		SetPolyFlag(DXFPOLY_CLOSED);	// �������ײ݈���

	SetMaxRect();

	// DataCount
	PLIST_FOREACH(CDXFdata*	pData, &m_ltVertex)
		switch ( pData->GetType() ) {
		case DXFPOINTDATA:
			m_nObjCnt[0]++;
			break;
		case DXFARCDATA:
			m_nObjCnt[1]++;
			m_nObjCnt[0]--;		// �I�_�̕������Z
			break;
		case DXFELLIPSEDATA:
			m_nObjCnt[2]++;
			m_nObjCnt[0]--;
			break;
		}
	END_FOREACH
	m_nObjCnt[0]--;		// �n�_���܂܂�邽�߁u-1�v�Ő��̐�
#ifdef _DEBUG
	printf("CDXFpolyline::EndSeq() LineCnt=%d ArcCnt=%d Ellipse=%d\n",
			m_nObjCnt[0], m_nObjCnt[1], m_nObjCnt[2]);
	printf("l=%.3f t=%.3f r=%.3f b=%.3f\n",
		m_rcMax.left, m_rcMax.top, m_rcMax.right, m_rcMax.bottom);
#endif
}

CDXFdata* CDXFpolyline::GetFirstObject(void) const
{
	POSITION	pos = GetFirstVertex();
	CDXFdata*	pData1 = GetNextVertex(pos);
	CDXFdata*	pData2 = NULL;

	if ( pos ) {
		pData2 = GetNextVertex(pos);
		if ( pData2->GetType() == DXFPOINTDATA ) {
			DXFLARGV	dxfLine;
			dxfLine.pLayer = NULL;
			dxfLine.s = pData1->GetNativePoint(0);
			dxfLine.e = pData2->GetNativePoint(0);
			pData2 = new CDXFline(&dxfLine);
		}
	}

	// �߂�l�� CDXFline �̏ꍇ�A
	// �Ăяo�����ŕK�� delete ���邱��
	return pData2;
}

CDXFdata* CDXFpolyline::GetTailObject(void) const
{
	CDXFdata*	pData1 = NULL;
	CDXFdata*	pData2;
	POSITION	pos;
	DXFLARGV	dxfLine;
	dxfLine.pLayer = NULL;

	if ( m_dwPolyFlags & DXFPOLY_SEQ ) {
		pos = m_ltVertex.GetTailPosition();
		pData2 = m_ltVertex.GetPrev(pos);
		if ( pos ) {
			pData1 = m_ltVertex.GetPrev(pos);
			if ( pData1->GetType() == DXFPOINTDATA ) {
				dxfLine.s = pData1->GetNativePoint(0);
				dxfLine.e = pData2->GetNativePoint(0);
				pData1 = new CDXFline(&dxfLine);
			}
		}
	}
	else {
		pos = m_ltVertex.GetHeadPosition();
		pData2 = m_ltVertex.GetNext(pos);
		if ( pos ) {
			pData1 = m_ltVertex.GetNext(pos);
			if ( pData1->GetType() == DXFPOINTDATA ) {
				dxfLine.s = pData1->GetNativePoint(0);
				dxfLine.e = pData2->GetNativePoint(0);
				pData1 = new CDXFline(&dxfLine);
			}
		}
	}

	// �߂�l�� CDXFline �̏ꍇ�A
	// �Ăяo�����ŕK�� delete ���邱��
	return pData1;
}

void CDXFpolyline::CheckPolylineIntersection(void)
{
	CDXFdata*	pData;
	optional<CPointF>	pts;
	CPointF				pte;

	// ��_��������܂�ٰ��
	for ( POSITION pos=GetFirstVertex(); pos && !IsIntersection(); ) {
		pData = GetNextVertex(pos);
		switch ( pData->GetType() ) {
		case DXFPOINTDATA:
			pte = pData->GetNativePoint(0);
			if ( pts && (*pts)!=pte && pos )
				CheckPolylineIntersection_SubLoop(*pts, pte, pos);
			pts = pte;
			break;
		case DXFARCDATA:
			CheckPolylineIntersection_SubLoop(static_cast<CDXFarc*>(pData), pos);
			pts.reset();
			break;
		case DXFELLIPSEDATA:
			CheckPolylineIntersection_SubLoop(static_cast<CDXFellipse*>(pData), pos);
			pts.reset();
			break;
		}
	}
}

void CDXFpolyline::CheckPolylineIntersection_SubLoop
	(const CPointF& pts1, const CPointF& pte1, POSITION pos1)
{
	int				nResult;
	CDXFdata*		pData;
	CDXFarc*		pArc;
	CDXFellipse*	pEllipse;
	CPointF			pr1, pr2, pte2;
	optional<CPointF>	pts2,
						ptResult;

	for ( POSITION pos2=pos1; pos2; ) {
		pData = GetNextVertex(pos2);
		switch ( pData->GetType() ) {
		case DXFPOINTDATA:
			pte2 = pData->GetNativePoint(0);
			if ( pts2 && (*pts2)!=pte2 && pts1!=pte2 && pos2 ) {
				ptResult = ::CalcIntersectionPoint_LL(pts1, pte1, *pts2, pte2);
				if ( ptResult ) {
#ifdef _DEBUG
					printf("PolyIn (%f, %f)-(%f, %f) .. (%f, %f)-(%f, %f)\n",
						pts1.x, pts1.y, pte1.x, pte1.y, (*pts2).x, (*pts2).y, pte2.x, pte2.y);
#endif
					SetPolyFlag(DXFPOLY_INTERSEC);
					pos2 = NULL;	// ����ȏ�̌����͕K�v�Ȃ�
					break;
				}
			}
			pts2 = pte2;
			break;

		case DXFARCDATA:
			pArc = static_cast<CDXFarc*>(pData);
			tie(nResult, pr1, pr2) = ::CalcIntersectionPoint_LC(pts1, pte1,
					pArc->GetCenter(), pArc->GetR());
			if ( nResult>1 && pArc->IsRangeAngle(pr2) ) {
				SetPolyFlag(DXFPOLY_INTERSEC);
				pos2 = NULL;
				break;
			}
			if ( nResult>0 && pte1!=pArc->GetNativePoint(0) && pts1!=pArc->GetNativePoint(1) &&
							pArc->IsRangeAngle(pr1) ) {
				// pte1!=pArc->GetNativePoint(0) --> �אڌ�_�̖h�~
				// pts1!=pArc->GetNativePoint(1) --> �����_�̖h�~
				SetPolyFlag(DXFPOLY_INTERSEC);
				pos2 = NULL;
				break;
			}
			pts2.reset();
			break;

		case DXFELLIPSEDATA:
			pEllipse = static_cast<CDXFellipse*>(pData);
			tie(nResult, pr1, pr2) = ::CalcIntersectionPoint_LE(pts1, pte1,
					pEllipse->GetCenter(), pEllipse->GetLongLength(), pEllipse->GetShortLength(), pEllipse->GetLean());
			if ( nResult>1 && pEllipse->IsRangeAngle(pr2) ) {
				SetPolyFlag(DXFPOLY_INTERSEC);
				pos2 = NULL;
				break;
			}
			if ( nResult>0 && pte1!=pEllipse->GetNativePoint(0) && pts1!=pEllipse->GetNativePoint(1) &&
							pEllipse->IsRangeAngle(pr1) ) {
				SetPolyFlag(DXFPOLY_INTERSEC);
				pos2 = NULL;
				break;
			}
			pts2.reset();
			break;
		}
	}
}

void CDXFpolyline::CheckPolylineIntersection_SubLoop(const CDXFarc* pData1, POSITION pos1)
{
	int				nResult;
	CDXFdata*		pData2;
	CDXFarc*		pArc;
	CDXFellipse*	pEllipse;
	float			r = pData1->GetR();
	CPointF			pr1, pr2, pte2,
					pts1(pData1->GetNativePoint(0)),
					pte1(pData1->GetNativePoint(1)),
					ptc(pData1->GetCenter());
	optional<CPointF>	pts2;

	for ( POSITION pos2=pos1; pos2; ) {
		pData2 = GetNextVertex(pos2);
		switch ( pData2->GetType() ) {
		case DXFPOINTDATA:
			pte2 = pData2->GetNativePoint(0);
			if ( pts2 ) {
				tie(nResult, pr1, pr2) = ::CalcIntersectionPoint_LC(*pts2, pte2, ptc, r);
				if ( nResult>1 && pData1->IsRangeAngle(pr2) ) {
					SetPolyFlag(DXFPOLY_INTERSEC);
					pos2 = NULL;
					break;
				}
				if ( nResult>0 && pte1!=(*pts2) && pts1!=pte2 && pData1->IsRangeAngle(pr1) ) {
					SetPolyFlag(DXFPOLY_INTERSEC);
					pos2 = NULL;
					break;
				}
			}
			pts2 = pte2;
			break;

		case DXFARCDATA:
			pArc = static_cast<CDXFarc*>(pData2);
			tie(nResult, pr1, pr2) = ::CalcIntersectionPoint_CC(ptc, pArc->GetCenter(), r, pArc->GetR());
			if ( nResult>1 && pData1->IsRangeAngle(pr2) && pArc->IsRangeAngle(pr2) ) {
				SetPolyFlag(DXFPOLY_INTERSEC);
				pos2 = NULL;
				break;
			}
			if ( nResult>0 && pte1!=pArc->GetNativePoint(0) && pts1!=pArc->GetNativePoint(1) &&
							pData1->IsRangeAngle(pr1) && pArc->IsRangeAngle(pr1) ) {
#ifdef _DEBUG
				printf("PolyIn c(%f, %f) q(%f, %f) r%f .. c(%f, %f) q(%f, %f) r%f\n",
					ptc.x, ptc.y, DEG(pData1->GetStartAngle()), DEG(pData1->GetEndAngle()), r,
					pArc->GetCenter().x, pArc->GetCenter().y, DEG(pArc->GetStartAngle()), DEG(pArc->GetEndAngle()), pArc->GetR());
#endif
				SetPolyFlag(DXFPOLY_INTERSEC);
				pos2 = NULL;
				break;
			}
			pts2.reset();
			break;

		case DXFELLIPSEDATA:
			pEllipse = static_cast<CDXFellipse*>(pData2);
			// �ȉ~�v�Z���o���Ă���R�[�f�B���O
			pts2.reset();
			break;
		}
	}
}

void CDXFpolyline::CheckPolylineIntersection_SubLoop(const CDXFellipse* pData1, POSITION pos1)
{
	int				nResult;
	CDXFdata*		pData2;
	CDXFarc*		pArc;
	CDXFellipse*	pEllipse;
	float			dLong = pData1->GetLongLength(), dShort = pData1->GetShortLength(),
					q = pData1->GetLean();
	CPointF			pr1, pr2, pte2,
					pts1(pData1->GetNativePoint(0)),
					pte1(pData1->GetNativePoint(1)),
					ptc(pData1->GetCenter());
	optional<CPointF>	pts2;

	for ( POSITION pos2=pos1; pos2; ) {
		pData2 = GetNextVertex(pos2);
		switch ( pData2->GetType() ) {
		case DXFPOINTDATA:
			pte2 = pData2->GetNativePoint(0);
			if ( pts2 ) {
				tie(nResult, pr1, pr2) = ::CalcIntersectionPoint_LE(*pts2, pte2,
												ptc, dLong, dShort, q);
				if ( nResult>1 && pData1->IsRangeAngle(pr2) ) {
					SetPolyFlag(DXFPOLY_INTERSEC);
					pos2 = NULL;
					break;
				}
				if ( nResult>0 && pte1!=(*pts2) && pts1!=pte2 && pData1->IsRangeAngle(pr1) ) {
					SetPolyFlag(DXFPOLY_INTERSEC);
					pos2 = NULL;
					break;
				}
			}
			pts2 = pte2;
			break;

		case DXFARCDATA:
			pArc = static_cast<CDXFarc*>(pData2);
				// �ȉ~�v�Z���o���Ă���R�[�f�B���O
			pts2.reset();
			break;

		case DXFELLIPSEDATA:
			pEllipse = static_cast<CDXFellipse*>(pData2);
			pts2.reset();
			break;
		}
	}
}

void CDXFpolyline::DrawTuning(float f)
{
	PLIST_FOREACH(CDXFdata* pData, &m_ltVertex)
		pData->DrawTuning(f);
	END_FOREACH
}

void CDXFpolyline::Draw(CDC* pDC) const
{
#ifdef _DEBUGDRAW_DXF
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
			printf("CDXFpolyline::Draw() No.%03d: x=%d y=%d\n", nDbgCnt, ptDbg.x, ptDbg.y);
#endif
			pDC->LineTo( static_cast<CDXFpoint*>(pData)->GetDrawPoint() );
		}
		else {
			pData->Draw(pDC);
			// �I�_�����΂����݈ʒu�̈ړ�
			ASSERT( pos );
			pData = m_ltVertex.GetNext(pos);
			ASSERT( pData->GetType() == DXFPOINTDATA );
			pDC->MoveTo( static_cast<CDXFpoint*>(pData)->GetDrawPoint() );
		}
	}
}

float CDXFpolyline::OrgTuning(BOOL bCalc)
{
	// ������������Ԃɖ߂�
	m_dwPolyFlags = ((m_dwPolyFlags & DXFPOLY_SEQBAK)>>1) | (m_dwPolyFlags & ~DXFPOLY_SEQ);
	// �e�v�f�͒l�̓���ւ��⌴�_����̋����͕K�v�Ȃ��̂� OrgTuning(FALSE) �ŌĂ�
	PLIST_FOREACH(CDXFdata* pData, &m_ltVertex)
		pData->OrgTuning(FALSE);
	END_FOREACH
	// �ȉ� CDXFline �Ɠ���
	return CDXFline::OrgTuning(bCalc);
}

float CDXFpolyline::GetSelectPointGap(const CPointF& pt)
{
	float		dGap, dGapMin = FLT_MAX;
	POSITION	pos1, pos2;
	CPointF		pte;
	optional<CPointF>	pts;
	CDXFdata*		pData;
	DXFLARGV		dxfLine;
	dxfLine.pLayer = NULL;

	for ( pos1=m_ltVertex.GetHeadPosition(); (pos2=pos1); ) {
		pData = m_ltVertex.GetNext(pos1);
		if ( pData->GetType() == DXFPOINTDATA ) {
			pte = pData->GetNativePoint(0);
			if ( pts ) {
				dxfLine.s = *pts;
				dxfLine.e = pte;
				pData = new CDXFline(&dxfLine);
				dGap = pData->GetSelectPointGap(pt);
				delete	pData;
			}
			else
				pData = NULL;
			pts = pte;
		}
		else {
			dGap = pData->GetSelectPointGap(pt);
			pts.reset();
		}
		if ( pData && dGap<dGapMin ) {
			dGapMin = dGap;
			m_posSel = pos2;
		}
	}

	return dGapMin;
}

BOOL CDXFpolyline::GetDirectionArraw(const CPointF& ptClick, CPointF ptResult[][3])
{
	// ���ײݑS�̂̎n�_�I�_�ł́u�������ײ݁v�̖�󂪏����Ȃ��̂�
	// �Y���v�f(�د��_)�ɋ߂���޼ު�Ă̖����W�����߂�
	ASSERT( m_posSel );	// GetSelectPointGap() �ž�Ă���Ă���ʽ�

	POSITION	pos = m_posSel;
	BOOL		bResult;
	CDXFdata*	pData = GetNextVertex(pos);
	DXFLARGV	dxfLine;
	dxfLine.pLayer = NULL;

	if ( m_pArrawLine ) {
		delete	m_pArrawLine;
		m_pArrawLine = NULL;
	}

	if ( pData->GetType() == DXFPOINTDATA ) {
		dxfLine.s = pData->GetNativePoint(0);		// �n�_
		dxfLine.e = pos ? GetNextVertex(pos)->GetNativePoint(0) : GetFirstPoint();	// �I�_
		m_pArrawLine = new CDXFline(&dxfLine, DXFFLG_POLYCHILD);
		bResult = m_pArrawLine->GetDirectionArraw(ptClick, ptResult);
	}
	else
		bResult = pData->GetDirectionArraw(ptClick, ptResult);

	return bResult;
}

int CDXFpolyline::GetIntersectionPoint(const CDXFdata* pData1, CPointF pt[], BOOL bEdge/*=TRUE*/) const
{
	int			nResult = 0;
	CPointF		pte;
	optional<CPointF>	pts;
	DXFLARGV	dxfLine;
	dxfLine.pLayer = NULL;

	PLIST_FOREACH(CDXFdata* pData2, &m_ltVertex)
		if ( pData2->GetType() == DXFPOINTDATA ) {
			pte = pData2->GetNativePoint(0);
			if ( pts ) {
				// ����CDXFline�𐶐�
				dxfLine.s = *pts;
				dxfLine.e = pte;
				pData2 = new CDXFline(&dxfLine);
				// ���ꂼ��� pData1 �ŏ���
				nResult = pData1->GetIntersectionPoint(pData2, pt, bEdge);
				delete	pData2;
			}
			pts = pte;
		}
		else {
			// pData1, pData2 ������ DXFPOLYDATA �͂��蓾�Ȃ�
			nResult = pData1->GetIntersectionPoint(pData2, pt, bEdge);
			pts.reset();
		}
		if ( nResult )
			break;
	END_FOREACH

	return nResult;
}

int CDXFpolyline::GetIntersectionPoint(const CPointF& pts, const CPointF& pte, CPointF pt[], BOOL bEdge/*=TRUE*/) const
{
	DXFLARGV	dxfLine;
	// ����CDXFline�𐶐�
	dxfLine.pLayer	= NULL;
	dxfLine.s		= pts;
	dxfLine.e		= pte;
	CDXFdata* pData = new CDXFline(&dxfLine);
	int nResult = GetIntersectionPoint(pData, pt, bEdge);
	delete	pData;

	return nResult;
}

optional<CPointF>
CDXFpolyline::CalcOffsetIntersectionPoint(const CDXFdata* pNext, float r, BOOL bLeft) const
{
	optional<CPointF>	ptResult;

	// �������g�̍Ō�̵�޼ު�Ă��擾
	CDXFdata*	pData = GetTailObject();
	if ( pData ) {
		ptResult = pData->CalcOffsetIntersectionPoint(pNext, r, bLeft);
		if ( pData->GetType() == DXFLINEDATA )
			delete	pData;
	}

	return ptResult;
}

int CDXFpolyline::CheckIntersectionCircle(const CPointF& ptc, float r) const
{
	int			nResult = 0;
	CPointF		pte;
	optional<CPointF>	pts;
	;
	DXFLARGV	dxfLine;
	dxfLine.pLayer = NULL;

	// ��_����(nResult==2)��������܂ŌJ��Ԃ�
	PLIST_FOREACH(CDXFdata* pData, &m_ltVertex)
		if ( pData->GetType() == DXFPOINTDATA ) {
			pte = pData->GetNativePoint(0);
			if ( pts ) {
				dxfLine.s = *pts;
				dxfLine.e = pte;
				pData = new CDXFline(&dxfLine);
				nResult = pData->CheckIntersectionCircle(ptc, r);
				delete	pData;
			}
			pts = pte;
		}
		else {
			nResult = pData->CheckIntersectionCircle(ptc, r);
			pts.reset();
		}
		if ( nResult >= 2 )
			break;
	END_FOREACH

	return nResult;
}

//////////////////////////////////////////////////////////////////////
// �c�w�e�f�[�^��Spline�N���X
//////////////////////////////////////////////////////////////////////

CDXFspline::CDXFspline() : CDXFline(DXFSPLINEDATA, NULL, 2, 0)
{
}

#ifdef _DEBUG
void CDXFspline::DbgDump(void)
{
}
#endif

void CDXFspline::Serialize(CArchive& ar)
{
	CDXFdata::Serialize(ar);
	if ( ar.IsStoring() ) {
	}
	else {
	}
}

//////////////////////////////////////////////////////////////////////
// �c�w�e�f�[�^��Text�N���X
//////////////////////////////////////////////////////////////////////

CDXFtext::CDXFtext() : CDXFpoint(DXFTEXTDATA, NULL, 1, 0)
{
}

CDXFtext::CDXFtext(LPCDXFTARGV lpText, DWORD dwFlags) :
	CDXFpoint(DXFTEXTDATA, lpText->pLayer, 1, dwFlags)
{
	m_pt[0] = lpText->c;
	m_strValue = lpText->strValue;
	SetMaxRect();
}

CDXFtext::CDXFtext(CLayerData* pLayer, const CDXFtext* pData, LPCDXFBLOCK lpBlock, DWORD dwFlags) :
	CDXFpoint(DXFTEXTDATA, pLayer, 1, dwFlags)
{
	m_pt[0] = pData->GetNativePoint(0);
	if ( lpBlock->dwBlockFlg & DXFBLFLG_X )
		m_pt[0].x *= lpBlock->dMagni[NCA_X];
	if ( lpBlock->dwBlockFlg & DXFBLFLG_Y )
		m_pt[0].y *= lpBlock->dMagni[NCA_Y];
	if ( lpBlock->dwBlockFlg & DXFBLFLG_R )
		m_pt[0].RoundPoint(RAD(lpBlock->dRound));
	m_pt[0] += lpBlock->ptOrg;
	m_strValue = pData->GetStrValue();
	SetMaxRect();
}

#ifdef _DEBUG
void CDXFtext::DbgDump(void)
{
}
#endif

void CDXFtext::Serialize(CArchive& ar)
{
	CDXFdata::Serialize(ar);
	if ( ar.IsStoring() )
		ar << m_pt[0].x << m_pt[0].y << m_strValue;
	else {
		if ( g_dwCamVer < NCVCSERIALVERSION_3620 ) {
			CPointD	pt;
			ar >> pt.x >> pt.y >> m_strValue;
			m_pt[0] = pt;
		}
		else {
			ar >> m_pt[0].x >> m_pt[0].y >> m_strValue;
		}
		SetMaxRect();
	}
}

void CDXFtext::Draw(CDC* pDC) const
{
#ifdef _DEBUGDRAW_DXF
	printf("CDXFtext::Draw() (%d, %d)\n", m_ptDraw.x, m_ptDraw.y);
#endif
	pDC->TextOut(m_ptDraw.x, m_ptDraw.y, m_strValue);
	pDC->Ellipse(&m_rcDraw);
}

float CDXFtext::OrgTuning(BOOL/*=TRUE*/)
{
	CDXFpoint::OrgTuning(FALSE);	// �����v�Z�̕K�v�Ȃ�
	return HUGE_VALF;
}
