// TH_NCRead.cpp
// �m�b�R�[�h�̓����ϊ�
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "MCOption.h"
#include "NCdata.h"
#include "NCDoc.h"
#include "ThreadDlg.h"
#include "NCVCdefine.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

using namespace std;
using namespace boost;
using namespace boost::spirit::classic;

#define	IsThread()	g_pParent->IsThreadContinue()
//	��Ȳٕ\���ł�����گ�ޑΉ�
__declspec(thread)	static	CThreadDlg*	g_pParent;
__declspec(thread)	static	CNCDoc*		g_pDoc;
__declspec(thread)	static	NCARGV		g_ncArgv;		// NCVCdefine.h
__declspec(thread)	static	DWORD		g_dwValFlags;	// UVW���W�p
__declspec(thread)	static	double		g_dValue[NCXYZ];
__declspec(thread)	static	int			g_nSubprog;		// �����۸��ьĂяo���̊K�w
__declspec(thread)	static	LPTSTR		g_lpstrComma;	// ������ۯ��Ƃ̌v�Z

// �Œ軲�ق�Ӱ��ٕ�Ԓl
struct	CYCLE_INTERPOLATE
{
	BOOL	bCycle;		// �Œ軲�ُ�����
	double	dVal[3];	// Z, R, P
	void	clear(void) {
		bCycle = FALSE;
		dVal[0] = dVal[1] = dVal[2] = HUGE_VAL;
	}
};
__declspec(thread)	static	CYCLE_INTERPOLATE	g_Cycle;
static	void	CycleInterpolate(void);

// IsGcodeObject() �߂�l
static	enum	ENGCODEOBJ {NOOBJ, MAKEOBJ, MAKEOBJ_NOTMODAL};

// G68���W��]
static	void	G68RoundCheck(CNCblock*);
static	void	G68RoundClear(void);
//
static	void		InitialVariable(void);
// ��͊֐�
static	int			NC_GSeparater(int, CNCdata*&);
static	CNCdata*	AddGcode(CNCblock*, CNCdata*, int);
static	CNCdata*	AddM98code(CNCblock*, CNCdata*, int);
static	int			CallSubProgram(CNCblock*, CNCdata*&);
typedef	ENGCODEOBJ	(*PFNCHECKGCODE)(int);
static	ENGCODEOBJ	IsGcodeObject_Milling(int);
static	ENGCODEOBJ	IsGcodeObject_Lathe(int);
__declspec(thread)	static	PFNCHECKGCODE	g_pfnIsGcode;
static	ENGCODEOBJ	CheckGcodeOther_Milling(int);
static	ENGCODEOBJ	CheckGcodeOther_Lathe(int);
__declspec(thread)	static	PFNCHECKGCODE	g_pfnCheckGcodeOther;
// �ʎ��E��ŰR����
static	void	MakeChamferingObject(CNCblock*, CNCdata*, CNCdata*);
// F���Ұ�, �޳�َ��Ԃ̉���
typedef double (*PFNFEEDANALYZE)(const string&);
static	double	FeedAnalyze_Dot(const string&);
static	double	FeedAnalyze_Int(const string&);
__declspec(thread)	static	PFNFEEDANALYZE	g_pfnFeedAnalyze;
// �H��a���
static	void	SetEndmillDiameter(const string&);
static	void	SetEndmillDiameter_fromComment(double);
static	void	SetEndmillType(char);
// ܰ���`���ݒ�
static	void	SetWorkRect_fromComment(vector<double>&);
static	void	SetLatheRect_fromComment(vector<double>&);
// �����ہCϸۂ̌���
static	CString	g_strSearchFolder[2];	// ���ĂƎw��̫���
static	CString	SearchFolder(regex&);
static	CString	SearchFolder_Sub(int, LPCTSTR, regex&);
static	BOOL	SearchProgNo(LPCTSTR, regex&);
static	regex	g_reMacroStr;		// __declspec(thread)�͕s�v
static	int		NC_SearchSubProgram(int*);
typedef	int		(*PFNBLOCKPROCESS)(CNCblock*);
static	int		NC_SearchMacroProgram(CNCblock*);
static	int		NC_NoSearch(CNCblock*);
__declspec(thread)	static	PFNBLOCKPROCESS	g_pfnSearchMacro;
// ������ڲ����ތ���
static	regex	g_reAutoBreak;		// __declspec(thread)�͕s�v
static	int		NC_SearchAutoBreak(CNCblock*);
__declspec(thread)	static	PFNBLOCKPROCESS	g_pfnSearchAutoBreak;

//////////////////////////////////////////////////////////////////////

// ���l�ϊ�( 1/1000 �ް��𔻒f����)
inline	double		GetNCValue(const string& str)
{
	double	dResult = atof(str.c_str());
	if ( str.find('.') == string::npos )
		dResult /= 1000.0;		// �����_���Ȃ���� 1/1000
	return dResult;
}
// g_lpstrComma ���ް�����
inline	void	SetStrComma(const string& strComma)
{
	if ( g_lpstrComma )
		delete	g_lpstrComma;
	if ( strComma.length() <= 0 )
		g_lpstrComma = NULL;
	else {
		g_lpstrComma = new TCHAR[strComma.length()+1];
		lstrcpy(g_lpstrComma, strComma.c_str());
	}
}

//////////////////////////////////////////////////////////////////////
//	NC���ނ̵�޼ު�Đ����گ��
//////////////////////////////////////////////////////////////////////

UINT NCDtoXYZ_Thread(LPVOID pVoid)
{
#ifdef _DEBUG
	CMagaDbg	dbg("NCDtoXYZ_Thread()\nStart", DBG_RED);
	CMagaDbg	dbg1(DBG_BLUE);
#endif

	int			i, nLoopCnt,
				nResult = IDOK;
	CNCdata*	pDataFirst = NULL;
	CNCdata*	pData;	// �P�O�̐�����޼ު��

	// �ϐ�������
	LPNCVCTHREADPARAM	pParam = reinterpret_cast<LPNCVCTHREADPARAM>(pVoid);
	g_pParent = pParam->pParent;
	g_pDoc  = static_cast<CNCDoc*>(pParam->pDoc);

	ASSERT(g_pDoc);
	InitialVariable();

	nLoopCnt = g_pDoc->GetNCBlockSize();
	if ( g_pParent ) {
		CString	strMsg;
		VERIFY(strMsg.LoadString(IDS_READ_NCD));
		g_pParent->SetFaseMessage(strMsg);
		g_pParent->m_ctReadProgress.SetRange32(0, nLoopCnt);
	}
#ifdef _DEBUG
	if ( !g_pDoc->IsNCDocFlag(NCDOC_THUMBNAIL) )
		dbg.printf("LoopCount=%d", nLoopCnt);
#endif

	try {
		// �P�O�̵�޼ު�ĎQ�Ƃ�NULL�Q�Ƃ��Ȃ�����
		pDataFirst = pData = new CNCdata(&g_ncArgv);
		// 1�s(1��ۯ�)��͂���޼ު�Ă̓o�^
		for ( i=0; i<nLoopCnt; i++ ) {
			if ( g_pParent && !IsThread() )
				break;
			if ( NC_GSeparater(i, pData) != 0 )
				break;
			if ( (i & 0x003f)==0 && g_pParent )	// 64�񂨂�(����6�ޯ�Ͻ�)
				g_pParent->m_ctReadProgress.SetPos(i);		// ��۸�ڽ�ް
		}
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		nResult = IDCANCEL;
	}

	if ( g_lpstrComma )
		delete	g_lpstrComma;
	// ����o�^�p��а�ް��̏���
	if ( pDataFirst )
		delete	pDataFirst;

	if ( g_pParent ) {
		g_pParent->m_ctReadProgress.SetPos(nLoopCnt);
		g_pParent->PostMessage(WM_USERFINISH, IsThread() ? nResult : IDCANCEL);
	}

	return 0;	// AfxEndThread(0);
}

