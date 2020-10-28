// TH_NCRead.cpp
// ＮＣコードの内部変換
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "MCOption.h"
#include "NCdata.h"
#include "NCDoc.h"
#include "ThreadDlg.h"
#include "NCVCdefine.h"
#pragma warning( push )
#pragma	warning( disable : 4244 )		// Boost 1.59～
#pragma	warning( disable : 4018 )
#include "boost/spirit/include/qi.hpp"
#include "boost/spirit/repository/include/qi_confix.hpp"
#pragma	warning( pop )

#ifdef _DEBUG
#define new DEBUG_NEW
#define	_DEBUG_GSPIRIT
#endif

using std::string;
using namespace boost;
using namespace boost::spirit;
namespace sw = qi::standard_wide;	// 2ﾊﾞｲﾄ文字対応

// 解析中のﾌﾞﾛｯｸ状態ﾌﾗｸﾞ
#define	NCBLK_WORKX		(NCD_X)
#define	NCBLK_WORKY		(NCD_Y)
#define	NCBLK_WORKZ		(NCD_Z)
#define	NCBLK_TOOLX		(NCD_X<<NCXYZ)
#define	NCBLK_TOOLY		(NCD_Y<<NCXYZ)
#define	NCBLK_TOOLZ		(NCD_Z<<NCXYZ)
#define	NCBLK_WORKPOS	(NCD_X|NCD_Y|NCD_Z)
#define	NCBLK_TOOLPOS	(NCBLK_WORKPOS<<NCXYZ)
#define	NCBLK_ERR_FILE	0x0100	// 下位６ビットを[WORK|TOOL]POSで使用
#define	NCBLK_ERR_HOLE	0x0200

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
						g_nSubprog;			// ｻﾌﾞﾌﾟﾛｸﾞﾗﾑ呼び出しの階層
static	float			g_dWorkRect[NCXYZ*2],
						g_dWorkCylinder[2+NCXYZ],
						g_dLatheView[3],
						g_dWireView,
						g_dToolPos[NCXYZ],
						g_dWorkPos[NCXYZ];
static	DWORD			g_dwBlockFlags;
static	string			g_strComma;			// 次のﾌﾞﾛｯｸとの計算
//
static	BOOL	IsThreadContinue(void)
{
	return g_pParent->IsThreadContinue();
}
static	BOOL	IsThreadThumbnail(void)
{
	return TRUE;
}
static	function<BOOL ()>	IsThread;
#define	IsThumbnail()	g_pDoc->IsDocFlag(NCDOC_THUMBNAIL)

// 固定ｻｲｸﾙのﾓｰﾀﾞﾙ補間値
struct	CYCLE_INTERPOLATE
{
	BOOL	bCycle,		// 固定ｻｲｸﾙ処理中
			bAbs;		// Abs(G90)|Inc(G91)
	double	dValI;
	optional<double>	dValR, dValZ, dValP;
	void	clear(void) {
		bCycle = FALSE;
		dValR.reset();
		dValZ.reset();
		dValP.reset();
	}
	void	ChangeAbs(void) {
		if ( bCycle && !bAbs ) {
			bAbs = TRUE;
			if ( dValZ ) {
				if ( dValR ) {
					dValR = dValR.get() + dValI;
					dValZ = dValZ.get() + dValR.get();
				}
				else {
					dValZ = dValZ.get() + dValI;
				}
			}
		}
	}
	void	ChangeInc(void) {
		if ( bCycle && bAbs ) {
			bAbs = FALSE;
			if ( dValZ ) {
				if ( dValR ) {
					dValZ = dValZ.get() - dValR.get();
					dValR = dValR.get() - dValI;
				}
				else {
					dValZ = dValZ.get() - dValI;
				}
			}
		}
	}
};
static	CYCLE_INTERPOLATE	g_Cycle;
static	void	CycleInterpolate(void);

// IsGcodeObject() 戻り値
enum	ENGCODEOBJ {NOOBJ, MAKEOBJ, MAKEOBJ_NOTMODAL};

// G68座標回転
static	void	G68RoundCheck(CNCblock*);
static	void	G68RoundClear(void);
// ﾃｰﾊﾟ加工情報
static	void	TaperClear(void);
//
static	void	InitialVariable(void);
// 解析関数
static	int			NC_GSeparater(INT_PTR, CNCdata*&);
static	CNCdata*	AddGcode(CNCblock*, CNCdata*, int);
static	void		AddM98code(CNCblock*, CNCdata*, INT_PTR);
static	int			CallSubProgram(CNCblock*, CNCdata*&);
static	ENGCODEOBJ	IsGcodeObject_Milling(int);
static	ENGCODEOBJ	IsGcodeObject_Wire(int);
static	ENGCODEOBJ	IsGcodeObject_Lathe(int);
static	function<ENGCODEOBJ (int)>	IsGcode;
static	int			CheckGcodeOther_Milling(int);
static	int			CheckGcodeOther_Wire(int);
static	int			CheckGcodeOther_Lathe(int);
static	function<int (int)>		CheckGcodeOther;
// 面取り・ｺｰﾅｰR処理
static	void	MakeChamferingObject(CNCblock*, CNCdata*, CNCdata*);
// Fﾊﾟﾗﾒｰﾀ, ﾄﾞｳｪﾙ時間の解釈
static	float	FeedAnalyze_Dot(const string&);
static	float	FeedAnalyze_Int(const string&);
static	function<float (const string&)>	FeedAnalyze;
// 工具径解析
static	void	SetEndmillDiameter(const string&);
// ﾃｰﾊﾟ角度
static	int		SetTaperAngle(const string&);
// 工具位置情報
static	CNCdata*	SetToolPosition_fromComment(CNCblock*, CNCdata*);
// ﾜｰｸ矩形情報設定
static	void	SetWorkRect_fromComment(void);
static	void	SetWorkCylinder_fromComment(void);
static	void	SetLatheView_fromComment(void);
static	void	SetWireView_fromComment(void);
// ｻﾌﾞﾌﾟﾛ，ﾏｸﾛの検索
static	CString	g_strSearchFolder[2];	// ｶﾚﾝﾄと指定ﾌｫﾙﾀﾞ
static	CString	SearchFolder(const regex&);
static	CString	SearchFolder_Sub(int, LPCTSTR, const regex&);
static	BOOL	SearchProgNo(LPCTSTR, const regex&);
static	regex	g_reMacroStr;
static	INT_PTR	NC_SearchSubProgram(INT_PTR*);
static	INT_PTR	NC_SearchMacroProgram(const string&, CNCblock*);
static	INT_PTR	NC_NoSearch(const string&, CNCblock*);
// 自動ﾌﾞﾚｲｸｺｰﾄﾞ検索
static	regex	g_reAutoBreak;
static	INT_PTR	NC_SearchAutoBreak(const string&, CNCblock*);
static	function<INT_PTR (const string&, CNCblock*)>	SearchMacro,
														SearchAutoBreak;

//////////////////////////////////////////////////////////////////////

// Ｇコードの小数点判定
static inline	int		_GetGcode(const string& str)
{
	float	dResult = (float)atof(str.c_str());
	if ( str.find('.') != string::npos )
		dResult *= 1000.0f;		// 小数点があれば ｺｰﾄﾞ1000倍
	return (int)dResult;
}
// 基準平面に対する直交軸
static inline	int		_GetPlaneZ(void)
{
	int	z;
	switch ( g_ncArgv.nc.enPlane ) {
	case XZ_PLANE:
		z = NCA_Y;
		break;
	case YZ_PLANE:
		z = NCA_X;
		break;
	default:	// XY_PLANE:
		z = NCA_Z;
		break;
	}
	return z;
}
// 数値変換（小数点がなければ 1/1000）
static	float	GetNCValue_NoCheck(const string& str)
{
	float	dResult = (float)atof(str.c_str());
	return str.find('.')==string::npos ? dResult/1000.0f : dResult;
}
static	float	GetNCValue_CheckDecimal4(const string& str)
{
	float	dResult = (float)atof(str.c_str());
	string::size_type	n, pos = str.find('.');
	if ( pos == string::npos )
		dResult /= 1000.0f;
	else {
		n = str.length() - pos - 1;
		if ( n >= 4 ) g_pDoc->SetDocFlag(NCDOC_DECIMAL4);
	}
	return dResult;
}
static	function<float (const string&)>	GetNCValue;
static	const INT_PTR	MAXCHECKCNT = 50;

//////////////////////////////////////////////////////////////////////
//	NCｺｰﾄﾞのｵﾌﾞｼﾞｪｸﾄ生成ｽﾚｯﾄﾞ
//////////////////////////////////////////////////////////////////////

