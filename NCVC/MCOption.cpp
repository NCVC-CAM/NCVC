// MCOption.cpp: CMCOption ÉNÉâÉXÇÃÉCÉìÉvÉäÉÅÉìÉeÅ[ÉVÉáÉì
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NCVC.h"
#include "MCOption.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

using namespace boost;

extern	LPCTSTR	g_szNdelimiter;		// "XYZRIJKPLDH" from NCDoc.cpp
extern	LPCTSTR	gg_szComma;			// ","
//
extern	LPCTSTR	gg_szRegKey;
// ã@äBèÓïÒóöóÉTÉCÉY
#define	NCMAXMCFILE		10

// intå^ñΩóﬂ
static	LPCTSTR	g_szNOrder[] = {
	"Modal%d",
	"G0Speed%c",
	"FDot", "CorrectType"
};
static	const	int		g_dfNOrder[] = {
	0, 0, 0, 0, 0,
	0, 0, 0,
	0, 0
};

// doubleå^ñΩóﬂ
static	LPCTSTR	g_szDOrder[] = {
	"DefaultFeed",
	"BlockTime",
	"Initial%c"
};
static	const	double	g_dfDOrder[] = {
	30.0,
	0.0, 0.0, 0.0,
	0.0,
	// G54Å`G59
	0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
	0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0
};

// BOOLå^ñΩóﬂ
static	LPCTSTR	g_szBOrder[] = {
	"L0Cycle"
};
static	const	BOOL	g_dfBOrder[] = {
	FALSE
};

// CStringå^ñΩóﬂ
static	LPCTSTR	g_szSOrder[] = {
	"Name",
	"Tool%d"
};
static	LPCTSTR	g_szSOrderMacro[] = {
	"MacroCode", "MacroFolder", "MacroIF", "MacroArgv",
	"MacroResult", "MachineFile", "CurrentFolder"	// íuä∑à¯êîÇÃÇ›Ç…égóp
};
extern	int		g_nDefaultMacroID[] = {
	MCMACHINEFILE, MCMACROCODE, MCMACROFOLDER, MCCURRENTFOLDER, MCMACRORESULT
};

/////////////////////////////////////////////////////////////////////////////
// CMCOption ÉNÉâÉXÇÃç\íz/è¡ñ≈