//////////////////////////////////////////////////////////////////////
//	G���ނ̍\�����

//	NC��۸��щ��
struct CGcodeParser : grammar<CGcodeParser>
{
	vector<string>&	vResult;
	CGcodeParser(vector<string>& v) : vResult(v) {}

	template<typename T>
	struct definition {
		typedef	rule<T>	rule_t;
		rule_t	rr;
		definition( const CGcodeParser& self ) {
			// ���m���ނɂ��Ή����邽�߁u�S�Ẳp�啶���ɑ������l�v�ŉ��
			rr = +( +( upper_p >> real_p )[push_back_a(self.vResult)]
					>> !( ch_p(',') >> ((ch_p('R')|'C') >> real_p) )[push_back_a(self.vResult)] )
				>> *(anychar_p - alnum_p)	// EOF�Ȃ�
				>> end_p;	// �I�[(��������Ȃ��� parse()�֐��� full ��Ԃ��Ȃ�)
		}
		const rule_t& start() const {
			return rr;
		}
	};
};

//	������߰�
struct CSkipParser : grammar<CSkipParser>
{
	template<typename T>
	struct definition {
		typedef	rule<T>	rule_t;
		rule_t	skip_p;
		definition( const CSkipParser& self )
		{
			skip_p = space_p | comment_p('(', ')');
		}
		const rule_t& start() const {
			return skip_p;
		}
	};
};

//	���ĕ�������
struct CCommentParser : grammar<CCommentParser>
{
	vector<double>&	vRect;
	vector<double>&	vLathe;
	CCommentParser(vector<double>& v1, vector<double>& v2) : vRect(v1), vLathe(v2) {}

	template<typename T>
	struct definition
	{
		typedef	rule<T>	rule_t;
		rule_t	rr, rs1, rs2, rs3, rr1, rr2, rr3;
		definition( const CCommentParser& self )
		{
			// Endmill ��ܰ��
			rs1 = as_lower_d[str_p("endmill")]  >> ch_p('=');
			rr1 = real_p[&SetEndmillDiameter_fromComment] >> !str_p("mm") >>
					!(ch_p(',') >> digit_p[&SetEndmillType]);	// 0:�����, 1:�ް�
			// WorkRect ��ܰ��
			rs2 = as_lower_d[str_p("workrect")] >> ch_p('=');
			rr2 = real_p[push_back_a(self.vRect)] % ch_p(',');
			// ViewMode
			rs3 = as_lower_d[str_p("latheview")] >> ch_p('=');
			rr3 = real_p[push_back_a(self.vLathe)] % ch_p(',');
			//
			rr = ch_p('(') >> *(anychar_p-(rs1|rs2|rs3)) >>
					+( rs1>>rr1 | rs2>>rr2 | rs3>>rr3 );
		}
		const rule_t& start() const {
			return rr;
		}
	};
};

