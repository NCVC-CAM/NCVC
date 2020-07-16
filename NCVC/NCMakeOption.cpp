// NCMakeOption.cpp: CNCMakeOption �N���X�̃C���v�������e�[�V����
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "NCMakeOption.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif
//#define	_DEBUGOLD
#undef	_DEBUGOLD

// int�^����
static	LPCTSTR	g_szNOrder[] = {
	"Spindle",
	"LineAddType", "G90", "ZReturn",
		"Dot", "FDot", "CircleCode", "CircleIJ",
	"MakeEnd", "FinishSpindle",
		"DeepZProcess", "DeepAProcess", "DeepCProcess",
	"DrillSpindle", "Dwell", "DwellFormat", "DrillZProcess",
		"DrillProcess", "DrillSort", "DrillCircleProcess",
	"MoveZ",
	"ToleranceProcess", "DrillOptimaize"
};
static	const	int		g_dfNOrder[] = {
	3000,
	1, 0, 1, 0, 2, 0, 1,
	0, 8000, 0, 0, 0,
	3000, 10, 0, 0, 0, 0, 0,
	0,
	0, 0
};

// double�^����
static	LPCTSTR	g_szDOrder[] = {
	"Feed", "ZFeed",
	"ZG0Stop", "ZCut", "G92X", "G92Y", "G92Z",
	"Ellipse",
	"MakeEndValue", "MakeEndFeed", "DeepFinal", "ZStep", "FinishFeed",
	"DrillFeed", "DrillR", "DrillZ", "DrillCircleR",
	"Tolerance", "DrillMargin"
};
static	const	double	g_dfDOrder[] = {
	300.0, 100.0,
	-9.0, -12.0, 0.0, 0.0, 0.0,
	0.5,
	0.0, 1000.0, -20.0, -2.0, 100.0,
	60.0, -9.0, -12.0, 10.0,
	NCMIN, 1.0
};

// BOOL�^����
static	LPCTSTR	g_szBOrder[] = {
	"XRev", "YRev", "LineAdd", "ZeroCut", "GClip", "DisableSpindle",
	"CircleHalf", "EllipseFlg",
	"Deep", "DeepFinishSet",
	"DrillMatch", "DrillCircle", "DrillBreak",
	"LayerComment"
};
static	const	BOOL	g_dfBOrder[] = {
	FALSE, FALSE, FALSE, TRUE, TRUE, FALSE,
	FALSE, TRUE,
	FALSE, TRUE,
	TRUE, FALSE, TRUE,
	TRUE
};

// CString�^����
static	LPCTSTR	g_szSOrder[] = {
	"LineForm", "EOB", "CustomMoveB", "CustomMoveA",
	"Header", "Footer"
};
static	LPCTSTR	g_dfSOrder[] = {
	"N%04d", "", "", "",
	"Header.txt", "Footer.txt"
};

