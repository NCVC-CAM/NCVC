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

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using std::string;
using namespace boost;

extern	LPCTSTR	g_szGdelimiter;	// "GSMOF" from NCDoc.cpp

/////////////////////////////////////////////////////////////////////////////
// �ÓI�ϐ��̏�����
const	CNCMakeOption*	CNCMakeBase::ms_pMakeOpt = NULL;
float	CNCMakeBase::ms_xyz[] = {HUGE_VALF, HUGE_VALF, HUGE_VALF};
int		CNCMakeBase::NCAX = NCA_X;
int		CNCMakeBase::NCAY = NCA_Y;
int		CNCMakeBase::NCAI = NCA_I;
int		CNCMakeBase::NCAJ = NCA_J;
#undef	NCA_X	// �ȍ~�ANCA_X, NCA_Y �͖���
#undef	NCA_Y	// NCA_I, NCA_J �́AGetIJK() �ɂ̂ݗL��
int		CNCMakeBase::ms_nGcode = -1;
int		CNCMakeBase::ms_nSpindle = -1;
float	CNCMakeBase::ms_dFeed = -1.0f;
int		CNCMakeBase::ms_nCnt = 1;
int		CNCMakeBase::ms_nMagni = 1;
int		CNCMakeBase::ms_nCircleCode = 2;
BOOL	CNCMakeBase::ms_bIJValue = FALSE;
float	CNCMakeBase::ms_dEllipse = 0.5f;
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
	m_strGcode = GetChangeEnter(strGcode);
}

//////////////////////////////////////////////////////////////////////
// ���ފ֐�

void CNCMakeBase::InitialVariable(void)
{
	ms_nGcode = -1;
	ms_nSpindle = -1;
	ms_dFeed = -1.0f;
	ms_nCnt = 1;
}

void CNCMakeBase::SetStaticOption(const CNCMakeOption* pNCMake)
{
	ms_pMakeOpt = pNCMake;
}

void CNCMakeBase::MakeEllipse(const CDXFellipse* pEllipse, float dFeed)
{
	CString	strGcode;
	BOOL	bFeed = TRUE;
	CPointF	pt, ptMake;
	float	sq, eq,
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
		sq = pt.arctan() - pEllipse->GetMakeLean();	// �X�����z��
		if ( pEllipse->GetRound() )
			eq = sq + PI2;
		else
			eq = sq - PI2;
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
				AddGcode(strGcode);
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
				AddGcode(strGcode);
		}
	}
	strGcode = MakeEllipse_Tolerance(pEllipse, eq);
	if ( bFeed && !strGcode.IsEmpty() )
		strGcode += GetFeedString(dFeed);
	if ( !strGcode.IsEmpty() )
		AddGcode(strGcode);
}

CString	CNCMakeBase::MakeEllipse_Tolerance(const CDXFellipse* pEllipse, float q)
{
	CPointF	pt    ( pEllipse->GetLongLength()  * cos(q),
					pEllipse->GetShortLength() * sin(q) );
	CPointF	ptMake( pt.x * pEllipse->GetMakeLeanCos() - pt.y * pEllipse->GetMakeLeanSin(),
					pt.x * pEllipse->GetMakeLeanSin() + pt.y * pEllipse->GetMakeLeanCos() );
	ptMake += pEllipse->GetMakeCenter();
	pt = ptMake.RoundUp();
	return CString( (*ms_pfnGetGString)(1) +
		(*ms_pfnGetValString)(NCAX, pt.x, FALSE) + (*ms_pfnGetValString)(NCAY, pt.y, FALSE) );
}

