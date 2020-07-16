// NCMakeClass.cpp: CNCMake �N���X�̃C���v�������e�[�V����
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NCVC.h"
#include "DXFdata.h"
#include "NCMakeOption.h"
#include "NCMakeClass.h"

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
const	CNCMakeOption*	CNCMake::ms_pMakeOpt = NULL;
double	CNCMake::ms_xyz[] = {HUGE_VAL, HUGE_VAL, HUGE_VAL};
int		CNCMake::ms_nGcode = -1;
int		CNCMake::ms_nSpindle = -1;
double	CNCMake::ms_dFeed = -1.0;
double	CNCMake::ms_dCycleZ[] = {HUGE_VAL, HUGE_VAL};
double	CNCMake::ms_dCycleR[] = {HUGE_VAL, HUGE_VAL};
double	CNCMake::ms_dCycleP[] = {HUGE_VAL, HUGE_VAL};
int		CNCMake::ms_nCnt = 1;
int		CNCMake::ms_nMagni = 1;
int		CNCMake::ms_nCircleCode = 2;
int		CNCMake::ms_nCycleCode = 81;
int		CNCMake::ms_nCycleReturn = 88;
CString	CNCMake::ms_strEOB;
PFNGETSPINDLE		CNCMake::ms_pfnGetSpindle = &CNCMake::GetSpindleString;
PFNGETFEED			CNCMake::ms_pfnGetFeed = &CNCMake::GetFeedString_Integer;
PFNGETLINENO		CNCMake::ms_pfnGetLineNo = &CNCMake::GetLineNoString;
PFNGETGSTRING		CNCMake::ms_pfnGetGString = &CNCMake::GetGString;
PFNGETCYCLESTRING	CNCMake::ms_pfnGetCycleString = &CNCMake::GetCycleString;
PFNGETVALSTRING		CNCMake::ms_pfnGetValString = &CNCMake::GetValString_Normal;
PFNMAKECIRCLESUB	CNCMake::ms_pfnMakeCircleSub = &CNCMake::MakeCircleSub_IJ;
PFNMAKECIRCLE		CNCMake::ms_pfnMakeCircle = &CNCMake::MakeCircle_IJ;
PFNMAKEARC			CNCMake::ms_pfnMakeArc = &CNCMake::MakeArc_IJ;