// �ۑ��Ɋւ�����
static	LPCTSTR	g_szInitComment[] = {
	"##\n",
	"## NCVC:�؍����̧��\n",
};
static	enum	ENORDERTYPE {	// ���߂̌^
	NC_PAGE,	// �߰�ދ�؂�
	NC_NUM, NC_DBL, NC_FLG, NC_STR
};
typedef	struct	tagSAVEORDER {
	ENORDERTYPE		enType;
	int				nID;
	LPCTSTR			lpszComment;
} SAVEORDER;
static	SAVEORDER	g_stSaveOrder[] = {
	{NC_PAGE,	1},		// Page1(��{:Dialog1)
	{NC_NUM,	MKNC_NUM_SPINDLE,		"�厲��]��"},
	{NC_DBL,	MKNC_DBL_FEED,			"�؍푗��"},
	{NC_DBL,	MKNC_DBL_ZFEED,			"Z������"},
	{NC_DBL,	MKNC_DBL_ZG0STOP,		"R�_"},
	{NC_DBL,	MKNC_DBL_ZCUT,			"Z���؂荞��"},
	{NC_DBL,	MKNC_DBL_G92X,			"�؍팴�_(G92X)"},
	{NC_DBL,	MKNC_DBL_G92Y,			"�؍팴�_(G92Y)"},
	{NC_DBL,	MKNC_DBL_G92Z,			"�؍팴�_(G92Z)"},
	{NC_STR,	MKNC_STR_HEADER,		"����ͯ�ް"},
	{NC_STR,	MKNC_STR_FOOTER,		"����̯�ް"},
	{NC_PAGE,	2},		// Page2(����:Dialog2)
	{NC_FLG,	MKNC_FLG_XREV,			"X�����]"},
	{NC_FLG,	MKNC_FLG_YREV,			"Y�����]"},
	{NC_FLG,	MKNC_FLG_LINEADD,		"�s�ԍ��ǉ�"},
	{NC_STR,	MKNC_STR_LINEFORM,		"�s�ԍ�����"},
	{NC_NUM,	MKNC_NUM_LINEADD,		"�s�ԍ��{��"},
	{NC_STR,	MKNC_STR_EOB,			"EOB"},
	{NC_NUM,	MKNC_NUM_G90,			"�ʒu�w��(0:G90,1:G91)"},
	{NC_NUM,	MKNC_NUM_ZRETURN,		"Z�����A(0:�Ƽ��,1:R�_)"},
	{NC_FLG,	MKNC_FLG_GCLIP,			"Ӱ���"},
	{NC_FLG,	MKNC_FLG_DISABLESPINDLE,"S���Ұ��𐶐����Ȃ�"},
	{NC_PAGE,	3},		// Page3(�\�L:Dialog6)
	{NC_NUM,	MKNC_NUM_DOT,			"���W�\�L(0:�����_,1:1/1000)"},
	{NC_NUM,	MKNC_NUM_FDOT,			"F���Ұ��\�L(0:�����_,1:1/1000,2:����)"},
	{NC_FLG,	MKNC_FLG_ZEROCUT,		"�����_�ȉ��̾�۶��"},
	{NC_NUM,	MKNC_NUM_CIRCLECODE,	"�~�ް��̐؍�(0:G2,1:G3)"},
	{NC_NUM,	MKNC_NUM_IJ,			"�~�ʎw��(0:R,1:I/J)"},
	{NC_FLG,	MKNC_FLG_CIRCLEHALF,	"�S�~��2����"},
	{NC_DBL,	MKNC_DBL_ELLIPSE,		"�ȉ~����"},
	{NC_FLG,	MKNC_FLG_ELLIPSE,		"���a�ƒZ�a���������ȉ~�͉~�Ƃ݂Ȃ�"},
	{NC_PAGE,	4},		// Page4(�[��:Dialog3)
	{NC_NUM,	MKNC_NUM_MAKEEND,		"���H�ςݐ[���̎w��(0:�Ȃ�,1:�̾��,2:�Œ�Z)"},
	{NC_DBL,	MKNC_DBL_MAKEEND,		"�̾�Ēl or �Œ�Z�l"},
	{NC_DBL,	MKNC_DBL_MAKEENDFEED,	"���H�ςݐ[����Z���葬�x"},
	{NC_FLG,	MKNC_FLG_DEEP,			"�[�����s��"},
	{NC_DBL,	MKNC_DBL_DEEP,			"�ŏI�؂荞��"},
	{NC_DBL,	MKNC_DBL_ZSTEP,			"Z���؂荞�ݽï��"},
	{NC_NUM,	MKNC_NUM_DEEPAPROCESS,	"�[���؍�菇(0:�S��,1:��M)"},
	{NC_NUM,	MKNC_NUM_DEEPCPROCESS,	"�[���؍����(0:����,1:���)"},
	{NC_NUM,	MKNC_NUM_DEEPZPROCESS,	"R�_�ւ�Z�����A(0:������,1:�؍푗��)"},
	{NC_FLG,	MKNC_FLG_DEEPFINISH,	"�ŏIZ�l�d�グ�K�p"},
	{NC_NUM,	MKNC_NUM_DEEPSPINDLE,	"�d�グ��]��"},
	{NC_DBL,	MKNC_DBL_DEEPFEED,		"�d�グ����"},
	{NC_PAGE,	5},		// Page5(�����H:Dialog4)
	{NC_NUM,	MKNC_NUM_DRILLSPINDLE,	"�����H��]��"},
	{NC_DBL,	MKNC_DBL_DRILLFEED,		"�����H����"},
	{NC_DBL,	MKNC_DBL_DRILLR,		"�����HR�_"},
	{NC_DBL,	MKNC_DBL_DRILLZ,		"�����H�؂荞��"},
	{NC_FLG,	MKNC_FLG_DRILLMATCH,	"�����H������W����"},
	{NC_NUM,	MKNC_NUM_DWELL,			"�޳�َ���"},
	{NC_NUM,	MKNC_NUM_DWELLFORMAT,	"�޳�َ��ԕ\�L(0:�����_,1:����)"},
	{NC_NUM,	MKNC_NUM_DRILLPROCESS,	"�����H�菇(0:��,1:��,2:�̂�)"},
	{NC_NUM,	MKNC_NUM_DRILLZPROCESS,	"Z�����A(0:G81|G82,1:G85|G89)"},
	{NC_FLG,	MKNC_FLG_DRILLCIRCLE,	"�~�ް��������H"},
	{NC_DBL,	MKNC_DBL_DRILLCIRCLE,	"�Ώ۔��a"},
	{NC_NUM,	MKNC_NUM_DRILLSORT,		"��ٰ��ݸ�(0:����,1:�~��)"},
	{NC_FLG,	MKNC_FLG_DRILLBREAK,	"�傫�����Ƃɺ��Ă𖄂ߍ���"},
	{NC_NUM,	MKNC_NUM_DRILLCIRCLEPROCESS,	"���_�Ɖ~�ް��Ƃ̎菇(0:����,1:��,2:��)"},
	{NC_PAGE,	6},		// Page6(ڲ�:Dialog8)
	{NC_FLG,	MKNC_FLG_LAYERCOMMENT,	"ڲԂ��Ƃɺ��Ă𖄂ߍ���"},
	{NC_NUM,	MKNC_NUM_MOVEZ,			"�ړ�ڲԂ�Z��(0:���̂܂�,1:R�_,2:�Ƽ�ٓ_)"},
	{NC_STR,	MKNC_STR_CUSTMOVE_B,	"���шړ�����(�O)"},
	{NC_STR,	MKNC_STR_CUSTMOVE_A,	"���шړ�����(��)"},
	{NC_PAGE,	7},		// Page7(�œK��:Dialog5)
	{NC_DBL,	MKNC_DBL_TOLERANCE,		"������W�ƌ��Ȃ����e��"},
	{NC_NUM,	MKNC_NUM_TOLERANCE,		"���������̓���(0:Z�㏸G0�ړ�,1:G1���)"},
	{NC_NUM,	MKNC_NUM_OPTIMAIZEDRILL,"�����H���(0:�Ȃ�,1:X,2:Y)"},
	{NC_DBL,	MKNC_DBL_DRILLMARGIN,	"�����H���ꎲ��ƌ��Ȃ����e��"}
};

