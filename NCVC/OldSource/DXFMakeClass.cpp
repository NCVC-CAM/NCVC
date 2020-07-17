// DXFMakeClass.cpp: CDXFMake �N���X�̃C���v�������e�[�V����
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NCVC.h"
#include "NCdata.h"
#include "NCDoc.h"
#include "DXFMakeOption.h"
#include "DXFMakeClass.h"
#include "DXFkeyword.h"
#include "ViewOption.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

// from ReadDXF.cpp
// ��ٰ�ߺ���
extern	int		g_nGroupCode[];
	//		0, 1, 2, 3, 6, 8, 9, 70

// ����ݖ�
extern	LPCTSTR	g_szSection[];
	//	"SECTION", "ENDSEC", "EOF"

// ͯ�ް�ϐ���
extern	LPCTSTR	g_szHeader[];
	//	"$ACADVER", "$EXTMIN", "$EXTMAX", "$LIMMIN", "$LIMMAX"
// ð��ٻ�޷�
extern	LPCTSTR	g_szTables[];
	//	"TABLE", "ENDTAB",
	//		"LTYPE", "LAYER", "VPORT"

// ���s(StdAfx.cpp)
extern	LPCTSTR	gg_szReturn;	// "\n"
// DXF����(ViewOption.cpp)
extern	const	PENSTYLE	g_penStyle[];

// �悭�g���ϐ���Ăяo���̊ȗ��u��
#define	GetFlg		ms_pMakeOpt->GetFlag
#define	GetNum		ms_pMakeOpt->GetNum
#define	GetDbl		ms_pMakeOpt->GetDbl
#define	GetStr		ms_pMakeOpt->GetStr
//////////////////////////////
// �ÓI�ϐ��̏�����
const	CDXFMakeOption*	CDXFMake::ms_pMakeOpt = NULL;
PFNMAKEVALUE		CDXFMake::ms_pfnMakeValueLine = NULL;
PFNMAKEVALUE		CDXFMake::ms_pfnMakeValueCircle = NULL;
PFNMAKEVALUE		CDXFMake::ms_pfnMakeValueCircleToLine = NULL;
PFNMAKEVALUECYCLE	CDXFMake::ms_pfnMakeValueCycle = NULL;

//////////////////////////////////////////////////////////////////////
// �\�z/����
//////////////////////////////////////////////////////////////////////
CDXFMake::CDXFMake(enSECNAME nType, const CNCDoc* pDoc)
{
	m_strDXFarray.SetSize(0, 32);

	switch ( nType ) {
	case SEC_HEADER:
		MakeSection_Header(pDoc);
		break;
	case SEC_TABLES:
		MakeSection_Tables(pDoc);
		break;
	case SEC_BLOCKS:
		MakeSection_Blocks();
		break;
	case SEC_ENTITIES:
		MakeSection_Entities();
		break;
	case SEC_NOSECTION:
		MakeSection_EOF();
		break;
	}
}

CDXFMake::CDXFMake(const CNCdata* pData, BOOL bCorrect/*=FALSE*/)
{
	m_strDXFarray.SetSize(0, 32);

	// �e���ʂɑ΂���L����Gxx�������Ă΂��
	// ��޼ު�Ď��
	switch ( pData->GetType() ) {
	case NCDLINEDATA:	// �������
		MakeDXF_Line(static_cast<const CNCline*>(pData), bCorrect);
		break;
	case NCDARCDATA:	// �~�ʕ��
		if ( bCorrect || GetFlg(MKDX_FLG_OUT_C) )
			MakeDXF_Arc(static_cast<const CNCcircle*>(pData), bCorrect);
		break;
	case NCDCYCLEDATA:	// �Œ軲��
		MakeDXF_Cycle(static_cast<const CNCcycle*>(pData));
		break;
	}
}

