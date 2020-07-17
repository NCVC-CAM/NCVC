// NCdata.cpp: CNCdata �N���X�̃C���v�������e�[�V����
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NCVC.h"
#include "NCdata.h"
#include "ViewOption.h"

//#define	_DEBUGDRAW_NCD		// �`�揈����۸�
#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

using namespace boost;

extern	const	DWORD		g_dwSetValFlags[];	// NCDoc.cpp

// CalcRoundPoint() ���
// --- �̾�ĕ����̌���
static	int		CalcRoundPoint_OffsetFlag(const CPointD&, const CPointD&, int);
// --- �~���m�̓��O���a�v�Z
static	optional<double>	CalcRoundPoint_CircleInOut(const CPointD&, const CPointD&, int, int, double);

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
	for ( i=0; i<NCXYZ; i++ )
		m_pRead->m_ptValOrg[i] = m_ptValS[i] = m_ptValE[i] = m_nc.dValue[i];
	m_pt2D = m_ptValE.PointConvert();

	m_enType = NCDBASEDATA;
}

// �؍�(�`��)���ވȊO�̺ݽ�׸�
CNCdata::CNCdata(const CNCdata* pData, LPNCARGV lpArgv, const CPoint3D& ptOffset)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CNCdata", DBG_MAGENTA);
#endif
	int		i;

	Constracter(lpArgv);

	// ���W�w��̂Ȃ��ް��͑O��v�Z���W����擾
	for ( i=0; i<NCXYZ; i++ ) {
		// �w�肳��Ă��镪�������(XYZ�̂�)
		m_nc.dValue[i] = lpArgv->nc.dwValFlags & g_dwSetValFlags[i] ?
			lpArgv->nc.dValue[i] : pData->GetOriginalEndValue(i);
	}
	// ���W�l�ȊO(UVW�܂�)���w�肳��Ă��镪�͑��
	for ( ; i<VALUESIZE; i++ )
		m_nc.dValue[i] = lpArgv->nc.dwValFlags & g_dwSetValFlags[i] ?
			lpArgv->nc.dValue[i] : pData->GetValue(i);

	m_pRead->m_ptOffset = ptOffset;
	if ( m_nc.nGcode == 92 ) {
		// NCDoc�̵̾��(m_ptNcWorkOrg)�ŏ��������̂�G92�w��l���
		m_pRead->m_ptValOrg.SetPoint(GetValue(NCA_X), GetValue(NCA_Y), GetValue(NCA_Z));
		m_ptValS = m_ptValE = m_pRead->m_ptValOrg + ptOffset;
		m_pt2D = m_ptValE.PointConvert();
	}
	else {
		if ( lpArgv->nc.dwValFlags & (NCD_X|NCD_Y|NCD_Z) ) {
			m_pRead->m_ptValOrg.SetPoint(GetValue(NCA_X), GetValue(NCA_Y), GetValue(NCA_Z));
			m_ptValS = pData->GetEndPoint();
			m_ptValE = m_pRead->m_ptValOrg + ptOffset;
			if ( lpArgv->g68.bG68 )
				CalcG68Round(&(lpArgv->g68), m_ptValE);
			m_pt2D = m_ptValE.PointConvert();
		}
		else {
			m_pRead->m_ptValOrg = pData->GetOriginalEndPoint();
			m_ptValS = m_ptValE = pData->GetEndPoint();
			m_pt2D   = pData->Get2DPoint();
		}
	}

	m_enType = NCDBASEDATA;

#ifdef _DEBUG_DUMP
	DbgDump();
	Dbg_sep();
#endif
}

// �h���׽�p�ݽ�׸�
CNCdata::CNCdata
	(ENNCDTYPE enType, const CNCdata* pData, LPNCARGV lpArgv, const CPoint3D& ptOffset)
{
	int		i;

	Constracter(lpArgv);

	// ���W�w��̂Ȃ��ް��͑O�񏃐����W������
	for ( i=0; i<NCXYZ; i++ ) {
		// �w�肳��Ă��镪�������(XYZ�̂�)
		if ( lpArgv->nc.dwValFlags & g_dwSetValFlags[i] ) {
			m_nc.dValue[i] = lpArgv->nc.dValue[i];
			if ( !lpArgv->bAbs )		// �ݸ����ٕ␳
				m_nc.dValue[i] += pData->GetOriginalEndValue(i);	// �ؼ��ْl�ŉ��Z
		}
		else
			m_nc.dValue[i] = pData->GetOriginalEndValue(i);
	}
	// UVW���W�l
	for ( ; i<NCXYZ*2; i++ ) {
		if ( lpArgv->bAbs )
			m_nc.dValue[i]  = lpArgv->nc.dwValFlags & g_dwSetValFlags[i] ?
				lpArgv->nc.dValue[i] : pData->GetValue(i);
		else
			m_nc.dValue[i] += lpArgv->nc.dwValFlags & g_dwSetValFlags[i] ?
				lpArgv->nc.dValue[i] : 0.0;
	}
	// ��L�ȊO
	for ( ; i<VALUESIZE; i++ )
		m_nc.dValue[i] = lpArgv->nc.dwValFlags & g_dwSetValFlags[i] ?
			lpArgv->nc.dValue[i] : 0.0;
	// ---------------------------------------------------------------
	// m_ptValS, m_ptValE, m_ptValOrg, m_pt2D �͔h���׽�ő��
	// m_nc.dLength �� TH_Cuttime.cpp �ɂăZ�b�g
	// ---------------------------------------------------------------
	m_enType = enType;
	m_pRead->m_ptOffset = ptOffset;
}

// �����p�ݽ�׸�
CNCdata::CNCdata(const CNCdata* pData)
{
	int		i;
	m_enType = pData->GetType();
	m_nc.nErrorCode	= pData->GetNCObjErrorCode();
	m_nc.nLine		= pData->GetBlockLineNo();
	m_nc.nGtype		= pData->GetGtype();
	m_nc.nGcode		= pData->GetGcode();
	m_nc.enPlane	= pData->GetPlane();
	m_nc.dLength	= pData->GetCutLength();
	m_nc.dwValFlags	= pData->GetValFlags();
	for ( i=0; i<VALUESIZE; i++ )
		m_nc.dValue[i] = pData->GetValue(i);
	for ( i=0; i<NCXYZ; i++ )
		m_dMove[i] = pData->GetMove(i);
	m_dFeed		= pData->GetFeed();
	m_dEndmill	= pData->GetEndmill();
	m_nEndmillType	= pData->GetEndmillType();
	m_ptValS = pData->GetStartPoint();
	m_ptValE = pData->GetEndPoint();
	m_pRead = new CNCread;
	m_pRead->m_ptOffset = pData->GetOffsetPoint();
	m_pRead->m_ptValOrg = pData->GetOriginalEndPoint();
	memcpy(&(m_pRead->m_g68),   &(pData->GetReadData()->m_g68),   sizeof(G68ROUND));
	memcpy(&(m_pRead->m_taper), &(pData->GetReadData()->m_taper), sizeof(TAPER));
	m_pWireObj = NULL;
}

CNCdata::~CNCdata()
{
	for ( int i=0; i<m_obCdata.GetSize(); i++ )
		delete	m_obCdata[i];
	if ( m_pRead )
		delete	m_pRead;
	if ( m_pWireObj )
		delete	m_pWireObj;
}

void CNCdata::DeleteReadData(void)
{
	for ( int i=0; i<m_obCdata.GetSize(); i++ )
		m_obCdata[i]->DeleteReadData();
	if ( m_pRead ) {
		delete	m_pRead;
		m_pRead = NULL;
	}
	if ( m_pWireObj )
		m_pWireObj->DeleteReadData();
}

CPointD CNCdata::GetPlaneValue(const CPoint3D& ptVal) const
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
	}
	return pt;
}

void CNCdata::SetPlaneValue(const CPointD& pt, CPoint3D& ptResult)
{
	switch ( GetPlane() ) {
	case XY_PLANE:
		ptResult.x = pt.x;
		ptResult.y = pt.y;
		break;
	case XZ_PLANE:
		ptResult.x = pt.x;
		ptResult.z = pt.y;
		break;
	case YZ_PLANE:
		ptResult.y = pt.x;
		ptResult.z = pt.y;
		break;
	}
}

CPointD	CNCdata::GetPlaneValueOrg(const CPoint3D& pt1, const CPoint3D& pt2) const
{
	CPointD	pt;
	switch ( GetPlane() ) {
	case XY_PLANE:
		pt = pt1.GetXY() - pt2.GetXY();
		break;
	case XZ_PLANE:
		pt = pt1.GetXZ() - pt2.GetXZ();
		break;
	case YZ_PLANE:
		pt = pt1.GetYZ() - pt2.GetYZ();
		break;
	}
	return pt;
}

void CNCdata::CalcG68Round(LPG68ROUND lpG68, CPoint3D& ptResult)
{
	CPoint3D	ptOrg(lpG68->dOrg[NCA_X], lpG68->dOrg[NCA_Y], lpG68->dOrg[NCA_Z]);
	CPointD		pt(GetPlaneValueOrg(ptResult, ptOrg));
	pt.RoundPoint(lpG68->dRound);
	SetPlaneValue(pt, ptResult);
	ptResult += ptOrg;
}

CNCdata* CNCdata::NC_CopyObject(void)
{
	CNCdata*	pData;

	switch ( GetType() ) {
	case NCDLINEDATA:
		pData = new CNCline(this);
		break;
	case NCDARCDATA:
		pData = new CNCcircle(this);
		break;
	default:
		pData = NULL;	// ���͍H��a�␳�ɖ��֌W(�װ)
	}

	return pData;
}

void CNCdata::DrawTuning(double f)
{
	for ( int i=0; i<m_obCdata.GetSize(); i++ )
		m_obCdata[i]->DrawTuning(f);
	if ( m_pWireObj )
		m_pWireObj->DrawTuning(f);
}

void CNCdata::DrawTuningXY(double f)
{
	for ( int i=0; i<m_obCdata.GetSize(); i++ )
		m_obCdata[i]->DrawTuningXY(f);
	if ( m_pWireObj )
		m_pWireObj->DrawTuningXY(f);
}

void CNCdata::DrawTuningXZ(double f)
{
	for ( int i=0; i<m_obCdata.GetSize(); i++ )
		m_obCdata[i]->DrawTuningXZ(f);
	if ( m_pWireObj )
		m_pWireObj->DrawTuningXZ(f);
}

void CNCdata::DrawTuningYZ(double f)
{
	for ( int i=0; i<m_obCdata.GetSize(); i++ )
		m_obCdata[i]->DrawTuningYZ(f);
	if ( m_pWireObj )
		m_pWireObj->DrawTuningYZ(f);
}

void CNCdata::Draw(CDC* pDC, BOOL bSelect) const
{
	for ( int i=0; i<m_obCdata.GetSize(); i++ )
		m_obCdata[i]->Draw(pDC, bSelect);
}

void CNCdata::DrawXY(CDC* pDC, BOOL bSelect) const
{
	for ( int i=0; i<m_obCdata.GetSize(); i++ )
		m_obCdata[i]->DrawXY(pDC, bSelect);
}

void CNCdata::DrawXZ(CDC* pDC, BOOL bSelect) const
{
	for ( int i=0; i<m_obCdata.GetSize(); i++ )
		m_obCdata[i]->DrawXZ(pDC, bSelect);
}

void CNCdata::DrawYZ(CDC* pDC, BOOL bSelect) const
{
	for ( int i=0; i<m_obCdata.GetSize(); i++ )
		m_obCdata[i]->DrawYZ(pDC, bSelect);
}

void CNCdata::DrawWire(CDC* pDC, BOOL bSelect) const
{
}

void CNCdata::DrawWireXY(CDC* pDC, BOOL bSelect) const
{
}

void CNCdata::DrawWireXZ(CDC* pDC, BOOL bSelect) const
{
}

void CNCdata::DrawWireYZ(CDC* pDC, BOOL bSelect) const
{
}

#ifdef _DEBUG_DUMP
void CNCdata::DbgDump(void)
{
	CMagaDbg	dbg("CNCdata", DBG_MAGENTA);
	extern	LPCTSTR	g_szGdelimiter;	// "GSMOF" from NCDoc.cpp
	extern	LPCTSTR	g_szNdelimiter; // "XYZUVWIJKRPLDH";

	CString	strBuf, strTmp;
	if ( GetGtype()<0 || GetGtype()>GTYPESIZE )
		strBuf.Format("%s%d: ", "NO_TYPE:", GetGcode());
	else
		strBuf.Format("%c%02d: ", g_szGdelimiter[GetGtype()], GetGcode());
	for ( int i=0; i<VALUESIZE; i++ ) {
		if ( GetValFlags() & g_dwSetValFlags[i] ) {
			strTmp.Format("%c%.3f", g_szNdelimiter[i], GetValue(i));
			strBuf += strTmp;
		}
	}
	dbg.printf("%s", strBuf);
}
#endif

//////////////////////////////////////////////////////////////////////
// CNCline �N���X
//////////////////////////////////////////////////////////////////////

CNCline::CNCline(const CNCdata* pData, LPNCARGV lpArgv, const CPoint3D& ptOffset) :
	CNCdata(NCDLINEDATA, pData, lpArgv, ptOffset)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CNCline", DBG_MAGENTA);
#endif
	// �`��n�_��O��̌v�Z�l����擾
	m_ptValS = pData->GetEndPoint();
	m_pt2Ds  = pData->Get2DPoint();
	// �ŏI���W(==�w����W)���
	m_pRead->m_ptValOrg.SetPoint(GetValue(NCA_X), GetValue(NCA_Y), GetValue(NCA_Z));
	m_ptValE = m_pRead->m_ptValOrg + ptOffset;
	// ���W��]
	if ( lpArgv->g68.bG68 )
		CalcG68Round(&(lpArgv->g68), m_ptValE);
	// �`��I�_���v�Z���ۑ�
	m_pt2D = m_ptValE.PointConvert();

