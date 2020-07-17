// NCMakeLathe.cpp: CNCMakeLathe �N���X�̃C���v�������e�[�V����
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NCVC.h"
#include "DXFdata.h"
#include "NCMakeLatheOpt.h"
#include "NCMakeLathe.h"

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

//////////////////////////////////////////////////////////////////////
// CNCMakeMill �\�z/����
//////////////////////////////////////////////////////////////////////

CNCMakeLathe::CNCMakeLathe(const CDXFdata* pData)
{
	CString	strGcode;
	CPointD	pt;

	switch ( pData->GetMakeType() ) {
	case DXFLINEDATA:
		pt = pData->GetEndMakePoint();
		strGcode = GetValString(NCA_Z, pt.x, FALSE) +
				   GetValString(NCA_X, pt.y, FALSE);
		if ( !strGcode.IsEmpty() )
			m_strGcode = (*ms_pfnGetLineNo)() + (*ms_pfnGetGString)(1) +
						strGcode + GetFeedString(GetDbl(MKLA_DBL_FEED)) + ms_strEOB;
		break;

	case DXFCIRCLEDATA:
		m_strGcode = (*ms_pfnMakeCircle)(static_cast<const CDXFcircle*>(pData),
							GetDbl(MKLA_DBL_FEED));
		break;

	case DXFARCDATA:
		m_strGcode = (*ms_pfnMakeArc)(static_cast<const CDXFarc*>(pData),
							GetDbl(MKLA_DBL_FEED));
		break;

	case DXFELLIPSEDATA:
		m_strGarray.SetSize(0, 1024);
		MakeEllipse(static_cast<const CDXFellipse*>(pData), GetDbl(MKLA_DBL_FEED));
		break;

	case DXFPOLYDATA:
		m_strGarray.SetSize(0, 1024);
		MakePolylineCut(static_cast<const CDXFpolyline*>(pData), GetDbl(MKLA_DBL_FEED));
		break;
	}
}

// �w��ʒu�ɒ����ړ�
CNCMakeLathe::CNCMakeLathe(const CPointD& pt)
{
	CString	strGcode;

	strGcode = (*ms_pfnGetGString)(0) +
		GetValString(NCA_Z, pt.x, FALSE) + GetValString(NCA_X, pt.y, FALSE);
	if ( !strGcode.IsEmpty() )
		m_strGcode = (*ms_pfnGetLineNo)() + strGcode + ms_strEOB;
}

// �w��ʒu�ɂQ���ړ�
CNCMakeLathe::CNCMakeLathe(TWOMOVEMODE enMode, const CPointD& pt)
{
	CString	strGcode1, strGcode2, strGcode;

	if ( enMode == ZXMOVE ) {
		// Z���ړ���AX���ړ�
		strGcode1 = (*ms_pfnGetGString)(0) + GetValString(NCA_Z, pt.x, FALSE);
		strGcode  = (*ms_pfnGetGString)(1) + GetValString(NCA_X, pt.y, FALSE);
		if ( !strGcode.IsEmpty() )
			strGcode2 = strGcode + GetFeedString(GetDbl(MKLA_DBL_FEED));
	}
	else {
		// X���ړ���AZ���ړ�
		strGcode  = (*ms_pfnGetGString)(1) + GetValString(NCA_X, pt.y, FALSE);
		if ( !strGcode.IsEmpty() )
			strGcode1 = strGcode + GetFeedString(GetDbl(MKLA_DBL_XFEED));
		strGcode2 = (*ms_pfnGetGString)(0) + GetValString(NCA_Z, pt.x, FALSE);
	}
	if ( !strGcode1.IsEmpty() )
		m_strGcode  = (*ms_pfnGetLineNo)() + strGcode1 + ms_strEOB;
	if ( !strGcode2.IsEmpty() )
		m_strGcode += (*ms_pfnGetLineNo)() + strGcode2 + ms_strEOB;
}

// X|Z���̕ω�
CNCMakeLathe::CNCMakeLathe(int nCode, int xz, double dVal)
{
	CString	strGcode;
	CString	strValue(GetValString(xz, dVal, FALSE));
	if ( !strValue.IsEmpty() ) {
		strGcode = (*ms_pfnGetGString)(nCode) + strValue;
		if ( nCode > 0 )	// G00�ȊO
			strGcode += GetFeedString(GetDbl(
				xz==NCA_X ? MKLA_DBL_XFEED : MKLA_DBL_FEED));
	}
	if ( !strGcode.IsEmpty() )
		m_strGcode = (*ms_pfnGetLineNo)() + strGcode + ms_strEOB;
}

// �C�ӂ̕�������
CNCMakeLathe::CNCMakeLathe(const CString& strGcode) : CNCMakeBase(strGcode)
{
}

//////////////////////////////////////////////////////////////////////
// ���ފ֐�

CString	CNCMakeLathe::MakeSpindle(void)
{
	return (*ms_pfnGetSpindle)(GetNum(MKLA_NUM_SPINDLE));
}

CString CNCMakeLathe::GetValString(int xyz, double dVal, BOOL)
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
	}

	// ���l�����͵�߼�݂ɂ���ē��I�Ɋ֐����Ăяo��
	if ( xyz == NCA_X )
		dVal *= 2.0;	// X�����a�w��
	strResult = g_szNdelimiter[xyz] + (*ms_pfnGetValDetail)(dVal);

	return strResult;
}

void CNCMakeLathe::SetStaticOption(const CNCMakeLatheOpt* pNCMake)
{
	extern	LPCTSTR	gg_szReturn;
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
	ms_pfnGetValDetail = GetNum(MKLA_NUM_DOT) == 0 ?
		(GetFlg(MKLA_FLG_ZEROCUT) ?
			&GetValString_UZeroCut : &GetValString_Normal) : &GetValString_Multi1000;
	// --- �s�ԍ�������
	ms_nMagni = GetNum(MKLA_NUM_LINEADD)<0 ||
					 GetNum(MKLA_NUM_LINEADD)>=SIZEOF(nLineMulti) ?
		nLineMulti[0] : nLineMulti[GetNum(MKLA_NUM_LINEADD)];
	// --- EOB
	ms_strEOB = GetStr(MKLA_STR_EOB).IsEmpty() ? 
		gg_szReturn : GetStr(MKLA_STR_EOB) + gg_szReturn;
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
	// --- �ȉ~����
	ms_dEllipse = GetDbl(MKLA_DBL_ELLIPSE);
}