CDXFMake::CDXFMake(const CPoint3D& pt)
{
	m_strDXFarray.SetSize(0, 16);
	double	dVal[DXFMAXVALUESIZE];

	// ���_(�~)���o��
	if ( GetFlg(MKDX_FLG_ORGCIRCLE) ) {
		// ��޼ު�ď��
		m_strDXFarray.Add( MakeDxfInfo(TYPE_CIRCLE, MKDX_STR_ORIGIN) );
		// ���W�l
		switch ( GetNum(MKDX_NUM_PLANE) ) {
		case 1:		// XZ
			dVal[VALUE10] = pt.x;	dVal[VALUE20] = pt.z;
			break;
		case 2:		// YZ
			dVal[VALUE10] = pt.y;	dVal[VALUE20] = pt.z;
			break;
		default:	// XY
			dVal[VALUE10] = pt.x;	dVal[VALUE20] = pt.y;
			break;
		}
		dVal[VALUE40] = GetDbl(MKDX_DBL_ORGLENGTH);
		m_strDXFarray.Add( MakeValue(VALFLG_CIRCLE, dVal) );
	}

	// ���_(�۽)���o��
	if ( GetFlg(MKDX_FLG_ORGCROSS) ) {
		double	d1 = GetDbl(MKDX_DBL_ORGLENGTH), d2 = 0.0;
		for ( int i=0; i<2; i++ ) {
			// ��޼ު�ď��
			m_strDXFarray.Add( MakeDxfInfo(TYPE_LINE, MKDX_STR_ORIGIN) );
			// ���W�l
			switch ( GetNum(MKDX_NUM_PLANE) ) {
			case 1:		// XZ
				dVal[VALUE10] = pt.x - d1;	dVal[VALUE11] = pt.x + d1;
				dVal[VALUE20] = pt.z - d2;	dVal[VALUE21] = pt.z + d2;
				break;
			case 2:		// YZ
				dVal[VALUE10] = pt.y - d1;	dVal[VALUE11] = pt.y + d1;
				dVal[VALUE20] = pt.z - d2;	dVal[VALUE21] = pt.z + d2;
				break;
			default:	// XY
				dVal[VALUE10] = pt.x - d1;	dVal[VALUE11] = pt.x + d1;
				dVal[VALUE20] = pt.y - d2;	dVal[VALUE21] = pt.y + d2;
				break;
			}
			m_strDXFarray.Add( MakeValue(VALFLG_LINE, dVal) );
			// �n�_�I�_�������W����ւ�
			d1 = 0.0;	d2 = GetDbl(MKDX_DBL_ORGLENGTH);
		}
	}
}

//////////////////////////////////////////////////////////////////////
// ���ފ֐�

void CDXFMake::MakeSection_Header(const CNCDoc* pDoc)
{
	// ����ݒ�`
	m_strDXFarray.Add(MakeSection(SEC_HEADER));
	// HEADER����ݒl
	double	dMin[DXFMAXVALUESIZE], dMax[DXFMAXVALUESIZE];
	CRect3D	rc( pDoc->GetMaxRect() );
	switch ( GetNum(MKDX_NUM_PLANE) ) {
	case 1:		// XZ
		dMin[VALUE10] = rc.left;	dMin[VALUE20] = rc.low;
		dMax[VALUE10] = rc.right;	dMax[VALUE20] = rc.high;
		break;
	case 2:		// YZ
		dMin[VALUE10] = rc.top;		dMin[VALUE20] = rc.low;
		dMax[VALUE10] = rc.bottom;	dMax[VALUE20] = rc.high;
		break;
	default:	// XY
		dMin[VALUE10] = rc.left;	dMin[VALUE20] = rc.top;
		dMax[VALUE10] = rc.right;	dMax[VALUE20] = rc.bottom;
		break;
	}
	m_strDXFarray.Add(
		GROUPCODE(g_nGroupCode[GROUP9])+g_szHeader[HEAD_ACADVER]+gg_szReturn+
		GROUPCODE(g_nGroupCode[GROUP1])+"AC1009\n"+
		GROUPCODE(g_nGroupCode[GROUP9])+g_szHeader[HEAD_EXTMIN]+gg_szReturn+
			MakeValue(VALFLG_START, dMin)+
		GROUPCODE(g_nGroupCode[GROUP9])+g_szHeader[HEAD_EXTMAX]+gg_szReturn+
			MakeValue(VALFLG_START, dMax)+
		GROUPCODE(g_nGroupCode[GROUP9])+g_szHeader[HEAD_LIMMIN]+gg_szReturn+
			MakeValue(VALFLG_START, dMin)+
		GROUPCODE(g_nGroupCode[GROUP9])+g_szHeader[HEAD_LIMMAX]+gg_szReturn+
			MakeValue(VALFLG_START, dMax)
	);
	// ����ݏI��
	m_strDXFarray.Add(MakeEndSec());
}

