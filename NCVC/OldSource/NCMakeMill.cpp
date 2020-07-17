// NCMakeMill.cpp: CNCMakeMill �N���X�̃C���v�������e�[�V����
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NCVC.h"
#include "DXFdata.h"
#include "NCMakeMillOpt.h"
#include "NCMakeMill.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

// �悭�g���ϐ���Ăяo���̊ȗ��u��
#define	GetFlg		ms_pMakeOpt->GetFlag
#define	GetNum		ms_pMakeOpt->GetNum
#define	GetDbl		ms_pMakeOpt->GetDbl
#define	GetStr		ms_pMakeOpt->GetStr

/////////////////////////////////////////////////////////////////////////////
// �ÓI�ϐ��̏�����
double	CNCMakeMill::ms_dCycleZ[] = {HUGE_VAL, HUGE_VAL};
double	CNCMakeMill::ms_dCycleR[] = {HUGE_VAL, HUGE_VAL};
double	CNCMakeMill::ms_dCycleP[] = {HUGE_VAL, HUGE_VAL};
int		CNCMakeMill::ms_nCycleCode = 81;
int		CNCMakeMill::ms_nCycleReturn = 88;
PFNGETCYCLESTRING	CNCMakeMill::ms_pfnGetCycleString = &CNCMakeMill::GetCycleString;

//////////////////////////////////////////////////////////////////////
// CNCMakeMill �\�z/����
//////////////////////////////////////////////////////////////////////

CNCMakeMill::CNCMakeMill()
{
}

CNCMakeMill::CNCMakeMill(const CDXFdata* pData, double dFeed, const double* pdHelical/*=NULL*/)
{
	CString	strGcode;
	CPointD	pt;

	// �{�ް�
	switch ( pData->GetMakeType() ) {
	case DXFPOINTDATA:
		pt = pData->GetEndMakePoint();
		strGcode = (*ms_pfnGetCycleString)() +
			GetValString(NCA_X, pt.x, FALSE) +
			GetValString(NCA_Y, pt.y, FALSE);
		if ( !GetFlg(MKNC_FLG_GCLIP) || ms_dCycleZ[0]!=ms_dCycleZ[1] ) {
			strGcode += GetValString(NCA_Z, ms_dCycleZ[0], FALSE);
			ms_dCycleZ[1] = ms_dCycleZ[0];
		}
		if ( !GetFlg(MKNC_FLG_GCLIP) || ms_dCycleR[0]!=ms_dCycleR[1] ) {
			strGcode += GetValString(NCA_R, ms_dCycleR[0], TRUE);
			ms_dCycleR[1] = ms_dCycleR[0];
		}
		if ( ms_dCycleP[0]>0 &&
				(!GetFlg(MKNC_FLG_GCLIP) || ms_dCycleP[0]!=ms_dCycleP[1]) ) {
			strGcode += GetValString(NCA_P, ms_dCycleP[0], TRUE);
			ms_dCycleP[1] = ms_dCycleP[0];
		}
		if ( !strGcode.IsEmpty() )
			m_strGcode += (*ms_pfnGetLineNo)() + strGcode +
								GetFeedString(dFeed) + ms_strEOB;
		// Z���̕��A�_��ÓI�ϐ���
		ms_xyz[NCA_Z] = GetNum(MKNC_NUM_ZRETURN) == 0 ?
			::RoundUp(GetDbl(MKNC_DBL_G92Z)) : ::RoundUp(GetDbl(MKNC_DBL_ZG0STOP));
		break;

	case DXFLINEDATA:
		pt = pData->GetEndMakePoint();
		strGcode = (*ms_pfnGetGString)(1) +
			GetValString(NCA_X, pt.x, FALSE) +
			GetValString(NCA_Y, pt.y, FALSE);
		if ( !strGcode.IsEmpty() )
			m_strGcode += (*ms_pfnGetLineNo)() + strGcode + GetFeedString(dFeed) + ms_strEOB;
		break;

	case DXFCIRCLEDATA:
		m_strGcode += pdHelical ?
			(*ms_pfnMakeHelical)(static_cast<const CDXFcircle*>(pData), dFeed, *pdHelical) :
			(*ms_pfnMakeCircle) (static_cast<const CDXFcircle*>(pData), dFeed);
		break;

	case DXFARCDATA:
		m_strGcode += (*ms_pfnMakeArc)(static_cast<const CDXFarc*>(pData), dFeed);
		break;

	case DXFELLIPSEDATA:
		m_strGarray.SetSize(0, 1024);
		MakeEllipse(static_cast<const CDXFellipse*>(pData), dFeed);
		break;

	case DXFPOLYDATA:
		m_strGarray.SetSize(0, 1024);
		MakePolylineCut(static_cast<const CDXFpolyline*>(pData), dFeed);
		break;
	}
}

