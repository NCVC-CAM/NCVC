// NCMakeWire.cpp: CNCMakeWire �N���X�̃C���v�������e�[�V����
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NCVC.h"
#include "DXFdata.h"
#include "NCMakeWireOpt.h"
#include "NCMakeWire.h"

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
// CNCMakeWire �\�z/����
//////////////////////////////////////////////////////////////////////

CNCMakeWire::CNCMakeWire(const CDXFdata* pData, double dFeed) : CNCMakeMill(pData, dFeed)
{
}

// XY��G[0|1]�ړ� -> �v CNCMakeMill ��̫�ĺݽ�׸�
CNCMakeWire::CNCMakeWire(int nCode, const CPointD& pt, double dFeed, double dTaper)
{
	// ð�ߎw������
	CString	strTaper;
	if ( dTaper != 0.0 ) {
		if ( nCode == 0 )
			strTaper = GetGString(50) + "T0";
		else {
			if ( dTaper > 0 )
				strTaper = GetGString(51) + "T" + (*ms_pfnGetValDetail)(dTaper);
			else
				strTaper = GetGString(52) + "T" + (*ms_pfnGetValDetail)(fabs(dTaper));
		}
	}
	// �ړ�����
	CString	strGcode(
		GetValString(NCA_X, pt.x, FALSE) +
		GetValString(NCA_Y, pt.y, FALSE) );
	if ( !strGcode.IsEmpty() ) {
		if ( nCode != 0 )	// G0�ȊO
			strGcode += GetFeedString(dFeed);
	}
	// �ŏI���`
	if ( !strTaper.IsEmpty() || !strGcode.IsEmpty() ) {
		m_strGcode = (*ms_pfnGetLineNo)() + strTaper;
		if ( !strGcode.IsEmpty() )
			m_strGcode += (*ms_pfnGetGString)(nCode) + strGcode;
		m_strGcode += ms_strEOB;
	}
}

// �C�ӂ̕�������
CNCMakeWire::CNCMakeWire(const CString& strGcode) : CNCMakeMill(strGcode)
{
}

//////////////////////////////////////////////////////////////////////
// ���ފ֐�

void CNCMakeWire::SetStaticOption(const CNCMakeWireOpt* pNCMake)
{
	extern	LPCTSTR	gg_szReturn;
	static	const	int		nLineMulti[] = {	// �s�ԍ�������
		1, 5, 10, 100
	};

	CNCMakeBase::SetStaticOption(pNCMake);

	// --- ���w��
	NCAX = NCA_X;	NCAY = NCA_Y;
	NCAI = NCA_I;	NCAJ = NCA_J;
	// --- ����w��
	switch ( GetNum(MKWI_NUM_FDOT) ) {
	case 0:		// �����_�\�L
		ms_pfnGetFeed = GetFlg(MKWI_FLG_ZEROCUT) ?
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
	ms_pfnGetLineNo = GetFlg(MKWI_FLG_LINEADD) && !(GetStr(MKWI_STR_LINEFORM).IsEmpty()) ?
		&GetLineNoString : &GetLineNoString_Clip;
	// --- G����Ӱ���
	ms_pfnGetGString =  GetFlg(MKWI_FLG_GCLIP) ? &GetGString_Clip : &GetGString;
	// --- ���W�\�L
//	ms_pfnGetValString = &GetValString;	// �ް��׽����̌ďo�p
	ms_pfnGetValDetail = GetNum(MKWI_NUM_DOT) == 0 ?
		(GetFlg(MKWI_FLG_ZEROCUT) ?
			&GetValString_UZeroCut : &GetValString_Normal) : &GetValString_Multi1000;
	// --- �s�ԍ�������
	ms_nMagni = GetNum(MKWI_NUM_LINEADD)<0 ||
					 GetNum(MKWI_NUM_LINEADD)>=SIZEOF(nLineMulti) ?
		nLineMulti[0] : nLineMulti[GetNum(MKWI_NUM_LINEADD)];
	// --- EOB
	ms_strEOB = GetStr(MKWI_STR_EOB).IsEmpty() ? 
		gg_szReturn : GetStr(MKWI_STR_EOB) + gg_szReturn;
	// --- �~�ް��̐؍�w��
	ms_nCircleCode = GetNum(MKWI_NUM_CIRCLECODE) == 0 ? 2 : 3;
	// --- �~,�~���ް��̐���
	ms_pfnMakeCircle	= &MakeCircle_IJ;
	ms_pfnMakeCircleSub	= &MakeCircleSub_IJ;
	ms_pfnMakeHelical	= &MakeCircle_IJ_Helical;	// �ی�(ܲԉ��H�ɂ͖���)
	ms_pfnMakeArc		= &MakeArc_IJ;
	// --- �ȉ~����
	ms_dEllipse = GetDbl(MKWI_DBL_ELLIPSE);
}