void CNCMakeBase::MakePolylineCut(const CDXFpolyline* pPoly, float dFeed)
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
				AddGcode(strGcode);
			break;

		case DXFARCDATA:
			strGcode = (*ms_pfnMakeArc)(static_cast<CDXFarc*>(pData), dFeed);
			if ( !strGcode.IsEmpty() ) {
				AddGcode(strGcode);
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
	(int nCode, DWORD dwValFlags/*=0*/, float* dValue/*=NULL*/, BOOL bReflect/*=TRUE*/)
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

CString CNCMakeBase::MakeCustomString(int nCode, int nValFlag[], float dValue[])
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

// ���s�����u��
CString	CNCMakeBase::GetChangeEnter(const CString& strGcode)
{
	extern	LPCTSTR	gg_szReturn;	// "\n"
	CString	strResult;

	if ( strGcode.IsEmpty() )
		strResult = gg_szReturn;
	else if ( strGcode[0] == '%' )
		strResult = strGcode + gg_szReturn;
	else {
		CString	str(strGcode);
		str.Replace("\\n", gg_szReturn);	// ���s���ނ̒u��
		strResult = (*ms_pfnGetLineNo)() + str + ms_strEOB;
	}
	return strResult;
}

// ��]�w��
CString	CNCMakeBase::GetSpindleString(int nSpindle)
{
	CString	strResult;
	if ( ms_nSpindle != nSpindle ) {
		strResult = (g_szGdelimiter[S_TYPE] + lexical_cast<string>(nSpindle)).c_str();
		ms_nSpindle = nSpindle;
	}
	return strResult;
}

CString	CNCMakeBase::GetSpindleString_Clip(int)
{
	return CString();
}

// ���葬�x
CString	CNCMakeBase::GetFeedString(float dFeed)
{
	CString		strResult;
	if ( dFeed!=0.0f && ms_dFeed!=dFeed ) {
		ms_dFeed = dFeed;
		strResult = g_szGdelimiter[F_TYPE] + (*ms_pfnGetFeed)(dFeed);
	}
	return strResult;
}

CString	CNCMakeBase::GetFeedString_Integer(float dFeed)
{
	CString	strResult;
	// 	GetFeedString()����̎Q�Ƃ̂��� if() �s�v
	strResult = lexical_cast<string>((int)dFeed).c_str();
	return strResult;
}

// �s�ԍ��t��
CString	CNCMakeBase::GetLineNoString(void)
{
	CString	strResult;
	strResult.Format(ms_pMakeOpt->GetLineNoForm(), ms_nCnt++ * ms_nMagni);
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
	strResult.Format(IDS_COMMON_FORMAT, "G", nCode);	// %s%02d
	return strResult;
}

CString	CNCMakeBase::GetGString_Clip(int nCode)
{
	CString		strResult;
	
	switch ( nCode ) {
	// NCVC�Ő����\���̂��郏���V���b�g�R�[�h
	case 4:
	case 92:
		strResult = GetGString(nCode);
		break;
	// ���[�_���w��
	default:
		if ( ms_nGcode != nCode ) {
			strResult = GetGString(nCode);
			ms_nGcode = nCode;
		}
	}
	return strResult;
}

// ���W�l�ݒ�
CString	CNCMakeBase::GetValString_Normal(float dVal)
{
	CString		strResult;
	strResult.Format(IDS_MAKENCD_FORMAT, dVal);
	return strResult;
}

CString	CNCMakeBase::GetValString_UZeroCut(float dVal)
{
	CString		strResult;
	if ( fabs(dVal) < NCMIN ) {
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

CString	CNCMakeBase::GetValString_Multi1000(float dVal)
{
	CString		strResult;
	// �P����1000�{�ł́C�ۂߌ덷������(??)
	strResult = lexical_cast<string>((int)(dVal*1000.0f+copysign(0.0001f, dVal))).c_str();
	return strResult;
}

// �~�E�~�ʂ̐����⏕
CString	CNCMakeBase::MakeCircleSub_R(int nCode, const CPointF& pt, const CPointF&, float r)
{
	return CString( (*ms_pfnGetGString)(nCode) +
		(*ms_pfnGetValString)(NCAX,  pt.x, FALSE) +
		(*ms_pfnGetValString)(NCAY,  pt.y, FALSE) +
		(*ms_pfnGetValString)(NCA_R, r,    FALSE) );
}

CString	CNCMakeBase::MakeCircleSub_IJ(int nCode, const CPointF& pt, const CPointF& ptij, float)
{
	return CString( (*ms_pfnGetGString)(nCode) +
		(*ms_pfnGetValString)(NCAX, pt.x,   FALSE) +
		(*ms_pfnGetValString)(NCAY, pt.y,   FALSE) +
		(*ms_pfnGetValString)(NCAI, ptij.x, ms_bIJValue) +
		(*ms_pfnGetValString)(NCAJ, ptij.y, ms_bIJValue) );
}

CString	CNCMakeBase::MakeCircleSub_Helical(int nCode, const CPoint3F& pt)
{
	return CString( (*ms_pfnGetLineNo)() + (*ms_pfnGetGString)(nCode) +
		(*ms_pfnGetValString)(NCAX,  pt.x, FALSE) +
		(*ms_pfnGetValString)(NCAY,  pt.y, FALSE) +
		(*ms_pfnGetValString)(NCA_Z, pt.z, FALSE) );	// �ضق�MILL�̂�
}

// �~�ް��̐���
CString	CNCMakeBase::MakeCircle_R(const CDXFcircle* pCircle, float dFeed)
{
	int		a = pCircle->GetBaseAxis(), b = a & 0x01 ? (a-1) : (a+1),
			nCode = pCircle->IsRoundFixed() ? pCircle->GetG() : ms_nCircleCode;
	float	r = pCircle->GetMakeR();
	CPointF	pt;		// dummy
	// R�w��C180�K�������Đ���
	CString	strGcode;
	CString	strBuf1( MakeCircleSub_R(nCode, pCircle->GetMakePoint(b), pt, r) );
	CString	strBuf2( MakeCircleSub_R(nCode, pCircle->GetMakePoint(a), pt, r) );
	if ( !strBuf1.IsEmpty() && !strBuf2.IsEmpty() )
		strGcode = (*ms_pfnGetLineNo)() + strBuf1 + GetFeedString(dFeed) + ms_strEOB +
						(*ms_pfnGetLineNo)() + strBuf2 + ms_strEOB;
	return strGcode;
}

CString	CNCMakeBase::MakeCircle_IJ(const CDXFcircle* pCircle, float dFeed)
{
	int		nCode = pCircle->IsRoundFixed() ? pCircle->GetG() : ms_nCircleCode;
	return CString( (*ms_pfnGetLineNo)() + (*ms_pfnGetGString)(nCode) +
				(*ms_pfnGetValString)(NCAI, pCircle->GetIJK(NCA_I), ms_bIJValue) +
				(*ms_pfnGetValString)(NCAJ, pCircle->GetIJK(NCA_J), ms_bIJValue) +
				GetFeedString(dFeed) + ms_strEOB );
}

CString	CNCMakeBase::MakeCircle_IJHALF(const CDXFcircle* pCircle, float dFeed)
{
	int		a = pCircle->GetBaseAxis(), b = a & 0x01 ? (a-1) : (a+1),
			nCode = pCircle->IsRoundFixed() ? pCircle->GetG() : ms_nCircleCode;
	CPointF	ij;
	// ����̌���
	if ( a > 1 ) 	// Y���ް�
		ij.y = pCircle->GetIJK(NCA_J);
	else 			// X���ް�
		ij.x = pCircle->GetIJK(NCA_I);
	CString	strGcode;
	CString	strBuf1( (*ms_pfnMakeCircleSub)(nCode, pCircle->GetMakePoint(b), ij, 0.0f) );
	ij *= -1.0f;	// ij = -ij;
	CString	strBuf2( (*ms_pfnMakeCircleSub)(nCode, pCircle->GetMakePoint(a), ij, 0.0f) );
	if ( !strBuf1.IsEmpty() && !strBuf2.IsEmpty() )
		strGcode = (*ms_pfnGetLineNo)() + strBuf1 + GetFeedString(dFeed) + ms_strEOB +
						(*ms_pfnGetLineNo)() + strBuf2 + ms_strEOB;
	return strGcode;
}

CString	CNCMakeBase::MakeCircle_R_Helical(const CDXFcircle* pCircle, float dFeed, float dHelical)
{
	int		a = pCircle->GetBaseAxis(), b = a & 0x01 ? (a-1) : (a+1),
			nCode = pCircle->IsRoundFixed() ? pCircle->GetG() : ms_nCircleCode;
	float	r = pCircle->GetMakeR(),
			s = ms_pMakeOpt->GetDbl(MKNC_DBL_ZSTEP),	// �ضق�MILL�̂�
			z = dHelical - s + s/2.0f;
	CPoint3F	pt1(pCircle->GetMakePoint(b).x, pCircle->GetMakePoint(b).y, z),
				pt2(pCircle->GetMakePoint(a).x, pCircle->GetMakePoint(a).y, dHelical);
	CString	strGcode, strBuf( (*ms_pfnGetValString)(NCA_R, r, FALSE) );
	if ( !strBuf.IsEmpty() ) {
		// �v�Z�����̊֌W�łP�s�ɂł��Ȃ�
		strGcode  = MakeCircleSub_Helical(nCode, pt1) + strBuf + GetFeedString(dFeed) + ms_strEOB;
		strGcode += MakeCircleSub_Helical(nCode, pt2) + strBuf + ms_strEOB;
	}
	return strGcode;
}

CString	CNCMakeBase::MakeCircle_IJ_Helical(const CDXFcircle* pCircle, float dFeed, float dHelical)
{
	int		nCode = pCircle->IsRoundFixed() ? pCircle->GetG() : ms_nCircleCode;
	return CString( (*ms_pfnGetLineNo)() + (*ms_pfnGetGString)(nCode) +
				(*ms_pfnGetValString)(NCA_Z, dHelical, FALSE) +
				(*ms_pfnGetValString)(NCAI, pCircle->GetIJK(NCA_I), ms_bIJValue) +
				(*ms_pfnGetValString)(NCAJ, pCircle->GetIJK(NCA_J), ms_bIJValue) +
				GetFeedString(dFeed) + ms_strEOB );
}

CString	CNCMakeBase::MakeCircle_IJHALF_Helical(const CDXFcircle* pCircle, float dFeed, float dHelical)
{
	int		a = pCircle->GetBaseAxis(), b = a & 0x01 ? (a-1) : (a+1),
			nCode = pCircle->IsRoundFixed() ? pCircle->GetG() : ms_nCircleCode;
	float	s = ms_pMakeOpt->GetDbl(MKNC_DBL_ZSTEP),	// �ضق�MILL�̂�
			z = dHelical - s + s/2.0f;
	CPointF		ij;
	CPoint3F	pt1(pCircle->GetMakePoint(b).x, pCircle->GetMakePoint(b).y, z),
				pt2(pCircle->GetMakePoint(a).x, pCircle->GetMakePoint(a).y, dHelical);
	if ( a > 1 )
		ij.y = pCircle->GetIJK(NCA_J);
	else
		ij.x = pCircle->GetIJK(NCA_I);
	CString	strGcode;
	CString	strBuf1( (*ms_pfnGetValString)(NCAI, ij.x, ms_bIJValue) + (*ms_pfnGetValString)(NCAJ, ij.y, ms_bIJValue) );
	ij *= -1.0f;	// ij = -ij;
	CString	strBuf2( (*ms_pfnGetValString)(NCAI, ij.x, ms_bIJValue) + (*ms_pfnGetValString)(NCAJ, ij.y, ms_bIJValue) );
	if ( !strBuf1.IsEmpty() && !strBuf2.IsEmpty() ) {
		// �v�Z�����̊֌W�łP�s�ɂł��Ȃ�
		strGcode  = MakeCircleSub_Helical(nCode, pt1) + strBuf1 + GetFeedString(dFeed) + ms_strEOB;
		strGcode += MakeCircleSub_Helical(nCode, pt2) + strBuf2 + ms_strEOB;
	}
	return strGcode;
}

// �~���ް��̐���
CString	CNCMakeBase::MakeArc_R(const CDXFarc* pArc, float dFeed)
{
	CString	strGcode,
			strBuf( MakeCircleSub_R(pArc->GetG(), pArc->GetEndMakePoint(), CPointF(), pArc->GetMakeR()) );
	if ( !strBuf.IsEmpty() )
		strGcode = (*ms_pfnGetLineNo)() + strBuf + GetFeedString(dFeed) + ms_strEOB;
	return strGcode;
}

CString	CNCMakeBase::MakeArc_IJ(const CDXFarc* pArc, float dFeed)
{
	CPointF	ij(pArc->GetIJK(NCA_I), pArc->GetIJK(NCA_J));
	CString	strGcode,
			strBuf( (*ms_pfnMakeCircleSub)(pArc->GetG(), pArc->GetEndMakePoint(), ij, 0.0f) );
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
