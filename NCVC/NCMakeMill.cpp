// NCMakeMill.cpp: CNCMakeMill �N���X�̃C���v�������e�[�V����
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NCVC.h"
#include "DXFdata.h"
#include "NCMakeMillOpt.h"
#include "NCMakeMill.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using std::string;
using namespace boost;

// �悭�g���ϐ���Ăяo���̊ȗ��u��
#define	GetFlg		ms_pMakeOpt->GetFlag
#define	GetNum		ms_pMakeOpt->GetNum
#define	GetDbl		ms_pMakeOpt->GetDbl
#define	GetStr		ms_pMakeOpt->GetStr

/////////////////////////////////////////////////////////////////////////////
// �ÓI�ϐ��̏�����
float	CNCMakeMill::ms_dCycleZ[] = {HUGE_VALF, HUGE_VALF};
float	CNCMakeMill::ms_dCycleR[] = {HUGE_VALF, HUGE_VALF};
float	CNCMakeMill::ms_dCycleP[] = {HUGE_VALF, HUGE_VALF};
float	CNCMakeMill::ms_dCycleQ[] = {HUGE_VALF, HUGE_VALF};
int		CNCMakeMill::ms_nCycleCode = NCMAKECYCLECODE;	// 81
int		CNCMakeMill::ms_nCycleReturn = 98;
PFNGETCYCLESTRING	CNCMakeMill::ms_pfnGetCycleString = &CNCMakeMill::GetCycleString;

//////////////////////////////////////////////////////////////////////
// CNCMakeMill �\�z/����
//////////////////////////////////////////////////////////////////////

CNCMakeMill::CNCMakeMill()
{
}