void CDXFMake::MakeSection_Tables(const CNCDoc* pDoc)
{
	int		i, j, nLayer = 1;
	CRect3D	rc( pDoc->GetMaxRect() );
	CPointD	pt;

	// ����ݒ�`
	m_strDXFarray.Add(MakeSection(SEC_TABLES));
	// TABLES����ݒl
	double	dVal[DXFMAXVALUESIZE], d1, d2;
	CString	strGroup0(GROUPCODE(g_nGroupCode[GROUP0])),
			strGroup2(GROUPCODE(g_nGroupCode[GROUP2])),
			strGroup3(GROUPCODE(g_nGroupCode[GROUP3])),
			strGroup70(MakeValue(70, 64)),
			strGroup72(MakeValue(72, 65));
	CString	strTABLE(strGroup0+g_szTables[TABLES_TABLE]+gg_szReturn),
			strENDTAB(strGroup0+g_szTables[TABLES_ENDTAB]+gg_szReturn),
			strLTYPE(g_szTables[TABLEKEY_LTYPE]),
			strLAYER(g_szTables[TABLEKEY_LAYER]),
			strVPORT(g_szTables[TABLEKEY_VPORT]);
	strLTYPE += gg_szReturn;	strLAYER += gg_szReturn;
	strVPORT += gg_szReturn;

	// VPORT��ʕ\���ر�ݒ�
	m_strDXFarray.Add(	
		strTABLE+strGroup2+strVPORT+MakeValue(70, 1)+
		strGroup0+strVPORT+strGroup2+"*ACTIVE\n"+MakeValue(70, 0)
	);
	dVal[VALUE10] = 0.0;	dVal[VALUE20] = 0.0;
	dVal[VALUE11] = 1.0;	dVal[VALUE21] = 1.0;
	m_strDXFarray.Add( MakeValue(VALFLG_LINE, dVal) );
	switch ( GetNum(MKDX_NUM_PLANE) ) {
	case 1:		// XZ
		dVal[VALUE10] = (rc.right + rc.left) / 2.0;		// VALUE10�͉�����v�f
		dVal[VALUE20] = (rc.high  + rc.low)  / 2.0;
		d1 = rc.right - rc.left;
		d2 = rc.high  - rc.low;
		dVal[VALUE40] = max( d1, d2 ) * 1.1;
		break;
	case 2:		// YZ
		dVal[VALUE10] = (rc.bottom + rc.top) / 2.0;
		dVal[VALUE20] = (rc.high   + rc.low) / 2.0;
		d1 = rc.bottom - rc.top;
		d2 = rc.high   - rc.low;
		dVal[VALUE40] = max( d1, d2 ) * 1.1;
		break;
	default:	// XY
		pt = rc.CenterPoint();
		dVal[VALUE10] = pt.x;		dVal[VALUE20] = pt.y;
		d1 = rc.Width();
		d2 = rc.Height();
		dVal[VALUE40] = max( d1, d2 ) * 1.1;
		break;
	}
	dVal[VALUE41] = 1.5376;		// 1280x1024�c����
	dVal[VALUE42] = 50.0;
	m_strDXFarray.Add(
		MakeValue(12, dVal[VALUE10]) +
		MakeValue(22, dVal[VALUE20]) +
		MakeValue(13,  0.0) + MakeValue(23,  0.0) +
		MakeValue(14, 10.0) + MakeValue(24, 10.0) +
		MakeValue(15, 10.0) + MakeValue(25, 10.0) +
		MakeValue(16,  0.0) + MakeValue(26,  0.0) + MakeValue(36,  1.0) +
		MakeValue(17,  0.0) + MakeValue(27,  0.0) + MakeValue(37,  1.0) +
		MakeValue(VALFLG40|VALFLG41|VALFLG42, dVal) +
		MakeValue(43,  0.0) + MakeValue(44,  0.0) +
		MakeValue(50,  0.0) + MakeValue(51,  0.0)
	);
	m_strDXFarray.Add(
		MakeValue(71, 0) + MakeValue(72, 1000) +
		MakeValue(73, 1) + MakeValue(74, 3) +
		MakeValue(75, 0) + MakeValue(76, 0) +
		MakeValue(77, 0) + MakeValue(78, 0)
	);
	m_strDXFarray.Add(strENDTAB);

	// ����ð��ِ�
	m_strDXFarray.Add(	
		strTABLE+strGroup2+strLTYPE+MakeValue(70, MAXPENSTYLE)
	);
	for ( i=0; i<MAXPENSTYLE; i++ ) {
		dVal[VALUE40] = g_penStyle[i].dDXFpattern;
		m_strDXFarray.Add(
			strGroup0+strLTYPE+strGroup2+g_penStyle[i].lpszDXFname+gg_szReturn+	// ���햼
			strGroup70+strGroup3+g_penStyle[i].lpszDXFpattern+gg_szReturn+		// �����
			strGroup72+MakeValue(73, g_penStyle[i].nDXFdash)+					// �ޯ�����ڐ�
			MakeValue(VALFLG40, dVal)											// ����ݒ���
		);
		for ( j=0; j<g_penStyle[i].nDXFdash; j++ )
			m_strDXFarray.Add( MakeValue(49, g_penStyle[i].dDXFdash[j]) );
	}
	m_strDXFarray.Add(strENDTAB);

	// TABLES����ݒl(ڲԏ��)
	CString	strLayer[4];	// SIZEOF(CDXFMakeOption::m_strOption)
	// ������ڲԖ��͓���
	strLayer[0] = GetStr(MKDX_STR_ORIGIN);
	for ( i=1; i<SIZEOF(strLayer); i++ ) {
		for ( j=0; j<nLayer; j++ ) {
			if ( strLayer[j].CompareNoCase(GetStr(i+MKDX_STR_ORIGIN)) == 0 )
				break;	// ������ڲԖ�������Β��f
		}
		if ( j >= nLayer )
			strLayer[nLayer++] = GetStr(i+MKDX_STR_ORIGIN);
	}
	// ڲԏ��o�^(ڲԏ��̐���͎���,���ŌŒ�)
	m_strDXFarray.Add(strTABLE+strGroup2+strLAYER+MakeValue(70, nLayer));
	CString	strLType( GROUPCODE(g_nGroupCode[GROUP6])+
				g_penStyle[LTYPE_CONTINUOUS].lpszDXFname+gg_szReturn ),
			strLCol( strGroup70+MakeValue(62, 7) );
	// ��ڲԂ�"0"�u���Ƒ啶���ϊ�
	for ( i=0; i<nLayer; i++ ) {
		if ( strLayer[i].IsEmpty() )
			strLayer[i] = "0";
		else
			strLayer[i].MakeUpper();
		m_strDXFarray.Add(strGroup0+strLAYER+strGroup2+strLayer[i]+gg_szReturn+strLCol+strLType);
	}
	m_strDXFarray.Add(strENDTAB);

	// ����ݏI��
	m_strDXFarray.Add(MakeEndSec());
}

