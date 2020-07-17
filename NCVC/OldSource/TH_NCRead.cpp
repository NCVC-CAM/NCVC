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
#include "boost/spirit/include/qi.hpp"
#include "boost/spirit/include/phoenix.hpp"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
//#define	_DEBUG_GSPIRIT
#endif

using namespace std;
using namespace boost;
using namespace boost::spirit;

extern	const	DWORD	g_dwSetValFlags[];
extern	LPCTSTR			g_szNdelimiter; // "XYZUVWIJKRPLDH"

//	ｻﾑﾈｲﾙ表示でのﾏﾙﾁｽﾚｯﾄﾞ対応
__declspec(thread)	static	CThreadDlg*	g_pParent;
__declspec(thread)	static	CNCDoc*		g_pDoc;
__declspec(thread)	static	NCARGV		g_ncArgv;			// NCVCdefine.h
__declspec(thread)	static	DWORD		g_dwToolPosFlags;	// ToolPos用
__declspec(thread)	static	double		g_dToolPos[NCXYZ];
__declspec(thread)	static	int			g_nSubprog;		// ｻﾌﾞﾌﾟﾛｸﾞﾗﾑ呼び出しの階層
__declspec(thread)	static	LPTSTR		g_lpstrComma;	// 次のﾌﾞﾛｯｸとの計算

// 固定ｻｲｸﾙのﾓｰﾀﾞﾙ補間値
struct	CYCLE_INTERPOLATE
{
	BOOL	bCycle;		// 固定ｻｲｸﾙ処理中
	double	dVal[3];	// Z, R, P
	void	clear(void) {
		bCycle = FALSE;
		dVal[0] = dVal[1] = dVal[2] = HUGE_VAL;
	}
};
__declspec(thread)	static	CYCLE_INTERPOLATE	g_Cycle;
static	void	CycleInterpolate(void);

// IsGcodeObject() 戻り値
static	enum	ENGCODEOBJ {NOOBJ, MAKEOBJ, MAKEOBJ_NOTMODAL};

// G68座標回転
static	void	G68RoundCheck(CNCblock*);
static	void	G68RoundClear(void);
// ﾃｰﾊﾟ加工情報
static	void	TaperClear(void);
//
static	void	InitialVariable(void);
// 解析関数
static	int			NC_GSeparater(int, CNCdata*&);
static	CNCdata*	AddGcode(CNCblock*, CNCdata*, int);
static	CNCdata*	AddM98code(CNCblock*, CNCdata*, int);
static	int			CallSubProgram(CNCblock*, CNCdata*&);
typedef	ENGCODEOBJ	(*PFNCHECKGCODE)(int);
static	ENGCODEOBJ	IsGcodeObject_Milling(int);
static	ENGCODEOBJ	IsGcodeObject_Wire(int);
static	ENGCODEOBJ	IsGcodeObject_Lathe(int);
__declspec(thread)	static	PFNCHECKGCODE	g_pfnIsGcode;
typedef	int			(*PFNCHECKGCODEOTHER)(int);
static	int			CheckGcodeOther_Milling(int);
static	int			CheckGcodeOther_Wire(int);
static	int			CheckGcodeOther_Lathe(int);
__declspec(thread)	static	PFNCHECKGCODEOTHER	g_pfnCheckGcodeOther;
// 面取り・ｺｰﾅｰR処理
static	void	MakeChamferingObject(CNCblock*, CNCdata*, CNCdata*);
// Fﾊﾟﾗﾒｰﾀ, ﾄﾞｳｪﾙ時間の解釈
typedef double (*PFNFEEDANALYZE)(const string&);
static	double	FeedAnalyze_Dot(const string&);
static	double	FeedAnalyze_Int(const string&);
__declspec(thread)	static	PFNFEEDANALYZE	g_pfnFeedAnalyze;
// 工具径解析
static	void	SetEndmillDiameter(const string&);
// ﾃｰﾊﾟ角度
static	int		SetTaperAngle(const string&);
// 工具位置情報
static	CNCdata*	SetToolPosition_fromComment(CNCblock*, CNCdata*);
// ﾜｰｸ矩形情報設定
static	void	SetWorkRect_fromComment(const vector<double>&);
static	void	SetLatheRect_fromComment(const vector<double>&);
static	void	SetWireRect_fromComment(const vector<double>&);
// ｻﾌﾞﾌﾟﾛ，ﾏｸﾛの検索
static	CString	g_strSearchFolder[2];	// ｶﾚﾝﾄと指定ﾌｫﾙﾀﾞ
static	CString	SearchFolder(const regex&);
static	CString	SearchFolder_Sub(int, LPCTSTR, const regex&);
static	BOOL	SearchProgNo(LPCTSTR, const regex&);
static	regex	g_reMacroStr;		// __declspec(thread)は不要
static	int		NC_SearchSubProgram(int*);
typedef	int		(*PFNBLOCKPROCESS)(const string&, CNCblock*);
static	int		NC_SearchMacroProgram(const string&, CNCblock*);
static	int		NC_NoSearch(const string&, CNCblock*);
__declspec(thread)	static	PFNBLOCKPROCESS	g_pfnSearchMacro;
// 自動ﾌﾞﾚｲｸｺｰﾄﾞ検索
static	regex	g_reAutoBreak;		// __declspec(thread)は不要
static	int		NC_SearchAutoBreak(const string&, CNCblock*);
__declspec(thread)	static	PFNBLOCKPROCESS	g_pfnSearchAutoBreak;