#ifdef _DEBUG_DUMP
	DbgDump();
	Dbg_sep();
#endif
}

CNCline::CNCline(const CNCdata* pData) : CNCdata(pData)
{
	m_pt2Ds	= m_ptValS.PointConvert();
	m_pt2D	= m_ptValE.PointConvert();
}

double CNCline::SetCalcLength(void)
{
	CPoint3D	pt;

	if ( m_obCdata.IsEmpty() ) {
		// �e�����Ƃ̈ړ���(�����莞�Ԃ̌v�Z�p)
		pt = m_ptValE - m_ptValS;
		m_dMove[NCA_X] = fabs(pt.x);
		m_dMove[NCA_Y] = fabs(pt.y);
		m_dMove[NCA_Z] = fabs(pt.z);
		// �ړ���
		m_nc.dLength = pt.hypot();
	}
	else {
		CNCdata*	pData;
		m_nc.dLength = m_dMove[NCA_X] = m_dMove[NCA_Y] = m_dMove[NCA_Z] = 0.0;
		// �e�␳�v�f�̍��v
		for ( int i=0; i<m_obCdata.GetSize(); i++ ) {
			pData = m_obCdata[i];
			pt = pData->GetEndPoint() - pData->GetStartPoint();
			// DrawBottomFace() �ňړ��ʂ��K�v�Ȃ���
			// �␳�ް��ɑ΂��Ă� m_dMove ���
			m_dMove[NCA_X] += pData->SetMove(NCA_X, fabs(pt.x));
			m_dMove[NCA_Y] += pData->SetMove(NCA_Y, fabs(pt.y));
			m_dMove[NCA_Z] += pData->SetMove(NCA_Z, fabs(pt.z));
			m_nc.dLength += pt.hypot();
		}
	}

	return m_nc.dLength;
}

void CNCline::DrawTuning(double f)
{
	m_ptDrawS[NCDRAWVIEW_XYZ] = m_pt2Ds * f;
	m_ptDrawE[NCDRAWVIEW_XYZ] = m_pt2D  * f;
	CNCdata::DrawTuning(f);
}

void CNCline::DrawTuningXY(double f)
{
	m_ptDrawS[NCDRAWVIEW_XY] = m_ptValS.GetXY() * f;
	m_ptDrawE[NCDRAWVIEW_XY] = m_ptValE.GetXY() * f;
	CNCdata::DrawTuningXY(f);
}

void CNCline::DrawTuningXZ(double f)
{
	m_ptDrawS[NCDRAWVIEW_XZ] = m_ptValS.GetXZ() * f;
	m_ptDrawE[NCDRAWVIEW_XZ] = m_ptValE.GetXZ() * f;
	CNCdata::DrawTuningXZ(f);
}

void CNCline::DrawTuningYZ(double f)
{
	m_ptDrawS[NCDRAWVIEW_YZ] = m_ptValS.GetYZ() * f;
	m_ptDrawE[NCDRAWVIEW_YZ] = m_ptValE.GetYZ() * f;
	CNCdata::DrawTuningYZ(f);
}

void CNCline::Draw(CDC* pDC, BOOL bSelect) const
{
#ifdef _DEBUGDRAW_NCD
	CMagaDbg	dbg("CNCline::Draw()", DBG_RED);
	dbg.printf("Line=%d", GetBlockLineNo()+1);
#endif
	if ( m_obCdata.IsEmpty() ||
			AfxGetNCVCApp()->GetViewOption()->GetNCViewFlg(NCVIEWFLG_DRAWREVISE) )
		DrawLine(NCDRAWVIEW_XYZ, pDC, bSelect);
	CNCdata::Draw(pDC, bSelect);
}

void CNCline::DrawXY(CDC* pDC, BOOL bSelect) const
{
#ifdef _DEBUGDRAW_NCD
	CMagaDbg	dbg("CNCline::DrawXY()", DBG_RED);
	dbg.printf("Line=%d", GetBlockLineNo()+1);
#endif
	if ( m_obCdata.IsEmpty() ||
			AfxGetNCVCApp()->GetViewOption()->GetNCViewFlg(NCVIEWFLG_DRAWREVISE) )
		DrawLine(NCDRAWVIEW_XY, pDC, bSelect);
	CNCdata::DrawXY(pDC, bSelect);
}

void CNCline::DrawXZ(CDC* pDC, BOOL bSelect) const
{
#ifdef _DEBUGDRAW_NCD
	CMagaDbg	dbg("CNCline::DrawXZ()", DBG_RED);
	dbg.printf("Line=%d", GetBlockLineNo()+1);
#endif
	if ( m_obCdata.IsEmpty() ||
			AfxGetNCVCApp()->GetViewOption()->GetNCViewFlg(NCVIEWFLG_DRAWREVISE) )
		DrawLine(NCDRAWVIEW_XZ, pDC, bSelect);
	CNCdata::DrawXZ(pDC, bSelect);
}

void CNCline::DrawYZ(CDC* pDC, BOOL bSelect) const
{
#ifdef _DEBUGDRAW_NCD
	CMagaDbg	dbg("CNCline::DrawYZ()", DBG_RED);
	dbg.printf("Line=%d", GetBlockLineNo()+1);
#endif
	if ( m_obCdata.IsEmpty() ||
			AfxGetNCVCApp()->GetViewOption()->GetNCViewFlg(NCVIEWFLG_DRAWREVISE) )
		DrawLine(NCDRAWVIEW_YZ, pDC, bSelect);
	CNCdata::DrawYZ(pDC, bSelect);
}

void CNCline::DrawWire(CDC* pDC, BOOL bSelect) const
{
	DrawWireLine(NCDRAWVIEW_XYZ, pDC, bSelect);
}

void CNCline::DrawWireXY(CDC* pDC, BOOL bSelect) const
{
	DrawWireLine(NCDRAWVIEW_XY, pDC, bSelect);
}

void CNCline::DrawWireXZ(CDC* pDC, BOOL bSelect) const
{
	DrawWireLine(NCDRAWVIEW_XZ, pDC, bSelect);
}

void CNCline::DrawWireYZ(CDC* pDC, BOOL bSelect) const
{
	DrawWireLine(NCDRAWVIEW_YZ, pDC, bSelect);
}

void CNCline::DrawLine(EN_NCDRAWVIEW enDraw, CDC* pDC, BOOL bSelect) const
{
	CPen*	pOldPen;
	if ( bSelect )
		pOldPen = AfxGetNCVCMainWnd()->GetPenCom(COMPEN_SEL);
	else
		pOldPen = AfxGetNCVCMainWnd()->GetPenNC(
			m_obCdata.IsEmpty() ? GetPenType() : NCPEN_CORRECT );
	pOldPen = pDC->SelectObject(pOldPen);
	pDC->MoveTo(m_ptDrawS[enDraw]);
	pDC->LineTo(m_ptDrawE[enDraw]);
	pDC->SelectObject(pOldPen);
}

void CNCline::DrawWireLine(EN_NCDRAWVIEW enDraw, CDC* pDC, BOOL bSelect) const
{
	CPen*	pOldPen;
	if ( bSelect )
		pOldPen = AfxGetNCVCMainWnd()->GetPenCom(COMPEN_SEL);
	else
		pOldPen = AfxGetNCVCMainWnd()->GetPenNC(
			m_obCdata.IsEmpty() ? GetPenType() : NCPEN_CORRECT );
	pOldPen = pDC->SelectObject(pOldPen);
	// XY
	pDC->MoveTo(m_ptDrawS[enDraw]);
	pDC->LineTo(m_ptDrawE[enDraw]);
	// UV
	if ( m_pWireObj ) {
		CPoint	pts(static_cast<CNCline*>(m_pWireObj)->GetDrawStartPoint(enDraw)),
				pte(static_cast<CNCline*>(m_pWireObj)->GetDrawEndPoint(enDraw));
		pDC->MoveTo(pts);
		pDC->LineTo(pte);
		// XY��UV�̐ڑ�
		pDC->MoveTo(m_ptDrawS[enDraw]);
		pDC->LineTo(pts);
		pDC->MoveTo(m_ptDrawE[enDraw]);
		pDC->LineTo(pte);
	}
	pDC->SelectObject(pOldPen);
}

tuple<BOOL, CPointD, double, double> CNCline::CalcRoundPoint
	(const CNCdata* pNext, double r) const
{
	BOOL	bResult = FALSE;
	double	rr1, rr2;
	// �Q���̌�_(���g�̏I�_)�����_�ɂȂ�悤�ɕ␳
	CPointD	pt, pts( GetPlaneValueOrg(m_ptValS, m_ptValE) );

	if ( pNext->GetType() == NCDARCDATA ) {
		const CNCcircle* pCircle = static_cast<const CNCcircle *>(pNext);
		CPointD	pto( GetPlaneValueOrg(pCircle->GetOrg(), m_ptValE) );
		// �̾�ĕ���������
		int nOffset = CalcRoundPoint_OffsetFlag(pts, pto, pCircle->GetG23());
		if ( nOffset != 0 ) {
			// �̾�ĕ����s�ړ���������_�����߂�
			double	rr, xa, ya, rn = fabs(pCircle->GetR());
			optional<CPointD> ptResult = ::CalcOffsetIntersectionPoint_LC(pts, pto, rn, r,
							pCircle->GetG23(), nOffset>0);
			if ( ptResult ) {
				pt = *ptResult;
				// �ʎ��ɑ�������C�l�̌v�Z
				rr1 = sqrt(pt.x*pt.x + pt.y*pt.y - r*r);
				if ( nOffset > 0 )
					nOffset = pCircle->GetG23()==0 ? 1 : -1;
				else
					nOffset = pCircle->GetG23()==0 ? -1 : 1;
				rr = rn + r*nOffset;
				if ( nOffset > 0 ) {
					// �����_(+r)
					xa = (pt.x*rn+pto.x*r) / rr;
					ya = (pt.y*rn+pto.y*r) / rr;
				}
				else {
					// �O���_(-r)
					xa = (pt.x*rn-pto.x*r) / rr;
					ya = (pt.y*rn-pto.y*r) / rr;
				}
				rr2 = _hypot(xa, ya);
				bResult = TRUE;
			}
		}
	}
	else {
		CPointD	pte( GetPlaneValueOrg(pNext->GetEndPoint(), m_ptValE) );
		// pts �� X����ɉ�]
		double	q = atan2(pts.y, pts.x);
		pte.RoundPoint(-q);
		// �Q�̐��̊p�x���Q
		double	p = atan2(pte.y, pte.x) / 2.0,
				pp = fabs(p);
		if ( pp < RAD(90) ) {
			pt.x = rr1 = rr2 = r / tan(pp);	// �ʎ��ɑ�������C�l�͉�]�����O��X���W�Ɠ���
			pt.y = _copysign(r, p);			// y(����) = r�A�����͊p�x�ɂ��
			// ��]�𕜌�
			pt.RoundPoint(q);
			bResult = TRUE;
		}
	}

	// ���_�␳
	if ( bResult )
		pt += GetPlaneValue(m_ptValE);

	return make_tuple(bResult, pt, rr1, rr2);
}

optional<CPointD> CNCline::SetChamferingPoint(BOOL bStart, double c)
{
	// ����������Ȃ��Ƃ��ʹװ
	if ( c >= SetCalcLength() )
		return optional<CPointD>();

	CPointD		pt, pto, pte;
	CPoint3D&	ptValS = bStart ? m_ptValS : m_ptValE;	// ���������̂ŎQ�ƌ^(�ʖ�)
	CPoint3D&	ptValE = bStart ? m_ptValE : m_ptValS;
	
	pto = GetPlaneValue(ptValS);
	pte = GetPlaneValue(ptValE);

	// pto �𒆐S�Ƃ����~�� pte �̌�_
	pt = ::CalcIntersectionPoint_TC(pto, c, pte);

	// �������g�̓_���X�V
	SetPlaneValue(pt, ptValS);

	if ( bStart )
		m_pt2Ds = m_ptValS.PointConvert();
	else {
		m_pRead->m_ptValOrg = m_ptValE;
		if ( m_pRead->m_g68.bG68 ) {
			// m_ptValE ��G68��]�ςݍ��W�̂��߉�]�����ɖ߂��ĵ̾�Č��Z
			m_pRead->m_g68.dRound = -m_pRead->m_g68.dRound;
			CalcG68Round(&(m_pRead->m_g68), m_pRead->m_ptValOrg);
			m_pRead->m_g68.dRound = -m_pRead->m_g68.dRound;
		}
		m_pRead->m_ptValOrg -= m_pRead->m_ptOffset;	// m_ptValE��G68�̏ꍇ�A���������Ȃ�
		m_pt2D = m_ptValE.PointConvert();
	}

	return pt;
}

double CNCline::CalcBetweenAngle(const CNCdata* pNext) const
{
	// �Q���̌�_(���g�̏I�_)�����_�ɂȂ�悤�ɕ␳
	CPointD		pt1( GetPlaneValueOrg(m_ptValS, m_ptValE) ), pt2;

	if ( pNext->GetType() == NCDARCDATA ) {
		const CNCcircle* pCircle = static_cast<const CNCcircle *>(pNext);
		// ���̵�޼ު�Ă��~�ʂȂ璆�S��Ă�
		CPointD	pt( GetPlaneValueOrg(pCircle->GetOrg(), m_ptValE) );
		// �n�_�̐ڐ��v�Z
		int k = pCircle->CalcOffsetSign();
		pt2.x = -pt.y*k;	// G02:+90��
		pt2.y =  pt.x*k;	// G03:-90��
	}
	else {
		pt2 = GetPlaneValueOrg(pNext->GetEndPoint(), m_ptValE);
	}
	
	// �Q��(�܂��͉~�ʂ̐ڐ�)���Ȃ��p�x�����߂�
	return ::CalcBetweenAngle(pt1, pt2);
}