CMCOption::CMCOption()
{
	CString	strRegKey, strEntry, strResult, strFmt;
	int		i;

	// “› ﬁïœêîÇÃèâä˙âª
	ASSERT( SIZEOF(m_unNums) == SIZEOF(g_dfNOrder) );
	ASSERT( SIZEOF(m_udNums) == SIZEOF(g_dfDOrder) );
	ASSERT( SIZEOF(m_ubFlgs) == SIZEOF(g_dfBOrder) );
	for ( i=0; i<SIZEOF(g_dfNOrder); i++ )
		m_unNums[i] = g_dfNOrder[i];
	for ( i=0; i<SIZEOF(g_dfDOrder); i++ )
		m_udNums[i] = g_dfDOrder[i];
	for ( i=0; i<SIZEOF(g_dfBOrder); i++ )
		m_ubFlgs[i] = g_dfBOrder[i];

	VERIFY(strRegKey.LoadString(IDS_REGKEY_NC));

	// ã@äBèÓïÒóöó
	try {
		VERIFY(strEntry.LoadString(IDS_REG_NCV_MCFILE));
		for ( i=0; i<NCMAXMCFILE; i++ ) {
			strFmt.Format(IDS_COMMON_FORMAT, strEntry, i);
			strResult = AfxGetApp()->GetProfileString(strRegKey, strFmt);
			// Ãß≤ŸÇ™ë∂ç›Ç∑ÇÈÇ∆Ç´ÇæÇØóöóÇ…ìoò^
			if ( !strResult.IsEmpty() && ::IsFileExist(strResult, TRUE, FALSE) )
				m_strMCList.AddTail(strResult); 
		}
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		return;
	}

	// â∫à å›ä∑à⁄çsäÆóπ¡™Ø∏
	VERIFY(strEntry.LoadString(IDS_REG_CONVERT));
	UINT nResult = AfxGetApp()->GetProfileInt(strRegKey, strEntry, 0);
	switch ( nResult ) {
	case 0:		// Convert∑∞ñ≥Çµ(êVãK’∞ªﬁ)ÅCF ﬂ◊“∞¿Ç™intå^
	case 1:		// ã@äBèÓïÒÇ™⁄ºﬁΩƒÿÇ…Ç†ÇÈ
		VERIFY(strEntry.LoadString(IDS_REG_NCV_INPUT));
		m_nModal[MODALGROUP3] = AfxGetApp()->GetProfileInt(strRegKey, strEntry, 0);
		VERIFY(strEntry.LoadString(IDS_REG_NCV_MOVE));
		for ( i=0; i<NCXYZ; i++ )
			m_nG0Speed[i] = AfxGetApp()->GetProfileInt(strRegKey, g_szNdelimiter[i]+strEntry, 0);
		VERIFY(strEntry.LoadString(IDS_REG_NCV_FDOT));
		m_nFDot = AfxGetApp()->GetProfileInt(strRegKey, strEntry, 0);
		for ( i=0; i<WORKOFFSET; i++ ) {
			strEntry.Format("G%d", i+54);
			strResult = AfxGetApp()->GetProfileString(strRegKey, strEntry);
			if ( !strResult.IsEmpty() )
				ConvertWorkOffset(i, strResult);
		}
		VERIFY(strEntry.LoadString(IDS_REG_NCV_DEFFEED));
		if ( nResult == 1 ) {
			strResult = AfxGetApp()->GetProfileString(strRegKey, strEntry);
			m_dFeed = strResult.IsEmpty() ? g_dfDOrder[0] : atof(strResult);
		}
		else
			m_dFeed = AfxGetApp()->GetProfileInt(strRegKey, strEntry, (int)g_dfDOrder[0]);
		// Ãß≤ŸÇ÷ÇÃà⁄çsämîF
		Convert();
		break;

	default:	// ã@äBèÓïÒÇÃß≤ŸÇ©ÇÁì«Ç›çûÇ›
		if ( !m_strMCList.IsEmpty() )
			ReadMCoption(m_strMCList.GetHead());
		break;
	}
}

CMCOption::~CMCOption()
{
	int			i;
	POSITION	pos = m_strMCList.GetHeadPosition();
	CString	strRegKey, strEntry, strFmt;
	VERIFY(strRegKey.LoadString(IDS_REGKEY_NC));
	VERIFY(strEntry.LoadString(IDS_REG_NCV_MCFILE));

	// óöóÇÃï€ë∂
	for ( i=0; pos && i<NCMAXMCFILE; i++ ) {
		strFmt.Format(IDS_COMMON_FORMAT, strEntry, i);
		if ( !AfxGetApp()->WriteProfileString(strRegKey, strFmt, m_strMCList.GetNext(pos)) )
			break;
	}

	// à⁄çsäÆóπ∑∞ÇÃèëÇ´çûÇ›
	VERIFY(strEntry.LoadString(IDS_REG_CONVERT));
	AfxGetApp()->WriteProfileInt(strRegKey, strEntry, 2);

	// çHãÔèÓïÒÇÃçÌèú
	for ( pos=m_ltTool.GetHeadPosition(); pos; )
		delete	m_ltTool.GetNext(pos);
}

/////////////////////////////////////////////////////////////////////////////
// “› ﬁä÷êî