//////////////////////////////////////////////////////////////////////

// 共通
inline BOOL _IsThread(void)
{
	// g_pParent==NULL(ｻﾑﾈｲﾙ表示)は TRUE を返す
	return g_pParent ? g_pParent->IsThreadContinue() : TRUE;
}

// 数値変換( 1/1000 ﾃﾞｰﾀを判断する)
inline	double	_GetNCValue(const string& str)
{
	double	dResult = atof(str.c_str());
	if ( str.find('.') == string::npos )
		dResult /= 1000.0;		// 小数点がなければ 1/1000
	return dResult;
}
// g_lpstrComma にﾃﾞｰﾀを代入
inline	void	_SetStrComma(const string& strComma)
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
//	NCｺｰﾄﾞのｵﾌﾞｼﾞｪｸﾄ生成ｽﾚｯﾄﾞ
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
	CNCdata*	pData;	// １つ前の生成ｵﾌﾞｼﾞｪｸﾄ

	// 変数初期化
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
		// １つ前のｵﾌﾞｼﾞｪｸﾄ参照でNULL参照しないため
		pDataFirst = pData = new CNCdata(&g_ncArgv);
		// 1行(1ﾌﾞﾛｯｸ)解析しｵﾌﾞｼﾞｪｸﾄの登録
		for ( i=0; i<nLoopCnt && _IsThread(); i++ ) {
			if ( NC_GSeparater(i, pData) != 0 )
				break;
			if ( (i & 0x003f)==0 && g_pParent )	// 64回おき(下位6ﾋﾞｯﾄﾏｽｸ)
				g_pParent->m_ctReadProgress.SetPos(i);		// ﾌﾟﾛｸﾞﾚｽﾊﾞｰ
		}
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		nResult = IDCANCEL;
	}

	if ( g_lpstrComma )
		delete	g_lpstrComma;
	// 初回登録用ﾀﾞﾐｰﾃﾞｰﾀの消去
	if ( pDataFirst )
		delete	pDataFirst;

	if ( g_pParent ) {
		g_pParent->m_ctReadProgress.SetPos(nLoopCnt);
		g_pParent->PostMessage(WM_USERFINISH, _IsThread() ? nResult : IDCANCEL);
	}

	return 0;	// AfxEndThread(0);
}

//////////////////////////////////////////////////////////////////////
//	Gｺｰﾄﾞの構文解析

//	NCﾌﾟﾛｸﾞﾗﾑ解析
template<typename Iterator>
struct CGcodeParser : qi::grammar<Iterator, std::string(), qi::space_type>
{
	qi::rule<Iterator, std::string(), qi::space_type>		rr;

	CGcodeParser() : CGcodeParser::base_type(rr) {
		using qi::char_;
		using qi::double_;
		rr = qi::raw[ (qi::upper >> double_) |
				(char_(',') >> (char_('R')|'C') >> double_ ) ];
	}
};

//	ｺﾒﾝﾄ文字列解析
template<typename Iterator>
struct CCommentParser : qi::grammar<Iterator, qi::space_type>
{
	vector<double>&	vRect;
	vector<double>&	vLathe;
	vector<double>& vWire;

	qi::rule<Iterator, qi::space_type>		rr,
		rs1, rs2, rs3, rs4, rs5, rr1, r11, r12, rr2, rr3, rr4, rr5;