void CDXFMake::MakeSection_Blocks(void)
{
	// ����ݒ�`
	m_strDXFarray.Add(MakeSection(SEC_BLOCKS));
	// ����ݏI��
	m_strDXFarray.Add(MakeEndSec());
}

void CDXFMake::MakeSection_Entities(void)
{
	// ����ݒ�`
	m_strDXFarray.Add(MakeSection(SEC_ENTITIES));
	// ����ݏI�����ް��o�͌�Ȃ̂ŏȗ�
}

void CDXFMake::MakeSection_EOF(void)
{
	// Entities����ݏI����
	m_strDXFarray.Add(MakeEndSec());
	// EOF
	m_strDXFarray.Add(GROUPCODE(g_nGroupCode[GROUP0])+g_szSection[SEC_EOF]+gg_szReturn);
}

//////////////////////////////////////////////////////////////////////

void CDXFMake::MakeDXF_Line(const CNCline* pData, BOOL bCorrect)
{
	CString	strResult;

	// ��޼ު�ď��(������ or �؍푗��)
	if ( bCorrect )
		strResult = MakeDxfInfo(TYPE_LINE, MKDX_STR_CORRECT);
	else {
		if ( pData->GetGcode() == 0 ) {
			if ( GetFlg(MKDX_FLG_OUT_M) )
				strResult = MakeDxfInfo(TYPE_LINE, MKDX_STR_MOVE);
		}
		else {
			if ( GetFlg(MKDX_FLG_OUT_C) )
				strResult = MakeDxfInfo(TYPE_LINE, MKDX_STR_CAMLINE);
		}
	}

	if ( !strResult.IsEmpty() ) {
		m_strDXFarray.Add(strResult);
		// ���W�l
		m_strDXFarray.Add( (*ms_pfnMakeValueLine)(pData) );
	}
}

