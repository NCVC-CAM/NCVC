// MCOption.cpp: CMCOption �N���X�̃C���v�������e�[�V����
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
// �@�B��񗚗��T�C�Y
#define	NCMAXMCFILE		10

// ڼ޽�؁EINI̧�ق̈ڍs����
static	const	UINT	g_nConvert = 2;

// int�^����
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

// double�^����
static	LPCTSTR	g_szDOrder[] = {
	"DefaultFeed",
	"Initial%c",
	"BlockTime",
};
static	const	double	g_dfDOrder[] = {
	30.0,
	0.0, 0.0, 10.0,
	0.0,
	// G54�`G59
	0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
	0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0
};

// BOOL�^����
static	LPCTSTR	g_szBOrder[] = {
	"Lathe", "L0Cycle"
};
static	const	BOOL	g_dfBOrder[] = {
	FALSE, FALSE
};

// CString�^����
static	LPCTSTR	g_szSOrder[] = {
	"Name",
	"AutoBreak",
	"Tool%d"
};
static	LPCTSTR	g_szSOrderMacro[] = {
	"MacroCode", "MacroFolder", "MacroIF", "MacroArgv",
	"MacroResult", "MachineFile", "CurrentFolder"	// �u�������݂̂Ɏg�p
};
extern	const	int		g_nDefaultMacroID[] = {
	MCMACHINEFILE, MCMACROCODE, MCMACROFOLDER, MCCURRENTFOLDER, MCMACRORESULT
};

/////////////////////////////////////////////////////////////////////////////
// CMCOption �N���X�̍\�z/����

CMCOption::CMCOption()
{
	CString	strRegKey, strEntry, strResult, strFmt;
	int		i;

	// ���ޕϐ��̏�����
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

	// �@�B��񗚗�(�t���œǂݍ���)
	VERIFY(strEntry.LoadString(IDS_REG_NCV_MCFILE));
	for ( i=NCMAXMCFILE-1; i>=0; i-- ) {
		strFmt.Format(IDS_COMMON_FORMAT, strEntry, i);
		strResult = AfxGetApp()->GetProfileString(strRegKey, strFmt);
		// ̧�ق����݂���Ƃ����������ɓo�^
		if ( !strResult.IsEmpty() && ::IsFileExist(strResult, TRUE, FALSE) )
			AddMCListHistory(strResult);
	}

	// ���ʌ݊��ڍs��������
	VERIFY(strEntry.LoadString(IDS_REG_CONVERT));
	UINT nResult = AfxGetApp()->GetProfileInt(strRegKey, strEntry, 0);
	if ( nResult < g_nConvert ) {
		// 0:Convert������(�V�Kհ��)�CF���Ұ���int�^
		// 1:�@�B���ڼ޽�؂ɂ���
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
		// ̧�قւ̈ڍs�m�F
		Convert();
	}

	// �@�B����̧�ق���ǂݍ���(ڼ޽�؏����㏑��)
	if ( !m_strMCList.IsEmpty() )
		ReadMCoption(m_strMCList.GetHead());
}

CMCOption::~CMCOption()
{
	int			i;
	POSITION	pos = m_strMCList.GetHeadPosition();
	CString	strRegKey, strEntry, strFmt;
	VERIFY(strRegKey.LoadString(IDS_REGKEY_NC));
	VERIFY(strEntry.LoadString(IDS_REG_NCV_MCFILE));

	// �����̕ۑ�
	for ( i=0; i<NCMAXMCFILE; i++ ) {
		strFmt.Format(IDS_COMMON_FORMAT, strEntry, i);
		AfxGetApp()->WriteProfileString(strRegKey, strFmt, NULL);	// ��ɗ����폜
	}
	for ( i=0; pos && i<NCMAXMCFILE; i++ ) {
		strFmt.Format(IDS_COMMON_FORMAT, strEntry, i);
		if ( !AfxGetApp()->WriteProfileString(strRegKey, strFmt, m_strMCList.GetNext(pos)) )
			break;
	}

	// �ڍs�������̏�������
	VERIFY(strEntry.LoadString(IDS_REG_CONVERT));
	AfxGetApp()->WriteProfileInt(strRegKey, strEntry, g_nConvert);

	// �H����̍폜
	for ( pos=m_ltTool.GetHeadPosition(); pos; )
		delete	m_ltTool.GetNext(pos);
}

