// NCMakeLathe.cpp: CNCMakeLathe �N���X�̃C���v�������e�[�V����
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NCVC.h"
#include "DXFdata.h"
#include "NCDoc.h"		// g_szNCcomment[]
#include "NCMakeLatheOpt.h"
#include "NCMakeLathe.h"

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

//////////////////////////////////////////////////////////////////////
// CNCMakeMill �\�z/����
//////////////////////////////////////////////////////////////////////

CNCMakeLathe::CNCMakeLathe(void)
{
}

CNCMakeLathe::CNCMakeLathe(const CDXFdata* pData, float dFeed)
{
	switch ( pData->GetMakeType() ) {
	case DXFLINEDATA:
	{
		CPointF	pt(pData->GetEndMakePoint());
		CString	strGcode(GetValString(NCA_Z, pt.x)+GetValString(NCA_X, pt.y));
		if ( !strGcode.IsEmpty() ) {
			m_strGcode = MakeStrBlock((*ms_pfnGetGString)(1) + strGcode + GetFeedString(dFeed));
		}
	}
		break;

	case DXFCIRCLEDATA:
		m_strGcode = (*ms_pfnMakeCircle)(static_cast<const CDXFcircle*>(pData), dFeed);
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

// �w��ʒu�ɒ���[�ړ�|�؍�]
CNCMakeLathe::CNCMakeLathe(int nCode, const CPointF& pt, float dFeed)
{
	CString	strGcode( (*ms_pfnGetGString)(nCode) +
		GetValString(NCA_Z, pt.x) + GetValString(NCA_X, pt.y) );
	if ( !strGcode.IsEmpty() ) {
		if ( nCode > 0 )
			strGcode += GetFeedString(dFeed);
		m_strGcode = MakeStrBlock(strGcode);
	}
}

// �w��ʒu�ɂQ���ړ�
CNCMakeLathe::CNCMakeLathe(TWOMOVEMODE enMode, const CPointF& pt, float dFeed)
{
	CString	strGcode1, strGcode2, strGcode;

	if ( enMode == ZXMOVE ) {
		// Z���ړ���AX���ړ�
		strGcode1 = (*ms_pfnGetGString)(0) + GetValString(NCA_Z, pt.x);
		strGcode  = (*ms_pfnGetGString)(1) + GetValString(NCA_X, pt.y);
		if ( !strGcode.IsEmpty() )
			strGcode2 = strGcode + GetFeedString(dFeed);
	}
	else {
		// X���ړ���AZ���ړ�
		strGcode  = (*ms_pfnGetGString)(1) + GetValString(NCA_X, pt.y);
		if ( !strGcode.IsEmpty() )
			strGcode1 = strGcode + GetFeedString(dFeed);
		strGcode2 = (*ms_pfnGetGString)(0) + GetValString(NCA_Z, pt.x);
	}
	if ( !strGcode1.IsEmpty() )
		AddGcodeArray(strGcode1);
	if ( !strGcode2.IsEmpty() )
		AddGcodeArray(strGcode2);
}

// X|Z���̕ω�
CNCMakeLathe::CNCMakeLathe(int nCode, int xz, float dVal, float dFeed)
{
	CString	strGcode;
	CString	strValue(GetValString(xz, dVal));
	if ( !strValue.IsEmpty() ) {
		strGcode = (*ms_pfnGetGString)(nCode) + strValue;
		if ( nCode > 0 )
			strGcode += GetFeedString(dFeed);
		m_strGcode = MakeStrBlock(strGcode);
	}
}

// �C�ӂ̕�������
CNCMakeLathe::CNCMakeLathe(const CString& strGcode) : CNCMakeBase(strGcode)
{
}

//////////////////////////////////////////////////////////////////////
// ���ފ֐�

void CNCMakeLathe::CreateEndFace(const CPointF& pts)
{
	CString		strGcode;

	// ��]���ݒ�i����ͯ�ް�ŏ����ς݁j
	strGcode = MakeSpindle(GetNum(MKLA_NUM_E_SPINDLE));
	if ( !strGcode.IsEmpty() ) {
		AddGcodeArray(strGcode);
	}

	// ���Ѻ��ޑ}�� ���s���ނ̒u��, �s�ԍ��t�^ etc.
	if ( !GetStr(MKLA_STR_E_CUSTOM).IsEmpty() ) {
		m_strGarray.Add( GetChangeEnter(GetStr(MKLA_STR_E_CUSTOM)) );
	}

	// �O�a���W����[�ʐ؍�J�n�ʒu��ݒ�
	CPointF	pt;
	pt.x = max(GetDbl(MKLA_DBL_E_CUT), pts.x+GetDbl(MKLA_DBL_E_STEP));	// MKLA_DBL_ENDSTEP��ϲŽ�l
	pt.y = pts.y + GetDbl(MKLA_DBL_E_PULLX);
	// Z���̈ړ�
	strGcode = (*ms_pfnGetGString)(0) + GetValString(NCA_Z, pt.x);
	if ( !strGcode.IsEmpty() )
		AddGcodeArray(strGcode);
	// X���̐؍�ړ�
	strGcode = (*ms_pfnGetGString)(0) + GetValString(NCA_X, pt.y);
	if ( !strGcode.IsEmpty() )
		AddGcodeArray(strGcode);

	// �ŏI�؂荞�݂܂ŌJ��Ԃ�
	while (TRUE) {
		// ���S�܂Ő؍푗��
		strGcode = (*ms_pfnGetGString)(1) + GetValString(NCA_X, 0) + GetFeedString(GetDbl(MKLA_DBL_E_FEED));;
		AddGcodeArray(strGcode);
		// �����㕪�ړ�(Z���ړ���G00����OpenGL�`��ɔ��f����Ȃ�)
		strGcode = (*ms_pfnGetGString)(1) + GetValString(NCA_Z, pt.x+GetDbl(MKLA_DBL_E_PULLZ));
		AddGcodeArray(strGcode);
		strGcode = (*ms_pfnGetGString)(0) + GetValString(NCA_X, pt.y);
		AddGcodeArray(strGcode);
		// ���̒[�ʍ��W
		if ( fabs(GetDbl(MKLA_DBL_E_CUT)-pt.x) < NCMIN )
			break;
		pt.x = max(GetDbl(MKLA_DBL_E_CUT), pt.x+GetDbl(MKLA_DBL_E_STEP));
		strGcode = (*ms_pfnGetGString)(0) + GetValString(NCA_Z, pt.x);
		AddGcodeArray(strGcode);
	}
}

void CNCMakeLathe::CreatePilotHole(void)
{
	extern	LPCTSTR	g_szNCcomment[];
	CString		strGcode;
	VLATHEDRILLINFO	v;

	// ������������
	if ( GetDbl(MKLA_DBL_HOLE) > 0.0f ) {
		CString	strFmt;
		strFmt.Format(IDS_MAKENCD_FORMAT, GetDbl(MKLA_DBL_HOLE));
		strGcode = LATHEHOLE_S;		// g_szNCcomment[LATHEHOLE]
		strGcode += '=' + strFmt;
		AddGcodeArray( '(' + strGcode + ')' );
	}

	// �h�������
	if ( !static_cast<const CNCMakeLatheOpt*>(ms_pMakeOpt)->GetDrillInfo(v) )
		return;

	for ( const auto& info : v ) {
		// ��]���ݒ�
		strGcode = MakeSpindle(info.s);
		if ( !strGcode.IsEmpty() ) {
			AddGcodeArray(strGcode);
		}
		// �h�������
		strGcode.Format(IDS_MAKENCD_LATHEDRILL, info.d);
		AddGcodeArray(strGcode);
		// ���Ѻ��ޑ}�� ���s���ނ̒u��, �s�ԍ��t�^ etc.
		if ( !GetStr(MKLA_STR_D_CUSTOM).IsEmpty() ) {
			m_strGarray.Add( GetChangeEnter(GetStr(MKLA_STR_D_CUSTOM)) );
		}
		// �؍�J�n�ʒu(R�_)�ֈړ�
		if ( ms_xyz[NCA_Z] != GetDbl(MKLA_DBL_DRILLR) ) {
			strGcode = (*ms_pfnGetGString)(0) + GetValString(NCA_Z, GetDbl(MKLA_DBL_DRILLR));
			AddGcodeArray(strGcode);
		}
		if ( ms_xyz[NCA_X] != 0 ) {
			strGcode = (*ms_pfnGetGString)(0) + GetValString(NCA_X, 0);
			AddGcodeArray(strGcode);
		}
		if ( GetFlg(MKLA_FLG_CYCLE) ) {
			// �Œ軲�قŐ���
			strGcode = (*ms_pfnGetGString)(83) +
				GetValString(NCA_Z, GetDbl(MKLA_DBL_DRILLZ)) + GetValString(NCA_X, 0);
//				GetValString(NCA_R, GetDbl(MKLA_DBL_DRILLR), TRUE);	// ��ňړ����Ă���̂ŕs�v�₯��...
			if ( GetDbl(MKLA_DBL_DRILLQ) > 0 )
				strGcode += "Q" + (*ms_pfnGetValDetail)(GetDbl(MKLA_DBL_DRILLQ));
			if ( GetDbl(MKLA_DBL_D_DWELL) > 0 )
				strGcode += GetValString(NCA_P, GetDbl(MKLA_DBL_D_DWELL));
			strGcode += GetFeedString(info.f);
			AddGcodeArray(strGcode);
			// Z�l�����ɖ߂�
			ms_xyz[NCA_Z] = GetDbl(MKLA_DBL_DRILLR);
		}
		else {
			// ������ԂŐ���
			float z = ms_xyz[NCA_Z];	// GetDbl(MKLA_DBL_DRILLR)
			while (TRUE) {
				z -= GetDbl(MKLA_DBL_DRILLQ);
				if ( z <= GetDbl(MKLA_DBL_DRILLZ) ) {
					strGcode = (*ms_pfnGetGString)(1) + GetValString(NCA_Z, GetDbl(MKLA_DBL_DRILLZ)) + GetFeedString(info.f);
					AddGcodeArray(strGcode);
					if ( GetDbl(MKLA_DBL_D_DWELL) > 0 ) {
						strGcode = (*ms_pfnGetGString)(4) + GetValString(NCA_P, GetDbl(MKLA_DBL_D_DWELL));
						AddGcodeArray(strGcode);
					}
					strGcode = (*ms_pfnGetGString)(0) + GetValString(NCA_Z, GetDbl(MKLA_DBL_DRILLR));
					AddGcodeArray(strGcode);
					break;
				}
				strGcode = (*ms_pfnGetGString)(1) + GetValString(NCA_Z, z) + GetFeedString(info.f);
				AddGcodeArray(strGcode);
				strGcode = (*ms_pfnGetGString)(0) + GetValString(NCA_Z, z+GetDbl(MKLA_DBL_DRILLD));
				AddGcodeArray(strGcode);
			}
		}
	}

	if ( GetFlg(MKLA_FLG_CYCLE) ) {
		// �Œ軲�ٷ�ݾ�
		AddGcodeArray( (*ms_pfnGetGString)(80) );
	}
	// ���ٍH���I������
	strGcode = ENDDRILL_S;	// g_szNCcomment[ENDDRILL]
	AddGcodeArray( '(' + strGcode + ')' );
}

void CNCMakeLathe::CreateGroove(const CPointF& pt, float dPullX)
{
	CString	strGcode;

	strGcode = (*ms_pfnGetGString)(0) + GetValString(NCA_Z, pt.x);
	AddGcodeArray(strGcode);
	strGcode = (*ms_pfnGetGString)(0) + GetValString(NCA_X, dPullX);
	AddGcodeArray(strGcode);
	strGcode = (*ms_pfnGetGString)(1) + GetValString(NCA_X, pt.y) + GetFeedString(GetDbl(MKLA_DBL_G_FEEDX));
	AddGcodeArray(strGcode);
	if ( GetDbl(MKLA_DBL_G_DWELL) > 0 ) {
		strGcode = (*ms_pfnGetGString)(4) + GetValString(NCA_P, GetDbl(MKLA_DBL_G_DWELL));
		AddGcodeArray(strGcode);
	}
	strGcode = (*ms_pfnGetGString)(1) + GetValString(NCA_X, dPullX) + GetFeedString(GetDbl(MKLA_DBL_G_FEEDX));
	AddGcodeArray(strGcode);
}

void CNCMakeLathe::CreateGroove(const CPointF& pts, const CPointF& pte, float dPullX)
{
	CString	strGcode;

	strGcode = (*ms_pfnGetGString)(0) + GetValString(NCA_Z, pts.x);
	AddGcodeArray(strGcode);
	strGcode = (*ms_pfnGetGString)(0) + GetValString(NCA_X, dPullX);
	AddGcodeArray(strGcode);
	strGcode = (*ms_pfnGetGString)(1) + GetValString(NCA_X, pts.y) + GetFeedString(GetDbl(MKLA_DBL_G_FEEDX));
	AddGcodeArray(strGcode);
	if ( GetDbl(MKLA_DBL_G_DWELL) > 0 ) {
		strGcode = (*ms_pfnGetGString)(4) + GetValString(NCA_P, GetDbl(MKLA_DBL_G_DWELL));
		AddGcodeArray(strGcode);
	}
	strGcode = (*ms_pfnGetGString)(1) + GetValString(NCA_Z, pte.x) + GetValString(NCA_X, pte.y) + GetFeedString(GetDbl(MKLA_DBL_G_FEED));
	AddGcodeArray(strGcode);
	if ( GetDbl(MKLA_DBL_G_DWELL) > 0 ) {
		strGcode = (*ms_pfnGetGString)(4) + GetValString(NCA_P, GetDbl(MKLA_DBL_G_DWELL));
		AddGcodeArray(strGcode);
	}
	strGcode = (*ms_pfnGetGString)(1) + GetValString(NCA_X, dPullX) + GetFeedString(GetDbl(MKLA_DBL_G_FEEDX));
	AddGcodeArray(strGcode);
}

CString	CNCMakeLathe::MakeSpindle(int s)
{
	return (*ms_pfnGetSpindle)(s);
}

CString CNCMakeLathe::GetValString(int xyz, float dVal, BOOL bSpecial)
{
	extern	LPCTSTR	g_szNdelimiter;		// "XYZUVWIJKRPLDH" from NCDoc.cpp
	CString	strResult;

	// �����_�ȉ��R��(�ȉ��؂�グ)�ł̒l���v����
	switch ( xyz ) {
	case NCA_X:
	case NCA_Y:
	case NCA_Z:
		if ( GetFlg(MKLA_FLG_GCLIP) && fabs(ms_xyz[xyz]-dVal)<NCMIN )
			return strResult;
		else {
			if ( GetNum(MKLA_NUM_G90) == 0 ) {	// ��޿ح��
				ms_xyz[xyz] = dVal;
			}
			else {								// �ݸ�����
				float	d = dVal;
				dVal -= ms_xyz[xyz];
				ms_xyz[xyz] = d;
			}
		}
		break;
	case NCA_R:
		// ���aR�ł͂Ȃ��Œ軲��R�_�̏ꍇ
		if ( bSpecial ) {
			if ( GetNum(MKLA_NUM_G90) != 0 ) {	// �ݸ����قȂ�
				dVal -= ms_xyz[NCA_Z];			// ����Z����̍���
				if ( dVal == 0 )
					return strResult;
			}
		}
		break;
	case NCA_I:
	case NCA_J:
	case NCA_K:
		if ( fabs(dVal) < NCMIN )	// NC�̌������덷�����Ȃ疳��
			return strResult;
		break;
	case NCA_P:		// �Œ軲���޳�َ���
		strResult = (g_szNdelimiter[xyz] + lexical_cast<string>((int)dVal)).c_str();
		return strResult;
	}

	// ���l�����͵�߼�݂ɂ���ē��I�Ɋ֐����Ăяo��
	if ( xyz == NCA_X )
		dVal *= 2.0f;	// X�����a�w��
	strResult = g_szNdelimiter[xyz] + (*ms_pfnGetValDetail)(dVal);

	return strResult;
}

void CNCMakeLathe::SetStaticOption(const CNCMakeLatheOpt* pNCMake)
{
	extern	LPCTSTR	gg_szReturn;	// "\n"
	static	const	int		nLineMulti[] = {	// �s�ԍ�������
		1, 5, 10, 100
	};

	CNCMakeBase::SetStaticOption(pNCMake);

	// --- ���w��
	NCAX = NCA_Z;	NCAY = NCA_X;
	NCAI = NCA_K;	NCAJ = NCA_I;
	// --- ��]�w��
	ms_pfnGetSpindle = GetFlg(MKLA_FLG_DISABLESPINDLE) ?
		&GetSpindleString_Clip : &GetSpindleString;
	// --- ����w��
	switch ( GetNum(MKLA_NUM_FDOT) ) {
	case 0:		// �����_�\�L
		ms_pfnGetFeed = GetFlg(MKLA_FLG_ZEROCUT) ?
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
	ms_pfnGetLineNo = GetFlg(MKLA_FLG_LINEADD) && !(GetStr(MKLA_STR_LINEFORM).IsEmpty()) ?
		&GetLineNoString : &GetLineNoString_Clip;
	// --- G����Ӱ���
	ms_pfnGetGString = GetFlg(MKLA_FLG_GCLIP) ?
		&GetGString_Clip : &GetGString;
	// --- ���W�\�L
	ms_pfnGetValString = &GetValString;	// �ް��׽����̌ďo�p
	if ( GetNum(MKLA_NUM_DOT) < 2 ) {
		ms_pfnGetValDetail = GetFlg(MKLA_FLG_ZEROCUT) ?
				&GetValString_UZeroCut : &GetValString_Normal;
		if ( GetNum(MKLA_NUM_DOT) == 0 )
			_dp.SetDecimal3();		// ������3��
		else
			_dp.SetDecimal4();		// ������4��
	}
	else {
		ms_pfnGetValDetail = &GetValString_Multi1000;
	}
	// --- �s�ԍ�������
	ms_nMagni = GetNum(MKLA_NUM_LINEADD)<0 ||
					 GetNum(MKLA_NUM_LINEADD)>=SIZEOF(nLineMulti) ?
		nLineMulti[0] : nLineMulti[GetNum(MKLA_NUM_LINEADD)];
	// --- EOB
	ms_strEOB = GetStr(MKLA_STR_EOB) + gg_szReturn;
	// --- �~�ް��̐؍�w��
	ms_nCircleCode = GetNum(MKLA_NUM_CIRCLECODE) == 0 ? 2 : 3;
	// --- �~,�~���ް��̐���
	if ( GetNum(MKLA_NUM_IJ) == 0 ) {
		ms_pfnMakeCircle	= &MakeCircle_R;
		ms_pfnMakeCircleSub	= &MakeCircleSub_R;
		ms_pfnMakeHelical	= &MakeCircle_R_Helical;
		ms_pfnMakeArc		= &MakeArc_R;
	}
	else {
		if ( GetFlg(MKLA_FLG_CIRCLEHALF) ) {
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
	ms_bIJValue = !GetFlg(MKLA_FLG_ZEROCUT_IJ);
	// --- �ȉ~����
	ms_dEllipse = GetDbl(MKLA_DBL_ELLIPSE);
}
