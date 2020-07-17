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
#include "boost/spirit/include/qi.hpp"
#include "boost/spirit/include/phoenix.hpp"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#define	_DEBUG_GSPIRIT
#endif

using std::string;
using namespace boost;
using namespace boost::spirit;

extern	const	DWORD	g_dwSetValFlags[];
extern	LPCTSTR			g_szNdelimiter; // "XYZUVWIJKRPLDH"
extern	LPCTSTR			g_szNCcomment[];// "Endmill", "WorkRect", etc.
static	CThreadDlg*		g_pParent;
static	CNCDoc*			g_pDoc;
static	NCARGV			g_ncArgv;		// NCVCdefine.h
static	int				g_nWorkRect,
						g_nWorkCylinder,
						g_nLatheView,
						g_nWireView,
						g_nSubprog;		// �����۸��ьĂяo���̊K�w
static	double			g_dWorkRect[NCXYZ*2],
						g_dWorkCylinder[2+NCXYZ],
						g_dLatheView[3],
						g_dWireView,
						g_dToolPos[NCXYZ];
static	DWORD			g_dwToolPosFlags;	// ToolPos�p
static	LPTSTR			g_lpstrComma;		// ������ۯ��Ƃ̌v�Z
//
typedef	BOOL	(*PFNISTHREAD)(void);
static	BOOL	IsThreadContinue(void)
{
	return g_pParent->IsThreadContinue();
}
static	BOOL	IsThreadThumbnail(void)
{
	return TRUE;
}
static	PFNISTHREAD		g_pfnIsThread;
#define	IsThread()		(*g_pfnIsThread)()
#define	IsThumbnail()	g_pDoc->IsDocFlag(NCDOC_THUMBNAIL)

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
static	CYCLE_INTERPOLATE	g_Cycle;
static	void	CycleInterpolate(void);

// IsGcodeObject() �߂�l
static	enum	ENGCODEOBJ {NOOBJ, MAKEOBJ, MAKEOBJ_NOTMODAL};

// G68���W��]
static	void	G68RoundCheck(CNCblock*);
static	void	G68RoundClear(void);
// ð�߉��H���
static	void	TaperClear(void);
//
static	void	InitialVariable(void);
// ��͊֐�
static	int			NC_GSeparater(int, CNCdata*&);
static	CNCdata*	AddGcode(CNCblock*, CNCdata*, int);
static	CNCdata*	AddM98code(CNCblock*, CNCdata*, int);
static	int			CallSubProgram(CNCblock*, CNCdata*&);
typedef	ENGCODEOBJ	(*PFNCHECKGCODE)(int);
static	ENGCODEOBJ	IsGcodeObject_Milling(int);
static	ENGCODEOBJ	IsGcodeObject_Wire(int);
static	ENGCODEOBJ	IsGcodeObject_Lathe(int);
static	PFNCHECKGCODE	g_pfnIsGcode;
typedef	int			(*PFNCHECKGCODEOTHER)(int);
static	int			CheckGcodeOther_Milling(int);
static	int			CheckGcodeOther_Wire(int);
static	int			CheckGcodeOther_Lathe(int);
static	PFNCHECKGCODEOTHER	g_pfnCheckGcodeOther;
// �ʎ��E��ŰR����
static	void	MakeChamferingObject(CNCblock*, CNCdata*, CNCdata*);
// F���Ұ�, �޳�َ��Ԃ̉���
typedef double (*PFNFEEDANALYZE)(const string&);
static	double	FeedAnalyze_Dot(const string&);
static	double	FeedAnalyze_Int(const string&);
static	PFNFEEDANALYZE	g_pfnFeedAnalyze;
// �H��a���
static	void	SetEndmillDiameter(const string&);
// ð�ߊp�x
static	int		SetTaperAngle(const string&);
// �H��ʒu���
static	CNCdata*	SetToolPosition_fromComment(CNCblock*, CNCdata*);
// ܰ���`���ݒ�
static	void	SetWorkRect_fromComment(void);
static	void	SetWorkCylinder_fromComment(void);
static	void	SetLatheRect_fromComment(void);
static	void	SetWireRect_fromComment(void);
// �����ہCϸۂ̌���
static	CString	g_strSearchFolder[2];	// ���ĂƎw��̫���
static	CString	SearchFolder(const regex&);
static	CString	SearchFolder_Sub(int, LPCTSTR, const regex&);
static	BOOL	SearchProgNo(LPCTSTR, const regex&);
static	regex	g_reMacroStr;
static	int		NC_SearchSubProgram(int*);
typedef	int		(*PFNBLOCKPROCESS)(const string&, CNCblock*);
static	int		NC_SearchMacroProgram(const string&, CNCblock*);
static	int		NC_NoSearch(const string&, CNCblock*);
static	PFNBLOCKPROCESS	g_pfnSearchMacro;
// ������ڲ����ތ���
static	regex	g_reAutoBreak;
static	int		NC_SearchAutoBreak(const string&, CNCblock*);
static	PFNBLOCKPROCESS	g_pfnSearchAutoBreak;

//////////////////////////////////////////////////////////////////////