void CMCOption::Convert(void)
{
	extern	LPTSTR	g_pszExecDir;
	static	const	int		nRegNCkey[] = {
		IDS_REG_NCV_INPUT, IDS_REG_NCV_DEFFEED, IDS_REG_NCV_FDOT,
		IDS_REG_NCV_FPROCESS
	};

	int		i;
	CString	strRegKey, strEntry, strFile("Init.mnc");
	CRegKey	reg;

	// ⁄ºﬁΩƒÿèÓïÒÇÃämîF
	VERIFY(strRegKey.LoadString(IDS_REGKEY_NC));
	if ( reg.Open(HKEY_CURRENT_USER, gg_szRegKey+strRegKey) != ERROR_SUCCESS )
		return;
	VERIFY(strEntry.LoadString(IDS_REG_NCV_INPUT));
	if ( reg.QueryValue(strEntry, NULL, NULL, NULL) != ERROR_SUCCESS )
		return;

	// Ãß≤ŸÇ÷ÇÃà⁄çsämîF
	if ( AfxMessageBox(IDS_ANA_MCFILESHIFT, MB_YESNO|MB_ICONQUESTION) == IDYES ) {
		if ( ::NCVC_FileDlgCommon(IDS_OPTION_MCSAVE, IDS_MC_FILTER,
					strFile, g_pszExecDir,
					FALSE, OFN_OVERWRITEPROMPT|OFN_HIDEREADONLY) == IDOK ) {
			if ( !SaveMCoption(strFile) )
				AfxMessageBox(IDS_ANA_NOMCFILE, MB_OK|MB_ICONINFORMATION);
		}
		else
			AfxMessageBox(IDS_ANA_NOMCFILE, MB_OK|MB_ICONINFORMATION);
	}
	else
		AfxMessageBox(IDS_ANA_NOMCFILE, MB_OK|MB_ICONINFORMATION);

	// ãåå`éÆÇÃ⁄ºﬁΩƒÿílçÌèú
	for ( i=0; i<SIZEOF(nRegNCkey); i++ ) {
		VERIFY(strEntry.LoadString(nRegNCkey[i]));
		reg.DeleteValue(strEntry);
	}
	VERIFY(strEntry.LoadString(IDS_REG_NCV_MOVE));
	for ( i=0; i<NCXYZ; i++ )
		reg.DeleteValue(g_szNdelimiter[i]+strEntry);
	for ( i=0; i<WORKOFFSET; i++ ) {
		strEntry.Format("G%d", i+54);
		reg.DeleteValue(strEntry);
	}
	reg.Close();
}

