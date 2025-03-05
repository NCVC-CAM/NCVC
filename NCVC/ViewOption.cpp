// ViewOption.cpp: CViewOption ÉNÉâÉXÇÃÉCÉìÉvÉäÉÅÉìÉeÅ[ÉVÉáÉì
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "ViewOption.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace boost;
using std::string;

extern	LPCTSTR	g_szNdelimiter;	// "XYZUVWIJKRPLDH" from NCDoc.cpp

extern	LPCTSTR	g_szViewColDef[] = {
	"0:255:0",		// ägëÂèkè¨éwíËãÈå`
	"255:0:0"		// ëIëµÃﬁºﬁ™∏ƒ
};
extern	LPCTSTR	g_szNcViewColDef[] = {
	"0:0:0",		// îwåi1
	"0:0:0",		// îwåi2
	"255:255:255",	// Õﬂ≤›ï∂éö
	"255:255:0",	// X∂ﬁ≤ƒﬁ
	"0:255:255",	// Y
	"255:0:255",	// Z
	"0:255:0",		// ëÅëóÇË
	"255:0:0",		// êÿçÌëóÇË
	"255:0:0",		// êÿçÌëóÇË(Z)
	"255:128:0",	// å≈íËª≤∏Ÿ
	"255:255:0",	// â~å ï‚ä‘ÇÃíÜêS
	"255:255:255",	// ‹∞∏ãÈå`
	"0:0:255",		// ç≈ëÂêÿçÌãÈå`
	"255:255:0",	// ï‚ê≥ï\é¶
	"255:255:128",	// OpenGL ‹∞∏ãÈå`
	"255:128:128",	// OpenGL êÿçÌñ 
	"255:255:255"	// OpenGL ¥›ƒﬁ–Ÿ
};
extern	LPCTSTR	g_szNcInfoViewColDef[] = {
	"0:0:255",		// îwåi1
	"0:0:0",		// îwåi2
	"255:255:0"		// ï∂éöêF
};
extern	LPCTSTR	g_szDxfViewColDef[] = {
	"0:0:0",		// îwåi1
	"0:0:0",		// îwåi2
	"0:0:255",		// å¥ì_
	"255:0:0",		// êÿçÌ⁄≤‘
	"0:255:0",		// â¡çHäJénà íuéwé¶⁄≤‘
	"0:255:0",		// ã≠êßà⁄ìÆéwé¶⁄≤‘
	"255:255:0",	// ∫“›ƒóp√∑Ωƒï∂éö
	"0:255:255",	// ó÷äsµÃﬁºﬁ™∏ƒ
	"255:255:0"		// ‹∞∏ãÈå`(bind)
};
extern	const	int		g_nViewLineTypeDef[] = {
	2, 0
};
extern	const	int		g_nNcViewLineTypeDef[] = {
	1, 1, 1,
	2, 0, 0, 3, 2, 2
};
extern	const	int		g_nDxfViewLineTypeDef[] = {
	2, 0, 2, 0, 0, 1
};
extern	const	PENSTYLE	g_penStyle[] = {
	{"é¿ê¸", PS_SOLID,
		"CONTINUOUS", "Solid line", 0, 0.0f,
			{0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
		0xffff },
	{"îjê¸", PS_DASH,
		"DASHED", "___  ___  ___  ___  ", 2, 4.0f,
			{3.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f},
		0xfff0 },
	{"ì_ê¸", PS_DOT,
		"DOT", ". . . . . . . . ", 2, 1.0f,
			{0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f},
		0xcccc },
	{"àÍì_çΩê¸", PS_DASHDOT,
		"DASHDOT", "____ . ____ . ", 4, 32.0f,
			{30.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f},
		0xf0c3 },
	{"ìÒì_çΩê¸", PS_DASHDOTDOT,
		"DIVIDE", "____ . . ____ . . ", 6, 23.0f,
			{20.0f, -1.0f, 0.0f, -1.0f, 0.0f, -1.0f},
		0xf333 }
};
extern	const	int		g_nTraceSpeed[] = {
	10, 300, 600
};
extern	const	int		g_nForceView01[] = {
	// 0:XYZ, 1:XY, 2:XZ, 3:YZ
	0, 3, 2, 1		// 4ñ -1ÅFç∂è„, âEè„, ç∂â∫, âEâ∫
};
extern	const	int		g_nForceView02[] = {
	3, 2, 1, 0		// 4ñ -2ÅFç∂è„, ç∂íÜ, ç∂â∫, âE
};
extern	const	float	g_dDefaultGuideLength = 50.0f;

static	const	int	g_nFontSize[] = {	// Œﬂ≤›ƒéwíË
	9, 12
};
static	const	BOOL	g_bDefaultSetting[] = {
	TRUE,		// m_bDrawRevise
	FALSE,		// m_bDrawCircleCenter
	FALSE,		// m_bScale
	TRUE,		// m_bGuide
	TRUE,		// m_bSolidView
	TRUE,		// m_bUseFBO
	FALSE,		// m_bWirePath
	TRUE,		// m_bDragRender
	FALSE,		// m_bTexture
	FALSE,		// m_bLatheSlit
	FALSE,		// m_bNoActiveTraceGL
	TRUE		// m_bToolTrace
};
extern	LPCTSTR	g_szViewOptFlag[] = {	// to ViewSetup5.cpp
	"DrawRevise", "DrawCenterCircle",
	"GuideScale", "GuideLength",
	"SolidView", "UseFBO", "G00View", "DragRender",
	"Texture", "LatheSlit", "NoActiveTraceGL", "ToolTrace"
};
static	LPCTSTR	g_szViewOptInt[] = {
	"MillType", "FourView01", "FourView02"
};
static	LPCTSTR	g_szViewOptStr[] = {
	"TextureFile"
};
static	LPCTSTR	g_szDelFlag[] = {
	"MillT", "MillC"
};

/////////////////////////////////////////////////////////////////////////////
// CViewOption ÉNÉâÉXÇÃç\íz/è¡ñ≈

CViewOption::CViewOption()
{
	extern	LPTSTR	g_pszExecDir;	// é¿çs√ﬁ®⁄∏ƒÿ(NCVC.cpp)
	int		i;
	CString	strRegKey, strEntry, strEntryFormat, strResult;
	//
	m_dwUpdateFlg = 0;
	AllDefaultSetting();
	// NCVC.exe Ç∆ìØÇ∂Ã´Ÿ¿ﬁÇ… "NCVCcolor.ini" Ç™Ç†ÇÍÇŒ≤›Œﬂ∞ƒ
	strResult  = g_pszExecDir;
	strResult += "NCVCcolor.ini";
	CFileStatus		fStatus;
	if ( CFile::GetStatus(strResult, fStatus) )
		Inport(strResult);
	//
	VERIFY(strRegKey.LoadString(IDS_REGKEY_SETTINGS));
	VERIFY(strEntry.LoadString(IDS_REG_VIEW_WHEEL));
	m_bMouseWheel = AfxGetApp()->GetProfileInt(strRegKey, strEntry,
						m_bMouseWheel);
	VERIFY(strEntry.LoadString(IDS_REG_VIEW_WHEELTYPE));
	m_nWheelType = AfxGetApp()->GetProfileInt(strRegKey, strEntry,
						m_nWheelType);
	VERIFY(strEntry.LoadString(IDS_REG_CUSTOMCOLOR));
	for ( i=0; i<SIZEOF(m_colCustom); i++ ) {
		strEntryFormat.Format(IDS_COMMON_FORMAT, strEntry, i);
		strResult = AfxGetApp()->GetProfileString(strRegKey, strEntryFormat);
		if ( !strResult.IsEmpty() )
			m_colCustom[i] = ConvertSTRtoRGB(strResult);
	}
	VERIFY(strEntry.LoadString(IDS_REG_VIEW_COLOR));
	for ( i=0; i<SIZEOF(m_colView); i++ ) {
		strEntryFormat.Format(IDS_COMMON_FORMAT, strEntry, i);
		strResult = AfxGetApp()->GetProfileString(strRegKey, strEntryFormat);
		if ( !strResult.IsEmpty() )
			m_colView[i] = ConvertSTRtoRGB(strResult);
	}
	VERIFY(strEntry.LoadString(IDS_REG_VIEW_LINETYPE));
	for ( i=0; i<SIZEOF(m_nLineType); i++ ) {
		strEntryFormat.Format(IDS_COMMON_FORMAT, strEntry, i);
		m_nLineType[i] = AfxGetApp()->GetProfileInt(strRegKey, strEntryFormat,
								m_nLineType[i]);
	}
	VERIFY(strEntry.LoadString(IDS_REG_LOGFONT));
	UINT	nBytes = sizeof(LOGFONT);
	LPBYTE	lpFont = NULL;
	if ( AfxGetApp()->GetProfileBinary(strRegKey, strEntry, &lpFont, &nBytes) ) {
		memcpy(&m_lfFont[TYPE_NCD], lpFont, sizeof(LOGFONT));
		delete[]	lpFont;
	}
	//
	VERIFY(strRegKey.LoadString(IDS_REGKEY_NC));
	for ( i=0; i<SIZEOF(m_bNCFlag); i++ ) {
		m_bNCFlag[i] = AfxGetApp()->GetProfileInt(strRegKey, g_szViewOptFlag[i], m_bNCFlag[i]);
	}
	m_bNCFlag[GLOPTFLG_USEFBO] = TRUE;		// Ver4.16à⁄çsÇÕFBOã≠êß
	VERIFY(strEntry.LoadString(IDS_REG_VIEW_COLOR));
	for ( i=0; i<SIZEOF(m_colNCView); i++ ) {
		strEntryFormat.Format(IDS_COMMON_FORMAT, strEntry, i);
		strResult = AfxGetApp()->GetProfileString(strRegKey, strEntryFormat);
		if ( !strResult.IsEmpty() )
			m_colNCView[i] = ConvertSTRtoRGB(strResult);
	}
	VERIFY(strEntry.LoadString(IDS_REG_VIEW_NC_INFOCOL));
	for ( i=0; i<SIZEOF(m_colNCInfoView); i++ ) {
		strEntryFormat.Format(IDS_COMMON_FORMAT, strEntry, i);
		strResult = AfxGetApp()->GetProfileString(strRegKey, strEntryFormat);
		if ( !strResult.IsEmpty() )
			m_colNCInfoView[i] = ConvertSTRtoRGB(strResult);
	}
	VERIFY(strEntry.LoadString(IDS_REG_VIEW_LINETYPE));
	for ( i=0; i<SIZEOF(m_nNCLineType); i++ ) {
		strEntryFormat.Format(IDS_COMMON_FORMAT, strEntry, i);
		m_nNCLineType[i] = AfxGetApp()->GetProfileInt(strRegKey, strEntryFormat,
								m_nNCLineType[i]);
	}
	for ( i=0; i<NCXYZ; i++ ) {
		strResult = AfxGetApp()->GetProfileString(strRegKey, strEntry+g_szNdelimiter[i]);
		if ( !strResult.IsEmpty() )
			m_dGuide[i] = (float)atof(LPCTSTR(strResult.Trim()));
	}
	VERIFY(strEntry.LoadString(IDS_REG_NCV_TRACESPEED));
	for ( i=0; i<SIZEOF(m_nTraceSpeed); i++ ) {
		strEntryFormat.Format(IDS_COMMON_FORMAT, strEntry, i);
		m_nTraceSpeed[i] = AfxGetApp()->GetProfileInt(strRegKey, strEntryFormat,
								m_nTraceSpeed[i]);
	}
	VERIFY(strEntry.LoadString(IDS_REG_VIEW_NC_DEFAULTENDMILL));
	strResult = AfxGetApp()->GetProfileString(strRegKey, strEntry);
	if ( !strResult.IsEmpty() )
		m_dDefaultEndmill = (float)fabs(atof(LPCTSTR(strResult.Trim()))) / 2.0f;	// “”ÿè„ÇÕîºåa
	m_nMillType = AfxGetApp()->GetProfileInt(strRegKey, g_szViewOptInt[VIEWINT_MILLTYPE], m_nMillType);
	for ( i=0; i<SIZEOF(m_nForceView01); i++ ) {
		strEntryFormat.Format(IDS_COMMON_FORMAT, g_szViewOptInt[VIEWINT_FOURVIEW01], i);
		m_nForceView01[i] = AfxGetApp()->GetProfileInt(strRegKey, strEntryFormat,
								m_nForceView01[i]);
	}
	for ( i=0; i<SIZEOF(m_nForceView02); i++ ) {
		strEntryFormat.Format(IDS_COMMON_FORMAT, g_szViewOptInt[VIEWINT_FOURVIEW02], i);
		m_nForceView02[i] = AfxGetApp()->GetProfileInt(strRegKey, strEntryFormat,
								m_nForceView02[i]);
	}
	m_strTexture = AfxGetApp()->GetProfileString(strRegKey, g_szViewOptStr[0]);
	//
	VERIFY(strRegKey.LoadString(IDS_REGKEY_DXF));
	VERIFY(strEntry.LoadString(IDS_REG_VIEW_COLOR));
	for ( i=0; i<SIZEOF(m_colDXFView); i++ ) {
		strEntryFormat.Format(IDS_COMMON_FORMAT, strEntry, i);
		strResult = AfxGetApp()->GetProfileString(strRegKey, strEntryFormat);
		if ( !strResult.IsEmpty() )
			m_colDXFView[i] = ConvertSTRtoRGB(strResult);
	}
	VERIFY(strEntry.LoadString(IDS_REG_VIEW_LINETYPE));
	for ( i=0; i<SIZEOF(m_nDXFLineType); i++ ) {
		strEntryFormat.Format(IDS_COMMON_FORMAT, strEntry, i);
		m_nDXFLineType[i] = AfxGetApp()->GetProfileInt(strRegKey, strEntryFormat,
								m_nDXFLineType[i]);
	}
	VERIFY(strEntry.LoadString(IDS_REG_LOGFONT));
	nBytes = sizeof(LOGFONT);
	lpFont = NULL;
	if ( AfxGetApp()->GetProfileBinary(strRegKey, strEntry, &lpFont, &nBytes) ) {
		memcpy(&m_lfFont[TYPE_DXF], lpFont, sizeof(LOGFONT));
		delete[]	lpFont;
	}
}

/////////////////////////////////////////////////////////////////////////////
// “› ﬁä÷êî

void CViewOption::AllDefaultSetting(void)
{
	ASSERT( SIZEOF(m_colView) == SIZEOF(g_szViewColDef) );
	ASSERT( SIZEOF(m_colNCView) == SIZEOF(g_szNcViewColDef) );
	ASSERT( SIZEOF(m_colNCInfoView) == SIZEOF(g_szNcInfoViewColDef) );
	ASSERT( SIZEOF(m_colDXFView) == SIZEOF(g_szDxfViewColDef) );
	ASSERT( SIZEOF(m_nLineType) == SIZEOF(g_nViewLineTypeDef) );
	ASSERT( SIZEOF(m_nNCLineType) == SIZEOF(g_nNcViewLineTypeDef) );
	ASSERT( SIZEOF(m_nDXFLineType) == SIZEOF(g_nDxfViewLineTypeDef) );
	ASSERT( SIZEOF(m_nTraceSpeed) == SIZEOF(g_nTraceSpeed) );
	ASSERT( SIZEOF(m_lfFont) == SIZEOF(g_nFontSize) );
	ASSERT( SIZEOF(m_bNCFlag) == SIZEOF(g_bDefaultSetting) );
	ASSERT( SIZEOF(g_szViewOptFlag) == SIZEOF(g_bDefaultSetting) );
	ASSERT( SIZEOF(m_nForceView01) == SIZEOF(g_nForceView01) );
	ASSERT( SIZEOF(m_nForceView02) == SIZEOF(g_nForceView02) );
	//
	int		i;
	//
	m_bMouseWheel	= FALSE;
	m_nWheelType	= 0;
	for ( i=0; i<SIZEOF(m_colCustom); i++ )
		m_colCustom[i] = RGB(255, 255, 255);
	for ( i=0; i<SIZEOF(m_colView); i++ )
		m_colView[i] = ConvertSTRtoRGB(g_szViewColDef[i]);
	for ( i=0; i<SIZEOF(m_nLineType); i++ )
		m_nLineType[i] = g_nViewLineTypeDef[i];
	CClientDC	dc(AfxGetMainWnd());	
	for ( i=0; i<SIZEOF(m_lfFont); i++ ) {
		::ZeroMemory(&m_lfFont[i], sizeof(LOGFONT));
		m_lfFont[i].lfCharSet = DEFAULT_CHARSET;
	}
		// TYPE_NCD ÇÕí èÌª≤Ωﬁ
	m_lfFont[TYPE_NCD].lfHeight = -MulDiv(g_nFontSize[TYPE_NCD], dc.GetDeviceCaps(LOGPIXELSY), 72);
		// TYPE_DXF ÇÕMM_LOMETRICÇ…çáÇÌÇπÇƒª≤Ωﬁí≤êÆ
	dc.SetMapMode(MM_LOMETRIC);
	CPoint	pt(0, MulDiv(g_nFontSize[TYPE_DXF], dc.GetDeviceCaps(LOGPIXELSY), 72));
	dc.DPtoLP(&pt);
	m_lfFont[TYPE_DXF].lfHeight = -abs(pt.y);
	//
	for ( i=0; i<SIZEOF(m_bNCFlag); i++ )
		m_bNCFlag[i] = g_bDefaultSetting[i];
	for ( i=0; i<SIZEOF(m_colNCView); i++ )
		m_colNCView[i] = ConvertSTRtoRGB(g_szNcViewColDef[i]);
	for ( i=0; i<SIZEOF(m_colNCInfoView); i++ )
		m_colNCInfoView[i] = ConvertSTRtoRGB(g_szNcInfoViewColDef[i]);
	for ( i=0; i<SIZEOF(m_nNCLineType); i++ )
		m_nNCLineType[i] = g_nNcViewLineTypeDef[i];
	for ( i=0; i<NCXYZ; i++ )
		m_dGuide[i] = g_dDefaultGuideLength;	// 50mm å≈íË
	for ( i=0; i<SIZEOF(m_nTraceSpeed); i++ )
		m_nTraceSpeed[i] = g_nTraceSpeed[i];
	//
	m_dDefaultEndmill = 0.5;	// √ﬁÃ´Ÿƒ¥›ƒﬁ–Ÿåa 1mm
	m_nMillType = 0;			// Ω∏≥™±¥›ƒﬁ–Ÿ
	for ( i=0; i<SIZEOF(m_nForceView01); i++ )
		m_nForceView01[i] = g_nForceView01[i];
	for ( i=0; i<SIZEOF(m_nForceView02); i++ )
		m_nForceView02[i] = g_nForceView02[i];
	//
	for ( i=0; i<SIZEOF(m_colDXFView); i++ )
		m_colDXFView[i] = ConvertSTRtoRGB(g_szDxfViewColDef[i]);
	for ( i=0; i<SIZEOF(m_nDXFLineType); i++ )
		m_nDXFLineType[i] = g_nDxfViewLineTypeDef[i];
}

BOOL CViewOption::SaveViewOption(void)
{
	int		i;
	CString	strRegKey, strEntry, strEntryFormat, strResult;

	VERIFY(strRegKey.LoadString(IDS_REGKEY_SETTINGS));
	VERIFY(strEntry.LoadString(IDS_REG_VIEW_WHEEL));
	if ( !AfxGetApp()->WriteProfileInt(strRegKey, strEntry, m_bMouseWheel) )
		return FALSE;
	VERIFY(strEntry.LoadString(IDS_REG_VIEW_WHEELTYPE));
	if ( !AfxGetApp()->WriteProfileInt(strRegKey, strEntry, m_nWheelType) )
		return FALSE;
	VERIFY(strEntry.LoadString(IDS_REG_CUSTOMCOLOR));
	for ( i=0; i<SIZEOF(m_colCustom); i++ ) {
		strEntryFormat.Format(IDS_COMMON_FORMAT, strEntry, i);
		if ( !AfxGetApp()->WriteProfileString(strRegKey, strEntryFormat,
				ConvertRGBtoSTR(m_colCustom[i])) )
			return FALSE;
	}
	VERIFY(strEntry.LoadString(IDS_REG_VIEW_COLOR));
	for ( i=0; i<SIZEOF(m_colView); i++ ) {
		strEntryFormat.Format(IDS_COMMON_FORMAT, strEntry, i);
		if ( !AfxGetApp()->WriteProfileString(strRegKey, strEntryFormat,
				ConvertRGBtoSTR(m_colView[i])) )
			return FALSE;
	}
	VERIFY(strEntry.LoadString(IDS_REG_VIEW_LINETYPE));
	for ( i=0; i<SIZEOF(m_nLineType); i++ ) {
		strEntryFormat.Format(IDS_COMMON_FORMAT, strEntry, i);
		if ( !AfxGetApp()->WriteProfileInt(strRegKey, strEntryFormat, m_nLineType[i]) )
			return FALSE;
	}
	VERIFY(strEntry.LoadString(IDS_REG_LOGFONT));
	if ( !AfxGetApp()->WriteProfileBinary(strRegKey, strEntry,
			(LPBYTE)&m_lfFont[TYPE_NCD], sizeof(LOGFONT)) )
		return FALSE;
	//
	VERIFY(strRegKey.LoadString(IDS_REGKEY_NC));
	for ( i=0; i<SIZEOF(m_bNCFlag); i++ ) {
		if ( !AfxGetApp()->WriteProfileInt(strRegKey, g_szViewOptFlag[i], m_bNCFlag[i]) )
			return FALSE;
	}
	for ( i=0; i<SIZEOF(g_szDelFlag); i++ ) {
		AfxGetApp()->WriteProfileInt(strRegKey, g_szDelFlag[i], NULL);	// ïsóv∑∞ÇÃçÌèú
	}
	VERIFY(strEntry.LoadString(IDS_REG_VIEW_COLOR));
	for ( i=0; i<SIZEOF(m_colNCView); i++ ) {
		strEntryFormat.Format(IDS_COMMON_FORMAT, strEntry, i);
		if ( !AfxGetApp()->WriteProfileString(strRegKey, strEntryFormat,
				ConvertRGBtoSTR(m_colNCView[i])) )
			return FALSE;
	}
	VERIFY(strEntry.LoadString(IDS_REG_VIEW_NC_INFOCOL));
	for ( i=0; i<SIZEOF(m_colNCInfoView); i++ ) {
		strEntryFormat.Format(IDS_COMMON_FORMAT, strEntry, i);
		if ( !AfxGetApp()->WriteProfileString(strRegKey, strEntryFormat,
				ConvertRGBtoSTR(m_colNCInfoView[i])) )
			return FALSE;
	}
	VERIFY(strEntry.LoadString(IDS_REG_VIEW_LINETYPE));
	for ( i=0; i<SIZEOF(m_nNCLineType); i++ ) {
		strEntryFormat.Format(IDS_COMMON_FORMAT, strEntry, i);
		if ( !AfxGetApp()->WriteProfileInt(strRegKey, strEntryFormat, m_nNCLineType[i]) )
			return FALSE;
	}
	for ( i=0; i<NCXYZ; i++ ) {
		strResult.Format(IDS_MAKENCD_FORMAT, m_dGuide[i]);
		if ( !AfxGetApp()->WriteProfileString(strRegKey, strEntry+g_szNdelimiter[i], strResult) )
			return FALSE;
	}
	VERIFY(strEntry.LoadString(IDS_REG_NCV_TRACESPEED));
	for ( i=0; i<SIZEOF(m_nTraceSpeed); i++ ) {
		strEntryFormat.Format(IDS_COMMON_FORMAT, strEntry, i);
		if ( !AfxGetApp()->WriteProfileInt(strRegKey, strEntryFormat, m_nTraceSpeed[i]) )
			return FALSE;
	}
	VERIFY(strEntry.LoadString(IDS_REG_VIEW_NC_DEFAULTENDMILL));
	strResult.Format(IDS_MAKENCD_FORMAT, m_dDefaultEndmill * 2.0);	// ï€ë∂ÇÕíºåa
	if ( !AfxGetApp()->WriteProfileString(strRegKey, strEntry, strResult) )
		return FALSE;
	if ( !AfxGetApp()->WriteProfileInt(strRegKey, g_szViewOptInt[VIEWINT_MILLTYPE], m_nMillType) )
		return FALSE;
	for ( i=0; i<SIZEOF(m_nForceView01); i++ ) {
		strEntryFormat.Format(IDS_COMMON_FORMAT, g_szViewOptInt[VIEWINT_FOURVIEW01], i);
		if ( !AfxGetApp()->WriteProfileInt(strRegKey, strEntryFormat, m_nForceView01[i]) )
			return FALSE;
	}
	for ( i=0; i<SIZEOF(m_nForceView02); i++ ) {
		strEntryFormat.Format(IDS_COMMON_FORMAT, g_szViewOptInt[VIEWINT_FOURVIEW02], i);
		if ( !AfxGetApp()->WriteProfileInt(strRegKey, strEntryFormat, m_nForceView02[i]) )
			return FALSE;
	}
	if ( !AfxGetApp()->WriteProfileString(strRegKey, g_szViewOptStr[0], m_strTexture) )
		return FALSE;
	//
	VERIFY(strRegKey.LoadString(IDS_REGKEY_DXF));
	VERIFY(strEntry.LoadString(IDS_REG_VIEW_COLOR));
	for ( i=0; i<SIZEOF(m_colDXFView); i++ ) {
		strEntryFormat.Format(IDS_COMMON_FORMAT, strEntry, i);
		if ( !AfxGetApp()->WriteProfileString(strRegKey, strEntryFormat,
				ConvertRGBtoSTR(m_colDXFView[i])) )
			return FALSE;
	}
	VERIFY(strEntry.LoadString(IDS_REG_VIEW_LINETYPE));
	for ( i=0; i<SIZEOF(m_nDXFLineType); i++ ) {
		strEntryFormat.Format(IDS_COMMON_FORMAT, strEntry, i);
		if ( !AfxGetApp()->WriteProfileInt(strRegKey, strEntryFormat, m_nDXFLineType[i]) )
			return FALSE;
	}
	VERIFY(strEntry.LoadString(IDS_REG_LOGFONT));
	if ( !AfxGetApp()->WriteProfileBinary(strRegKey, strEntry,
			(LPBYTE)&m_lfFont[TYPE_DXF], sizeof(LOGFONT)) )
		return FALSE;

	return TRUE;
}

BOOL CViewOption::Export(LPCTSTR lpszFileName)
{
/* 
	ì∆é©ÇÃèoóÕÇÕñ ì|Ç»ÇÃÇ≈
	Win32API ÇÃ WritePrivateProfileString() ä÷êîÇégÇ§
*/
	int		i;
	CString	strRegKey, strEntry, strEntryFormat, strResult;
	//
	VERIFY(strRegKey.LoadString(IDS_REGKEY_SETTINGS));
	VERIFY(strEntry.LoadString(IDS_REG_VIEW_WHEEL));
	strResult = lexical_cast<string>(m_bMouseWheel ? 1 : 0).c_str();
	if ( !::WritePrivateProfileString(strRegKey, strEntry, strResult, lpszFileName) )
		return FALSE;
	VERIFY(strEntry.LoadString(IDS_REG_VIEW_WHEELTYPE));
	strResult = lexical_cast<string>(m_nWheelType).c_str();
	if ( !::WritePrivateProfileString(strRegKey, strEntry, strResult, lpszFileName) )
		return FALSE;
	VERIFY(strEntry.LoadString(IDS_REG_CUSTOMCOLOR));
	for ( i=0; i<SIZEOF(m_colCustom); i++ ) {
		strEntryFormat.Format(IDS_COMMON_FORMAT, strEntry, i);
		if ( !::WritePrivateProfileString(strRegKey, strEntryFormat,
					ConvertRGBtoSTR(m_colCustom[i]), lpszFileName) )
			return FALSE;
	}
	VERIFY(strEntry.LoadString(IDS_REG_VIEW_COLOR));
	for ( i=0; i<SIZEOF(m_colView); i++ ) {
		strEntryFormat.Format(IDS_COMMON_FORMAT, strEntry, i);
		if ( !::WritePrivateProfileString(strRegKey, strEntryFormat,
					ConvertRGBtoSTR(m_colView[i]), lpszFileName) )
			return FALSE;
	}
	VERIFY(strEntry.LoadString(IDS_REG_VIEW_LINETYPE));
	for ( i=0; i<SIZEOF(m_nLineType); i++ ) {
		strEntryFormat.Format(IDS_COMMON_FORMAT, strEntry, i);
		strResult = lexical_cast<string>(m_nLineType[i]).c_str();
		if ( !::WritePrivateProfileString(strRegKey, strEntryFormat, strResult, lpszFileName) )
			return FALSE;
	}
	//
	VERIFY(strRegKey.LoadString(IDS_REGKEY_NC));
	for ( i=0; i<SIZEOF(m_bNCFlag); i++ ) {
		strResult = lexical_cast<string>(m_bNCFlag[i] ? 1 : 0).c_str();
		if ( !::WritePrivateProfileString(strRegKey, g_szViewOptFlag[i], strResult, lpszFileName) )
			return FALSE;
	}
	VERIFY(strEntry.LoadString(IDS_REG_VIEW_COLOR));
	for ( i=0; i<SIZEOF(m_colNCView); i++ ) {
		strEntryFormat.Format(IDS_COMMON_FORMAT, strEntry, i);
		if ( !::WritePrivateProfileString(strRegKey, strEntryFormat,
					ConvertRGBtoSTR(m_colNCView[i]), lpszFileName) )
			return FALSE;
	}
	VERIFY(strEntry.LoadString(IDS_REG_VIEW_NC_INFOCOL));
	for ( i=0; i<SIZEOF(m_colNCInfoView); i++ ) {
		strEntryFormat.Format(IDS_COMMON_FORMAT, strEntry, i);
		if ( !::WritePrivateProfileString(strRegKey, strEntryFormat,
					ConvertRGBtoSTR(m_colNCInfoView[i]), lpszFileName) )
			return FALSE;
	}
	VERIFY(strEntry.LoadString(IDS_REG_VIEW_LINETYPE));
	for ( i=0; i<SIZEOF(m_nNCLineType); i++ ) {
		strEntryFormat.Format(IDS_COMMON_FORMAT, strEntry, i);
		strResult = lexical_cast<string>(m_nNCLineType[i]).c_str();
		if ( !::WritePrivateProfileString(strRegKey, strEntryFormat, strResult, lpszFileName) )
			return FALSE;
	}
	for ( i=0; i<NCXYZ; i++ ) {
		strResult.Format(IDS_MAKENCD_FORMAT, m_dGuide[i]);
		if ( !::WritePrivateProfileString(strRegKey, strEntry+g_szNdelimiter[i],
					strResult, lpszFileName) )
			return FALSE;
	}
	VERIFY(strEntry.LoadString(IDS_REG_NCV_TRACESPEED));
	for ( i=0; i<SIZEOF(m_nTraceSpeed); i++ ) {
		strEntryFormat.Format(IDS_COMMON_FORMAT, strEntry, i);
		strResult = lexical_cast<string>(m_nTraceSpeed[i]).c_str();
		if ( !::WritePrivateProfileString(strRegKey, strEntryFormat, strResult, lpszFileName) )
			return FALSE;
	}
	VERIFY(strEntry.LoadString(IDS_REG_VIEW_NC_DEFAULTENDMILL));
	strResult.Format(IDS_MAKENCD_FORMAT, m_dDefaultEndmill * 2.0);	// ï€ë∂ÇÕíºåa
	if ( !::WritePrivateProfileString(strRegKey, strEntry, strResult, lpszFileName) )
		return FALSE;
	strResult = lexical_cast<string>(m_nMillType).c_str();
	if ( !::WritePrivateProfileString(strRegKey, g_szViewOptInt[VIEWINT_MILLTYPE], strResult, lpszFileName) )
		return FALSE;
	for ( i=0; i<SIZEOF(m_nForceView01); i++ ) {
		strEntryFormat.Format(IDS_COMMON_FORMAT, g_szViewOptInt[VIEWINT_FOURVIEW01], i);
		strResult = lexical_cast<string>(m_nForceView01[i]).c_str();
		if ( !::WritePrivateProfileString(strRegKey, strEntryFormat, strResult, lpszFileName) )
			return FALSE;
	}
	for ( i=0; i<SIZEOF(m_nForceView02); i++ ) {
		strEntryFormat.Format(IDS_COMMON_FORMAT, g_szViewOptInt[VIEWINT_FOURVIEW02], i);
		strResult = lexical_cast<string>(m_nForceView02[i]).c_str();
		if ( !::WritePrivateProfileString(strRegKey, strEntryFormat, strResult, lpszFileName) )
			return FALSE;
	}
	if ( !::WritePrivateProfileString(strRegKey, g_szViewOptStr[0], m_strTexture, lpszFileName) )
		return FALSE;
	//
	VERIFY(strRegKey.LoadString(IDS_REGKEY_DXF));
	VERIFY(strEntry.LoadString(IDS_REG_VIEW_COLOR));
	for ( i=0; i<SIZEOF(m_colDXFView); i++ ) {
		strEntryFormat.Format(IDS_COMMON_FORMAT, strEntry, i);
		if ( !::WritePrivateProfileString(strRegKey, strEntryFormat,
					ConvertRGBtoSTR(m_colDXFView[i]), lpszFileName) )
			return FALSE;
	}
	VERIFY(strEntry.LoadString(IDS_REG_VIEW_LINETYPE));
	for ( i=0; i<SIZEOF(m_nDXFLineType); i++ ) {
		strEntryFormat.Format(IDS_COMMON_FORMAT, strEntry, i);
		strResult = lexical_cast<string>(m_nDXFLineType[i]).c_str();
		if ( !::WritePrivateProfileString(strRegKey, strEntryFormat, strResult, lpszFileName) )
			return FALSE;
	}

	return TRUE;
}

void CViewOption::Inport(LPCTSTR lpszFileName)
{
	int		i;
	TCHAR	szResult[_MAX_PATH];
	CString	strRegKey, strEntry, strEntryFormat, strResult;
	//
	VERIFY(strRegKey.LoadString(IDS_REGKEY_SETTINGS));
	VERIFY(strEntry.LoadString(IDS_REG_VIEW_WHEEL));
	m_bMouseWheel = (BOOL)::GetPrivateProfileInt(strRegKey, strEntry,
								(UINT)m_bMouseWheel, lpszFileName);
	VERIFY(strEntry.LoadString(IDS_REG_VIEW_WHEELTYPE));
	m_nWheelType = ::GetPrivateProfileInt(strRegKey, strEntry,
							m_nWheelType, lpszFileName);
	VERIFY(strEntry.LoadString(IDS_REG_CUSTOMCOLOR));
	for ( i=0; i<SIZEOF(m_colCustom); i++ ) {
		strEntryFormat.Format(IDS_COMMON_FORMAT, strEntry, i);
		::GetPrivateProfileString(strRegKey, strEntryFormat, "",
				szResult, _MAX_PATH, lpszFileName);
		if ( lstrlen(szResult) > 0 )
			m_colCustom[i] = ConvertSTRtoRGB(szResult);
	}
	VERIFY(strEntry.LoadString(IDS_REG_VIEW_COLOR));
	for ( i=0; i<SIZEOF(m_colView); i++ ) {
		strEntryFormat.Format(IDS_COMMON_FORMAT, strEntry, i);
		::GetPrivateProfileString(strRegKey, strEntryFormat, "",
				szResult, _MAX_PATH, lpszFileName);
		if ( lstrlen(szResult) > 0 )
			m_colView[i] = ConvertSTRtoRGB(szResult);
	}
	VERIFY(strEntry.LoadString(IDS_REG_VIEW_LINETYPE));
	for ( i=0; i<SIZEOF(m_nLineType); i++ ) {
		strEntryFormat.Format(IDS_COMMON_FORMAT, strEntry, i);
		m_nLineType[i] = ::GetPrivateProfileInt(strRegKey, strEntryFormat,
								m_nLineType[i], lpszFileName);
	}
	//
	VERIFY(strRegKey.LoadString(IDS_REGKEY_NC));
	for ( i=0; i<SIZEOF(m_bNCFlag); i++ ) {
		m_bNCFlag[i] = (BOOL)::GetPrivateProfileInt(strRegKey, g_szViewOptFlag[i],
								(UINT)m_bNCFlag[i], lpszFileName);
	}
	m_bNCFlag[GLOPTFLG_USEFBO] = TRUE;		// Ver4.16à⁄çsÇÕFBOã≠êß
	VERIFY(strEntry.LoadString(IDS_REG_VIEW_COLOR));
	for ( i=0; i<SIZEOF(m_colNCView); i++ ) {
		strEntryFormat.Format(IDS_COMMON_FORMAT, strEntry, i);
		::GetPrivateProfileString(strRegKey, strEntryFormat, "",
				szResult, _MAX_PATH, lpszFileName);
		if ( lstrlen(szResult) > 0 )
			m_colNCView[i] = ConvertSTRtoRGB(szResult);
	}
	VERIFY(strEntry.LoadString(IDS_REG_VIEW_NC_INFOCOL));
	for ( i=0; i<SIZEOF(m_colNCInfoView); i++ ) {
		strEntryFormat.Format(IDS_COMMON_FORMAT, strEntry, i);
		::GetPrivateProfileString(strRegKey, strEntryFormat, "",
				szResult, _MAX_PATH, lpszFileName);
		if ( lstrlen(szResult) > 0 )
			m_colNCInfoView[i] = ConvertSTRtoRGB(szResult);
	}
	VERIFY(strEntry.LoadString(IDS_REG_VIEW_LINETYPE));
	for ( i=0; i<SIZEOF(m_nNCLineType); i++ ) {
		strEntryFormat.Format(IDS_COMMON_FORMAT, strEntry, i);
		m_nNCLineType[i] = ::GetPrivateProfileInt(strRegKey, strEntryFormat,
								m_nNCLineType[i], lpszFileName);
	}
	for ( i=0; i<NCXYZ; i++ ) {
		::GetPrivateProfileString(strRegKey, strEntry+g_szNdelimiter[i], "",
				szResult, _MAX_PATH, lpszFileName);
		if ( lstrlen(szResult) > 0 )
			m_dGuide[i] = (float)atof(boost::algorithm::trim_copy(string(szResult)).c_str());
	}
	VERIFY(strEntry.LoadString(IDS_REG_NCV_TRACESPEED));
	for ( i=0; i<SIZEOF(m_nTraceSpeed); i++ ) {
		strEntryFormat.Format(IDS_COMMON_FORMAT, strEntry, i);
		m_nTraceSpeed[i] = ::GetPrivateProfileInt(strRegKey, strEntryFormat,
								m_nTraceSpeed[i], lpszFileName);
	}
	VERIFY(strEntry.LoadString(IDS_REG_VIEW_NC_DEFAULTENDMILL));
	::GetPrivateProfileString(strRegKey, strEntry, "",
			szResult, _MAX_PATH, lpszFileName);
	if ( lstrlen(szResult) > 0 )
		m_dDefaultEndmill = (float)fabs(atof(boost::algorithm::trim_copy(string(szResult)).c_str())) / 2.0f;	// “”ÿè„ÇÕîºåa
	m_nMillType = ::GetPrivateProfileInt(strRegKey, g_szViewOptInt[VIEWINT_MILLTYPE], m_nMillType, lpszFileName);
	for ( i=0; i<SIZEOF(m_nForceView01); i++ ) {
		strEntryFormat.Format(IDS_COMMON_FORMAT, g_szViewOptInt[VIEWINT_FOURVIEW01], i);
		m_nForceView01[i] = ::GetPrivateProfileInt(strRegKey, strEntryFormat,
								m_nForceView01[i], lpszFileName);
	}
	for ( i=0; i<SIZEOF(m_nForceView02); i++ ) {
		strEntryFormat.Format(IDS_COMMON_FORMAT, g_szViewOptInt[VIEWINT_FOURVIEW02], i);
		m_nForceView02[i] = ::GetPrivateProfileInt(strRegKey, strEntryFormat,
								m_nForceView02[i], lpszFileName);
	}
	::GetPrivateProfileString(strRegKey, g_szViewOptStr[0], "",
			szResult, _MAX_PATH, lpszFileName);
	if ( lstrlen(szResult) > 0 )
		m_strTexture = szResult;
	//
	VERIFY(strRegKey.LoadString(IDS_REGKEY_DXF));
	VERIFY(strEntry.LoadString(IDS_REG_VIEW_COLOR));
	for ( i=0; i<SIZEOF(m_colDXFView); i++ ) {
		strEntryFormat.Format(IDS_COMMON_FORMAT, strEntry, i);
		::GetPrivateProfileString(strRegKey, strEntryFormat, "",
				szResult, _MAX_PATH, lpszFileName);
		if ( lstrlen(szResult) > 0 )
			m_colDXFView[i] = ConvertSTRtoRGB(szResult);
	}
	VERIFY(strEntry.LoadString(IDS_REG_VIEW_LINETYPE));
	for ( i=0; i<SIZEOF(m_nDXFLineType); i++ ) {
		strEntryFormat.Format(IDS_COMMON_FORMAT, strEntry, i);
		m_nDXFLineType[i] = ::GetPrivateProfileInt(strRegKey, strEntryFormat,
								m_nDXFLineType[i], lpszFileName);
	}
}