/////////////////////////////////////////////////////////////////////////////
// CNCMakeOption �N���X�̍\�z/����

CNCMakeOption::CNCMakeOption(LPCTSTR lpszInit)
{
	int		i, nLen;

	ASSERT( SIZEOF(g_szNOrder) == SIZEOF(g_dfNOrder) );
	ASSERT( SIZEOF(g_szNOrder) == SIZEOF(m_unNums) );
	ASSERT( SIZEOF(g_szDOrder) == SIZEOF(g_dfDOrder) );
	ASSERT( SIZEOF(g_szDOrder) == SIZEOF(m_udNums) );
	ASSERT( SIZEOF(g_szBOrder) == SIZEOF(g_dfBOrder) );
	ASSERT( SIZEOF(g_szBOrder) == SIZEOF(m_ubFlags) );
	ASSERT( SIZEOF(g_szSOrder) == SIZEOF(g_dfSOrder) );
	ASSERT( SIZEOF(g_szSOrder) == SIZEOF(m_strOption) );

	// ���ߒ��̌v�Z(GetInsertSpace()�Ŏg�p)
	m_nOrderLength = 0;
	for ( i=0; i<SIZEOF(g_szNOrder); i++ ) {
		if ( m_nOrderLength < (nLen=lstrlen(g_szNOrder[i])) )
			m_nOrderLength = nLen;
	}
	for ( i=0; i<SIZEOF(g_szDOrder); i++ ) {
		if ( m_nOrderLength < (nLen=lstrlen(g_szDOrder[i])) )
			m_nOrderLength = nLen;
	}
	for ( i=0; i<SIZEOF(g_szBOrder); i++ ) {
		if ( m_nOrderLength < (nLen=lstrlen(g_szBOrder[i])) )
			m_nOrderLength = nLen;
	}
	for ( i=0; i<SIZEOF(g_szSOrder); i++ ) {
		if ( m_nOrderLength < (nLen=lstrlen(g_szSOrder[i])) )
			m_nOrderLength = nLen;
	}
	m_nOrderLength += 2;	// 2��������߰�

	// ���ʌ݊��̂��߂̏���
	BOOL bResult = Convert();
	// ���ʌ݊���߼�݂ɏ㏑��
	ReadMakeOption(lpszInit);
	// �ڍs����
	if ( bResult && lpszInit )
		SaveMakeOption();
}