/////////////////////////////////////////////////////////////////////////////
// ���ފ֐�

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

	// ڼ޽�؏��̊m�F
	VERIFY(strRegKey.LoadString(IDS_REGKEY_NC));
	if ( reg.Open(HKEY_CURRENT_USER, gg_szRegKey+strRegKey) != ERROR_SUCCESS )
		return;
	VERIFY(strEntry.LoadString(IDS_REG_NCV_INPUT));
	if ( reg.QueryValue(strEntry, NULL, NULL, NULL) != ERROR_SUCCESS )
		return;

	// ̧�قւ̈ڍs�m�F
	if ( AfxMessageBox(IDS_ANA_MCFILESHIFT, MB_YESNO|MB_ICONQUESTION) == IDYES ) {
		if ( ::NCVC_FileDlgCommon(IDS_OPTION_MCSAVE, IDS_MC_FILTER, FALSE, 
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

	// ���`����ڼ޽�ؒl�폜
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
	�Ǝ��̓��͖͂ʓ|�Ȃ̂�
	Win32API �� GetPrivateProfile[Int|String]() �֐����g��
*/
	VERIFY(strRegKey.LoadString(IDS_REGKEY_SETTINGS));

	// int�^����
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

	// double�^����
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
	strEntry = g_szDOrder[++k];
	::GetPrivateProfileString(strRegKey, strEntry, "", szResult, _MAX_PATH, lpszFile);
	m_dBlock = lstrlen(szResult) > 0 ? atof(szResult) : g_dfDOrder[j];
	// ----------
		// �`Ver1.72�܂ł��޸�����
		::GetPrivateProfileString(strRegKey, g_szDOrder[1], "", szResult, _MAX_PATH, lpszFile);
		if ( lstrlen(szResult) > 0 ) {
			// "BlockTime" �� "Initial%c" �ɏ�������ł����޸�
			m_dBlock = atof(szResult);
			// ���������ŏo��
			// �@���P�Ȃ�؂�ւ��ł� SaveMCoption() ���Ă΂�Ȃ��̂�
			// �@�@�����ŏ�������
			CString	strResult;
			strResult.Format(IDS_MAKENCD_FORMAT, m_dBlock);
			::WritePrivateProfileString(strRegKey, g_szDOrder[2], strResult, lpszFile);
			// "Initial%c" �̴��؂��폜
			::WritePrivateProfileString(strRegKey, g_szDOrder[1], NULL, lpszFile);
		}
	// ----------
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

	// BOOL�^����
	for ( i=0; i<SIZEOF(m_ubFlgs); i++ ) {
		strEntry = g_szBOrder[i];
		m_ubFlgs[i] = ::GetPrivateProfileInt(strRegKey, strEntry, g_dfBOrder[i], lpszFile);
	}

	// CString�^����
	for ( i=0; i<MCMACROSTRING/*SIZEOF(g_szSOrderMacro)-1*/; i++ ) {
		::GetPrivateProfileString(strRegKey, g_szSOrderMacro[i], "", szResult, _MAX_PATH, lpszFile);
		m_strMacroOpt[i] = szResult;
	}
	k = 0;
	::GetPrivateProfileString(strRegKey, g_szSOrder[k++], "", szResult, _MAX_PATH, lpszFile);
	m_strMCname = szResult;
	::GetPrivateProfileString(strRegKey, g_szSOrder[k++], "", szResult, _MAX_PATH, lpszFile);
	m_strAutoBreak = szResult;

	// �H�������U�폜
	for ( POSITION pos=m_ltTool.GetHeadPosition(); pos; )
		delete	m_ltTool.GetNext(pos);
	m_ltTool.RemoveAll();

	// �H����
	CMCTOOLINFO*	pToolInfo;
	CMCTOOLINFO		tool;

	// ���߂𕪊�
	typedef tokenizer< char_separator<TCHAR> > tokenizer;
	static	char_separator<TCHAR> sep(gg_szComma, "", keep_empty_tokens);
	std::string	str;
	tokenizer	tok( str, sep );
	tokenizer::iterator it;

	try {
		for ( i=0; TRUE; i++ ) {	// Tool���؂��ǂ߂Ȃ��Ȃ�܂�
			strEntry.Format(g_szSOrder[k], i);
			if ( ::GetPrivateProfileString(strRegKey, strEntry, "", szResult, _MAX_PATH, lpszFile) <= 0 )
				break;
			str = szResult;
			tok.assign(str);
			tool.ClearOption();
			for ( j=0, it=tok.begin(); j<MCTOOLINFOOPT && it!=tok.end(); j++, ++it ) {
				switch ( j ) {
				case 0:		// �s�ԍ�
					tool.m_nTool = atoi(it->c_str());
					break;
				case 1:		// �H�
					tool.m_strName = it->c_str();
					break;
				case 2:		// �a�␳
					tool.m_dToolD = atof(it->c_str());
					break;
				case 3:		// ���␳
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

	// �����̍X�V
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
	�Ǝ��̏o�͖͂ʓ|�Ȃ̂�
	Win32API �� WritePrivateProfileString() �֐����g��
*/
	VERIFY(strRegKey.LoadString(IDS_REGKEY_SETTINGS));

	// int�^����
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

	// double�^����
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
	strEntry = g_szDOrder[++k];
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

	// BOOL�^����
	for ( i=0; i<SIZEOF(m_ubFlgs); i++ ) {
		strEntry = g_szBOrder[i];
		strResult.Format("%d", m_ubFlgs[i]);
		if ( !::WritePrivateProfileString(strRegKey, strEntry, strResult, lpszFile) )
			return FALSE;
	}

	// CString�^����
	for ( i=0; i<MCMACROSTRING/*SIZEOF(g_szSOrderMacro)-1*/; i++ ) {
		if ( !::WritePrivateProfileString(strRegKey, g_szSOrderMacro[i], m_strMacroOpt[i], lpszFile) )
			return FALSE;
	}
	k = 0;
	if ( !::WritePrivateProfileString(strRegKey, g_szSOrder[k++], m_strMCname, lpszFile) )
		return FALSE;
	if ( !::WritePrivateProfileString(strRegKey, g_szSOrder[k++], m_strAutoBreak, lpszFile) )
		return FALSE;

	// �H�������U�폜(�ǂݍ��݌����m�F��)
	for ( j=0; TRUE; j++ ) {	// Tool���؂��ǂ߂Ȃ��Ȃ�܂�
		strEntry.Format(g_szSOrder[k], j);
		if ( ::GetPrivateProfileString(strRegKey, strEntry, "", szResult, _MAX_PATH, lpszFile) <= 0 )
			break;
	}
	for ( i=0; i<j; i++ ) {
		strEntry.Format(g_szSOrder[k], i);
		::WritePrivateProfileString(strRegKey, strEntry, NULL, lpszFile);
	}
	// �H����
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
		// ���o�^�׸ޏ�����
		pToolInfo->m_bDlgAdd = pToolInfo->m_bDlgDel = FALSE;
	}

	// �����̍X�V
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

	// ������̌���(Find�͑啶������������ʂ���̂Ŏg���Ȃ�)
	try {
		POSITION	pos1, pos2;
		for ( pos1=m_strMCList.GetHeadPosition(); (pos2 = pos1); ) {
			if ( m_strMCList.GetNext(pos1).CompareNoCase(lpszSearch) == 0 ) {
				// �����񂪂���΁C����������Đ擪��
				m_strMCList.RemoveAt(pos2);
				m_strMCList.AddHead(lpszSearch);
				return TRUE;
			}
		}
		// �Ȃ���ΐ擪�ɒǉ�
		m_strMCList.AddHead(lpszSearch);
		// 10�z����΍Ō������
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

	// ��������
	for ( POSITION pos=m_ltTool.GetHeadPosition(); pos; ) {
		pTool = m_ltTool.GetNext(pos);
		if ( pTool->m_nTool == nTool ) {
			bMatch = TRUE;
			break;
		}
	}
	if ( bMatch && !bAbs )
		d += pTool->m_dToolD;	// �ݸ�����

	// �␳�l���o�^
	CMCTOOLINFO		tool;
	tool.ClearOption();
	tool.m_nTool   = nTool;
	tool.m_dToolD  = d;
	tool.m_bDlgAdd = TRUE;		// ���o�^ϰ�
	try {
		pTool = new CMCTOOLINFO(tool);
		m_ltTool.AddHead(pTool);	// ��Ɍ��������悤�擪�ɓo�^
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

	// �޲�۸ޓ��ŉ��o�^���ꂽ�H����𐮗�(�폜)
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
	static	LPCTSTR	ss_lpszRefer = "�Q��...";

	combo.ResetContent();

	// �����ޯ���ɋ@�B���̗�����ǉ�
	CString	strPath, strFile;
	for ( POSITION pos=m_strMCList.GetHeadPosition(); pos; ) {
		::Path_Name_From_FullPath(m_strMCList.GetNext(pos), strPath, strFile);
		combo.AddString( strFile );
	}
	combo.AddString( ss_lpszRefer );

	combo.SetCurSel( combo.GetCount() > 1 ? 0 : -1 );
}
/*
	--- �ȉ� MainFrm.cpp ���Q�l
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
