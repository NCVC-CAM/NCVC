// NCMakeBase.cpp: CNCMakeBase �N���X�̃C���v�������e�[�V����
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NCVC.h"
#include "DXFdata.h"
#include "NCMakeOption.h"
#include "NCMakeMillOpt.h"
#include "NCMakeLatheOpt.h"
#include "NCMakeWireOpt.h"
#include "NCMakeBase.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

/////////////////////////////////////////////////////////////////////////////
// �ÓI�ϐ��̏�����
const	CNCMakeOption*	CNCMakeBase::ms_pMakeOpt = NULL;
double	CNCMakeBase::ms_xyz[] = {HUGE_VAL, HUGE_VAL, HUGE_VAL};
int		CNCMakeBase::NCAX = NCA_X;
int		CNCMakeBase::NCAY = NCA_Y;
int		CNCMakeBase::NCAI = NCA_I;
int		CNCMakeBase::NCAJ = NCA_J;
#undef	NCA_X	// �ȍ~�ANCA_X, NCA_Y �͖���
#undef	NCA_Y	// NCA_I, NCA_J �́AGetIJK() �ɂ̂ݗL��
int		CNCMakeBase::ms_nGcode = -1;
int		CNCMakeBase::ms_nSpindle = -1;
double	CNCMakeBase::ms_dFeed = -1.0;
int		CNCMakeBase::ms_nCnt = 1;
int		CNCMakeBase::ms_nMagni = 1;
int		CNCMakeBase::ms_nCircleCode = 2;
double	CNCMakeBase::ms_dEllipse = 0.5;
CString	CNCMakeBase::ms_strEOB;
PFNGETARGINT		CNCMakeBase::ms_pfnGetSpindle = &CNCMakeBase::GetSpindleString;
PFNGETARGDOUBLE		CNCMakeBase::ms_pfnGetFeed = &CNCMakeBase::GetFeedString_Integer;
PFNGETARGVOID		CNCMakeBase::ms_pfnGetLineNo = &CNCMakeBase::GetLineNoString;
PFNGETARGINT		CNCMakeBase::ms_pfnGetGString = &CNCMakeBase::GetGString;
PFNGETVALSTRING		CNCMakeBase::ms_pfnGetValString = NULL;		// �h���׽�Ō���
PFNGETARGDOUBLE		CNCMakeBase::ms_pfnGetValDetail = &CNCMakeBase::GetValString_Normal;
PFNMAKECIRCLESUB	CNCMakeBase::ms_pfnMakeCircleSub = &CNCMakeBase::MakeCircleSub_IJ;
PFNMAKECIRCLE		CNCMakeBase::ms_pfnMakeCircle = &CNCMakeBase::MakeCircle_IJ;
PFNMAKEHELICAL		CNCMakeBase::ms_pfnMakeHelical = &CNCMakeBase::MakeCircle_IJ_Helical;
PFNMAKEARC			CNCMakeBase::ms_pfnMakeArc = &CNCMakeBase::MakeArc_IJ;

//////////////////////////////////////////////////////////////////////
// �ݽ�׸�

CNCMakeBase::CNCMakeBase()
{
}

// �C�ӂ̕�������
CNCMakeBase::CNCMakeBase(const CString& strGcode)
{
	extern	LPCTSTR	gg_szReturn;

	if ( strGcode.IsEmpty() )
		m_strGcode = gg_szReturn;
	else if ( strGcode[0] == '%' )
		m_strGcode = strGcode + gg_szReturn;
	else {
		CString	str(strGcode);
		str.Replace("\\n", "\n");	// ���s���ނ̒u��
		m_strGcode = (*ms_pfnGetLineNo)() + str + ms_strEOB;
	}
}

//////////////////////////////////////////////////////////////////////
// ���ފ֐�

void CNCMakeBase::InitialVariable(void)
{
	ms_nGcode = -1;
	ms_nSpindle = -1;
	ms_dFeed = -1.0;
	ms_nCnt = 1;
}

void CNCMakeBase::SetStaticOption(const CNCMakeOption* pNCMake)
{
	ms_pMakeOpt = pNCMake;
}