int CNCline::CalcOffsetSign(void) const
{
	// �n�_�����_�ɐi�s�����̊p�x���v�Z
	return ::CalcOffsetSign( GetPlaneValueOrg(m_ptValE, m_ptValS) );
}

optional<CPointD> CNCline::CalcPerpendicularPoint
	(ENPOINTORDER enPoint, double r, int nSign) const
{
	CPoint3D	pts, pte;
	if ( enPoint == STARTPOINT ) {
		pts = m_ptValS;
		pte = m_ptValE;
	}
	else {
		pts = m_ptValE;
		pte = m_ptValS;
		nSign = -nSign;		// �I�_�ł�-90��
	}
	// ���̌X�����v�Z����90����]
	CPointD	pt( GetPlaneValueOrg(pte, pts) );
	double	q = atan2(pt.y, pt.x);
	CPointD	pt1(r*cos(q), r*sin(q));
	CPointD	pt2(-pt1.y*nSign, pt1.x*nSign);
	pt2 += GetPlaneValue(pts);

	return pt2;
}

optional<CPointD> CNCline::CalcOffsetIntersectionPoint
	(const CNCdata* pNext, double r, BOOL bLeft) const
{
	optional<CPointD>	ptResult;
	// �Q���̌�_(���g�̏I�_)�����_�ɂȂ�悤�ɕ␳
	CPointD		pt, pt1( GetPlaneValueOrg(m_ptValS, m_ptValE) ), pt2;

	// �̾�ĕ����s�ړ���������_�����߂�
	if ( pNext->GetType() == NCDARCDATA ) {
		const CNCcircle* pCircle = static_cast<const CNCcircle *>(pNext);
		pt2 = GetPlaneValueOrg(pCircle->GetOrg(), m_ptValE);
		ptResult = ::CalcOffsetIntersectionPoint_LC(pt1, pt2, fabs(pCircle->GetR()), r,
						pCircle->GetG23(), bLeft);
	}
	else {
		pt2 = GetPlaneValueOrg(pNext->GetEndPoint(), m_ptValE);
		ptResult = ::CalcOffsetIntersectionPoint_LL(pt1, pt2, r, bLeft);
	}

	// ���_�␳
	if ( ptResult ) {
		pt = *ptResult + GetPlaneValue(m_ptValE);
		return pt;
	}

	return optional<CPointD>();
}

optional<CPointD> CNCline::CalcOffsetIntersectionPoint2
	(const CNCdata* pNext, double r, BOOL bLeft) const
{
	// �Q���̌�_(���g�̏I�_)�����_�ɂȂ�悤�ɕ␳
	CPointD		pt, pt1( GetPlaneValueOrg(m_ptValS, m_ptValE) ), pt2;

	// �̾�ĕ����s�ړ���������_�����߂�
	if ( pNext->GetType() == NCDARCDATA ) {
		// �~�ʂ͎n�_�ڐ��Ƃ̵̾�Č�_�v�Z
		const CNCcircle* pCircle = static_cast<const CNCcircle *>(pNext);
		pt = GetPlaneValueOrg(pCircle->GetOrg(), m_ptValE);
		int k = pCircle->CalcOffsetSign();
		pt2.x = -pt.y * k;
		pt2.y =  pt.x * k;
	}
	else {
		pt2 = GetPlaneValueOrg(pNext->GetEndPoint(), m_ptValE);
	}

	// �������m�̵̾�Č�_�v�Z
	optional<CPointD> ptResult = ::CalcOffsetIntersectionPoint_LL(pt1, pt2, r, bLeft);
	// ���_�␳
	if ( ptResult ) {
		pt = *ptResult + GetPlaneValue(m_ptValE);
		return pt;
	}
	return ptResult;
}

void CNCline::SetCorrectPoint(ENPOINTORDER enPoint, const CPointD& ptSrc, double)
{
	CPoint3D&	ptVal    = enPoint==STARTPOINT ? m_ptValS : m_ptValE;	// �Q�ƌ^
	CPointD&	ptResult = enPoint==STARTPOINT ? m_pt2Ds  : m_pt2D;

	SetPlaneValue(ptSrc, ptVal);
	ptResult = ptVal.PointConvert();

	if ( enPoint == ENDPOINT ) {
		ASSERT( m_pRead );
		SetPlaneValue(ptSrc, m_pRead->m_ptValOrg);
	}
}

//////////////////////////////////////////////////////////////////////
// CNCcycle �N���X
//////////////////////////////////////////////////////////////////////

CNCcycle::CNCcycle
	(const CNCdata* pData, LPNCARGV lpArgv, const CPoint3D& ptOffset, BOOL bL0Cycle) :
		CNCline(NCDCYCLEDATA, pData, lpArgv, ptOffset)
{
/*
	Z, R, P �l�́CTH_NCRead.cpp �ł���Ԃ��Ă��邱�Ƃɒ���
*/
#ifdef _DEBUG
	CMagaDbg	dbg("CNCcycle", DBG_MAGENTA);
#endif
	double	dx, dy,	dox, doy,	// ����ʂ̈ړ�����
			dR, dI,				// R�_���W, �Ƽ�ٍ��W
			dRLength, dZLength;		// �ړ����C�؍풷
	CPoint3D	pt;
	int		i, x, y, z,
			nH, nV;		// �c���̌J��Ԃ���

	// ������
	for ( i=0; i<SIZEOF(m_Cycle); m_Cycle[i++]=NULL );
	m_Cycle3D = NULL;

	// ����ʂɂ����W�ݒ�
	switch ( GetPlane() ) {
	case XY_PLANE:
		x = NCA_X;
		y = NCA_Y;
		z = NCA_Z;
		break;
	case XZ_PLANE:
		x = NCA_X;
		y = NCA_Z;
		z = NCA_Y;
		break;
	case YZ_PLANE:
		x = NCA_Y;
		y = NCA_Z;
		z = NCA_X;
		break;
	}

	// �`��n�_��O��̌v�Z�l����擾
	m_pt2Ds  = pData->Get2DPoint();
	// �ȍ~�̾�Ė����Ōv�Z�I
	m_ptValS = pData->GetEndPoint() - pData->GetOffsetPoint();
	// �c�J��Ԃ����擾
	if ( GetValFlags() & NCD_K )
		nV = max(0, (int)GetValue(NCA_K));
	else if ( GetValFlags() & NCD_L )
		nV = max(0, (int)GetValue(NCA_L));
	else
		nV = 1;
	// ���A���W(�O��̵�޼ު�Ă��Œ軲�ق��ǂ���)
	m_dInitial = pData->GetType()!=NCDCYCLEDATA ? m_ptValS[z] :
				(static_cast<const CNCcycle*>(pData)->GetInitialValue() - pData->GetOffsetPoint()[z]);
	// �ݸ����ٕ␳(R���W���ް��׽�ō��W�␳�̑ΏۊO)
	if ( lpArgv->bAbs ) {
		dR = GetValFlags() & NCD_R ? GetValue(NCA_R) : m_ptValS[z];
		m_nDrawCnt = nH = min(1, nV);	// ��޿ح�ĂȂ牡�ւ�(0 or 1)��̂�
	}
	else {
		dR = GetValFlags() & NCD_R ? m_dInitial + GetValue(NCA_R) : m_dInitial;
		// !!! Z�l��R�_����̲ݸ���Ăɕ␳ !!!
		if ( GetValFlags() & g_dwSetValFlags[z] )
			m_nc.dValue[z] = dR + lpArgv->nc.dValue[z];
		m_nDrawCnt = nH = nV;	// �ݸ����قȂ牡�ւ��J��Ԃ�
	}
	dI = lpArgv->bG98 ? m_dInitial : dR;

	// �J��Ԃ�����ۂȂ�
	if ( m_nDrawCnt <= 0 ) {
		if ( bL0Cycle ) {
			// �ړ��ް������쐬
			nH = 1;
			dI = dR = m_ptValS[z];
		}
		else {
			// �ȍ~�̌v�Z�͕s�v
			for ( i=0; i<NCXYZ; m_dMove[i++]=0.0 );
			m_dDwell = 0.0;
			m_nc.dLength = m_dCycleMove = 0.0;
			m_ptValI = m_ptValR = m_ptValE = m_ptValS = pData->GetEndPoint();
			m_pRead->m_ptValOrg = pData->GetOriginalEndPoint();
			m_dInitial += ptOffset[z];
			m_pt2D = m_pt2Ds;
			return;
		}
	}

	m_pRead->m_ptValOrg.SetPoint(GetValue(NCA_X), GetValue(NCA_Y), GetValue(NCA_Z));
	m_ptValE = m_pRead->m_ptValOrg;
	pt = pData->GetOriginalEndPoint();
	// ���W��]
	if ( lpArgv->g68.bG68 )
		CalcG68Round(&(lpArgv->g68), m_ptValE);
	// �e�����Ƃ̈ړ����v�Z
	dx = m_ptValE[x] - m_ptValS[x];		dox = m_pRead->m_ptValOrg[x] - pt[x];
	dy = m_ptValE[y] - m_ptValS[y];		doy = m_pRead->m_ptValOrg[y] - pt[y];
	dRLength = fabs(dI - dR);
	dZLength = fabs(dR - m_ptValE[z]);
	m_dMove[x] = fabs(dx) * nH;
	m_dMove[y] = fabs(dy) * nH;
	m_dMove[z] = fabs(m_ptValS[z] - dR);	// ���񉺍~��
	m_dMove[z] += dRLength * (nV-1);
	// �ړ����v�Z
	m_dCycleMove = _hypot(m_dMove[x], m_dMove[y]);
	m_dCycleMove += m_dMove[z];
	// �؍풷
	m_nc.dLength = dZLength * nV;
	// �e���W�l�̐ݒ�
	switch ( GetPlane() ) {
	case XY_PLANE:
		m_ptValI.SetPoint(m_ptValE.x, m_ptValE.y, m_ptValS.z);	// �`��p
		m_ptValR.SetPoint(m_ptValE.x, m_ptValE.y, dR);
		m_ptValE.SetPoint(m_ptValS.x+dx*nH, m_ptValS.y+dy*nH, dI);
		m_pRead->m_ptValOrg.SetPoint(pt.x+dox*nH, pt.y+doy*nH, dI);
		break;
	case XZ_PLANE:
		m_ptValI.SetPoint(m_ptValE.x, m_ptValS.y, m_ptValE.z);
		m_ptValR.SetPoint(m_ptValE.x, dR, m_ptValE.z);
		m_ptValE.SetPoint(m_ptValS.x+dx*nH, dI, m_ptValS.z+dy*nH);
		m_pRead->m_ptValOrg.SetPoint(pt.x+dox*nH, dI, pt.z+doy*nH);
		break;
	case YZ_PLANE:
		m_ptValI.SetPoint(m_ptValS.x, m_ptValE.y, m_ptValE.z);
		m_ptValR.SetPoint(dR, m_ptValE.y, m_ptValE.z);
		m_ptValE.SetPoint(dI, m_ptValS.y+dx*nH, m_ptValS.z+dy*nH);
		m_pRead->m_ptValOrg.SetPoint(dI, pt.y+dox*nH, pt.z+doy*nH);
		break;
	}
	m_ptValS += ptOffset;
	m_ptValE += ptOffset;
	m_ptValI += ptOffset;
	m_ptValR += ptOffset;
	m_dInitial += ptOffset[z];
	m_pt2D = m_ptValE.PointConvert();

	// �؂荞�ݒl���w�肳��Ă��Ȃ���δװ(��Ԃ�TH_NCRead.cpp�ɂ�)
	if ( !bL0Cycle && !(GetValFlags() & g_dwSetValFlags[z]) ) {
		m_nc.nErrorCode = IDS_ERR_NCBLK_NOTCYCLEZ;
		m_nDrawCnt = 0;
		m_dMove[z] = 0.0;
		m_dDwell = 0.0;
		return;
	}

#ifdef _DEBUG_DUMP
	dbg.printf("StartPoint x=%.3f y=%.3f z=%.3f",
		m_ptValS.x, m_ptValS.y, m_ptValS.z);
	dbg.printf("           R-Point=%.3f C-Point=%.3f", dR, GetValue(z));
	dbg.printf("FinalPoint x=%.3f y=%.3f z=%.3f",
		m_ptValE.x, m_ptValE.y, m_ptValE.z);
	dbg.printf("m_nDrawCnt=%d", m_nDrawCnt);
#endif

	if ( m_nDrawCnt <= 0 )
		return;		// bL0Cycle �ł������܂�

	// ���W�i�[�̈�m��
	for ( i=0; i<SIZEOF(m_Cycle); i++ )
		m_Cycle[i] = new PTCYCLE[m_nDrawCnt];
	m_Cycle3D = new PTCYCLE3D[m_nDrawCnt];

	pt = m_ptValS;
	for ( i=0; i<m_nDrawCnt; i++ ) {
		pt[x] += dx;	pt[y] += dy;
#ifdef _DEBUG_DUMP
		dbg.printf("           No.%d [x]=%.3f [y]=%.3f", i+1, pt[x], pt[y]);
#endif
		// �e���ʂ��Ƃɍ��W�ݒ�
		pt[z] = dI;
		m_Cycle3D[i].ptI  = pt;
		m_Cycle[0][i].ptI = pt.PointConvert();
		m_Cycle[1][i].ptI = pt.GetXY();
		m_Cycle[2][i].ptI = pt.GetXZ();
		m_Cycle[3][i].ptI = pt.GetYZ();
		pt[z] = dR;
		m_Cycle3D[i].ptR  = pt;
		m_Cycle[0][i].ptR = pt.PointConvert();
		m_Cycle[1][i].ptR = pt.GetXY();
		m_Cycle[2][i].ptR = pt.GetXZ();
		m_Cycle[3][i].ptR = pt.GetYZ();
		pt[z] = GetValue(z);
		m_Cycle3D[i].ptC  = pt;
		m_Cycle[0][i].ptC = pt.PointConvert();
		m_Cycle[1][i].ptC = pt.GetXY();
		m_Cycle[2][i].ptC = pt.GetXZ();
		m_Cycle[3][i].ptC = pt.GetYZ();
	}
	
	// �㏸���̈ړ��E�؍풷�v�Z
	double	dResult;
	switch ( GetGcode() ) {
	case 84:	// R�_�܂Ő؍한�A�C�Ƽ�ٓ_�܂ő����蕜�A
	case 85:
	case 87:
	case 88:
	case 89:
		if ( lpArgv->bG98 ) {
			dResult = dRLength * nV;
			m_nc.dLength += dZLength * nV;
			m_dMove[z] += dResult;
			m_dCycleMove += dResult;
		}
		else
			m_nc.dLength += dZLength * nV;
		break;
	default:	// ����ȊO�͑����蕜�A
		dResult = (dRLength + dZLength) * nV;
		m_dMove[z] += dResult;
		m_dCycleMove += dResult;
		break;
	}

	// �޳�َ���
	if ( GetValFlags() & NCD_P &&
		(GetGcode()==82 || GetGcode()==88 || GetGcode()==89) )
		m_dDwell = GetValue(NCA_P) * nV;
	else
		m_dDwell = 0.0;

#ifdef _DEBUG_DUMP
	DbgDump();
	Dbg_sep();
#endif
}