UINT NCDtoXYZ_Thread(LPVOID pVoid)
{
#ifdef _DEBUG
	printf("NCDtoXYZ_Thread() Start\n");
#ifdef _DEBUG_FILEOPEN		// NCVC.h
	extern	CTime	dbgtimeFileOpen;	// NCVC.cpp
	CTime	t2 = CTime::GetCurrentTime();
	CTimeSpan ts = t2 - dbgtimeFileOpen;
	CString	strTime( ts.Format("%H:%M:%S") );
	printf("NCDtoXYZ_Thread() %s\n", LPCTSTR(strTime));
	dbgtimeFileOpen = t2;
#endif
#endif

	INT_PTR		i, nLoopCnt;
	int			nResult = 0;
	CNCdata*	pDataFirst = NULL;
	CNCdata*	pData;	// １つ前の生成ｵﾌﾞｼﾞｪｸﾄ

	// 変数初期化
	LPNCVCTHREADPARAM	pParam = reinterpret_cast<LPNCVCTHREADPARAM>(pVoid);
	g_pParent = pParam->pParent;
	g_pDoc  = static_cast<CNCDoc*>(pParam->pDoc);
	ASSERT(g_pDoc);
	IsThread = g_pParent ? &IsThreadContinue : &IsThreadThumbnail;
	InitialVariable();

	nLoopCnt = g_pDoc->GetNCBlockSize();
	if ( g_pParent ) {
		CString	strMsg;
		VERIFY(strMsg.LoadString(IDS_READ_NCD));
		g_pParent->SetFaseMessage(strMsg);
		g_pParent->m_ctReadProgress.SetRange32(0, (int)nLoopCnt);
	}
#ifdef _DEBUG
	if ( !IsThumbnail() )
		printf("LoopCount=%d\n", nLoopCnt);
#endif

	try {
		// １つ前のｵﾌﾞｼﾞｪｸﾄ参照でNULL参照しないため
		pDataFirst = pData = new CNCdata(&g_ncArgv);
		// 1行(1ﾌﾞﾛｯｸ)解析しｵﾌﾞｼﾞｪｸﾄの登録
		GetNCValue = g_pParent ? &GetNCValue_CheckDecimal4 : &GetNCValue_NoCheck;	// MAXCHECKCNTまで
		for ( i=0; i<nLoopCnt && i<MAXCHECKCNT && !g_pDoc->IsDocFlag(NCDOC_DECIMAL4) && nResult==0 && IsThread(); i++ ) {
			nResult = NC_GSeparater(i, pData);
		}
		GetNCValue = &GetNCValue_NoCheck;			// MAXCHECKCNT以降
		for ( ; i<nLoopCnt && nResult==0 && IsThread(); i++ ) {
			nResult = NC_GSeparater(i, pData);
			if ( (i & 0x003f)==0 && g_pParent )	// 64回おき(下位6ﾋﾞｯﾄﾏｽｸ)
				g_pParent->m_ctReadProgress.SetPos((int)i);		// ﾌﾟﾛｸﾞﾚｽﾊﾞｰ
		}
		nResult = IDOK;
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		nResult = IDCANCEL;
	}

	// 初回登録用ﾀﾞﾐｰﾃﾞｰﾀの消去
	if ( pDataFirst )
		delete	pDataFirst;

	if ( g_pParent ) {
		g_pParent->m_ctReadProgress.SetPos((int)nLoopCnt);
		g_pParent->PostMessage(WM_USERFINISH, IsThread() ? nResult : IDCANCEL);
	}

	return 0;	// AfxEndThread(0);
}

//////////////////////////////////////////////////////////////////////
//	Gｺｰﾄﾞの構文解析

//	NCﾌﾟﾛｸﾞﾗﾑ解析
template<typename Iterator, typename Skipper>
struct CGcodeParser : qi::grammar<Iterator, Skipper, string()>
{
	qi::rule<Iterator, Skipper, string()>	rule;

	CGcodeParser() : CGcodeParser::base_type(rule) {
		using sw::char_;
		using qi::int_;
		using qi::float_;
		rule = qi::raw[
				(sw::upper >> float_) |		// Nomal Gcode
				(char_('/') >> -int_) |		// Optional Block Skip
				(qi::lit(',') >> (char_('R')|'C') >> float_)	// CornerR/C
		];
	}
};

//	ｺﾒﾝﾄ文字列解析
template<typename Iterator, typename Skipper>
struct CCommentParser : qi::grammar<Iterator, Skipper>
{
	qi::rule<Iterator, Skipper>		ruleTop, rrMM,
		ruleOrder, ruleValue1, ruleValue2,
		rs01, rs02, rs03, rs04, rs05, rs06, rs07, rs08, rs09, rs10, rs11, rs12, rs13, rs14,
		rv01, rv02, rv03, rv04, rv05, rv06, rv07, rv08, rv09, rv10, rv11, rv12, rv13, rv14,
		r011, r012, r013, r031, r032, r033, r034, r041, r042, r051, r052,
		r061, r062,	r071, r072;

	CCommentParser() : CCommentParser::base_type(ruleTop) {
		using sw::no_case;
		using sw::char_;
		using qi::float_;
		using qi::omit;
		using qi::lit;
		using qi::lexeme;
		using qi::as_string;

		ruleTop = omit[*(char_-'(')] >> lit('(') >>
					+( omit[*(char_-ruleOrder)] >> (ruleValue1|ruleValue2) ) >>
					omit[*(char_-')')] >> lit(')');
		ruleOrder  = rs01|rs02|rs03|rs04|rs05|rs06|rs07|rs08|rs09|rs10|rs11|rs12|rs13|rs14;
		ruleValue1 = rs01>>rv01|rs02>>rv02|rs03>>rv03|rs04>>rv04|rs05>>rv05|rs06>>rv06;		// １つにすると
		ruleValue2 = rs07>>rv07|rs08>>rv08|rs09>>rv09|rs10>>rv10|rs11>>rv11|rv12|rv13|rv14;	// 名前が長すぎるｴﾗｰ?
		//
		// Endmill
		rs01 = no_case[sw::string(ENDMILL_S)|TAP_S|REAMER_S] >> lit('=');
		rv01 = (r011|r012|r013) >> rrMM;
		r011 = float_[_SetEndmill()];							// ｽｸｳｪｱｴﾝﾄﾞﾐﾙ
		r012 = (char_('R')|'r') >> float_[_SetBallMill()];		// ﾎﾞｰﾙｴﾝﾄﾞﾐﾙ
		r013 = (char_('C')|'c') >> float_[_SetChamferMill()];	// 面取りﾐﾙ
		// Drill
		rs02 = no_case[DRILL_S] >> lit('=');
		rv02 = float_[_SetDrill()] >> rrMM;
		// Groove
		rs03 = no_case[GROOVE_S] >> lit('=');
		rv03 = (r031|r032|r033|r034) >> rrMM;
		r031 = float_[_SetGrooving()];
		r032 = (char_('L')|'l') >> float_[_SetGrooving()];			// 工具基準点左
		r033 = (char_('R')|'r') >> float_[_SetGrooving_Right()];	//           右
		r034 = (char_('C')|'c') >> float_[_SetGrooving_Center()];	//           中央
		// WorkRect
		rs04 = no_case[WORKRECT_S] >> lit('=');
		rv04 = r041|r042;
		r041 = qi::raw[float_ >> (char_('X')|'x') >> float_ >> (char_('T')|'t') >> float_][_SetWorkRectStr()];
		r042 = float_[_SetWorkRect()] % ',';
		// WorkCylinder
		rs05 = no_case[WORKCYLINDER_S] >> lit('=');
		rv05 = r051|r052;
		r051 = qi::raw[float_ >> (char_('H')|'h') >> float_][_SetWorkCylinderStr()];
		r052 = float_[_SetWorkCylinder()] % ',';
		// WorkFile
		rs06 = no_case[WORKFILE_S] >> lit('=');
		rv06 = r061|r062;
		r061 = lit('\"') >> lexeme[ as_string[+(char_ - '\"')][_SetWorkFile()] ] >> lit('\"') >>
				-lit(',') >> -float_[_WorkPosX()] >>
					-(lit(',') >> -float_[_WorkPosY()] >>
						-(lit(',') >> -float_[_WorkPosZ()]) );
		r062 = lexeme[ as_string[+(char_ - ')')][_SetWorkFile()] ];
		// MCFile
		rs07 = no_case[MCFILE_S] >> lit('=');
		rv07 = r071|r072;
		r071 = lit('\"') >> lexeme[ as_string[+(char_ - '\"')][_SetMCFile()] ] >> lit('\"');
		r072 = lexeme[ as_string[+(char_ - ')')][_SetMCFile()] ];
		// ViewMode
		rs08 = no_case[LATHEVIEW_S] >> lit('=');
		rv08 = float_[_SetLatheView()] % ',';
		rs09 = no_case[WIREVIEW_S]  >> lit('=');
		rv09 = float_[_SetWireView()];
		// ToolPos
		rs10 = no_case[TOOLPOS_S] >> lit('=');
		rv10 = -float_[_ToolPosX()] >>
				-(lit(',') >> -float_[_ToolPosY()] >>
					-(lit(',') >> -float_[_ToolPosZ()]) );
		// LatheHole
		rs11 = no_case[LATHEHOLE_S] >> lit('=');
		rv11 = float_[_SetLatheHole()] >> rrMM;
		// Inside
		rs12 = no_case[INSIDE_S];
		rv12 = as_string[rs12][_SetLatheInside()];
		// EndInside/EndDrill
		rs13 = no_case[sw::string(ENDINSIDE_S)|ENDDRILL_S];
		rv13 = as_string[rs13][_EndLatheInside()];
		// EndGrooving
		rs14 = no_case[sw::string(ENDGROOVE_S)];
		rv14 = as_string[rs14][_EndGrooving()];
		// "mm"
		rrMM = -no_case["mm"];	// LoadString(IDCV_MILI)使えない
	}