//////////////////////////////////////////////////////////////////////
// G���ނ̕���(�ċA�֐�)
int NC_GSeparater(int nLine, CNCdata*& pDataResult)
{
	extern	LPCTSTR			g_szNdelimiter; // "XYZRIJKPLDH"
	extern	const	DWORD	g_dwSetValFlags[];
	vector<string>	vResult;
	vector<string>::iterator	it;
	vector<double>	dRect, dLathe;
	CGcodeParser	gr(vResult);
	CCommentParser	comment_p(dRect, dLathe);
	CSkipParser		skip_p;

	int			i, nCode,
				nNotModalCode = -1,	// Ӱ��ٕs�v��G����(��ۯ����ł̂ݗL��)
				nResult, nIndex;
	BOOL		bNCobj = FALSE, bNCval = FALSE, bNCsub = FALSE;
	ENGCODEOBJ	enGcode;
	CNCdata*	pData;
	CNCblock*	pBlock = g_pDoc->GetNCblock(nLine);
	string		strComma;		// ��ψȍ~�̕�����(������ۯ��֌p��)

	// �ϐ�������
	g_ncArgv.nc.nLine		= nLine;
	g_ncArgv.nc.nErrorCode	= 0;
	g_ncArgv.nc.dwValFlags &= 0xFFFF0000;
#ifdef _DEBUG
	CMagaDbg	dbg("NC_Gseparate()", DBG_BLUE);
	CMagaDbg	dbg1(DBG_GREEN);
	if ( !g_pDoc->IsNCDocFlag(NCDOC_THUMBNAIL) )
		dbg.printf("No.%004d Line=%s", nLine+1, pBlock->GetStrGcode());
#endif

	if ( !g_pDoc->IsNCDocFlag(NCDOC_THUMBNAIL) ) {	// ��Ȳٕ\���̂Ƃ��͏������Ȃ�
		// ������ڲ����ތ���
		(*g_pfnSearchAutoBreak)(pBlock);
	}

	// ���ĉ��(���Ӱ��, �����ٌa, ܰ���`���̎擾)
	parse((LPCTSTR)(pBlock->GetStrGcode()), comment_p, space_p);
	if ( !dRect.empty() )
		SetWorkRect_fromComment(dRect);
	if ( !dLathe.empty() )
		SetLatheRect_fromComment(dLathe);

	// ϸےu�����
	if ( (nIndex=(*g_pfnSearchMacro)(pBlock)) >= 0 ) {
		g_nSubprog++;
		// M98��޼ު�Ă�O�ԍ�(nIndex)�̓o�^
		pDataResult = AddM98code(pBlock, pDataResult, nIndex);
		// g_pfnSearchMacro ����ۯ����ǉ������\���A��
		// �����ł� nLoop �ϐ����g�킸�AȲè�ނ���ۯ����ނɂĔ���
		for ( i=nIndex; i<g_pDoc->GetNCBlockSize(); i++ ) {
			if ( g_pParent && !IsThread() )
				break;
			nResult = NC_GSeparater(i, pDataResult);	// �ċA
			if ( nResult == 30 )
				return 30;
			else if ( nResult == 99 )
				break;
		}
		// EOF�ŏI���Ȃ�M99���A����
		if ( i >= g_pDoc->GetNCBlockSize() && nResult == 0 ) {
			if ( g_nSubprog > 0 )
				g_nSubprog--;
		}
		return 0;
	}

	// G���ލ\�����
	if ( !parse((LPCTSTR)(pBlock->GetStrGcode()), gr, skip_p).full ||
			vResult.empty() )
		return 0;

	for ( it=vResult.begin(); it!=vResult.end(); ++it ) {
		if ( g_pParent && !IsThread() )
			break;
#ifdef _DEBUG
		if ( !g_pDoc->IsNCDocFlag(NCDOC_THUMBNAIL) )
			dbg1.printf("G Cut=%s", it->c_str());
#endif
		switch ( it->at(0) ) {
		case 'M':
			// �O��̺��ނœo�^��޼ު�Ă�����Ȃ�
			if ( bNCobj || bNCval ) {
				// ��޼ު�Đ���
				pData = AddGcode(pBlock, pDataResult, nNotModalCode);
				// �ʎ���޼ު�Ă̓o�^
				if ( g_lpstrComma )
					MakeChamferingObject(pBlock, pDataResult, pData);
				pDataResult = pData;
				SetStrComma(strComma);
				nNotModalCode = -1;
				bNCobj = bNCval = FALSE;
			}
			// �O��̺��ނŻ����یĂяo���������
			if ( bNCsub ) {
				bNCsub = FALSE;
				if ( CallSubProgram(pBlock, pDataResult) == 30 )
					return 30;	// �I������
			}
			nCode = atoi(it->substr(1).c_str());
			switch ( nCode ) {
			case 2:
			case 30:
				return 30;
			case 98:
				// 5�K�w�ȏ�̌Ăяo���ʹװ(4�K�w�܂�)
				if ( g_nSubprog+1 >= 5 ) {
					pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_M98L);
					continue;
				}
				bNCsub = TRUE;
				break;
			case 99:
				if ( g_nSubprog > 0 )
					g_nSubprog--;
				// ���A�p��޼ު�Đ���
				pDataResult = AddM98code(pBlock, pDataResult, -1);
				return 99;
			}
			break;
		case 'G':
			// ��Ɍ��݂̺��ގ�ʂ�����
			nCode = atoi(it->substr(1).c_str());
			enGcode = (*g_pfnIsGcode)(nCode);	// IsGcodeObject_�`
			if ( enGcode == NOOBJ ) {
				(*g_pfnCheckGcodeOther)(nCode);
				break;
			}
			// �O��̺��ނœo�^��޼ު�Ă�����Ȃ�
			if ( bNCobj || bNCval ) {
				pData = AddGcode(pBlock, pDataResult, nNotModalCode);
				if ( g_lpstrComma )
					MakeChamferingObject(pBlock, pDataResult, pData);
				pDataResult = pData;
				SetStrComma(strComma);
				nNotModalCode = -1;
				bNCval = FALSE;		// bNCobj �ر�s�v
			}
			if ( bNCsub ) {
				bNCsub = FALSE;
				if ( CallSubProgram(pBlock, pDataResult) == 30 )
					return 30;	// �I������
			}
			// ���݂̺��ޕۊ�
			if ( enGcode == MAKEOBJ )
				g_ncArgv.nc.nGcode = nCode;
			else
				nNotModalCode = nCode;
			bNCobj = TRUE;
			break;
		case 'F':
			g_ncArgv.dFeed = (*g_pfnFeedAnalyze)(it->substr(1));
			break;
		case 'S':
			g_ncArgv.nSpindle = abs(atoi(it->substr(1).c_str()));
			break;
		case 'T':
			SetEndmillDiameter(it->substr(1));
			break;
		case ',':
			strComma = ::Trim(it->substr(1));	// ��ψȍ~���擾
#ifdef _DEBUG
			if ( !g_pDoc->IsNCDocFlag(NCDOC_THUMBNAIL) )
				dbg1.printf("strComma=%s", strComma.c_str());
#endif
			break;
		case 'X':	case 'Y':	case 'Z':
		case 'R':	case 'I':	case 'J':	case 'K':
		case 'P':	case 'L':
		case 'D':	case 'H':
			nCode = (int)(strchr(g_szNdelimiter, it->at(0)) - g_szNdelimiter);
			// �l�擾
			if ( g_Cycle.bCycle ) {		// 81�`89
				// �Œ軲�ق̓��ʏ���
				if ( nCode == NCA_K )		// K��Ȳè�ނ�
					g_ncArgv.nc.dValue[NCA_K] = atoi(it->substr(1).c_str());
				else if ( nCode == NCA_P )	// P(�޳�َ���)�͑��葬�x�Ɠ�������
					g_ncArgv.nc.dValue[NCA_P] = (*g_pfnFeedAnalyze)(it->substr(1));
				else
					g_ncArgv.nc.dValue[nCode] = GetNCValue(it->substr(1));
			}
			else if ( nCode < NCA_P )
				g_ncArgv.nc.dValue[nCode] = GetNCValue(it->substr(1));
			else
				g_ncArgv.nc.dValue[nCode] = atoi(it->substr(1).c_str());
			//
			g_ncArgv.nc.dwValFlags |= g_dwSetValFlags[nCode];
			bNCval = TRUE;
			break;
		case 'U':	case 'V':	case 'W':
			nCode = (int)(it->at(0) - 'U');
			g_dwValFlags |= g_dwSetValFlags[nCode];	// XYZ�������׸�ON
			if ( g_ncArgv.bAbs )
				g_dValue[nCode]  = GetNCValue(it->substr(1));
			else
				g_dValue[nCode] += GetNCValue(it->substr(1));
			bNCval = TRUE;
			break;
		}
	} // End of for() iterator

	if ( bNCsub ) {
		// M���ތ㏈��
		if ( CallSubProgram(pBlock, pDataResult) == 30 )
			return 30;	// �I������
	}
	else if ( bNCobj || bNCval ) {
		// NC�ް��o�^����
		pData = AddGcode(pBlock, pDataResult, nNotModalCode);
		if ( g_lpstrComma )
			MakeChamferingObject(pBlock, pDataResult, pData);
		pDataResult = pData;
		SetStrComma(strComma);
		// ��ۯ����̍X�V
		pBlock->SetBlockToNCdata(pDataResult, g_pDoc->GetNCsize());
	}

	return 0;
}