/////////////////////////////////////////////////////////////////////////////
// հ�����ފ֐�

void CNCMakeOption::InitialDefault(void)
{
	extern	LPTSTR	g_pszExecDir;	// ���s�ިڸ��(NCVC.cpp)
	int		i;

	for ( i=0; i<SIZEOF(g_dfNOrder); i++ )
		m_unNums[i] = g_dfNOrder[i];
	for ( i=0; i<SIZEOF(g_dfDOrder); i++ )
		m_udNums[i] = g_dfDOrder[i];
	for ( i=0; i<SIZEOF(g_dfBOrder); i++ )
		m_ubFlags[i] = g_dfBOrder[i];
	for ( i=0; i<MKNC_STR_HEADER; i++ )
		m_strOption[i] = g_dfSOrder[i];
	for ( i=MKNC_STR_HEADER; i<=MKNC_STR_FOOTER; i++ ) {
		m_strOption[i]  = g_pszExecDir;
		m_strOption[i] += g_dfSOrder[i];
	}
}

BOOL CNCMakeOption::ReadMakeOption(LPCTSTR lpszInitFile)
{
	// ���߂�"="��";"(����)�ŕ���
	typedef boost::tokenizer< boost::escaped_list_separator<TCHAR> > tokenizer;
	static	boost::escaped_list_separator<TCHAR> sep("", "=;", "\"");	// �����ߖ���
	// �؍�����̖��ߌ���(�啶���������͖������邪���߂͊��S��v)
	static	CStringKeyIndex		stNOrder(SIZEOF(g_szNOrder), g_szNOrder);
	static	CStringKeyIndex		stDOrder(SIZEOF(g_szDOrder), g_szDOrder);
	static	CStringKeyIndex		stBOrder(SIZEOF(g_szBOrder), g_szBOrder);
	static	CStringKeyIndex		stSOrder(SIZEOF(g_szSOrder), g_szSOrder);

#ifdef _DEBUGOLD
	CMagaDbg	dbg("CNCMakeOption::ReadMakeOption()\nStart", DBG_GREEN);
#endif

	// �܂���̫�Ăŏ�����
	InitialDefault();
	if ( !lpszInitFile || lstrlen(lpszInitFile)<=0 ) {
		m_strInitFile.Empty();
		return TRUE;
	}
/*
	NCVC�N�����ɊO����ި��ɂĕύX����Ă���\��������̂ŁC
	���ɕێ����Ă���̧�ٖ��Ɠ������Ă��C���̊֐����Ă΂ꂽ�Ƃ��͕K���ǂݍ���
*/
	m_strInitFile = lpszInitFile;

	tokenizer::iterator it;
	std::string	str, strOrder, strResult;
	tokenizer	tok( str, sep );

	CString	strBuf;
	TCHAR	szCurrent[_MAX_PATH], szFile[_MAX_PATH];
	INT_PTR	n;
	BOOL	bResult = TRUE;

	// �V�K��߼�݂ւ���̫�Ēl�ݒ菀��
	BOOL	bDrill[] = {FALSE, FALSE};	// ������Z�ʒu�̓ǂݍ����׸�(Ver0.08.20�ȍ~)

	// �����ިڸ�؂� lpszFile �� -> PathSearchAndQualify()
	::GetCurrentDirectory(_MAX_PATH, szCurrent);
	::Path_Name_From_FullPath(m_strInitFile, strBuf, CString());
	::SetCurrentDirectory(strBuf);

	try {
		CStdioFile	fp(m_strInitFile,
			CFile::modeRead | CFile::shareDenyWrite | CFile::typeText);
		while ( fp.ReadString(strBuf) ) {
			strBuf.TrimLeft();
			if ( ::NC_IsNullLine(strBuf) )
				continue;
			// �����������
			str = strBuf;
			tok.assign(str);
#ifdef _DEBUGOLD
			dbg.printf("strBuf   = %s", strBuf);
#endif
			// ���߂ƒl�ɕ���
			it = tok.begin();
			strOrder  = ::Trim(*it);	++it;
			strResult = ::Trim(*it);
			if ( strOrder.empty() )
				continue;
#ifdef _DEBUGOLD
			dbg.printf("strOrder  =%s", strOrder.c_str());
			dbg.printf("strResult =%s", strResult.c_str());
#endif
			// ���ߌ���(int�^)
			n = stNOrder.GetIndex(strOrder.c_str());
			if ( n>=0 && n<SIZEOF(g_szNOrder) ) {
				m_unNums[n] = strResult.empty() ? 0 : atoi(strResult.c_str());
				continue;
			}
			// ���ߌ���(double�^)
			n = stDOrder.GetIndex(strOrder.c_str());
			if ( n>=0 && n<SIZEOF(g_szDOrder) ) {
				m_udNums[n] = strResult.empty() ? 0 : atof(strResult.c_str());
				// ������Z����߼�݂�ǂݍ��񂾂�
				if ( n>=MKNC_DBL_DRILLR && n<=MKNC_DBL_DRILLZ )
					bDrill[n-MKNC_DBL_DRILLR] = TRUE;
				continue;
			}
			// ���ߌ���(BOOL�^)
			n = stBOrder.GetIndex(strOrder.c_str());
			if ( n>=0 && n<SIZEOF(g_szBOrder) ) {
				m_ubFlags[n] = strResult.empty() ? FALSE :
						atoi(strResult.c_str())!=0 ? TRUE : FALSE;
				continue;
			}
			// ���ߌ���(CString�^)
			n = stSOrder.GetIndex(strOrder.c_str());
			if ( n>=0 && n<SIZEOF(g_szSOrder) ) {
				// �����߽�Ȃ����߽�ɕϊ�
				if ( n==MKNC_STR_HEADER || n==MKNC_STR_FOOTER ) {
					if ( !strResult.empty() &&
								::PathIsRelative(strResult.c_str()) &&		// Shlwapi.h
								::PathSearchAndQualify(strResult.c_str(), szFile, _MAX_PATH) )
						strResult = szFile;
				}
				m_strOption[n] = strResult.c_str();
				continue;
			}
		}	// End of while
	}
	catch (CFileException* e) {
		strBuf.Format(IDS_ERR_DXF2NCDINIT, m_strInitFile);
		AfxMessageBox(strBuf, MB_OK|MB_ICONSTOP);
		e->Delete();
		bResult = FALSE;
	}

	// �V�K��߼�݂ւ���̫�Ēl�ݒ�
	for ( size_t i=0; i<SIZEOF(bDrill); i++ ) {	// ������Z����߼��
		// �ǂݍ��݂��Ȃ���΁C�W����R�_�Ɛ؂荞�݈ʒu���
		if ( !bDrill[i] )
			m_udNums[i+MKNC_DBL_DRILLR] = m_udNums[i];
	}

	// �����ިڸ�؂����ɖ߂�
	::SetCurrentDirectory(szCurrent);

	return bResult;
}