	// ｴﾝﾄﾞﾐﾙ径
	struct	_SetEndmill {
		void operator()(const float& d, qi::unused_type, qi::unused_type) const {
			g_ncArgv.dEndmill = d / 2.0f;
			g_ncArgv.nEndmillType = NCMIL_SQUARE;
#ifdef _DEBUG_GSPIRIT
			if ( !IsThumbnail() )
				printf("_SetEndmill() Endmill=%f\n", g_ncArgv.dEndmill);
#endif
		}
	};
	// ﾎﾞｰﾙｴﾝﾄﾞﾐﾙ表記
	struct	_SetBallMill {
		void operator()(const float& d, qi::unused_type, qi::unused_type) const {
			g_ncArgv.dEndmill = d;
			g_ncArgv.nEndmillType = NCMIL_BALL;
#ifdef _DEBUG_GSPIRIT
			if ( !IsThumbnail() )
				printf("SetBallEndmill() BallEndmill=R%f\n", g_ncArgv.dEndmill);
#endif
		}
	};
	// 面取りﾐﾙ表記
	struct	_SetChamferMill {
		void operator()(const float& d, qi::unused_type, qi::unused_type) const {
			g_ncArgv.dEndmill = d;
			g_ncArgv.nEndmillType = NCMIL_CHAMFER;
#ifdef _DEBUG_GSPIRIT
			if ( !IsThumbnail() )
				printf("SetChamfermill() Chamfermill=C%f\n", g_ncArgv.dEndmill);
#endif
		}
	};
	// ドリル表記
	struct	_SetDrill {
		void operator()(const float& d, qi::unused_type, qi::unused_type) const {
			g_ncArgv.dEndmill = d / 2.0f;
			g_ncArgv.nEndmillType = NCMIL_DRILL;
			if ( g_pDoc->IsDocFlag(NCDOC_LATHE) ) {
				g_pDoc->SetDocFlag(NCDOC_LATHE_INSIDE);
				g_ncArgv.nc.dwValFlags |= NCFLG_LATHE_HOLE;
			}
#ifdef _DEBUG_GSPIRIT
			if ( !IsThumbnail() )
				printf("SetDrill() Drill=%f\n", g_ncArgv.dEndmill);
#endif
		}
	};
	// 突っ切りバイト（旋盤専用）
	struct	_SetGrooving {
		void operator()(const float& d, qi::unused_type, qi::unused_type) const {
			if ( g_pDoc->IsDocFlag(NCDOC_LATHE) && !(g_ncArgv.nc.dwValFlags&NCFLG_LATHE_INSIDE) ) {
				g_ncArgv.dEndmill = d;
				g_ncArgv.nEndmillType = NCMIL_GROOVE;
#ifdef _DEBUG_GSPIRIT
				if ( !IsThumbnail() )
					printf("SetGrooving() width=%f\n", g_ncArgv.dEndmill);
#endif
			}
		}
	};
	struct	_SetGrooving_Right {
		void operator()(const float& d, qi::unused_type, qi::unused_type) const {
			if ( g_pDoc->IsDocFlag(NCDOC_LATHE) && !(g_ncArgv.nc.dwValFlags&NCFLG_LATHE_INSIDE) ) {
				g_ncArgv.dEndmill = d;
				g_ncArgv.nEndmillType = NCMIL_GROOVE_R;
#ifdef _DEBUG_GSPIRIT
				if ( !IsThumbnail() )
					printf("SetGrooving_Right() width=%f\n", g_ncArgv.dEndmill);
#endif
			}
		}
	};
	struct	_SetGrooving_Center {
		void operator()(const float& d, qi::unused_type, qi::unused_type) const {
			if ( g_pDoc->IsDocFlag(NCDOC_LATHE) && !(g_ncArgv.nc.dwValFlags&NCFLG_LATHE_INSIDE) ) {
				g_ncArgv.dEndmill = d / 2.0f;	// 中央基準だけ1/2
				g_ncArgv.nEndmillType = NCMIL_GROOVE_C;
#ifdef _DEBUG_GSPIRIT
				if ( !IsThumbnail() )
					printf("SetGrooving_Center() width=%f\n", g_ncArgv.dEndmill);
#endif
			}
		}
	};
	struct _EndGrooving {
		void operator()(const string& s, qi::unused_type, qi::unused_type) const {
			if ( g_pDoc->IsDocFlag(NCDOC_LATHE) ) {
				g_ncArgv.dEndmill = 0.0;
				g_ncArgv.nEndmillType = NCMIL_SQUARE;
#ifdef _DEBUG_GSPIRIT
				if ( !IsThumbnail() )
					printf("EndGrooving()\n");
#endif
			}
		}
	};
	// ﾜｰｸ矩形
	struct _SetWorkRect {
		void operator()(const float& d, qi::unused_type, qi::unused_type) const {
			if ( g_nWorkRect < SIZEOF(g_dWorkRect) )
				g_dWorkRect[g_nWorkRect++] = d;
		}
	};
	struct _SetWorkRectStr {
		void operator()(const boost::iterator_range<Iterator>& raw, qi::unused_type, qi::unused_type) const {
			int		n = 0;
			float	dVal[3];
			string	str(raw.begin(), raw.end());
			char_separator<TCHAR>	sep("xXtT");
			tokenizer< char_separator<TCHAR> > tok(str, sep);
			typedef tokenizer< char_separator<TCHAR> >::iterator ITE;
			for ( ITE it=tok.begin(); it!=tok.end() && n<SIZEOF(dVal); ++it ) {
				dVal[n++] = (float)atof( (*it).c_str() );
			}
			dVal[0] /= 2.0f;	dVal[1] /= 2.0f;
			CRect3F	rc(-dVal[0], -dVal[1], dVal[0], dVal[1], 0, -dVal[2]);
			g_pDoc->SetWorkRectComment(rc);
		}
	};
	// ﾜｰｸ円柱
	struct _SetWorkCylinder {
		void operator()(const float& d, qi::unused_type, qi::unused_type) const {
			if ( g_nWorkCylinder < SIZEOF(g_dWorkCylinder) )
				g_dWorkCylinder[g_nWorkCylinder++] = d;
		}
	};
	struct _SetWorkCylinderStr {
		void operator()(const boost::iterator_range<Iterator>& raw, qi::unused_type, qi::unused_type) const {
			int		n = 0;
			float	dVal[2];
			string	str(raw.begin(), raw.end());
			char_separator<TCHAR>	sep("hH");
			tokenizer< char_separator<TCHAR> > tok(str, sep);
			typedef tokenizer< char_separator<TCHAR> >::iterator ITE;
			for ( ITE it=tok.begin(); it!=tok.end() && n<SIZEOF(dVal); ++it ) {
				dVal[n++] = (float)atof( (*it).c_str() );
			}
			CPoint3F	pt(0, 0, -dVal[1]);
			g_pDoc->SetWorkCylinderComment(dVal[0], dVal[1], pt);
		}
	};
	// ﾜｰｸﾌｧｲﾙ
	struct _SetWorkFile {
		void operator()(const string& s, qi::unused_type, qi::unused_type) const {
			if ( !IsThumbnail() ) {
				if ( !g_pDoc->ReadWorkFile(s.c_str()) )
					g_dwBlockFlags |= NCBLK_ERR_FILE;
			}
		}
	};
	// 機械情報ﾌｧｲﾙ
	struct _SetMCFile {
		void operator()(const string& s, qi::unused_type, qi::unused_type) const {
			if ( !IsThumbnail() ) {
				if ( g_pDoc->ReadMCFile(s.c_str()) )
					g_pDoc->SetDocFlag(NCDOC_MC_CHANGE);
				else
					g_dwBlockFlags |= NCBLK_ERR_FILE;
			}
		}
	};
	// 旋盤表示ﾓｰﾄﾞ
	struct _SetLatheView {
		void operator()(const float& d, qi::unused_type, qi::unused_type) const {
			if ( g_nLatheView < SIZEOF(g_dLatheView) )
				g_dLatheView[g_nLatheView++] = d;
		}
	};
	// ﾜｲﾔ加工機表示ﾓｰﾄﾞ
	struct _SetWireView {
		void operator()(const float& d, qi::unused_type, qi::unused_type) const {
			if ( g_nWireView++ < 1 )
				g_dWireView = d;
		}
	};
	// 工具位置変更
	struct _ToolPosX {
		void operator()(const float& d, qi::unused_type, qi::unused_type) const {
			g_dToolPos[NCA_X] = d;
			g_dwBlockFlags |= NCBLK_TOOLX;
#ifdef _DEBUG_GSPIRIT
			if ( !IsThumbnail() )
				printf("ToolPosX()=%f\n", d);
#endif
		}
	};
	struct _ToolPosY {
		void operator()(const float& d, qi::unused_type, qi::unused_type) const {
			g_dToolPos[NCA_Y] = d;
			g_dwBlockFlags |= NCBLK_TOOLY;
#ifdef _DEBUG_GSPIRIT
			if ( !IsThumbnail() )
				printf("ToolPosY()=%f\n", d);
#endif
		}
	};
	struct _ToolPosZ {
		void operator()(const float& d, qi::unused_type, qi::unused_type) const {
			g_dToolPos[NCA_Z] = d;
			g_dwBlockFlags |= NCBLK_TOOLZ;
#ifdef _DEBUG_GSPIRIT
			if ( !IsThumbnail() )
				printf("ToolPosZ()=%f\n", d);
#endif
		}
	};
	// 旋盤中空
	struct _SetLatheHole {
		void operator()(const float& d, qi::unused_type, qi::unused_type) const {
			if ( g_pDoc->IsDocFlag(NCDOC_LATHE) ) {
				if ( d >= g_dLatheView[0] )	// 外径より大きいとエラー
					g_dwBlockFlags |= NCBLK_ERR_HOLE;
				else
					g_pDoc->SetWorkLatheHole(d / 2.0f);
			}
		}
	};
	// ﾜｰｸ位置変更
	struct _WorkPosX {
		void operator()(const float& d, qi::unused_type, qi::unused_type) const {
			g_dWorkPos[NCA_X] = d;
			g_dwBlockFlags |= NCBLK_WORKX;
#ifdef _DEBUG_GSPIRIT
			if ( !IsThumbnail() )
				printf("WorkPosX()=%f\n", d);
#endif
		}
	};
	struct _WorkPosY {
		void operator()(const float& d, qi::unused_type, qi::unused_type) const {
			g_dWorkPos[NCA_Y] = d;
			g_dwBlockFlags |= NCBLK_WORKY;
#ifdef _DEBUG_GSPIRIT
			if ( !IsThumbnail() )
				printf("WorkPosY()=%f\n", d);
#endif
		}
	};
	struct _WorkPosZ {
		void operator()(const float& d, qi::unused_type, qi::unused_type) const {
			g_dWorkPos[NCA_Z] = d;
			g_dwBlockFlags |= NCBLK_WORKZ;
#ifdef _DEBUG_GSPIRIT
			if ( !IsThumbnail() )
				printf("WorkPosZ()=%f\n", d);
#endif
		}
	};
	// 旋盤中ぐり
	struct _SetLatheInside {
		void operator()(const string& s, qi::unused_type, qi::unused_type) const {
			if ( g_pDoc->IsDocFlag(NCDOC_LATHE) ) {
				g_pDoc->SetDocFlag(NCDOC_LATHE_INSIDE);
				g_ncArgv.nc.dwValFlags |= NCFLG_LATHE_INPASS;
			}
		}
	};
	struct _EndLatheInside {
		void operator()(const string& s, qi::unused_type, qi::unused_type) const {
			g_ncArgv.nc.dwValFlags &= ~NCFLG_LATHE_INSIDE;
		}
	};
};

typedef qi::rule<string::iterator>	SkipperType;
#define confix_p(S, E)	repository::confix(S, E)[*(sw::char_ - E)]
						// (qi::char_(S) >> *(qi::char_ - E) >> E)