CNCdata* AddGcode(CNCblock* pBlock, CNCdata* pDataBefore, int nNotModalCode)
{
	extern	const	DWORD	g_dwSetValFlags[];
	CNCdata*	pDataResult = pDataBefore;

	// UVW���W�̉��Z
	g_dwValFlags |= g_ncArgv.nc.dwValFlags;
	for ( int i=0; i<NCXYZ; i++ ) {
		if ( g_dwValFlags & g_dwSetValFlags[i] ) {
			g_ncArgv.nc.dValue[i]  += g_dValue[i];
			g_ncArgv.nc.dwValFlags |= g_dwSetValFlags[i];
		}
	}
	g_dwValFlags = 0;

	if ( g_pDoc->IsNCDocFlag(NCDOC_LATHE) ) {
		// ����Ӱ�ނł̍��W����ւ�
		boost::optional<double>	x, z, i, k;
		if ( g_ncArgv.nc.dwValFlags & NCD_X )
			x = g_ncArgv.nc.dValue[NCA_X] / 2.0;
		if ( g_ncArgv.nc.dwValFlags & NCD_Z )
			z = g_ncArgv.nc.dValue[NCA_Z];
		if ( g_ncArgv.nc.dwValFlags & NCD_I )
			i = g_ncArgv.nc.dValue[NCA_I];
		if ( g_ncArgv.nc.dwValFlags & NCD_K )
			k = g_ncArgv.nc.dValue[NCA_K];
		g_ncArgv.nc.dwValFlags &= ~(NCD_X|NCD_Y|NCD_Z|NCD_I|NCD_J|NCD_K);
		if ( x ) {
			g_ncArgv.nc.dValue[NCA_Z] = *x;
			g_ncArgv.nc.dwValFlags |=  NCD_Z;
		}
		if ( z ) {
			g_ncArgv.nc.dValue[NCA_X] = *z;
			g_ncArgv.nc.dwValFlags |=  NCD_X;
		}
		if ( i ) {
			g_ncArgv.nc.dValue[NCA_K] = *i;
			g_ncArgv.nc.dwValFlags |=  NCD_K;
		}
		if ( k ) {
			g_ncArgv.nc.dValue[NCA_I] = *k;
			g_ncArgv.nc.dwValFlags |=  NCD_I;
		}
		// G02/G03 �����ւ�
		if ( 2<=g_ncArgv.nc.nGcode && g_ncArgv.nc.nGcode<=3 )
			g_ncArgv.nc.nGcode = 1 - (g_ncArgv.nc.nGcode-2) + 2;
	}
	else {
		// NC�ް��̓o�^�O����
		if ( g_ncArgv.nc.nGtype == G_TYPE ) {
			// G68���W��]�w��������
			if ( nNotModalCode == 68 ) {
				G68RoundCheck(pBlock);
				if ( !g_ncArgv.g68.bG68 )
					return pDataResult;
			}
			// �Œ軲�ق�Ӱ��ٕ��
			if ( g_Cycle.bCycle )
				CycleInterpolate();
		}
	}

	// NC�ް��̓o�^
	// --- �ʎ�蓙�ɂ��Čv�Z���ڂ����邪�C
	// --- ���̎��_�ŵ�޼ު�ēo�^���Ă����Ȃ��ƐF�X�ʓ|(?)
	if ( nNotModalCode >= 0 ) {
		int nGcode = g_ncArgv.nc.nGcode;	// Ӱ��ٺ����ޯ�����
		g_ncArgv.nc.nGcode = nNotModalCode;
		pDataResult = g_pDoc->DataOperation(pDataBefore, &g_ncArgv);
		g_ncArgv.nc.nGcode = nGcode;
	}
	else
		pDataResult = g_pDoc->DataOperation(pDataBefore, &g_ncArgv);

	return pDataResult;
}

CNCdata* AddM98code(CNCblock* pBlock, CNCdata* pDataBefore, int nIndex)
{
	CNCdata*	pData;
	DWORD		dwFlags = g_ncArgv.nc.dwValFlags;

	g_ncArgv.nc.dwValFlags = 0;
	g_ncArgv.nc.nGtype = M_TYPE;
	if ( nIndex >= 0 ) {
		pData = AddGcode(pBlock, pDataBefore, 98);
		g_ncArgv.nc.nGtype = O_TYPE;
		g_ncArgv.nc.nLine  = nIndex;
		pData = AddGcode(g_pDoc->GetNCblock(nIndex), pData, 0);
	}
	else
		pData = AddGcode(pBlock, pDataBefore, 99);

	// ����
	g_ncArgv.nc.nGtype = G_TYPE;
	g_ncArgv.nc.dwValFlags = dwFlags;

	return pData;
}

int CallSubProgram(CNCblock* pBlock, CNCdata*& pDataResult)
{
	int			i, nIndex, nRepeat, nResult = 0;

	if ( !(g_ncArgv.nc.dwValFlags & NCD_P) )
		pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_ORDER);
	else if ( (nIndex=NC_SearchSubProgram(&nRepeat)) < 0 )
		pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_M98);
	else {
		g_nSubprog++;
		// M98��޼ު�Ă�O�ԍ�(nIndex)�̓o�^
		pDataResult = AddM98code(pBlock, pDataResult, nIndex);
		// nRepeat���J��Ԃ�
		while ( nRepeat-- > 0 ) {
			if ( g_pParent && !IsThread() )
				break;
			// NC_SearchSubProgram ����ۯ����ǉ������\���A��
			// �����ł� nLoop �ϐ����g�킸�AȲè�ނ���ۯ����ނɂĔ���
			for ( i=nIndex; i<g_pDoc->GetNCBlockSize(); i++ ) {
				if ( g_pParent && !IsThread() )
					break;
				nResult = NC_GSeparater(i, pDataResult);	// �ċA
				if ( nResult == 30 )
					return 30;
				else if ( nResult == 99 )
					break;
			}
		}
		// EOF�ŏI���Ȃ�M99���A����
		if ( i >= g_pDoc->GetNCBlockSize() && nResult == 0 ) {
			if ( g_nSubprog > 0 )
				g_nSubprog--;
		}
	}

	return nResult;
}

//////////////////////////////////////////////////////////////////////
// �⏕�֐�

ENGCODEOBJ	IsGcodeObject_Milling(int nCode)
{
	// G00�`G03, G04, G10, G52, G8x, G92
	// ��޼ު�Đ�������f��������
	ENGCODEOBJ	enResult;

	if ( 0<=nCode && nCode<=3 ) {
		g_Cycle.bCycle = FALSE;
		enResult = MAKEOBJ;
	}
	else if ( nCode==4 || nCode==10 || nCode==52 || nCode==68 || nCode==92 ) {
		enResult = MAKEOBJ_NOTMODAL;
	}
	else if ( 81<=nCode && nCode<=89 ) {
		g_Cycle.bCycle = TRUE;
		enResult = MAKEOBJ;
	}
	else
		enResult = NOOBJ;

	return enResult;
}

ENGCODEOBJ	IsGcodeObject_Lathe(int nCode)
{
	ENGCODEOBJ	enResult;

	if ( 0<=nCode && nCode<=3 ) {
		g_Cycle.bCycle = FALSE;
		enResult = MAKEOBJ;
	}
	else if ( nCode==4 || nCode==10 ) {
		enResult = MAKEOBJ_NOTMODAL;
	}
	else
		enResult = NOOBJ;

	return enResult;
}

// �؍��ވȊO�̏d�v��G���ތ���
ENGCODEOBJ CheckGcodeOther_Milling(int nCode)
{
	ENGCODEOBJ	enResult = NOOBJ;

	switch ( nCode ) {
	// �ضٕ��ʎw��
	case 17:
		g_ncArgv.nc.enPlane = XY_PLANE;
		break;
	case 18:
		g_ncArgv.nc.enPlane = XZ_PLANE;
		break;
	case 19:
		g_ncArgv.nc.enPlane = YZ_PLANE;
		break;
	// �H��a�␳
	case 40:
		g_ncArgv.nc.dwValFlags &= ~NCD_CORRECT;
		break;
	case 41:
		g_ncArgv.nc.dwValFlags |= NCD_CORRECT_L;
		break;
	case 42:
		g_ncArgv.nc.dwValFlags |= NCD_CORRECT_R;
		break;
	// ܰ����W�n
	case 54:
	case 55:
	case 56:
	case 57:
	case 58:
	case 59:
		g_pDoc->SelectWorkOffset(nCode - 54);
		break;
	// ���W��]��ݾ�
	case 69:
		G68RoundClear();
		break;
	// �Œ軲�ٷ�ݾ�
	case 80:
		g_Cycle.clear();
		break;
	// ��޿ح��, �ݸ����
	case 90:
		g_ncArgv.bAbs = TRUE;
		break;
	case 91:
		g_ncArgv.bAbs = FALSE;
		break;
	// �Œ軲�ٕ��A
	case 98:
		g_ncArgv.bG98 = TRUE;
		break;
	case 99:
		g_ncArgv.bG98 = FALSE;
		break;
	}

	return enResult;
}