static inline int _SetDXFtype(ENPLANE enPlane, const CNCdata* pData)
{
	int		nType;
	if ( pData->GetPlane() != enPlane )
		nType = TYPE_LINE;
	else
		nType = pData->GetValFlags() & (NCD_X|NCD_Y|NCD_Z) ? TYPE_ARC : TYPE_CIRCLE;
	return nType;
}

void CDXFMake::MakeDXF_Arc(const CNCcircle* pData, BOOL bCorrect)
{
	int		nType;

	// ��޼ު�Ď��
	switch ( GetNum(MKDX_NUM_PLANE) ) {
	case 1:		// XZ
		nType = _SetDXFtype(XZ_PLANE, pData);	// ��
		break;
	case 2:		// YZ
		nType = _SetDXFtype(YZ_PLANE, pData);
		break;
	default:	// XY
		nType = _SetDXFtype(XY_PLANE, pData);
		break;
	}
	// ��޼ު�ď��
	m_strDXFarray.Add( MakeDxfInfo(nType,
		bCorrect ? MKDX_STR_CORRECT : MKDX_STR_CAMLINE) );
	// ���W�l
	m_strDXFarray.Add( nType == TYPE_LINE ? 
		(*ms_pfnMakeValueCircleToLine)(pData) : (*ms_pfnMakeValueCircle)(pData) );
	// �~�ʂ̂݊p�x�̒ǉ�
	if ( nType == TYPE_ARC ) {
		double	dVal[DXFMAXVALUESIZE];
		// CNCcircle::AngleTuning() �ɂď�ɔ����v���
		dVal[VALUE50] = DEG(pData->GetStartAngle());
		dVal[VALUE51] = DEG(pData->GetEndAngle());
		while ( dVal[VALUE50] > 360.0 )
			dVal[VALUE50] -= 360.0;
		while ( dVal[VALUE51] > 360.0 )
			dVal[VALUE51] -= 360.0;
		m_strDXFarray.Add( MakeValue(VALFLG50|VALFLG51, dVal) );
	}
}

void CDXFMake::MakeDXF_Cycle(const CNCcycle* pData)
{
	if ( pData->GetDrawCnt() > 0 ) {
		for ( int i=0; i<pData->GetDrawCnt(); i++ )
			m_strDXFarray.Add( (*ms_pfnMakeValueCycle)(pData, i) );
	}
	else if ( pData->GetStartPoint()!=pData->GetEndPoint() && GetFlg(MKDX_FLG_OUT_M) ) {
		// L0 �ł��ړ�������ꍇ
		m_strDXFarray.Add( MakeDxfInfo(TYPE_LINE, MKDX_STR_MOVE) );
		// CNCline �Ƃ��ď���
		m_strDXFarray.Add( (*ms_pfnMakeValueLine)(pData) );
	}
}

//////////////////////////////////////////////////////////////////////