BOOL CMCOption::ReadMCoption(LPCTSTR lpszFile, BOOL bHistory/*=TRUE*/)
{
	int		i, j, k;
	CString	strRegKey, strEntry;
	TCHAR	szResult[_MAX_PATH];
/* 
	ì∆é©ÇÃì¸óÕÇÕñ ì|Ç»ÇÃÇ≈
	Win32API ÇÃ GetPrivateProfile[Int|String]() ä÷êîÇégÇ§
*/
	VERIFY(strRegKey.LoadString(IDS_REGKEY_SETTINGS));

	// intå^ñΩóﬂ
	for ( i=0, j=0, k=0; i<MODALGROUP; i++, j++ ) {
		strEntry.Format(g_szNOrder[k], i);
		m_nModal[i] = ::GetPrivateProfileInt(strRegKey, strEntry, g_dfNOrder[j], lpszFile);
	}
	for ( i=0, k++; i<NCXYZ; i++, j++ ) {
		strEntry.Format(g_szNOrder[k], g_szNdelimiter[i]);
		m_nG0Speed[i] = ::GetPrivateProfileInt(strRegKey, strEntry, g_dfNOrder[j], lpszFile);
	}
	for ( k++; j<SIZEOF(m_unNums); j++, k++ ) {
		strEntry = g_szNOrder[k];
		m_unNums[j] = ::GetPrivateProfileInt(strRegKey, strEntry, g_dfNOrder[j], lpszFile);
	}

	// doubleå^ñΩóﬂ
	j= k = 0;
	strEntry = g_szDOrder[k++];
	::GetPrivateProfileString(strRegKey, strEntry, "", szResult, _MAX_PATH, lpszFile);
	m_dFeed = lstrlen(szResult) > 0 ? atof(szResult) : g_dfDOrder[j];
	for ( i=0, j++; i<NCXYZ; i++, j++ ) {
		strEntry.Format(g_szDOrder[k], g_szNdelimiter[i]);
		m_dInitialXYZ[i] = 
			::GetPrivateProfileString(strRegKey, strEntry, "", szResult, _MAX_PATH, lpszFile) > 0 ?
				atof(szResult) : g_dfDOrder[j];
	}
	strEntry = g_szDOrder[k++];
	::GetPrivateProfileString(strRegKey, strEntry, "", szResult, _MAX_PATH, lpszFile);
	m_dBlock = lstrlen(szResult) > 0 ? atof(szResult) : g_dfDOrder[j];
	for ( i=0, j++; i<WORKOFFSET; i++ ) {
		strEntry.Format("G%d", i+54);
		if ( ::GetPrivateProfileString(strRegKey, strEntry, "", szResult, _MAX_PATH, lpszFile) > 0 ) {
			ConvertWorkOffset(i, szResult);
			j += NCXYZ;
		}
		else {
			for ( int ii=0; ii<NCXYZ; ii++, j++ )
				m_dWorkOffset[i][ii] = g_dfDOrder[j];
		}
	}

	// BOOLå^ñΩóﬂ
	for ( i=0; i<SIZEOF(m_ubFlgs); i++ ) {
		strEntry = g_szBOrder[i];
		m_ubFlgs[i] = ::GetPrivateProfileInt(strRegKey, strEntry, g_dfBOrder[i], lpszFile);
	}

	// CStringå^ñΩóﬂ
	for ( i=0; i<MCMACROSTRING/*SIZEOF(g_szSOrderMacro)-1*/; i++ ) {
		::GetPrivateProfileString(strRegKey, g_szSOrderMacro[i], "", szResult, _MAX_PATH, lpszFile);
		m_strMacroOpt[i] = szResult;
	}
	k = 0;
	::GetPrivateProfileString(strRegKey, g_szSOrder[k++], "", szResult, _MAX_PATH, lpszFile);
	m_strMCname = szResult;

	// çHãÔèÓïÒÇàÍíUçÌèú
	for ( POSITION pos=m_ltTool.GetHeadPosition(); pos; )
		delete	m_ltTool.GetNext(pos);
	m_ltTool.RemoveAll();

	// çHãÔèÓïÒ
	CMCTOOLINFO*	pToolInfo;
	CMCTOOLINFO		tool;

	// ñΩóﬂÇï™äÑ
	typedef tokenizer< char_separator<TCHAR> > tokenizer;
	static	char_separator<TCHAR> sep(gg_szComma, "", keep_empty_tokens);
	std::string	str;
	tokenizer	tok( str, sep );
	tokenizer::iterator it;

	try {
		for ( i=0; TRUE; i++ ) {	// Tool¥›ƒÿÇ™ì«ÇﬂÇ»Ç≠Ç»ÇÈÇ‹Ç≈
			strEntry.Format(g_szSOrder[k], i);
			if ( ::GetPrivateProfileString(strRegKey, strEntry, "", szResult, _MAX_PATH, lpszFile) <= 0 )
				break;
			str = szResult;
			tok.assign(str);
			tool.ClearOption();
			for ( j=0, it=tok.begin(); j<MCTOOLINFOOPT && it!=tok.end(); j++, ++it ) {
				switch ( j ) {
				case 0:		// Çsî‘çÜ
					tool.m_nTool = atoi(it->c_str());
					break;
				case 1:		// çHãÔñº
					tool.m_strName = it->c_str();
					break;
				case 2:		// åaï‚ê≥
					tool.m_dToolD = atof(it->c_str());
					break;
				case 3:		// í∑ï‚ê≥
					tool.m_dToolH = atof(it->c_str());
					break;
				}
			}
			pToolInfo = new CMCTOOLINFO(tool);
			m_ltTool.AddTail(pToolInfo);
		}
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		return FALSE;
	}

	// óöóÇÃçXêV
	if ( bHistory )
		return AddMCListHistory(lpszFile);

	return TRUE;
}