CNCMakeMill::CNCMakeMill(const CDXFdata* pData, BOOL bL0)
{
	CPointD	pt;

	switch ( pData->GetMakeType() ) {
	case DXFLINEDATA:
		pt = pData->GetStartMakePoint();
		// ���̵�޼ު�Ăƌ��݈ʒu���Ⴄ�Ȃ�A�����܂ňړ�(bL0����)
		if ( bL0 && (pt.x!=ms_xyz[NCA_X] || pt.y!=ms_xyz[NCA_Y]) ) {
			m_strGcode = (*ms_pfnGetLineNo)() + (*ms_pfnGetGString)(0) +
				GetValString(NCA_X, pt.x, FALSE) +
				GetValString(NCA_Y, pt.y, FALSE) +
				ms_strEOB;
		}
		// through
	case DXFCIRCLEDATA:
		// ��޼ު�Ă̈ړ��ް�����
		pt = pData->GetEndMakePoint();
		if ( pt.x!=ms_xyz[NCA_X] || pt.y!=ms_xyz[NCA_Y] ) {
			if ( bL0 ) {
				m_strGcode = (*ms_pfnGetLineNo)() + (*ms_pfnGetCycleString)() +
					GetValString(NCA_X, pt.x, FALSE) +
					GetValString(NCA_Y, pt.y, FALSE) +
					GetValString(NCA_L, 0,    FALSE) +
					ms_strEOB;
			}
			else {
				m_strGcode += (*ms_pfnGetLineNo)() + (*ms_pfnGetGString)(0) +
					GetValString(NCA_X, pt.x, FALSE) +
					GetValString(NCA_Y, pt.y, FALSE) +
					ms_strEOB;
			}
		}
		break;

	case DXFPOLYDATA:
		m_strGarray.SetSize(0, 1024);
		MakePolylineMov(static_cast<const CDXFpolyline*>(pData), bL0);
		break;
	}
}

// Z���̕ω�(�㏸�E���~)
CNCMakeMill::CNCMakeMill(int nCode, double ZVal, double dFeed)
{
	CString	strGcode;
	CString	strValue(GetValString(NCA_Z, ZVal, FALSE));
	if ( !strValue.IsEmpty() ) {
		strGcode = (*ms_pfnGetGString)(nCode) + strValue;
		if ( dFeed > 0 )
			strGcode += GetFeedString(dFeed);
	}
	if ( !strGcode.IsEmpty() )
		m_strGcode = (*ms_pfnGetLineNo)() + strGcode + ms_strEOB;
}

// XY��G[0|1]�ړ�
CNCMakeMill::CNCMakeMill(int nCode, const CPointD& pt, double dFeed)
{
	CString	strGcode(
		GetValString(NCA_X, pt.x, FALSE) +
		GetValString(NCA_Y, pt.y, FALSE) );
	if ( !strGcode.IsEmpty() ) {
		if ( nCode != 0 )	// G0�ȊO
			strGcode += GetFeedString(dFeed);
		m_strGcode = (*ms_pfnGetLineNo)() + (*ms_pfnGetGString)(nCode) +
			strGcode + ms_strEOB;
	}
}

// ���W�w���ɂ��~�ʂ̐���
CNCMakeMill::CNCMakeMill
	(int nCode, const CPointD& pts, const CPointD& pte, const CPointD& pto, double r)
{
	CString	strGcode( (*ms_pfnMakeCircleSub)(nCode, pte, pto-pts, r) );
	if ( !strGcode.IsEmpty() )
		m_strGcode = (*ms_pfnGetLineNo)() + strGcode +
			GetFeedString(GetDbl(MKNC_DBL_FEED)) + ms_strEOB;
}

// �C�ӂ̕�������
CNCMakeMill::CNCMakeMill(const CString& strGcode) : CNCMakeBase(strGcode)
{
}

//////////////////////////////////////////////////////////////////////
// ���ފ֐�

void CNCMakeMill::MakePolylineMov(const CDXFpolyline* pPoly, BOOL bL0)
{
	if ( pPoly->GetVertexCount() <= 1 )
		return;

	CPointD	pt;
	CString	strGcode;
	const	CDXFdata*	pData;

	POSITION pos = pPoly->GetFirstVertex();
	pPoly->GetNextVertex(pos);
	while ( pos ) {
		pData = pPoly->GetNextVertex(pos);
		// �~��(�ӂ���ݏ��)�͖���
		if ( pData->GetMakeType() == DXFPOINTDATA ) {
			pt = pData->GetEndMakePoint();
			if ( bL0 )
				strGcode = (*ms_pfnGetCycleString)() +
					GetValString(NCA_X, pt.x, FALSE) +
					GetValString(NCA_Y, pt.y, FALSE) +
					GetValString(NCA_L, 0,    FALSE);
			else
				strGcode = (*ms_pfnGetGString)(0) +
					GetValString(NCA_X, pt.x, FALSE) +
					GetValString(NCA_Y, pt.y, FALSE);
			if ( !strGcode.IsEmpty() )
				m_strGarray.Add((*ms_pfnGetLineNo)() + strGcode + ms_strEOB);
		}
	}
}

CString	CNCMakeMill::MakeSpindle(ENDXFTYPE enType, BOOL bDeep)
{
	CString	strResult;
	if ( enType != DXFPOINTDATA )
		strResult = bDeep ?
			(*ms_pfnGetSpindle)(GetNum(MKNC_NUM_DEEPSPINDLE)) :
			(*ms_pfnGetSpindle)(GetNum(MKNC_NUM_SPINDLE));
	else
		strResult = (*ms_pfnGetSpindle)(GetNum(MKNC_NUM_DRILLSPINDLE));
	return strResult;
}

CString CNCMakeMill::GetValString(int xyz, double dVal, BOOL bSpecial)
{
	extern	LPCTSTR	g_szNdelimiter;		// "XYZUVWIJKRPLDH" from NCDoc.cpp
	CString	strResult;

	// �����_�ȉ��R��(�ȉ��؂�グ)�ł̒l���v����
	switch ( xyz ) {
	case NCA_X:
	case NCA_Y:
	case NCA_Z:
		if ( GetFlg(MKNC_FLG_GCLIP) && fabs(ms_xyz[xyz]-dVal)<NCMIN )
			return strResult;
		else {
			if ( GetNum(MKNC_NUM_G90) == 0 ) {	// ��޿ح��
				ms_xyz[xyz] = dVal;
			}
			else {								// �ݸ�����
				double	d = dVal;
				dVal -= ms_xyz[xyz];
				ms_xyz[xyz] = d;
			}
		}
		break;
	case NCA_I:
	case NCA_J:
	case NCA_K:
		if ( fabs(dVal) < NCMIN )	// NC�̌������덷�����Ȃ疳��
			return strResult;
		break;
	case NCA_R:
		// ���aR�ł͂Ȃ��Œ軲��R�_�̏ꍇ
		if ( bSpecial ) {
			if ( GetNum(MKNC_NUM_G90) != 0 ) {	// �ݸ����قȂ�
				dVal -= ms_xyz[NCA_Z];			// ����Z����̍���
				if ( dVal == 0 )
					return strResult;
			}
		}
		break;
	case NCA_P:
		if ( bSpecial )	{	// G8x �Œ軲�� �޳�َ���
			if ( GetNum(MKNC_NUM_DWELLFORMAT) == 0 ) {	// �����_�\�L
				strResult = g_szNdelimiter[NCA_P] +
					( GetFlg(MKNC_FLG_ZEROCUT) ?
						GetValString_UZeroCut(dVal) : GetValString_Normal(dVal) );
				return strResult;
			}	// �����\�L�� default �ŏ���
		}
		// through
	default:	// L(�����_�w��Ȃ�)
		strResult.Format("%c%d", g_szNdelimiter[xyz], (int)dVal);
		return strResult;
	}

	// ���l�����͵�߼�݂ɂ���ē��I�Ɋ֐����Ăяo��
	strResult = g_szNdelimiter[xyz] + (*ms_pfnGetValDetail)(dVal);

	return strResult;
}