//////////////////////////////////////////////////////////////////////
// Gｺｰﾄﾞの分割(再帰関数)
int NC_GSeparater(INT_PTR nLine, CNCdata*& pDataResult)
{
	// static変数にすることで
	// 再入のたびに変数がｲﾝｽﾀﾝｽ化されることを防ぐ
	// 結果、ｽﾋﾟｰﾄﾞUP(??)
	static	SkipperType	skip_p = sw::space | confix_p('(', ')'),
						ignore_p = sw::space | confix_p("(/", ")");
	static	CGcodeParser<string::iterator, SkipperType>		gcode_p;
	static	CCommentParser<string::iterator, SkipperType>	comment_p;

	string		strBlock,		// NCﾌﾞﾛｯｸ（1行解析単位）
				strWord,		// 解析単位
				strComma;		// ｶﾝﾏ以降の文字列(次のﾌﾞﾛｯｸへ継ぐ)
	string::iterator	it;

	int			nCode,
				nNotModalCode = -1,	// ﾓｰﾀﾞﾙ不要のGｺｰﾄﾞ(ﾌﾞﾛｯｸ内でのみ有効)
				nResult;
	INT_PTR		i, nIndex;
	BOOL		bNCobj = FALSE, bNCval = FALSE, bNCsub = FALSE,
				bInvalidM = FALSE,
				bTcode = FALSE, bOptionalBlockSkip = FALSE;
	ENGCODEOBJ	enGcode;
	CNCdata*	pData;
	CNCblock*	pBlock = g_pDoc->GetNCblock(nLine);

	// 変数初期化
	g_ncArgv.nc.nLine		= (int)nLine;
	g_ncArgv.nc.nErrorCode	= 0;
	g_ncArgv.nc.dwValFlags &= NCD_CLEARVALUE;	// 0xffff0000
	g_ncArgv.taper.bTonly	= FALSE;
	g_nWorkRect = g_nWorkCylinder = g_nLatheView = g_nWireView = 0;
	g_dwBlockFlags = 0;

	strBlock = pBlock->GetStrGcode();

#ifdef _DEBUG_GSPIRIT
	if ( !IsThumbnail() )
		printf("NC_Gseparate() No.%004d Line=%s\n", nLine+1, strBlock.c_str());
#endif

	// ｻﾑﾈｲﾙ表示のときは処理しないﾙｰﾁﾝ
	if ( !IsThumbnail() ) {
		// 自動ﾌﾞﾚｲｸｺｰﾄﾞ検索
		SearchAutoBreak(strBlock, pBlock);
	}

	// ｺﾒﾝﾄ解析(解析ﾓｰﾄﾞ, ｴﾝﾄﾞﾐﾙ径, ﾜｰｸ矩形情報の取得)
	it = strBlock.begin();
	if ( qi::phrase_parse(it, strBlock.end(), comment_p, ignore_p) ) {
		if ( g_nWorkRect > 0 )
			SetWorkRect_fromComment();
		if ( g_nWorkCylinder > 0 )
			SetWorkCylinder_fromComment();
		if ( g_nLatheView > 0 )
			SetLatheView_fromComment();
		if ( g_nWireView > 0 )
			SetWireView_fromComment();
		if ( g_dwBlockFlags & NCBLK_TOOLPOS )
			pDataResult = SetToolPosition_fromComment(pBlock, pDataResult);	// create dummy object
		if ( g_dwBlockFlags & NCBLK_ERR_HOLE )
			pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_LATHEHOLE);
		if ( g_dwBlockFlags & NCBLK_ERR_FILE )
			pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_FILE);
#ifdef USE_KODATUNO
		else if ( g_dwBlockFlags & NCBLK_WORKPOS ) {
			float	xyz[NCXYZ] = {0,0,0};
			if ( g_dwBlockFlags & NCBLK_WORKX )
				xyz[NCA_X] = g_dWorkPos[NCA_X];
			if ( g_dwBlockFlags & NCBLK_WORKY )
				xyz[NCA_Y] = g_dWorkPos[NCA_Y];
			if ( g_dwBlockFlags & NCBLK_WORKZ )
				xyz[NCA_Z] = g_dWorkPos[NCA_Z];
			Coord shift = SetCoord(xyz[NCA_X], xyz[NCA_Y], xyz[NCA_Z]);
			g_pDoc->SetWorkFileOffset(shift);
		}
#endif
	}

	// ﾏｸﾛ置換解析
	if ( (nIndex=SearchMacro(strBlock, pBlock)) >= 0 ) {
		g_nSubprog++;
		// M98ｵﾌﾞｼﾞｪｸﾄとO番号(nIndex)の登録
		AddM98code(pBlock, pDataResult, nIndex);
		// SearchMacro でﾌﾞﾛｯｸが追加される可能性アリ
		// ここでは nLoop 変数を使わず、ﾈｲﾃｨﾌﾞのﾌﾞﾛｯｸｻｲｽﾞにて判定
		for ( i=nIndex; i<g_pDoc->GetNCBlockSize() && IsThread(); i++ ) {
			nResult = NC_GSeparater(i, pDataResult);	// 再帰
			if ( nResult == 30 )
				return 30;
			else if ( nResult == 99 )
				break;
		}
		// EOFで終了ならM99復帰扱い
		if ( i >= g_pDoc->GetNCBlockSize() && nResult == 0 ) {
			if ( g_nSubprog > 0 )
				g_nSubprog--;
		}
		return 0;
	}

	// Gｺｰﾄﾞ構文解析
	it = strBlock.begin();
	while ( IsThread() && !bOptionalBlockSkip ) {
		strWord.clear();
		if ( !qi::phrase_parse(it, strBlock.end(), gcode_p, skip_p, strWord) )
			break;
#ifdef _DEBUG_GSPIRIT
		if ( !IsThumbnail() ) {
			printf("G Cut=%s\n", strWord.c_str());
		}
#endif
		switch ( strWord[0] ) {
		case '/':
			if ( strWord.length() == 1 )
				nCode = 0;
			else {
				nCode = strWord[1] - '0';	// 0～9
				if ( nCode < 0 || nCode > 9 )
					nCode = 0;
			}
			if ( AfxGetNCVCApp()->GetMCOption()->GetFlag(nCode) )
				bOptionalBlockSkip = TRUE;
			break;
		case 'M':
			// 前回のｺｰﾄﾞで登録ｵﾌﾞｼﾞｪｸﾄがあるなら
			if ( bNCobj ) {
				// ｵﾌﾞｼﾞｪｸﾄ生成
				pData = AddGcode(pBlock, pDataResult, nNotModalCode);
				// 面取りｵﾌﾞｼﾞｪｸﾄの登録
				if ( !g_strComma.empty() )
					MakeChamferingObject(pBlock, pDataResult, pData);
				pDataResult = pData;
				g_strComma = strComma;
				nNotModalCode = -1;
				bNCobj = bNCval = FALSE;
			}
			// 前回のｺｰﾄﾞでｻﾌﾞﾌﾟﾛ呼び出しがあれば
			if ( bNCsub ) {
				bNCsub = FALSE;
				if ( CallSubProgram(pBlock, pDataResult) == 30 )
					return 30;	// 終了ｺｰﾄﾞ
			}
			nCode = _GetGcode(strWord.substr(1));
			switch ( nCode ) {
			case 2:
			case 30:
				return 30;
			case 98:
				// 5階層以上の呼び出しはｴﾗｰ(4階層まで)
				if ( g_nSubprog+1 >= 5 ) {
					pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_M98L);
					continue;
				}
				bNCsub = TRUE;
				break;
			case 99:
				if ( g_nSubprog > 0 ) {
					g_nSubprog--;
					// 復帰用ｵﾌﾞｼﾞｪｸﾄ生成
					AddM98code(pBlock, pDataResult, -1);
					return 99;
				}
				// through
			default:
				bInvalidM = TRUE;	// 無効なMｺｰﾄﾞ
			}
			break;
		case 'G':
			// 先に現在のｺｰﾄﾞ種別をﾁｪｯｸ
			nCode = _GetGcode(strWord.substr(1));
			enGcode = IsGcode(nCode);	// IsGcodeObject_～
			if ( enGcode == NOOBJ ) {
				nResult = CheckGcodeOther(nCode);
				if ( nResult > 0 )
					pBlock->SetNCBlkErrorCode(nResult);
				break;
			}
			// 前回のｺｰﾄﾞで登録ｵﾌﾞｼﾞｪｸﾄがあるなら
			if ( bNCobj ) {
				pData = AddGcode(pBlock, pDataResult, nNotModalCode);
				if ( !g_strComma.empty() )
					MakeChamferingObject(pBlock, pDataResult, pData);
				pDataResult = pData;
				g_strComma = strComma;
				nNotModalCode = -1;
			}
			if ( bNCsub ) {
				bNCsub = FALSE;
				if ( CallSubProgram(pBlock, pDataResult) == 30 )
					return 30;	// 終了ｺｰﾄﾞ
			}
			// 現在のｺｰﾄﾞ保管
			if ( enGcode == MAKEOBJ )
				g_ncArgv.nc.nGcode = nCode;
			else
				nNotModalCode = nCode;
			bNCobj = TRUE;
			bInvalidM = FALSE;
			break;
		case 'F':
			g_ncArgv.dFeed = FeedAnalyze(strWord.substr(1));
			break;
		case 'S':
			g_ncArgv.nSpindle = abs(atoi(strWord.substr(1).c_str()));
			break;
		case 'T':
			if ( g_pDoc->IsDocFlag(NCDOC_WIRE) ) {
				// ﾃｰﾊﾟ角度ｾｯﾄ
				nResult = SetTaperAngle(strWord.substr(1));
				if ( nResult > 0 )	
					pBlock->SetNCBlkErrorCode(nResult);
				else
					bTcode = TRUE;	// Tｺｰﾄﾞ指定(ﾜｲﾔのみ)
			}
			else {
				SetEndmillDiameter(strWord.substr(1));	// ﾂｰﾙ番号
			}
			break;
		case ',':
			if ( g_pDoc->IsDocFlag(NCDOC_WIRE) ) {
				// ﾜｲﾔﾓｰﾄﾞでｻﾎﾟｰﾄすべきか...
				string	strTmp = boost::algorithm::trim_copy(strWord.substr(1));
				if ( strTmp[0] == 'R' ) {
					g_ncArgv.nc.dValue[NCA_R] = GetNCValue(strTmp.substr(1));
					g_ncArgv.nc.dwValFlags |= NCD_R;
				}
			}
			else {
				strComma = boost::algorithm::trim_copy(strWord.substr(1));	// ｶﾝﾏ以降を取得
#ifdef _DEBUG_GSPIRIT
				if ( !IsThumbnail() )
					printf("strComma=%s\n", strComma.c_str());
#endif
			}
			break;
		case 'X':	case 'Y':	case 'Z':
		case 'U':	case 'V':	case 'W':
		case 'I':	case 'J':	case 'K':	case 'R':
		case 'P':	case 'L':
		case 'D':	case 'H':
			if ( bInvalidM )
				break;	// 無効なMｺｰﾄﾞに続くｱﾄﾞﾚｽ値は無視
			nCode = (int)(strchr(g_szNdelimiter, strWord[0]) - g_szNdelimiter);
			// 値取得
			if ( g_Cycle.bCycle ) {		// 81～89
				// 固定ｻｲｸﾙの特別処理
				switch ( nCode ) {
				case NCA_K:		// Kはﾈｲﾃｨﾌﾞで(これ何やろ？)
					g_ncArgv.nc.dValue[NCA_K] = atoi(strWord.substr(1).c_str());
					break;
				case NCA_P:
					if ( bNCsub ) {
						// 固定ｻｲｸﾙﾓｰﾄﾞ中のM98ｻﾌﾞﾌﾟﾛ呼び出し対応
						g_ncArgv.nc.dValue[NCA_P] = atoi(strWord.substr(1).c_str());
						break;
					}
					// through
				default:	// P(ﾄﾞｳｪﾙ時間)もGetNCValue()でOK
					g_ncArgv.nc.dValue[nCode] = GetNCValue(strWord.substr(1));
				}
			}
			else if ( g_pDoc->IsDocFlag(NCDOC_WIRE) ) {
				// ﾜｲﾔﾓｰﾄﾞにおける特別処理(L値)
				g_ncArgv.nc.dValue[nCode] = nCode<GVALSIZE || nCode==NCA_L ?
					GetNCValue(strWord.substr(1)) : atoi(strWord.substr(1).c_str());
			}
			else {
				g_ncArgv.nc.dValue[nCode] = nCode < GVALSIZE ?	// nCode < NCA_P
					GetNCValue(strWord.substr(1)) : atoi(strWord.substr(1).c_str());
			}
			g_ncArgv.nc.dwValFlags |= g_dwSetValFlags[nCode];
			bNCval = TRUE;
			break;
		}	// End of switch()
	}	// End of while() phrase_parse

	if ( bTcode && !bNCobj && !bNCval ) {
		// Tｺｰﾄﾞ単独指示(ﾜｲﾔのみ)
		nNotModalCode = 01;		// dummy
		g_ncArgv.taper.bTonly = TRUE;	// TH_UVWire.cpp での処理目印
		bNCobj = TRUE;			// dummy object の生成
	}
	//
	if ( bNCsub && bNCval ) {
		// M98での[P_|L_]のｵﾌﾞｼﾞｪｸﾄ生成を抑制
		if ( !(g_ncArgv.nc.dwValFlags & ~(NCD_P|NCD_L)) )
			bNCval = FALSE;
	}
	//
	if ( bNCobj || bNCval ) {
		// NCﾃﾞｰﾀ登録処理
		pData = AddGcode(pBlock, pDataResult, nNotModalCode);
		if ( !g_strComma.empty() )
			MakeChamferingObject(pBlock, pDataResult, pData);
		pDataResult = pData;
		g_strComma = strComma;
		// ﾌﾞﾛｯｸ情報の更新
		pBlock->SetBlockToNCdata(pDataResult, g_pDoc->GetNCsize());
	}
	if ( bNCsub ) {
		// Mｺｰﾄﾞ後処理
		if ( CallSubProgram(pBlock, pDataResult) == 30 )
			return 30;	// 終了ｺｰﾄﾞ
	}

	return 0;
}