BOOL CMCOption::SaveMCoption(LPCTSTR lpszFile)
{
	int		i, j, k;
	CString	strRegKey, strEntry, strResult, strFormat;
	TCHAR	szResult[_MAX_PATH];

	if ( !lpszFile || lstrlen(lpszFile)<=0 )
		return FALSE;
/* 
	ì∆é©ÇÃèoóÕÇÕñ ì|Ç»ÇÃÇ≈
	Win32API ÇÃ WritePrivateProfileString() ä÷êîÇégÇ§
*/
	VERIFY(strRegKey.LoadString(IDS_REGKEY_SETTINGS));

	// intå^ñΩóﬂ
	for ( i=0, j=0, k=0; i<MODALGROUP; i++, j++ ) {
		strEntry.Format(g_szNOrder[k], i);
		strResult.Format("%d", m_nModal[i]);
		if ( !::WritePrivateProfileString(strRegKey, strEntry, strResult, lpszFile) )
			return FALSE;
	}
	for ( i=0, k++; i<NCXYZ; i++, j++ ) {
		strEntry.Format(g_szNOrder[k], g_szNdelimiter[i]);
		strResult.Format("%d", m_nG0Speed[i]);
		if ( !::WritePrivateProfileString(strRegKey, strEntry, strResult, lpszFile) )
			return FALSE;
	}
	for ( k++; j<SIZEOF(m_unNums); j++, k++ ) {
		strEntry = g_szNOrder[k];
		strResult.Format("%d", m_unNums[j]);
		if ( !::WritePrivateProfileString(strRegKey, strEntry, strResult, lpszFile) )
			return FALSE;
	}

	// doubleå^ñΩóﬂ
	k = 0;
	strEntry = g_szDOrder[k++];
	strResult.Format(IDS_MAKENCD_FORMAT, m_dFeed);
	if ( !::WritePrivateProfileString(strRegKey, strEntry, strResult, lpszFile) )
		return FALSE;
	for ( i=0; i<NCXYZ; i++ ) {
		strEntry.Format(g_szDOrder[k], g_szNdelimiter[i]);
		strResult.Format(IDS_MAKENCD_FORMAT, m_dInitialXYZ[i]);
		if ( !::WritePrivateProfileString(strRegKey, strEntry, strResult, lpszFile) )
			return FALSE;
	}
	strEntry = g_szDOrder[k++];
	strResult.Format(IDS_MAKENCD_FORMAT, m_dBlock);
	if ( !::WritePrivateProfileString(strRegKey, strEntry, strResult, lpszFile) )
		return FALSE;
	for ( i=0; i<WORKOFFSET; i++ ) {
		strResult.Empty();
		for ( j=0; j<NCXYZ; j++ ) {
			if ( !strResult.IsEmpty() )
				strResult += gg_szComma;
			strFormat.Format(IDS_MAKENCD_FORMAT, m_dWorkOffset[i][j]);
			strResult += strFormat;
		}
		strEntry.Format("G%d", i+54);
		if ( !::WritePrivateProfileString(strRegKey, strEntry, strResult, lpszFile) )
			return FALSE;
	}

	// BOOLå^ñΩóﬂ
	for ( i=0; i<SIZEOF(m_ubFlgs); i++ ) {
		strEntry = g_szBOrder[i];
		strResult.Format("%d", m_ubFlgs[i]);
		if ( !::WritePrivateProfileString(strRegKey, strEntry, strResult, lpszFile) )
			return FALSE;
	}

	// CStringå^ñΩóﬂ
	for ( i=0; i<MCMACROSTRING/*SIZEOF(g_szSOrderMacro)-1*/; i++ ) {
		if ( !::WritePrivateProfileString(strRegKey, g_szSOrderMacro[i], m_strMacroOpt[i], lpszFile) )
			return FALSE;
	}
	k = 0;
	if ( !::WritePrivateProfileString(strRegKey, g_szSOrder[k++], m_strMCname, lpszFile) )
		return FALSE;

	// çHãÔèÓïÒÇàÍíUçÌèú(ì«Ç›çûÇ›åèêîämîFå„)
	for ( j=0; TRUE; j++ ) {	// Tool¥›ƒÿÇ™ì«ÇﬂÇ»Ç≠Ç»ÇÈÇ‹Ç≈
		strEntry.Format(g_szSOrder[k], j);
		if ( ::GetPrivateProfileString(strRegKey, strEntry, "", szResult, _MAX_PATH, lpszFile) <= 0 )
			break;
	}
	for ( i=0; i<j; i++ ) {
		strEntry.Format(g_szSOrder[k], i);
		::WritePrivateProfileString(strRegKey, strEntry, NULL, lpszFile);
	}
	// çHãÔèÓïÒ
	POSITION	pos = m_ltTool.GetHeadPosition();
	CMCTOOLINFO*	pToolInfo;
	for ( i=0; pos; i++ ) {
		pToolInfo = m_ltTool.GetNext(pos);
		strEntry.Format(g_szSOrder[k], i);
		strResult.Format("%d", pToolInfo->m_nTool);
		strResult += gg_szComma + pToolInfo->m_strName;
		strFormat.Format(IDS_MAKENCD_FORMAT, pToolInfo->m_dToolD);
		strResult += gg_szComma + strFormat;
		strFormat.Format(IDS_MAKENCD_FORMAT, pToolInfo->m_dToolH);
		strResult += gg_szComma + strFormat;
		if ( !::WritePrivateProfileString(strRegKey, strEntry, strResult, lpszFile) )
			return FALSE;
		// âºìoò^Ã◊∏ﬁèâä˙âª
		pToolInfo->m_bDlgAdd = pToolInfo->m_bDlgDel = FALSE;
	}

	// óöóÇÃçXêV
	return AddMCListHistory(lpszFile);
}