CNCcycle::~CNCcycle()
{
	if ( m_Cycle[0] ) {
		for ( int i=0; i<SIZEOF(m_Cycle); i++ )
			delete[] m_Cycle[i];
	}
	if ( m_Cycle3D )
		delete m_Cycle3D;
}

void CNCcycle::DrawTuning(double f)
{
	CNCline::DrawTuning(f);
	m_ptDrawI[0] = m_ptValI.PointConvert() * f;
	m_ptDrawR[0] = m_ptValR.PointConvert() * f;
	if ( m_nDrawCnt > 0 ) {
		for ( int i=0; i<m_nDrawCnt; i++ )
			m_Cycle[0][i].DrawTuning(f);
	}
}

void CNCcycle::DrawTuningXY(double f)
{
	CNCline::DrawTuningXY(f);
	m_ptDrawI[1] = m_ptValI.GetXY() * f;
	m_ptDrawR[1] = m_ptValR.GetXY() * f;
	if ( m_nDrawCnt > 0 ) {
		for ( int i=0; i<m_nDrawCnt; i++ )
			m_Cycle[1][i].DrawTuning(f);
	}
}

void CNCcycle::DrawTuningXZ(double f)
{
	CNCline::DrawTuningXZ(f);
	m_ptDrawI[2] = m_ptValI.GetXZ() * f;
	m_ptDrawR[2] = m_ptValR.GetXZ() * f;
	if ( m_nDrawCnt > 0 ) {
		for ( int i=0; i<m_nDrawCnt; i++ )
			m_Cycle[2][i].DrawTuning(f);
	}
}

void CNCcycle::DrawTuningYZ(double f)
{
	CNCline::DrawTuningYZ(f);
	m_ptDrawI[3] = m_ptValI.GetYZ() * f;
	m_ptDrawR[3] = m_ptValR.GetYZ() * f;
	if ( m_nDrawCnt > 0 ) {
		for ( int i=0; i<m_nDrawCnt; i++ )
			m_Cycle[3][i].DrawTuning(f);
	}
}

void CNCcycle::Draw(CDC* pDC, BOOL bSelect) const
{
#ifdef _DEBUGDRAW_NCD
	CMagaDbg	dbg("CNCcycle::Draw()", DBG_RED);
	dbg.printf("Line=%d", GetBlockLineNo()+1);
#endif
	DrawCycle(NCDRAWVIEW_XYZ, pDC, bSelect);
}

void CNCcycle::DrawXY(CDC* pDC, BOOL bSelect) const
{
#ifdef _DEBUGDRAW_NCD
	CMagaDbg	dbg("CNCcycle::DrawXY()", DBG_RED);
	dbg.printf("Line=%d", GetBlockLineNo()+1);
#endif
	if ( GetPlane() == XY_PLANE ) {
		CNCline::DrawXY(pDC, bSelect);
		DrawCyclePlane(NCDRAWVIEW_XY, pDC, bSelect);
	}
	else
		DrawCycle(NCDRAWVIEW_XY, pDC, bSelect);
}

void CNCcycle::DrawXZ(CDC* pDC, BOOL bSelect) const
{
#ifdef _DEBUGDRAW_NCD
	CMagaDbg	dbg("CNCcycle::DrawXZ()", DBG_RED);
	dbg.printf("Line=%d", GetBlockLineNo()+1);
#endif
	if ( GetPlane() == XZ_PLANE ) {
		CNCline::DrawXZ(pDC, bSelect);
		DrawCyclePlane(NCDRAWVIEW_XZ, pDC, bSelect);
	}
	else
		DrawCycle(NCDRAWVIEW_XZ, pDC, bSelect);
}

void CNCcycle::DrawYZ(CDC* pDC, BOOL bSelect) const
{
#ifdef _DEBUGDRAW_NCD
	CMagaDbg	dbg("CNCcycle::DrawYZ()", DBG_RED);
	dbg.printf("Line=%d", GetBlockLineNo()+1);
#endif
	if ( GetPlane() == YZ_PLANE ) {
		CNCline::DrawYZ(pDC, bSelect);
		DrawCyclePlane(NCDRAWVIEW_YZ, pDC, bSelect);
	}
	else
		DrawCycle(NCDRAWVIEW_YZ, pDC, bSelect);
}

void CNCcycle::DrawWire(CDC* pDC, BOOL bSelect) const
{
}

void CNCcycle::DrawWireXY(CDC* pDC, BOOL bSelect) const
{
}

void CNCcycle::DrawWireXZ(CDC* pDC, BOOL bSelect) const
{
}

void CNCcycle::DrawWireYZ(CDC* pDC, BOOL bSelect) const
{
}

void CNCcycle::DrawCyclePlane(EN_NCDRAWVIEW enDraw, CDC* pDC, BOOL bSelect) const
{
	CPen*	pOldPen = bSelect ?
		AfxGetNCVCMainWnd()->GetPenCom(COMPEN_SEL) :
		AfxGetNCVCMainWnd()->GetPenNC(NCPEN_CYCLE);
	pOldPen = pDC->SelectObject(pOldPen);
	CBrush*	pOldBrush = pDC->SelectObject(AfxGetNCVCMainWnd()->GetBrushNC(NCBRUSH_CYCLEXY));
	for ( int i=0; i<m_nDrawCnt; i++ )
		pDC->Ellipse(&m_Cycle[enDraw][i].rcDraw);
	pDC->SelectObject(pOldBrush);
	pDC->SelectObject(pOldPen);
}

void CNCcycle::DrawCycle(EN_NCDRAWVIEW enDraw, CDC* pDC, BOOL bSelect) const
{
	CPen*	pPenM = AfxGetNCVCMainWnd()->GetPenNC(NCPEN_G0);
	CPen*	pPenC = AfxGetNCVCMainWnd()->GetPenNC(NCPEN_CYCLE);
	CPen*	pOldPen;
	if ( bSelect )
		pPenM = pPenC = pOldPen = AfxGetNCVCMainWnd()->GetPenCom(COMPEN_SEL);
	else
		pOldPen = pPenM;
	pOldPen = pDC->SelectObject(pOldPen);
	// �O��ʒu����P�_�ڂ̌����H�܂ł̈ړ�
	pDC->MoveTo(m_ptDrawS[enDraw]);
	pDC->LineTo(m_ptDrawI[enDraw]);		// ���݈ʒu����P�_�ڂ̲Ƽ�ٓ_
	pDC->LineTo(m_ptDrawR[enDraw]);		// �Ƽ�ٓ_����R�_
	pDC->LineTo(m_ptDrawE[enDraw]);		// R�_����Ō�̌����H
	// �؍�����̕`��
	for ( int i=0; i<m_nDrawCnt; i++ ) {
		pDC->SelectObject(pPenM);
		pDC->MoveTo(m_Cycle[enDraw][i].ptDrawI);
		pDC->LineTo(m_Cycle[enDraw][i].ptDrawR);
		pDC->SelectObject(pPenC);
		pDC->LineTo(m_Cycle[enDraw][i].ptDrawC);
	}
	pDC->SelectObject(pOldPen);
}

//////////////////////////////////////////////////////////////////////
// CNCcircle �N���X
//////////////////////////////////////////////////////////////////////

CNCcircle::CNCcircle(const CNCdata* pData, LPNCARGV lpArgv, const CPoint3D& ptOffset, BOOL bLathe/*=FALSE*/) :
	CNCline(NCDARCDATA, pData, lpArgv, ptOffset)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CNCcircle", DBG_MAGENTA);
#endif
	BOOL		fError = TRUE;	// Error

	m_nG23 = GetGcode() - 2;	// G2=0, G3=1

	if ( !bLathe ) {
		// XZ���ʂ�(Z->X, X->Y)�Ȃ̂ł��̂܂܌v�Z����� -90����]������K�v������
		// �ȒP�ɑΉ�����ɂ͉�]�����𔽑΂ɂ���΂悢 -> ����������ƃ}�V�ȑΉ����I
		if ( GetPlane() == XZ_PLANE )
			m_nG23 = 1 - m_nG23;	// 0->1 , 1->0;
	}

	// Ȳè�ނ̍��W�ް��Œ��S���v�Z���Ă�����W��]
	m_pRead->m_ptValOrg.SetPoint(GetValue(NCA_X), GetValue(NCA_Y), GetValue(NCA_Z));
	m_ptValS = pData->GetOriginalEndPoint();	// ���ƂŐݒ肵����
	m_ptValE = m_pRead->m_ptValOrg;

	// ���ʍ��W�擾
	CPointD	pts( GetPlaneValue(m_ptValS) ),
			pte( GetPlaneValue(m_ptValE) ),
			pto;

	// ���a�ƒ��S���W�̌v�Z(R�D��)
	if ( GetValFlags() & NCD_R ) {
		m_r = GetValue(NCA_R);
		fError = CalcCenter(pts, pte);
	}
	else if ( GetValFlags() & (NCD_I|NCD_J|NCD_K) ) {
		m_ptOrg = m_ptValS;
		switch ( GetPlane() ) {
		case XY_PLANE:
			m_r = _hypot(GetValue(NCA_I), GetValue(NCA_J));
			m_ptOrg.x += GetValue(NCA_I);
			m_ptOrg.y += GetValue(NCA_J);
			pto = m_ptOrg.GetXY();
			break;
		case XZ_PLANE:
			m_r = _hypot(GetValue(NCA_I), GetValue(NCA_K));
			m_ptOrg.x += GetValue(NCA_I);
			m_ptOrg.z += GetValue(NCA_K);
			pto = m_ptOrg.GetXZ();
			break;
		case YZ_PLANE:
			m_r = _hypot(GetValue(NCA_J), GetValue(NCA_K));
			m_ptOrg.y += GetValue(NCA_J);
			m_ptOrg.z += GetValue(NCA_K);
			pto = m_ptOrg.GetYZ();
			break;
		}
		pts -= pto;		// �p�x�����p�̌��_�␳
		pte -= pto;
		AngleTuning(pts, pte);
		fError = FALSE;
	}

	// �`��֐��̌�����ضوړ��ʂ̌v�Z
	Constracter();

	m_ptValS = pData->GetEndPoint();	// �O��̌v�Z�l����
	m_ptValE += ptOffset;
	m_ptOrg  += ptOffset;
	// ���W��]
	if ( lpArgv->g68.bG68 ) {
		CalcG68Round(&(lpArgv->g68), m_ptValE);
		CalcG68Round(&(lpArgv->g68), m_ptOrg);
		// �p�x�̍Ē���( [m_sq|m_eq] += lpArgv->g68.dRound; �ł�OK )
		AngleTuning(GetPlaneValueOrg(m_ptValS, m_ptOrg), GetPlaneValueOrg(m_ptValE, m_ptOrg));
	}

	// �`��I�_���v�Z���ۑ�
	m_pt2D = m_ptValE.PointConvert();

#ifdef _DEBUG_DUMP
//	dbg.printf("gcode=%d", m_nG23);
	dbg.printf("sx=%.3f sy=%.3f sz=%.3f / ex=%.3f ey=%.3f ez=%.3f / r=%.3f",
		m_ptValS.x, m_ptValS.y, m_ptValS.z,
		m_ptValE.x, m_ptValE.y, m_ptValE.z, m_r);
	dbg.printf("px=%.3f py=%.3f pz=%.3f / sq=%f eq=%f",
		m_ptOrg.x, m_ptOrg.y, m_ptOrg.z, DEG(m_sq), DEG(m_eq));
#endif

	if ( fError )
		m_nc.nErrorCode = IDS_ERR_NCBLK_CIRCLECENTER;
#ifdef _DEBUG_DUMP
	DbgDump();
	Dbg_sep();
#endif
}