CString CDXFMake::MakeValueLine_XY(const CNCdata* pData)
{
	double	dVal[DXFMAXVALUESIZE];
	CPoint3D	pts(pData->GetStartPoint()), pte(pData->GetEndPoint());
	dVal[VALUE10] = pts.x;	dVal[VALUE20] = pts.y;
	dVal[VALUE11] = pte.x;	dVal[VALUE21] = pte.y;
	return MakeValue(VALFLG_LINE, dVal);
}

CString CDXFMake::MakeValueLine_XZ(const CNCdata* pData)
{
	double	dVal[DXFMAXVALUESIZE];
	CPoint3D	pts(pData->GetStartPoint()), pte(pData->GetEndPoint());
	dVal[VALUE10] = pts.x;	dVal[VALUE20] = pts.z;
	dVal[VALUE11] = pte.x;	dVal[VALUE21] = pte.z;
	return MakeValue(VALFLG_LINE, dVal);
}

CString	CDXFMake::MakeValueLine_YZ(const CNCdata* pData)
{
	double	dVal[DXFMAXVALUESIZE];
	CPoint3D	pts(pData->GetStartPoint()), pte(pData->GetEndPoint());
	dVal[VALUE10] = pts.y;	dVal[VALUE20] = pts.z;
	dVal[VALUE11] = pte.y;	dVal[VALUE21] = pte.z;
	return MakeValue(VALFLG_LINE, dVal);
}

CString CDXFMake::MakeValueCircle_XY(const CNCdata* pData)
{
	double	dVal[DXFMAXVALUESIZE];
	CPoint3D	ptOrg( static_cast<const CNCcircle *>(pData)->GetOrg() );
	dVal[VALUE10] = ptOrg.x;	dVal[VALUE20] = ptOrg.y;
	dVal[VALUE40] = fabs( static_cast<const CNCcircle *>(pData)->GetR() );
	return MakeValue(VALFLG_CIRCLE, dVal);
}

CString CDXFMake::MakeValueCircle_XZ(const CNCdata* pData)
{
	double	dVal[DXFMAXVALUESIZE];
	CPoint3D	ptOrg( static_cast<const CNCcircle *>(pData)->GetOrg() );
	dVal[VALUE10] = ptOrg.x;	dVal[VALUE20] = ptOrg.z;
	dVal[VALUE40] = fabs( static_cast<const CNCcircle *>(pData)->GetR() );
	return MakeValue(VALFLG_CIRCLE, dVal);
}

CString CDXFMake::MakeValueCircle_YZ(const CNCdata* pData)
{
	double	dVal[DXFMAXVALUESIZE];
	CPoint3D	ptOrg( static_cast<const CNCcircle *>(pData)->GetOrg() );
	dVal[VALUE10] = ptOrg.y;	dVal[VALUE20] = ptOrg.z;
	dVal[VALUE40] = fabs( static_cast<const CNCcircle *>(pData)->GetR() );
	return MakeValue(VALFLG_CIRCLE, dVal);
}

CString CDXFMake::MakeValueCircleToLine_XY(const CNCdata* pData)
{
	double	dVal[DXFMAXVALUESIZE];
	CRect3D	rc( pData->GetMaxRect() );
	dVal[VALUE10] = rc.left;	dVal[VALUE20] = rc.top;
	dVal[VALUE11] = rc.right;	dVal[VALUE21] = rc.bottom;
	return MakeValue(VALFLG_LINE, dVal);
}

CString	CDXFMake::MakeValueCircleToLine_XZ(const CNCdata* pData)
{
	double	dVal[DXFMAXVALUESIZE];
	CRect3D	rc( pData->GetMaxRect() );
	dVal[VALUE10] = rc.left;	dVal[VALUE20] = rc.low;
	dVal[VALUE11] = rc.right;	dVal[VALUE21] = rc.high;
	return MakeValue(VALFLG_LINE, dVal);
}

CString	CDXFMake::MakeValueCircleToLine_YZ(const CNCdata* pData)
{
	double	dVal[DXFMAXVALUESIZE];
	CRect3D	rc( pData->GetMaxRect() );
	dVal[VALUE10] = rc.top;		dVal[VALUE20] = rc.low;
	dVal[VALUE11] = rc.bottom;	dVal[VALUE21] = rc.high;
	return MakeValue(VALFLG_LINE, dVal);
}