BOOL CNCMakeOption::SaveMakeOption(LPCTSTR lpszInitFile)
{
	int		i, n;
	CString	strBuf, strResult;
	TCHAR	szFile[_MAX_PATH];

#ifdef _DEBUG
	CMagaDbg	dbg("CNCMakeOption::SaveMakeOption()\nStart", DBG_GREEN);
#endif

	// �V�K�ۑ��̏ꍇ
	if ( lpszInitFile )
		m_strInitFile = lpszInitFile;

	ASSERT( !m_strInitFile.IsEmpty() );

	try {
		CStdioFile	fp(m_strInitFile,
			CFile::modeCreate | CFile::modeWrite | CFile::shareDenyNone | CFile::typeText);
		// ����ͯ�ް
		for ( i=0; i<SIZEOF(g_szInitComment); i++ )
			fp.WriteString(g_szInitComment[i]);
		fp.WriteString(g_szInitComment[0]);
		// ���߂̏����o��
		for ( i=0; i<SIZEOF(g_stSaveOrder); i++ ) {
			n = g_stSaveOrder[i].nID;
			switch ( g_stSaveOrder[i].enType ) {
			case NC_PAGE:	// �߰��ͯ�ް
				strBuf.Format("#--> Page:%d\n", n);
				break;
			case NC_NUM:	// int�^
				strBuf.Format("%s%s= %8d     ; %s\n",
					g_szNOrder[n], GetInsertSpace(lstrlen(g_szNOrder[n])),
					m_unNums[n],
					g_stSaveOrder[i].lpszComment);
				break;
			case NC_DBL:	// double�^
				strBuf.Format("%s%s= %12.3f ; %s\n",
					g_szDOrder[n], GetInsertSpace(lstrlen(g_szDOrder[n])),
					m_udNums[n],
					g_stSaveOrder[i].lpszComment);
				break;
			case NC_FLG:	// BOOL�^
				strBuf.Format("%s%s= %8d     ; %s\n",
					g_szBOrder[n], GetInsertSpace(lstrlen(g_szBOrder[n])),
					m_ubFlags[n] ? 1 : 0,
					g_stSaveOrder[i].lpszComment);
				break;
			case NC_STR:	// CString�^
				if ( n==MKNC_STR_HEADER || n==MKNC_STR_FOOTER ) {
					// ����ٰ��߽�Ȃ瑊���߽�ɕϊ�
					if ( !m_strOption[n].IsEmpty() &&
							::PathIsSameRoot(m_strInitFile, m_strOption[n]) )	// Shlwapi.h
						strResult = ::PathRelativePathTo(szFile, m_strInitFile, FILE_ATTRIBUTE_NORMAL,
										m_strOption[n], FILE_ATTRIBUTE_NORMAL) ? szFile : m_strOption[n];
					else
						strResult = m_strOption[n];
				}
				else
					strResult = m_strOption[n];
				// �����𐮂��邽�߂Ɏc��߰��̌v�Z
				strBuf.Format("%s%s= \"%s\"%s; %s\n",
					g_szSOrder[n], GetInsertSpace(lstrlen(g_szSOrder[n])),
					strResult, CString(' ', max(1, 11-strResult.GetLength())),
					g_stSaveOrder[i].lpszComment);
				break;
			default:
				strBuf.Empty();
				break;
			}
			if ( !strBuf.IsEmpty() )
				fp.WriteString(strBuf);
		}
	}
	catch (CFileException* e) {
		strBuf.Format(IDS_ERR_DXF2NCDINIT, m_strInitFile);
		AfxMessageBox(strBuf, MB_OK|MB_ICONSTOP);
		e->Delete();
		return FALSE;
	}
	return TRUE;
}