CNCcircle::CNCcircle(const CNCdata* pData) : CNCline(pData)
{
	m_pt2D	= m_ptValE.PointConvert();
	m_nG23	= static_cast<const CNCcircle *>(pData)->GetG23();
	m_ptOrg	= static_cast<const CNCcircle *>(pData)->GetOrg();
	m_r		= static_cast<const CNCcircle *>(pData)->GetR();
	m_sq	= static_cast<const CNCcircle *>(pData)->GetStartAngle();
	m_eq	= static_cast<const CNCcircle *>(pData)->GetEndAngle();
	Constracter();
}

void CNCcircle::Constracter(void)
{
	// �`��֐��̌�����ضوړ��ʂ̌v�Z
	switch ( GetPlane() ) {
	case XY_PLANE:
		m_pfnCircleDraw = &CNCcircle::Draw_G17;
		m_dHelicalStep = GetValFlags() & NCD_Z ?
			(m_ptValE.z - m_ptValS.z) / ((m_eq-m_sq)/ARCSTEP) : 0.0;
		break;
	case XZ_PLANE:
		m_pfnCircleDraw = &CNCcircle::Draw_G18;
		m_dHelicalStep = GetValFlags() & NCD_Y ?
			(m_ptValE.y - m_ptValS.y) / ((m_eq-m_sq)/ARCSTEP) : 0.0;
		break;
	case YZ_PLANE:
		m_pfnCircleDraw = &CNCcircle::Draw_G19;
		m_dHelicalStep = GetValFlags() & NCD_X ?
			(m_ptValE.x - m_ptValS.x) / ((m_eq-m_sq)/ARCSTEP) : 0.0;
		break;
	}
}

BOOL CNCcircle::CalcCenter(const CPointD& pts, const CPointD& pte)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CalcCenter()", DBG_RED);
#endif
	// R �w��Ŏn�_�I�_�������ꍇ�̓G���[
	if ( pts == pte )
		return TRUE;	// �G���[�͐^�ŕԂ�

	// �Q�̉~�̌�_�����߂�
	CPointD	pt1, pt2;
	int		nResult;
	tie(nResult, pt1, pt2) = ::CalcIntersectionPoint_CC(pts, pte, m_r, m_r);
	if ( nResult < 1 )
		return TRUE;

	// �ǂ���̉����̗p���邩
	AngleTuning(pts-pt1, pte-pt1);	// �܂�����̒��S���W����p�x�����߂�
	double	q = ::RoundUp(DEG(m_eq-m_sq));
	if ( nResult==1 ||
			(m_r>0.0 && q<=180.0) ||	// 180���ȉ�
			(m_r<0.0 && q> 180.0) ) {	// 180��������
		SetCenter(pt1);
	}
	else {
		// ������ϯ����Ȃ������̂ő����̉����̗p
		AngleTuning(pts-pt2, pte-pt2);
		SetCenter(pt2);
	}
	return FALSE;		// �����͋U�ŕԂ�
}

void CNCcircle::SetCenter(const CPointD& pt)
{
	switch ( GetPlane() ) {
	case XY_PLANE:
		m_ptOrg.x = pt.x;
		m_ptOrg.y = pt.y;
		m_ptOrg.z = m_ptValS.z;		// �n�_�̊J�n�ʒu
		break;
	case XZ_PLANE:
		m_ptOrg.x = pt.x;
		m_ptOrg.y = m_ptValS.y;
		m_ptOrg.z = pt.y;
		break;
	case YZ_PLANE:
		m_ptOrg.x = m_ptValS.x;
		m_ptOrg.y = pt.x;
		m_ptOrg.z = pt.y;
		break;
	}
}

void CNCcircle::AngleTuning(const CPointD& pts, const CPointD& pte)
{
	if ( (m_sq=atan2(pts.y, pts.x)) < 0.0 )
		m_sq += RAD(360.0);
	if ( (m_eq=atan2(pte.y, pte.x)) < 0.0 )
		m_eq += RAD(360.0);
	// ��� s<e (�����v���) �Ƃ���
	if ( m_nG23 == 0 )	// G02 �Ȃ�J�n�p�x�ƏI���p�x�����ւ�
		std::swap(m_sq, m_eq);
	double	sq = ::RoundUp(DEG(m_sq));
	while ( sq >= ::RoundUp(DEG(m_eq)) )
		m_eq += RAD(360.0);
}

double CNCcircle::SetCalcLength(void)
{
	// �؍풷�̂݌v�Z
	m_dMove[NCA_X] = m_dMove[NCA_Y] = m_dMove[NCA_Z] = 0.0;

	if ( m_obCdata.IsEmpty() )
		m_nc.dLength = fabs(m_r * (m_eq - m_sq));
	else {
		m_nc.dLength = 0.0;
		// �e�␳�v�f�̍��v
		CNCdata*	pData;
		CNCcircle*	pCircle;
		CPoint3D	pt;
		for ( int i=0; i<m_obCdata.GetSize(); i++ ) {
			pData = m_obCdata[i];
			switch ( pData->GetType() ) {
			case NCDLINEDATA:
				pt = pData->GetEndPoint() - pData->GetStartPoint();
				m_nc.dLength += pt.hypot();
				break;
			case NCDARCDATA:
				pCircle = static_cast<CNCcircle *>(pData);
				m_nc.dLength += fabs(pCircle->GetR() * (pCircle->GetEndAngle() - pCircle->GetStartAngle()));
				break;
			}
		}
	}

	return m_nc.dLength;
}

void CNCcircle::DrawTuning(double f)
{
	// �v�Z���Ȃ���g��W����^����
	m_dFactor = f;

	// ܲԉ��H�\���p�̎n�_�I�_
	double		sq, eq, r = fabs(m_r) * f;
	CPoint3D	pt3D, ptOrg(m_ptOrg);	ptOrg *= f;
	tie(sq, eq) = GetSqEq();
	pt3D.x = r * cos(sq) + ptOrg.x;
	pt3D.y = r * sin(sq) + ptOrg.y;
	pt3D.z = ptOrg.z;
	m_ptDrawS[NCDRAWVIEW_XYZ] = pt3D.PointConvert();
	pt3D.x = r * cos(eq) + ptOrg.x;
	pt3D.y = r * sin(eq) + ptOrg.y;
	pt3D.z = m_ptValE.z * f;
	m_ptDrawE[NCDRAWVIEW_XYZ] = pt3D.PointConvert();

	CNCdata::DrawTuning(f);
}

void CNCcircle::DrawTuningXY(double f)
{
	m_dFactorXY = f;

	double		sq, eq, r = fabs(m_r) * f;
	CPoint3D	ptOrg(m_ptOrg);	ptOrg *= f;
	tie(sq, eq) = GetSqEq();
	m_ptDrawS[NCDRAWVIEW_XY].x = (int)(r * cos(sq) + ptOrg.x);
	m_ptDrawS[NCDRAWVIEW_XY].y = (int)(r * sin(sq) + ptOrg.y);
	m_ptDrawE[NCDRAWVIEW_XY].x = (int)(r * cos(eq) + ptOrg.x);
	m_ptDrawE[NCDRAWVIEW_XY].y = (int)(r * sin(eq) + ptOrg.y);

	CNCdata::DrawTuningXY(f);
}

void CNCcircle::DrawTuningXZ(double f)
{
	m_dFactorXZ = f;

	double		sq, eq, r = fabs(m_r) * f;
	CPoint3D	ptOrg(m_ptOrg);	ptOrg *= f;
	tie(sq, eq) = GetSqEq();
	m_ptDrawS[NCDRAWVIEW_XZ].x = (int)(r * cos(sq) + ptOrg.x);
	m_ptDrawS[NCDRAWVIEW_XZ].y = (int)(ptOrg.z);
	m_ptDrawE[NCDRAWVIEW_XZ].x = (int)(r * cos(eq) + ptOrg.x);
	m_ptDrawE[NCDRAWVIEW_XZ].y = (int)(m_ptValE.z * f);

	CNCdata::DrawTuningXZ(f);
}

void CNCcircle::DrawTuningYZ(double f)
{
	m_dFactorYZ = f;

	double		sq, eq, r = fabs(m_r) * f;
	CPoint3D	ptOrg(m_ptOrg);	ptOrg *= f;
	tie(sq, eq) = GetSqEq();
	m_ptDrawS[NCDRAWVIEW_YZ].x = (int)(r * sin(sq) + ptOrg.y);
	m_ptDrawS[NCDRAWVIEW_YZ].y = (int)(ptOrg.z);
	m_ptDrawE[NCDRAWVIEW_YZ].x = (int)(r * sin(eq) + ptOrg.y);
	m_ptDrawE[NCDRAWVIEW_YZ].y = (int)(m_ptValE.z * f);

	CNCdata::DrawTuningYZ(f);
}

void CNCcircle::Draw(CDC* pDC, BOOL bSelect) const
{
#ifdef _DEBUGDRAW_NCD
	CMagaDbg	dbg("CNCcircle::Draw()", DBG_RED);
	dbg.printf("Line=%d", GetBlockLineNo()+1);
#endif
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();

	// ���ʂ��Ƃ̕`��֐�
	if ( m_obCdata.IsEmpty() || pOpt->GetNCViewFlg(NCVIEWFLG_DRAWREVISE) ) {
		CPen*	pOldPen;
		if ( bSelect )
			pOldPen = AfxGetNCVCMainWnd()->GetPenCom(COMPEN_SEL);
		else
			pOldPen = AfxGetNCVCMainWnd()->GetPenNC(
				m_obCdata.IsEmpty() ? NCPEN_G1 : NCPEN_CORRECT);
		pOldPen = pDC->SelectObject(pOldPen);
		(this->*m_pfnCircleDraw)(NCDRAWVIEW_XYZ, pDC);
		pDC->SelectObject(pOldPen);
	}
	// ���S���W(���F)
	if ( pOpt->GetNCViewFlg(NCVIEWFLG_DRAWCIRCLECENTER) )
		pDC->SetPixelV(m_ptOrg.PointConvert()*m_dFactor,
			pOpt->GetNcDrawColor(NCCOL_CENTERCIRCLE));
	// �a�␳�ް��̕`��
	CNCdata::Draw(pDC, bSelect);
}

void CNCcircle::DrawXY(CDC* pDC, BOOL bSelect) const
{
#ifdef _DEBUGDRAW_NCD
	CMagaDbg	dbg("CNCcircle::DrawXY()", DBG_RED);
	dbg.printf("Line=%d", GetBlockLineNo()+1);
#endif
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();

	if ( m_obCdata.IsEmpty() || pOpt->GetNCViewFlg(NCVIEWFLG_DRAWREVISE) ) {
		CPen*	pOldPen;
		if ( bSelect )
			pOldPen = AfxGetNCVCMainWnd()->GetPenCom(COMPEN_SEL);
		else
			pOldPen = AfxGetNCVCMainWnd()->GetPenNC(
				m_obCdata.IsEmpty() ? NCPEN_G1 : NCPEN_CORRECT);
		pOldPen = pDC->SelectObject(pOldPen);
		(this->*m_pfnCircleDraw)(NCDRAWVIEW_XY, pDC);
		pDC->SelectObject(pOldPen);
	}
	if ( pOpt->GetNCViewFlg(NCVIEWFLG_DRAWCIRCLECENTER) )
		pDC->SetPixelV(m_ptOrg.GetXY()*m_dFactorXY,
			pOpt->GetNcDrawColor(NCCOL_CENTERCIRCLE));
	CNCdata::DrawXY(pDC, bSelect);
}

void CNCcircle::DrawXZ(CDC* pDC, BOOL bSelect) const
{
#ifdef _DEBUGDRAW_NCD
	CMagaDbg	dbg("CNCcircle::DrawXZ()", DBG_RED);
	dbg.printf("Line=%d", GetBlockLineNo()+1);
#endif
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();

	if ( m_obCdata.IsEmpty() || pOpt->GetNCViewFlg(NCVIEWFLG_DRAWREVISE) ) {
		CPen*	pOldPen;
		if ( bSelect )
			pOldPen = AfxGetNCVCMainWnd()->GetPenCom(COMPEN_SEL);
		else
			pOldPen = AfxGetNCVCMainWnd()->GetPenNC(
				m_obCdata.IsEmpty() ? NCPEN_G1 : NCPEN_CORRECT);
		pOldPen = pDC->SelectObject(pOldPen);
		(this->*m_pfnCircleDraw)(NCDRAWVIEW_XZ, pDC);
		pDC->SelectObject(pOldPen);
	}
	if ( pOpt->GetNCViewFlg(NCVIEWFLG_DRAWCIRCLECENTER) )
		pDC->SetPixelV(m_ptOrg.GetXZ()*m_dFactorXZ,
			pOpt->GetNcDrawColor(NCCOL_CENTERCIRCLE));
	CNCdata::DrawXZ(pDC, bSelect);
}

void CNCcircle::DrawYZ(CDC* pDC, BOOL bSelect) const
{
#ifdef _DEBUGDRAW_NCD
	CMagaDbg	dbg("CNCcircle::DrawYZ()", DBG_RED);
	dbg.printf("Line=%d", GetBlockLineNo()+1);
#endif
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();

	if ( m_obCdata.IsEmpty() || pOpt->GetNCViewFlg(NCVIEWFLG_DRAWREVISE) ) {
		CPen*	pOldPen;
		if ( bSelect )
			pOldPen = AfxGetNCVCMainWnd()->GetPenCom(COMPEN_SEL);
		else
			pOldPen = AfxGetNCVCMainWnd()->GetPenNC(
				m_obCdata.IsEmpty() ? NCPEN_G1 : NCPEN_CORRECT);
		pOldPen = pDC->SelectObject(pOldPen);
		(this->*m_pfnCircleDraw)(NCDRAWVIEW_YZ, pDC);
		pDC->SelectObject(pOldPen);
	}
	if ( pOpt->GetNCViewFlg(NCVIEWFLG_DRAWCIRCLECENTER) )
		pDC->SetPixelV(m_ptOrg.GetYZ()*m_dFactorYZ,
			pOpt->GetNcDrawColor(NCCOL_CENTERCIRCLE));
	CNCdata::DrawYZ(pDC, bSelect);
}