CNCMakeMill::CNCMakeMill(const CDXFdata* pData, float dFeed, const float* pdHelical/*=NULL*/)
{
	CString	strGcode;
	CPointF	pt;

	// �{�ް�
	switch ( pData->GetMakeType() ) {
	case DXFPOINTDATA:
		pt = pData->GetEndMakePoint();
		strGcode = GetValString(NCA_X, pt.x) + GetValString(NCA_Y, pt.y);
		if ( !GetFlg(MKNC_FLG_GCLIP) || ms_dCycleZ[0]!=ms_dCycleZ[1] ) {
			strGcode += GetValString(NCA_Z, ms_dCycleZ[0]);
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
		if ( GetNum(MKNC_NUM_DRILLRETURN)==2 &&		// G83
				(!GetFlg(MKNC_FLG_GCLIP) || ms_dCycleQ[0]!=ms_dCycleQ[1]) ) {
			strGcode += "Q" + (*ms_pfnGetValDetail)(ms_dCycleQ[0]);
			ms_dCycleQ[1] = ms_dCycleQ[0];
		}
		if ( !strGcode.IsEmpty() ) {
			m_strGcode = MakeStrBlock((*ms_pfnGetCycleString)() + strGcode + GetFeedString(dFeed));
		}
		// Z���̕��A�_��ÓI�ϐ���
		ms_xyz[NCA_Z] = GetNum(MKNC_NUM_ZRETURN) == 0 ?
			::RoundUp(GetDbl(MKNC_DBL_G92Z)) : ::RoundUp(GetDbl(MKNC_DBL_ZG0STOP));
		break;

	case DXFLINEDATA:
		pt = pData->GetEndMakePoint();
		strGcode = GetValString(NCA_X, pt.x) + GetValString(NCA_Y, pt.y);
		if ( !strGcode.IsEmpty() ) {
			m_strGcode = MakeStrBlock((*ms_pfnGetGString)(1) + strGcode + GetFeedString(dFeed));
		}
		break;

	case DXFCIRCLEDATA:
		m_strGcode = pdHelical ?
			(*ms_pfnMakeHelical)(static_cast<const CDXFcircle*>(pData), dFeed, *pdHelical) :
			(*ms_pfnMakeCircle) (static_cast<const CDXFcircle*>(pData), dFeed);
		break;

	case DXFARCDATA:
		m_strGcode = (*ms_pfnMakeArc)(static_cast<const CDXFarc*>(pData), dFeed);
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
	CPointF	pt;

	switch ( pData->GetMakeType() ) {
	case DXFLINEDATA:
		pt = pData->GetStartMakePoint();
		// ���̵�޼ު�Ăƌ��݈ʒu���Ⴄ�Ȃ�A�����܂ňړ�(bL0����)
		if ( bL0 && (pt.x!=ms_xyz[NCA_X] || pt.y!=ms_xyz[NCA_Y]) ) {
			m_strGcode = MakeStrBlock((*ms_pfnGetGString)(0) +
				GetValString(NCA_X, pt.x) + GetValString(NCA_Y, pt.y));
		}
		// through
	case DXFCIRCLEDATA:
		// ��޼ު�Ă̈ړ��ް�����
		pt = pData->GetEndMakePoint();
		if ( pt.x!=ms_xyz[NCA_X] || pt.y!=ms_xyz[NCA_Y] ) {
			if ( bL0 ) {
				m_strGcode = MakeStrBlock((*ms_pfnGetCycleString)() +
					GetValString(NCA_X, pt.x) + GetValString(NCA_Y, pt.y) +
					GetValString(NCA_L, 0));
			}
			else {
				m_strGcode = MakeStrBlock((*ms_pfnGetGString)(0) +
					GetValString(NCA_X, pt.x) + GetValString(NCA_Y, pt.y));
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
CNCMakeMill::CNCMakeMill(int nCode, float ZVal, float dFeed)
{
	CString	strGcode;
	CString	strValue(GetValString(NCA_Z, ZVal));
	if ( !strValue.IsEmpty() ) {
		strGcode = (*ms_pfnGetGString)(nCode) + strValue;
		if ( dFeed > 0 )
			strGcode += GetFeedString(dFeed);
	}
	if ( !strGcode.IsEmpty() ) {
		m_strGcode = MakeStrBlock(strGcode);
	}
}

// XY��G[0|1]�ړ�
CNCMakeMill::CNCMakeMill(int nCode, const CPointF& pt, float dFeed)
{
	CString	strGcode(GetValString(NCA_X, pt.x) +
					 GetValString(NCA_Y, pt.y) );
	if ( !strGcode.IsEmpty() ) {
		if ( nCode != 0 )	// G00�ȊO
			strGcode += GetFeedString(dFeed);
		m_strGcode = MakeStrBlock((*ms_pfnGetGString)(nCode) + strGcode);
	}
}

// XYZ��G01
CNCMakeMill::CNCMakeMill(const CPoint3F& pt, float dFeed)
{
	CString	strGcode(GetValString(NCA_X, pt.x) +
					 GetValString(NCA_Y, pt.y) +
					 GetValString(NCA_Z, pt.z) );
	if ( !strGcode.IsEmpty() ) {
		m_strGcode = MakeStrBlock((*ms_pfnGetGString)(1) + strGcode + GetFeedString(dFeed));
	}
}

// ���W�w���ɂ��~�ʂ̐���
CNCMakeMill::CNCMakeMill
	(int nCode, const CPointF& pts, const CPointF& pte, const CPointF& pto, float r)
{
	CString	strGcode( (*ms_pfnMakeCircleSub)(nCode, pte, pto-pts, r) );
	if ( !strGcode.IsEmpty() ) {
		m_strGcode = MakeStrBlock(strGcode + GetFeedString(GetDbl(MKNC_DBL_FEED)));
	}
}

// �h�E�F�����ԁiG04�j
CNCMakeMill::CNCMakeMill(float t)
{
	m_strGcode = MakeStrBlock((*ms_pfnGetGString)(4) + GetValString(NCA_P, t));
}

// �C�ӂ̕�������
CNCMakeMill::CNCMakeMill(const CString& strGcode) : CNCMakeBase(strGcode)
{
}

// Kodatuno���W
CNCMakeMill::CNCMakeMill(const Coord& xyz) :
	CNCMakeMill( CPoint3D(xyz), GetDbl(MKNC_DBL_FEED) )	// �Ϗ��R���X�g���N�^(XYZ��G01��)
{
	// double -> float �̃L���X�g������̂ŉ��ǂ̗]�n����
}

//////////////////////////////////////////////////////////////////////
// ���ފ֐�

void CNCMakeMill::MakePolylineMov(const CDXFpolyline* pPoly, BOOL bL0)
{
	if ( pPoly->GetVertexCount() <= 1 )
		return;

	CPointF	pt;
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
					GetValString(NCA_X, pt.x) +
					GetValString(NCA_Y, pt.y) +
					GetValString(NCA_L, 0);
			else
				strGcode = (*ms_pfnGetGString)(0) +
					GetValString(NCA_X, pt.x) +
					GetValString(NCA_Y, pt.y);
			if ( !strGcode.IsEmpty() )
				AddGcodeArray(strGcode);
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

CString CNCMakeMill::GetValString(int xyz, float dVal, BOOL bSpecial)
{
	// *** CNCMakeWire �Ƌ��p ***
	// WireMode ��K��L�̐����ɂ́AbSpecial��TRUE���w��

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
				float	d = dVal;
				dVal -= ms_xyz[xyz];
				ms_xyz[xyz] = d;
			}
		}
		break;
	case NCA_U:		// WireMode
	case NCA_V:
		break;			// ��ۂ��ȗ������ɏo��
	case NCA_I:		// [I|J]0���o�� -> bSpecial==TRUE
	case NCA_J:
	case NCA_K:		// WireMode -> bSpecial==TRUE
		if ( !bSpecial && fabs(dVal)<NCMIN )	// NC�̌������덷�����Ȃ疳��
			return strResult;
		break;
	case NCA_L:
		if ( !bSpecial ) {
			// �����_�w��Ȃ�
			strResult = (g_szNdelimiter[xyz] + lexical_cast<string>((int)dVal)).c_str();
			return strResult;
		}
		break;		// -> WireMode
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
	default:
		// �����_�w��Ȃ�
		strResult = (g_szNdelimiter[xyz] + lexical_cast<string>((int)dVal)).c_str();
		return strResult;
	}

	// ���l�����͵�߼�݂ɂ���ē��I�Ɋ֐����Ăяo��
	strResult = g_szNdelimiter[xyz] + (*ms_pfnGetValDetail)(dVal);

	return strResult;
}

void CNCMakeMill::SetStaticOption(const CNCMakeMillOpt* pNCMake)
{
	extern	LPCTSTR	gg_szReturn;	// "\n"
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
	ms_pfnGetValString = &GetValString;	// �ް��׽����̌ďo�p
	if ( GetNum(MKNC_NUM_DOT) < 2 ) {
		ms_pfnGetValDetail = GetFlg(MKNC_FLG_ZEROCUT) ?
				&GetValString_UZeroCut : &GetValString_Normal;
		if ( GetNum(MKNC_NUM_DOT) == 0 )
			_dp.SetDecimal3();		// ������3��
		else
			_dp.SetDecimal4();		// ������4��
	}
	else {
		ms_pfnGetValDetail = &GetValString_Multi1000;
	}
	// --- �s�ԍ�������
	ms_nMagni = GetNum(MKNC_NUM_LINEADD)<0 ||
					 GetNum(MKNC_NUM_LINEADD)>=SIZEOF(nLineMulti) ?
		nLineMulti[0] : nLineMulti[GetNum(MKNC_NUM_LINEADD)];
	// --- EOB
	ms_strEOB = GetStr(MKNC_STR_EOB) + gg_szReturn;
	// --- �Œ軲�َw��
	ms_nCycleReturn = GetNum(MKNC_NUM_ZRETURN) == 0 ? 98 : 99;
	if ( GetNum(MKNC_NUM_DRILLRETURN) == 2 ) {
		ms_nCycleCode = 83;
	}
	else {
		if ( GetDbl(MKNC_DBL_DWELL) > 0 )
			ms_nCycleCode = GetNum(MKNC_NUM_DRILLRETURN) == 0 ? 82 : 89;
		else
			ms_nCycleCode = GetNum(MKNC_NUM_DRILLRETURN) == 0 ? 81 : 85;
	}
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
	ms_bIJValue = !GetFlg(MKNC_FLG_ZEROCUT_IJ);
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