// ڼ޽�؂���̈ڍs
BOOL CNCMakeOption::Convert()
{
	extern	LPCTSTR	gg_szRegKey;

	// NC������߼��(���ް�ޮ݌݊��p)
	static	const	int		nRegDxfNums[] = {
		IDS_REG_DXF_LINEADD, IDS_REG_DXF_G90, IDS_REG_DXF_DOTP
	};
	static	const	int		nRegDxfNumsDef[] = {
		1, 0, 0
	};
	static	const	int		nRegDxfNumsID[] = {
		MKNC_NUM_LINEADD, MKNC_NUM_G90, MKNC_NUM_DOT
	};

	static	const	int		nRegDxfFlags[] = {
		IDS_REG_DXF_XREV,
		IDS_REG_DXF_LINE, IDS_REG_DXF_ZERO, IDS_REG_DXF_CUT
	};
	static	const	int		nRegDxfFlagsDef[] = {
		FALSE,
		FALSE, TRUE, TRUE
	};
	static	const	int		nRegDxfFlagsID[] = {
		MKNC_FLG_XREV,
		MKNC_FLG_LINEADD, MKNC_FLG_ZEROCUT, MKNC_FLG_GCLIP
	};

	CString	strRegKey, strEntry;
	// �ڍs��������
	VERIFY(strRegKey.LoadString(IDS_REGKEY_DXF));
	VERIFY(strEntry.LoadString(IDS_REG_CONVERT));
	if ( AfxGetApp()->GetProfileInt(strRegKey, strEntry, 0) > 0 )
		return FALSE;

	// �ް��̈ڍs
	AfxGetApp()->WriteProfileInt(strRegKey, strEntry, 1);
	int	i, nID;
	for ( i=0; i<SIZEOF(nRegDxfNums); i++ ) {
		VERIFY(strEntry.LoadString(nRegDxfNums[i]));
		nID = nRegDxfNumsID[i];
		m_unNums[nID] = AfxGetApp()->GetProfileInt(strRegKey, strEntry, nRegDxfNumsDef[i]);
	}
	for ( i=0; i<SIZEOF(nRegDxfFlags); i++ ) {
		VERIFY(strEntry.LoadString(nRegDxfFlags[i]));
		nID = nRegDxfFlagsID[i];
		m_ubFlags[nID] = (BOOL)(AfxGetApp()->GetProfileInt(strRegKey, strEntry, nRegDxfFlagsDef[i]));
	}
	VERIFY(strEntry.LoadString(IDS_REG_DXF_CIRCLEIJ));
	m_unNums[MKNC_NUM_IJ] = AfxGetApp()->GetProfileInt(strRegKey, strEntry, 1);
	VERIFY(strEntry.LoadString(IDS_REG_DXF_LINEFORM));
	m_strOption[MKNC_STR_LINEFORM] = AfxGetApp()->GetProfileString(strRegKey, strEntry);

	// ��ڼ޽�؂̏���
	CRegKey	reg;

	// --- "Software\MNCT-S\NCVC\DXF"
	if ( reg.Open(HKEY_CURRENT_USER, gg_szRegKey+strRegKey) != ERROR_SUCCESS )
		return FALSE;

	for ( i=0; i<SIZEOF(nRegDxfNums); i++ ) {
		VERIFY(strEntry.LoadString(nRegDxfNums[i]));
		reg.DeleteValue(strEntry);
	}
	for ( i=0; i<SIZEOF(nRegDxfFlags); i++ ) {
		VERIFY(strEntry.LoadString(nRegDxfFlags[i]));
		reg.DeleteValue(strEntry);
	}
	VERIFY(strEntry.LoadString(IDS_REG_DXF_CIRCLEIJ));
	reg.DeleteValue(strEntry);
	VERIFY(strEntry.LoadString(IDS_REG_DXF_LINEFORM));
	reg.DeleteValue(strEntry);

	// Init����̧�ٖ��̍폜
	VERIFY(strEntry.LoadString(IDS_REG_DXF_INIT));
	reg.DeleteValue(strEntry);

	return TRUE;
}

