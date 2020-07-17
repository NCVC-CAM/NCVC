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

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

using namespace std;
using namespace boost;
using namespace boost::spirit::classic;

#define	IsThread()	g_pParent->IsThreadContinue()
//	ｻﾑﾈｲﾙ表示でのﾏﾙﾁｽﾚｯﾄﾞ対応
__declspec(thread)	static	CThreadDlg*	g_pParent;
__declspec(thread)	static	CNCDoc*		g_pDoc;
__declspec(thread)	static	NCARGV		g_ncArgv;		// NCVCdefine.h
__declspec(thread)	static	DWORD		g_dwValFlags;	// UVW座標用
__declspec(thread)	static	double		g_dValue[NCXYZ];
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
//
static	void		InitialVariable(void);
// 解析関数
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
// 面取り・ｺｰﾅｰR処理
static	void	MakeChamferingObject(CNCblock*, CNCdata*, CNCdata*);
// Fﾊﾟﾗﾒｰﾀ, ﾄﾞｳｪﾙ時間の解釈
typedef double (*PFNFEEDANALYZE)(const string&);
static	double	FeedAnalyze_Dot(const string&);
static	double	FeedAnalyze_Int(const string&);
__declspec(thread)	static	PFNFEEDANALYZE	g_pfnFeedAnalyze;
// 工具径解析
static	void	SetEndmillDiameter(const string&);
static	void	SetEndmillDiameter_fromComment(double);
static	void	SetEndmillType(char);
// ﾜｰｸ矩形情報設定
static	void	SetWorkRect_fromComment(vector<double>&);
static	void	SetLatheRect_fromComment(vector<double>&);
// ｻﾌﾞﾌﾟﾛ，ﾏｸﾛの検索
static	CString	g_strSearchFolder[2];	// ｶﾚﾝﾄと指定ﾌｫﾙﾀﾞ
static	CString	SearchFolder(regex&);
static	CString	SearchFolder_Sub(int, LPCTSTR, regex&);
static	BOOL	SearchProgNo(LPCTSTR, regex&);
static	regex	g_reMacroStr;		// __declspec(thread)は不要
static	int		NC_SearchSubProgram(int*);
typedef	int		(*PFNBLOCKPROCESS)(CNCblock*);
static	int		NC_SearchMacroProgram(CNCblock*);
static	int		NC_NoSearch(CNCblock*);
__declspec(thread)	static	PFNBLOCKPROCESS	g_pfnSearchMacro;
// 自動ﾌﾞﾚｲｸｺｰﾄﾞ検索
static	regex	g_reAutoBreak;		// __declspec(thread)は不要
static	int		NC_SearchAutoBreak(CNCblock*);
__declspec(thread)	static	PFNBLOCKPROCESS	g_pfnSearchAutoBreak;

//////////////////////////////////////////////////////////////////////

// 数値変換( 1/1000 ﾃﾞｰﾀを判断する)
inline	double		GetNCValue(const string& str)
{
	double	dResult = atof(str.c_str());
	if ( str.find('.') == string::npos )
		dResult /= 1000.0;		// 小数点がなければ 1/1000
	return dResult;
}
// g_lpstrComma にﾃﾞｰﾀを代入
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
//	NCｺｰﾄﾞのｵﾌﾞｼﾞｪｸﾄ生成ｽﾚｯﾄﾞ
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
		for ( i=0; i<nLoopCnt; i++ ) {
			if ( g_pParent && !IsThread() )
				break;
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
		g_pParent->PostMessage(WM_USERFINISH, IsThread() ? nResult : IDCANCEL);
	}

	return 0;	// AfxEndThread(0);
}

//////////////////////////////////////////////////////////////////////
//	Gｺｰﾄﾞの構文解析

//	NCﾌﾟﾛｸﾞﾗﾑ解析
struct CGcodeParser : grammar<CGcodeParser>
{
	vector<string>&	vResult;
	CGcodeParser(vector<string>& v) : vResult(v) {}