ENGCODEOBJ CheckGcodeOther_Lathe(int nCode)
{
	ENGCODEOBJ	enResult = NOOBJ;

	switch ( nCode ) {
	// �n��q�␳
//	case 40:
//		g_ncArgv.nc.dwValFlags &= ~NCD_CORRECT;
//		break;
//	case 41:
//		g_ncArgv.nc.dwValFlags |= NCD_CORRECT_L;
//		break;
//	case 42:
//		g_ncArgv.nc.dwValFlags |= NCD_CORRECT_R;
//		break;
	// ܰ����W�n
	case 54:
	case 55:
	case 56:
	case 57:
	case 58:
	case 59:
		g_pDoc->SelectWorkOffset(nCode - 54);
		break;
	// ��޿ح��, �ݸ����
	case 90:
		g_ncArgv.bAbs = TRUE;
		break;
	case 91:
		g_ncArgv.bAbs = FALSE;
		break;
	// ��������
	case 98:
		g_ncArgv.bG98 = TRUE;
		break;
	// ����]����
	case 99:
		g_ncArgv.bG98 = FALSE;
		break;
	}

	return enResult;
}

// �����۸��т̌���
int NC_SearchSubProgram(int *pRepeat)
{
	int		nProg, n;
	CString	strProg;

	if ( g_ncArgv.nc.dwValFlags & NCD_L ) {
		*pRepeat = (int)g_ncArgv.nc.dValue[NCA_L];
		nProg    = (int)g_ncArgv.nc.dValue[NCA_P];
	}
	else {
		// L:�J��Ԃ������w�肳��Ă��Ȃ���΁C
		// [times][number] (n��:4��) ���擾
		CString	strBuf;
		strBuf.Format("%d", (int)g_ncArgv.nc.dValue[NCA_P]);
		n = strBuf.GetLength();
		if ( n > 4 ) {
			*pRepeat = atoi(strBuf.Left(n-4));
			nProg    = atoi(strBuf.Right(4));
		}
		else {
			*pRepeat = 1;
			nProg    = (int)g_ncArgv.nc.dValue[NCA_P];
		}
	}

	// ���K�\��(Oxxxx��ϯ�����)
	strProg.Format("(O(0)*%d)($|[^0-9])", nProg);
	regex	r(strProg);

	// ���݂�(����)�����ۯ����猟��
	n = g_pDoc->SearchBlockRegex(r);
	if ( n >= 0 )
		return n;

	// �@�B���̫��ނ���̧�ٌ���
	CString	strFile( SearchFolder(r) );
	if ( strFile.IsEmpty() )
		return -1;

	// �n�ԍ������݂����NC��ۯ��̑}�� ->�u���وʒu�ɓǂݍ��݁v����ۯ��ǉ�
	n = g_pDoc->GetNCBlockSize();
	if ( g_pDoc->SerializeInsertBlock(strFile, n, NCF_AUTOREAD, FALSE) ) {
		// �}����ۯ��̍ŏ�����NCF_FOLDER
		if ( n < g_pDoc->GetNCBlockSize() )	// �}���O < �}����
			g_pDoc->GetNCblock(n)->SetBlockFlag(NCF_FOLDER);
		return n;
	}

	return -1;
}

// ϸ���۸��т̌���
int NC_SearchMacroProgram(CNCblock* pBlock)
{
	extern	const	int		g_nDefaultMacroID[];	// MCOption.cpp

	CString	strBlock(pBlock->GetStrGcode());
	const CMCOption* pMCopt = AfxGetNCVCApp()->GetMCOption();

	if ( !regex_search((LPCTSTR)strBlock, g_reMacroStr) )
		return -1;
	// 5�K�w�ȏ�̌Ăяo���ʹװ(4�K�w�܂�)
	if ( g_nSubprog+1 >= 5 ) {
		pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_M98L);
		return -1;
	}

	// ϸۈ����̉��
	CString	strMacroFile,	// �ꎞ�o��̧��
			strArgv(pMCopt->GetMacroStr(MCMACROARGV)),	// ����
			strKey, strPath, strFile;
	int		nID;
	TCHAR	szPath[_MAX_PATH], szFile[_MAX_PATH];
	for ( int i=0; i<5/*SIZEOF(g_nDefaultMacroID)*/; i++ ) {
		nID = g_nDefaultMacroID[i];
		strKey = pMCopt->MakeMacroCommand(nID);
		switch ( i ) {
		case 0:		// MachineFile
			strArgv.Replace(strKey, pMCopt->GetMCHeadFileName());
			break;
		case 1:		// MacroCode
			strArgv.Replace(strKey, strBlock);
			break;
		case 2:		// MacroFolder
			strArgv.Replace(strKey, pMCopt->GetMacroStr(MCMACROFOLDER));
			break;
		case 3:		// CurrentFolder
			::Path_Name_From_FullPath(g_pDoc->GetCurrentFileName(), strPath, strFile);
			strArgv.Replace(strKey, strPath.Left(strPath.GetLength()-1));
			break;
		case 4:		// MacroResult
			::GetTempPath(_MAX_PATH, szPath);
			::GetTempFileName(szPath, AfxGetNCVCApp()->GetDocExtString(TYPE_NCD).Right(3)/*ncd*/,
				0, szFile);
			strMacroFile = szFile;
			strArgv.Replace(strKey, szFile);
			break;
		}
	}
	// ϸەϊ�I/F�N��
	AfxGetNCVCMainWnd()->CreateOutsideProcess(
		pMCopt->GetMacroStr(MCMACROIF), strArgv, FALSE, TRUE);
	// ϸۓW�J�ꎞ̧�ق�o�^ -> �޷���Ĕj����ɏ���
	g_pDoc->AddMacroFile(strMacroFile);
	// ��ۯ��}��
	int	n = g_pDoc->GetNCBlockSize();
	if ( g_pDoc->SerializeInsertBlock(strMacroFile, n, NCF_AUTOREAD, FALSE) ) {
		// �}����ۯ��̍ŏ�����NCF_FOLDER
		if ( n < g_pDoc->GetNCBlockSize() )	// ��ۯ��}�������s�̉\��������
			g_pDoc->GetNCblock(n)->SetBlockFlag(NCF_FOLDER);
		return n;
	}

	return -1;
}

// ������ڲ����ނ̌���
int NC_SearchAutoBreak(CNCblock* pBlock)
{
	if ( regex_search((LPCTSTR)(pBlock->GetStrBlock()), g_reAutoBreak) )
		pBlock->SetBlockFlag(NCF_BREAK);

	return 0;	// dummy
}

int NC_NoSearch(CNCblock*)
{
	return -1;
}