void CNCcircle::DrawWire(CDC* pDC, BOOL bSelect) const
{
	DrawWireLine(NCDRAWVIEW_XYZ, pDC, bSelect);
}

void CNCcircle::DrawWireXY(CDC* pDC, BOOL bSelect) const
{
	DrawWireLine(NCDRAWVIEW_XY, pDC, bSelect);
}

void CNCcircle::DrawWireXZ(CDC* pDC, BOOL bSelect) const
{
	DrawWireLine(NCDRAWVIEW_XZ, pDC, bSelect);
}

void CNCcircle::DrawWireYZ(CDC* pDC, BOOL bSelect) const
{
	DrawWireLine(NCDRAWVIEW_YZ, pDC, bSelect);
}

void CNCcircle::DrawWireLine(EN_NCDRAWVIEW enDraw, CDC* pDC, BOOL bSelect) const
{
	CPen*	pOldPen;
	if ( bSelect )
		pOldPen = AfxGetNCVCMainWnd()->GetPenCom(COMPEN_SEL);
	else
		pOldPen = AfxGetNCVCMainWnd()->GetPenNC(
			m_obCdata.IsEmpty() ? NCPEN_G1 : NCPEN_CORRECT);
	pOldPen = pDC->SelectObject(pOldPen);
	// XY
	Draw_G17(enDraw, pDC);
	// UV
	if ( m_pWireObj ) {
		static_cast<CNCcircle*>(m_pWireObj)->Draw_G17(enDraw, pDC);
		// XY��UV�̐ڑ�
		pDC->MoveTo(m_ptDrawS[enDraw]);
		pDC->LineTo(static_cast<CNCcircle*>(m_pWireObj)->GetDrawStartPoint(enDraw));
		pDC->MoveTo(m_ptDrawE[enDraw]);
		pDC->LineTo(static_cast<CNCcircle*>(m_pWireObj)->GetDrawEndPoint(enDraw));
	}
	pDC->SelectObject(pOldPen);
}

// CDC::Arc() ���g���Ƃǂ����Ă��\�����Y����D
// ���ꕽ�ʂł����Ă����א����ɂ��ߎ����s��
void CNCcircle::Draw_G17(EN_NCDRAWVIEW enDraw, CDC* pDC) const	// XY_PLANE
{
	double		sq, eq, dHelical = m_dHelicalStep, r = fabs(m_r);
	CPoint3D	pt3D, ptDrawOrg(m_ptOrg);
	CPointD		ptDraw;

	tie(sq, eq) = GetSqEq();

	switch ( enDraw ) {
	case NCDRAWVIEW_XYZ:
		r *= m_dFactor;
		dHelical  *= m_dFactor;
		ptDrawOrg *= m_dFactor;
		pt3D.x = r * cos(sq) + ptDrawOrg.x;
		pt3D.y = r * sin(sq) + ptDrawOrg.y;
		pt3D.z = ptDrawOrg.z;		// �ضيJ�n���W
		ptDraw = pt3D.PointConvert();
		pDC->MoveTo(ptDraw);	// �J�n�_�ֈړ�
		// ARCSTEP �Â��א����ŕ`��
		if ( m_nG23 == 0 ) {
			for ( sq-=ARCSTEP; sq>eq; sq-=ARCSTEP ) {
				pt3D.x = r * cos(sq) + ptDrawOrg.x;
				pt3D.y = r * sin(sq) + ptDrawOrg.y;
				pt3D.z += dHelical;
				ptDraw = pt3D.PointConvert();
				pDC->LineTo(ptDraw);
			}
		}
		else {
			for ( sq+=ARCSTEP; sq<eq; sq+=ARCSTEP ) {
				pt3D.x = r * cos(sq) + ptDrawOrg.x;
				pt3D.y = r * sin(sq) + ptDrawOrg.y;
				pt3D.z += dHelical;
				ptDraw = pt3D.PointConvert();
				pDC->LineTo(ptDraw);
			}
		}
		// �[�����`��
		pt3D.x = r * cos(eq) + ptDrawOrg.x;
		pt3D.y = r * sin(eq) + ptDrawOrg.y;
		pt3D.z = m_ptValE.z * m_dFactor;
		ptDraw = pt3D.PointConvert();
		pDC->LineTo(ptDraw);
		break;

	case NCDRAWVIEW_XY:	// Don't ARC()
		r *= m_dFactorXY;
		ptDrawOrg *= m_dFactorXY;
		ptDraw.x = r * cos(sq) + ptDrawOrg.x;
		ptDraw.y = r * sin(sq) + ptDrawOrg.y;
		pDC->MoveTo(ptDraw);
		if ( m_nG23 == 0 ) {
			for ( sq-=ARCSTEP; sq>eq; sq-=ARCSTEP ) {
				ptDraw.x = r * cos(sq) + ptDrawOrg.x;
				ptDraw.y = r * sin(sq) + ptDrawOrg.y;
				pDC->LineTo(ptDraw);
			}
		}
		else {
			for ( sq+=ARCSTEP; sq<eq; sq+=ARCSTEP ) {
				ptDraw.x = r * cos(sq) + ptDrawOrg.x;
				ptDraw.y = r * sin(sq) + ptDrawOrg.y;
				pDC->LineTo(ptDraw);
			}
		}
		ptDraw.x = r * cos(eq) + ptDrawOrg.x;
		ptDraw.y = r * sin(eq) + ptDrawOrg.y;
		pDC->LineTo(ptDraw);
		break;

	case NCDRAWVIEW_XZ:	// ��L��`�̒��_�����Ԃ����ł��ضِ؍�̕`��ɑΉ��ł��Ȃ�
		r *= m_dFactorXZ;
		dHelical  *= m_dFactorXZ;
		ptDrawOrg *= m_dFactorXZ;
		ptDraw.x = r * cos(sq) + ptDrawOrg.x;
		ptDraw.y = ptDrawOrg.z;
		pDC->MoveTo(ptDraw);
		if ( m_nG23 == 0 ) {
			for ( sq-=ARCSTEP; sq>eq; sq-=ARCSTEP ) {
				ptDraw.x  = r * cos(sq) + ptDrawOrg.x;
				ptDraw.y += dHelical;
				pDC->LineTo(ptDraw);
			}
		}
		else {
			for ( sq+=ARCSTEP; sq<eq; sq+=ARCSTEP ) {
				ptDraw.x  = r * cos(sq) + ptDrawOrg.x;
				ptDraw.y += dHelical;
				pDC->LineTo(ptDraw);
			}
		}
		ptDraw.x = r * cos(eq) + ptDrawOrg.x;
		ptDraw.y = m_ptValE.z * m_dFactorXZ;
		pDC->LineTo(ptDraw);
		break;

	case NCDRAWVIEW_YZ:
		r *= m_dFactorYZ;
		dHelical  *= m_dFactorYZ;
		ptDrawOrg *= m_dFactorYZ;
		ptDraw.x = r * sin(sq) + ptDrawOrg.y;
		ptDraw.y = ptDrawOrg.z;
		pDC->MoveTo(ptDraw);
		if ( m_nG23 == 0 ) {
			for ( sq-=ARCSTEP; sq>eq; sq-=ARCSTEP ) {
				ptDraw.x  = r * sin(sq) + ptDrawOrg.y;
				ptDraw.y += dHelical;
				pDC->LineTo(ptDraw);
			}
		}
		else {
			for ( sq+=ARCSTEP; sq<eq; sq+=ARCSTEP ) {
				ptDraw.x  = r * sin(sq) + ptDrawOrg.y;
				ptDraw.y += dHelical;
				pDC->LineTo(ptDraw);
			}
		}
		ptDraw.x = r * sin(eq) + ptDrawOrg.y;
		ptDraw.y = m_ptValE.z * m_dFactorYZ;
		pDC->LineTo(ptDraw);
		break;
	}
}

void CNCcircle::Draw_G18(EN_NCDRAWVIEW enDraw, CDC* pDC) const	// XZ_PLANE
{
	double		sq, eq, dHelical = m_dHelicalStep, r = fabs(m_r);
	CPoint3D	pt3D, ptDrawOrg(m_ptOrg);
	CPointD		ptDraw;

	tie(sq, eq) = GetSqEq();

	switch ( enDraw ) {
	case NCDRAWVIEW_XYZ:
		r *= m_dFactor;
		dHelical  *= m_dFactor;
		ptDrawOrg *= m_dFactor;
		pt3D.x = r * cos(sq) + ptDrawOrg.x;
		pt3D.y = ptDrawOrg.y;
		pt3D.z = r * sin(sq) + ptDrawOrg.z;
		ptDraw = pt3D.PointConvert();
		pDC->MoveTo(ptDraw);
		if ( m_nG23 == 0 ) {
			for ( sq-=ARCSTEP; sq>eq; sq-=ARCSTEP ) {
				pt3D.x = r * cos(sq) + ptDrawOrg.x;
				pt3D.y += dHelical;
				pt3D.z = r * sin(sq) + ptDrawOrg.z;
				ptDraw = pt3D.PointConvert();
				pDC->LineTo(ptDraw);
			}
		}
		else {
			for ( sq+=ARCSTEP; sq<eq; sq+=ARCSTEP ) {
				pt3D.x = r * cos(sq) + ptDrawOrg.x;
				pt3D.y += dHelical;
				pt3D.z = r * sin(sq) + ptDrawOrg.z;
				ptDraw = pt3D.PointConvert();
				pDC->LineTo(ptDraw);
			}
		}
		pt3D.x = r * cos(eq) + ptDrawOrg.x;
		pt3D.y = m_ptValE.y * m_dFactor;
		pt3D.z = r * sin(eq) + ptDrawOrg.z;
		ptDraw = pt3D.PointConvert();
		pDC->LineTo(ptDraw);
		break;

	case NCDRAWVIEW_XY:
		r *= m_dFactorXY;
		dHelical  *= m_dFactorXY;
		ptDrawOrg *= m_dFactorXY;
		ptDraw.x = r * cos(sq) + ptDrawOrg.x;
		ptDraw.y = ptDrawOrg.y;
		pDC->MoveTo(ptDraw);
		if ( m_nG23 == 0 ) {
			for ( sq-=ARCSTEP; sq>eq; sq-=ARCSTEP ) {
				ptDraw.x  = r * cos(sq) + ptDrawOrg.x;
				ptDraw.y += dHelical;
				pDC->LineTo(ptDraw);
			}
		}
		else {
			for ( sq+=ARCSTEP; sq<eq; sq+=ARCSTEP ) {
				ptDraw.x  = r * cos(sq) + ptDrawOrg.x;
				ptDraw.y += dHelical;
				pDC->LineTo(ptDraw);
			}
		}
		ptDraw.x = r * cos(eq) + ptDrawOrg.x;
		ptDraw.y = m_ptValE.y * m_dFactorXY;
		pDC->LineTo(ptDraw);
		break;

	case NCDRAWVIEW_XZ:	// Don't ARC()
		r *= m_dFactorXZ;
		ptDrawOrg *= m_dFactorXZ;
		ptDraw.x = r * cos(sq) + ptDrawOrg.x;
		ptDraw.y = r * sin(sq) + ptDrawOrg.z;
		pDC->MoveTo(ptDraw);
		if ( m_nG23 == 0 ) {
			for ( sq-=ARCSTEP; sq>eq; sq-=ARCSTEP ) {
				ptDraw.x = r * cos(sq) + ptDrawOrg.x;
				ptDraw.y = r * sin(sq) + ptDrawOrg.z;
				pDC->LineTo(ptDraw);
			}
		}
		else {
			for ( sq+=ARCSTEP; sq<eq; sq+=ARCSTEP ) {
				ptDraw.x = r * cos(sq) + ptDrawOrg.x;
				ptDraw.y = r * sin(sq) + ptDrawOrg.z;
				pDC->LineTo(ptDraw);
			}
		}
		ptDraw.x = r * cos(eq) + ptDrawOrg.x;
		ptDraw.y = r * sin(eq) + ptDrawOrg.z;
		pDC->LineTo(ptDraw);
		break;

	case NCDRAWVIEW_YZ:
		r *= m_dFactorYZ;
		dHelical  *= m_dFactorYZ;
		ptDrawOrg *= m_dFactorYZ;
		ptDraw.x = ptDrawOrg.y;
		ptDraw.y = r * sin(sq) + ptDrawOrg.z;
		pDC->MoveTo(ptDraw);
		if ( m_nG23 == 0 ) {
			for ( sq-=ARCSTEP; sq>eq; sq-=ARCSTEP ) {
				ptDraw.x += dHelical;
				ptDraw.y  = r * sin(sq) + ptDrawOrg.z;
				pDC->LineTo(ptDraw);
			}
		}
		else {
			for ( sq+=ARCSTEP; sq<eq; sq+=ARCSTEP ) {
				ptDraw.x += dHelical;
				ptDraw.y  = r * sin(sq) + ptDrawOrg.z;
				pDC->LineTo(ptDraw);
			}
		}
		ptDraw.x = m_ptValE.y * m_dFactorYZ;
		ptDraw.y = r * sin(eq) + ptDrawOrg.z;
		pDC->LineTo(ptDraw);
		break;
	}
}

