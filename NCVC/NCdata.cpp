// NCdata.cpp: CNCdata �N���X�̃C���v�������e�[�V����
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NCVC.h"
#include "NCdata.h"
#include "ViewOption.h"

//#define	_DEBUGDRAW_NCD		// �`�揈����۸�
#ifdef _DEBUG
#define new DEBUG_NEW
#ifdef _DEBUG_DUMP
#include "boost/format.hpp"
using std::string;
#endif
#endif

using namespace boost;
using namespace boost::core;

extern	const	DWORD		g_dwSetValFlags[];	// NCDoc.cpp

// CalcRoundPoint() ���
// --- �̾�ĕ����̌���
static	int		_CalcRoundPoint_OffsetFlag(const CPointF&, const CPointF&, BOOL);
// --- �~���m�̓��O���a�v�Z
static	optional<float>	_CalcRoundPoint_CircleInOut(const CPointF&, const CPointF&, BOOL, BOOL, float);

//////////////////////////////////////////////////////////////////////

G68ROUND_F::G68ROUND_F(const G68ROUND_F* pG68) {
	enPlane	= pG68->enPlane;
	dRound	= pG68->dRound;
	for ( int i=0; i<SIZEOF(dOrg); i++ )
		dOrg[i]	= pG68->dOrg[i];
}

G68ROUND_F::G68ROUND_F(const G68ROUND& G68)
{
	enPlane	= G68.enPlane;
	dRound	= (float)G68.dRound;
	for ( int i=0; i<SIZEOF(dOrg); i++ )
		dOrg[i]	= (float)G68.dOrg[i];
}

TAPER_F::TAPER_F(const TAPER_F* pTP)
{
	nTaper	= pTP->nTaper;
	dTaper	= pTP->dTaper;
	nDiff	= pTP->nDiff;
	bTonly	= pTP->bTonly;
}

TAPER_F::TAPER_F(const TAPER& TP)
{
	nTaper	= TP.nTaper;
	dTaper	= (float)TP.dTaper;
	nDiff	= TP.nDiff;
	bTonly	= TP.bTonly;
}

CNCread::~CNCread()
{
	if ( m_pG68 )
		delete	m_pG68;
	if ( m_pTaper )
		delete	m_pTaper;
}

//////////////////////////////////////////////////////////////////////
// NC�ް��̊�b�ް��׽
//////////////////////////////////////////////////////////////////////

// ����o�^�p�ݽ�׸�
CNCdata::CNCdata(LPNCARGV lpArgv)
{
	int		i;

	Constracter(lpArgv);

	for ( i=0; i<VALUESIZE; i++ )
		m_nc.dValue[i] = (float)lpArgv->nc.dValue[i];
	for ( i=0; i<NCXYZ; i++ )
		m_pRead->m_ptValOrg[i] = m_ptValS[i] = m_ptValE[i] = (float)lpArgv->nc.dValue[i];
	m_pt2D = m_ptValE.PointConvert();

	m_enType = NCDBASEDATA;
}