	CCommentParser(vector<double>& v1, vector<double>& v2, vector<double>& v3) :
			vRect(v1), vLathe(v2), vWire(v3), CCommentParser::base_type(rr) {

		using qi::no_case;
		using qi::char_;
		using qi::double_;
		using qi::_1;
		using phoenix::push_back;
		using phoenix::ref;

		// Endmill
		rs1 = no_case["endmill"] >> '=';
		r11 = double_[SetEndmillComment()] >> -no_case["mm"] >>
				-(',' >> qi::digit[SetEndmillType()]);
		r12 = (char_('R')|'r') >> double_[SetBallEndmillComment()];	// ﾎﾞｰﾙｴﾝﾄﾞﾐﾙ表記
		rr1 = r11 | r12;
		// WorkRect
		rs2 = no_case["workrect"] >> '=';
		rr2 = double_[push_back(ref(vRect), _1)] % ',';
		// ViewMode
		rs3 = no_case["latheview"] >> '=';
		rr3 = double_[push_back(ref(vLathe), _1)] % ',';
		rs4 = no_case["wireview"] >> '=';
		rr4 = double_[push_back(ref(vWire), _1)] % ',';
		// ToolPos
		rs5 = no_case["toolpos"] >> '=';
		rr5 = -double_[ToolPosX()] >>
				-(',' >> -double_[ToolPosY()] >> -(',' >> -double_[ToolPosZ()]));
		//
		rr  = char_('(') >> *(char_ - (rs1|rs2|rs3|rs4|rs5)) >>
					// コメント行を全て処理させるために
					// ↓は＊としている
					*( rs1>>rr1 | rs2>>rr2 | rs3>>rr3 | rs4>>rr4 | rs5>>rr5 );
	}

	// ｴﾝﾄﾞﾐﾙ径
	struct	SetEndmillComment {
		void operator()(const double& d, qi::unused_type, qi::unused_type) const {
			g_ncArgv.dEndmill = d / 2.0;
#ifdef _DEBUG_GSPIRIT
			if ( !g_pDoc->IsNCDocFlag(NCDOC_THUMBNAIL) ) {
				CMagaDbg	dbg("SetEndmillComment()", DBG_MAGENTA);
				dbg.printf("Endmill=%f", g_ncArgv.dEndmill);
			}
#endif
		}
	};
	// ﾎﾞｰﾙｴﾝﾄﾞﾐﾙ表記
	struct	SetBallEndmillComment {
		void operator()(double& d, qi::unused_type, qi::unused_type) const {
			g_ncArgv.dEndmill = d;
			g_ncArgv.nEndmillType = 1;
#ifdef _DEBUG_GSPIRIT
			if ( !g_pDoc->IsNCDocFlag(NCDOC_THUMBNAIL) ) {
				CMagaDbg	dbg("SetBallEndmillComment()", DBG_MAGENTA);
				dbg.printf("BallEndmill=R%f", g_ncArgv.dEndmill);
			}
#endif
		}
	};
	// ｴﾝﾄﾞﾐﾙﾀｲﾌﾟ
	struct SetEndmillType {
		void operator()(char& c, qi::unused_type, qi::unused_type) const {
			g_ncArgv.nEndmillType = c - '0';	// １文字を数値に変換
#ifdef _DEBUG_GSPIRIT
			if ( !g_pDoc->IsNCDocFlag(NCDOC_THUMBNAIL) ) {
				CMagaDbg	dbg("SetEndmillType()", DBG_MAGENTA);
				dbg.printf("EndmillType=%d", g_ncArgv.nEndmillType);
			}
#endif
		}
	};
	// 工具位置変更
	struct ToolPosX {
		void operator()(double& d, qi::unused_type, qi::unused_type) const {
			g_dToolPos[NCA_X] = d;
			g_dwToolPosFlags |= NCD_X;
#ifdef _DEBUG_GSPIRIT
			if ( !g_pDoc->IsNCDocFlag(NCDOC_THUMBNAIL) ) {
				CMagaDbg	dbg("ToolPosX()", DBG_MAGENTA);
				dbg.printf("x=%f", d);
			}
#endif
		}
	};
	struct ToolPosY {
		void operator()(double& d, qi::unused_type, qi::unused_type) const {
			g_dToolPos[NCA_Y] = d;
			g_dwToolPosFlags |= NCD_Y;
#ifdef _DEBUG_GSPIRIT
			if ( !g_pDoc->IsNCDocFlag(NCDOC_THUMBNAIL) ) {
				CMagaDbg	dbg("ToolPosY()", DBG_MAGENTA);
				dbg.printf("y=%f", d);
			}
#endif
		}
	};
	struct ToolPosZ {
		void operator()(double& d, qi::unused_type, qi::unused_type) const {
			g_dToolPos[NCA_Z] = d;
			g_dwToolPosFlags |= NCD_Z;
#ifdef _DEBUG_GSPIRIT
			if ( !g_pDoc->IsNCDocFlag(NCDOC_THUMBNAIL) ) {
				CMagaDbg	dbg("ToolPosZ()", DBG_MAGENTA);
				dbg.printf("z=%f", d);
			}
#endif
		}
	};
};

static	vector<double>	vRect, vLathe, vWire;
static	CGcodeParser<string::iterator>		gr;
static	CCommentParser<string::iterator>	comment_p(vRect, vLathe, vWire);