void CNCcircle::Draw_G19(EN_NCDRAWVIEW enDraw, CDC* pDC) const	// YZ_PLANE
{
	double		sq, eq, dHelical = m_dHelicalStep, r = fabs(m_r);
	CPoint3D	pt3D, ptDrawOrg(m_ptOrg);
	CPointD		ptDraw;

	tie(sq, eq) = GetSqEq();

	switch ( enDraw ) {
	case NCDRAWVIEW_XYZ:
		r *= m_dFactor;
		dHelical  *= m_dFactor;
		ptDrawOrg *= m_dFactor;
		pt3D.x = ptDrawOrg.x;
		pt3D.y = r * cos(sq) + ptDrawOrg.y;
		pt3D.z = r * sin(sq) + ptDrawOrg.z;
		ptDraw = pt3D.PointConvert();
		pDC->MoveTo(ptDraw);
		if ( m_nG23 == 0 ) {
			for ( sq-=ARCSTEP; sq>eq; sq-=ARCSTEP ) {
				pt3D.x += dHelical;
				pt3D.y = r * cos(sq) + ptDrawOrg.y;
				pt3D.z = r * sin(sq) + ptDrawOrg.z;
				ptDraw = pt3D.PointConvert();
				pDC->LineTo(ptDraw);
			}
		}
		else {
			for ( sq+=ARCSTEP; sq<eq; sq+=ARCSTEP ) {
				pt3D.x += dHelical;
				pt3D.y = r * cos(sq) + ptDrawOrg.y;
				pt3D.z = r * sin(sq) + ptDrawOrg.z;
				ptDraw = pt3D.PointConvert();
				pDC->LineTo(ptDraw);
			}
		}
		pt3D.x = m_ptValE.x * m_dFactor;
		pt3D.y = r * cos(eq) + ptDrawOrg.y;
		pt3D.z = r * sin(eq) + ptDrawOrg.z;
		ptDraw = pt3D.PointConvert();
		pDC->LineTo(ptDraw);
		break;

	case NCDRAWVIEW_XY:
		r *= m_dFactorXY;
		dHelical  *= m_dFactorXY;
		ptDrawOrg *= m_dFactorXY;
		ptDraw.x = ptDrawOrg.x;
		ptDraw.y = r * cos(sq) + ptDrawOrg.y;
		pDC->MoveTo(ptDraw);
		if ( m_nG23 == 0 ) {
			for ( sq-=ARCSTEP; sq>eq; sq-=ARCSTEP ) {
				ptDraw.x += dHelical;
				ptDraw.y  = r * cos(sq) + ptDrawOrg.y;
				pDC->LineTo(ptDraw);
			}
		}
		else {
			for ( sq+=ARCSTEP; sq<eq; sq+=ARCSTEP ) {
				ptDraw.x += dHelical;
				ptDraw.y  = r * cos(sq) + ptDrawOrg.y;
				pDC->LineTo(ptDraw);
			}
		}
		ptDraw.x = m_ptValE.x * m_dFactorXY;
		ptDraw.y = r * cos(eq) + ptDrawOrg.y;
		pDC->LineTo(ptDraw);
		break;

	case NCDRAWVIEW_XZ:
		r *= m_dFactorXZ;
		dHelical  *= m_dFactorXZ;
		ptDrawOrg *= m_dFactorXZ;
		ptDraw.x = ptDrawOrg.x;
		ptDraw.y = r * sin(sq) + ptDrawOrg.z;
		pDC->MoveTo(ptDraw);
		if ( m_nG23 == 0 ) {
			for ( sq-=ARCSTEP; sq>eq; sq-=ARCSTEP ) {
				ptDraw.x += dHelical;
				ptDraw.y  = r * sin(sq) + ptDrawOrg.z;
				pDC->LineTo(ptDraw);
			}
		}
		else {
			for ( sq+=ARCSTEP; sq<eq; sq+=ARCSTEP ) {
				ptDraw.x += dHelical;
				ptDraw.y  = r * sin(sq) + ptDrawOrg.z;
				pDC->LineTo(ptDraw);
			}
		}
		ptDraw.x = m_ptValE.x * m_dFactorXZ;
		ptDraw.y = r * sin(eq) + ptDrawOrg.z;
		pDC->LineTo(ptDraw);
		break;

	case NCDRAWVIEW_YZ:	// Don't ARC()
		r *= m_dFactorYZ;
		ptDrawOrg *= m_dFactorYZ;
		ptDraw.x = r * cos(sq) + ptDrawOrg.y;
		ptDraw.y = r * sin(sq) + ptDrawOrg.z;
		pDC->MoveTo(ptDraw);
		if ( m_nG23 == 0 ) {
			for ( sq-=ARCSTEP; sq>eq; sq-=ARCSTEP ) {
				ptDraw.x = r * cos(sq) + ptDrawOrg.y;
				ptDraw.y = r * sin(sq) + ptDrawOrg.z;
				pDC->LineTo(ptDraw);
			}
		}
		else {
			for ( sq+=ARCSTEP; sq<eq; sq+=ARCSTEP ) {
				ptDraw.x = r * cos(sq) + ptDrawOrg.y;
				ptDraw.y = r * sin(sq) + ptDrawOrg.z;
				pDC->LineTo(ptDraw);
			}
		}
		ptDraw.x = r * cos(eq) + ptDrawOrg.y;
		ptDraw.y = r * sin(eq) + ptDrawOrg.z;
		pDC->LineTo(ptDraw);
		break;
	}
}

CRect3D CNCcircle::GetMaxRect(void) const
{
#ifdef _DEBUG
	CMagaDbg	dbg("GetMaxRect()", DBG_RED);
#endif

	CRect3D	rcResult;
	// �O�ڂ���l�p�`
	double	r = fabs(m_r), sq = m_sq, eq = m_eq;
	CRectD	rcMax;
	CPointD	ptInit[4];

	// �n�_�E�I�_�̊J�n�ʒu
	// m_ptValS, m_ptValE ���g���ƁC���ʂ��Ƃ̏������K�v�Ȃ̂�
	// m_ptOrg �����_(0,0) �Ƃ����n�_�I�_���v�Z
	CPointD	pts(r*cos(sq), r*sin(sq));
	CPointD	pte(r*cos(eq), r*sin(eq));

	// �e�ی��̎��ő�l
	ptInit[0].SetPoint(  r,  0 );
	ptInit[1].SetPoint(  0,  r );
	ptInit[2].SetPoint( -r,  0 );
	ptInit[3].SetPoint(  0, -r );

	// �Q�_�̋�`�͕K���ʂ�̂ŁC
	// �����l�Ƃ��čő�l�E�ŏ��l����
	// �޶�č��W�Ȃ̂ŁAtop��bottom�͋t
	tie(rcMax.left,   rcMax.right) = minmax(pts.x, pte.x);
	tie(rcMax.bottom, rcMax.top)   = minmax(pts.y, pte.y);

	// �p�x�̒����ƊJ�n�I���ی�(i,j)�̐ݒ�
	int	i = 0, j = 0;
	while ( sq >= RAD(90.0) ) {
		sq -= RAD(90.0);
		i++;
	}
	while ( eq >= RAD(90.0) ) {
		eq -= RAD(90.0);
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
		if ( rcMax.left > ptInit[a].x )
			rcMax.left = ptInit[a].x;
		if ( rcMax.top < ptInit[a].y )
			rcMax.top = ptInit[a].y;
		if ( rcMax.right < ptInit[a].x )
			rcMax.right = ptInit[a].x;
		if ( rcMax.bottom > ptInit[a].y )
			rcMax.bottom = ptInit[a].y;
	}
	rcMax.NormalizeRect();

	// ��Ԑ�L��`���W�ݒ�
	switch ( GetPlane() ) {
	case XY_PLANE:
		rcMax.OffsetRect(m_ptOrg.GetXY());
		rcResult.left	= rcMax.left;
		rcResult.top	= rcMax.top;
		rcResult.right	= rcMax.right;
		rcResult.bottom	= rcMax.bottom;
		rcResult.low	= m_ptValS.z;
		rcResult.high	= m_ptValE.z;
		break;
	case XZ_PLANE:
		rcMax.OffsetRect(m_ptOrg.GetXZ());
		rcResult.left	= rcMax.left;
		rcResult.top	= m_ptValS.y;
		rcResult.right	= rcMax.right;
		rcResult.bottom	= m_ptValE.y;
		rcResult.low	= rcMax.top;
		rcResult.high	= rcMax.bottom;
		break;
	case YZ_PLANE:
		rcMax.OffsetRect(m_ptOrg.GetYZ());
		rcResult.left	= m_ptValS.x;
		rcResult.top	= rcMax.left;
		rcResult.right	= m_ptValE.x;
		rcResult.bottom	= rcMax.right;
		rcResult.low	= rcMax.top;
		rcResult.high	= rcMax.bottom;
		break;
	}
	rcResult.NormalizeRect();

#ifdef _DEBUGOLD
	dbg.printf("rcResult(left, top   )=(%f, %f)", rcResult.left, rcResult.top);
	dbg.printf("rcResult(right,bottom)=(%f, %f)", rcResult.right, rcResult.bottom);
	dbg.printf("rcResult(high, low   )=(%f, %f)", rcResult.high, rcResult.low);
#endif

	return rcResult;
}