CString CDXFMake::MakeValueCycle_XY_Circle(const CNCcycle* pData, int nIndex)
{
	return MakeValueCycle(pData, nIndex, TYPE_CIRCLE, XY_PLANE);
}

CString CDXFMake::MakeValueCycle_XZ_Circle(const CNCcycle* pData, int nIndex)
{
	return MakeValueCycle(pData, nIndex, TYPE_CIRCLE, XZ_PLANE);
}

CString CDXFMake::MakeValueCycle_YZ_Circle(const CNCcycle* pData, int nIndex)
{
	return MakeValueCycle(pData, nIndex, TYPE_CIRCLE, YZ_PLANE);
}

CString CDXFMake::MakeValueCycle_XY_Point(const CNCcycle* pData, int nIndex)
{
	return MakeValueCycle(pData, nIndex, TYPE_POINT, XY_PLANE);
}

CString CDXFMake::MakeValueCycle_XZ_Point(const CNCcycle* pData, int nIndex)
{
	return MakeValueCycle(pData, nIndex, TYPE_POINT, XZ_PLANE);
}

CString CDXFMake::MakeValueCycle_YZ_Point(const CNCcycle* pData, int nIndex)
{
	return MakeValueCycle(pData, nIndex, TYPE_POINT, YZ_PLANE);
}

CString CDXFMake::MakeValueCycle
	(const CNCcycle* pData, int nIndex, int nType, ENPLANE enPlane)
{
	const	PTCYCLE*	pCycleInside = pData->GetCycleInside(enPlane+1);
	int		nStart = nIndex - 1;
	double	dVal[DXFMAXVALUESIZE];
	CPointD	pts, pti, ptr;
	CString	strResult;
	if ( GetFlg(MKDX_FLG_OUT_M) )
		strResult = MakeDxfInfo(TYPE_LINE, MKDX_STR_MOVE);

	// �Ăяo���֐��̓��I����
	CPointD	(CPoint3D::*pfnGetAxis)(void) const;
	switch ( enPlane ) {
	case XY_PLANE:
		pfnGetAxis = &(CPoint3D::GetXY);
		break;
	case XZ_PLANE:
		pfnGetAxis = &(CPoint3D::GetXZ);
		break;
	case YZ_PLANE:
		pfnGetAxis = &(CPoint3D::GetYZ);
		break;
	}
	
	// ��޼ު�Ă̕��ʂƎw�����ʂ����������ǂ���
	if ( pData->GetPlane() == enPlane ) {
		if ( GetFlg(MKDX_FLG_OUT_M) ) {
			// �ړ��ް�
			if ( nStart < 0 ) {
				// ���݈ʒu����1�_�ڂ̲Ƽ�ٓ_
				pts = (pData->GetStartPoint().*pfnGetAxis)();
				dVal[VALUE10] = pts.x;
				dVal[VALUE20] = pts.y;
			}
			else {
				dVal[VALUE10] = pCycleInside[nStart].ptI.x;
				dVal[VALUE20] = pCycleInside[nStart].ptI.y;
			}
			dVal[VALUE11] = pCycleInside[nIndex].ptI.x;
			dVal[VALUE21] = pCycleInside[nIndex].ptI.y;
			strResult += MakeValue(VALFLG_LINE, dVal);
		}
		if ( GetFlg(MKDX_FLG_OUT_C) ) {
			// �؍��ް�(�~�܂��͎��_)
			strResult += MakeDxfInfo(nType, MKDX_STR_CAMLINE);
			dVal[VALUE10] = pCycleInside[nIndex].ptI.x;
			dVal[VALUE20] = pCycleInside[nIndex].ptI.y;
			DWORD dwFlags = VALFLG_START;
			if ( nType == TYPE_CIRCLE ) {
				dVal[VALUE40] = GetDbl(MKDX_DBL_CYCLER);
				dwFlags |= VALFLG40;
			}
			strResult += MakeValue(dwFlags, dVal);
		}
	}
	else {
		if ( GetFlg(MKDX_FLG_OUT_M) ) {
			if ( nStart < 0 ) {
				pts = (pData->GetStartPoint().*pfnGetAxis)();
				pti = (pData->GetIPoint().*pfnGetAxis)();
				ptr = (pData->GetRPoint().*pfnGetAxis)();
				dVal[VALUE10] = pts.x;
				dVal[VALUE20] = pts.y;
				dVal[VALUE11] = pti.x;
				dVal[VALUE21] = pti.y;
				strResult += MakeValue(VALFLG_LINE, dVal);
				if ( pti != ptr ) {
					// �Ƽ�ٓ_����R�_
					strResult += MakeDxfInfo(TYPE_LINE, MKDX_STR_MOVE);
					dVal[VALUE10] = pti.x;
					dVal[VALUE20] = pti.y;
					dVal[VALUE11] = ptr.x;
					dVal[VALUE21] = ptr.y;
					strResult += MakeValue(VALFLG_LINE, dVal);
				}
			}
			else {
				pts = pCycleInside[nStart].ptI;
				pti = pCycleInside[nIndex].ptI;
				ptr = pCycleInside[nIndex].ptR;
				dVal[VALUE10] = pts.x;
				dVal[VALUE20] = pts.y;
				dVal[VALUE11] = pti.x;
				dVal[VALUE21] = pti.y;
				strResult += MakeValue(VALFLG_LINE, dVal);
				if ( pti != ptr ) {
					strResult += MakeDxfInfo(TYPE_LINE, MKDX_STR_MOVE);
					dVal[VALUE10] = pti.x;
					dVal[VALUE20] = pti.y;
					dVal[VALUE11] = ptr.x;
					dVal[VALUE21] = ptr.y;
					strResult += MakeValue(VALFLG_LINE, dVal);
				}
			}
		}
		if ( GetFlg(MKDX_FLG_OUT_C) ) {
			// �؍��ް�
			strResult += MakeDxfInfo(TYPE_LINE, MKDX_STR_CAMLINE);
			dVal[VALUE10] = pCycleInside[nIndex].ptR.x;
			dVal[VALUE20] = pCycleInside[nIndex].ptR.y;
			dVal[VALUE11] = pCycleInside[nIndex].ptC.x;
			dVal[VALUE21] = pCycleInside[nIndex].ptC.y;
			strResult += MakeValue(VALFLG_LINE, dVal);
		}
	}

	return strResult;
}