CNCdata* AddGcode(CNCblock* pBlock, CNCdata* pDataBefore, int nNotModalCode)
{
	CNCdata*	pDataResult = pDataBefore;

	if ( !g_pDoc->IsDocFlag(NCDOC_WIRE) ) {
		// ﾜｲﾔ加工以外はUVW座標の加算
		for ( int i=0; i<NCXYZ; i++ ) {
			if ( g_ncArgv.nc.dwValFlags & g_dwSetValFlags[i+NCA_U] ) {
				g_ncArgv.nc.dValue[i]  += g_ncArgv.nc.dValue[i+NCA_U];
				g_ncArgv.nc.dwValFlags |= g_dwSetValFlags[i];
			}
		}
	}

	if ( g_pDoc->IsDocFlag(NCDOC_LATHE) ) {
		// 旋盤ﾓｰﾄﾞでの座標入れ替え(ﾄﾞｳｪﾙ除く)
		if ( g_ncArgv.nc.nGcode != 4 ) {
			optional<double>	x, z, i, k;
			if ( g_ncArgv.nc.dwValFlags & NCD_X )
				x = g_ncArgv.nc.dValue[NCA_X] / 2.0;	// 直径指示
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
		// NCﾃﾞｰﾀの登録前処理
		if ( g_ncArgv.nc.nGtype == G_TYPE ) {
			switch ( nNotModalCode ) {
			case 68:
				// G68座標回転指示のﾁｪｯｸ
				G68RoundCheck(pBlock);
				if ( !g_ncArgv.g68.bG68 )
					return pDataResult;
				break;
			case 92:
				if ( g_pDoc->IsDocFlag(NCDOC_WIRE) && g_ncArgv.nc.dwValFlags&NCD_J ) {
					// ﾌﾟﾛｸﾞﾗﾑ面(XY軸)の設定
					g_ncArgv.nc.dValue[NCA_Z] = g_ncArgv.nc.dValue[NCA_J];
					g_ncArgv.nc.dwValFlags |= NCD_Z;
				}
				break;
			}
		}
	}

	if ( g_Cycle.bCycle && g_ncArgv.nc.nGtype==G_TYPE && !g_pDoc->IsDocFlag(NCDOC_WIRE) ) {
		// 固定ｻｲｸﾙのﾓｰﾀﾞﾙ補間
		CycleInterpolate();
	}

	// NCﾃﾞｰﾀの登録
	// --- 面取り等による再計算項目があるが，
	// --- この時点でｵﾌﾞｼﾞｪｸﾄ登録しておかないと色々面倒(?)
	if ( nNotModalCode >= 0 ) {
		int nGcode = g_ncArgv.nc.nGcode;	// ﾓｰﾀﾞﾙｺｰﾄﾞﾊﾞｯｸｱｯﾌﾟ
		g_ncArgv.nc.nGcode = nNotModalCode;
		pDataResult = g_pDoc->DataOperation(pDataBefore, &g_ncArgv);
		g_ncArgv.nc.nGcode = nGcode;
	}
	else
		pDataResult = g_pDoc->DataOperation(pDataBefore, &g_ncArgv);

	return pDataResult;
}

void AddM98code(CNCblock* pBlock, CNCdata* pDataBefore, INT_PTR nIndex)
{
	CNCdata*	pData;
	DWORD		dwFlags = g_ncArgv.nc.dwValFlags;

	g_ncArgv.nc.dwValFlags = 0;
	g_ncArgv.nc.nGtype = M_TYPE;
	if ( nIndex >= 0 ) {
		pData = AddGcode(pBlock, pDataBefore, 98);
		g_ncArgv.nc.nGtype = O_TYPE;
		g_ncArgv.nc.nLine  = (int)nIndex;
		pData = AddGcode(g_pDoc->GetNCblock(nIndex), pData, 0);
	}
	else
		pData = AddGcode(pBlock, pDataBefore, 99);

	// 復元
	g_ncArgv.nc.nGtype = G_TYPE;
	g_ncArgv.nc.dwValFlags = dwFlags;

	// M98コードはオブジェクト登録するが
	// pDataBefore は更新しない
}

int CallSubProgram(CNCblock* pBlock, CNCdata*& pDataResult)
{
	int			nResult = 0;
	INT_PTR		i, nIndex, nRepeat;

	if ( !(g_ncArgv.nc.dwValFlags & NCD_P) )
		pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_ORDER);
	else if ( (nIndex=NC_SearchSubProgram(&nRepeat)) < 0 )
		pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_M98);
	else {
		// M98ｵﾌﾞｼﾞｪｸﾄとO番号(nIndex)の登録
		AddM98code(pBlock, pDataResult, nIndex);
		// nRepeat分繰り返し
		while ( nRepeat-- > 0 && IsThread() ) {
			g_nSubprog++;	// M99で--されるのでここで++
			// NC_SearchSubProgram でﾌﾞﾛｯｸが追加される可能性アリ
			// ここでは nLoop 変数を使わず、ﾈｲﾃｨﾌﾞのﾌﾞﾛｯｸｻｲｽﾞにて判定
			for ( i=nIndex; i<g_pDoc->GetNCBlockSize() && IsThread(); i++ ) {
				nResult = NC_GSeparater(i, pDataResult);	// 再帰
				if ( nResult == 30 )
					return 30;
				else if ( nResult == 99 )
					break;
			}
			// EOFで終了ならM99復帰扱い
			if ( i >= g_pDoc->GetNCBlockSize() && nResult == 0 )
				g_nSubprog--;
		}
	}

	return nResult;
}

//////////////////////////////////////////////////////////////////////
// 補助関数

ENGCODEOBJ	IsGcodeObject_Milling(int nCode)
{
	// G00～G03, G04, G10, G52, G8x, G92
	// ｵﾌﾞｼﾞｪｸﾄ生成するＧｺｰﾄﾞﾁｪｯｸ
	ENGCODEOBJ	enResult;

	switch ( nCode ) {
	case 0:		case 1:		case 2:		case 3:
		g_Cycle.bCycle = FALSE;
		enResult = MAKEOBJ;
		break;
	case 73:	case 74:	case 76:
	case 81:	case 82:	case 83:	case 84:	case 85:
	case 86:	case 87:	case 88:	case 89:
		g_Cycle.bCycle	= TRUE;
		g_Cycle.bAbs	= g_ncArgv.bAbs;
		g_Cycle.dValI	= g_ncArgv.nc.dValue[_GetPlaneZ()];
		enResult = MAKEOBJ;
		break;
	case 4:		case 10:	case 28:	case 52:	case 68:
	case 92:
		enResult = MAKEOBJ_NOTMODAL;
		break;
	default:
		enResult = NOOBJ;
	}

	return enResult;
}

ENGCODEOBJ	IsGcodeObject_Wire(int nCode)
{
	ENGCODEOBJ	enResult;

	switch ( nCode ) {
	case 0:		case 1:		case 2:		case 3:
		enResult = MAKEOBJ;
		break;
	case 4:		case 10:	case 11:	case 28:	
	case 92:	case 93:
		enResult = MAKEOBJ_NOTMODAL;
		break;
	default:
		enResult = NOOBJ;
	}

	return enResult;
}