void CMCOption::ConvertWorkOffset(size_t n, LPCTSTR lpszResult)
{
	ASSERT( n>=0 && n<WORKOFFSET );
	LPTSTR	lpsztok, lpszcontext, lpszBuf = NULL;
	int		i;

	for ( i=0; i<NCXYZ; i++ )
		m_dWorkOffset[n][i] = 0;

	try {
		lpszBuf = new TCHAR[lstrlen(lpszResult)+1];
		lpsztok = strtok_s(lstrcpy(lpszBuf, lpszResult), gg_szComma, &lpszcontext);
		for ( i=0; i<NCXYZ && lpsztok; i++ ) {
			m_dWorkOffset[n][i] = atof(lpsztok);
			lpsztok = strtok_s(NULL, gg_szComma, &lpszcontext);
		}
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
	}
	if ( lpszBuf )
		delete[]	lpszBuf;
}

BOOL CMCOption::AddMCListHistory(LPCTSTR lpszSearch)
{
	if ( !lpszSearch || lstrlen(lpszSearch) <= 0 )
		return TRUE;
	if ( !m_strMCList.IsEmpty() &&
			m_strMCList.GetHead().CompareNoCase(lpszSearch) == 0 )
		return TRUE;

	// ï∂éöóÒÇÃåüçı(FindÇÕëÂï∂éöè¨ï∂éöÇãÊï Ç∑ÇÈÇÃÇ≈égÇ¶Ç»Ç¢)
	try {
		POSITION	pos1, pos2;
		for ( pos1=m_strMCList.GetHeadPosition(); (pos2 = pos1); ) {
			if ( m_strMCList.GetNext(pos1).CompareNoCase(lpszSearch) == 0 ) {
				// ï∂éöóÒÇ™Ç†ÇÍÇŒÅCÇªÇÍÇè¡ÇµÇƒêÊì™Ç÷
				m_strMCList.RemoveAt(pos2);
				m_strMCList.AddHead(lpszSearch);
				return TRUE;
			}
		}
		// Ç»ÇØÇÍÇŒêÊì™Ç…í«â¡
		m_strMCList.AddHead(lpszSearch);
		// 10å¬âzÇ¶ÇÍÇŒç≈å„Çè¡ãé
		if ( m_strMCList.GetCount() > NCMAXMCFILE )
			m_strMCList.RemoveTail();
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		return FALSE;
	}

	return TRUE;
}

optional<double> CMCOption::GetToolD(int nTool) const
{
	CMCTOOLINFO*	pTool;
	for ( POSITION pos=m_ltTool.GetHeadPosition(); pos; ) {
		pTool = m_ltTool.GetNext(pos);
		if ( pTool->m_nTool == nTool )
			return pTool->m_dToolD;
	}
	return optional<double>();
}