	template<typename T>
	struct definition {
		typedef	rule<T>	rule_t;
		rule_t	rr;
		definition( const CGcodeParser& self ) {
			// 未知ｺｰﾄﾞにも対応するため「全ての英大文字に続く数値」で解析
			rr = +( +( upper_p >> real_p )[push_back_a(self.vResult)]
					>> !( ch_p(',') >> ((ch_p('R')|'C') >> real_p) )[push_back_a(self.vResult)] )
				>> *(anychar_p - alnum_p)	// EOFなど
				>> end_p;	// 終端(これを入れないと parse()関数で full を返さない)
		}
		const rule_t& start() const {
			return rr;
		}
	};
};

//	ｽｷｯﾌﾟﾊﾟｰｻ
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

//	ｺﾒﾝﾄ文字列解析
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
			// Endmill ｷｰﾜｰﾄﾞ
			rs1 = as_lower_d[str_p("endmill")]  >> ch_p('=');
			rr1 = real_p[&SetEndmillDiameter_fromComment] >> !str_p("mm") >>
					!(ch_p(',') >> digit_p[&SetEndmillType]);	// 0:ｽｸｳｪｱ, 1:ﾎﾞｰﾙ
			// WorkRect ｷｰﾜｰﾄﾞ
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
// Gｺｰﾄﾞの分割(再帰関数)
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
				nNotModalCode = -1,	// ﾓｰﾀﾞﾙ不要のGｺｰﾄﾞ(ﾌﾞﾛｯｸ内でのみ有効)
				nResult, nIndex;
	BOOL		bNCobj = FALSE, bNCval = FALSE, bNCsub = FALSE;
	ENGCODEOBJ	enGcode;
	CNCdata*	pData;
	CNCblock*	pBlock = g_pDoc->GetNCblock(nLine);
	string		strComma;		// ｶﾝﾏ以降の文字列(次のﾌﾞﾛｯｸへ継ぐ)

	// 変数初期化
	g_ncArgv.nc.nLine		= nLine;
	g_ncArgv.nc.nErrorCode	= 0;
	g_ncArgv.nc.dwValFlags &= 0xFFFF0000;
#ifdef _DEBUG
	CMagaDbg	dbg("NC_Gseparate()", DBG_BLUE);
	CMagaDbg	dbg1(DBG_GREEN);
	if ( !g_pDoc->IsNCDocFlag(NCDOC_THUMBNAIL) )
		dbg.printf("No.%004d Line=%s", nLine+1, pBlock->GetStrGcode());