// �؍�(�`��)���ވȊO�̺ݽ�׸�
CNCdata::CNCdata(const CNCdata* pData, LPNCARGV lpArgv, const CPoint3F& ptOffset)
{
	int		i;

	Constracter(lpArgv);

	// ���W�w��̂Ȃ��ް��͑O��v�Z���W����擾
	for ( i=0; i<NCXYZ; i++ ) {
		// �w�肳��Ă��镪�������(XYZ�̂�)
		m_nc.dValue[i] = lpArgv->nc.dwValFlags & g_dwSetValFlags[i] ?
			(float)lpArgv->nc.dValue[i] : pData->GetOriginalEndValue(i);
	}
	// ���W�l�ȊO(UVW�܂�)���w�肳��Ă��镪�͑��
	for ( ; i<VALUESIZE; i++ )
		m_nc.dValue[i] = lpArgv->nc.dwValFlags & g_dwSetValFlags[i] ?
//			lpArgv->nc.dValue[i] : pData->GetValue(i);
			(float)lpArgv->nc.dValue[i] : 0.0f;

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
	(ENNCDTYPE enType, const CNCdata* pData, LPNCARGV lpArgv, const CPoint3F& ptOffset)
{
	int		i;

	Constracter(lpArgv);

	// ���W�w��̂Ȃ��ް��͑O�񏃐����W������
	for ( i=0; i<NCXYZ; i++ ) {
		// �w�肳��Ă��镪�������(XYZ�̂�)
		if ( lpArgv->nc.dwValFlags & g_dwSetValFlags[i] ) {
			m_nc.dValue[i] = (float)lpArgv->nc.dValue[i];
			if ( !lpArgv->bAbs )		// �ݸ����ٕ␳
				m_nc.dValue[i] += pData->GetOriginalEndValue(i);	// �ؼ��ْl�ŉ��Z
		}
		else
			m_nc.dValue[i] = pData->GetOriginalEndValue(i);
	}
	// ��L�ȊO
	for ( ; i<VALUESIZE; i++ ) {
		m_nc.dValue[i] = lpArgv->nc.dwValFlags & g_dwSetValFlags[i] ?
//			lpArgv->nc.dValue[i] : pData->GetValue(i);
			(float)lpArgv->nc.dValue[i] : 0.0f;
	}
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
	m_enType		= pData->GetType();
	m_dwFlags		= pData->GetFlags();
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
	m_ptValS = pData->GetStartPoint();
	m_ptValE = pData->GetEndPoint();
	m_pRead = new CNCread;
	m_pRead->m_ptOffset = pData->GetOffsetPoint();
	m_pRead->m_ptValOrg = pData->GetOriginalEndPoint();
//	memcpy(&(m_pRead->m_g68),   &(pData->GetReadData()->m_g68),   sizeof(G68ROUND));
//	memcpy(&(m_pRead->m_taper), &(pData->GetReadData()->m_taper), sizeof(TAPER));
	if ( pData->GetReadData()->m_pG68 )
		m_pRead->m_pG68 = new G68ROUND_F(pData->GetReadData()->m_pG68);
	else
		m_pRead->m_pG68 = NULL;
	if ( pData->GetReadData()->m_pTaper )
		m_pRead->m_pTaper = new TAPER_F(pData->GetReadData()->m_pTaper);
	else
		m_pRead->m_pTaper = NULL;
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

CPointF CNCdata::GetPlaneValue(const CPoint3F& ptVal) const
{
	CPointF	pt;
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

void CNCdata::SetPlaneValue(const CPointF& pt, CPoint3F& ptResult) const
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

CPointF	CNCdata::GetPlaneValueOrg(const CPoint3F& pt1, const CPoint3F& pt2) const
{
	CPointF	pt;
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

void CNCdata::CalcG68Round(LPG68ROUND lpG68, CPoint3F& ptResult) const
{
	CPoint3F	ptOrg((float)lpG68->dOrg[NCA_X], (float)lpG68->dOrg[NCA_Y], (float)lpG68->dOrg[NCA_Z]);
	CPointF		pt(GetPlaneValueOrg(ptResult, ptOrg));
	pt.RoundPoint((float)lpG68->dRound);
	SetPlaneValue(pt, ptResult);
	ptResult += ptOrg;
}

void CNCdata::CalcG68Round(LPG68ROUND_F lpG68, CPoint3F& ptResult) const
{
	CPoint3F	ptOrg(lpG68->dOrg[NCA_X], lpG68->dOrg[NCA_Y], lpG68->dOrg[NCA_Z]);
	CPointF		pt(GetPlaneValueOrg(ptResult, ptOrg));
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

void CNCdata::DrawTuning(float f)
{
	for ( int i=0; i<m_obCdata.GetSize(); i++ )
		m_obCdata[i]->DrawTuning(f);
	if ( m_pWireObj )
		m_pWireObj->DrawTuning(f);
}

void CNCdata::DrawTuningXY(float f)
{
	for ( int i=0; i<m_obCdata.GetSize(); i++ )
		m_obCdata[i]->DrawTuningXY(f);
	if ( m_pWireObj )
		m_pWireObj->DrawTuningXY(f);
}

void CNCdata::DrawTuningXZ(float f)
{
	for ( int i=0; i<m_obCdata.GetSize(); i++ )
		m_obCdata[i]->DrawTuningXZ(f);
	if ( m_pWireObj )
		m_pWireObj->DrawTuningXZ(f);
}

void CNCdata::DrawTuningYZ(float f)
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

CPoint CNCdata::GetDrawStartPoint(size_t n) const
{
	ASSERT( FALSE );	// �װ
	return CPoint();
}

CPoint CNCdata::GetDrawEndPoint(size_t n) const
{
	ASSERT( FALSE );
	return CPoint();
}

void CNCdata::DrawWireLine(ENNCDRAWVIEW enDraw, CDC* pDC, BOOL bSelect) const
{
	ASSERT( FALSE );
}

#ifdef _DEBUG_DUMP
void CNCdata::DbgDump(void)
{
	extern	LPCTSTR	g_szGdelimiter;	// "GSMOF" from NCDoc.cpp
	extern	LPCTSTR	g_szNdelimiter; // "XYZUVWIJKRPLDH";

	string	strBuf;
	if ( GetGtype()<0 || GetGtype()>GTYPESIZE )
		strBuf = "NO_TYPE:" + lexical_cast<string>(GetGcode());
	else
		strBuf = str(format("%c%02d") % g_szGdelimiter[GetGtype()] % GetGcode());
	for ( int i=0; i<VALUESIZE; i++ ) {
		if ( GetValFlags() & g_dwSetValFlags[i] )
			strBuf += str(format("%c%.3f") % g_szNdelimiter[i] % GetValue(i));
	}
	printf("CNCdata %s", strBuf.c_str());
}
#endif

//////////////////////////////////////////////////////////////////////
// CNCline �N���X
//////////////////////////////////////////////////////////////////////

CNCline::CNCline(const CNCdata* pData, LPNCARGV lpArgv, const CPoint3F& ptOffset) :
	CNCdata(NCDLINEDATA, pData, lpArgv, ptOffset)
{
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

float CNCline::SetCalcLength(void)
{
	CPoint3F	pt;

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
		m_nc.dLength = 0.0f;
		ZEROCLR(m_dMove);
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

	float	dResult = 0;
	if ( m_pWireObj )
		dResult = m_pWireObj->SetCalcLength();

	return max((float)m_nc.dLength, dResult);
}

void CNCline::DrawTuning(float f)
{
	m_ptDrawS[NCDRAWVIEW_XYZ] = m_pt2Ds * f;
	m_ptDrawE[NCDRAWVIEW_XYZ] = m_pt2D  * f;
	CNCdata::DrawTuning(f);
}

void CNCline::DrawTuningXY(float f)
{
	m_ptDrawS[NCDRAWVIEW_XY] = m_ptValS.GetXY() * f;
	m_ptDrawE[NCDRAWVIEW_XY] = m_ptValE.GetXY() * f;
	CNCdata::DrawTuningXY(f);
}

void CNCline::DrawTuningXZ(float f)
{
	m_ptDrawS[NCDRAWVIEW_XZ] = m_ptValS.GetXZ() * f;
	m_ptDrawE[NCDRAWVIEW_XZ] = m_ptValE.GetXZ() * f;
	CNCdata::DrawTuningXZ(f);
}

void CNCline::DrawTuningYZ(float f)
{
	m_ptDrawS[NCDRAWVIEW_YZ] = m_ptValS.GetYZ() * f;
	m_ptDrawE[NCDRAWVIEW_YZ] = m_ptValE.GetYZ() * f;
	CNCdata::DrawTuningYZ(f);
}

void CNCline::Draw(CDC* pDC, BOOL bSelect) const
{
#ifdef _DEBUGDRAW_NCD
	printf("Line Draw()=%d\n", GetBlockLineNo()+1);
#endif
	if ( m_obCdata.IsEmpty() ||
			AfxGetNCVCApp()->GetViewOption()->GetNCViewFlg(GLOPTFLG_DRAWREVISE) )
		DrawLine(NCDRAWVIEW_XYZ, pDC, bSelect);
	CNCdata::Draw(pDC, bSelect);
}

void CNCline::DrawXY(CDC* pDC, BOOL bSelect) const
{
#ifdef _DEBUGDRAW_NCD
	printf("Line DrawXY()=%d\n", GetBlockLineNo()+1);
#endif
	if ( m_obCdata.IsEmpty() ||
			AfxGetNCVCApp()->GetViewOption()->GetNCViewFlg(GLOPTFLG_DRAWREVISE) )
		DrawLine(NCDRAWVIEW_XY, pDC, bSelect);
	CNCdata::DrawXY(pDC, bSelect);
}

void CNCline::DrawXZ(CDC* pDC, BOOL bSelect) const
{
#ifdef _DEBUGDRAW_NCD
	printf("Line DrawXZ()=%d\n", GetBlockLineNo()+1);
#endif
	if ( m_obCdata.IsEmpty() ||
			AfxGetNCVCApp()->GetViewOption()->GetNCViewFlg(GLOPTFLG_DRAWREVISE) )
		DrawLine(NCDRAWVIEW_XZ, pDC, bSelect);
	CNCdata::DrawXZ(pDC, bSelect);
}

void CNCline::DrawYZ(CDC* pDC, BOOL bSelect) const
{
#ifdef _DEBUGDRAW_NCD
	printf("Line DrawYZ()=%d\n", GetBlockLineNo()+1);
#endif
	if ( m_obCdata.IsEmpty() ||
			AfxGetNCVCApp()->GetViewOption()->GetNCViewFlg(GLOPTFLG_DRAWREVISE) )
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

void CNCline::DrawLine(ENNCDRAWVIEW enDraw, CDC* pDC, BOOL bSelect) const
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

void CNCline::DrawWireLine(ENNCDRAWVIEW enDraw, CDC* pDC, BOOL bSelect) const
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
		// m_pWireObj��CNCline�Ƃ͌���Ȃ�
		m_pWireObj->DrawWireLine(enDraw, pDC, bSelect);
		// XY��UV�̐ڑ�
		pDC->MoveTo(m_ptDrawS[enDraw]);
		pDC->LineTo(m_pWireObj->GetDrawStartPoint(enDraw));
		pDC->MoveTo(m_ptDrawE[enDraw]);
		pDC->LineTo(m_pWireObj->GetDrawEndPoint(enDraw));
	}
	pDC->SelectObject(pOldPen);
}

tuple<BOOL, CPointF, float, float> CNCline::CalcRoundPoint(const CNCdata* pNext, float r) const
{
	BOOL		bResult = FALSE;
	float		rr1, rr2;
	CPointF		pt, pts;

	if ( pNext->GetType() == NCDARCDATA ) {
		const CNCcircle* pCircle = static_cast<const CNCcircle *>(pNext);
		// �v�Z���_�␳
		pts = GetPlaneValueOrg(m_ptValS, m_ptValE);
		CPointF	ptc( GetPlaneValueOrg(pCircle->GetOrg(), m_ptValE) );
		// �̾�ĕ���������
		int nOffset = _CalcRoundPoint_OffsetFlag(pts, ptc, pCircle->GetG03());
		if ( nOffset != 0 ) {
			// �̾�ĕ����s�ړ���������_�����߂�
			float	rr, xa, ya, rn = fabs(pCircle->GetR());
			optional<CPointF> ptResult = ::CalcOffsetIntersectionPoint_LC(pts, ptc, rn, r, r,
							pCircle->GetG03(), nOffset>0);
			if ( ptResult ) {
				pt = *ptResult;
				// �ʎ��ɑ�������C�l�̌v�Z
				rr1 = sqrt(pt.x*pt.x + pt.y*pt.y - r*r);
				if ( nOffset > 0 )
					nOffset = pCircle->GetG03() ? -1 : 1;
				else
					nOffset = pCircle->GetG03() ? 1 : -1;
				rr = rn + r*nOffset;
				if ( nOffset > 0 ) {
					// �����_(+r)
					xa = (pt.x*rn+ptc.x*r) / rr;
					ya = (pt.y*rn+ptc.y*r) / rr;
				}
				else {
					// �O���_(-r)
					xa = (pt.x*rn-ptc.x*r) / rr;
					ya = (pt.y*rn-ptc.y*r) / rr;
				}
				rr2 = _hypotf(xa, ya);
				bResult = TRUE;
			}
		}
	}
	else {
		// �v�Z���_�␳
		pts = GetPlaneValueOrg(m_ptValS, m_ptValE);
		CPointF	pte( GetPlaneValueOrg(pNext->GetEndPoint(), m_ptValE) );
		// pts �� X����ɉ�]
		float	q = pts.arctan();
		pte.RoundPoint(-q);
		// �Q�̐��̊p�x���Q
		float	p = pte.arctan() / 2.0f,
				pp = fabs(p);
		if ( pp < RAD(90.0f) ) {
			pt.x = rr1 = rr2 = r / tan(pp);	// �ʎ��ɑ�������C�l�͉�]�����O��X���W�Ɠ���
			pt.y = copysign(r, p);			// y(����) = r�A�����͊p�x�ɂ��
			// ��]�𕜌�
			pt.RoundPoint(q);
			bResult = TRUE;
		}
	}

	if ( bResult )
		pt += GetPlaneValue(m_ptValE);

	return make_tuple(bResult, pt, rr1, rr2);
}

optional<CPointF> CNCline::SetChamferingPoint(BOOL bStart, float c)
{
	// ����������Ȃ��Ƃ��ʹװ
	if ( c >= SetCalcLength() )
		return boost::none;

	CPointF		pt;
	CPointF		pto, pte;
	CPoint3F&	ptValS = bStart ? m_ptValS : m_ptValE;	// ���������̂ŎQ�ƌ^(�ʖ�)
	CPoint3F&	ptValE = bStart ? m_ptValE : m_ptValS;
	
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
		if ( m_pRead->m_pG68 ) {
			// m_ptValE ��G68��]�ςݍ��W�̂��߉�]�����ɖ߂��ĵ̾�Č��Z
			m_pRead->m_pG68->dRound = -m_pRead->m_pG68->dRound;
			CalcG68Round(m_pRead->m_pG68, m_pRead->m_ptValOrg);
			m_pRead->m_pG68->dRound = -m_pRead->m_pG68->dRound;
		}
		m_pRead->m_ptValOrg -= m_pRead->m_ptOffset;	// m_ptValE��G68�̏ꍇ�A���������Ȃ�
		m_pt2D = m_ptValE.PointConvert();
	}

	return pt;
}

float CNCline::CalcBetweenAngle(const CNCdata* pNext) const
{
	// �Q���̌�_(���g�̏I�_)�����_�ɂȂ�悤�ɕ␳
	CPointF		pt1( GetPlaneValueOrg(m_ptValS, m_ptValE) ), pt2;

	if ( pNext->GetType() == NCDARCDATA ) {
		const CNCcircle* pCircle = static_cast<const CNCcircle *>(pNext);
		// ���̵�޼ު�Ă��~�ʂȂ璆�S��Ă�
		CPointF	pt( GetPlaneValueOrg(pCircle->GetOrg(), m_ptValE) );
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

optional<CPointF> CNCline::CalcPerpendicularPoint
	(ENPOINTORDER enPoint, float r, int nSign) const
{
	CPoint3F	pts, pte;
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
	CPointF	pt( GetPlaneValueOrg(pte, pts) );
	float	q = pt.arctan();
	CPointF	pt1(r*cos(q), r*sin(q));
	CPointF	pt2(-pt1.y*nSign, pt1.x*nSign);
	pt2 += GetPlaneValue(pts);

	return pt2;
}

optional<CPointF> CNCline::CalcOffsetIntersectionPoint
	(const CNCdata* pNext, float t1, float t2, BOOL bLeft) const
{
	optional<CPointF>	ptResult;
	// �Q���̌�_(���g�̏I�_)�����_�ɂȂ�悤�ɕ␳
	CPointF		pt, pt1( GetPlaneValueOrg(m_ptValS, m_ptValE) ), pt2;

	// �̾�ĕ����s�ړ���������_�����߂�
	if ( pNext->GetType() == NCDARCDATA ) {
		const CNCcircle* pCircle = static_cast<const CNCcircle *>(pNext);
		pt2 = GetPlaneValueOrg(pCircle->GetOrg(), m_ptValE);
		ptResult = ::CalcOffsetIntersectionPoint_LC(pt1, pt2, fabs(pCircle->GetR()), t1, t2,
						pCircle->GetG03(), bLeft);
	}
	else {
		pt2 = GetPlaneValueOrg(pNext->GetEndPoint(), m_ptValE);
		ptResult = ::CalcOffsetIntersectionPoint_LL(pt1, pt2, t1, t2, bLeft);
	}

	// ���_�␳
	if ( ptResult ) {
		pt = *ptResult + GetPlaneValue(m_ptValE);
		return pt;
	}

	return boost::none;
}

optional<CPointF> CNCline::CalcOffsetIntersectionPoint2
	(const CNCdata* pNext, float r, BOOL bLeft) const
{
	// �Q���̌�_(���g�̏I�_)�����_�ɂȂ�悤�ɕ␳
	CPointF		pt, pt1( GetPlaneValueOrg(m_ptValS, m_ptValE) ), pt2;

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
	optional<CPointF> ptResult = ::CalcOffsetIntersectionPoint_LL(pt1, pt2, r, r, bLeft);
	// ���_�␳
	if ( ptResult ) {
		pt  = *ptResult;
		pt += GetPlaneValue(m_ptValE);
		return pt;
	}
	return boost::none;
}

void CNCline::SetCorrectPoint(ENPOINTORDER enPoint, const CPointF& ptSrc, float)
{
	CPoint3F&	ptVal    = enPoint==STARTPOINT ? m_ptValS : m_ptValE;	// �Q�ƌ^
	CPointF&	ptResult = enPoint==STARTPOINT ? m_pt2Ds  : m_pt2D;

	SetPlaneValue(ptSrc, ptVal);
	ptResult = ptVal.PointConvert();

	if ( enPoint == ENDPOINT ) {
		ASSERT( m_pRead );
		SetPlaneValue(ptSrc, m_pRead->m_ptValOrg + m_pRead->m_ptOffset);
	}
}

//////////////////////////////////////////////////////////////////////
// CNCcycle �N���X
//////////////////////////////////////////////////////////////////////

CNCcycle::CNCcycle
	(const CNCdata* pData, LPNCARGV lpArgv, const CPoint3F& ptOffset, BOOL bL0Cycle, NCMAKETYPE enType) :
		CNCline(NCDCYCLEDATA, pData, lpArgv, ptOffset)
{
	//	!!! Z, R, P �l�́CTH_NCRead.cpp �ł���Ԃ��Ă��邱�Ƃɒ��� !!!
	CPoint3F	pt;
	float	dx, dy, dox, doy,	// ����ʂ̈ړ�����
			dR, dI,				// R�_���W, �Ƽ�ٍ��W
			dRLength, dZLength,	// �ړ����C�؍풷
			dResult;
	int		i, x, y, z,
			nH, nV;		// �c���̌J��Ԃ���

	// ������
	ZEROCLR(m_Cycle);	// m_Cycle[i++]=NULL
	m_Cycle3D = NULL;

	// ����ʂɂ����W�ݒ�
	switch ( GetPlane() ) {
	case XY_PLANE:
		x = NCA_X;
		y = NCA_Y;
		z = NCA_Z;
		break;
	case XZ_PLANE:
		if ( enType == NCMAKELATHE ) {
			// TH_NCRead.cpp �Ŏ��ϊ����Ă���̂ŁA����ɑΉ������␳
			x = NCA_Z;
			y = NCA_Z;
			z = NCA_X;
		}
		else {
			x = NCA_X;
			y = NCA_Z;
			z = NCA_Y;
		}
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
	if ( enType == NCMAKELATHE ) {
		nV = 1;
	}
	else {
		if ( GetValFlags() & NCD_K )
			nV = max(0, (int)lpArgv->nc.dValue[NCA_K]);
		else if ( GetValFlags() & NCD_L )
			nV = max(0, (int)lpArgv->nc.dValue[NCA_L]);
		else
			nV = 1;
	}
	// ���A���W(�O��̵�޼ު�Ă��Œ軲�ق��ǂ���)
	m_dInitial = pData->GetType()==NCDCYCLEDATA ?
				static_cast<const CNCcycle*>(pData)->GetInitialValue() - pData->GetOffsetPoint()[z] :
				m_ptValS[z];
	// �ݸ����ٕ␳(R���W���ް��׽�ō��W�␳�̑ΏۊO)
	if ( lpArgv->bAbs ) {
		dR = GetValFlags() & NCD_R ? (float)lpArgv->nc.dValue[NCA_R] : m_ptValS[z];
		m_nDrawCnt = nH = min(1, nV);	// ��޿ح�ĂȂ牡�ւ�(0 or 1)��̂�
	}
	else {
		dR = GetValFlags() & NCD_R ? m_dInitial + (float)lpArgv->nc.dValue[NCA_R] : m_dInitial;
		// !!! Z�l��R�_����̲ݸ���Ăɕ␳ !!!
		if ( GetValFlags() & g_dwSetValFlags[z] )
			m_nc.dValue[z] = dR + (float)lpArgv->nc.dValue[z];
		m_nDrawCnt = nH = nV;	// �ݸ����قȂ牡�ւ��J��Ԃ�
	}
	if ( enType == NCMAKELATHE )
		dI = m_dInitial;	// ����Ӱ�ނ�G98/G99�͕ʂ̈Ӗ�
	else
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
			ZEROCLR(m_dMove);
			m_dDwell = 0.0f;
			m_nc.dLength = m_dCycleMove = 0.0f;
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
	if ( lpArgv->g68.bG68 && enType!=NCMAKELATHE )
		CalcG68Round(&(lpArgv->g68), m_ptValE);
	// �e�����Ƃ̈ړ����v�Z
	dx = m_ptValE[x] - m_ptValS[x];		dox = m_pRead->m_ptValOrg[x] - pt[x];
	dy = m_ptValE[y] - m_ptValS[y];		doy = m_pRead->m_ptValOrg[y] - pt[y];
	dRLength = fabs(dI - dR);
	dZLength = fabs(dR - m_ptValE[z]);
	m_dMove[x]  = fabs(dx) * nH;
	m_dMove[y]  = fabs(dy) * nH;
	m_dMove[z]  = fabs(m_ptValS[z] - dR);	// ���񉺍~��
	m_dMove[z] += dRLength * (nV-1);
	// �ړ����v�Z
	m_dCycleMove = _hypotf(m_dMove[x], m_dMove[y]);
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
		if ( enType == NCMAKELATHE ) {
			// ����Ӱ�ނł̏c���J��Ԃ��͖���߰�
			m_ptValI.SetPoint(dI, m_ptValS.y, m_ptValE.z);
			m_ptValR.SetPoint(dR, m_ptValS.y, m_ptValE.z);
			m_ptValE.SetPoint(dI, m_ptValS.y, m_ptValE.z);
			m_pRead->m_ptValOrg.SetPoint(dI, m_ptValS.y, pt.z);
		}
		else {
			m_ptValI.SetPoint(m_ptValE.x, m_ptValS.y, m_ptValE.z);
			m_ptValR.SetPoint(m_ptValE.x, dR, m_ptValE.z);
			m_ptValE.SetPoint(m_ptValS.x+dx*nH, dI, m_ptValS.z+dy*nH);
			m_pRead->m_ptValOrg.SetPoint(pt.x+dox*nH, dI, pt.z+doy*nH);
		}
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
	dI += ptOffset[z];
	dR += ptOffset[z];
	m_pt2D = m_ptValE.PointConvert();

	// �؂荞�ݒl���w�肳��Ă��Ȃ���δװ(��Ԃ�TH_NCRead.cpp�ɂ�)
	if ( !bL0Cycle && !(GetValFlags() & (g_dwSetValFlags[z]|NCFLG_LATHE_HOLE)) ) {
		m_nc.nErrorCode = IDS_ERR_NCBLK_NOTCYCLEZ;
		m_nDrawCnt = 0;
		m_dMove[z] = 0.0f;
		m_dDwell = 0.0f;
		return;
	}

#ifdef _DEBUG_DUMP
	printf("CNCcycle\n");
	printf("StartPoint x=%.3f y=%.3f z=%.3f\n",
		m_ptValS.x, m_ptValS.y, m_ptValS.z);
	printf("           R-Point=%.3f C-Point=%.3f\n", dR, GetValue(z)+ptOffset[z]);
	printf("FinalPoint x=%.3f y=%.3f z=%.3f\n",
		m_ptValE.x, m_ptValE.y, m_ptValE.z);
	printf("m_nDrawCnt=%d\n", m_nDrawCnt);
#endif

	if ( m_nDrawCnt <= 0 )
		return;		// bL0Cycle �ł������܂�

	// ���W�i�[�̈�m��
	for ( i=0; i<SIZEOF(m_Cycle); i++ )
		m_Cycle[i] = new PTCYCLE[m_nDrawCnt];
	m_Cycle3D = new PTCYCLE3D[m_nDrawCnt];

	pt = m_ptValS;
	int zz = enType==NCMAKELATHE ? NCA_X : z;
	for ( i=0; i<m_nDrawCnt; i++ ) {
		pt[x] += dx;	pt[y] += dy;
#ifdef _DEBUG_DUMP
		printf("           No.%d [x]=%.3f [y]=%.3f\n", i+1, pt[x], pt[y]);
#endif
		// �e���ʂ��Ƃɍ��W�ݒ�
		pt[zz] = dI;
		m_Cycle3D[i].ptI  = pt;
		m_Cycle[NCDRAWVIEW_XYZ][i].ptI = pt.PointConvert();
		m_Cycle[NCDRAWVIEW_XY ][i].ptI = pt.GetXY();
		m_Cycle[NCDRAWVIEW_XZ ][i].ptI = pt.GetXZ();
		m_Cycle[NCDRAWVIEW_YZ ][i].ptI = pt.GetYZ();
		pt[zz] = dR;
		m_Cycle3D[i].ptR  = pt;
		m_Cycle[NCDRAWVIEW_XYZ][i].ptR = pt.PointConvert();
		m_Cycle[NCDRAWVIEW_XY ][i].ptR = pt.GetXY();
		m_Cycle[NCDRAWVIEW_XZ ][i].ptR = pt.GetXZ();
		m_Cycle[NCDRAWVIEW_YZ ][i].ptR = pt.GetYZ();
		pt[zz] = GetValue(zz) + ptOffset[zz];
		m_Cycle3D[i].ptC  = pt;
		m_Cycle[NCDRAWVIEW_XYZ][i].ptC = pt.PointConvert();
		m_Cycle[NCDRAWVIEW_XY ][i].ptC = pt.GetXY();
		m_Cycle[NCDRAWVIEW_XZ ][i].ptC = pt.GetXZ();
		m_Cycle[NCDRAWVIEW_YZ ][i].ptC = pt.GetYZ();
	}
	
	// �㏸���̈ړ��E�؍풷�v�Z
	switch ( GetGcode() ) {
	case 84:	// R�_�܂Ő؍한�A�C�Ƽ�ٓ_�܂ő����蕜�A
	case 85:
	case 87:
	case 88:
	case 89:
		if ( lpArgv->bG98 && enType!=NCMAKELATHE ) {
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
//	if ( GetValFlags() & NCD_P &&
//		(GetGcode()==82 || GetGcode()==88 || GetGcode()==89) )
	if ( GetValFlags() & NCD_P )	// P_����΃R�[�h�Ɋ֌W�Ȃ����Z�Ɏd�l�ύX
		m_dDwell = (float)lpArgv->nc.dValue[NCA_P] * nV;
	else
		m_dDwell = 0.0f;

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

void CNCcycle::DrawTuning(float f)
{
	CNCline::DrawTuning(f);
	m_ptDrawI[NCDRAWVIEW_XYZ] = m_ptValI.PointConvert() * f;
	m_ptDrawR[NCDRAWVIEW_XYZ] = m_ptValR.PointConvert() * f;
	if ( m_nDrawCnt > 0 ) {
		for ( int i=0; i<m_nDrawCnt; i++ )
			m_Cycle[NCDRAWVIEW_XYZ][i].DrawTuning(f);
	}
}

void CNCcycle::DrawTuningXY(float f)
{
	CNCline::DrawTuningXY(f);
	m_ptDrawI[NCDRAWVIEW_XY] = m_ptValI.GetXY() * f;
	m_ptDrawR[NCDRAWVIEW_XY] = m_ptValR.GetXY() * f;
	if ( m_nDrawCnt > 0 ) {
		for ( int i=0; i<m_nDrawCnt; i++ )
			m_Cycle[NCDRAWVIEW_XY][i].DrawTuning(f);
	}
}

void CNCcycle::DrawTuningXZ(float f)
{
	CNCline::DrawTuningXZ(f);
	m_ptDrawI[NCDRAWVIEW_XZ] = m_ptValI.GetXZ() * f;
	m_ptDrawR[NCDRAWVIEW_XZ] = m_ptValR.GetXZ() * f;
	if ( m_nDrawCnt > 0 ) {
		for ( int i=0; i<m_nDrawCnt; i++ )
			m_Cycle[NCDRAWVIEW_XZ][i].DrawTuning(f);
	}
}

void CNCcycle::DrawTuningYZ(float f)
{
	CNCline::DrawTuningYZ(f);
	m_ptDrawI[NCDRAWVIEW_YZ] = m_ptValI.GetYZ() * f;
	m_ptDrawR[NCDRAWVIEW_YZ] = m_ptValR.GetYZ() * f;
	if ( m_nDrawCnt > 0 ) {
		for ( int i=0; i<m_nDrawCnt; i++ )
			m_Cycle[NCDRAWVIEW_YZ][i].DrawTuning(f);
	}
}

void CNCcycle::Draw(CDC* pDC, BOOL bSelect) const
{
#ifdef _DEBUGDRAW_NCD
	printf("Cycle Draw()=%d\n", GetBlockLineNo()+1);
#endif
	DrawCycle(NCDRAWVIEW_XYZ, pDC, bSelect);
}

void CNCcycle::DrawXY(CDC* pDC, BOOL bSelect) const
{
#ifdef _DEBUGDRAW_NCD
	printf("Cycle DrawXY()=%d\n", GetBlockLineNo()+1);
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
	printf("Cycle DrawXZ()=%d\n", GetBlockLineNo()+1);
#endif
	if ( GetPlane() == XZ_PLANE ) {
		if ( GetValFlags() & NCFLG_LATHE_HOLE )
			DrawCycle(NCDRAWVIEW_XZ, pDC, bSelect);
		else {
			CNCline::DrawXZ(pDC, bSelect);
			DrawCyclePlane(NCDRAWVIEW_XZ, pDC, bSelect);
		}
	}
	else
		DrawCycle(NCDRAWVIEW_XZ, pDC, bSelect);
}

void CNCcycle::DrawYZ(CDC* pDC, BOOL bSelect) const
{
#ifdef _DEBUGDRAW_NCD
	printf("Cycle DrawYZ()=%d\n", GetBlockLineNo()+1);
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

void CNCcycle::DrawCyclePlane(ENNCDRAWVIEW enDraw, CDC* pDC, BOOL bSelect) const
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

void CNCcycle::DrawCycle(ENNCDRAWVIEW enDraw, CDC* pDC, BOOL bSelect) const
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

CNCcircle::CNCcircle
(const CNCdata* pData, LPNCARGV lpArgv, const CPoint3F& ptOffset, NCMAKETYPE enType) :
	CNCline(NCDARCDATA, pData, lpArgv, ptOffset)
{
	BOOL	bError = TRUE;				// Error

	if ( GetGcode() == 2 )
		m_dwFlags &= ~NCFLG_G02G03;		// 0:G02
	else
		m_dwFlags |=  NCFLG_G02G03;		// 1:G03

	if ( enType != NCMAKELATHE ) {
		// XZ���ʂ�(Z->X, X->Y)�Ȃ̂ł��̂܂܌v�Z����� -90����]������K�v������
		// �ȒP�ɑΉ�����ɂ͉�]�����𔽑΂ɂ���΂悢 -> ����������ƃ}�V�ȑΉ����I
		if ( GetPlane() == XZ_PLANE )
			m_dwFlags ^= NCFLG_G02G03;
	}

	// Ȳè�ނ̍��W�ް��Œ��S���v�Z���Ă�����W��]
	m_pRead->m_ptValOrg.SetPoint(GetValue(NCA_X), GetValue(NCA_Y), GetValue(NCA_Z));
	m_ptValS = pData->GetOriginalEndPoint();	// ���ƂŐݒ肵����
	m_ptValE = m_pRead->m_ptValOrg;

	// ���ʍ��W�擾
	CPointF	pts( GetPlaneValue(m_ptValS) ),
			pte( GetPlaneValue(m_ptValE) ),
			pto;

	// ���a�ƒ��S���W�̌v�Z(R�D�� �������Aܲ�Ӱ�ނ͖���)
	if ( GetValFlags()&NCD_R && enType!=NCMAKEWIRE ) {
		m_r = (float)lpArgv->nc.dValue[NCA_R];
		bError = !CalcCenter(pts, pte);
	}
	else if ( GetValFlags() & (NCD_I|NCD_J|NCD_K) ) {
		float	i = GetValFlags() & NCD_I ? (float)lpArgv->nc.dValue[NCA_I] : 0.0f,
				j = GetValFlags() & NCD_J ? (float)lpArgv->nc.dValue[NCA_J] : 0.0f,
				k = GetValFlags() & NCD_K ? (float)lpArgv->nc.dValue[NCA_K] : 0.0f;
		m_ptOrg = m_ptValS;
		switch ( GetPlane() ) {
		case XY_PLANE:
			m_r = _hypotf(i, j);
			m_ptOrg.x += i;
			m_ptOrg.y += j;
			pto = m_ptOrg.GetXY();
			break;
		case XZ_PLANE:
			m_r = _hypotf(i, k);
			m_ptOrg.x += i;
			m_ptOrg.z += k;
			pto = m_ptOrg.GetXZ();
			break;
		case YZ_PLANE:
			m_r = _hypotf(j, k);
			m_ptOrg.y += j;
			m_ptOrg.z += k;
			pto = m_ptOrg.GetYZ();
			break;
		}
		pts -= pto;		// �p�x�����p�̌��_�␳
		pte -= pto;
		AngleTuning(pts, pte);
		bError = FALSE;
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

	if ( bError ) {
		m_nc.nErrorCode = IDS_ERR_NCBLK_CIRCLECENTER;
	}

#ifdef _DEBUG_DUMP
	printf("CNCcircle gcode=%d\n", GetGcode());
	printf("sx=%.3f sy=%.3f sz=%.3f / ex=%.3f ey=%.3f ez=%.3f / r=%.3f\n",
		m_ptValS.x, m_ptValS.y, m_ptValS.z,
		m_ptValE.x, m_ptValE.y, m_ptValE.z, m_r);
	printf("px=%.3f py=%.3f pz=%.3f / sq=%f eq=%f\n",
		m_ptOrg.x, m_ptOrg.y, m_ptOrg.z, DEG(m_sq), DEG(m_eq));
	DbgDump();
	Dbg_sep();
#endif
}

CNCcircle::CNCcircle(const CNCdata* pData) : CNCline(pData)
{
	m_pt2D	= m_ptValE.PointConvert();
	const CNCcircle*	pCircle = static_cast<const CNCcircle *>(pData);
	m_ptOrg	= pCircle->GetOrg();
	m_r		= pCircle->GetR();
	m_sq	= pCircle->GetStartAngle();
	m_eq	= pCircle->GetEndAngle();
	Constracter();
}

void CNCcircle::Constracter(void)
{
	// �`��֐��̌�����ضوړ��ʂ̌v�Z
	switch ( GetPlane() ) {
	case XY_PLANE:
		m_pfnCircleDraw = &CNCcircle::DrawG17;
		m_dHelicalStep = GetValFlags() & NCD_Z ?
			(m_ptValE.z - m_ptValS.z) / ((m_eq - m_sq)/ARCSTEP) : 0.0f;
		break;
	case XZ_PLANE:
		m_pfnCircleDraw = &CNCcircle::DrawG18;
		m_dHelicalStep = GetValFlags() & NCD_Y ?
			(m_ptValE.y - m_ptValS.y) / ((m_eq - m_sq)/ARCSTEP) : 0.0f;
		break;
	case YZ_PLANE:
		m_pfnCircleDraw = &CNCcircle::DrawG19;
		m_dHelicalStep = GetValFlags() & NCD_X ?
			(m_ptValE.x - m_ptValS.x) / ((m_eq - m_sq)/ARCSTEP) : 0.0f;
		break;
	}
}

BOOL CNCcircle::CalcCenter(const CPointF& pts, const CPointF& pte)
{
	// R �w��Ŏn�_�I�_�������ꍇ�̓G���[
	if ( pts == pte )
		return FALSE;

	// �Q�̉~�̌�_�����߂�
	CPointF	pt1, pt2;
	int		nResult;
	tie(nResult, pt1, pt2) = ::CalcIntersectionPoint_CC(pts, pte, m_r, m_r);
	if ( nResult < 1 )
		return FALSE;

	// �ǂ���̉����̗p���邩
	AngleTuning(pts-pt1, pte-pt1);	// �܂�����̒��S���W����p�x�����߂�
	float	q = ::RoundUp(DEG(m_eq-m_sq));
	if ( nResult==1 ||
			(m_r>0.0f && q<=180.0f) ||	// 180���ȉ�
			(m_r<0.0f && q> 180.0f) ) {	// 180��������
		SetCenter(pt1);
	}
	else {
		// ������ϯ����Ȃ������̂ő����̉����̗p
		AngleTuning(pts-pt2, pte-pt2);
		SetCenter(pt2);
	}
	return TRUE;
}

void CNCcircle::SetCenter(const CPointF& pt)
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

tuple<float, float>	CNCcircle::CalcAngle(BOOL bG03, const CPointF& pts, const CPointF& pte) const
{
	float	sq, eq, q;

	if ( (sq=pts.arctan()) < 0.0f )
		sq += PI2;
	if ( (eq=pte.arctan()) < 0.0f )
		eq += PI2;

	// ��� s<e (�����v���) �Ƃ���
	if ( !bG03 )	// G02 �Ȃ�J�n�p�x�ƏI���p�x�����ւ�
		invoke_swap(sq, eq);

	// ���׉~�ʂɒ���
	q = ::RoundUp(DEG(sq));
	while ( q > ::RoundUp(DEG(eq)) )	// CDXFarc::AngleTuning()�Ɠ��������ɕύX
		eq += PI2;

	// �p�x����
	if ( q>=360.0f && ::RoundUp(DEG(eq))>=360.0f ) {
		sq -= PI2;
		eq -= PI2;
	}

	return make_tuple(sq, eq);
}

void CNCcircle::AngleTuning(const CPointF& pts, const CPointF& pte)
{
	tie(m_sq, m_eq) = CalcAngle(GetG03(), pts, pte);
	if ( pts == pte ) {	
		// �n�_�I�_�������^�~�Ȃ�
		// �I�_�p�x = �J�n�p�x + 360���ɋ����u��
		m_eq = m_sq + PI2;
	}
}

float CNCcircle::SetCalcLength(void)
{
	// �؍풷�̂݌v�Z
	ZEROCLR(m_dMove);

	if ( m_obCdata.IsEmpty() )
		m_nc.dLength = fabs(m_r * (m_eq - m_sq));
	else {
		m_nc.dLength = 0.0f;
		// �e�␳�v�f�̍��v
		for ( int i=0; i<m_obCdata.GetSize(); i++ )
			m_nc.dLength += m_obCdata[i]->SetCalcLength();
	}

	float	dResult = 0.0f;
	if ( m_pWireObj )
		dResult = m_pWireObj->SetCalcLength();

	return max((float)m_nc.dLength, dResult);
}

void CNCcircle::DrawTuning(float f)
{
	// �v�Z���Ȃ���g��W����^����
	m_dFactor = f;

	// ܲԉ��H�\���p�̎n�_�I�_
	float		sq, eq, r = fabs(m_r) * f;
	CPoint3F	pt3D, ptOrg(m_ptOrg);	ptOrg *= f;
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

void CNCcircle::DrawTuningXY(float f)
{
	m_dFactorXY = f;

	float		sq, eq, r = fabs(m_r) * f;
	CPoint3F	ptOrg(m_ptOrg);	ptOrg *= f;
	tie(sq, eq) = GetSqEq();
	m_ptDrawS[NCDRAWVIEW_XY].x = (int)(r * cos(sq) + ptOrg.x);
	m_ptDrawS[NCDRAWVIEW_XY].y = (int)(r * sin(sq) + ptOrg.y);
	m_ptDrawE[NCDRAWVIEW_XY].x = (int)(r * cos(eq) + ptOrg.x);
	m_ptDrawE[NCDRAWVIEW_XY].y = (int)(r * sin(eq) + ptOrg.y);

	CNCdata::DrawTuningXY(f);
}

void CNCcircle::DrawTuningXZ(float f)
{
	m_dFactorXZ = f;

	float		sq, eq, r = fabs(m_r) * f;
	CPoint3F	ptOrg(m_ptOrg);	ptOrg *= f;
	tie(sq, eq) = GetSqEq();
	m_ptDrawS[NCDRAWVIEW_XZ].x = (int)(r * cos(sq) + ptOrg.x);
	m_ptDrawS[NCDRAWVIEW_XZ].y = (int)(ptOrg.z);
	m_ptDrawE[NCDRAWVIEW_XZ].x = (int)(r * cos(eq) + ptOrg.x);
	m_ptDrawE[NCDRAWVIEW_XZ].y = (int)(m_ptValE.z * f);

	CNCdata::DrawTuningXZ(f);
}

void CNCcircle::DrawTuningYZ(float f)
{
	m_dFactorYZ = f;

	float		sq, eq, r = fabs(m_r) * f;
	CPoint3F	ptOrg(m_ptOrg);	ptOrg *= f;
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
	printf("Circle Draw()=%d\n", GetBlockLineNo()+1);
#endif
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();

	// ���ʂ��Ƃ̕`��֐�
	if ( m_obCdata.IsEmpty() || pOpt->GetNCViewFlg(GLOPTFLG_DRAWREVISE) ) {
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
	if ( pOpt->GetNCViewFlg(GLOPTFLG_DRAWCIRCLECENTER) )
		pDC->SetPixelV(m_ptOrg.PointConvert()*m_dFactor,
			pOpt->GetNcDrawColor(NCCOL_CENTERCIRCLE));
	// �a�␳�ް��̕`��
	CNCdata::Draw(pDC, bSelect);
}

void CNCcircle::DrawXY(CDC* pDC, BOOL bSelect) const
{
#ifdef _DEBUGDRAW_NCD
	printf("Circle DrawXY()=%d\n", GetBlockLineNo()+1);
#endif
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();

	if ( m_obCdata.IsEmpty() || pOpt->GetNCViewFlg(GLOPTFLG_DRAWREVISE) ) {
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
	if ( pOpt->GetNCViewFlg(GLOPTFLG_DRAWCIRCLECENTER) )
		pDC->SetPixelV(m_ptOrg.GetXY()*m_dFactorXY,
			pOpt->GetNcDrawColor(NCCOL_CENTERCIRCLE));
	CNCdata::DrawXY(pDC, bSelect);
}

void CNCcircle::DrawXZ(CDC* pDC, BOOL bSelect) const
{
#ifdef _DEBUGDRAW_NCD
	printf("Circle DrawXZ()=%d\n", GetBlockLineNo()+1);
#endif
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();

	if ( m_obCdata.IsEmpty() || pOpt->GetNCViewFlg(GLOPTFLG_DRAWREVISE) ) {
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
	if ( pOpt->GetNCViewFlg(GLOPTFLG_DRAWCIRCLECENTER) )
		pDC->SetPixelV(m_ptOrg.GetXZ()*m_dFactorXZ,
			pOpt->GetNcDrawColor(NCCOL_CENTERCIRCLE));
	CNCdata::DrawXZ(pDC, bSelect);
}

void CNCcircle::DrawYZ(CDC* pDC, BOOL bSelect) const
{
#ifdef _DEBUGDRAW_NCD
	printf("Circle DrawYZ()=%d\n", GetBlockLineNo()+1);
#endif
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();

	if ( m_obCdata.IsEmpty() || pOpt->GetNCViewFlg(GLOPTFLG_DRAWREVISE) ) {
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
	if ( pOpt->GetNCViewFlg(GLOPTFLG_DRAWCIRCLECENTER) )
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

void CNCcircle::DrawWireLine(ENNCDRAWVIEW enDraw, CDC* pDC, BOOL bSelect) const
{
	CPen*	pOldPen;
	if ( bSelect )
		pOldPen = AfxGetNCVCMainWnd()->GetPenCom(COMPEN_SEL);
	else
		pOldPen = AfxGetNCVCMainWnd()->GetPenNC(
			m_obCdata.IsEmpty() ? NCPEN_G1 : NCPEN_CORRECT);
	pOldPen = pDC->SelectObject(pOldPen);
	// XY
	DrawG17(enDraw, pDC);
	// UV
	if ( m_pWireObj ) {
		// m_pWireObj��CNCcircle�Ƃ͌���Ȃ�
		m_pWireObj->DrawWireLine(enDraw, pDC, bSelect);
		// XY��UV�̐ڑ�
		pDC->MoveTo(m_ptDrawS[enDraw]);
		pDC->LineTo(m_pWireObj->GetDrawStartPoint(enDraw));
		pDC->MoveTo(m_ptDrawE[enDraw]);
		pDC->LineTo(m_pWireObj->GetDrawEndPoint(enDraw));
	}
	pDC->SelectObject(pOldPen);
}

// CDC::Arc() ���g���Ƃǂ����Ă��\�����Y����D
// ���ꕽ�ʂł����Ă����א����ɂ��ߎ����s��
void CNCcircle::DrawG17(ENNCDRAWVIEW enDraw, CDC* pDC) const	// XY_PLANE
{
	float		sq, eq,
				dHelical = m_dHelicalStep, r = fabs(m_r);
	CPoint3F	pt3D, ptDrawOrg(m_ptOrg);
	CPointF		ptDraw;

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
		if ( GetG03() ) {
			for ( sq+=ARCSTEP; sq<eq; sq+=ARCSTEP ) {
				pt3D.x = r * cos(sq) + ptDrawOrg.x;
				pt3D.y = r * sin(sq) + ptDrawOrg.y;
				pt3D.z += dHelical;
				ptDraw = pt3D.PointConvert();
				pDC->LineTo(ptDraw);
			}
		}
		else {
			for ( sq-=ARCSTEP; sq>eq; sq-=ARCSTEP ) {
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
		if ( GetG03() ) {
			for ( sq+=ARCSTEP; sq<eq; sq+=ARCSTEP ) {
				ptDraw.x = r * cos(sq) + ptDrawOrg.x;
				ptDraw.y = r * sin(sq) + ptDrawOrg.y;
				pDC->LineTo(ptDraw);
			}
		}
		else {
			for ( sq-=ARCSTEP; sq>eq; sq-=ARCSTEP ) {
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
		if ( GetG03() ) {
			for ( sq+=ARCSTEP; sq<eq; sq+=ARCSTEP ) {
				ptDraw.x  = r * cos(sq) + ptDrawOrg.x;
				ptDraw.y += dHelical;
				pDC->LineTo(ptDraw);
			}
		}
		else {
			for ( sq-=ARCSTEP; sq>eq; sq-=ARCSTEP ) {
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
		if ( GetG03() ) {
			for ( sq+=ARCSTEP; sq<eq; sq+=ARCSTEP ) {
				ptDraw.x  = r * sin(sq) + ptDrawOrg.y;
				ptDraw.y += dHelical;
				pDC->LineTo(ptDraw);
			}
		}
		else {
			for ( sq-=ARCSTEP; sq>eq; sq-=ARCSTEP ) {
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

void CNCcircle::DrawG18(ENNCDRAWVIEW enDraw, CDC* pDC) const	// XZ_PLANE
{
	float		sq, eq,
				dHelical = m_dHelicalStep, r = fabs(m_r);
	CPoint3F	pt3D, ptDrawOrg(m_ptOrg);
	CPointF		ptDraw;

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
		if ( GetG03() ) {
			for ( sq+=ARCSTEP; sq<eq; sq+=ARCSTEP ) {
				pt3D.x = r * cos(sq) + ptDrawOrg.x;
				pt3D.y += dHelical;
				pt3D.z = r * sin(sq) + ptDrawOrg.z;
				ptDraw = pt3D.PointConvert();
				pDC->LineTo(ptDraw);
			}
		}
		else {
			for ( sq-=ARCSTEP; sq>eq; sq-=ARCSTEP ) {
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
		if ( GetG03() ) {
			for ( sq+=ARCSTEP; sq<eq; sq+=ARCSTEP ) {
				ptDraw.x  = r * cos(sq) + ptDrawOrg.x;
				ptDraw.y += dHelical;
				pDC->LineTo(ptDraw);
			}
		}
		else {
			for ( sq-=ARCSTEP; sq>eq; sq-=ARCSTEP ) {
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
		if ( GetG03() ) {
			for ( sq+=ARCSTEP; sq<eq; sq+=ARCSTEP ) {
				ptDraw.x = r * cos(sq) + ptDrawOrg.x;
				ptDraw.y = r * sin(sq) + ptDrawOrg.z;
				pDC->LineTo(ptDraw);
			}
		}
		else {
			for ( sq-=ARCSTEP; sq>eq; sq-=ARCSTEP ) {
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
		if ( GetG03() ) {
			for ( sq+=ARCSTEP; sq<eq; sq+=ARCSTEP ) {
				ptDraw.x += dHelical;
				ptDraw.y  = r * sin(sq) + ptDrawOrg.z;
				pDC->LineTo(ptDraw);
			}
		}
		else {
			for ( sq-=ARCSTEP; sq>eq; sq-=ARCSTEP ) {
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

void CNCcircle::DrawG19(ENNCDRAWVIEW enDraw, CDC* pDC) const	// YZ_PLANE
{
	float		sq, eq,
				dHelical = m_dHelicalStep, r = fabs(m_r);
	CPoint3F	pt3D, ptDrawOrg(m_ptOrg);
	CPointF		ptDraw;

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
		if ( GetG03() ) {
			for ( sq+=ARCSTEP; sq<eq; sq+=ARCSTEP ) {
				pt3D.x += dHelical;
				pt3D.y = r * cos(sq) + ptDrawOrg.y;
				pt3D.z = r * sin(sq) + ptDrawOrg.z;
				ptDraw = pt3D.PointConvert();
				pDC->LineTo(ptDraw);
			}
		}
		else {
			for ( sq-=ARCSTEP; sq>eq; sq-=ARCSTEP ) {
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
		if ( GetG03() ) {
			for ( sq+=ARCSTEP; sq<eq; sq+=ARCSTEP ) {
				ptDraw.x += dHelical;
				ptDraw.y  = r * cos(sq) + ptDrawOrg.y;
				pDC->LineTo(ptDraw);
			}
		}
		else {
			for ( sq-=ARCSTEP; sq>eq; sq-=ARCSTEP ) {
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
		if ( GetG03() ) {
			for ( sq+=ARCSTEP; sq<eq; sq+=ARCSTEP ) {
				ptDraw.x += dHelical;
				ptDraw.y  = r * sin(sq) + ptDrawOrg.z;
				pDC->LineTo(ptDraw);
			}
		}
		else {
			for ( sq-=ARCSTEP; sq>eq; sq-=ARCSTEP ) {
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
		if ( GetG03() ) {
			for ( sq+=ARCSTEP; sq<eq; sq+=ARCSTEP ) {
				ptDraw.x = r * cos(sq) + ptDrawOrg.y;
				ptDraw.y = r * sin(sq) + ptDrawOrg.z;
				pDC->LineTo(ptDraw);
			}
		}
		else {
			for ( sq-=ARCSTEP; sq>eq; sq-=ARCSTEP ) {
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

inline void _SetMaxRect_(const CPointF& pt, CRectF& rc)
{
	if ( rc.left > pt.x )
		rc.left = pt.x;
	if ( rc.right < pt.x )
		rc.right = pt.x;
	if ( rc.top > pt.y )
		rc.top = pt.y;
	if ( rc.bottom < pt.y )
		rc.bottom = pt.y;
}

CRect3F CNCcircle::GetMaxRect(void) const
{
	// �O�ڂ���l�p�`
	CRect3F	rcResult;
	CRectF	rcMax;
	CPointF	pt;
	float	sq, eq, r = fabs(m_r);

	tie(sq, eq) = GetSqEq();

	if ( fabs(eq-sq) >= RAD(270.0f) ) {
		rcMax.SetRect(-r, -r, r, r);
	}
	else {
		pt.x = r * cos(sq);
		pt.y = r * sin(sq);
		rcMax.SetRect(pt, 0, 0);
		if ( GetG03() ) {
			for ( sq+=ARCSTEP; sq<eq; sq+=ARCSTEP ) {
				pt.x = r * cos(sq);
				pt.y = r * sin(sq);
				_SetMaxRect_(pt, rcMax);
			}
		}
		else {
			for ( sq-=ARCSTEP; sq>eq; sq-=ARCSTEP ) {
				pt.x = r * cos(sq);
				pt.y = r * sin(sq);
				_SetMaxRect_(pt, rcMax);
			}
		}
		pt.x = r * cos(eq);
		pt.y = r * sin(eq);
		_SetMaxRect_(pt, rcMax);
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
	printf("CNCcircle::GetMaxRect()\n");
	printf(" rcResult(left, top   )=(%f, %f)\n", rcResult.left, rcResult.top);
	printf(" rcResult(right,bottom)=(%f, %f)\n", rcResult.right, rcResult.bottom);
	printf(" rcResult(high, low   )=(%f, %f)\n", rcResult.high, rcResult.low);
#endif

	return rcResult;
}

tuple<BOOL, CPointF, float, float> CNCcircle::CalcRoundPoint(const CNCdata* pNext, float r) const
{
	BOOL		bResult = FALSE;
	int			nResult;
	float		rr1, rr2, xa, ya, r0 = fabs(m_r);
	CPointF		pt, pts, pte, pt1, pt2;

	pts = GetPlaneValueOrg(m_ptOrg, m_ptValE);

	if ( pNext->GetType() == NCDARCDATA ) {
		const CNCcircle* pCircle = static_cast<const CNCcircle *>(pNext);
		pte = GetPlaneValueOrg(pCircle->GetOrg(), m_ptValE);
		float	r1, r2, rn = fabs(pCircle->GetR());
		// ���g�̉~�Ƒ����̐ڐ��Ŏ��g�́}r�������v�Z�i��_�ւ̐i���͔��Ή�]�j
		optional<float> dResult = _CalcRoundPoint_CircleInOut(pts, pte, GetG03(), !pCircle->GetG03(), r);
		if ( dResult )
			r1 = *dResult;
		else
			return make_tuple(bResult, pt, rr1, rr2);
		// �����̉~�Ǝ��g�̐ڐ��ő����́}r�������v�Z
		dResult = _CalcRoundPoint_CircleInOut(pte, pts, !pCircle->GetG03(), GetG03(), r);
		if ( dResult )
			r2 = *dResult;
		else
			return make_tuple(bResult, pt, rr1, rr2);
		// �Q�̉~�̌�_�����߂� -> �����Q�Ȃ��Ɩʎ��o���Ȃ��Ɣ��f����
		rr1 = r0 + r1;		rr2 = rn + r2;
		tie(nResult, pt1, pt2) = ::CalcIntersectionPoint_CC(pts, pte, rr1, rr2);
		if ( nResult != 2 )
			return make_tuple(bResult, pt, rr1, rr2);
		// ���̑I��
		float	sx = fabs(pts.x), sy = fabs(pts.y), ex = fabs(pte.x), ey = fabs(pte.y);
		if ( (sx<NCMIN && ex<NCMIN) || (sy<NCMIN && ey<NCMIN) ||
				(sx>NCMIN && ex>NCMIN && fabs(pts.y/pts.x - pte.y/pte.x)<NCMIN) ) {
			// ���S���������ɂ���Ƃ��C�ڐ��ƕ�������������I��
			if ( sy < NCMIN )
				pt = (GetG03() ? pts.x : -pts.x) * pt1.y > 0 ? pt1 : pt2;
			else
				pt = (GetG03() ? -pts.y : pts.y) * pt1.x > 0 ? pt1 : pt2;
		}
		else
			pt = pt1.x*pt1.x+pt1.y*pt1.y < pt2.x*pt2.x+pt2.y*pt2.y ? pt1 : pt2;
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
		rr1 = _hypotf(xa, ya);
		if ( r2 > 0 ) {
			xa = (pt.x*rn+pte.x*r2) / rr2;
			ya = (pt.y*rn+pte.y*r2) / rr2;
		}
		else {
			r2 = fabs(r2);
			xa = (pt.x*rn-pte.x*r2) / rr2;
			ya = (pt.y*rn-pte.y*r2) / rr2;
		}
		rr2 = _hypotf(xa, ya);
		bResult = TRUE;
	}
	else {
		pte = GetPlaneValueOrg(pNext->GetEndPoint(), m_ptValE);
		// �̾�ĕ���������
		int	nOffset = _CalcRoundPoint_OffsetFlag(pte, pts, GetG03());
		if ( nOffset == 0 )
			return make_tuple(bResult, pt, rr1, rr2);
		// �̾�ĕ����s�ړ���������_�����߂�
		optional<CPointF> ptResult = ::CalcOffsetIntersectionPoint_LC(pte, pts,	// ��������̱��۰���
								r0, r, r, !GetG03(), nOffset>0);						// ��]�����𔽓]
		if ( ptResult ) {
			pt = *ptResult;
			// �ʎ��ɑ�������C�l�̌v�Z
			if ( nOffset > 0 )
				nOffset = GetG03() ? 1 : -1;
			else
				nOffset = GetG03() ? -1 : 1;
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
			rr1 = _hypotf(xa, ya);
			rr2 = sqrt(pt.x*pt.x + pt.y*pt.y - r*r);
			bResult = TRUE;
		}
	}

	if ( bResult )
		pt += GetPlaneValue(m_ptValE);

	return make_tuple(bResult, pt, rr1, rr2);
}

optional<CPointF> CNCcircle::SetChamferingPoint(BOOL bStart, float c)
{
	CPoint3F	ptOrg3D( bStart ? m_ptValS : m_ptValE );
	CPointF		pt, ptOrg1, ptOrg2, pt1, pt2;
	float		pa, pb, ps;

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
	if ( m_eq - m_sq > PI ) {
		// 180���𒴂���Ƃ��͒��a�Ɣ�r
		if ( c >= fabs(m_r)*2 )
			return boost::none;
	}
	else {
		// 180�������̏ꍇ�͌��̒����Ɣ�r
		if ( c >= pt1.hypot(&pt2) )
			return boost::none;
	}

	// �Q�̉~�̌�_�����߂� -> �����Q�Ȃ��Ɩʎ��o���Ȃ��Ɣ��f����
	int	nResult;
	tie(nResult, pt1, pt2) = ::CalcIntersectionPoint_CC(ptOrg1, ptOrg2, fabs(m_r), c);
	if ( nResult != 2 )
		return boost::none;

	// ���v���̏ꍇ�C�n�p�ƏI�p������ւ���Ă���̂ňꎞ�I�Ɍ��ɖ߂�
	if ( !GetG03() )
		invoke_swap(m_sq, m_eq);

	// ���̑I��
	ps = bStart ? m_sq : m_eq;
	if ( (pa=ptOrg1.arctan(pt1)) < 0.0f )
		pa += PI2;
	if ( (pb=ptOrg1.arctan(pt2)) < 0.0f )
		pb += PI2;
	// 180�x�ȏ�̍��͕␳
	if ( fabs(ps-pa) > PI ) {
		if ( ps > pa )
			ps -= PI2;
		else
			pa -= PI2;
	}

	// �n�p�E�I�p�ɋ߂����̑I���Ǝ������g�̓_���X�V
	if ( bStart ) {
		if ( GetG03() ) {	// �����v���̎��́C�傫������I��
			if ( ps < pa ) {
				pt = pt1;
			}
			else {
				pt = pt2;
				pa = pb;
			}
		}
		else {				// ���v���̎��́C���p��������������
			if ( ps > pa ) {
				pt = pt1;
			}
			else {
				pt = pt2;
				pa = pb;
			}
		}
		SetPlaneValue(pt, m_ptValS);
		m_sq = pa;
	}
	else {
		if ( GetG03() ) {
			if ( ps > pa ) {
				pt = pt1;
			}
			else {
				pt = pt2;
				pa = pb;
			}
		}
		else {
			if ( ps < pa ) {
				pt = pt1;
			}
			else {
				pt = pt2;
				pa = pb;
			}
		}
		SetPlaneValue(pt, m_ptValE);
		m_pRead->m_ptValOrg = m_ptValE;
		if ( m_pRead->m_pG68 ) {
			// m_ptValE ��G68��]�ςݍ��W�̂��߉�]�����ɖ߂��ĵ̾�Č��Z
			m_pRead->m_pG68->dRound = -m_pRead->m_pG68->dRound;
			CalcG68Round(m_pRead->m_pG68, m_pRead->m_ptValOrg);
			m_pRead->m_pG68->dRound = -m_pRead->m_pG68->dRound;
		}
		m_pRead->m_ptValOrg -= m_pRead->m_ptOffset;
		m_eq = pa;
		m_pt2D = m_ptValE.PointConvert();
	}

	// �p�x�␳
	if ( !GetG03() )
		invoke_swap(m_sq, m_eq);
	ps = ::RoundUp(DEG(m_sq));
	while ( ps >= ::RoundUp(DEG(m_eq)) )
		m_eq += PI2;

	return pt;
}

float CNCcircle::CalcBetweenAngle(const CNCdata* pNext) const
{
	// �Q���̌�_(���g�̏I�_)�����_�ɂȂ�悤�ɕ␳
	CPointF		pt( GetPlaneValueOrg(m_ptOrg, m_ptValE) ), pt1, pt2;

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
	return GetG03() ? -1 : 1;
}

optional<CPointF> CNCcircle::CalcPerpendicularPoint
	(ENPOINTORDER enPoint, float r, int nSign) const
{
	const CPoint3F	pts( enPoint==STARTPOINT ? m_ptValS : m_ptValE );
	// �n�_�I�_�֌W�Ȃ� ��]�������␳����
	// pts�ƒ��S�̌X�����v�Z���Ĕ��a�}r
	CPointF	pt( GetPlaneValueOrg(pts, m_ptOrg) );
	float	q = pt.arctan(),
			rr = fabs(m_r) + r * CalcOffsetSign() * nSign;
	CPointF	pt1(rr*cos(q), rr*sin(q));
	pt1 += GetPlaneValue(m_ptOrg);

	return pt1;
}

optional<CPointF> CNCcircle::CalcOffsetIntersectionPoint
	(const CNCdata* pNext, float t1, float t2, BOOL bLeft) const
{
	BOOL	bResult = FALSE;
	// �Q���̌�_(���g�̏I�_)�����_�ɂȂ�悤�ɕ␳
	CPointF	pt;

	if ( pNext->GetType() == NCDARCDATA ) {
		const CNCcircle* pCircle = static_cast<const CNCcircle *>(pNext);
		CPointF	pto1( GetPlaneValueOrg(m_ptOrg, m_ptValE) ),
				pto2( GetPlaneValueOrg(pCircle->GetOrg(), m_ptValE) ),
				p1, p2;
		int		k1 = CalcOffsetSign(),
				k2 = pCircle->CalcOffsetSign();
		if ( !bLeft ) {
			k1 = -k1;
			k2 = -k2;
		}
		float	r1 = fabs(m_r)+t1*k1, r2 = fabs(pCircle->GetR())+t2*k2;
		// ����~�����f
		if ( pto1.IsMatchPoint(&pto2) && fabs(r1-r2)<NCMIN ) {
			// �~�̌�_�͋��߂��Ȃ��̂ŁA�P���̾�č��W�v�Z
			float	q = GetG03() ? m_eq : m_sq;	// �I�_�p�x
			CPointF	pto( GetPlaneValue(m_ptOrg) );
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
		CPointF	pt1( GetPlaneValueOrg(pNext->GetEndPoint(), m_ptValE) ),
				pt2( GetPlaneValueOrg(m_ptOrg, m_ptValE) );
		// ������̱��۰��ŉ�]�����𔽓]
		optional<CPointF> ptResult = ::CalcOffsetIntersectionPoint_LC(pt1, pt2,
								fabs(m_r), t1, t2, !GetG03(), !bLeft);
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

	return boost::none;
}

optional<CPointF> CNCcircle::CalcOffsetIntersectionPoint2
	(const CNCdata* pNext, float r, BOOL bLeft) const
{
	int		k;
	// �Q���̌�_(���g�̏I�_)�����_�ɂȂ�悤�ɕ␳
	CPointF	pt, pt1, pt2;

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
	optional<CPointF> ptResult = ::CalcOffsetIntersectionPoint_LL(pt1, pt2, r, r, bLeft);
	// ���_�␳
	if ( ptResult ) {
		pt  = *ptResult;
		pt += GetPlaneValue(m_ptValE);
		return pt;
	}
	return boost::none;
}

void CNCcircle::SetCorrectPoint(ENPOINTORDER enPoint, const CPointF& ptSrc, float rr)
{
	CPoint3F&	ptVal = enPoint==STARTPOINT ? m_ptValS : m_ptValE;	// �Q�ƌ^
	CPointF		pt;

	SetPlaneValue(ptSrc, ptVal);
	pt = GetPlaneValueOrg(ptVal, m_ptOrg);

	// �p�x����
	if ( enPoint == STARTPOINT ) {
		float&	q = GetG03() ? m_sq : m_eq;	// �Q�ƌ^
		if ( (q=pt.arctan()) < 0.0f )
			q += PI2;
	}
	else {
		m_r = copysign(fabs(m_r)+rr, m_r);		// �I�_�̎��������a�␳
		float&	q = GetG03() ? m_eq : m_sq;
		if ( (q=pt.arctan()) < 0.0f )
			q += PI2;
		m_pt2D = m_ptValE.PointConvert();
	}
	float	sq = ::RoundUp(DEG(m_sq));
	while ( sq >= ::RoundUp(DEG(m_eq)) )
		m_eq += PI2;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

// �̾�ĕ����̌���
int _CalcRoundPoint_OffsetFlag(const CPointF& pts, const CPointF& pto, BOOL bG03)
{
	// ��]����
	// Issue #12 �����������������؂��K�v
	int		k = bG03 ? 1 : -1;		// G02:-90��,G03:+90��
	// �����̊p�x
	float	q = pts.arctan();
	// �~�̐ڐ�
	CPointF	pte(-pto.y*k, pto.x);
	// �����̊p�x�ŕ␳
	pte.RoundPoint(-q);
	// y �̕����ŵ̾�ĕ����𔻒f
	if ( fabs(pte.y) < NCMIN )
		k = 0;		// ���Ȃ�
	else if ( pte.y > 0.0f )
		k = -1;		// �E��
	else
		k = 1;		// ����

	return k;
}

// �~���m�̓��O���a�v�Z
optional<float> _CalcRoundPoint_CircleInOut
	(const CPointF& pts, const CPointF& pte, BOOL bG03, BOOL bG03next, float r)
{
	CPointF	pto;
	float	sx = fabs(pts.x), sy = fabs(pts.y), ex = fabs(pte.x), ey = fabs(pte.y), rr;
	int		k1 = bG03 ? 1 : -1, k2 = bG03next==0 ? 1 : -1;

	// ������̔��f
	if ( (sx<NCMIN && ex<NCMIN && pts.y*pte.y>0) ||
			(sy<NCMIN && ey<NCMIN && pts.x*pte.x>0) ||
			(sx>NCMIN && ex>NCMIN && fabs(pts.y/pts.x - pte.y/pte.x)<NCMIN && pts.x*pte.x>0) ) {
		// ���S���������ɂ���C���Cx,y �̕����������Ƃ�
		float	l1 = pts.x*pts.x + pts.y*pts.y;
		float	l2 = pte.x*pte.x + pte.y*pte.y;
		if ( fabs(l1 - l2) < NCMIN )	// ������������==���O�Չ~
			return boost::none;
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
		rr = copysign(r, pto.y*pts.x*k1);
	else if ( fabs(pto.y) < NCMIN && sx < NCMIN )
		rr = copysign(r, -pto.x*pts.y*k1);
	else
		rr = copysign(r, -(pto.x*pts.x + pto.y*pts.y));

	return rr;
}