void MakeChamferingObject(CNCblock* pBlock, CNCdata* pData1, CNCdata* pData2)
{
#ifdef _DEBUG
	CMagaDbg	dbg("MakeChamferingObject()", DBG_BLUE);
#endif
	// �ް�����
	if ( g_pDoc->IsNCDocFlag(NCDOC_LATHE) ) {
		// ����Ӱ�ނłͻ�߰Ă���Ȃ�
		pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_NOTLATHE);
		return;
	}
	if ( pData1->GetGtype() != G_TYPE || pData2->GetGtype() != G_TYPE ||
			pData1->GetGcode() < 1 || pData1->GetGcode() > 3 ||
			pData2->GetGcode() < 1 || pData2->GetGcode() > 3 ) {
		pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_GTYPE);
		return;
	}
	if ( pData1->GetPlane() != pData2->GetPlane() ) {
		pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_PLANE);
		return;
	}
	TCHAR	cCham = g_lpstrComma[0];
	if ( cCham != 'R' && cCham != 'C' ) {
		pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_CHAMFERING);
		return;
	}

	double	r1, r2, cr = fabs(atof(g_lpstrComma + 1));
	CPointD	pts, pte, pto, ptOffset(g_pDoc->GetOffsetOrig());
	boost::optional<CPointD>	ptResult;
	BOOL	bResult;

	// �v�Z�J�n
	if ( cCham == 'C' )
		r1 = r2 = cr;
	else {
		// ��ŰR�̏ꍇ�́C�ʎ��ɑ�������C�l�̌v�Z
		tie(bResult, pto, r1, r2) = pData1->CalcRoundPoint(pData2, cr);
		if ( !bResult ) {
			pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_INTERSECTION);
			return;
		}
		pto -= ptOffset;
	}

	// pData1(�O�̵�޼ު��)�̏I�_��␳
	ptResult = pData1->SetChamferingPoint(FALSE, r1);
	if ( !ptResult ) {
		pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_LENGTH);
		return;
	}
	pts = *ptResult - ptOffset;
	// pData2(���̵�޼ު��)�̎n�_��␳
	ptResult = pData2->SetChamferingPoint(TRUE, r2);
	if ( !ptResult ) {
		pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_LENGTH);
		return;
	}
	pte = *ptResult - ptOffset;

#ifdef _DEBUG
	if ( !g_pDoc->IsNCDocFlag(NCDOC_THUMBNAIL) ) {
		dbg.printf("%c=%f, %f", cCham, r1, r2);
		dbg.printf("pts=(%f, %f)", pts.x, pts.y);
		dbg.printf("pte=(%f, %f)", pte.x, pte.y);
	}
#endif

	// pts, pte �Ŗʎ���޼ު�Ă̐���
	NCARGV	ncArgv;
	ZeroMemory(&ncArgv, sizeof(NCARGV));
	ncArgv.bAbs			= TRUE;
	ncArgv.nSpindle		= pData1->GetSpindle();
	ncArgv.dFeed		= pData1->GetFeed();
	ncArgv.dEndmill		= pData1->GetEndmill();
	ncArgv.nEndmillType	= pData1->GetEndmillType();
	ncArgv.bG98			= pData1->GetG98();
	ncArgv.nc.nLine		= pData1->GetBlockLineNo();
	ncArgv.nc.nGtype	= G_TYPE;
	ncArgv.nc.enPlane	= pData1->GetPlane();
	// ���W�l�̾��
	switch ( pData1->GetPlane() ) {
	case XY_PLANE:
		ncArgv.nc.dValue[NCA_X] = pte.x;
		ncArgv.nc.dValue[NCA_Y] = pte.y;
		ncArgv.nc.dwValFlags = NCD_X|NCD_Y;
		break;
	case XZ_PLANE:
		ncArgv.nc.dValue[NCA_X] = pte.x;
		ncArgv.nc.dValue[NCA_Z] = pte.y;
		ncArgv.nc.dwValFlags = NCD_X|NCD_Z;
		break;
	case YZ_PLANE:
		ncArgv.nc.dValue[NCA_Y] = pte.x;
		ncArgv.nc.dValue[NCA_Z] = pte.y;
		ncArgv.nc.dwValFlags = NCD_Y|NCD_Z;
		break;
	}

	if ( cCham == 'C' )
		ncArgv.nc.nGcode = 1;
	else {
		// ��ŰR�̏ꍇ�́C���߂���ŰR�̒��S(pto)�����]�������v�Z
		double	pa, pb;
		pts -= pto;		pte -= pto;
		if ( (pa=atan2(pts.y, pts.x)) < 0.0 )
			pa += 360.0*RAD;
		if ( (pb=atan2(pte.y, pte.x)) < 0.0 )
			pb += 360.0*RAD;
		if ( fabs(pa-pb) > 180.0*RAD ) {
			if ( pa > pb )
				pa -= 360.0*RAD;
			else
				pb -= 360.0*RAD;
		}
		ncArgv.nc.nGcode = pa > pb ? 2 : 3;
		ncArgv.nc.dValue[NCA_R] = cr;
		ncArgv.nc.dwValFlags |= NCD_R;
	}
	// ���ɓo�^���ꂽ�P�O�ɖʎ���޼ު�Ă�}��
	g_pDoc->DataOperation(pData1, &ncArgv, g_pDoc->GetNCsize()-1, NCINS);
}

double FeedAnalyze_Dot(const string& str)
{
	return fabs(GetNCValue(str));
}

double FeedAnalyze_Int(const string& str)
{
	return fabs(atof(str.c_str()));
}

void SetEndmillDiameter(const string& str)
{
#ifdef _DEBUG
	CMagaDbg	dbg("SetEndmillDiameter()\nStart", DBG_MAGENTA);
#endif
	const CMCOption* pMCopt = AfxGetNCVCApp()->GetMCOption();
	boost::optional<double> dResult = pMCopt->GetToolD( atoi(str.c_str()) );
	if ( dResult ) {
		g_ncArgv.dEndmill = *dResult;	// �̾�Ă͔��a�Ȃ̂ŁA���̂܂܎g�p
#ifdef _DEBUG
		if ( !g_pDoc->IsNCDocFlag(NCDOC_THUMBNAIL) )
			dbg.printf("Endmill=%f from T-No.%d", g_ncArgv.dEndmill, atoi(str.c_str()));
#endif
	}
#ifdef _DEBUG
	else {
		if ( !g_pDoc->IsNCDocFlag(NCDOC_THUMBNAIL) )
			dbg.printf("Endmill T-No.%d nothing", atoi(str.c_str()));
	}
#endif
}

void SetEndmillDiameter_fromComment(double d)
{
	// CCommentParser ��ͱ���݂���̌Ăяo��
	g_ncArgv.dEndmill = d / 2.0;
#ifdef _DEBUG
	if ( !g_pDoc->IsNCDocFlag(NCDOC_THUMBNAIL) ) {
		CMagaDbg	dbg("SetEndmillDiameter()\nStart", DBG_MAGENTA);
		dbg.printf("Endmill=%f from CommentParser", g_ncArgv.dEndmill);
	}
#endif
}

void SetEndmillType(char c)
{
	g_ncArgv.nEndmillType = c - '0';	// �P�����𐔒l�ɕϊ�
#ifdef _DEBUG
	if ( !g_pDoc->IsNCDocFlag(NCDOC_THUMBNAIL) ) {
		CMagaDbg	dbg("SetEndmillType()\nStart", DBG_MAGENTA);
		dbg.printf("EndmillType=%d from CommentParser", g_ncArgv.nEndmillType);
	}
#endif
}