ENGCODEOBJ	IsGcodeObject_Lathe(int nCode)
{
	ENGCODEOBJ	enResult;

	switch ( nCode ) {
	case 0:		case 1:		case 2:		case 3:
		g_Cycle.bCycle = FALSE;
		enResult = MAKEOBJ;
		break;
	case 83:	case 84:	case 85:	// 端面穴あけ
		g_Cycle.bCycle	= TRUE;
		g_Cycle.bAbs	= g_ncArgv.bAbs;
		g_Cycle.dValI	= g_ncArgv.nc.dValue[_GetPlaneZ()];
		enResult = MAKEOBJ;
		break;
	case 4:		case 10:	case 28:
		enResult = MAKEOBJ_NOTMODAL;
		break;
	default:
		enResult = NOOBJ;
	}

	return enResult;
}

// 切削ｺｰﾄﾞ以外の重要なGｺｰﾄﾞ検査
int CheckGcodeOther_Milling(int nCode)
{
	int		nResult = 0;

	switch ( nCode ) {
	// ﾍﾘｶﾙ平面指定
	case 17:
		g_ncArgv.nc.enPlane = XY_PLANE;
		break;
	case 18:
		g_ncArgv.nc.enPlane = XZ_PLANE;
		break;
	case 19:
		g_ncArgv.nc.enPlane = YZ_PLANE;
		break;
	// 工具径補正
	case 40:
		g_ncArgv.nc.dwValFlags &= ~NCD_CORRECT;
		break;
	case 41:
		g_ncArgv.nc.dwValFlags |= NCD_CORRECT_L;
		break;
	case 42:
		g_ncArgv.nc.dwValFlags |= NCD_CORRECT_R;
		break;
	// ﾜｰｸ座標系
	case 54: case 55: case 56: case 57: case 58: case 59:
		g_pDoc->SelectWorkOffset(nCode - 54);
		break;
	// 座標回転ｷｬﾝｾﾙ
	case 69:
		G68RoundClear();
		break;
	// 固定ｻｲｸﾙｷｬﾝｾﾙ
	case 80:
		g_Cycle.clear();
		break;
	// ｱﾌﾞｿﾘｭｰﾄ, ｲﾝｸﾘﾒﾝﾄ
	case 90:
		g_ncArgv.bAbs = TRUE;
		g_Cycle.ChangeAbs();	// 固定サイクル用の補間情報を更新
		break;
	case 91:
		g_ncArgv.bAbs = FALSE;
		g_Cycle.ChangeInc();
		break;
	// 固定ｻｲｸﾙ復帰
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
	// ﾃｰﾊﾟ処理
	case 50:
		g_ncArgv.taper.nTaper = 0;
		break;
	case 51: case 52:
		g_ncArgv.taper.nTaper = nCode==51 ? 1 : -1;
		break;
	// ﾜｰｸ座標系
	case 54: case 55: case 56: case 57: case 58: case 59:
		g_pDoc->SelectWorkOffset(nCode - 54);
		break;
	// 上下独立ｺｰﾅｰ
	case 60: case 61: case 62: case 63:
		if ( g_ncArgv.taper.nTaper == 0 )
			nResult = IDS_ERR_NCBLK_TAPER;
		else
			g_ncArgv.taper.nDiff = nCode - 60;
		break;
	// ｱﾌﾞｿﾘｭｰﾄ, ｲﾝｸﾘﾒﾝﾄ
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
	// 刃先Ｒ補正
//	case 40:
//		g_ncArgv.nc.dwValFlags &= ~NCD_CORRECT;
//		break;
//	case 41:
//		g_ncArgv.nc.dwValFlags |= NCD_CORRECT_L;
//		break;
//	case 42:
//		g_ncArgv.nc.dwValFlags |= NCD_CORRECT_R;
//		break;
	// ﾜｰｸ座標系
	case 54: case 55: case 56: case 57: case 58: case 59:
		g_pDoc->SelectWorkOffset(nCode - 54);
		break;
	// 固定ｻｲｸﾙｷｬﾝｾﾙ
	case 80:
		g_Cycle.clear();
		break;
	// ｱﾌﾞｿﾘｭｰﾄ, ｲﾝｸﾘﾒﾝﾄ
	case 90:
		g_ncArgv.bAbs = TRUE;
		g_Cycle.ChangeAbs();	// 固定サイクル用の補間情報を更新
		break;
	case 91:
		g_ncArgv.bAbs = FALSE;
		g_Cycle.ChangeInc();
		break;
	// 毎分送り
	case 98:
		g_ncArgv.bG98 = TRUE;
		break;
	// 毎回転送り
	case 99:
		g_ncArgv.bG98 = FALSE;
		break;
	}

	return nResult;
}

// ｻﾌﾞﾌﾟﾛｸﾞﾗﾑの検索
INT_PTR NC_SearchSubProgram(INT_PTR* pRepeat)
{
	INT_PTR	nProg, n;

	if ( g_ncArgv.nc.dwValFlags & NCD_L ) {
		*pRepeat = (INT_PTR)g_ncArgv.nc.dValue[NCA_L];
		nProg    = (INT_PTR)g_ncArgv.nc.dValue[NCA_P];
	}
	else {
		// L:繰り返し数が指定されていなければ，
		// [times][number] (n桁:4桁) を取得
		string	strBuf = lexical_cast<string>((int)g_ncArgv.nc.dValue[NCA_P]);
		n = strBuf.length();
		if ( n > 4 ) {
			*pRepeat = atoi(strBuf.substr(0, n-4).c_str());
			nProg    = atoi(strBuf.substr(n-4, 4).c_str());
		}
		else {
			*pRepeat = 1;
			nProg    = (INT_PTR)g_ncArgv.nc.dValue[NCA_P];
		}
	}

	// 正規表現(Oxxxxにﾏｯﾁする)
	regex	r("^O(0)*"+lexical_cast<string>(nProg)+"$");

	// 現在の(同じ)ﾒﾓﾘﾌﾞﾛｯｸから検索
	n = g_pDoc->SearchBlockRegex(r);
	if ( n >= 0 )
		return n;

	// 機械情報ﾌｫﾙﾀﾞからﾌｧｲﾙ検索
	CString	strFile( SearchFolder(r) );
	if ( strFile.IsEmpty() )
		return -1;

	// Ｏ番号が存在すればNCﾌﾞﾛｯｸの挿入 ->「ｶｰｿﾙ位置に読み込み」でﾌﾞﾛｯｸ追加
	n = g_pDoc->GetNCBlockSize();
	if ( g_pDoc->SerializeInsertBlock(strFile, n, NCF_AUTOREAD) ) {
		// 挿入ﾌﾞﾛｯｸの最初だけNCF_FOLDER
		if ( n < g_pDoc->GetNCBlockSize() )	// 挿入前 < 挿入後
			g_pDoc->GetNCblock(n)->SetBlockFlag(NCF_FOLDER);
		return n;
	}

	return -1;
}

// ﾏｸﾛﾌﾟﾛｸﾞﾗﾑの検索
INT_PTR NC_SearchMacroProgram(const string& strBlock, CNCblock* pBlock)
{
	extern	const	int		g_nDefaultMacroID[];	// MCOption.cpp

	const CMCOption* pMCopt = AfxGetNCVCApp()->GetMCOption();

	if ( !regex_search(strBlock, g_reMacroStr) )
		return -1;
	// 5階層以上の呼び出しはｴﾗｰ(4階層まで)
	if ( g_nSubprog+1 >= 5 ) {
		pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_M98L);
		return -1;
	}

	// ﾏｸﾛ引数の解析
	CString	strMacroFile,	// 一時出力ﾌｧｲﾙ
			strArgv(pMCopt->GetMacroStr(MCMACROARGV)),	// 引数
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
	// ﾏｸﾛ変換I/F起動
	AfxGetNCVCMainWnd()->CreateOutsideProcess(
		pMCopt->GetMacroStr(MCMACROIF), strArgv, FALSE, TRUE);
	// ﾏｸﾛ展開一時ﾌｧｲﾙを登録 -> ﾄﾞｷｭﾒﾝﾄ破棄後に消去
	g_pDoc->AddMacroFile(strMacroFile);
	// ﾌﾞﾛｯｸ挿入
	INT_PTR	n = g_pDoc->GetNCBlockSize();
	if ( g_pDoc->SerializeInsertBlock(strMacroFile, n, NCF_AUTOREAD) ) {
		// 挿入ﾌﾞﾛｯｸの最初だけNCF_FOLDER
		if ( n < g_pDoc->GetNCBlockSize() )	// ﾌﾞﾛｯｸ挿入が失敗の可能性もある
			g_pDoc->GetNCblock(n)->SetBlockFlag(NCF_FOLDER);
		return n;
	}

	return -1;
}

// 自動ﾌﾞﾚｲｸｺｰﾄﾞの検索
INT_PTR NC_SearchAutoBreak(const string& strBlock, CNCblock* pBlock)
{
	if ( regex_search(strBlock, g_reAutoBreak) )
		pBlock->SetBlockFlag(NCF_BREAK);

	return 0;	// dummy
}

INT_PTR NC_NoSearch(const string&, CNCblock*)
{
	return -1;
}

void MakeChamferingObject(CNCblock* pBlock, CNCdata* pData1, CNCdata* pData2)
{
#ifdef _DEBUG
	printf("MakeChamferingObject()\n");
#endif
	// ﾃﾞｰﾀﾁｪｯｸ
	if ( g_pDoc->IsDocFlag(NCDOC_LATHE) ) {
		// 旋盤ﾓｰﾄﾞではｻﾎﾟｰﾄされない
		pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_NOTLATHE);
		return;
	}
	if ( !pData1->IsCutCode() || !pData2->IsCutCode() ) {
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
	TCHAR	cCham = g_strComma[0];
	if ( cCham != 'R' && cCham != 'C' ) {
		pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_CHAMFERING);
		return;
	}

	float	r1, r2, cr = GetNCValue(g_strComma.substr(1));
	CPointF	pts, pte, pto, ptOffset(g_pDoc->GetOffsetOrig());
	optional<CPointF>	ptResult;
	BOOL	bResult;

	// 計算開始
	if ( cr < NCMIN ) {
		r1 = r2 = 0.0f;
		cCham = 'C';		// nGcode = 1
	}
	else if ( cCham == 'C' )
		r1 = r2 = cr;
	else {
		// ｺｰﾅｰRの場合は，面取りに相当するC値の計算
		tie(bResult, pto, r1, r2) = pData1->CalcRoundPoint(pData2, cr);
		if ( !bResult ) {
			pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_INTERSECTION);
			return;
		}
		pto -= ptOffset;
	}

	// pData1(前のｵﾌﾞｼﾞｪｸﾄ)の終点を補正
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
	// pData2(次のｵﾌﾞｼﾞｪｸﾄ)の始点を補正
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
		printf("%c=%f, %f\n", cCham, r1, r2);
		printf("pts=(%f, %f)\n", pts.x, pts.y);
		printf("pte=(%f, %f)\n", pte.x, pte.y);
	}