#endif

	if ( !g_pDoc->IsNCDocFlag(NCDOC_THUMBNAIL) ) {	// ｻﾑﾈｲﾙ表示のときは処理しない
		// 自動ﾌﾞﾚｲｸｺｰﾄﾞ検索
		(*g_pfnSearchAutoBreak)(pBlock);
	}

	// ｺﾒﾝﾄ解析(解析ﾓｰﾄﾞ, ｴﾝﾄﾞﾐﾙ径, ﾜｰｸ矩形情報の取得)
	parse((LPCTSTR)(pBlock->GetStrGcode()), comment_p, space_p);
	if ( !dRect.empty() )
		SetWorkRect_fromComment(dRect);
	if ( !dLathe.empty() )
		SetLatheRect_fromComment(dLathe);

	// ﾏｸﾛ置換解析
	if ( (nIndex=(*g_pfnSearchMacro)(pBlock)) >= 0 ) {
		g_nSubprog++;
		// M98ｵﾌﾞｼﾞｪｸﾄとO番号(nIndex)の登録
		pDataResult = AddM98code(pBlock, pDataResult, nIndex);
		// g_pfnSearchMacro でﾌﾞﾛｯｸが追加される可能性アリ
		// ここでは nLoop 変数を使わず、ﾈｲﾃｨﾌﾞのﾌﾞﾛｯｸｻｲｽﾞにて判定
		for ( i=nIndex; i<g_pDoc->GetNCBlockSize(); i++ ) {
			if ( g_pParent && !IsThread() )
				break;
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
			// 前回のｺｰﾄﾞで登録ｵﾌﾞｼﾞｪｸﾄがあるなら
			if ( bNCobj || bNCval ) {
				// ｵﾌﾞｼﾞｪｸﾄ生成
				pData = AddGcode(pBlock, pDataResult, nNotModalCode);
				// 面取りｵﾌﾞｼﾞｪｸﾄの登録
				if ( g_lpstrComma )
					MakeChamferingObject(pBlock, pDataResult, pData);
				pDataResult = pData;
				SetStrComma(strComma);
				nNotModalCode = -1;
				bNCobj = bNCval = FALSE;
			}
			// 前回のｺｰﾄﾞでｻﾌﾞﾌﾟﾛ呼び出しがあれば
			if ( bNCsub ) {
				bNCsub = FALSE;
				if ( CallSubProgram(pBlock, pDataResult) == 30 )
					return 30;	// 終了ｺｰﾄﾞ
			}
			nCode = atoi(it->substr(1).c_str());
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
			}
			break;
		case 'G':
			// 先に現在のｺｰﾄﾞ種別をﾁｪｯｸ
			nCode = atoi(it->substr(1).c_str());
			enGcode = (*g_pfnIsGcode)(nCode);	// IsGcodeObject_～
			if ( enGcode == NOOBJ ) {
				(*g_pfnCheckGcodeOther)(nCode);
				break;
			}
			// 前回のｺｰﾄﾞで登録ｵﾌﾞｼﾞｪｸﾄがあるなら
			if ( bNCobj || bNCval ) {
				pData = AddGcode(pBlock, pDataResult, nNotModalCode);
				if ( g_lpstrComma )
					MakeChamferingObject(pBlock, pDataResult, pData);
				pDataResult = pData;
				SetStrComma(strComma);
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
			g_ncArgv.dFeed = (*g_pfnFeedAnalyze)(it->substr(1));
			break;
		case 'S':
			g_ncArgv.nSpindle = abs(atoi(it->substr(1).c_str()));
			break;
		case 'T':
			SetEndmillDiameter(it->substr(1));
			break;
		case ',':
			strComma = ::Trim(it->substr(1));	// ｶﾝﾏ以降を取得
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
			// 値取得
			if ( g_Cycle.bCycle ) {		// 81～89
				// 固定ｻｲｸﾙの特別処理
				if ( nCode == NCA_K )		// Kはﾈｲﾃｨﾌﾞで
					g_ncArgv.nc.dValue[NCA_K] = atoi(it->substr(1).c_str());
				else if ( nCode == NCA_P )	// P(ﾄﾞｳｪﾙ時間)は送り速度と同じ判定
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
			g_dwValFlags |= g_dwSetValFlags[nCode];	// XYZ扱いでﾌﾗｸﾞON
			if ( g_ncArgv.bAbs )
				g_dValue[nCode]  = GetNCValue(it->substr(1));
			else
				g_dValue[nCode] += GetNCValue(it->substr(1));
			bNCval = TRUE;
			break;
		}
	} // End of for() iterator

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
		SetStrComma(strComma);
		// ﾌﾞﾛｯｸ情報の更新
		pBlock->SetBlockToNCdata(pDataResult, g_pDoc->GetNCsize());
	}

	return 0;
}