tuple<BOOL, CPointD, double, double> CNCcircle::CalcRoundPoint
	(const CNCdata* pNext, double r) const
{
	// �Q���̌�_(���g�̏I�_)�����_�ɂȂ�悤�ɕ␳
	CPointD	pt, pts( GetPlaneValueOrg(m_ptOrg, m_ptValE) ), pte;
	double	rr1, rr2, xa, ya, r0 = fabs(m_r);
	int		nResult;
	BOOL	bResult = TRUE;

	if ( pNext->GetType() == NCDARCDATA ) {
		const CNCcircle* pCircle = static_cast<const CNCcircle *>(pNext);
		int	nG23next = 1 - pCircle->GetG23();	// ��_�ւ̐i���͔��Ή�]
		pte = GetPlaneValueOrg(pCircle->GetOrg(), m_ptValE);
		CPointD	p1, p2;
		double	r1, r2, rn = fabs(pCircle->GetR());
		// ���g�̉~�Ƒ����̐ڐ��Ŏ��g�́}r�������v�Z
		optional<double> dResult = CalcRoundPoint_CircleInOut(pts, pte, m_nG23, nG23next, r);
		if ( dResult )
			r1 = *dResult;
		else {
			bResult = FALSE;
			return make_tuple(bResult, pt, rr1, rr2);
		}
		// �����̉~�Ǝ��g�̐ڐ��ő����́}r�������v�Z
		dResult = CalcRoundPoint_CircleInOut(pte, pts, nG23next, m_nG23, r);
		if ( dResult )
			r2 = *dResult;
		else {
			bResult = FALSE;
			return make_tuple(bResult, pt, rr1, rr2);
		}
		// �Q�̉~�̌�_�����߂� -> �����Q�Ȃ��Ɩʎ��o���Ȃ��Ɣ��f����
		rr1 = r0 + r1;		rr2 = rn + r2;
		tie(nResult, p1, p2) = ::CalcIntersectionPoint_CC(pts, pte, rr1, rr2);
		if ( nResult != 2 ) {
			bResult = FALSE;
			return make_tuple(bResult, pt, rr1, rr2);
		}
		// ���̑I��
		double	sx = fabs(pts.x), sy = fabs(pts.y), ex = fabs(pte.x), ey = fabs(pte.y);
		if ( (sx<NCMIN && ex<NCMIN) || (sy<NCMIN && ey<NCMIN) ||
				(sx>NCMIN && ex>NCMIN && fabs(pts.y/pts.x - pte.y/pte.x)<NCMIN) ) {
			// ���S���������ɂ���Ƃ��C�ڐ��ƕ�������������I��
			if ( sy < NCMIN )
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
		rr1 = _hypot(xa, ya);
		if ( r2 > 0 ) {
			xa = (pt.x*rn+pte.x*r2) / rr2;
			ya = (pt.y*rn+pte.y*r2) / rr2;
		}
		else {
			r2 = fabs(r2);
			xa = (pt.x*rn-pte.x*r2) / rr2;
			ya = (pt.y*rn-pte.y*r2) / rr2;
		}
		rr2 = _hypot(xa, ya);
	}
	else {
		pte = GetPlaneValueOrg(pNext->GetEndPoint(), m_ptValE);
		// �̾�ĕ���������
		int nG23 = 1 - m_nG23,
			nOffset = CalcRoundPoint_OffsetFlag(pte, pts, nG23);
		if ( nOffset == 0 )
			return make_tuple(bResult, pt, rr1, rr2);
		// �̾�ĕ����s�ړ���������_�����߂�
		optional<CPointD> ptResult = ::CalcOffsetIntersectionPoint_LC(pte, pts,	// ��������̱��۰���
								r0, r, nG23, nOffset>0);						// ��]�����𔽓]
		if ( !ptResult )
			return make_tuple(bResult, pt, rr1, rr2);
		pt = *ptResult;
		// �ʎ��ɑ�������C�l�̌v�Z
		if ( nOffset > 0 )
			nOffset = nG23==0 ? 1 : -1;
		else
			nOffset = nG23==0 ? -1 : 1;
		rr1 = r0 + r*nOffset;
		if ( nOffset > 0 ) {
			// �����_
			xa = (pt.x*r0+pts.x*r) / rr1;
			ya = (pt.y*r0+pts.y*r) / rr1;
		}
		else {
			// �O���_
			xa = (pt.x*r0-pts.x*r) / rr1;
			ya = (pt.y*r0-pts.y*r) / rr1;
		}
		rr1 = _hypot(xa, ya);
		rr2 = sqrt(pt.x*pt.x + pt.y*pt.y - r*r);
	}

	// ���_�␳
	pt += GetPlaneValue(m_ptValE);

	return make_tuple(bResult, pt, rr1, rr2);
}

optional<CPointD> CNCcircle::SetChamferingPoint(BOOL bStart, double c)
{
	CPoint3D	ptOrg3D( bStart ? m_ptValS : m_ptValE );
	CPointD		pt, ptOrg1, ptOrg2, pt1, pt2;
	double		pa, pb, ps;

	switch ( GetPlane() ) {	// ���������̂� GetPlaneValue() �͎g��Ȃ�
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
	}

	// ����������Ȃ��Ƃ��ʹװ
	if ( m_eq - m_sq > RAD(180.0) ) {
		// 180���𒴂���Ƃ��͒��a�Ɣ�r
		if ( c >= fabs(m_r)*2 )
			return optional<CPointD>();
	}
	else {
		// 180�������̏ꍇ�͌��̒����Ɣ�r
		if ( c >= _hypot(pt1.x-pt2.x, pt1.y-pt2.y) )
			return optional<CPointD>();
	}

	// �Q�̉~�̌�_�����߂� -> �����Q�Ȃ��Ɩʎ��o���Ȃ��Ɣ��f����
	int	nResult;
	tie(nResult, pt1, pt2) = ::CalcIntersectionPoint_CC(ptOrg1, ptOrg2, fabs(m_r), c);
	if ( nResult != 2 )
		return optional<CPointD>();

	// ���v���̏ꍇ�C�n�p�ƏI�p������ւ���Ă���̂ňꎞ�I�Ɍ��ɖ߂�
	if ( m_nG23 == 0 )
		std::swap(m_sq, m_eq);

	// ���̑I��
	ps = bStart ? m_sq : m_eq;
	if ( (pa=atan2(pt1.y-ptOrg1.y, pt1.x-ptOrg1.x)) < 0.0 )
		pa += RAD(360.0);
	if ( (pb=atan2(pt2.y-ptOrg1.y, pt2.x-ptOrg1.x)) < 0.0 )
		pb += RAD(360.0);
	// 180�x�ȏ�̍��͕␳
	if ( fabs(ps-pa) > RAD(180.0) ) {
		if ( ps > pa )
			ps -= RAD(360.0);
		else
			pa -= RAD(360.0);
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
		SetPlaneValue(pt, m_ptValS);
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
		SetPlaneValue(pt, m_ptValE);
		m_pRead->m_ptValOrg = m_ptValE;
		if ( m_pRead->m_g68.bG68 ) {
			// m_ptValE ��G68��]�ςݍ��W�̂��߉�]�����ɖ߂��ĵ̾�Č��Z
			m_pRead->m_g68.dRound = -m_pRead->m_g68.dRound;
			CalcG68Round(&(m_pRead->m_g68), m_pRead->m_ptValOrg);
			m_pRead->m_g68.dRound = -m_pRead->m_g68.dRound;
		}
		m_pRead->m_ptValOrg -= m_pRead->m_ptOffset;
		m_eq = pa;
		m_pt2D = m_ptValE.PointConvert();
	}

	// �p�x�␳
	if ( m_nG23 == 0 )
		std::swap(m_sq, m_eq);
	ps = ::RoundUp(DEG(m_sq));
	while ( ps >= ::RoundUp(DEG(m_eq)) )
		m_eq += RAD(360.0);

	return pt;
}

double CNCcircle::CalcBetweenAngle(const CNCdata* pNext) const
{
	// �Q���̌�_(���g�̏I�_)�����_�ɂȂ�悤�ɕ␳
	CPointD		pt( GetPlaneValueOrg(m_ptOrg, m_ptValE) ), pt1, pt2;

	// �I�_�ڐ��v�Z
	int k = -CalcOffsetSign();
	pt1.x = -pt.y*k;	// G02:-90��
	pt1.y =  pt.x*k;	// G03:+90��

	if ( pNext->GetType() == NCDARCDATA ) {
		const CNCcircle* pCircle = static_cast<const CNCcircle *>(pNext);
		// ���̵�޼ު�Ă��~�ʂȂ璆�S��Ă�
		pt = GetPlaneValueOrg(pCircle->GetOrg(), m_ptValE);
		// �n�_�ڐ��v�Z
		k = pCircle->CalcOffsetSign();
		pt2.x = -pt.y*k;
		pt2.y =  pt.x*k;
	}
	else {
		pt2 = GetPlaneValueOrg(pNext->GetEndPoint(), m_ptValE);
	}

	// �Q��(�܂��͉~�ʂ̐ڐ�)���Ȃ��p�x�����߂�
	return ::CalcBetweenAngle(pt1, pt2);
}

int CNCcircle::CalcOffsetSign(void) const
{
	// ��]��������G41�̕␳���������߂�
	return m_nG23==0 ? 1 : -1;
}

optional<CPointD> CNCcircle::CalcPerpendicularPoint
	(ENPOINTORDER enPoint, double r, int nSign) const
{
	const CPoint3D	pts( enPoint==STARTPOINT ? m_ptValS : m_ptValE );
	// �n�_�I�_�֌W�Ȃ� ��]�������␳����
	// pts�ƒ��S�̌X�����v�Z���Ĕ��a�}r
	CPointD	pt( GetPlaneValueOrg(pts, m_ptOrg) );
	double	q = atan2(pt.y, pt.x), rr = fabs(m_r) + r * CalcOffsetSign() * nSign;
	CPointD	pt1(rr*cos(q), rr*sin(q));
	pt1 += GetPlaneValue(m_ptOrg);

	return pt1;
}

optional<CPointD> CNCcircle::CalcOffsetIntersectionPoint
	(const CNCdata* pNext, double r, BOOL bLeft) const
{
	BOOL	bResult = FALSE;
	// �Q���̌�_(���g�̏I�_)�����_�ɂȂ�悤�ɕ␳
	CPointD	pt;

	if ( pNext->GetType() == NCDARCDATA ) {
		const CNCcircle* pCircle = static_cast<const CNCcircle *>(pNext);
		CPointD	pto1( GetPlaneValueOrg(m_ptOrg, m_ptValE) ),
				pto2( GetPlaneValueOrg(pCircle->GetOrg(), m_ptValE) ),
				p1, p2;
		int		k1 = CalcOffsetSign(),
				k2 = pCircle->CalcOffsetSign();
		if ( !bLeft ) {
			k1 = -k1;
			k2 = -k2;
		}
		double	r1 = fabs(m_r)+r*k1, r2 = fabs(pCircle->GetR())+r*k2;
		// ����~�����f
		if ( pto1.IsMatchPoint(&pto2) && fabs(r1-r2)<NCMIN ) {
			// �~�̌�_�͋��߂��Ȃ��̂ŁA�P���̾�č��W�v�Z
			double	q = (m_nG23==0) ? m_sq : m_eq;	// �I�_�p�x
			CPointD	pto( GetPlaneValue(m_ptOrg) );
			pt.x = r1 * cos(q) + pto.x;
			pt.y = r1 * sin(q) + pto.y;
			return pt;	// m_ptValE �̕␳�s�v
		}
		else {
			// �̾�ĕ����a�𒲐߂��ĂQ�̉~�̌�_�����߂�
			int		nResult;
			tie(nResult, p1, p2) = ::CalcIntersectionPoint_CC(pto1, pto2, r1, r2);
			// ���̑I��
			if ( nResult > 1 ) {
				pt = GAPCALC(p1) < GAPCALC(p2) ? p1 : p2;
				bResult = TRUE;
			}
			else if ( nResult > 0 ) {
				pt = p1;
				bResult = TRUE;
			}
		}
	}
	else {
		// �̾�ĕ����s�ړ���������_�����߂�
		CPointD	pt1( GetPlaneValueOrg(pNext->GetEndPoint(), m_ptValE) ),
				pt2( GetPlaneValueOrg(m_ptOrg, m_ptValE) );
		// ������̱��۰��ŉ�]�����𔽓]
		optional<CPointD> ptResult = ::CalcOffsetIntersectionPoint_LC(pt1, pt2,
								fabs(m_r), r, 1-GetG23(), !bLeft);
		if ( ptResult ) {
			pt = *ptResult;
			bResult = TRUE;
		}
	}

	// ���_�␳
	if ( bResult ) {
		pt += GetPlaneValue(m_ptValE);
		return pt;
	}

	return optional<CPointD>();
}

optional<CPointD> CNCcircle::CalcOffsetIntersectionPoint2
	(const CNCdata* pNext, double r, BOOL bLeft) const
{
	int		k;
	// �Q���̌�_(���g�̏I�_)�����_�ɂȂ�悤�ɕ␳
	CPointD	pt, pt1, pt2;

	// �n�_�ڐ����W
	pt = GetPlaneValueOrg(m_ptOrg, m_ptValE);
	k = -CalcOffsetSign();
	pt1.x = -pt.y * k;
	pt1.y =  pt.x * k;

	// �����̍��W�v�Z
	if ( pNext->GetType() == NCDARCDATA ) {
		const CNCcircle* pCircle = static_cast<const CNCcircle *>(pNext);
		pt = GetPlaneValueOrg(pCircle->GetOrg(), m_ptValE);
		k = pCircle->CalcOffsetSign();
		pt2.x = -pt.y * k;
		pt2.y =  pt.x * k;
	}
	else {
		pt2 = GetPlaneValueOrg(pNext->GetEndPoint(), m_ptValE);
	}

	// �������m�̵̾�Č�_�v�Z
	optional<CPointD> ptResult = ::CalcOffsetIntersectionPoint_LL(pt1, pt2, r, bLeft);
	// ���_�␳
	if ( ptResult ) {
		pt = *ptResult + GetPlaneValue(m_ptValE);
		return pt;
	}
	return ptResult;
}

void CNCcircle::SetCorrectPoint(ENPOINTORDER enPoint, const CPointD& ptSrc, double rr)
{
#ifdef _DEBUG
	CMagaDbg	dbg("SetCorrectPoint()", DBG_MAGENTA);
#endif
	CPoint3D&	ptVal = enPoint==STARTPOINT ? m_ptValS : m_ptValE;	// �Q�ƌ^
	CPointD		pt;

	SetPlaneValue(ptSrc, ptVal);
	pt = GetPlaneValueOrg(ptVal, m_ptOrg);

	// �p�x����
	if ( enPoint == STARTPOINT ) {
		double&	q = m_nG23==0 ? m_eq : m_sq;	// �Q�ƌ^
		if ( (q=atan2(pt.y, pt.x)) < 0.0 )
			q += RAD(360.0);
	}
	else {
		m_r = _copysign(fabs(m_r)+rr, m_r);		// �I�_�̎��������a�␳
		double&	q = m_nG23==0 ? m_sq : m_eq;
		if ( (q=atan2(pt.y, pt.x)) < 0.0 )
			q += RAD(360.0);
		m_pt2D = m_ptValE.PointConvert();
	}
	double	sq = ::RoundUp(DEG(m_sq));
	while ( sq >= ::RoundUp(DEG(m_eq)) )
		m_eq += RAD(360.0);
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

// �̾�ĕ����̌���
int CalcRoundPoint_OffsetFlag(const CPointD& pts, const CPointD& pto, int nRound)
{
	// ��]����
	int		k = nRound==0 ? 1 : -1;		// G02:+90��,G03:-90��
	// �����̊p�x
	double	q = atan2(pts.y, pts.x);
	// �~�̐ڐ�
	CPointD	pte(-pto.y*k, pto.x);
	// �����̊p�x�ŕ␳
	pte.RoundPoint(-q);
	// y �̕����ŵ̾�ĕ����𔻒f
	if ( fabs(pte.y) < NCMIN )
		k = 0;		// ���Ȃ�
	else if ( pte.y > 0.0 )
		k = -1;		// �E��
	else
		k = 1;		// ����

	return k;
}

// �~���m�̓��O���a�v�Z
optional<double> CalcRoundPoint_CircleInOut
	(const CPointD& pts, const CPointD& pte, int nG23, int nG23next, double r)
{
	CPointD	pto;
	double	sx = fabs(pts.x), sy = fabs(pts.y), ex = fabs(pte.x), ey = fabs(pte.y), rr;
	int		k1 = nG23==0 ? -1 : 1, k2 = nG23next==0 ? -1 : 1;

	// ������̔��f
	if ( (sx<NCMIN && ex<NCMIN && pts.y*pte.y>0) ||
			(sy<NCMIN && ey<NCMIN && pts.x*pte.x>0) ||
			(sx>NCMIN && ex>NCMIN && fabs(pts.y/pts.x - pte.y/pte.x)<NCMIN && pts.x*pte.x>0) ) {
		// ���S���������ɂ���C���Cx,y �̕����������Ƃ�
		double	l1 = pts.x*pts.x + pts.y*pts.y;
		double	l2 = pte.x*pte.x + pte.y*pte.y;
		if ( fabs(l1 - l2) < NCMIN )	// ������������==���O�Չ~
			return optional<double>();
		else if ( l1 > l2 )
			return -r;
		else
			return r;
	}
	// ���g�̉~�Ƒ����̐ڐ��Ŏ��g�́}r�������v�Z
	pto.x = -pte.y*k2;	// G02:-90��
	pto.y =  pte.x*k2;	// G03:+90��

	// ���Ɖ~�ʂ̏ꍇ�ƍl����(�������@)�͓���
	if ( fabs(pto.x) < NCMIN && sy < NCMIN )
		rr = _copysign(r, pto.y*pts.x*k1);
	else if ( fabs(pto.y) < NCMIN && sx < NCMIN )
		rr = _copysign(r, -pto.x*pts.y*k1);
	else
		rr = _copysign(r, -(pto.x*pts.x + pto.y*pts.y));

	return rr;
}