void SetWorkRect_fromComment(vector<double>& vWork)
{
	CRect3D		rc;
	CPoint3D	pt;

	// �s�׍H�₯��vWork�̐����s��Ȃ̂ŗL�����ް��������o��
	for ( size_t i=0; i<vWork.size() && i<NCXYZ*2; i++ ) {
		switch ( i ) {
		case 0:		// ��
			rc.right = vWork[0];
			break;
		case 1:		// ���s
			rc.bottom = vWork[1];
			break;
		case 2:		// ����
			rc.high = vWork[2];
			break;
		case 3:		// X�̾��
			pt.x = vWork[3];
			break;
		case 4:		// Y�̾��
			pt.y = vWork[4];
			break;
		case 5:		// Z�̾��
			pt.z = vWork[5];
			break;
		}
	}
	rc.OffsetRect(pt);

	// �޷���Ă��ێ�����ܰ���`�̍X�V
	g_pDoc->SetWorkRectOrg(rc);

#ifdef _DEBUG
	if ( !g_pDoc->IsNCDocFlag(NCDOC_THUMBNAIL) ) {
		CMagaDbg	dbg("SetWorkRect_fromComment()\nStart", DBG_MAGENTA);
		dbg.printf("(%f,%f)-(%f,%f)", rc.left, rc.top, rc.right, rc.bottom);
		dbg.printf("(%f,%f)", rc.low, rc.high);
	}
#endif
}

void SetLatheRect_fromComment(vector<double>& vLathe)
{
#ifdef _DEBUG
	CMagaDbg	dbg("SetLatheRect_fromComment()\nStart", DBG_MAGENTA);
#endif
	// ����Ӱ�ނ��׸�ON
	g_pDoc->SetLatheViewMode();
	// �Ăяo���֐��̎w��
	g_pfnIsGcode = &IsGcodeObject_Lathe;
	g_pfnCheckGcodeOther = &CheckGcodeOther_Lathe;
	// ��̫�ĕ��ʁFXZ_PLANE
	g_ncArgv.nc.enPlane = XZ_PLANE;

	// �ް��̎��o��
	for ( size_t i=0; i<vLathe.size() && i<3; i++ ) {
		switch ( i ) {
		case 0:		// ܰ��a
			g_pDoc->SetWorkLatheR( vLathe[0] / 2.0 );	// ���a�ŕۊ�
#ifdef _DEBUG
			if ( !g_pDoc->IsNCDocFlag(NCDOC_THUMBNAIL) )
				dbg.printf("r=%f", vLathe[0]/2);
#endif
			break;
		case 2:		// z1, z2
			// z2 ������Ƃ�����
			g_pDoc->SetWorkLatheZ( vLathe[1], vLathe[2] );
#ifdef _DEBUG
			if ( !g_pDoc->IsNCDocFlag(NCDOC_THUMBNAIL) )
				dbg.printf("(%f)-(%f)", vLathe[1], vLathe[2]);
#endif
			break;
		}
	}
}

CString SearchFolder(regex& r)
{
	extern	LPCTSTR	gg_szWild;	// "*.";
	CString	strResult, strExt;
	LPVOID	pFunc;

	for ( int i=0; i<SIZEOF(g_strSearchFolder); i++ ) {
		if ( g_strSearchFolder[i].IsEmpty() )
			continue;
		// ̫��ނ�W���g���q�Ō���
		strResult = SearchFolder_Sub(i,
			gg_szWild + AfxGetNCVCApp()->GetDocExtString(TYPE_NCD).Right(3),	// "." �����uncd�v
			r);
		if ( !strResult.IsEmpty() )
			return strResult;
		// �o�^�g���q�ł�̫��ތ���
		for ( int j=0; j<2/*EXT_ADN,EXT_DLG*/; j++ ) {
			const CMapStringToPtr* pMap = AfxGetNCVCApp()->GetDocTemplate(TYPE_NCD)->GetExtMap((EXTTYPE)j);
			for ( POSITION pos=pMap->GetStartPosition(); pos; ) {
				pMap->GetNextAssoc(pos, strExt, pFunc);
				strResult = SearchFolder_Sub(i, gg_szWild + strExt, r);
				if ( !strResult.IsEmpty() )
					return strResult;
			}
		}
	}

	return CString();
}

CString	SearchFolder_Sub(int n, LPCTSTR lpszFind, regex& r)
{
	CString	strFile;
	HANDLE	hFind;
	WIN32_FIND_DATA	fd;
	DWORD	dwFlags = FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_SYSTEM;

	if ( (hFind=::FindFirstFile(g_strSearchFolder[n]+lpszFind, &fd)) != INVALID_HANDLE_VALUE ) {
		do {
			strFile = g_strSearchFolder[n] + fd.cFileName;
			if ( !(fd.dwFileAttributes & dwFlags) &&
					regex_search(fd.cFileName, r) &&
					SearchProgNo(strFile, r) )
				return strFile;
		} while ( ::FindNextFile(hFind, &fd) );
		::FindClose(hFind);
	}

	return CString();
}

BOOL SearchProgNo(LPCTSTR lpszFile, regex& r)
{
	// ̧��ϯ��ݸނ�����۸��єԍ�(������)�̑��݊m�F
	BOOL	bResult = FALSE;
	CFile	fp;

	if ( fp.Open(lpszFile, CFile::modeRead) ) {
		HANDLE hMap = CreateFileMapping((HANDLE)(fp.m_hFile), NULL,
							PAGE_READONLY, 0, 0, NULL);
		if ( hMap ) {
			LPCTSTR pMap = (LPCTSTR)MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
			if ( pMap ) {
				if ( regex_search(pMap, r) )
					bResult = TRUE;
				UnmapViewOfFile(pMap);
			}
			CloseHandle(hMap);
		}
		fp.Close();
	}

	return bResult;
}

void CycleInterpolate(void)
{
	extern	const	DWORD	g_dwSetValFlags[];

	// �O�̂��߂�����
	if ( g_ncArgv.nc.nGcode<81 || g_ncArgv.nc.nGcode>89 ) {
		g_Cycle.clear();
		return;
	}

	int	z;
	// ����ʂɑ΂��钼����
	switch ( g_ncArgv.nc.enPlane ) {
	case XY_PLANE:
		z = NCA_Z;
		break;
	case XZ_PLANE:
		z = NCA_Y;
		break;
	case YZ_PLANE:
		z = NCA_X;
		break;
	}

	// �Œ軲�ق̍��W���
	if ( g_ncArgv.nc.dwValFlags & g_dwSetValFlags[z] )
		g_Cycle.dVal[0] = g_ncArgv.nc.dValue[z];
	else if ( g_Cycle.dVal[0] != HUGE_VAL ) {
		g_ncArgv.nc.dValue[z] = g_Cycle.dVal[0];
		g_ncArgv.nc.dwValFlags |= g_dwSetValFlags[z];
	}

	if ( g_ncArgv.nc.dwValFlags & NCD_R )
		g_Cycle.dVal[1] = g_ncArgv.nc.dValue[NCA_R];
	else if ( g_Cycle.dVal[1] != HUGE_VAL ) {
		g_ncArgv.nc.dValue[NCA_R] = g_Cycle.dVal[1];
		g_ncArgv.nc.dwValFlags |= NCD_R;
	}

	if ( g_ncArgv.nc.dwValFlags & NCD_P )
		g_Cycle.dVal[2] = g_ncArgv.nc.dValue[NCA_P];
	else if ( g_Cycle.dVal[2] != HUGE_VAL ) {
		g_ncArgv.nc.dValue[NCA_P] = g_Cycle.dVal[2];
		g_ncArgv.nc.dwValFlags |= NCD_P;
	}
}