optional<double> CMCOption::GetToolH(int nTool) const
{
	CMCTOOLINFO*	pTool;
	for ( POSITION pos=m_ltTool.GetHeadPosition(); pos; ) {
		pTool = m_ltTool.GetNext(pos);
		if ( pTool->m_nTool == nTool )
			return pTool->m_dToolH;
	}
	return optional<double>();
}

BOOL CMCOption::AddTool(int nTool, double d, BOOL bAbs)
{
	CMCTOOLINFO*	pTool;
	BOOL			bMatch = FALSE, bResult = TRUE;

	// ë∂ç›¡™Ø∏
	for ( POSITION pos=m_ltTool.GetHeadPosition(); pos; ) {
		pTool = m_ltTool.GetNext(pos);
		if ( pTool->m_nTool == nTool ) {
			bMatch = TRUE;
			break;
		}
	}
	if ( bMatch && !bAbs )
		d += pTool->m_dToolD;	// ≤›∏ÿ“›¿Ÿ

	// ï‚ê≥ílâºìoò^
	CMCTOOLINFO		tool;
	tool.ClearOption();
	tool.m_nTool   = nTool;
	tool.m_dToolD  = d;
	tool.m_bDlgAdd = TRUE;		// âºìoò^œ∞∏
	try {
		pTool = new CMCTOOLINFO(tool);
		m_ltTool.AddHead(pTool);	// êÊÇ…åüçıÇ≥ÇÍÇÈÇÊÇ§êÊì™Ç…ìoò^
	}
	catch (CMemoryException* e) {
		e->Delete();
		bResult = FALSE;
	}

	return bResult;
}

void CMCOption::ReductionTools(BOOL bAdd)
{
	CMCTOOLINFO*	pToolInfo;
	POSITION		pos1, pos2;

	// ¿ﬁ≤±€∏ﬁìôÇ≈âºìoò^Ç≥ÇÍÇΩçHãÔèÓïÒÇêÆóù(çÌèú)
	for ( pos1=m_ltTool.GetHeadPosition(); (pos2 = pos1); ) {
		pToolInfo = m_ltTool.GetNext(pos1);
		BOOL&	bFlg = bAdd ? pToolInfo->m_bDlgAdd : pToolInfo->m_bDlgDel;
		if ( bFlg ) {
			delete	pToolInfo;
			m_ltTool.RemoveAt(pos2);
		}
	}
}

void CMCOption::AddMCHistory_ComboBox(CComboBox& combo)
{
	static	LPCTSTR	ss_lpszRefer = "éQè∆...";

	combo.ResetContent();

	// ∫›ŒﬁŒﬁØ∏ΩÇ…ã@äBèÓïÒÇÃóöóÇí«â¡
	CString	strPath, strFile;
	for ( POSITION pos=m_strMCList.GetHeadPosition(); pos; ) {
		::Path_Name_From_FullPath(m_strMCList.GetNext(pos), strPath, strFile);
		combo.AddString( strFile );
	}
	combo.AddString( ss_lpszRefer );

	combo.SetCurSel( combo.GetCount() > 1 ? 0 : -1 );
}
/*
	--- à»â∫ MainFrm.cpp ÇéQçl
*/
CString	CMCOption::MakeMacroCommand(int a) const
{
	CString	strResult;
	ASSERT( a>=0 && a<SIZEOF(g_szSOrderMacro) );
	if ( a>=0 && a<SIZEOF(g_szSOrderMacro) ) {
		strResult  = "${";
		strResult += g_szSOrderMacro[a];
		strResult += "}";
	}
	return strResult;
}

CString CMCOption::GetDefaultOption(void) const
{
	CString	strResult;
	for ( int i=0; i<SIZEOF(g_nDefaultMacroID); i++ ) {
		if ( !strResult.IsEmpty() )
			strResult += " ";
		strResult += MakeMacroCommand(g_nDefaultMacroID[i]);
	}
	return strResult;
}