//////////////////////////////////////////////////////////////////////
// CNCMake �\�z/����
//////////////////////////////////////////////////////////////////////
CNCMake::CNCMake(const CDXFdata* pData, double dFeed, const CPointD* lpt/*=NULL*/)
{
	MAKECIRCLE	mc;
	CString		strGcode;
	CPointD		pt;

	// �{�ް�
	switch ( pData->GetMakeType() ) {
	case DXFPOINTDATA:
		strGcode = (*ms_pfnGetCycleString)() +
			GetValString(NCA_X, pData->GetEndMakePoint().x) +
			GetValString(NCA_Y, pData->GetEndMakePoint().y);
		if ( !GetFlg(MKNC_FLG_GCLIP) || ms_dCycleZ[0]!=ms_dCycleZ[1] ) {
			strGcode += GetValString(NCA_Z, ms_dCycleZ[0]);
			ms_dCycleZ[1] = ms_dCycleZ[0];
		}
		if ( !GetFlg(MKNC_FLG_GCLIP) || ms_dCycleR[0]!=ms_dCycleR[1] ) {
			strGcode += GetValString(NCA_R, ms_dCycleR[0], TRUE);
			ms_dCycleR[1] = ms_dCycleR[0];
		}
		if ( ms_dCycleP[0] > 0 ) {
			if ( !GetFlg(MKNC_FLG_GCLIP) || ms_dCycleP[0]!=ms_dCycleP[1] ) {
				strGcode += GetValString(NCA_P, ms_dCycleP[0], TRUE);
				ms_dCycleP[1] = ms_dCycleP[0];
			}
		}
		if ( !strGcode.IsEmpty() )
			m_strGcode += (*ms_pfnGetLineNo)() + strGcode + GetFeedString(dFeed) + ms_strEOB;
		// Z���̕��A�_��ÓI�ϐ���
		ms_xyz[NCA_Z] = GetNum(MKNC_NUM_ZRETURN) == 0 ?
			::RoundUp(GetDbl(MKNC_DBL_G92Z)) : ::RoundUp(GetDbl(MKNC_DBL_ZG0STOP));
		break;

	case DXFLINEDATA:
		if ( lpt ) {	// �������W�w��
			pt.x = lpt->x;
			pt.y = lpt->y;
		}
		else
			pt = pData->GetEndMakePoint();
		strGcode = (*ms_pfnGetGString)(1) + GetValString(NCA_X, pt.x) + GetValString(NCA_Y, pt.y);
		if ( !strGcode.IsEmpty() )
			m_strGcode += (*ms_pfnGetLineNo)() + strGcode + GetFeedString(dFeed) + ms_strEOB;
		break;

	case DXFCIRCLEDATA:
		m_strGcode += (*ms_pfnMakeCircle)((CDXFcircle *)pData, dFeed);
		break;

	case DXFARCDATA:
		m_strGcode += (*ms_pfnMakeArc)((CDXFarc *)pData, dFeed, lpt);
		break;

	case DXFELLIPSEDATA:
		m_strGarray.SetSize(0, 1024);
		MakeEllipse((CDXFellipse *)pData, dFeed);
		break;

	case DXFPOLYDATA:
		m_strGarray.SetSize(0, 1024);
		MakePolylineCut((CDXFpolyline *)pData, dFeed);
		break;
	}
}

CNCMake::CNCMake(const CDXFdata* pData)
{
	CPointD	pt;

	switch ( pData->GetMakeType() ) {
	case DXFLINEDATA:
		pt = pData->GetMakePoint(0);
		// ���̵�޼ު�Ăƌ��݈ʒu���Ⴄ�Ȃ�
		if ( pt.x!=ms_xyz[NCA_X] || pt.y!=ms_xyz[NCA_Y] ) {
			m_strGcode = (*ms_pfnGetLineNo)() + (*ms_pfnGetGString)(0) +
				GetValString(NCA_X, pt.x) + GetValString(NCA_Y, pt.y) +
				ms_strEOB;
		}
		// through
	case DXFCIRCLEDATA:
		// ��޼ު�Ă̈ړ��ް�����
		pt = pData->GetEndMakePoint();
		if ( pt.x!=ms_xyz[NCA_X] || pt.y!=ms_xyz[NCA_Y] ) {
			m_strGcode += (*ms_pfnGetLineNo)() + (*ms_pfnGetGString)(0) +
				GetValString(NCA_X, pt.x) + GetValString(NCA_Y, pt.y) +
				ms_strEOB;
		}
		break;

	case DXFPOLYDATA:
		m_strGarray.SetSize(0, 1024);
		MakePolylineMov((CDXFpolyline *)pData);
		break;
	}
}

//////////////////////////////////////////////////////////////////////
// ���ފ֐�

void CNCMake::MakeEllipse(const CDXFellipse* pEllipse, double dFeed)
{
	CString	strGcode;
	BOOL	bFeed = TRUE;
	CPointD	pt, ptMake;
	double	sq = pEllipse->GetStartAngle(), eq = pEllipse->GetEndAngle();
	// �p�x�̽ï�ߐ������߂� -> (sq-eq) / (r*(sq-eq) / STEP)
	double	dStep = 1.0 / (pEllipse->GetR() / GetDbl(MKNC_DBL_ELLIPSE));

	// �����J�n
	if ( pEllipse->GetRound() ) {
		for ( sq+=dStep; sq<eq; sq+=dStep ) {
			strGcode = MakeEllipse_Tolerance(pEllipse, sq);
			if ( bFeed && !strGcode.IsEmpty() ) {
				strGcode += GetFeedString(dFeed);
				bFeed = FALSE;
			}
			if ( !strGcode.IsEmpty() )
				m_strGarray.Add((*ms_pfnGetLineNo)() + strGcode + ms_strEOB);
		}
	}
	else {
		for ( sq-=dStep; sq>eq; sq-=dStep ) {
			strGcode = MakeEllipse_Tolerance(pEllipse, sq);
			if ( bFeed && !strGcode.IsEmpty() ) {
				strGcode += GetFeedString(dFeed);
				bFeed = FALSE;
			}
			if ( !strGcode.IsEmpty() )
				m_strGarray.Add((*ms_pfnGetLineNo)() + strGcode + ms_strEOB);
		}
	}
	strGcode = MakeEllipse_Tolerance(pEllipse, eq);
	if ( bFeed && !strGcode.IsEmpty() )
		strGcode += GetFeedString(dFeed);
	if ( !strGcode.IsEmpty() )
		m_strGarray.Add((*ms_pfnGetLineNo)() + strGcode + ms_strEOB);
}

void CNCMake::MakePolylineCut(const CDXFpolyline* pPoly, double dFeed)
{
	if ( pPoly->GetVertexCount() <= 1 )
		return;

	BOOL	bFeed = TRUE;
	CString	strGcode;
	const	CDXFdata*		pData;

	// SwapPt()�ŏ���������ւ���Ă��[�_�͕K�� CDXFpoint
	POSITION pos = pPoly->GetFirstVertex();
	pPoly->GetNextVertex(pos);
	// �Q�_�ڂ���ٰ��
	while ( pos ) {
		pData = pPoly->GetNextVertex(pos);
		switch ( pData->GetMakeType() ) {
		case DXFPOINTDATA:
			strGcode = (*ms_pfnGetGString)(1) +
					GetValString(NCA_X, pData->GetEndMakePoint().x) +
					GetValString(NCA_Y, pData->GetEndMakePoint().y);
			// �ŏ��������葬�x�ǉ�
			if ( bFeed && !strGcode.IsEmpty() ) {
				strGcode += GetFeedString(dFeed);
				bFeed = FALSE;
			}
			if ( !strGcode.IsEmpty() )
				m_strGarray.Add((*ms_pfnGetLineNo)() + strGcode + ms_strEOB);
			break;

		case DXFARCDATA:
			strGcode = (*ms_pfnMakeArc)((CDXFarc *)pData, dFeed, NULL);
			if ( !strGcode.IsEmpty() ) {
				m_strGarray.Add((*ms_pfnGetLineNo)() + strGcode);
				bFeed = FALSE;
			}
			// �I�_�����΂�
			pPoly->GetNextVertex(pos);
			break;

		case DXFELLIPSEDATA:
			MakeEllipse((CDXFellipse *)pData, dFeed);
			bFeed = FALSE;
			// �I�_�����΂�
			pPoly->GetNextVertex(pos);
			break;
		}
	}	// End of loop

	if ( pPoly->GetPolyFlag() & 1 )	{	// �������ײ݂Ȃ�
		// �Ōオ�ӂ����(�~��)�̏ꍇ�́C�ǂݍ��ݎ��Ɂu�J�������ײ݁v�����̂���
		// ������Ԑ����ŗǂ�
		strGcode = (*ms_pfnGetGString)(1) +
				GetValString(NCA_X, pPoly->GetMakePoint(0).x) +
				GetValString(NCA_Y, pPoly->GetMakePoint(0).y);
		if ( bFeed && !strGcode.IsEmpty() )
			strGcode += GetFeedString(dFeed);
		if ( !strGcode.IsEmpty() )
			m_strGarray.Add((*ms_pfnGetLineNo)() + strGcode + ms_strEOB);
	}
}