void CNCMakeBase::MakeEllipse(const CDXFellipse* pEllipse, double dFeed)
{
	CString	strGcode;
	BOOL	bFeed = TRUE;
	CPointD	pt, ptMake;
	double	sq, eq,
			// �p�x�̽ï�ߐ������߂� -> (sq-eq) / (r*(sq-eq) / STEP)
			dStep = ms_dEllipse / pEllipse->GetR();

	// �ȉ~���̊J�n�I���p�x���Čv�Z
	// -> XY���]�ȂǌX�����l�����Ȃ��Ɛ������J�n�ʒu���ێ��ł��Ȃ�����
	if ( pEllipse->IsArc() ) {
		sq = pEllipse->GetStartAngle();
		eq = pEllipse->GetEndAngle();
	}
	else {
		pt = pEllipse->GetStartCutterPoint() - pEllipse->GetMakeCenter();
		sq = atan2(pt.y, pt.x) - pEllipse->GetMakeLean();	// �X�����z��
		if ( pEllipse->GetRound() )
			eq = sq + RAD(360.0);
		else
			eq = sq - RAD(360.0);
	}

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

CString	CNCMakeBase::MakeEllipse_Tolerance(const CDXFellipse* pEllipse, double q)
{
	CPointD	pt    ( pEllipse->GetLongLength()  * cos(q),
					pEllipse->GetShortLength() * sin(q) );
	CPointD	ptMake( pt.x * pEllipse->GetMakeLeanCos() - pt.y * pEllipse->GetMakeLeanSin(),
					pt.x * pEllipse->GetMakeLeanSin() + pt.y * pEllipse->GetMakeLeanCos() );
	ptMake += pEllipse->GetMakeCenter();
	pt = ptMake.RoundUp();
	return CString( (*ms_pfnGetGString)(1) +
		(*ms_pfnGetValString)(NCAX, pt.x, FALSE) + (*ms_pfnGetValString)(NCAY, pt.y, FALSE) );
}

void CNCMakeBase::MakePolylineCut(const CDXFpolyline* pPoly, double dFeed)
{
	if ( pPoly->GetVertexCount() <= 1 )
		return;

	BOOL	bFeed = TRUE;
	CString	strGcode;
	CDXFdata*	pData;

	// SwapMakePt()�ŏ���������ւ���Ă��[�_�͕K�� CDXFpoint
	POSITION pos = pPoly->GetFirstVertex();
	pPoly->GetNextVertex(pos);
	// �Q�_�ڂ���ٰ��
	while ( pos ) {
		pData = pPoly->GetNextVertex(pos);
		switch ( pData->GetMakeType() ) {
		case DXFPOINTDATA:
			strGcode = (*ms_pfnGetGString)(1) +
					(*ms_pfnGetValString)(NCAX, pData->GetEndMakePoint().x, FALSE) +
					(*ms_pfnGetValString)(NCAY, pData->GetEndMakePoint().y, FALSE);
			// �ŏ��������葬�x�ǉ�
			if ( bFeed && !strGcode.IsEmpty() ) {
				strGcode += GetFeedString(dFeed);
				bFeed = FALSE;
			}
			if ( !strGcode.IsEmpty() )
				m_strGarray.Add((*ms_pfnGetLineNo)() + strGcode + ms_strEOB);
			break;

		case DXFARCDATA:
			strGcode = (*ms_pfnMakeArc)(static_cast<CDXFarc*>(pData), dFeed);
			if ( !strGcode.IsEmpty() ) {
				m_strGarray.Add((*ms_pfnGetLineNo)() + strGcode);
				bFeed = FALSE;
			}
			// �I�_�����΂�
			pPoly->GetNextVertex(pos);
			break;

		case DXFELLIPSEDATA:
			MakeEllipse(static_cast<CDXFellipse*>(pData), dFeed);
			bFeed = FALSE;
			// �I�_�����΂�
			pPoly->GetNextVertex(pos);
			break;
		}
	}	// End of loop
}

CString CNCMakeBase::MakeCustomString
	(int nCode, DWORD dwValFlags/*=0*/, double* dValue/*=NULL*/, BOOL bReflect/*=TRUE*/)
{
	extern	LPCTSTR	g_szNdelimiter;		// "XYZUVWIJKRPLDH" from NCDoc.cpp
	extern	const	DWORD	g_dwSetValFlags[];

	// ���W�l��O��l�ɔ��f�����Ȃ��ꍇ(G92�Ȃ�)�͒��� ms_pfnGetValDetail ���Ăяo��
	CString	strResult;
	if ( nCode >= 0 )
		strResult = bReflect ? (*ms_pfnGetGString)(nCode) : GetGString(nCode);
	if ( dValue ) {
		for ( int i=0; i<VALUESIZE; i++ ) {
			if ( dwValFlags & g_dwSetValFlags[i] )
				strResult += bReflect || i>=GVALSIZE ?	// i>=NCA_P
								(*ms_pfnGetValString)(i, dValue[i], FALSE) :
								(g_szNdelimiter[i]+(*ms_pfnGetValDetail)(dValue[i]));
		}
	}
	return strResult;
}

CString CNCMakeBase::MakeCustomString(int nCode, int nValFlag[], double dValue[])
{
	int		n;
	// nValFlag�Ɏw�肳�ꂽ���ɒl��ǉ�
	CString	strResult( (*ms_pfnGetGString)(nCode) );
	for ( int i=0; i<VALUESIZE && nValFlag[i]>0; i++ ) {
		n = nValFlag[i];
		strResult += (*ms_pfnGetValString)(n, dValue[n], FALSE);
	}
	return strResult;
}

//////////////////////////////////////////////////////////////////////

// ��]�w��
CString	CNCMakeBase::GetSpindleString(int nSpindle)
{
	CString	strResult;
	if ( ms_nSpindle != nSpindle ) {
		strResult.Format("S%d", nSpindle);
		ms_nSpindle = nSpindle;
	}
	return strResult;
}

CString	CNCMakeBase::GetSpindleString_Clip(int)
{
	return CString();
}

// ���葬�x
CString	CNCMakeBase::GetFeedString(double dFeed)
{
	CString		strResult;
	if ( dFeed!=0.0 && ms_dFeed!=dFeed ) {
		ms_dFeed = dFeed;
		strResult = "F" + (*ms_pfnGetFeed)(dFeed);
	}
	return strResult;
}

CString	CNCMakeBase::GetFeedString_Integer(double dFeed)
{
	CString	strResult;
	// 	GetFeedString()����̎Q�Ƃ̂��� if() �s�v
	strResult.Format("%d", (int)dFeed);
	return strResult;
}

// �s�ԍ��t��
CString	CNCMakeBase::GetLineNoString(void)
{
	ASSERT( MKNC_STR_LINEFORM == MKLA_STR_LINEFORM );	// ����̍�
	ASSERT( MKNC_STR_LINEFORM == MKWI_STR_LINEFORM );

	CString	strResult;
	strResult.Format(ms_pMakeOpt->GetStr(MKNC_STR_LINEFORM), ms_nCnt++ * ms_nMagni);
	return strResult;
}

CString	CNCMakeBase::GetLineNoString_Clip(void)
{
	return CString();
}

// G����Ӱ���
CString	CNCMakeBase::GetGString(int nCode)
{
	CString		strResult;
	strResult.Format("G%02d", nCode);
	return strResult;
}

CString	CNCMakeBase::GetGString_Clip(int nCode)
{
	CString		strResult;
	if ( ms_nGcode != nCode ) {
		strResult = GetGString(nCode);
		ms_nGcode = nCode;
	}
	return strResult;
}

// ���W�l�ݒ�
CString	CNCMakeBase::GetValString_Normal(double dVal)
{
	CString		strResult;
	strResult.Format(IDS_MAKENCD_FORMAT, dVal);
	return strResult;
}

CString	CNCMakeBase::GetValString_UZeroCut(double dVal)
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

CString	CNCMakeBase::GetValString_Multi1000(double dVal)
{
	CString		strResult;
/*	--- RoundUp() �ɂď����ς݂̂��ߕs�v
	dVal *= 1000.0;
	dVal  = (dVal<0 ? floor(dVal) : ceil(dVal));
	strResult.Format("%ld", (long)dVal);
*/
	// �P����1000�{�ł́C�ۂߌ덷������(??)
	strResult.Format("%d", (int)(dVal*1000.0+_copysign(0.0001, dVal)));
	return strResult;
}

// �~�E�~�ʂ̐����⏕
CString	CNCMakeBase::MakeCircleSub_R(int nCode, const CPointD& pt, const CPointD&, double r)
{
	return CString( (*ms_pfnGetGString)(nCode) +
		(*ms_pfnGetValString)(NCAX, pt.x, FALSE) +
		(*ms_pfnGetValString)(NCAY, pt.y, FALSE) +
		(*ms_pfnGetValString)(NCA_R, r,    FALSE) );
}

CString	CNCMakeBase::MakeCircleSub_IJ(int nCode, const CPointD& pt, const CPointD& ptij, double)
{
	return CString( (*ms_pfnGetGString)(nCode) +
		(*ms_pfnGetValString)(NCAX, pt.x,   FALSE) +
		(*ms_pfnGetValString)(NCAY, pt.y,   FALSE) +
		(*ms_pfnGetValString)(NCAI, ptij.x, FALSE) +
		(*ms_pfnGetValString)(NCAJ, ptij.y, FALSE) );
}

CString	CNCMakeBase::MakeCircleSub_Helical(int nCode, const CPoint3D& pt)
{
	return CString( (*ms_pfnGetLineNo)() + (*ms_pfnGetGString)(nCode) +
		(*ms_pfnGetValString)(NCAX,  pt.x, FALSE) +
		(*ms_pfnGetValString)(NCAY,  pt.y, FALSE) +
		(*ms_pfnGetValString)(NCA_Z, pt.z, FALSE) );	// �ضق�MILL�̂�
}

// �~�ް��̐���
CString	CNCMakeBase::MakeCircle_R(const CDXFcircle* pCircle, double dFeed)
{
	int		a = pCircle->GetBaseAxis(), b = a & 0x01 ? (a-1) : (a+1),
			nCode = pCircle->IsRoundFixed() ? pCircle->GetG() : ms_nCircleCode;
	double	r = pCircle->GetMakeR();
	CPointD	pt;		// dummy
	// R�w��C180�K�������Đ���
	CString	strGcode;
	CString	strBuf1( MakeCircleSub_R(nCode, pCircle->GetMakePoint(b), pt, r) );
	CString	strBuf2( MakeCircleSub_R(nCode, pCircle->GetMakePoint(a), pt, r) );
	if ( !strBuf1.IsEmpty() && !strBuf2.IsEmpty() )
		strGcode = (*ms_pfnGetLineNo)() + strBuf1 + GetFeedString(dFeed) + ms_strEOB +
						(*ms_pfnGetLineNo)() + strBuf2 + ms_strEOB;
	return strGcode;
}

CString	CNCMakeBase::MakeCircle_IJ(const CDXFcircle* pCircle, double dFeed)
{
	CString	strGcode, strBuf( pCircle->GetBaseAxis() > 1 ?	// X����Y����
						(*ms_pfnGetValString)(NCAJ, pCircle->GetIJK(NCA_J), FALSE) :
						(*ms_pfnGetValString)(NCAI, pCircle->GetIJK(NCA_I), FALSE) ); 
	int		nCode = pCircle->IsRoundFixed() ? pCircle->GetG() : ms_nCircleCode;
	if ( !strBuf.IsEmpty() )
		strGcode = (*ms_pfnGetLineNo)() + (*ms_pfnGetGString)(nCode) +
					strBuf + GetFeedString(dFeed) + ms_strEOB;
	return strGcode;
}

CString	CNCMakeBase::MakeCircle_IJHALF(const CDXFcircle* pCircle, double dFeed)
{
	int		a = pCircle->GetBaseAxis(), b = a & 0x01 ? (a-1) : (a+1),
			nCode = pCircle->IsRoundFixed() ? pCircle->GetG() : ms_nCircleCode;
	CPointD	ij;
	// ����̌���
	if ( a > 1 ) 	// Y���ް�
		ij.y = pCircle->GetIJK(NCA_J);
	else 			// X���ް�
		ij.x = pCircle->GetIJK(NCA_I);
	CString	strGcode;
	CString	strBuf1( MakeCircleSub_IJ(nCode, pCircle->GetMakePoint(b), ij, 0.0) );
	ij *= -1.0;		// ij = -ij;
	CString	strBuf2( MakeCircleSub_IJ(nCode, pCircle->GetMakePoint(a), ij, 0.0) );
	if ( !strBuf1.IsEmpty() && !strBuf2.IsEmpty() )
		strGcode = (*ms_pfnGetLineNo)() + strBuf1 + GetFeedString(dFeed) + ms_strEOB +
						(*ms_pfnGetLineNo)() + strBuf2 + ms_strEOB;
	return strGcode;
}

CString	CNCMakeBase::MakeCircle_R_Helical(const CDXFcircle* pCircle, double dFeed, double dHelical)
{
	int		a = pCircle->GetBaseAxis(), b = a & 0x01 ? (a-1) : (a+1),
			nCode = pCircle->IsRoundFixed() ? pCircle->GetG() : ms_nCircleCode;
	double	r = pCircle->GetMakeR(),
			s = ms_pMakeOpt->GetDbl(MKNC_DBL_ZSTEP),	// �ضق�MILL�̂�
			z = dHelical - s + s/2.0;
	CPoint3D	pt1(pCircle->GetMakePoint(b).x, pCircle->GetMakePoint(b).y, z),
				pt2(pCircle->GetMakePoint(a).x, pCircle->GetMakePoint(a).y, dHelical);
	CString	strGcode, strBuf( (*ms_pfnGetValString)(NCA_R, r, FALSE) );
	if ( !strBuf.IsEmpty() ) {
		// �v�Z�����̊֌W�łP�s�ɂł��Ȃ�
		strGcode  = MakeCircleSub_Helical(nCode, pt1) + strBuf + GetFeedString(dFeed) + ms_strEOB;
		strGcode += MakeCircleSub_Helical(nCode, pt2) + strBuf + ms_strEOB;
	}
	return strGcode;
}

CString	CNCMakeBase::MakeCircle_IJ_Helical(const CDXFcircle* pCircle, double dFeed, double dHelical)
{
	CString	strGcode, strBuf( pCircle->GetBaseAxis() > 1 ?
						(*ms_pfnGetValString)(NCAJ, pCircle->GetIJK(NCA_J), FALSE) :
						(*ms_pfnGetValString)(NCAI, pCircle->GetIJK(NCA_I), FALSE) ); 
	int		nCode = pCircle->IsRoundFixed() ? pCircle->GetG() : ms_nCircleCode;
	if ( !strBuf.IsEmpty() )
		strGcode = (*ms_pfnGetLineNo)() + (*ms_pfnGetGString)(nCode) +
					(*ms_pfnGetValString)(NCA_Z, dHelical, FALSE) +
					strBuf + GetFeedString(dFeed) + ms_strEOB;
	return strGcode;
}

CString	CNCMakeBase::MakeCircle_IJHALF_Helical(const CDXFcircle* pCircle, double dFeed, double dHelical)
{
	int		a = pCircle->GetBaseAxis(), b = a & 0x01 ? (a-1) : (a+1),
			nCode = pCircle->IsRoundFixed() ? pCircle->GetG() : ms_nCircleCode;
	double	s = ms_pMakeOpt->GetDbl(MKNC_DBL_ZSTEP),	// �ضق�MILL�̂�
			z = dHelical - s + s/2.0;
	CPointD		ij;
	CPoint3D	pt1(pCircle->GetMakePoint(b).x, pCircle->GetMakePoint(b).y, z),
				pt2(pCircle->GetMakePoint(a).x, pCircle->GetMakePoint(a).y, dHelical);
	if ( a > 1 )
		ij.y = pCircle->GetIJK(NCA_J);
	else
		ij.x = pCircle->GetIJK(NCA_I);
	CString	strGcode;
	CString	strBuf1( (*ms_pfnGetValString)(NCAI, ij.x, FALSE) + (*ms_pfnGetValString)(NCAJ, ij.y, FALSE) );
	ij *= -1.0;		// ij = -ij;
	CString	strBuf2( (*ms_pfnGetValString)(NCAI, ij.x, FALSE) + (*ms_pfnGetValString)(NCAJ, ij.y, FALSE) );
	if ( !strBuf1.IsEmpty() && !strBuf2.IsEmpty() ) {
		// �v�Z�����̊֌W�łP�s�ɂł��Ȃ�
		strGcode  = MakeCircleSub_Helical(nCode, pt1) + strBuf1 + GetFeedString(dFeed) + ms_strEOB;
		strGcode += MakeCircleSub_Helical(nCode, pt2) + strBuf2 + ms_strEOB;
	}
	return strGcode;
}

// �~���ް��̐���
CString	CNCMakeBase::MakeArc_R(const CDXFarc* pArc, double dFeed)
{
	CString	strGcode,
			strBuf( MakeCircleSub_R(pArc->GetG(), pArc->GetEndMakePoint(), CPointD(), pArc->GetMakeR()) );
	if ( !strBuf.IsEmpty() )
		strGcode = (*ms_pfnGetLineNo)() + strBuf + GetFeedString(dFeed) + ms_strEOB;
	return strGcode;
}

CString	CNCMakeBase::MakeArc_IJ(const CDXFarc* pArc, double dFeed)
{
	CPointD	ij(pArc->GetIJK(NCA_I), pArc->GetIJK(NCA_J));
	CString	strGcode,
			strBuf( MakeCircleSub_IJ(pArc->GetG(), pArc->GetEndMakePoint(), ij, 0.0) );
	if ( !strBuf.IsEmpty() )
		strGcode = (*ms_pfnGetLineNo)() + strBuf + GetFeedString(dFeed) + ms_strEOB;
	return strGcode;
}

//////////////////////////////////////////////////////////////////////

// G���ޏo��
void CNCMakeBase::WriteGcode(CStdioFile& fp)
{
	if ( !m_strGcode.IsEmpty() )
		fp.WriteString( m_strGcode );
	for ( int i=0; i<m_strGarray.GetSize(); i++ )
		fp.WriteString( m_strGarray[i] );
}
