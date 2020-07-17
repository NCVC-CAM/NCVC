// DXFMakeOption.cpp: CDXFMakeOption ÉNÉâÉXÇÃÉCÉìÉvÉäÉÅÉìÉeÅ[ÉVÉáÉì
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

using namespace boost;

// intå^ñΩóﬂ
static	LPCTSTR	g_szNOrder[] = {
	"LTypeO", "LTypeC", "LTypeM", "LTypeH",
	"LColorO", "LColorC", "LColorM", "LColorH",
	"Plane", "Cycle"
};
static	const	int		g_dfNOrder[] = {
	1, 0, 2, 0,		// îjê¸, é¿ê¸, ì_ê¸, é¿ê¸
	4, 6, 2, 3,		// ê¬, îí, óŒ, êÖ
	0, 0			// XYïΩñ , å≈íËª≤∏Ÿâ~èoóÕ
};

// floatå^ñΩóﬂ
static	LPCTSTR	g_szDOrder[] = {
	"OrgLength", "CycleR"
};
static	const	float	g_dfDOrder[] = {
	10.0, 10.0
};

// BOOLå^ñΩóﬂ
static	LPCTSTR	g_szBOrder[] = {
	"OutO", "OutC", "OutM", "OutH",
	"OrgCircle", "OrgCross"
};
static	const	BOOL	g_dfBOrder[] = {
	TRUE, TRUE, TRUE, TRUE,
	TRUE, TRUE
};

// CStringå^ñΩóﬂ
static	LPCTSTR	g_szSOrder[] = {
	"LayerO", "LayerC", "LayerM", "LayerH"
};
extern	LPCTSTR	g_szDefaultLayer[];		// from DXFOption.cpp
//	"ORIGIN", "CAM",
//	"MOVE", "CORRECT"

/////////////////////////////////////////////////////////////////////////////
// CDXFMakeOption ÉNÉâÉXÇÃç\íz/è¡ñ≈

CDXFMakeOption::CDXFMakeOption(BOOL bRegist)
{
	ASSERT( MKDX_NUM_NUMS == SIZEOF(g_szNOrder) );
	ASSERT( MKDX_NUM_NUMS == SIZEOF(g_dfNOrder) );
	ASSERT( MKDX_DBL_NUMS == SIZEOF(g_szDOrder) );
	ASSERT( MKDX_DBL_NUMS == SIZEOF(g_dfDOrder) );
	ASSERT( MKDX_FLG_NUMS == SIZEOF(g_szBOrder) );
	ASSERT( MKDX_FLG_NUMS == SIZEOF(g_dfBOrder) );
	ASSERT( MKDX_STR_NUMS == SIZEOF(g_szSOrder) );

	if ( bRegist )
		Initialize_Registry();
	else
		Initialize_Default();
}