#ifdef _DEBUGOLD
void CNCMakeOption::DbgDump(void) const
{
	CMagaDbg	dbg("CNCMakeOption", DBG_RED);

	dbg.printf("InitFile=%s", m_strInitFile);
	dbg.printf("----------");
	dbg.printf("  Spindle      =%d", m_nSpindle);
	dbg.printf("  Feed         =%f", m_dFeed);
	dbg.printf("  ZFeed        =%f", m_dZFeed);
	dbg.printf("  ZG0Stop      =%f", m_dZG0Stop);
	dbg.printf("  ZCut         =%f", m_dZCut);
	dbg.printf("  G92[X]       =%f", m_dG92[NCA_X]);
	dbg.printf("  G92[Y]       =%f", m_dG92[NCA_Y]);
	dbg.printf("  G92[Z]       =%f", m_dG92[NCA_Z]);
	dbg.printf("  Header       =%s", m_strOption[MKNC_STR_HEADER]);
	dbg.printf("  Footer       =%s", m_strOption[MKNC_STR_FOOTER]);
	dbg.printf("----------");
	dbg.printf("  Xrev         =%d", m_bXrev);
	dbg.printf("  Yrev         =%d", m_bYrev);
	dbg.printf("  bLineAdd     =%d", m_bLineAdd);
	dbg.printf("  LineForm     =%s", m_strOption[MKNC_STR_LINEFORM]);
	dbg.printf("  nLineAdd     =%d", m_nLineAdd);
	dbg.printf("  EOB          =%s", m_strOption[MKNC_STR_EOB]);
	dbg.printf("  G90          =%d", m_nG90);
	dbg.printf("  ZReturn      =%d", m_nZReturn);
	dbg.printf("  Gclip        =%d", m_bGclip);
	dbg.printf("  DisSpindle   =%d", m_bDisableSpindle);
	dbg.printf("----------");
	dbg.printf("  Dot          =%d", m_nDot);
	dbg.printf("  FDot         =%d", m_nFDot);
	dbg.printf("  ZeroCut      =%d", m_bZeroCut);
	dbg.printf("  CircleCode   =%d", m_nCircleCode);
	dbg.printf("  IJ           =%d", m_nIJ);
	dbg.printf("  CircleHalf   =%d", m_bCircleHalf);
	dbg.printf("----------");
	dbg.printf("  Ellipse      =%f", m_dEllipse);
	dbg.printf("  EllipseFlg   =%d", m_bEllipse);
	dbg.printf("----------");
	dbg.printf("  MakeEnd      =%d", m_nMakeEnd);
	dbg.printf("  MakeValue    =%f", m_dMakeValue);
	dbg.printf("  MakeFeed     =%f", m_dMakeFeed);
	dbg.printf("  Deep         =%d", m_bDeep);
	dbg.printf("  DeepFinal    =%f", m_dDeep);
	dbg.printf("  ZStep        =%f", m_dZStep);
	dbg.printf("  DeepZProcess =%d", m_nDeepZProcess);
	dbg.printf("  DeepAProcess =%d", m_nDeepAProcess);
	dbg.printf("  DeepCProcess =%d", m_nDeepCProcess);
	dbg.printf("  DeepFinish   =%d", m_bDeepFinish);
	dbg.printf("  DeepSpindle  =%d", m_nDeepSpindle);
	dbg.printf("  DeepFeed     =%f", m_dDeepFeed);
	dbg.printf("----------");
	dbg.printf("  DrillSpindle =%d", m_nDrillSpindle);
	dbg.printf("  DrillFeed    =%f", m_dDrillFeed);
	dbg.printf("  DrillR       =%f", m_dDrillR);
	dbg.printf("  DrillZ       =%f", m_dDrillZ);
	dbg.printf("  DrillMatch   =%d", m_bDrillMatch);
	dbg.printf("  Dwell        =%d", m_nDwell);
	dbg.printf("  DwellFormat  =%d", m_nDwellFormat);
	dbg.printf("  DrillProcess =%d", m_nDrillProcess);
	dbg.printf("  DrillZProcess=%d", m_nDrillZProcess);
	dbg.printf("----------");
	dbg.printf("  DrillCircle  =%d", m_bDrillCircle);
	dbg.printf("  DrillCircleR =%f", m_dDrillCircle);
	dbg.printf("  DrillSort    =%d", m_nDrillSort);
	dbg.printf("  DrillCProcess=%d", m_nDrillCircleProcess);
	dbg.printf("  DrillBreak   =%d", m_bDrillBreak);
	dbg.printf("----------");
	dbg.printf("  LayerComment =%d", m_bLayerComment);
	dbg.printf("  MoveZ        =%d", m_nMoveZ);
	dbg.printf("  CustMoveB    =%s", m_strOption[MKNC_STR_CUSTMOVE_B]);
	dbg.printf("  CustMoveA    =%s", m_strOption[MKNC_STR_CUSTMOVE_A]);
	dbg.printf("----------");
	dbg.printf("  Tolerance    =%f", m_dTolerance);
	dbg.printf("  TolerancePro =%d", m_nTolerance);
	dbg.printf("  DrillOptimaiz=%d", m_nOptimaizeDrill);
	dbg.printf("  DrillMargin  =%f", m_dDrillMargin);
}
#endif