void CNCMake::MakePolylineMov(const CDXFpolyline* pPoly)
{
	if ( pPoly->GetVertexCount() <= 1 )
		return;

	CString	strGcode;
	const	CDXFdata*	pData;

	POSITION pos = pPoly->GetFirstVertex();
	pPoly->GetNextVertex(pos);
	while ( pos ) {
		pData = pPoly->GetNextVertex(pos);
		// �~��(�ӂ���ݏ��)�͖���
		if ( pData->GetMakeType() == DXFPOINTDATA ) {
			strGcode = (*ms_pfnGetGString)(0) +
					GetValString(NCA_X, pData->GetEndMakePoint().x) +
					GetValString(NCA_Y, pData->GetEndMakePoint().y);
			if ( !strGcode.IsEmpty() )
				m_strGarray.Add((*ms_pfnGetLineNo)() + strGcode + ms_strEOB);
		}
	}
	if ( pPoly->GetPolyFlag() & 1 )	{
		strGcode = (*ms_pfnGetGString)(0) +
				GetValString(NCA_X, pPoly->GetMakePoint(0).x) +
				GetValString(NCA_Y, pPoly->GetMakePoint(0).y);
		if ( !strGcode.IsEmpty() )
			m_strGarray.Add((*ms_pfnGetLineNo)() + strGcode + ms_strEOB);
	}
}

CString CNCMake::MakeCustomString
	(int nCode, DWORD dwValFlags/*=0*/, double* dValue/*=NULL*/, BOOL bReflect/*=TRUE*/)
{
	extern	LPCTSTR	g_szNdelimiter;		// "XYZRIJKPLDH" from NCDoc.cpp
	extern	const	DWORD	g_dwSetValFlags[];

	// ���W�l��O��l�ɔ��f�����Ȃ��ꍇ(G92�Ȃ�)�͒��� ms_pfnGetValString ���Ăяo��
	CString	strResult;
	if ( nCode >= 0 )
		strResult = bReflect ? (*ms_pfnGetGString)(nCode) : GetGString(nCode);
	if ( dValue ) {
		for ( int i=0; i<VALUESIZE; i++ ) {
			if ( dwValFlags & g_dwSetValFlags[i] )
				strResult += bReflect || i>=NCA_P ? GetValString(i, dValue[i]) :
								(g_szNdelimiter[i]+(*ms_pfnGetValString)(dValue[i]));
		}
	}
	return strResult;
}

CString CNCMake::MakeCustomString(int nCode, int nValFlag[], double dValue[])
{
	int		n;
	// nValFlag�Ɏw�肳�ꂽ���ɒl��ǉ�
	CString	strResult( (*ms_pfnGetGString)(nCode) );
	for ( int i=0; i<VALUESIZE && nValFlag[i]>0; i++ ) {
		n = nValFlag[i];
		strResult += GetValString(n, dValue[n]);
	}
	return strResult;
}

CString CNCMake::GetValString(int xyz, double dVal, BOOL bSpecial)
{
	extern	LPCTSTR	g_szNdelimiter;		// "XYZRIJKPLDH" from NCDoc.cpp
	CString	strResult;

	// �����_�ȉ��R��(�ȉ��؂�グ)�ł̒l���v����
	switch ( xyz ) {
	case NCA_X:
	case NCA_Y:
	case NCA_Z:
		if ( GetFlg(MKNC_FLG_GCLIP) && fabs(ms_xyz[xyz]-dVal)<EPS )
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
	case NCA_I:
	case NCA_J:
	case NCA_K:
		if ( fabs(dVal) < NCMIN )	// NC�̌������덷�����Ȃ疳��
			return strResult;
		break;
	case NCA_P:
		if ( bSpecial )	{	// G8x �Œ軲�� �޳�َ���
			if ( GetNum(MKNC_NUM_DWELLFORMAT) == 0 ) {	// �����_�\�L
				strResult = g_szNdelimiter[NCA_P] +
					(GetFlg(MKNC_FLG_ZEROCUT) ?
						GetValString_UZeroCut(dVal) : GetValString_Normal(dVal));
				return strResult;
			}	// �����\�L�� default �ŏ���
		}
		// through
	default:	// L(�����_�w��Ȃ�)
		strResult.Format("%c%02d", g_szNdelimiter[xyz], (int)dVal);
		return strResult;
	}

	// ���l�����͵�߼�݂ɂ���ē��I�Ɋ֐����Ăяo��
	strResult = g_szNdelimiter[xyz] + (*ms_pfnGetValString)(dVal);

	return strResult;
}