#endif

	// pts, pte で面取りｵﾌﾞｼﾞｪｸﾄの生成
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
//	memcpy(&(ncArgv.g68),   &(pData1->GetReadData()->m_g68),   sizeof(G68ROUND));
//	memcpy(&(ncArgv.taper), &(pData1->GetReadData()->m_taper), sizeof(TAPER));
	if ( pData1->GetReadData()->m_pG68 ) {
		ncArgv.g68.bG68		= TRUE;
		ncArgv.g68.enPlane	= pData1->GetReadData()->m_pG68->enPlane;
		ncArgv.g68.dRound	= pData1->GetReadData()->m_pG68->dRound;
		for ( int ii=0; ii<SIZEOF(ncArgv.g68.dOrg); ii++ )
			ncArgv.g68.dOrg[ii] = pData1->GetReadData()->m_pG68->dOrg[ii];
	}
	else {
		ncArgv.g68.bG68 = FALSE;
	}
	if ( pData1->GetReadData()->m_pTaper ) {
		ncArgv.taper.nTaper	= pData1->GetReadData()->m_pTaper->nTaper;
		ncArgv.taper.dTaper	= pData1->GetReadData()->m_pTaper->dTaper;
		ncArgv.taper.nDiff	= pData1->GetReadData()->m_pTaper->nDiff;
		ncArgv.taper.bTonly	= pData1->GetReadData()->m_pTaper->bTonly;
	}
	else {
		ncArgv.taper.nTaper = 0;
		ncArgv.taper.bTonly = FALSE;
	}
	// 座標値のｾｯﾄ
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
		// ｺｰﾅｰRの場合は，求めたｺｰﾅｰRの中心(pto)から回転方向を計算
		float	pa, pb;
		pts -= pto;		pte -= pto;
		if ( (pa=pts.arctan()) < 0.0f )
			pa += PI2;
		if ( (pb=pte.arctan()) < 0.0f )
			pb += PI2;
		if ( fabs(pa-pb) > PI ) {
			if ( pa > pb )
				pa -= PI2;
			else
				pb -= PI2;
		}
		ncArgv.nc.nGcode = pa > pb ? 2 : 3;
		ncArgv.nc.dValue[NCA_R] = cr;
		ncArgv.nc.dwValFlags |= NCD_R;
	}
	// 既に登録された１つ前に面取りｵﾌﾞｼﾞｪｸﾄを挿入
	g_pDoc->DataOperation(pData1, &ncArgv, g_pDoc->GetNCsize()-1, NCINS);
}

float FeedAnalyze_Dot(const string& str)
{
	return fabs(GetNCValue(str));
}

float FeedAnalyze_Int(const string& str)
{
	return (float)fabs(atof(str.c_str()));
}

void SetEndmillDiameter(const string& str)
{
#ifdef _DEBUG
	printf("SetEndmillDiameter()\n");
#endif

	int		nTool = atoi(str.c_str());
	const CMCOption* pMCopt = AfxGetNCVCApp()->GetMCOption();
	optional<float> dResult = pMCopt->GetToolD(nTool);
	if ( dResult ) {
		g_ncArgv.dEndmill = *dResult;	// ｵﾌｾｯﾄは半径なので、そのまま使用
#ifdef _DEBUG
		if ( !IsThumbnail() )
			printf("Endmill=%f from T-No.%d\n", g_ncArgv.dEndmill, stoi(str));
#endif
	}
#ifdef _DEBUG
	else {
		if ( !IsThumbnail() )
			printf("Endmill T-No.%d nothing\n", stoi(str));
	}
#endif
	g_ncArgv.nEndmillType = pMCopt->GetMillType(nTool);
}

int SetTaperAngle(const string& str)
{
#ifdef _DEBUG
	printf("SetTaperAngle()\n");
#endif
	int		nResult = 0;
	float	dTaper  = (float)atof(str.c_str());

	if ( g_ncArgv.taper.nTaper == 0 ) {
		if ( dTaper == 0.0f )
			g_ncArgv.taper.dTaper = 0.0;
		else
			nResult = IDS_ERR_NCBLK_TAPER;
	}
	else {
		if ( 45.0 < fabs(dTaper) )
			nResult = IDS_ERR_NCBLK_OVER;
		else
			g_ncArgv.taper.dTaper = RAD(dTaper);	// ﾗｼﾞｱﾝ保持
#ifdef _DEBUG
		if ( !IsThumbnail() ) {
			int dbgTaper = 0;
			if ( g_ncArgv.taper.nTaper == 1 )
				dbgTaper = 51;
			else if ( g_ncArgv.taper.nTaper == -1 )
				dbgTaper = 52;
			printf("Mode=%d angle=%f\n", dbgTaper, dTaper);
		}
#endif
	}

	return nResult;
}

CNCdata* SetToolPosition_fromComment(CNCblock* pBlock, CNCdata* pDataBefore)
{
	CNCdata*	pData;
	DWORD		dwToolPos = g_dwBlockFlags >> NCXYZ;

	for ( int i=0; i<NCXYZ; i++ ) {
		if ( dwToolPos & g_dwSetValFlags[i] ) {
			g_ncArgv.nc.dValue[i] = g_dToolPos[i];
			g_ncArgv.nc.dwValFlags |= g_dwSetValFlags[i];
		}
	}
	// 描画しないｵﾌﾞｼﾞｪｸﾄで生成
	int nGtype = g_ncArgv.nc.nGtype;
	g_ncArgv.nc.nGtype = O_TYPE;
	pData = AddGcode(pBlock, pDataBefore, -1);
	// 復元
	g_ncArgv.nc.nGtype = nGtype;

	return pData;
}

void SetWorkRect_fromComment(void)
{
#ifdef _DEBUG
	printf("SetWorkRect_fromComment()\n");
#endif
	CRect3F		rc;
	CPoint3F	pt;

	// 不細工やけどvWorkの数が不定なので有効なﾃﾞｰﾀだけ取り出し
	for ( int i=0; i<g_nWorkRect && i<SIZEOF(g_dWorkRect); i++ ) {
		switch ( i ) {
		case 0:		// 幅
			rc.right = g_dWorkRect[0];
			break;
		case 1:		// 奥行
			rc.bottom = g_dWorkRect[1];
			break;
		case 2:		// 高さ
			rc.high = g_dWorkRect[2];
			break;
		case 3:		// Xｵﾌｾｯﾄ
			pt.x = g_dWorkRect[3];
			break;
		case 4:		// Yｵﾌｾｯﾄ
			pt.y = g_dWorkRect[4];
			break;
		case 5:		// Zｵﾌｾｯﾄ
			pt.z = g_dWorkRect[5];
			break;
		}
	}
	rc.OffsetRect(pt);

	// ﾄﾞｷｭﾒﾝﾄが保持するﾜｰｸ矩形の更新
	g_pDoc->SetWorkRectComment(rc);

#ifdef _DEBUG
	if ( !IsThumbnail() ) {
		printf("(%f,%f)-(%f,%f)\n", rc.left, rc.top, rc.right, rc.bottom);
		printf("(%f,%f)\n", rc.low, rc.high);
	}
#endif
}

void SetWorkCylinder_fromComment(void)
{
#ifdef _DEBUG
	printf("SetWorkCylinder_fromComment()\n");
#endif
	float		d, h;
	CPoint3F	pt;

	for ( int i=0; i<g_nWorkCylinder && i<SIZEOF(g_dWorkCylinder); i++ ) {
		switch ( i ) {
		case 0:		// 直径
			d = g_dWorkCylinder[0];
			break;
		case 1:		// 高さ
			h = g_dWorkCylinder[1];
			break;
		case 2:		// Xｵﾌｾｯﾄ
			pt.x = g_dWorkCylinder[2];
			break;
		case 3:		// Yｵﾌｾｯﾄ
			pt.y = g_dWorkCylinder[3];
			break;
		case 4:		// Zｵﾌｾｯﾄ
			pt.z = g_dWorkCylinder[4];
			break;
		}
	}

	// ﾄﾞｷｭﾒﾝﾄが保持する円柱情報の更新
	g_pDoc->SetWorkCylinderComment(d, h, pt);

#ifdef _DEBUG
	if ( !IsThumbnail() ) {
		printf("d=%f h=%f\n", d, h);
		printf("offset=%f,%f,%f\n", pt.x, pt.y, pt.z);
	}
#endif
}

void SetLatheView_fromComment(void)
{
#ifdef _DEBUG
	printf("SetLatheView_fromComment()\n");
#endif
	// 旋盤ﾓｰﾄﾞのﾌﾗｸﾞON
	g_pDoc->SetLatheViewMode();
	// 呼び出す関数の指定
	IsGcode = &IsGcodeObject_Lathe;
	CheckGcodeOther = &CheckGcodeOther_Lathe;
	// ﾃﾞﾌｫﾙﾄ平面：XZ_PLANE
	g_ncArgv.nc.enPlane = XZ_PLANE;

	// ﾃﾞｰﾀの取り出し
	for ( int i=0; i<g_nLatheView && i<SIZEOF(g_dLatheView); i++ ) {
		switch ( i ) {
		case 0:		// ﾜｰｸ径
			g_pDoc->SetWorkLatheR( g_dLatheView[0] / 2.0f );	// 半径で保管
#ifdef _DEBUG
			if ( !IsThumbnail() )
				printf("r=%f\n", g_dLatheView[0]/2.0f);
#endif
			break;
		case 2:		// z1, z2
			// z2 があるときだけ
			g_pDoc->SetWorkLatheZ( g_dLatheView[1], g_dLatheView[2] );
#ifdef _DEBUG
			if ( !IsThumbnail() )
				printf("(%f)-(%f)\n", g_dLatheView[1], g_dLatheView[2]);
#endif
			break;
		}
	}

	// 機械情報のﾋﾞｭｰﾓｰﾄﾞ初期値ｸﾘｱ
	if ( AfxGetNCVCApp()->GetMCOption()->GetInt(MC_INT_FORCEVIEWMODE) == MC_VIEWMODE_WIRE ) {
		g_pDoc->SetDocFlag(NCDOC_WIRE, FALSE);
		// InitialVariableでの座標初期値は...
	}
}