void CNCMakeMill::SetStaticOption(const CNCMakeMillOpt* pNCMake)
{
	extern	LPCTSTR	gg_szReturn;
	static	const	int		nLineMulti[] = {	// �s�ԍ�������
		1, 5, 10, 100
	};

	CNCMakeBase::SetStaticOption(pNCMake);

	// --- ���w��
	NCAX = NCA_X;	NCAY = NCA_Y;
	NCAI = NCA_I;	NCAJ = NCA_J;
	// --- ��]�w��
	ms_pfnGetSpindle = GetFlg(MKNC_FLG_DISABLESPINDLE) ?
		&GetSpindleString_Clip : &GetSpindleString;
	// --- ����w��
	switch ( GetNum(MKNC_NUM_FDOT) ) {
	case 0:		// �����_�\�L
		ms_pfnGetFeed = GetFlg(MKNC_FLG_ZEROCUT) ?
			&GetValString_UZeroCut : &GetValString_Normal;
		break;
	case 1:		// 1/1000�\�L
		ms_pfnGetFeed = &GetValString_Multi1000;
		break;
	default:	// �����\�L
		ms_pfnGetFeed = &GetFeedString_Integer;
		break;
	}
	// --- �s�ԍ��t��
	ms_pfnGetLineNo = GetFlg(MKNC_FLG_LINEADD) && !(GetStr(MKNC_STR_LINEFORM).IsEmpty()) ?
		&GetLineNoString : &GetLineNoString_Clip;
	// --- G����Ӱ���
	if ( GetFlg(MKNC_FLG_GCLIP) ) {
		ms_pfnGetGString =  &GetGString_Clip;
		ms_pfnGetCycleString = &GetCycleString_Clip;
	}
	else {
		ms_pfnGetGString = &GetGString;
		ms_pfnGetCycleString = &GetCycleString;
	}
	// --- ���W�\�L
//	ms_pfnGetValString = &GetValString;	// �ް��׽����̌ďo�p
	ms_pfnGetValDetail = GetNum(MKNC_NUM_DOT) == 0 ?
		(GetFlg(MKNC_FLG_ZEROCUT) ?
			&GetValString_UZeroCut : &GetValString_Normal) : &GetValString_Multi1000;
	// --- �s�ԍ�������
	ms_nMagni = GetNum(MKNC_NUM_LINEADD)<0 ||
					 GetNum(MKNC_NUM_LINEADD)>=SIZEOF(nLineMulti) ?
		nLineMulti[0] : nLineMulti[GetNum(MKNC_NUM_LINEADD)];
	// --- EOB
	ms_strEOB = GetStr(MKNC_STR_EOB).IsEmpty() ? 
		gg_szReturn : GetStr(MKNC_STR_EOB) + gg_szReturn;
	// --- �Œ軲�َw��
	ms_nCycleReturn = GetNum(MKNC_NUM_ZRETURN) == 0 ? 98 : 99;
	if ( GetNum(MKNC_NUM_DWELL) > 0 )
		ms_nCycleCode = GetNum(MKNC_NUM_DRILLRETURN) == 0 ? 82 : 89;
	else
		ms_nCycleCode = GetNum(MKNC_NUM_DRILLRETURN) == 0 ? 81 : 85;
	// --- �~�ް��̐؍�w��
	ms_nCircleCode = GetNum(MKNC_NUM_CIRCLECODE) == 0 ? 2 : 3;
	// --- �~,�~���ް��̐���
	if ( GetNum(MKNC_NUM_IJ) == 0 ) {
		ms_pfnMakeCircle	= &MakeCircle_R;
		ms_pfnMakeCircleSub	= &MakeCircleSub_R;
		ms_pfnMakeHelical	= &MakeCircle_R_Helical;
		ms_pfnMakeArc		= &MakeArc_R;
	}
	else {
		if ( GetFlg(MKNC_FLG_CIRCLEHALF) ) {
			ms_pfnMakeCircle  = &MakeCircle_IJHALF;
			ms_pfnMakeHelical = &MakeCircle_IJHALF_Helical;
		}
		else {
			ms_pfnMakeCircle  = &MakeCircle_IJ;
			ms_pfnMakeHelical = &MakeCircle_IJ_Helical;
		}
		ms_pfnMakeCircleSub	= &MakeCircleSub_IJ;
		ms_pfnMakeArc		= &MakeArc_IJ;
	}
	// --- �ȉ~����
	ms_dEllipse = GetDbl(MKNC_DBL_ELLIPSE);
}

//////////////////////////////////////////////////////////////////////

// G����Ӱ���(�Œ軲��)
CString	CNCMakeMill::GetCycleString(void)
{
	return GetGString(ms_nCycleReturn) + GetGString(ms_nCycleCode);
}

CString	CNCMakeMill::GetCycleString_Clip(void)
{
	CString		strResult;
	if ( ms_nGcode != NCMAKECYCLECODE ) {
		strResult = GetCycleString();
		ms_nGcode = NCMAKECYCLECODE;
	}
	return strResult;
}