//////////////////////////////////////////////////////////////////////

void CDXFMake::SetStaticOption(const CDXFMakeOption* pDXFMake)
{
	ms_pMakeOpt = pDXFMake;
	switch ( GetNum(MKDX_NUM_PLANE) ) {
	case 1:		// XZ
		ms_pfnMakeValueLine			= &MakeValueLine_XZ;
		ms_pfnMakeValueCircle		= &MakeValueCircle_XZ;
		ms_pfnMakeValueCircleToLine	= &MakeValueCircleToLine_XZ;
		ms_pfnMakeValueCycle = GetNum(MKDX_NUM_CYCLE) == 0 ?
			&MakeValueCycle_XZ_Circle : MakeValueCycle_XZ_Point;
		break;
	case 2:		// YZ
		ms_pfnMakeValueLine			= &MakeValueLine_YZ;
		ms_pfnMakeValueCircle		= &MakeValueCircle_YZ;
		ms_pfnMakeValueCircleToLine	= &MakeValueCircleToLine_YZ;
		ms_pfnMakeValueCycle = GetNum(MKDX_NUM_CYCLE) == 0 ?
			&MakeValueCycle_YZ_Circle : MakeValueCycle_YZ_Point;
		break;
	default:	// XY
		ms_pfnMakeValueLine			= &MakeValueLine_XY;
		ms_pfnMakeValueCircle		= &MakeValueCircle_XY;
		ms_pfnMakeValueCircleToLine	= &MakeValueCircleToLine_XY;
		ms_pfnMakeValueCycle = GetNum(MKDX_NUM_CYCLE) == 0 ?
			&MakeValueCycle_XY_Circle : MakeValueCycle_XY_Point;
		break;
	}
}