//////////////////////////////////////////////////////////////////////
// Gｺｰﾄﾞの分割(再帰関数)
int NC_GSeparater(int nLine, CNCdata*& pDataResult)
{
	string		strBlock,		// NCﾌﾞﾛｯｸ（1行解析単位）
				strWord,		// 解析単位
				strComma;		// ｶﾝﾏ以降の文字列(次のﾌﾞﾛｯｸへ継ぐ)
	string::iterator	it;

	// ｸﾞﾙｰﾊﾞﾙ変数に移すことで
	// 再入のたびに変数がｲﾝｽﾀﾝｽ化されることを防ぐ
	// 結果、大幅ｽﾋﾟｰﾄﾞUP -> ｻﾑﾈｲﾙ表示でﾋﾞｭｰﾓｰﾄﾞが判定できない
//	vector<double>	vRect, vLathe, vWire;
//	CGcodeParser<string::iterator>		gr;
//	CCommentParser<string::iterator>	comment_p(vRect, vLathe, vWire);

	int			i, nCode,
				nNotModalCode = -1,	// ﾓｰﾀﾞﾙ不要のGｺｰﾄﾞ(ﾌﾞﾛｯｸ内でのみ有効)
				nResult, nIndex;
	BOOL		bNCobj = FALSE, bNCval = FALSE, bNCsub = FALSE,
				bTcode = FALSE;
	ENGCODEOBJ	enGcode;
	CNCdata*	pData;
	CNCblock*	pBlock = g_pDoc->GetNCblock(nLine);

	// 変数初期化
	g_ncArgv.nc.nLine		= nLine;
	g_ncArgv.nc.nErrorCode	= 0;
	g_ncArgv.nc.dwValFlags &= NCD_CLEARVALUE;	// 0xffff0000
	g_ncArgv.taper.bTonly	= FALSE;
	g_dwToolPosFlags		= 0;

	vRect.clear();
	vLathe.clear();
	vWire.clear();
	strBlock = pBlock->GetStrGcode();

#ifdef _DEBUG_GSPIRIT
	CMagaDbg	dbg("NC_Gseparate()", DBG_BLUE);
	CMagaDbg	dbg1(DBG_GREEN);
	if ( !g_pDoc->IsNCDocFlag(NCDOC_THUMBNAIL) )
		dbg.printf("No.%004d Line=%s", nLine+1, strBlock.c_str());
#endif

	// ｻﾑﾈｲﾙ表示のときは処理しないﾙｰﾁﾝ
	if ( !g_pDoc->IsNCDocFlag(NCDOC_THUMBNAIL) ) {
		// 自動ﾌﾞﾚｲｸｺｰﾄﾞ検索
		(*g_pfnSearchAutoBreak)(strBlock, pBlock);

		// ｺﾒﾝﾄ解析(解析ﾓｰﾄﾞ, ｴﾝﾄﾞﾐﾙ径, ﾜｰｸ矩形情報の取得)
		it = strBlock.begin();
		if ( qi::phrase_parse(it, strBlock.end(), comment_p, qi::space) ) {
			if ( !vRect.empty() )
				SetWorkRect_fromComment(vRect);
			if ( !vLathe.empty() )
				SetLatheRect_fromComment(vLathe);
			if ( !vWire.empty() )
				SetWireRect_fromComment(vWire);
			if ( g_dwToolPosFlags )
				pDataResult = SetToolPosition_fromComment(pBlock, pDataResult);	// create dummy object
#ifdef _DEBUG_GSPIRIT
			if ( !g_pDoc->IsNCDocFlag(NCDOC_THUMBNAIL) )
				dbg.printf("--- Comment OK");
#endif
			return 0;	// ｺﾒﾝﾄ行なら終了
		}
	}

	// ﾏｸﾛ置換解析
	if ( (nIndex=(*g_pfnSearchMacro)(strBlock, pBlock)) >= 0 ) {
		g_nSubprog++;
		// M98ｵﾌﾞｼﾞｪｸﾄとO番号(nIndex)の登録
		pDataResult = AddM98code(pBlock, pDataResult, nIndex);
		// g_pfnSearchMacro でﾌﾞﾛｯｸが追加される可能性アリ
		// ここでは nLoop 変数を使わず、ﾈｲﾃｨﾌﾞのﾌﾞﾛｯｸｻｲｽﾞにて判定
		for ( i=nIndex; i<g_pDoc->GetNCBlockSize() && _IsThread(); i++ ) {
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
	while ( _IsThread() ) {
		strWord.clear();
		if ( !qi::phrase_parse(it, strBlock.end(), gr, qi::space, strWord) )
			break;
#ifdef _DEBUG_GSPIRIT
		if ( !g_pDoc->IsNCDocFlag(NCDOC_THUMBNAIL) ) {
			dbg1.printf("G Cut=%s", strWord.c_str());
		}
#endif
		switch ( strWord[0] ) {
		case 'M':
			// 前回のｺｰﾄﾞで登録ｵﾌﾞｼﾞｪｸﾄがあるなら
			if ( bNCobj || bNCval ) {
				// ｵﾌﾞｼﾞｪｸﾄ生成
				pData = AddGcode(pBlock, pDataResult, nNotModalCode);
				// 面取りｵﾌﾞｼﾞｪｸﾄの登録
				if ( g_lpstrComma )
					MakeChamferingObject(pBlock, pDataResult, pData);
				pDataResult = pData;
				_SetStrComma(strComma);
				nNotModalCode = -1;
				bNCobj = bNCval = FALSE;
			}
			// 前回のｺｰﾄﾞでｻﾌﾞﾌﾟﾛ呼び出しがあれば
			if ( bNCsub ) {
				bNCsub = FALSE;
				if ( CallSubProgram(pBlock, pDataResult) == 30 )
					return 30;	// 終了ｺｰﾄﾞ
			}
			nCode = atoi(strWord.substr(1).c_str());
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
				if ( g_nSubprog > 0 )
					g_nSubprog--;
				// 復帰用ｵﾌﾞｼﾞｪｸﾄ生成
				pDataResult = AddM98code(pBlock, pDataResult, -1);
				return 99;
			default:
				return 0;	// 認識できないMｺｰﾄﾞﾌﾞﾛｯｸは以降のｺｰﾄﾞを処理しない
			}
			break;
		case 'G':
			// 先に現在のｺｰﾄﾞ種別をﾁｪｯｸ
			nCode = atoi(strWord.substr(1).c_str());
			enGcode = (*g_pfnIsGcode)(nCode);	// IsGcodeObject_～
			if ( enGcode == NOOBJ ) {
				nResult = (*g_pfnCheckGcodeOther)(nCode);
				if ( nResult > 0 )
					pBlock->SetNCBlkErrorCode(nResult);
				break;
			}
			// 前回のｺｰﾄﾞで登録ｵﾌﾞｼﾞｪｸﾄがあるなら
			if ( bNCobj || bNCval ) {
				pData = AddGcode(pBlock, pDataResult, nNotModalCode);
				if ( g_lpstrComma )
					MakeChamferingObject(pBlock, pDataResult, pData);
				pDataResult = pData;
				_SetStrComma(strComma);
				nNotModalCode = -1;
				bNCval = FALSE;		// bNCobj ｸﾘｱ不要
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
			break;
		case 'F':
			g_ncArgv.dFeed = (*g_pfnFeedAnalyze)(strWord.substr(1));
			break;
		case 'S':
			g_ncArgv.nSpindle = abs(atoi(strWord.substr(1).c_str()));
			break;
		case 'T':
			if ( g_pDoc->IsNCDocFlag(NCDOC_WIRE) ) {
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
			if ( g_pDoc->IsNCDocFlag(NCDOC_WIRE) ) {
				// ﾜｲﾔﾓｰﾄﾞでｻﾎﾟｰﾄすべきか...
				string	strTmp = ::Trim(strWord.substr(1));
				if ( strTmp[0] == 'R' ) {
					g_ncArgv.nc.dValue[NCA_R] = _GetNCValue(strTmp.substr(1));
					g_ncArgv.nc.dwValFlags |= NCD_R;
				}
			}
			else {
				strComma = ::Trim(strWord.substr(1));	// ｶﾝﾏ以降を取得
#ifdef _DEBUG_GSPIRIT
				if ( !g_pDoc->IsNCDocFlag(NCDOC_THUMBNAIL) )
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
			// 値取得
			if ( g_Cycle.bCycle ) {		// 81～89
				// 固定ｻｲｸﾙの特別処理
				switch ( nCode ) {
				case NCA_K:		// Kはﾈｲﾃｨﾌﾞで
					g_ncArgv.nc.dValue[NCA_K] = atoi(strWord.substr(1).c_str());
					break;
				case NCA_P:		// P(ﾄﾞｳｪﾙ時間)は送り速度と同じ判定
					g_ncArgv.nc.dValue[NCA_P] = (*g_pfnFeedAnalyze)(strWord.substr(1));
					break;
				default:
					g_ncArgv.nc.dValue[nCode] = _GetNCValue(strWord.substr(1));
				}
			}
			else if ( g_pDoc->IsNCDocFlag(NCDOC_WIRE) ) {
				// ﾜｲﾔﾓｰﾄﾞにおける特別処理(L値)
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
		// Tｺｰﾄﾞ単独指示(ﾜｲﾔのみ)
		nNotModalCode = 01;		// dummy
		g_ncArgv.taper.bTonly = TRUE;	// TH_UVWire.cpp での処理目印
		bNCobj = TRUE;			// dummy object の生成
	}

	if ( bNCsub ) {
		// Mｺｰﾄﾞ後処理
		if ( CallSubProgram(pBlock, pDataResult) == 30 )
			return 30;	// 終了ｺｰﾄﾞ
	}
	else if ( bNCobj || bNCval ) {
		// NCﾃﾞｰﾀ登録処理
		pData = AddGcode(pBlock, pDataResult, nNotModalCode);
		if ( g_lpstrComma )
			MakeChamferingObject(pBlock, pDataResult, pData);
		pDataResult = pData;
		_SetStrComma(strComma);
		// ﾌﾞﾛｯｸ情報の更新
		pBlock->SetBlockToNCdata(pDataResult, g_pDoc->GetNCsize());
	}

	return 0;
}

CNCdata* AddGcode(CNCblock* pBlock, CNCdata* pDataBefore, int nNotModalCode)
{
	CNCdata*	pDataResult = pDataBefore;

	if ( !g_pDoc->IsNCDocFlag(NCDOC_WIRE) ) {
		// ﾜｲﾔ加工以外はUVW座標の加算
		for ( int i=0; i<NCXYZ; i++ ) {
			if ( g_ncArgv.nc.dwValFlags & g_dwSetValFlags[i+NCA_U] ) {
				g_ncArgv.nc.dValue[i]  += g_ncArgv.nc.dValue[i+NCA_U];
				g_ncArgv.nc.dwValFlags |= g_dwSetValFlags[i];
			}
		}
	}

	if ( g_pDoc->IsNCDocFlag(NCDOC_LATHE) ) {
		// 旋盤ﾓｰﾄﾞでの座標入れ替え(ﾄﾞｳｪﾙ除く)
		if ( g_ncArgv.nc.nGcode != 4 ) {
			boost::optional<double>	x, z, i, k;
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
				if ( g_pDoc->IsNCDocFlag(NCDOC_WIRE) && g_ncArgv.nc.dwValFlags&NCD_J ) {
					// ﾌﾟﾛｸﾞﾗﾑ面(XY軸)の設定
					g_ncArgv.nc.dValue[NCA_Z] = g_ncArgv.nc.dValue[NCA_J];
					g_ncArgv.nc.dwValFlags |= NCD_Z;
				}
				break;
			}
			// 固定ｻｲｸﾙのﾓｰﾀﾞﾙ補間
			if ( g_Cycle.bCycle )
				CycleInterpolate();
		}
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

	// 復元
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
		// M98ｵﾌﾞｼﾞｪｸﾄとO番号(nIndex)の登録
		pDataResult = AddM98code(pBlock, pDataResult, nIndex);
		// nRepeat分繰り返し
		while ( nRepeat-- > 0 && _IsThread() ) {
			// NC_SearchSubProgram でﾌﾞﾛｯｸが追加される可能性アリ
			// ここでは nLoop 変数を使わず、ﾈｲﾃｨﾌﾞのﾌﾞﾛｯｸｻｲｽﾞにて判定
			for ( i=nIndex; i<g_pDoc->GetNCBlockSize() && _IsThread(); i++ ) {
				nResult = NC_GSeparater(i, pDataResult);	// 再帰
				if ( nResult == 30 )
					return 30;
				else if ( nResult == 99 )
					break;
			}
		}
		// EOFで終了ならM99復帰扱い
		if ( i >= g_pDoc->GetNCBlockSize() && nResult == 0 ) {
			if ( g_nSubprog > 0 )
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
	case 54:
	case 55:
	case 56:
	case 57:
	case 58:
	case 59:
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
		break;
	case 91:
		g_ncArgv.bAbs = FALSE;
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
	case 51:
	case 52:
		g_ncArgv.taper.nTaper = nCode==51 ? 1 : -1;
		break;
	// ﾜｰｸ座標系
	case 54:
	case 55:
	case 56:
	case 57:
	case 58:
	case 59:
		g_pDoc->SelectWorkOffset(nCode - 54);
		break;
	// 上下独立ｺｰﾅｰ
	case 60:
	case 61:
	case 62:
	case 63:
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
	case 54:
	case 55:
	case 56:
	case 57:
	case 58:
	case 59:
		g_pDoc->SelectWorkOffset(nCode - 54);
		break;
	// ｱﾌﾞｿﾘｭｰﾄ, ｲﾝｸﾘﾒﾝﾄ
	case 90:
		g_ncArgv.bAbs = TRUE;
		break;
	case 91:
		g_ncArgv.bAbs = FALSE;
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
int NC_SearchSubProgram(int *pRepeat)
{
	int		nProg, n;
	CString	strProg;

	if ( g_ncArgv.nc.dwValFlags & NCD_L ) {
		*pRepeat = (int)g_ncArgv.nc.dValue[NCA_L];
		nProg    = (int)g_ncArgv.nc.dValue[NCA_P];
	}
	else {
		// L:繰り返し数が指定されていなければ，
		// [times][number] (n桁:4桁) を取得
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

	// 正規表現(Oxxxxにﾏｯﾁする)
	strProg.Format("(O(0)*%d)($|[^0-9])", nProg);
	regex	r(strProg);

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
	if ( g_pDoc->SerializeInsertBlock(strFile, n, NCF_AUTOREAD, FALSE) ) {
		// 挿入ﾌﾞﾛｯｸの最初だけNCF_FOLDER
		if ( n < g_pDoc->GetNCBlockSize() )	// 挿入前 < 挿入後
			g_pDoc->GetNCblock(n)->SetBlockFlag(NCF_FOLDER);
		return n;
	}

	return -1;
}

// ﾏｸﾛﾌﾟﾛｸﾞﾗﾑの検索
int NC_SearchMacroProgram(const string& strBlock, CNCblock* pBlock)
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
	int	n = g_pDoc->GetNCBlockSize();
	if ( g_pDoc->SerializeInsertBlock(strMacroFile, n, NCF_AUTOREAD, FALSE) ) {
		// 挿入ﾌﾞﾛｯｸの最初だけNCF_FOLDER
		if ( n < g_pDoc->GetNCBlockSize() )	// ﾌﾞﾛｯｸ挿入が失敗の可能性もある
			g_pDoc->GetNCblock(n)->SetBlockFlag(NCF_FOLDER);
		return n;
	}

	return -1;
}

// 自動ﾌﾞﾚｲｸｺｰﾄﾞの検索
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
	// ﾃﾞｰﾀﾁｪｯｸ
	if ( g_pDoc->IsNCDocFlag(NCDOC_LATHE) ) {
		// 旋盤ﾓｰﾄﾞではｻﾎﾟｰﾄされない
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
	boost::optional<CPointD>	ptResult;
	BOOL	bResult;

	// 計算開始
	if ( cr < NCMIN ) {
		r1 = r2 = 0.0;
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
	if ( !g_pDoc->IsNCDocFlag(NCDOC_THUMBNAIL) ) {
		dbg.printf("%c=%f, %f", cCham, r1, r2);
		dbg.printf("pts=(%f, %f)", pts.x, pts.y);
		dbg.printf("pte=(%f, %f)", pte.x, pte.y);
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
	memcpy(&(ncArgv.g68),   &(pData1->GetReadData()->m_g68),   sizeof(G68ROUND));
	memcpy(&(ncArgv.taper), &(pData1->GetReadData()->m_taper), sizeof(TAPER));
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
	// 既に登録された１つ前に面取りｵﾌﾞｼﾞｪｸﾄを挿入
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
	boost::optional<double> dResult = pMCopt->GetToolD( atoi(str.c_str()) );
	if ( dResult ) {
		g_ncArgv.dEndmill = *dResult;	// ｵﾌｾｯﾄは半径なので、そのまま使用
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
			g_ncArgv.taper.dTaper = RAD(dTaper);	// ﾗｼﾞｱﾝ保持
#ifdef _DEBUG
		if ( !g_pDoc->IsNCDocFlag(NCDOC_THUMBNAIL) ) {
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
	// 描画しないｵﾌﾞｼﾞｪｸﾄで生成
	g_ncArgv.nc.nGtype = O_TYPE;
	pData = AddGcode(pBlock, pDataBefore, -1);
	// 復元
	g_ncArgv.nc.nGtype = G_TYPE;

	return pData;
}

void SetWorkRect_fromComment(const vector<double>& vWork)
{
#ifdef _DEBUG
	CMagaDbg	dbg("SetWorkRect_fromComment()", DBG_MAGENTA);
#endif
	CRect3D		rc;
	CPoint3D	pt;

	// 不細工やけどvWorkの数が不定なので有効なﾃﾞｰﾀだけ取り出し
	for ( size_t i=0; i<vWork.size() && i<NCXYZ*2; i++ ) {
		switch ( i ) {
		case 0:		// 幅
			rc.right = vWork[0];
			break;
		case 1:		// 奥行
			rc.bottom = vWork[1];
			break;
		case 2:		// 高さ
			rc.high = vWork[2];
			break;
		case 3:		// Xｵﾌｾｯﾄ
			pt.x = vWork[3];
			break;
		case 4:		// Yｵﾌｾｯﾄ
			pt.y = vWork[4];
			break;
		case 5:		// Zｵﾌｾｯﾄ
			pt.z = vWork[5];
			break;
		}
	}
	rc.OffsetRect(pt);

	// ﾄﾞｷｭﾒﾝﾄが保持するﾜｰｸ矩形の更新
	g_pDoc->SetWorkRectOrg(rc);

#ifdef _DEBUG
	if ( !g_pDoc->IsNCDocFlag(NCDOC_THUMBNAIL) ) {
		dbg.printf("(%f,%f)-(%f,%f)", rc.left, rc.top, rc.right, rc.bottom);
		dbg.printf("(%f,%f)", rc.low, rc.high);
	}
#endif
}

void SetLatheRect_fromComment(const vector<double>& vLathe)
{
#ifdef _DEBUG
	CMagaDbg	dbg("SetLatheRect_fromComment()", DBG_MAGENTA);
#endif
	// 旋盤ﾓｰﾄﾞのﾌﾗｸﾞON
	g_pDoc->SetLatheViewMode();
	// 呼び出す関数の指定
	g_pfnIsGcode = &IsGcodeObject_Lathe;
	g_pfnCheckGcodeOther = &CheckGcodeOther_Lathe;
	// ﾃﾞﾌｫﾙﾄ平面：XZ_PLANE
	g_ncArgv.nc.enPlane = XZ_PLANE;

	// ﾃﾞｰﾀの取り出し
	for ( size_t i=0; i<vLathe.size() && i<3; i++ ) {
		switch ( i ) {
		case 0:		// ﾜｰｸ径
			g_pDoc->SetWorkLatheR( vLathe[0] / 2.0 );	// 半径で保管
#ifdef _DEBUG
			if ( !g_pDoc->IsNCDocFlag(NCDOC_THUMBNAIL) )
				dbg.printf("r=%f", vLathe[0]/2);
#endif
			break;
		case 2:		// z1, z2
			// z2 があるときだけ
			g_pDoc->SetWorkLatheZ( vLathe[1], vLathe[2] );
#ifdef _DEBUG
			if ( !g_pDoc->IsNCDocFlag(NCDOC_THUMBNAIL) )
				dbg.printf("(%f)-(%f)", vLathe[1], vLathe[2]);
#endif
			break;
		}
	}
}

void SetWireRect_fromComment(const vector<double>& vWire)
{
#ifdef _DEBUG
	CMagaDbg	dbg("SetWireRect_fromComment()", DBG_MAGENTA);
#endif
	// ﾜｲﾔ加工ﾓｰﾄﾞのﾌﾗｸﾞON
	g_pDoc->SetWireViewMode();
	// 呼び出す関数の指定
	g_pfnIsGcode = &IsGcodeObject_Wire;
	g_pfnCheckGcodeOther = &CheckGcodeOther_Wire;

	if ( vWire.size() == 1 ) {
		// ﾜｰｸ厚さ指示
		CRect3D		rc;
		rc.high = vWire[0];
		g_pDoc->SetWorkRectOrg(rc, FALSE);	// 描画領域を更新しない(CNCDoc::SerializeAfterCheck)
#ifdef _DEBUG
		if ( !g_pDoc->IsNCDocFlag(NCDOC_THUMBNAIL) )
			dbg.printf("t=%f", rc.high);
#endif
	}
	else {
		// ﾜｰｸ矩形処理
		SetWorkRect_fromComment(vWire);
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
			gg_szWild + AfxGetNCVCApp()->GetDocExtString(TYPE_NCD).Right(3),	// "." 除く「ncd」
			r);
		if ( !strResult.IsEmpty() )
			return strResult;
		// 登録拡張子でのﾌｫﾙﾀﾞ検索
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
	if ( g_ncArgv.nc.nGcode<81 || g_ncArgv.nc.nGcode>89 ) {
		g_Cycle.clear();
		return;
	}

	int	z;
	// 基準平面に対する直交軸
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

	// 固定ｻｲｸﾙの座標補間
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
			g_ncArgv.g68.dRound = RAD(g_ncArgv.nc.dValue[NCA_R]);	// ﾗｼﾞｱﾝで格納
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
	default:	// or 「0」
		g_ncArgv.nc.enPlane = XY_PLANE;
		break;
	}
//	g_ncArgv.nc.nErrorCode = 0;
//	g_ncArgv.nc.dwValFlags = 0;
	vector<double>	vDummy;
	switch ( pMCopt->GetInt(MC_INT_FORCEVIEWMODE) ) {
	case 1:		// 旋盤
		SetLatheRect_fromComment(vDummy);	// 旋盤ﾓｰﾄﾞへ強制切替
		g_ncArgv.nc.dValue[NCA_X] = pMCopt->GetInitialXYZ(NCA_Z);
		g_ncArgv.nc.dValue[NCA_Y] = 0.0;
		g_ncArgv.nc.dValue[NCA_Z] = pMCopt->GetInitialXYZ(NCA_X) / 2.0;
		i = 3;	// XYZ初期化ｲﾝﾃﾞｯｸｽを進める
		break;
	case 2:		// ﾜｲﾔ加工機
		vDummy.push_back( pMCopt->GetDbl(MC_DBL_DEFWIREDEPTH) );
		SetWireRect_fromComment(vDummy);
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

	// ｶﾚﾝﾄと指定ﾌｫﾙﾀﾞの初期化
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