// �f�R�[�h�̏����_����
static inline	int		_GetGcode(const string& str)
{
	double	dResult = atof(str.c_str());
	if ( str.find('.') != string::npos )
		dResult += 1000.0;		// �����_������� ����+1000
	return (int)dResult;
}
// ���l�ϊ�( 1/1000 �ް��𔻒f����)
static inline	double	_GetNCValue(const string& str)
{
	double	dResult = atof(str.c_str());
	if ( str.find('.') == string::npos )
		dResult /= 1000.0;		// �����_���Ȃ���� 1/1000
	return dResult;
}
// g_lpstrComma ���ް�����
static inline	void	_SetStrComma(const string& strComma)
{
	if ( g_lpstrComma )
		delete[]	g_lpstrComma;
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
#ifdef _DEBUG_FILEOPEN		// NCVC.h
	extern	CTime	dbgtimeFileOpen;	// NCVC.cpp
	CTime	t2 = CTime::GetCurrentTime();
	CTimeSpan ts = t2 - dbgtimeFileOpen;
	CString	strTime( ts.Format("%H:%M:%S") );
	g_dbg.printf("NCDtoXYZ_Thread() %s", strTime);
	dbgtimeFileOpen = t2;
#endif
#endif

	int			i, nLoopCnt,
				nResult = IDOK;
	CNCdata*	pDataFirst = NULL;
	CNCdata*	pData;	// �P�O�̐�����޼ު��

	// �ϐ�������
	LPNCVCTHREADPARAM	pParam = reinterpret_cast<LPNCVCTHREADPARAM>(pVoid);
	g_pParent = pParam->pParent;
	g_pDoc  = static_cast<CNCDoc*>(pParam->pDoc);
	g_pfnIsThread = g_pParent ? &IsThreadContinue : &IsThreadThumbnail;
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
	if ( !IsThumbnail() )
		dbg.printf("LoopCount=%d", nLoopCnt);
#endif

	try {
		// �P�O�̵�޼ު�ĎQ�Ƃ�NULL�Q�Ƃ��Ȃ�����
		pDataFirst = pData = new CNCdata(&g_ncArgv);
		// 1�s(1��ۯ�)��͂���޼ު�Ă̓o�^
		for ( i=0; i<nLoopCnt && IsThread(); i++ ) {
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
		delete[]	g_lpstrComma;
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

//typedef qi::rule<string::iterator, qi::space_type, string()>	SkipperType;
//typedef qi::rule<string::iterator>	SkipperType;
//static	SkipperType	skip_p = ascii::space | (qi::char_('(') >> *(qi::char_ - ')') >> ')');

//	NC��۸��щ��
template<typename Iterator>
//struct CGcodeParser : qi::grammar<Iterator, SkipperType, string()>
struct CGcodeParser : qi::grammar<Iterator, qi::space_type, string()>
{
//	qi::rule<Iterator, SkipperType, string()>	rr;
	qi::rule<Iterator, qi::space_type, string()>	rr;

	CGcodeParser() : CGcodeParser::base_type(rr) {
		using qi::char_;
		using qi::double_;
		rr = qi::raw[ (qi::upper >> double_) |
				(char_(',') >> (char_('R')|'C') >> double_ ) ];
		// �ق�Ƃ́��ŃA�N�V����������o������
//		rr = +(qi::upper >> double_)[hoge()] >>
//				!(char_(',') >> (char_('R')|'C') >> double_ )[hoge()];
	}
};
/*
// NC��۸��т���͂���Ƃ��̽�����߰�
template<typename Iterator>
struct CCommentSkip : qi::grammar<Iterator>
{
	qi::rule<Iterator>	rc;
	
	CCommentSkip() : CCommentSkip::base_type(rc) {
		using qi::char_;
		rc = ascii::space | (char_('(') >> *(char_ - ')') >> ')');
	};
};
*/

//	���ĕ�������
template<typename Iterator>
struct CCommentParser : qi::grammar<Iterator, qi::space_type>
{
	qi::rule<Iterator, qi::space_type>		rr,
		rs1, rs2, rs3, rs4, rs5, rs6,
		rr1, r11, r12, rr2, rr3, rr4, rr5, rr6;

	CCommentParser() : CCommentParser::base_type(rr) {
		using qi::no_case;
		using qi::char_;
		using qi::double_;

		// Endmill
		rs1 = no_case[ ascii::string(ENDMILL_S) | DRILL_S | TAP_S | REAMER_S ] >> '=';
		r11 = double_[_SetEndmill()] >> -no_case["mm"] >>
						-(',' >> qi::digit[_SetEndmillType()]);
		r12 = (char_('R')|'r') >> double_[_SetBallEndmill()] >>	// �ްٴ����ٕ\�L
						-no_case["mm"];
		rr1 = r11 | r12;
		// WorkRect
		rs2 = no_case[ WORKRECT_S ] >> '=';
		rr2 = double_[_SetWorkRect()] % ',';
		// WorkCylinder
		rs3 = no_case[ WORKCYLINDER_S ] >> '=';
		rr3 = double_[_SetWorkCylinder()] % ',';
		// ViewMode
		rs4 = no_case[ LATHEVIEW_S ] >> '=';
		rr4 = double_[_SetLatheView()] % ',';
		rs5 = no_case[ WIREVIEW_S ] >> '=';
		rr5 = double_[_SetWireView()];
		// ToolPos
		rs6 = no_case[ TOOLPOS_S ] >> '=';
		rr6 = -double_[_ToolPosX()] >>
				-(',' >> -double_[_ToolPosY()] >> -(',' >> -double_[_ToolPosZ()]));
		//
		rr  = *(char_ - '(') >> '(' >> *(char_ - (rs1|rs2|rs3|rs4|rs5|rs6)) >>
				// �R�����g�s��S�ď��������邽�߂�
				// ���́��Ƃ��Ă���
				*( rs1>>rr1 | rs2>>rr2 | rs3>>rr3 | rs4>>rr4 | rs5>>rr5 | rs6>>rr6 );
	}

	// �����ٌa
	struct	_SetEndmill {
		void operator()(const double& d, qi::unused_type, qi::unused_type) const {
			g_ncArgv.dEndmill = d / 2.0;
#ifdef _DEBUG_GSPIRIT
			if ( !IsThumbnail() ) {
				CMagaDbg	dbg("_SetEndmill()", DBG_MAGENTA);
				dbg.printf("Endmill=%f", g_ncArgv.dEndmill);
			}
#endif
		}
	};
	// �ްٴ����ٕ\�L
	struct	_SetBallEndmill {
		void operator()(double& d, qi::unused_type, qi::unused_type) const {
			g_ncArgv.dEndmill = d;
			g_ncArgv.nEndmillType = 1;
#ifdef _DEBUG_GSPIRIT
			if ( !IsThumbnail() ) {
				CMagaDbg	dbg("SetBallEndmill()", DBG_MAGENTA);
				dbg.printf("BallEndmill=R%f", g_ncArgv.dEndmill);
			}
#endif
		}
	};
	// ����������
	struct _SetEndmillType {
		void operator()(char& c, qi::unused_type, qi::unused_type) const {
			g_ncArgv.nEndmillType = c - '0';	// �P�����𐔒l�ɕϊ�
#ifdef _DEBUG_GSPIRIT
			if ( !IsThumbnail() ) {
				CMagaDbg	dbg("SetEndmillType()", DBG_MAGENTA);
				dbg.printf("EndmillType=%d", g_ncArgv.nEndmillType);
			}
#endif
		}
	};
	// ܰ���`
	struct _SetWorkRect {
		void operator()(double& d, qi::unused_type, qi::unused_type) const {
			if ( g_nWorkRect < SIZEOF(g_dWorkRect) )
				g_dWorkRect[g_nWorkRect++] = d;
		}
	};
	// ܰ��~��
	struct _SetWorkCylinder {
		void operator()(double& d, qi::unused_type, qi::unused_type) const {
			if ( g_nWorkCylinder < SIZEOF(g_dWorkCylinder) )
				g_dWorkCylinder[g_nWorkCylinder++] = d;
		}
	};
	// ���Օ\��Ӱ��
	struct _SetLatheView {
		void operator()(double& d, qi::unused_type, qi::unused_type) const {
			if ( g_nLatheView < SIZEOF(g_dLatheView) )
				g_dLatheView[g_nLatheView++] = d;
		}
	};
	// ܲԉ��H�@�\��Ӱ��
	struct _SetWireView {
		void operator()(double& d, qi::unused_type, qi::unused_type) const {
			if ( g_nWireView++ < 1 )
				g_dWireView = d;
		}
	};
	// �H��ʒu�ύX
	struct _ToolPosX {
		void operator()(double& d, qi::unused_type, qi::unused_type) const {
			g_dToolPos[NCA_X] = d;
			g_dwToolPosFlags |= NCD_X;
#ifdef _DEBUG_GSPIRIT
			if ( !IsThumbnail() ) {
				CMagaDbg	dbg("ToolPosX()", DBG_MAGENTA);
				dbg.printf("x=%f", d);
			}
#endif
		}
	};
	struct _ToolPosY {
		void operator()(double& d, qi::unused_type, qi::unused_type) const {
			g_dToolPos[NCA_Y] = d;
			g_dwToolPosFlags |= NCD_Y;
#ifdef _DEBUG_GSPIRIT
			if ( !IsThumbnail() ) {
				CMagaDbg	dbg("ToolPosY()", DBG_MAGENTA);
				dbg.printf("y=%f", d);
			}
#endif
		}
	};
	struct _ToolPosZ {
		void operator()(double& d, qi::unused_type, qi::unused_type) const {
			g_dToolPos[NCA_Z] = d;
			g_dwToolPosFlags |= NCD_Z;
#ifdef _DEBUG_GSPIRIT
			if ( !IsThumbnail() ) {
				CMagaDbg	dbg("ToolPosZ()", DBG_MAGENTA);
				dbg.printf("z=%f", d);
			}
#endif
		}
	};
};

// ��۰��ٕϐ��Ɉڂ����Ƃ�
// �ē��̂��тɕϐ����ݽ�ݽ������邱�Ƃ�h��
// ���ʁA��߰��UP
// --- �Ǝ�Skipper�̃R���p�C�����ʂ�Ȃ�...
//typedef CCommentSkip<string::iterator>				SkipperType;
//static	SkipperType									skip_p;
//static	CGcodeParser<string::iterator, SkipperType>	gr_p;
// ---
static	CGcodeParser<string::iterator>		gr_p;
static	CCommentParser<string::iterator>	comment_p;
// �s�{�ӂȂ��� regex_replace �Œu��
static	regex	reComment("\\([^\\)]*\\)");	// (hoge)��ϯ�

//////////////////////////////////////////////////////////////////////
// G���ނ̕���(�ċA�֐�)
int NC_GSeparater(int nLine, CNCdata*& pDataResult)
{
	string		strBlock,		// NC��ۯ��i1�s��͒P�ʁj
				strWord,		// ��͒P��
				strComma;		// ��ψȍ~�̕�����(������ۯ��֌p��)
	string::iterator	it;

	int			i, nCode,
				nNotModalCode = -1,	// Ӱ��ٕs�v��G����(��ۯ����ł̂ݗL��)
				nResult, nIndex;
	BOOL		bNCobj = FALSE, bNCval = FALSE, bNCsub = FALSE,
				bTcode = FALSE;
	ENGCODEOBJ	enGcode;
	CNCdata*	pData;
	CNCblock*	pBlock = g_pDoc->GetNCblock(nLine);

	// �ϐ�������
	g_ncArgv.nc.nLine		= nLine;
	g_ncArgv.nc.nErrorCode	= 0;
	g_ncArgv.nc.dwValFlags &= NCD_CLEARVALUE;	// 0xffff0000
	g_ncArgv.taper.bTonly	= FALSE;
	g_nWorkRect = g_nWorkCylinder = g_nLatheView = g_nWireView = 0;
	g_dwToolPosFlags		= 0;

	strBlock = pBlock->GetStrGcode();

#ifdef _DEBUG_GSPIRIT
	CMagaDbg	dbg("NC_Gseparate()", DBG_BLUE);
	CMagaDbg	dbg1(DBG_GREEN);
	if ( !IsThumbnail() )
		dbg.printf("No.%004d Line=%s", nLine+1, strBlock.c_str());
#endif

	// ��Ȳٕ\���̂Ƃ��͏������Ȃ�ٰ��
	if ( !IsThumbnail() ) {
		// ������ڲ����ތ���
		(*g_pfnSearchAutoBreak)(strBlock, pBlock);
	}

	// ���ĉ��(���Ӱ��, �����ٌa, ܰ���`���̎擾)
	it = strBlock.begin();
	if ( qi::phrase_parse(it, strBlock.end(), comment_p, qi::space) ) {
		if ( g_nWorkRect > 0 )
			SetWorkRect_fromComment();
		if ( g_nWorkCylinder > 0 )
			SetWorkCylinder_fromComment();
		if ( g_nLatheView > 0 )
			SetLatheRect_fromComment();
		if ( g_nWireView > 0 )
			SetWireRect_fromComment();
		if ( g_dwToolPosFlags )
			pDataResult = SetToolPosition_fromComment(pBlock, pDataResult);	// create dummy object
		// ��ɑ��������̂��߂ɃJ�b�R������
		strBlock = regex_replace(strBlock, reComment, "");
#ifdef _DEBUG_GSPIRIT
		if ( !IsThumbnail() )
			dbg.printf("--- [%s] --- Comment remove OK", strBlock.c_str());
#endif
	}

	// ϸےu�����
	if ( (nIndex=(*g_pfnSearchMacro)(strBlock, pBlock)) >= 0 ) {
		g_nSubprog++;
		// M98��޼ު�Ă�O�ԍ�(nIndex)�̓o�^
		pDataResult = AddM98code(pBlock, pDataResult, nIndex);
		// g_pfnSearchMacro ����ۯ����ǉ������\���A��
		// �����ł� nLoop �ϐ����g�킸�AȲè�ނ���ۯ����ނɂĔ���
		for ( i=nIndex; i<g_pDoc->GetNCBlockSize() && IsThread(); i++ ) {
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
	it = strBlock.begin();
	while ( IsThread() ) {
		strWord.clear();
//		if ( !qi::phrase_parse(it, strBlock.end(), gr_p, skip_p, strWord) )
		if ( !qi::phrase_parse(it, strBlock.end(), gr_p, qi::space, strWord) )
			break;
#ifdef _DEBUG_GSPIRIT
		if ( !IsThumbnail() ) {
			dbg1.printf("G Cut=%s", strWord.c_str());
		}
#endif
		switch ( strWord[0] ) {
		case 'M':
			// �O��̺��ނœo�^��޼ު�Ă�����Ȃ�
//			if ( bNCobj || bNCval ) {
			if ( bNCobj ) {
				// ��޼ު�Đ���
				pData = AddGcode(pBlock, pDataResult, nNotModalCode);
				// �ʎ���޼ު�Ă̓o�^
				if ( g_lpstrComma )
					MakeChamferingObject(pBlock, pDataResult, pData);
				pDataResult = pData;
				_SetStrComma(strComma);
				nNotModalCode = -1;
//				bNCobj = bNCval = FALSE;
				bNCobj = FALSE;
			}
			// �O��̺��ނŻ����یĂяo���������
			if ( bNCsub ) {
				bNCsub = FALSE;
				if ( CallSubProgram(pBlock, pDataResult) == 30 )
					return 30;	// �I������
			}
			nCode = _GetGcode(strWord.substr(1));
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
			default:
				return 0;	// �F���ł��Ȃ�M������ۯ��͈ȍ~�̺��ނ��������Ȃ�
			}
			break;
		case 'G':
			// ��Ɍ��݂̺��ގ�ʂ�����
			nCode = _GetGcode(strWord.substr(1));
			enGcode = (*g_pfnIsGcode)(nCode);	// IsGcodeObject_�`
			if ( enGcode == NOOBJ ) {
				nResult = (*g_pfnCheckGcodeOther)(nCode);
				if ( nResult > 0 )
					pBlock->SetNCBlkErrorCode(nResult);
				break;
			}
			// �O��̺��ނœo�^��޼ު�Ă�����Ȃ�
//			if ( bNCobj || bNCval ) {
			if ( bNCobj ) {
				pData = AddGcode(pBlock, pDataResult, nNotModalCode);
				if ( g_lpstrComma )
					MakeChamferingObject(pBlock, pDataResult, pData);
				pDataResult = pData;
				_SetStrComma(strComma);
				nNotModalCode = -1;
//				bNCval = FALSE;		// bNCobj �ر�s�v
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
			g_ncArgv.dFeed = (*g_pfnFeedAnalyze)(strWord.substr(1));
			break;
		case 'S':
			g_ncArgv.nSpindle = abs(atoi(strWord.substr(1).c_str()));
			break;
		case 'T':
			if ( g_pDoc->IsDocFlag(NCDOC_WIRE) ) {
				// ð�ߊp�x���
				nResult = SetTaperAngle(strWord.substr(1));
				if ( nResult > 0 )	
					pBlock->SetNCBlkErrorCode(nResult);
				else
					bTcode = TRUE;	// T���ގw��(ܲԂ̂�)
			}
			else {
				SetEndmillDiameter(strWord.substr(1));	// °ٔԍ�
			}
			break;
		case ',':
			if ( g_pDoc->IsDocFlag(NCDOC_WIRE) ) {
				// ܲ�Ӱ�ނŻ�߰Ă��ׂ���...
				string	strTmp = ::Trim(strWord.substr(1));
				if ( strTmp[0] == 'R' ) {
					g_ncArgv.nc.dValue[NCA_R] = _GetNCValue(strTmp.substr(1));
					g_ncArgv.nc.dwValFlags |= NCD_R;
				}
			}
			else {
				strComma = ::Trim(strWord.substr(1));	// ��ψȍ~���擾
#ifdef _DEBUG_GSPIRIT
				if ( !IsThumbnail() )
					dbg1.printf("strComma=%s", strComma.c_str());
#endif
			}
			break;
		case 'X':	case 'Y':	case 'Z':
		case 'U':	case 'V':	case 'W':
		case 'I':	case 'J':	case 'K':	case 'R':
		case 'P':	case 'L':
		case 'D':	case 'H':
			nCode = (int)(strchr(g_szNdelimiter, strWord[0]) - g_szNdelimiter);
			// �l�擾
			if ( g_Cycle.bCycle ) {		// 81�`89
				// �Œ軲�ق̓��ʏ���
				switch ( nCode ) {
				case NCA_K:		// K��Ȳè�ނ�
					g_ncArgv.nc.dValue[NCA_K] = atoi(strWord.substr(1).c_str());
					break;
				case NCA_P:		// P(�޳�َ���)�͑��葬�x�Ɠ�������
					g_ncArgv.nc.dValue[NCA_P] = (*g_pfnFeedAnalyze)(strWord.substr(1));
					break;
				default:
					g_ncArgv.nc.dValue[nCode] = _GetNCValue(strWord.substr(1));
				}
			}
			else if ( g_pDoc->IsDocFlag(NCDOC_WIRE) ) {
				// ܲ�Ӱ�ނɂ�������ʏ���(L�l)
				g_ncArgv.nc.dValue[nCode] = nCode<GVALSIZE || nCode==NCA_L ?
					_GetNCValue(strWord.substr(1)) : atoi(strWord.substr(1).c_str());
			}
			else {
				g_ncArgv.nc.dValue[nCode] = nCode < GVALSIZE ?	// nCode < NCA_P
					_GetNCValue(strWord.substr(1)) : atoi(strWord.substr(1).c_str());
			}
			g_ncArgv.nc.dwValFlags |= g_dwSetValFlags[nCode];
			bNCval = TRUE;
			break;
		}	// End of switch()
	}	// End of while() phrase_parse

	if ( bTcode && !bNCobj && !bNCval ) {
		// T���ޒP�Ǝw��(ܲԂ̂�)
		nNotModalCode = 01;		// dummy
		g_ncArgv.taper.bTonly = TRUE;	// TH_UVWire.cpp �ł̏����ڈ�
		bNCobj = TRUE;			// dummy object �̐���
	}
	//
	if ( bNCsub && bNCval ) {
		// M98�ł�[P_|L_]�̵�޼ު�Đ�����}��
		if ( !(g_ncArgv.nc.dwValFlags & ~(NCD_P|NCD_L)) )
			bNCval = FALSE;
	}
	//
	if ( bNCobj || bNCval ) {
		// NC�ް��o�^����
		pData = AddGcode(pBlock, pDataResult, nNotModalCode);
		if ( g_lpstrComma )
			MakeChamferingObject(pBlock, pDataResult, pData);
		pDataResult = pData;
		_SetStrComma(strComma);
		// ��ۯ����̍X�V
		pBlock->SetBlockToNCdata(pDataResult, g_pDoc->GetNCsize());
	}
	if ( bNCsub ) {
		// M���ތ㏈��
		if ( CallSubProgram(pBlock, pDataResult) == 30 )
			return 30;	// �I������
	}

	return 0;
}

CNCdata* AddGcode(CNCblock* pBlock, CNCdata* pDataBefore, int nNotModalCode)
{
	CNCdata*	pDataResult = pDataBefore;

	if ( !g_pDoc->IsDocFlag(NCDOC_WIRE) ) {
		// ܲԉ��H�ȊO��UVW���W�̉��Z
		for ( int i=0; i<NCXYZ; i++ ) {
			if ( g_ncArgv.nc.dwValFlags & g_dwSetValFlags[i+NCA_U] ) {
				g_ncArgv.nc.dValue[i]  += g_ncArgv.nc.dValue[i+NCA_U];
				g_ncArgv.nc.dwValFlags |= g_dwSetValFlags[i];
			}
		}
	}

	if ( g_pDoc->IsDocFlag(NCDOC_LATHE) ) {
		// ����Ӱ�ނł̍��W����ւ�(�޳�ُ���)
		if ( g_ncArgv.nc.nGcode != 4 ) {
			optional<double>	x, z, i, k;
			if ( g_ncArgv.nc.dwValFlags & NCD_X )
				x = g_ncArgv.nc.dValue[NCA_X] / 2.0;	// ���a�w��
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
		}
	}
	else {
		// NC�ް��̓o�^�O����
		if ( g_ncArgv.nc.nGtype == G_TYPE ) {
			switch ( nNotModalCode ) {
			case 68:
				// G68���W��]�w��������
				G68RoundCheck(pBlock);
				if ( !g_ncArgv.g68.bG68 )
					return pDataResult;
				break;
			case 92:
				if ( g_pDoc->IsDocFlag(NCDOC_WIRE) && g_ncArgv.nc.dwValFlags&NCD_J ) {
					// ��۸��і�(XY��)�̐ݒ�
					g_ncArgv.nc.dValue[NCA_Z] = g_ncArgv.nc.dValue[NCA_J];
					g_ncArgv.nc.dwValFlags |= NCD_Z;
				}
				break;
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
		while ( nRepeat-- > 0 && IsThread() ) {
			// NC_SearchSubProgram ����ۯ����ǉ������\���A��
			// �����ł� nLoop �ϐ����g�킸�AȲè�ނ���ۯ����ނɂĔ���
			for ( i=nIndex; i<g_pDoc->GetNCBlockSize() && IsThread(); i++ ) {
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
	else if ( 81<=nCode && nCode<=89 ) {
		g_Cycle.bCycle = TRUE;
		enResult = MAKEOBJ;
	}
	else if ( nCode==4 || nCode==10 || nCode==52 || nCode==68 || nCode==92 )
		enResult = MAKEOBJ_NOTMODAL;
	else
		enResult = NOOBJ;

	return enResult;
}

ENGCODEOBJ	IsGcodeObject_Wire(int nCode)
{
	ENGCODEOBJ	enResult;

	if ( 0<=nCode && nCode<=3 )
		enResult = MAKEOBJ;
	else if ( nCode==4 || nCode==10 || nCode==11 || nCode==92 || nCode==93 )
		enResult = MAKEOBJ_NOTMODAL;
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
	else if ( nCode==4 || nCode==10 )
		enResult = MAKEOBJ_NOTMODAL;
	else
		enResult = NOOBJ;

	return enResult;
}

// �؍��ވȊO�̏d�v��G���ތ���
int CheckGcodeOther_Milling(int nCode)
{
	int		nResult = 0;

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

	return nResult;
}

int CheckGcodeOther_Wire(int nCode)
{
	int		nResult = 0;

	switch ( nCode ) {
	// ð�ߏ���
	case 50:
		g_ncArgv.taper.nTaper = 0;
		break;
	case 51:
	case 52:
		g_ncArgv.taper.nTaper = nCode==51 ? 1 : -1;
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
	// �㉺�Ɨ���Ű
	case 60:
	case 61:
	case 62:
	case 63:
		if ( g_ncArgv.taper.nTaper == 0 )
			nResult = IDS_ERR_NCBLK_TAPER;
		else
			g_ncArgv.taper.nDiff = nCode - 60;
		break;
	// ��޿ح��, �ݸ����
	case 90:
		g_ncArgv.bAbs = TRUE;
		break;
	case 91:
		g_ncArgv.bAbs = FALSE;
		break;
	}

	return nResult;
}

int CheckGcodeOther_Lathe(int nCode)
{
	int		nResult = 0;

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

	return nResult;
}

// �����۸��т̌���
int NC_SearchSubProgram(int* pRepeat)
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
	if ( g_pDoc->SerializeInsertBlock(strFile, n, NCF_AUTOREAD) ) {
		// �}����ۯ��̍ŏ�����NCF_FOLDER
		if ( n < g_pDoc->GetNCBlockSize() )	// �}���O < �}����
			g_pDoc->GetNCblock(n)->SetBlockFlag(NCF_FOLDER);
		return n;
	}

	return -1;
}

// ϸ���۸��т̌���
int NC_SearchMacroProgram(const string& strBlock, CNCblock* pBlock)
{
	extern	const	int		g_nDefaultMacroID[];	// MCOption.cpp

	const CMCOption* pMCopt = AfxGetNCVCApp()->GetMCOption();

	if ( !regex_search(strBlock, g_reMacroStr) )
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
			strArgv.Replace(strKey, strBlock.c_str());
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
	if ( g_pDoc->SerializeInsertBlock(strMacroFile, n, NCF_AUTOREAD) ) {
		// �}����ۯ��̍ŏ�����NCF_FOLDER
		if ( n < g_pDoc->GetNCBlockSize() )	// ��ۯ��}�������s�̉\��������
			g_pDoc->GetNCblock(n)->SetBlockFlag(NCF_FOLDER);
		return n;
	}

	return -1;
}

// ������ڲ����ނ̌���
int NC_SearchAutoBreak(const string& strBlock, CNCblock* pBlock)
{
	if ( regex_search(strBlock, g_reAutoBreak) )
		pBlock->SetBlockFlag(NCF_BREAK);

	return 0;	// dummy
}

int NC_NoSearch(const string&, CNCblock*)
{
	return -1;
}

void MakeChamferingObject(CNCblock* pBlock, CNCdata* pData1, CNCdata* pData2)
{
#ifdef _DEBUG
	CMagaDbg	dbg("MakeChamferingObject()", DBG_BLUE);
#endif
	// �ް�����
	if ( g_pDoc->IsDocFlag(NCDOC_LATHE) ) {
		// ����Ӱ�ނłͻ�߰Ă���Ȃ�
		pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_NOTLATHE);
		return;
	}
	if ( pData1->GetGtype() != G_TYPE || pData2->GetGtype() != G_TYPE ||
			!pData1->IsCutCode() || !pData2->IsCutCode() ) {
		pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_GTYPE);
		return;
	}
	if ( !(pData1->GetValFlags()&(NCD_X|NCD_Y|NCD_Z)) || !(pData2->GetValFlags()&(NCD_X|NCD_Y|NCD_Z)) ) {
		pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_VALUE);
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
	optional<CPointD>	ptResult;
	BOOL	bResult;

	// �v�Z�J�n
	if ( cr < NCMIN ) {
		r1 = r2 = 0.0;
		cCham = 'C';		// nGcode = 1
	}
	else if ( cCham == 'C' )
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
	if ( r1 < NCMIN )
		pts = pData1->GetPlaneValue(pData1->GetEndPoint());
	else {
		ptResult = pData1->SetChamferingPoint(FALSE, r1);
		if ( !ptResult ) {
			pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_LENGTH);
			return;
		}
		pts = *ptResult;
	}
	pts -= ptOffset;
	// pData2(���̵�޼ު��)�̎n�_��␳
	if ( r2 < NCMIN )
		pte = pData2->GetPlaneValue(pData2->GetStartPoint());
	else {
		ptResult = pData2->SetChamferingPoint(TRUE, r2);
		if ( !ptResult ) {
			pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_LENGTH);
			return;
		}
		pte = *ptResult;
	}
	pte -= ptOffset;

#ifdef _DEBUG_GSPIRIT
	if ( !IsThumbnail() ) {
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
	memcpy(&(ncArgv.g68),   &(pData1->GetReadData()->m_g68),   sizeof(G68ROUND));
	memcpy(&(ncArgv.taper), &(pData1->GetReadData()->m_taper), sizeof(TAPER));
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
			pa += RAD(360.0);
		if ( (pb=atan2(pte.y, pte.x)) < 0.0 )
			pb += RAD(360.0);
		if ( fabs(pa-pb) > RAD(180.0) ) {
			if ( pa > pb )
				pa -= RAD(360.0);
			else
				pb -= RAD(360.0);
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
	return fabs(_GetNCValue(str));
}

double FeedAnalyze_Int(const string& str)
{
	return fabs(atof(str.c_str()));
}

void SetEndmillDiameter(const string& str)
{
#ifdef _DEBUG
	CMagaDbg	dbg("SetEndmillDiameter()", DBG_MAGENTA);
#endif

	const CMCOption* pMCopt = AfxGetNCVCApp()->GetMCOption();
	optional<double> dResult = pMCopt->GetToolD( atoi(str.c_str()) );
	if ( dResult ) {
		g_ncArgv.dEndmill = *dResult;	// �̾�Ă͔��a�Ȃ̂ŁA���̂܂܎g�p
#ifdef _DEBUG
		if ( !IsThumbnail() )
			dbg.printf("Endmill=%f from T-No.%d", g_ncArgv.dEndmill, atoi(str.c_str()));
#endif
	}
#ifdef _DEBUG
	else {
		if ( !IsThumbnail() )
			dbg.printf("Endmill T-No.%d nothing", atoi(str.c_str()));
	}
#endif
}

int SetTaperAngle(const string& str)
{
#ifdef _DEBUG
	CMagaDbg	dbg("SetTaperAngle()", DBG_MAGENTA);
#endif
	int		nResult = 0;
	double	dTaper  = atof(str.c_str());

	if ( g_ncArgv.taper.nTaper == 0 ) {
		if ( dTaper == 0.0 )
			g_ncArgv.taper.dTaper = 0.0;
		else
			nResult = IDS_ERR_NCBLK_TAPER;
	}
	else {
		if ( 45.0 < fabs(dTaper) )
			nResult = IDS_ERR_NCBLK_OVER;
		else
			g_ncArgv.taper.dTaper = RAD(dTaper);	// ׼ޱݕێ�
#ifdef _DEBUG
		if ( !IsThumbnail() ) {
			int dbgTaper = 0;
			if ( g_ncArgv.taper.nTaper == 1 )
				dbgTaper = 51;
			else if ( g_ncArgv.taper.nTaper == -1 )
				dbgTaper = 52;
			dbg.printf("Mode=%d angle=%f", dbgTaper, dTaper);
		}
#endif
	}

	return nResult;
}

CNCdata* SetToolPosition_fromComment(CNCblock* pBlock, CNCdata* pDataBefore)
{
	CNCdata*	pData;

	for ( int i=0; i<NCXYZ; i++ ) {
		if ( g_dwToolPosFlags & g_dwSetValFlags[i] ) {
			g_ncArgv.nc.dValue[i] = g_dToolPos[i];
			g_ncArgv.nc.dwValFlags |= g_dwSetValFlags[i];
		}
	}
	// �`�悵�Ȃ���޼ު�ĂŐ���
	g_ncArgv.nc.nGtype = O_TYPE;
	pData = AddGcode(pBlock, pDataBefore, -1);
	// ����
	g_ncArgv.nc.nGtype = G_TYPE;

	return pData;
}

void SetWorkRect_fromComment(void)
{
#ifdef _DEBUG
	CMagaDbg	dbg("SetWorkRect_fromComment()", DBG_MAGENTA);
#endif
	CRect3D		rc;
	CPoint3D	pt;

	// �s�׍H�₯��vWork�̐����s��Ȃ̂ŗL�����ް��������o��
	for ( int i=0; i<g_nWorkRect && i<SIZEOF(g_dWorkRect); i++ ) {
		switch ( i ) {
		case 0:		// ��
			rc.right = g_dWorkRect[0];
			break;
		case 1:		// ���s
			rc.bottom = g_dWorkRect[1];
			break;
		case 2:		// ����
			rc.high = g_dWorkRect[2];
			break;
		case 3:		// X�̾��
			pt.x = g_dWorkRect[3];
			break;
		case 4:		// Y�̾��
			pt.y = g_dWorkRect[4];
			break;
		case 5:		// Z�̾��
			pt.z = g_dWorkRect[5];
			break;
		}
	}
	rc.OffsetRect(pt);

	// �޷���Ă��ێ�����ܰ���`�̍X�V
	g_pDoc->SetWorkRectComment(rc);

#ifdef _DEBUG
	if ( !IsThumbnail() ) {
		dbg.printf("(%f,%f)-(%f,%f)", rc.left, rc.top, rc.right, rc.bottom);
		dbg.printf("(%f,%f)", rc.low, rc.high);
	}
#endif
}

void SetWorkCylinder_fromComment(void)
{
#ifdef _DEBUG
	CMagaDbg	dbg("SetWorkCylinder_fromComment()", DBG_MAGENTA);
#endif
	double		d, h;
	CPoint3D	pt;

	for ( int i=0; i<g_nWorkCylinder && i<SIZEOF(g_dWorkCylinder); i++ ) {
		switch ( i ) {
		case 0:		// ���a
			d = g_dWorkCylinder[0];
			break;
		case 1:		// ����
			h = g_dWorkCylinder[1];
			break;
		case 2:		// X�̾��
			pt.x = g_dWorkCylinder[2];
			break;
		case 3:		// Y�̾��
			pt.y = g_dWorkCylinder[3];
			break;
		case 4:		// Z�̾��
			pt.z = g_dWorkCylinder[4];
			break;
		}
	}

	// �޷���Ă��ێ�����~�����̍X�V
	g_pDoc->SetWorkCylinderComment(d, h, pt);

#ifdef _DEBUG
	if ( !IsThumbnail() ) {
		dbg.printf("d=%f h=%f", d, h);
		dbg.printf("offset=%f,%f,%f", pt.x, pt.y, pt.z);
	}
#endif
}

void SetLatheRect_fromComment(void)
{
#ifdef _DEBUG
	CMagaDbg	dbg("SetLatheRect_fromComment()", DBG_MAGENTA);
#endif
	// ����Ӱ�ނ��׸�ON
	g_pDoc->SetLatheViewMode();
	// �Ăяo���֐��̎w��
	g_pfnIsGcode = &IsGcodeObject_Lathe;
	g_pfnCheckGcodeOther = &CheckGcodeOther_Lathe;
	// ��̫�ĕ��ʁFXZ_PLANE
	g_ncArgv.nc.enPlane = XZ_PLANE;

	// �ް��̎��o��
	for ( int i=0; i<g_nLatheView && i<SIZEOF(g_dLatheView); i++ ) {
		switch ( i ) {
		case 0:		// ܰ��a
			g_pDoc->SetWorkLatheR( g_dLatheView[0] / 2.0 );	// ���a�ŕۊ�
#ifdef _DEBUG
			if ( !IsThumbnail() )
				dbg.printf("r=%f", g_dLatheView[0]/2);
#endif
			break;
		case 2:		// z1, z2
			// z2 ������Ƃ�����
			g_pDoc->SetWorkLatheZ( g_dLatheView[1], g_dLatheView[2] );
#ifdef _DEBUG
			if ( !IsThumbnail() )
				dbg.printf("(%f)-(%f)", g_dLatheView[1], g_dLatheView[2]);
#endif
			break;
		}
	}
}

void SetWireRect_fromComment(void)
{
#ifdef _DEBUG
	CMagaDbg	dbg("SetWireRect_fromComment()", DBG_MAGENTA);
#endif
	// ܲԉ��HӰ�ނ��׸�ON
	g_pDoc->SetDocFlag(NCDOC_WIRE);
	// �Ăяo���֐��̎w��
	g_pfnIsGcode = &IsGcodeObject_Wire;
	g_pfnCheckGcodeOther = &CheckGcodeOther_Wire;

	// ܰ������w��
	CRect3D		rc;
	rc.high = g_dWireView;
	g_pDoc->SetWorkRectComment(rc, FALSE);	// �`��̈���X�V���Ȃ�(CNCDoc::SerializeAfterCheck)
#ifdef _DEBUG
	if ( !IsThumbnail() )
		dbg.printf("t=%f", rc.high);
#endif
}

CString SearchFolder(const regex& r)
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
			const CMapStringToPtr* pMap = AfxGetNCVCApp()->GetDocTemplate(TYPE_NCD)->GetExtMap((eEXTTYPE)j);
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

CString	SearchFolder_Sub(int n, LPCTSTR lpszFind, const regex& r)
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

BOOL SearchProgNo(LPCTSTR lpszFile, const regex& r)
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
	if ( g_pDoc->IsDocFlag(NCDOC_LATHE) ) {
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
			g_ncArgv.g68.dRound = RAD(g_ncArgv.nc.dValue[NCA_R]);	// ׼ޱ݂Ŋi�[
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

void TaperClear(void)
{
	g_ncArgv.taper.nTaper  = 0;
	g_ncArgv.taper.dTaper  = 0.0;
	g_ncArgv.taper.nDiff   = 0;
}

// �ϐ�������
void InitialVariable(void)
{
	int		i;
	const CMCOption*	pMCopt = AfxGetNCVCApp()->GetMCOption();
	const CViewOption*	pVopt  = AfxGetNCVCApp()->GetViewOption();

	ZeroMemory(&g_ncArgv, sizeof(NCARGV));

	g_pfnFeedAnalyze = pMCopt->GetInt(MC_INT_FDOT)==0 ? &FeedAnalyze_Int : &FeedAnalyze_Dot;
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
//	g_ncArgv.nc.nErrorCode = 0;
//	g_ncArgv.nc.dwValFlags = 0;
	switch ( pMCopt->GetInt(MC_INT_FORCEVIEWMODE) ) {
	case MC_VIEWMODE_LATHE:		// ����
		g_nLatheView = 0;
		SetLatheRect_fromComment();	// ����Ӱ�ނ֋����ؑ�
		g_ncArgv.nc.dValue[NCA_X] = pMCopt->GetInitialXYZ(NCA_Z);
		g_ncArgv.nc.dValue[NCA_Y] = 0.0;
		g_ncArgv.nc.dValue[NCA_Z] = pMCopt->GetInitialXYZ(NCA_X) / 2.0;
		i = 3;	// XYZ���������ޯ����i�߂�
		break;
	case MC_VIEWMODE_WIRE:		// ܲԉ��H�@
		g_dWireView = pMCopt->GetDbl(MC_DBL_DEFWIREDEPTH);
		SetWireRect_fromComment();
		for ( i=0; i<NCXYZ; i++ )
			g_ncArgv.nc.dValue[i] = pMCopt->GetInitialXYZ(i);
		break;
	default:
		g_pfnIsGcode = &IsGcodeObject_Milling;
		g_pfnCheckGcodeOther = &CheckGcodeOther_Milling;
		for ( i=0; i<NCXYZ; i++ )
			g_ncArgv.nc.dValue[i] = pMCopt->GetInitialXYZ(i);
	}
	for ( ; i<VALUESIZE; i++ )
		g_ncArgv.nc.dValue[i] = 0.0;
	g_ncArgv.bAbs		= pMCopt->GetModalSetting(MODALGROUP3) == 0 ? TRUE : FALSE;
	g_ncArgv.bG98		= pMCopt->GetModalSetting(MODALGROUP4) == 0 ? TRUE : FALSE;
//	g_ncArgv.nSpindle	= 0;
	g_ncArgv.dFeed		= pMCopt->GetDbl(MC_DBL_FEED);
	g_ncArgv.dEndmill	= pVopt->GetDefaultEndmill();
	g_ncArgv.nEndmillType	= pVopt->GetDefaultEndmillType();

	g_Cycle.clear();
	TaperClear();
	G68RoundClear();

	g_nSubprog = 0;
	g_lpstrComma = NULL;

	if ( IsThumbnail() ) {
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