void G68RoundCheck(CNCblock* pBlock)
{
	if ( g_pDoc->IsNCDocFlag(NCDOC_LATHE) ) {
		// ����Ӱ�ނłͻ�߰Ă���Ȃ�
		pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_NOTLATHE);
		return;
	}

	// �e�w�����W������
	g_ncArgv.g68.dOrg[NCA_X] =
	g_ncArgv.g68.dOrg[NCA_Y] =
	g_ncArgv.g68.dOrg[NCA_Z] = 0.0;
	switch ( g_ncArgv.nc.enPlane ) {
	case XY_PLANE:
		if ( g_ncArgv.nc.dwValFlags & NCD_Z )
			pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_PLANE);
		else {
			g_ncArgv.g68.bG68    = TRUE;
			g_ncArgv.g68.enPlane = XY_PLANE;
			g_ncArgv.g68.dOrg[NCA_X] = g_ncArgv.nc.dValue[NCA_X];
			g_ncArgv.g68.dOrg[NCA_Y] = g_ncArgv.nc.dValue[NCA_Y];
		}
		break;
	case XZ_PLANE:
		if ( g_ncArgv.nc.dwValFlags & NCD_Y )
			pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_PLANE);
		else {
			g_ncArgv.g68.bG68    = TRUE;
			g_ncArgv.g68.enPlane = XZ_PLANE;
			g_ncArgv.g68.dOrg[NCA_X] = g_ncArgv.nc.dValue[NCA_X];
			g_ncArgv.g68.dOrg[NCA_Z] = g_ncArgv.nc.dValue[NCA_Z];
		}
		break;
	case YZ_PLANE:
		if ( g_ncArgv.nc.dwValFlags & NCD_X )
			pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_PLANE);
		else {
			g_ncArgv.g68.bG68    = TRUE;
			g_ncArgv.g68.enPlane = YZ_PLANE;
			g_ncArgv.g68.dOrg[NCA_Y] = g_ncArgv.nc.dValue[NCA_Y];
			g_ncArgv.g68.dOrg[NCA_Z] = g_ncArgv.nc.dValue[NCA_Z];
		}
		break;
	}

	if ( g_ncArgv.g68.bG68 ) {
		if ( g_ncArgv.nc.dwValFlags & NCD_R )
			g_ncArgv.g68.dRound = g_ncArgv.nc.dValue[NCA_R] * RAD;	// ׼ޱ݂Ŋi�[
		else {
			pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_ORDER);
			g_ncArgv.g68.bG68 = FALSE;		// G68��ݾ�
		}
	}
}

void G68RoundClear(void)
{
	g_ncArgv.g68.bG68	= FALSE;
	g_ncArgv.g68.enPlane= XY_PLANE;
	g_ncArgv.g68.dRound	= 0.0;
	g_ncArgv.g68.dOrg[NCA_X] =
	g_ncArgv.g68.dOrg[NCA_Y] =
	g_ncArgv.g68.dOrg[NCA_Z] = 0.0;
}

// �ϐ�������
void InitialVariable(void)
{
	int		i;
	const CMCOption*	pMCopt = AfxGetNCVCApp()->GetMCOption();
	const CViewOption*	pVopt  = AfxGetNCVCApp()->GetViewOption();

	g_pfnFeedAnalyze = pMCopt->GetFDot()==0 ? &FeedAnalyze_Int : &FeedAnalyze_Dot;
	g_ncArgv.nc.nGtype = G_TYPE;
	g_ncArgv.nc.nGcode = pMCopt->GetModalSetting(MODALGROUP0);
	switch ( pMCopt->GetModalSetting(MODALGROUP1) ) {
	case 1:
		g_ncArgv.nc.enPlane = XZ_PLANE;
		break;
	case 2:
		g_ncArgv.nc.enPlane = YZ_PLANE;
		break;
	default:	// or �u0�v
		g_ncArgv.nc.enPlane = XY_PLANE;
		break;
	}
	g_ncArgv.nc.nErrorCode = 0;
	g_ncArgv.nc.dwValFlags = 0;
	if ( pMCopt->GetFlag(MC_FLG_LATHE) ) {	// pDoc->IsNCDocFlag(NCDOC_LATHE)
		vector<double>	dDummy;
		SetLatheRect_fromComment(dDummy);	// ����Ӱ�ނ֋����ؑ�
		g_ncArgv.nc.dValue[NCA_X] = pMCopt->GetInitialXYZ(NCA_Z);
		g_ncArgv.nc.dValue[NCA_Y] = 0.0;
		g_ncArgv.nc.dValue[NCA_Z] = pMCopt->GetInitialXYZ(NCA_X) / 2.0;
	}
	else {
		g_pfnIsGcode = &IsGcodeObject_Milling;
		g_pfnCheckGcodeOther = &CheckGcodeOther_Milling;
		for ( i=0; i<NCXYZ; i++ )
			g_ncArgv.nc.dValue[i] = pMCopt->GetInitialXYZ(i);
	}
	for ( i=0; i<NCXYZ; i++ )
		g_dValue[i] = 0.0;
	for ( ; i<VALUESIZE; i++ )
		g_ncArgv.nc.dValue[i] = 0.0;
	g_ncArgv.bAbs		= pMCopt->GetModalSetting(MODALGROUP3) == 0 ? TRUE : FALSE;
	g_ncArgv.bG98		= pMCopt->GetModalSetting(MODALGROUP4) == 0 ? TRUE : FALSE;
	g_ncArgv.nSpindle	= 0;
	g_ncArgv.dFeed		= pMCopt->GetFeed();
	g_ncArgv.dEndmill	= pVopt->GetDefaultEndmill();
	g_ncArgv.nEndmillType	= pVopt->GetDefaultEndmillType();

	g_Cycle.clear();
	G68RoundClear();

	g_dwValFlags = 0;
	g_nSubprog = 0;
	g_lpstrComma = NULL;

	if ( g_pDoc->IsNCDocFlag(NCDOC_THUMBNAIL) ) {
		g_pfnSearchMacro = &NC_NoSearch;
		g_pfnSearchAutoBreak = &NC_NoSearch;
	}
	else {
		if ( pMCopt->GetMacroStr(MCMACROCODE).IsEmpty() || pMCopt->GetMacroStr(MCMACROIF).IsEmpty() )
			g_pfnSearchMacro = &NC_NoSearch;
		else {
			g_reMacroStr = pMCopt->GetMacroStr(MCMACROCODE);
			g_pfnSearchMacro = &NC_SearchMacroProgram;
		}
		if ( pMCopt->GetAutoBreakStr().IsEmpty() )
			g_pfnSearchAutoBreak = &NC_NoSearch;
		else {
			g_reAutoBreak = pMCopt->GetAutoBreakStr();
			g_pfnSearchAutoBreak = &NC_SearchAutoBreak;
		}
	}

	// ���ĂƎw��̫��ނ̏�����
	CString	strFile;	// dummy
	::Path_Name_From_FullPath(g_pDoc->GetCurrentFileName(), g_strSearchFolder[0], strFile);
	g_strSearchFolder[1] = pMCopt->GetMacroStr(MCMACROFOLDER);
	for ( i=0; i<SIZEOF(g_strSearchFolder); i++ ) {
		if ( !g_strSearchFolder[i].IsEmpty() && g_strSearchFolder[i].Right(1) != "\\" )
			g_strSearchFolder[i] += "\\";
	}
	if ( g_strSearchFolder[0].CompareNoCase(g_strSearchFolder[1]) == 0 )
		g_strSearchFolder[1].Empty();
}