void SetWireView_fromComment(void)
{
#ifdef _DEBUG
	printf("SetWireView_fromComment()\n");
#endif
	// ﾜｲﾔ加工ﾓｰﾄﾞのﾌﾗｸﾞON
	g_pDoc->SetDocFlag(NCDOC_WIRE);
	// 呼び出す関数の指定
	IsGcode = &IsGcodeObject_Wire;
	CheckGcodeOther = &CheckGcodeOther_Wire;

	// ﾜｰｸ厚さ指示
	CRect3F		rc;
	rc.high = g_dWireView;
	g_pDoc->SetWorkRectComment(rc, FALSE);	// 描画領域を更新しない(CNCDoc::SerializeAfterCheck)
#ifdef _DEBUG
	if ( !IsThumbnail() )
		printf("t=%f\n", rc.high);
#endif

	// 機械情報のﾋﾞｭｰﾓｰﾄﾞ初期値ｸﾘｱ
	if ( AfxGetNCVCApp()->GetMCOption()->GetInt(MC_INT_FORCEVIEWMODE) == MC_VIEWMODE_LATHE ) {
		g_pDoc->SetDocFlag(NCDOC_LATHE, FALSE);
	}
}

CString SearchFolder(const regex& r)
{
	extern	LPCTSTR	gg_szWild;	// "*.";
	CString	strResult, strExt;
	LPVOID	pFunc;

	for ( int i=0; i<SIZEOF(g_strSearchFolder); i++ ) {
		if ( g_strSearchFolder[i].IsEmpty() )
			continue;
		// ﾌｫﾙﾀﾞを標準拡張子で検索
		strResult = SearchFolder_Sub(i,
			gg_szWild + AfxGetNCVCApp()->GetDocExtString(TYPE_NCD).Right(3),	// '.' 除く「ncd」
			r);
		if ( !strResult.IsEmpty() )
			return strResult;
		// 登録拡張子でのﾌｫﾙﾀﾞ検索
		for ( int j=0; j<2/*EXT_ADN,EXT_DLG*/; j++ ) {
			PMAP_FOREACH(strExt, pFunc, AfxGetNCVCApp()->GetDocTemplate(TYPE_NCD)->GetExtMap((eEXTTYPE)j));
				strResult = SearchFolder_Sub(i, gg_szWild + strExt, r);
				if ( !strResult.IsEmpty() )
					return strResult;
			END_FOREACH
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
	// ﾌｧｲﾙﾏｯﾋﾟﾝｸﾞしてﾌﾟﾛｸﾞﾗﾑ番号(文字列)の存在確認
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
	// 念のためにﾁｪｯｸ
//	if ( g_ncArgv.nc.nGcode<81 || g_ncArgv.nc.nGcode>89 ) {
//		g_Cycle.clear();
//		return;
//	}

	// 固定ｻｲｸﾙの補間
	int	z = _GetPlaneZ();
	if ( g_ncArgv.nc.dwValFlags & g_dwSetValFlags[z] )
		g_Cycle.dValZ = g_ncArgv.nc.dValue[z];
	else if ( g_Cycle.dValZ ) {
		g_ncArgv.nc.dValue[z] = g_Cycle.dValZ.get();
		g_ncArgv.nc.dwValFlags |= g_dwSetValFlags[z];
	}
	if ( g_ncArgv.nc.dwValFlags & NCD_R )
		g_Cycle.dValR = g_ncArgv.nc.dValue[NCA_R];
	else if ( g_Cycle.dValR ) {
		g_ncArgv.nc.dValue[NCA_R] = g_Cycle.dValR.get();
		g_ncArgv.nc.dwValFlags |= NCD_R;
	}
	if ( g_ncArgv.nc.dwValFlags & NCD_P )
		g_Cycle.dValP = g_ncArgv.nc.dValue[NCA_P];
	else if ( g_Cycle.dValP ) {
		g_ncArgv.nc.dValue[NCA_P] = g_Cycle.dValP.get();
		g_ncArgv.nc.dwValFlags |= NCD_P;
	}
}

void G68RoundCheck(CNCblock* pBlock)
{
	if ( g_pDoc->IsDocFlag(NCDOC_LATHE) ) {
		// 旋盤ﾓｰﾄﾞではｻﾎﾟｰﾄされない
		pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_NOTLATHE);
		return;
	}

	// 各指示座標のﾁｪｯｸ
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
			g_ncArgv.g68.dRound = RAD((float)g_ncArgv.nc.dValue[NCA_R]);	// ﾗｼﾞｱﾝで格納
		else {
			pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_ORDER);
			g_ncArgv.g68.bG68 = FALSE;		// G68ｷｬﾝｾﾙ
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

// 変数初期化
void InitialVariable(void)
{
	extern	LPCTSTR	gg_szEn;	// "\\";
	int		i;
	const CMCOption*	pMCopt = AfxGetNCVCApp()->GetMCOption();
	const CViewOption*	pVopt  = AfxGetNCVCApp()->GetViewOption();

	ZeroMemory(&g_ncArgv, sizeof(NCARGV));

	FeedAnalyze = pMCopt->GetInt(MC_INT_FDOT)==0 ? &FeedAnalyze_Int : &FeedAnalyze_Dot;
	g_ncArgv.nc.nGtype = G_TYPE;
	g_ncArgv.nc.nGcode = pMCopt->GetModalSetting(MODALGROUP0);
	switch ( pMCopt->GetModalSetting(MODALGROUP1) ) {
	case 1:
		g_ncArgv.nc.enPlane = XZ_PLANE;
		break;
	case 2:
		g_ncArgv.nc.enPlane = YZ_PLANE;
		break;
	default:	// or 「0」
		g_ncArgv.nc.enPlane = XY_PLANE;
		break;
	}
	switch ( pMCopt->GetInt(MC_INT_FORCEVIEWMODE) ) {
	case MC_VIEWMODE_LATHE:		// 旋盤
		g_nLatheView = 0;
		SetLatheView_fromComment();	// 旋盤ﾓｰﾄﾞへ強制切替
		g_ncArgv.nc.dValue[NCA_X] = pMCopt->GetInitialXYZ(NCA_Z);
		g_ncArgv.nc.dValue[NCA_Y] = 0.0;
		g_ncArgv.nc.dValue[NCA_Z] = pMCopt->GetInitialXYZ(NCA_X) / 2.0;
		i = 3;	// XYZ初期化ｲﾝﾃﾞｯｸｽを進める
		break;
	case MC_VIEWMODE_WIRE:		// ﾜｲﾔ加工機
		g_dWireView = pMCopt->GetDbl(MC_DBL_DEFWIREDEPTH);
		SetWireView_fromComment();	// ﾜｲﾔ加工機ﾓｰﾄﾞへ強制切替
		for ( i=0; i<NCXYZ; i++ )
			g_ncArgv.nc.dValue[i] = pMCopt->GetInitialXYZ(i);
		break;
	default:
		IsGcode = &IsGcodeObject_Milling;
		CheckGcodeOther = &CheckGcodeOther_Milling;
		for ( i=0; i<NCXYZ; i++ )
			g_ncArgv.nc.dValue[i] = pMCopt->GetInitialXYZ(i);
	}
	for ( ; i<VALUESIZE; i++ )
		g_ncArgv.nc.dValue[i] = 0.0;
	g_ncArgv.bAbs		= pMCopt->GetModalSetting(MODALGROUP3) == 0 ? TRUE : FALSE;
	g_ncArgv.bG98		= pMCopt->GetModalSetting(MODALGROUP4) == 0 ? TRUE : FALSE;
	g_ncArgv.nSpindle	= 0;
	g_ncArgv.dFeed		= pMCopt->GetDbl(MC_DBL_FEED);
	g_ncArgv.dEndmill	= pVopt->GetDefaultEndmill();
	g_ncArgv.nEndmillType	= pVopt->GetDefaultEndmillType();

	g_Cycle.clear();
	TaperClear();
	G68RoundClear();

	g_nSubprog = 0;
	g_strComma.clear();
	g_dLatheView[0] = 0;	// 内径大きさ判定のため初期化

	if ( IsThumbnail() ) {
		SearchMacro = &NC_NoSearch;
		SearchAutoBreak = &NC_NoSearch;
	}
	else {
		if ( pMCopt->GetMacroStr(MCMACROCODE).IsEmpty() || pMCopt->GetMacroStr(MCMACROIF).IsEmpty() )
			SearchMacro = &NC_NoSearch;
		else {
			g_reMacroStr = pMCopt->GetMacroStr(MCMACROCODE);
			SearchMacro = &NC_SearchMacroProgram;
		}
		if ( pMCopt->GetAutoBreakStr().IsEmpty() )
			SearchAutoBreak = &NC_NoSearch;
		else {
			g_reAutoBreak = pMCopt->GetAutoBreakStr();
			SearchAutoBreak = &NC_SearchAutoBreak;
		}
	}

	// ｶﾚﾝﾄと指定ﾌｫﾙﾀﾞの初期化
	CString	strFile;	// dummy
	::Path_Name_From_FullPath(g_pDoc->GetCurrentFileName(), g_strSearchFolder[0], strFile);
	g_strSearchFolder[1] = pMCopt->GetMacroStr(MCMACROFOLDER);
	for ( i=0; i<SIZEOF(g_strSearchFolder); i++ ) {
		if ( !g_strSearchFolder[i].IsEmpty() && g_strSearchFolder[i].Right(1) != _T(gg_szEn) )
			g_strSearchFolder[i] += _T(gg_szEn);
	}
	if ( g_strSearchFolder[0].CompareNoCase(g_strSearchFolder[1]) == 0 )
		g_strSearchFolder[1].Empty();
}