CNCdata* AddGcode(CNCblock* pBlock, CNCdata* pDataBefore, int nNotModalCode)
{
	extern	const	DWORD	g_dwSetValFlags[];
	CNCdata*	pDataResult = pDataBefore;

	// UVW座標の加算
	g_dwValFlags |= g_ncArgv.nc.dwValFlags;
	for ( int i=0; i<NCXYZ; i++ ) {
		if ( g_dwValFlags & g_dwSetValFlags[i] ) {
			g_ncArgv.nc.dValue[i]  += g_dValue[i];
			g_ncArgv.nc.dwValFlags |= g_dwSetValFlags[i];
		}
	}
	g_dwValFlags = 0;

	if ( g_pDoc->IsNCDocFlag(NCDOC_LATHE) ) {
		// 旋盤ﾓｰﾄﾞでの座標入れ替え
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
		// G02/G03 を入れ替え
		if ( 2<=g_ncArgv.nc.nGcode && g_ncArgv.nc.nGcode<=3 )
			g_ncArgv.nc.nGcode = 1 - (g_ncArgv.nc.nGcode-2) + 2;
	}
	else {
		// NCﾃﾞｰﾀの登録前処理
		if ( g_ncArgv.nc.nGtype == G_TYPE ) {
			// G68座標回転指示のﾁｪｯｸ
			if ( nNotModalCode == 68 ) {
				G68RoundCheck(pBlock);
				if ( !g_ncArgv.g68.bG68 )
					return pDataResult;
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
		while ( nRepeat-- > 0 ) {
			if ( g_pParent && !IsThread() )
				break;
			// NC_SearchSubProgram でﾌﾞﾛｯｸが追加される可能性アリ
			// ここでは nLoop 変数を使わず、ﾈｲﾃｨﾌﾞのﾌﾞﾛｯｸｻｲｽﾞにて判定
			for ( i=nIndex; i<g_pDoc->GetNCBlockSize(); i++ ) {
				if ( g_pParent && !IsThread() )
					break;
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

// 切削ｺｰﾄﾞ以外の重要なGｺｰﾄﾞ検査
ENGCODEOBJ CheckGcodeOther_Milling(int nCode)
{
	ENGCODEOBJ	enResult = NOOBJ;

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

	return enResult;
}

ENGCODEOBJ CheckGcodeOther_Lathe(int nCode)
{
	ENGCODEOBJ	enResult = NOOBJ;

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

	return enResult;
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
int NC_SearchMacroProgram(CNCblock* pBlock)
{
	extern	const	int		g_nDefaultMacroID[];	// MCOption.cpp

	CString	strBlock(pBlock->GetStrGcode());
	const CMCOption* pMCopt = AfxGetNCVCApp()->GetMCOption();

	if ( !regex_search((LPCTSTR)strBlock, g_reMacroStr) )
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
	// ﾃﾞｰﾀﾁｪｯｸ
	if ( g_pDoc->IsNCDocFlag(NCDOC_LATHE) ) {
		// 旋盤ﾓｰﾄﾞではｻﾎﾟｰﾄされない
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

	// 計算開始
	if ( cCham == 'C' )
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
	ptResult = pData1->SetChamferingPoint(FALSE, r1);
	if ( !ptResult ) {
		pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_LENGTH);
		return;
	}
	pts = *ptResult - ptOffset;
	// pData2(次のｵﾌﾞｼﾞｪｸﾄ)の始点を補正
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
	// 既に登録された１つ前に面取りｵﾌﾞｼﾞｪｸﾄを挿入
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

void SetEndmillDiameter_fromComment(double d)
{
	// CCommentParser 解析ｱｸｼｮﾝからの呼び出し
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
	g_ncArgv.nEndmillType = c - '0';	// １文字を数値に変換
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

CString SearchFolder(regex& r)
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
	extern	const	DWORD	g_dwSetValFlags[];

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
			g_ncArgv.g68.dRound = g_ncArgv.nc.dValue[NCA_R] * RAD;	// ﾗｼﾞｱﾝで格納
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

// 変数初期化
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
	default:	// or 「0」
		g_ncArgv.nc.enPlane = XY_PLANE;
		break;
	}
	g_ncArgv.nc.nErrorCode = 0;
	g_ncArgv.nc.dwValFlags = 0;
	if ( pMCopt->GetFlag(MC_FLG_LATHE) ) {	// pDoc->IsNCDocFlag(NCDOC_LATHE)
		vector<double>	dDummy;
		SetLatheRect_fromComment(dDummy);	// 旋盤ﾓｰﾄﾞへ強制切替
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