CString	CNCMake::GetValString_UZeroCut(double dVal)
{
	CString		strResult;
	if ( dVal == 0.0 ) {
		strResult = "0";
		return strResult;
	}
	int			nCnt;
	strResult = GetValString_Normal(dVal);
	for ( nCnt=strResult.GetLength(); nCnt>0 ; nCnt-- ) {
		if ( strResult[nCnt-1]=='.' || strResult[nCnt-1]!='0' )
			break;
	}
	return	strResult.Left(nCnt);
}

void CNCMake::SetStaticOption(void)
{
	extern	LPCTSTR	gg_szReturn;
	static	const	int		nLineMulti[] = {	// �s�ԍ�������
		1, 5, 10, 100
	};

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
	// --- ���W�\�L
	ms_pfnGetValString = GetNum(MKNC_NUM_DOT) == 0 ?
		(GetFlg(MKNC_FLG_ZEROCUT) ?
			&GetValString_UZeroCut : &GetValString_Normal) : &GetValString_Multi1000;
	// --- �s�ԍ��t��
	ms_pfnGetLineNo = GetFlg(MKNC_FLG_LINEADD) && !(GetStr(MKNC_STR_LINEFORM).IsEmpty()) ?
		&GetLineNoString : &GetLineNoString_Clip;
	// --- �s�ԍ�������
	ms_nMagni = GetNum(MKNC_NUM_LINEADD)<0 ||
					 GetNum(MKNC_NUM_LINEADD)>=SIZEOF(nLineMulti) ?
		nLineMulti[0] : nLineMulti[GetNum(MKNC_NUM_LINEADD)];
	// --- EOB
	ms_strEOB = GetStr(MKNC_STR_EOB).IsEmpty() ? 
		gg_szReturn : GetStr(MKNC_STR_EOB) + gg_szReturn;
	// --- G����Ӱ���
	if ( GetFlg(MKNC_FLG_GCLIP) ) {
		ms_pfnGetGString =  &GetGString_Clip;
		ms_pfnGetCycleString = &GetCycleString_Clip;
	}
	else {
		ms_pfnGetGString = &GetGString;
		ms_pfnGetCycleString = &GetCycleString;
	}
	// --- �Œ軲�َw��
	ms_nCycleReturn = GetNum(MKNC_NUM_ZRETURN) == 0 ? 98 : 99;
	if ( GetNum(MKNC_NUM_DWELL) > 0 )
		ms_nCycleCode = GetNum(MKNC_NUM_DRILLZPROCESS) == 0 ? 82 : 89;
	else
		ms_nCycleCode = GetNum(MKNC_NUM_DRILLZPROCESS) == 0 ? 81 : 85;
	// --- �~�ް��̐؍�w��
	ms_nCircleCode = GetNum(MKNC_NUM_CIRCLECODE) == 0 ? 2 : 3;
	// --- �~,�~���ް��̐���
	if ( GetNum(MKNC_NUM_IJ) == 0 ) {
		ms_pfnMakeCircleSub	= &MakeCircleSub_R;
		ms_pfnMakeCircle	= &MakeCircle_R;
		ms_pfnMakeArc		= &MakeArc_R;
	}
	else {
		ms_pfnMakeCircleSub	= &MakeCircleSub_IJ;
		ms_pfnMakeCircle	= GetFlg(MKNC_FLG_CIRCLEHALF) ? &MakeCircle_IJ_HALF : &MakeCircle_IJ;
		ms_pfnMakeArc		= &MakeArc_IJ;
	}
}
