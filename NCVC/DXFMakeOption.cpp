// DXFMakeOption.cpp: CDXFMakeOption �N���X�̃C���v�������e�[�V����
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NCVC.h"
#include "DXFMakeOption.h"
#include "DXFOption.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

// int�^����
static	LPCTSTR	g_szNOrder[] = {
	"LTypeO", "LTypeC", "LTypeM", "LTypeH",
	"LColorO", "LColorC", "LColorM", "LColorH",
	"Plane", "Cycle"
};
static	const	int		g_dfNOrder[] = {
	1, 0, 2, 0,		// �j��, ����, �_��, ����
	4, 6, 2, 3,		// ��, ��, ��, ��
	0, 0			// XY����, �Œ軲�ى~�o��
};

// double�^����
static	LPCTSTR	g_szDOrder[] = {
	"OrgLength", "CycleR"
};
static	const	double	g_dfDOrder[] = {
	10.0, 10.0
};

// BOOL�^����
static	LPCTSTR	g_szBOrder[] = {
	"OutO", "OutC", "OutM", "OutH",
	"OrgCircle", "OrgCross"
};
static	const	BOOL	g_dfBOrder[] = {
	TRUE, TRUE, TRUE, TRUE,
	TRUE, TRUE
};

// CString�^����
static	LPCTSTR	g_szSOrder[] = {
	"LayerO", "LayerC", "LayerM", "LayerH"
};
extern	LPCTSTR	g_szDefaultLayer[];		// from DXFOption.cpp
//	"ORIGIN", "CAM",
//	"MOVE", "CORRECT"

/////////////////////////////////////////////////////////////////////////////
// CDXFMakeOption �N���X�̍\�z/����

CDXFMakeOption::CDXFMakeOption()
{
	ASSERT( SIZEOF(g_szNOrder) == SIZEOF(g_dfNOrder) );
	ASSERT( SIZEOF(g_szNOrder) == SIZEOF(m_unNums) );
	ASSERT( SIZEOF(g_szDOrder) == SIZEOF(g_dfDOrder) );
	ASSERT( SIZEOF(g_szDOrder) == SIZEOF(m_udNums) );
	ASSERT( SIZEOF(g_szBOrder) == SIZEOF(g_dfBOrder) );
	ASSERT( SIZEOF(g_szBOrder) == SIZEOF(m_ubFlags) );
	ASSERT( SIZEOF(g_szSOrder) == SIZEOF(m_strOption) );

	// ڼ޽�؂�����ǂݍ���
	int		i;
	CString	strRegKey(GetSubTreeRegKey(IDS_REGKEY_NC, IDS_REGKEY_MAKEDXF)),	// StdAfx.h
			strResult;

	// int�^���ߓǂݍ���
	for ( i=0; i<SIZEOF(g_szNOrder); i++ )
		m_unNums[i] = AfxGetApp()->GetProfileInt(strRegKey, g_szNOrder[i], g_dfNOrder[i]);
	// double�^���ߓǂݍ���
	for ( i=0; i<SIZEOF(g_szDOrder); i++ ) {
		strResult = AfxGetApp()->GetProfileString(strRegKey, g_szDOrder[i]);
		m_udNums[i] = strResult.IsEmpty() ? g_dfDOrder[i] : atof(strResult);
	}
	// BOOL�^���ߓǂݍ���
	for ( i=0; i<SIZEOF(g_szBOrder); i++ )
		m_ubFlags[i] = AfxGetApp()->GetProfileInt(strRegKey, g_szBOrder[i], g_dfBOrder[i]) ?
				TRUE : FALSE;
	// CString�^���ߓǂݍ���(DXFOption������̫�ď���������)
	const CDXFOption* pOpt = AfxGetNCVCApp()->GetDXFOption();
	CString	strLayer[SIZEOF(g_szSOrder)];
	strLayer[MKDX_STR_ORIGIN]	= pOpt->GetReadLayer(DXFORGLAYER);
	strLayer[MKDX_STR_CAMLINE]	= pOpt->GetReadLayer(DXFCAMLAYER);
	if ( strLayer[MKDX_STR_CAMLINE].FindOneOf(".|*?+(){}[]^$-\\") >= 0 ) {
		// ���K�\���̂��߂̓���ȋL�����܂܂��Ȃ�
		strLayer[MKDX_STR_CAMLINE].Empty();
	}
	strLayer[MKDX_STR_MOVE]		= pOpt->GetReadLayer(DXFMOVLAYER);
	for ( i=0; i<SIZEOF(g_szSOrder); i++ ) {
		if ( strLayer[i].IsEmpty() )
			strLayer[i] = g_szDefaultLayer[i];
		m_strOption[i] = AfxGetApp()->GetProfileString(strRegKey, g_szSOrder[i], strLayer[i]);
	}
}

/////////////////////////////////////////////////////////////////////////////
// ���ފ֐�

BOOL CDXFMakeOption::SaveDXFMakeOption(void)
{
	// ڼ޽�؂ւ̕ۑ�
	int		i;
	CString	strRegKey(GetSubTreeRegKey(IDS_REGKEY_NC, IDS_REGKEY_MAKEDXF)),
			strResult;

	// int�^���߂̕ۑ�
	for ( i=0; i<SIZEOF(g_szNOrder); i++ ) {
		if ( !AfxGetApp()->WriteProfileInt(strRegKey, g_szNOrder[i], m_unNums[i]) )
			return FALSE;
	}
	// double�^���߂̕ۑ�
	for ( i=0; i<SIZEOF(g_szDOrder); i++ ) {
		strResult.Format(IDS_MAKENCD_FORMAT, m_udNums[i]);
		if ( !AfxGetApp()->WriteProfileString(strRegKey, g_szDOrder[i], strResult) )
			return FALSE;
	}
	// BOOL�^���߂̕ۑ�
	for ( i=0; i<SIZEOF(g_szBOrder); i++ ) {
		if ( !AfxGetApp()->WriteProfileInt(strRegKey, g_szBOrder[i], m_ubFlags[i]) )
			return FALSE;
	}
	// CString�^���߂̕ۑ�
	for ( i=0; i<SIZEOF(g_szSOrder); i++ ) {
		if ( !AfxGetApp()->WriteProfileString(strRegKey, g_szSOrder[i], m_strOption[i]) )
			return FALSE;
	}

	return TRUE;
}
