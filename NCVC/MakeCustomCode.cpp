// ¶½ÀÑÍ¯ÀŞ°, Ì¯ÀŞ°ˆ—‚ÌŠî’ê¸×½
//	from TH_MakeNCD.cpp TH_MakeLathe.cpp
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NCVC.h"
#include "DXFdata.h"
#include "DXFDoc.h"
#include "NCMakeOption.h"
#include "MakeCustomCode.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

using namespace std;
using namespace boost;

static	LPCTSTR	g_szCustomCode[] = {
	"MakeUser", "MakeDate", "MakeTime", "MakeNCD", "MakeDXF", "MakeCondition"
};

CMakeCustomCode::CMakeCustomCode
	(const CDXFDoc* pDoc, const CDXFdata* pData, const CNCMakeOption* pMakeOpt)
{
	m_pDoc		= pDoc;
	m_pData		= pData;
	m_pMakeOpt	= pMakeOpt;
	// ‹¤’Ê·°Ü°ÄŞ‚ğ“o˜^
	m_strOrderIndex.SetElement(SIZEOF(g_szCustomCode), g_szCustomCode);
}

tuple<int, CString>	CMakeCustomCode::ReplaceCustomCode(const string& str) const
{
	static	LPCTSTR	szReplaceErr = "???";

	int		nTestCode = m_strOrderIndex.GetIndex(str.substr(1, str.length()-2).c_str());
	CString	strResult, strPath, strFile;
	TCHAR	szUserName[_MAX_PATH];
	DWORD	dwResult;
	CTime	time;

	// replace
	switch ( nTestCode ) {
	case 0:		// MakeUser
		dwResult = _MAX_PATH;
		// Õ°»Ş–¼‚ÉŠ¿š‚ªŠÜ‚Ü‚ê‚Ä‚¢‚é‚Æ¶¬‚µ‚È‚¢
		strResult = ::GetUserName(szUserName, &dwResult) && IsNCchar(szUserName) ?
			szUserName : szReplaceErr;
		break;
	case 1:		// MakeDate
		time = CTime::GetCurrentTime();
		VERIFY(strPath.LoadString(ID_INDICATOR_DATE_F2));	// %y/%m/%d
		strResult = time.Format(strPath);
		break;
	case 2:		// MakeTime
		time = CTime::GetCurrentTime();
		VERIFY(strPath.LoadString(ID_INDICATOR_TIME_F));	// %H:%M
		strResult = time.Format(strPath);
		break;
	case 3:		// MakeNCD
		::Path_Name_From_FullPath(m_pDoc->GetNCFileName(), strPath, strFile);
		strResult = IsNCchar(strFile) ? strFile : szReplaceErr;
		break;
	case 4:		// MakeDXF
		::Path_Name_From_FullPath(m_pDoc->GetPathName(), strPath, strFile);	// GetTitle()‚É‚Í"*"‚ª•t‚­‰Â”\«‚ ‚è
		strResult = IsNCchar(strFile) ? strFile : szReplaceErr;
		break;
	case 5:		// MakeCondition
		::Path_Name_From_FullPath(m_pMakeOpt->GetInitFile(), strPath, strFile);
		strResult = IsNCchar(strFile) ? strFile : szReplaceErr;
		break;
	}

	return make_tuple(nTestCode - SIZEOF(g_szCustomCode), strResult);	// ‹¤’Ê·°Ü°ÄŞ•ª‚ğŒ¸Z
}

BOOL CMakeCustomCode::IsNCchar(LPCTSTR lpsz) const
{
	for ( int i=0; i<lstrlen(lpsz); i++ ) {
//		if ( isprint(lpsz[i]) == 0 )
		if ( ::IsDBCSLeadByte(lpsz[i]) )
			return FALSE;
	}
	return TRUE;
}