void CDXFMakeOption::Initialize_Registry(void)
{
	// ⁄ºﬁΩƒÿÇ©ÇÁèÓïÒì«Ç›çûÇ›
	int		i;
	CString	strRegKey(GetSubTreeRegKey(IDS_REGKEY_NC, IDS_REGKEY_MAKEDXF)),	// StdAfx.h
			strResult;

	// intå^ñΩóﬂì«Ç›çûÇ›
	for ( i=0; i<SIZEOF(g_szNOrder); i++ )
		m_unNums[i] = AfxGetApp()->GetProfileInt(strRegKey, g_szNOrder[i], g_dfNOrder[i]);
	// floatå^ñΩóﬂì«Ç›çûÇ›
	for ( i=0; i<SIZEOF(g_szDOrder); i++ ) {
		strResult = AfxGetApp()->GetProfileString(strRegKey, g_szDOrder[i]);
		m_udNums[i] = strResult.IsEmpty() ? g_dfDOrder[i] : (float)atof((LPCTSTR)strResult.Trim());
	}
	// BOOLå^ñΩóﬂì«Ç›çûÇ›
	for ( i=0; i<SIZEOF(g_szBOrder); i++ )
		m_ubFlags[i] = AfxGetApp()->GetProfileInt(strRegKey, g_szBOrder[i], g_dfBOrder[i]) ?
				TRUE : FALSE;
	// CStringå^ñΩóﬂì«Ç›çûÇ›(DXFOptionÇ©ÇÁ√ﬁÃ´ŸƒèÓïÒÇèâä˙âª)
	const CDXFOption* pOpt = AfxGetNCVCApp()->GetDXFOption();
	CString	strLayer[SIZEOF(g_szSOrder)];
	strLayer[MKDX_STR_ORIGIN]	= pOpt->GetReadLayer(DXFORGLAYER);
	strLayer[MKDX_STR_CAMLINE]	= pOpt->GetReadLayer(DXFCAMLAYER);
	if ( strLayer[MKDX_STR_CAMLINE].FindOneOf(".|*?+(){}[]^$-\\") >= 0 ) {
		// ê≥ãKï\åªÇÃÇΩÇﬂÇÃì¡éÍÇ»ãLçÜÇ™ä‹Ç‹ÇÍÇÈÇ»ÇÁ
		strLayer[MKDX_STR_CAMLINE].Empty();
	}
	strLayer[MKDX_STR_MOVE]		= pOpt->GetReadLayer(DXFMOVLAYER);
	for ( i=0; i<SIZEOF(g_szSOrder); i++ ) {
		if ( strLayer[i].IsEmpty() )
			strLayer[i] = g_szDefaultLayer[i];
		m_strOption[i] = AfxGetApp()->GetProfileString(strRegKey, g_szSOrder[i], strLayer[i]);
	}
}

void CDXFMakeOption::Initialize_Default(void)
{
	// √ﬁÃ´Ÿƒê›íËÇ≈èâä˙âª
	int		i;

	for ( i=0; i<SIZEOF(g_szNOrder); i++ )
		m_unNums[i] = g_dfNOrder[i];
	for ( i=0; i<SIZEOF(g_szDOrder); i++ )
		m_udNums[i] = g_dfDOrder[i];
	for ( i=0; i<SIZEOF(g_szBOrder); i++ )
		m_ubFlags[i] = g_dfBOrder[i];

	// CStringå^µÃﬂºÆ›ÇÕè»ó™
}

/////////////////////////////////////////////////////////////////////////////
// “› ﬁä÷êî

BOOL CDXFMakeOption::SaveDXFMakeOption(void)
{
	// ⁄ºﬁΩƒÿÇ÷ÇÃï€ë∂
	int		i;
	CString	strRegKey(GetSubTreeRegKey(IDS_REGKEY_NC, IDS_REGKEY_MAKEDXF)),
			strResult;

	// intå^ñΩóﬂÇÃï€ë∂
	for ( i=0; i<SIZEOF(g_szNOrder); i++ ) {
		if ( !AfxGetApp()->WriteProfileInt(strRegKey, g_szNOrder[i], m_unNums[i]) )
			return FALSE;
	}
	// floatå^ñΩóﬂÇÃï€ë∂
	for ( i=0; i<SIZEOF(g_szDOrder); i++ ) {
		strResult.Format(IDS_MAKENCD_FORMAT, m_udNums[i]);
		if ( !AfxGetApp()->WriteProfileString(strRegKey, g_szDOrder[i], strResult) )
			return FALSE;
	}
	// BOOLå^ñΩóﬂÇÃï€ë∂
	for ( i=0; i<SIZEOF(g_szBOrder); i++ ) {
		if ( !AfxGetApp()->WriteProfileInt(strRegKey, g_szBOrder[i], m_ubFlags[i]) )
			return FALSE;
	}
	// CStringå^ñΩóﬂÇÃï€ë∂
	for ( i=0; i<SIZEOF(g_szSOrder); i++ ) {
		if ( !AfxGetApp()->WriteProfileString(strRegKey, g_szSOrder[i], m_strOption[i]) )
			return FALSE;
	}

	return TRUE;
}
