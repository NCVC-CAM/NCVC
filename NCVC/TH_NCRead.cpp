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

using namespace boost;
using namespace boost::spirit;

#define	IsThread()	g_pParent->IsThreadContinue()
//
static	CThreadDlg*	g_pParent;
static	CNCDoc*		g_pDoc;
static	NCARGV		g_ncArgv;		// NCVCdefine.h
static	DWORD		g_dwValFlags;	// 座標以外の値指示ﾌﾗｸﾞ
static	int			g_nSubprog;		// ｻﾌﾞﾌﾟﾛｸﾞﾗﾑ呼び出しの階層
static	string		g_strComma;		// ｶﾝﾏ以降の文字列
// 固定ｻｲｸﾙのﾓｰﾀﾞﾙ補間値
typedef	struct	tagCYCLE {
	BOOL	bCycle;		// 固定ｻｲｸﾙ処理中
	double	dVal[3];	// Z, R, P
} CYCLE_INTERPOLATE;
static	CYCLE_INTERPOLATE	g_Cycle;
static	void	CycleInterpolate(void);

//
static	void		InitialVariable(void);
// 数値変換( 1/1000 ﾃﾞｰﾀを判断する)
inline	double		GetNCValue(const string& str)
{
	double	dResult = atof(str.c_str());
	if ( str.find('.') == string::npos )
		dResult /= 1000.0;		// 小数点がなければ 1/1000
	return dResult;
}
// G00～G03, G04, G10, G52, G8x, G92
// ｵﾌﾞｼﾞｪｸﾄ生成するＧｺｰﾄﾞﾁｪｯｸ
static	enum	ENGCODEOBJ {NOOBJ, MAKEOBJ, MAKEOBJ_NOTMODAL};
inline	ENGCODEOBJ	IsGcodeObject(int nCode)
{
	ENGCODEOBJ	enResult;

	if ( (0<=nCode && nCode<=3) || nCode==52 || nCode==92 ) {
		g_Cycle.bCycle = FALSE;
		enResult = MAKEOBJ;
	}
	else if ( nCode==4 || nCode==10 ) {
		enResult = MAKEOBJ_NOTMODAL;
	}
	else if ( nCode>=81 && nCode<=89 ) {
		g_Cycle.bCycle = TRUE;
		enResult = MAKEOBJ;
	}
	else
		enResult = NOOBJ;

	return enResult;
}

// 解析関数
static	int		NC_GSeparater(int, CNCdata*&);
static	void	CheckGcodeOther(int);
static	int		NC_SearchSubProgram(int*);
typedef	int		(*PFNSEARCHMACRO)(CNCblock*);
static	int		NC_SearchMacroProgram(CNCblock*);
static	int		NC_NoSearchMacro(CNCblock*);
static	PFNSEARCHMACRO	g_pfnSearchMacro;
static	CNCdata*	MakeChamferingObject(CNCblock*, CNCdata*, CNCdata*);
// Fﾊﾟﾗﾒｰﾀ, ﾄﾞｳｪﾙ時間の解釈
typedef double (*PFNFEEDANALYZE)(const string&);
static	double	FeedAnalyze_Dot(const string&);
static	double	FeedAnalyze_Int(const string&);
static	PFNFEEDANALYZE	g_pfnFeedAnalyze;
// ｻﾌﾞﾌﾟﾛ，ﾏｸﾛの検索
static	CString	g_strSearchFolder[2];	// ｶﾚﾝﾄと指定ﾌｫﾙﾀﾞ
static	CString	SearchFolder(regex&);
static	CString	SearchFolder_Sub(int, LPCTSTR, regex&);
static	BOOL	SearchProgNo(LPCTSTR, regex&);

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
	LPNCVCTHREADPARAM	pParam = (LPNCVCTHREADPARAM)pVoid;
	g_pParent = pParam->pParent;
	g_pDoc = (CNCDoc *)(pParam->pDoc);
	ASSERT(g_pParent);
	ASSERT(g_pDoc);
	InitialVariable();

	{
		CString	strMsg;
		VERIFY(strMsg.LoadString(IDS_READ_NCD));
		g_pParent->SetFaseMessage(strMsg);
	}
	nLoopCnt = g_pDoc->GetNCBlockSize();
	g_pParent->m_ctReadProgress.SetRange32(0, nLoopCnt);
#ifdef _DEBUG
	dbg.printf("LoopCount=%d", nLoopCnt);
#endif

	try {
		// １つ前のｵﾌﾞｼﾞｪｸﾄ参照でNULL参照しないため
		pDataFirst = pData = new CNCdata(&g_ncArgv);
		// 1行(1ﾌﾞﾛｯｸ)解析しｵﾌﾞｼﾞｪｸﾄの登録
		for ( i=0; i<nLoopCnt && IsThread(); i++ ) {
			if ( NC_GSeparater(i, pData) != 0 )
				break;
			if ( (i & 0x003f) == 0 )	// 64回おき(下位6ﾋﾞｯﾄﾏｽｸ)
				g_pParent->m_ctReadProgress.SetPos(i);		// ﾌﾟﾛｸﾞﾚｽﾊﾞｰ
		}
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		nResult = IDCANCEL;
	}

	// 初回登録用ﾀﾞﾐｰﾃﾞｰﾀの消去
	if ( pDataFirst )
		delete	pDataFirst;

	g_pParent->m_ctReadProgress.SetPos(nLoopCnt);
	g_pParent->PostMessage(WM_USERFINISH, IsThread() ? nResult : IDCANCEL);

	return 0;	// AfxEndThread(0);
}

//////////////////////////////////////////////////////////////////////
// Gｺｰﾄﾞの構文解析

struct CGcode : grammar<CGcode>
{
	vector<string>&	vResult;
	CGcode(vector<string>& v) : vResult(v) {}

	template<typename T>
	struct definition
	{
		typedef	rule<T>	rule_t;
		rule_t	rr;
		definition( const CGcode& a )
		{
			// 未知ｺｰﾄﾞにも対応するため「全ての英大文字に続く数値」で解析
			rr = +( upper_p >> real_p )[append(a.vResult)]
				>> !( ch_p(',') >> ((ch_p('R')|'C') >> real_p) )[append(a.vResult)];
		}
		const rule_t& start() const { return rr; }
	};
};

//////////////////////////////////////////////////////////////////////
// Gｺｰﾄﾞの分割(再帰関数)
int NC_GSeparater(int i, CNCdata*& pDataResult)
{
	extern	LPCTSTR			g_szNdelimiter; // "XYZRIJKPLDH"
	extern	const	DWORD	g_dwSetValFlags[];
	vector<string>	vResult;
	vector<string>::iterator	it;
	CGcode		a(vResult);
	int			ii, nCode, nNotModalCode = -1,
				nLoopCnt = g_pDoc->GetNCBlockSize(),
				nResult, nIndex, nRepeat;
	BOOL		bNCobj = FALSE, bNCval = FALSE, bNCsub = FALSE;
	ENGCODEOBJ	enGcode;
	string		strComma;
	CNCdata*	pData = NULL;
	CNCblock*	pBlock = g_pDoc->GetNCblock(i);
	// 変数初期化
	g_ncArgv.nc.nLine		= i;
	g_ncArgv.nc.nErrorCode	= 0;
	g_ncArgv.nc.dwValFlags	&= 0xFFFF0000;
#ifdef _DEBUG
	CMagaDbg	dbg("NC_Gseparate()", DBG_BLUE);
	CMagaDbg	dbg1(DBG_GREEN);
	dbg.printf("No.%004d Line=%s", i+1, g_pDoc->GetNCblock(i)->GetStrGcode());
#endif

	// ﾏｸﾛ置換解析
	if ( (nIndex=(*g_pfnSearchMacro)(pBlock)) >= 0 ) {
		g_nSubprog++;
		for ( ii=nIndex; ii<nLoopCnt && IsThread(); ii++ ) {
			nResult = NC_GSeparater(ii, pDataResult);	// 再帰
			if ( nResult == 30 )
				return 30;
			else if ( nResult == 99 )
				break;
		}
		// EOFで終了ならM99復帰扱い
		if ( ii >= nLoopCnt && nResult == 0 ) {
			if ( g_nSubprog > 0 )
				g_nSubprog--;
		}
		return 0;
	}

	// Gｺｰﾄﾞ構文解析
	if ( !parse( (LPCTSTR)(pBlock->GetStrGcode()),
					a, space_p|comment_p('(', ')') ).full ||
			vResult.empty() )
		return 0;

	for ( it=vResult.begin(); it!=vResult.end() && IsThread(); it++ ) {
#ifdef _DEBUG
		dbg1.printf("G Cut=%s", it->c_str());
#endif
		switch ( it->at(0) ) {
		case 'M':
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
				return 99;
			}
			break;
		case 'G':
			nCode = atoi(it->substr(1).c_str());
			enGcode = IsGcodeObject(nCode);
			if ( enGcode != NOOBJ ) {
				// 前回のｺｰﾄﾞで登録ｵﾌﾞｼﾞｪｸﾄがあるなら
				if ( bNCobj ) {
					// 座標値ﾁｪｯｸしてｵﾌﾞｼﾞｪｸﾄ生成
					if ( !(g_ncArgv.nc.dwValFlags & 0x0000FFFF) )
						pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_VALUE);
					else {
						// ﾓｰﾀﾞﾙでないｺｰﾄﾞのﾁｪｯｸ
						if ( nNotModalCode >= 0 ) {
							nResult = g_ncArgv.nc.nGcode;
							g_ncArgv.nc.nGcode = nNotModalCode;
							pDataResult = g_pDoc->DataOperation(pDataResult, &g_ncArgv);
							g_ncArgv.nc.nGcode = nResult;
							nNotModalCode = -1;
						}
						else
							pDataResult = g_pDoc->DataOperation(pDataResult, &g_ncArgv);
					}
				}
				else
					bNCobj = TRUE;
				if ( enGcode == MAKEOBJ )
					g_ncArgv.nc.nGcode = nCode;
				else
					nNotModalCode = nCode;
			}
			else
				CheckGcodeOther(nCode);
			break;
		case 'F':
			g_ncArgv.dFeed = (*g_pfnFeedAnalyze)(it->substr(1));
			break;
		case ',':
			strComma = ::Trim(it->substr(1));	// ｶﾝﾏ以降を取得
#ifdef _DEBUG
			dbg1.printf("strComma=%s", strComma.c_str());
#endif
			break;
		case 'X':
		case 'Y':
		case 'Z':
		case 'R':
		case 'I':
		case 'J':
		case 'K':
		case 'P':
		case 'L':
		case 'D':
		case 'H':
			nCode = (int)(strchr(g_szNdelimiter, it->at(0)) - g_szNdelimiter);
			// 値取得
			if ( g_ncArgv.nc.nGcode>=81 && g_ncArgv.nc.nGcode<=89 ) {
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
			//
			if ( nCode <= NCA_L ) {
				if ( IsGcodeObject(g_ncArgv.nc.nGcode) != NOOBJ ) {
					g_ncArgv.nc.dwValFlags |= g_dwValFlags;	// 前回のﾙｰﾌﾟ分も加味
					bNCval = TRUE;
					g_dwValFlags = 0;
				}
			}
			else {		// D, H
				// 座標以外の値指示は次のﾙｰﾌﾟ用に退避
				g_dwValFlags |= g_dwSetValFlags[nCode];
			}
			break;
		}
	} // End of for() iterator

	// Mｺｰﾄﾞ後処理
	if ( bNCsub ) {
		if ( !(g_ncArgv.nc.dwValFlags & NCD_P) )
			pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_ORDER);
		else if ( (nIndex=NC_SearchSubProgram(&nRepeat)) < 0 )
			pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_M98);
		else {
			g_nSubprog++;
			// nRepeat分繰り返し
			while ( nRepeat-- > 0 && IsThread() ) {
				for ( ii=nIndex; ii<nLoopCnt && IsThread(); ii++ ) {
					nResult = NC_GSeparater(ii, pDataResult);	// 再帰
					if ( nResult == 30 )
						return 30;
					else if ( nResult == 99 )
						break;
				}
			}
			// EOFで終了ならM99復帰扱い
			if ( ii >= nLoopCnt && nResult == 0 ) {
				if ( g_nSubprog > 0 )
					g_nSubprog--;
			}
		}
		return 0;
	}

	// NCﾃﾞｰﾀ登録処理
	if ( bNCobj || bNCval ) {
		// 座標値ﾁｪｯｸ
		if ( !(g_ncArgv.nc.dwValFlags & 0x0000FFFF) )
			pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_VALUE);
		else {
			// 固定ｻｲｸﾙのﾓｰﾀﾞﾙ補間
			if ( g_Cycle.bCycle )
				CycleInterpolate();
			// NCﾃﾞｰﾀの登録
			// --- 面取り等による再計算項目があるが，
			// --- この時点でｵﾌﾞｼﾞｪｸﾄ登録しておかないと色々面倒(?)
			if ( nNotModalCode >= 0 ) {
				nResult = g_ncArgv.nc.nGcode;
				g_ncArgv.nc.nGcode = nNotModalCode;
				pData = g_pDoc->DataOperation(pDataResult, &g_ncArgv);
				g_ncArgv.nc.nGcode = nResult;
			}
			else
				pData = g_pDoc->DataOperation(pDataResult, &g_ncArgv);
			// 面取りｵﾌﾞｼﾞｪｸﾄの登録
			if ( !g_strComma.empty() )
				MakeChamferingObject(pBlock, pDataResult, pData);
			// ﾌﾞﾛｯｸ情報の更新
			pBlock->SetBlockToNCdata(pData, g_pDoc->GetNCsize());
			// 次の処理へ
			pDataResult = pData;
		}
	}

	// 次の処理に備えてｶﾝﾏ文字列を静的変数へ
	if ( pData )
		g_strComma = strComma;

	return 0;
}

//////////////////////////////////////////////////////////////////////
// 補助関数

// 切削ｺｰﾄﾞ以外の重要なGｺｰﾄﾞ検査
void CheckGcodeOther(int nCode)
{
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
	// 固定ｻｲｸﾙｷｬﾝｾﾙ
	case 80:
		g_Cycle.bCycle = FALSE;
		g_Cycle.dVal[0] = g_Cycle.dVal[1] = g_Cycle.dVal[2] = HUGE_VAL;
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
		g_ncArgv.bInitial = TRUE;
		break;
	case 99:
		g_ncArgv.bInitial = FALSE;
		break;
	}
}

// ｻﾌﾞﾌﾟﾛｸﾞﾗﾑの検索
int NC_SearchSubProgram(int *pRepeat)
{
	int		nProg, i, n,
			nLoopCnt = g_pDoc->GetNCBlockSize();
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

	strProg.Format("(O(0)*%d)($|[^0-9])", nProg);	// 正規表現(Oxxxxにﾏｯﾁする)
	regex	r(strProg);

	// 同じﾒﾓﾘﾌﾞﾛｯｸから検索
	for ( i=0; i<nLoopCnt && IsThread(); i++ ) {
		if ( regex_search((LPCTSTR)(g_pDoc->GetNCblock(i)->GetStrGcode()), r) )
			return i;
	}

	// 機械情報ﾌｫﾙﾀﾞからﾌｧｲﾙ検索
	CString	strFile( SearchFolder(r) );
	if ( strFile.IsEmpty() )
		return -1;

	// Ｏ番号が存在すればNCﾌﾞﾛｯｸの挿入 ->「ｶｰｿﾙ位置に読み込み」でﾌﾞﾛｯｸ追加
	n = nLoopCnt;
	if ( g_pDoc->SerializeInsertBlock(strFile, n, NCF_AUTOREAD, FALSE) ) {
		// 挿入ﾌﾞﾛｯｸの最初だけNCF_FOLDER
		if ( n < nLoopCnt )	// ﾌﾞﾛｯｸ挿入が失敗の可能性もある
			g_pDoc->GetNCblock(n)->SetBlockFlag(NCF_FOLDER);
		return n;
	}

	return -1;
}

// ﾏｸﾛﾌﾟﾛｸﾞﾗﾑの検索
int NC_SearchMacroProgram(CNCblock* pBlock)
{
	extern	int		g_nDefaultMacroID[];	// MCOption.cpp
	
	CString	strBlock(pBlock->GetStrGcode());
	const CMCOption* pMCopt = AfxGetNCVCApp()->GetMCOption();

	// pMCopt->IsMacroSearch() はﾁｪｯｸ済み
	regex	r(pMCopt->GetMacroStr(MCMACROCODE));
	if ( !regex_search((LPCTSTR)strBlock, r) )
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

int NC_NoSearchMacro(CNCblock*)
{
	return -1;
}

CNCdata* MakeChamferingObject(CNCblock* pBlock, CNCdata* pData1, CNCdata* pData2)
{
#ifdef _DEBUG
	CMagaDbg	dbg("MakeChamferingObject()", DBG_BLUE);
#endif
	// ﾃﾞｰﾀﾁｪｯｸ
	if ( pData1->GetGtype() != G_TYPE || pData2->GetGtype() != G_TYPE ||
			pData1->GetGcode() < 1 || pData1->GetGcode() > 3 ||
			pData2->GetGcode() < 1 || pData2->GetGcode() > 3 ) {
		pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_GTYPE);
		return NULL;
	}
	if ( pData1->GetPlane() != pData2->GetPlane() ) {
		pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_PLANE);
		return NULL;
	}
	TCHAR	cCham = g_strComma[0];
	if ( cCham != 'R' && cCham != 'C' ) {
		pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_CHAMFERING);
		return NULL;
	}

	double	r1, r2, cr = fabs(atof(g_strComma.substr(1).c_str()));
	CPointD	pts, pte, pto;
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
			return NULL;
		}
	}

	// pData1(前のｵﾌﾞｼﾞｪｸﾄ)の終点を補正
	ptResult = pData1->SetChamferingPoint(FALSE, r1);
	if ( !ptResult ) {
		pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_LENGTH);
		return NULL;
	}
	pts = *ptResult;
	// pData2(次のｵﾌﾞｼﾞｪｸﾄ)の始点を補正
	ptResult = pData2->SetChamferingPoint(TRUE, r2);
	if ( !ptResult ) {
		pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_LENGTH);
		return NULL;
	}
	pte = *ptResult;

#ifdef _DEBUG
	dbg.printf("%c=%f, %f", cCham, r1, r2);
	dbg.printf("pts=(%f, %f)", pts.x, pts.y);
	dbg.printf("pte=(%f, %f)", pte.x, pte.y);
#endif

	// pts, pte で面取りｵﾌﾞｼﾞｪｸﾄの生成
	NCARGV	ncArgv;
	ZeroMemory(&ncArgv, sizeof(NCARGV));
	ncArgv.bAbs			= TRUE;
	ncArgv.dFeed		= pData1->GetFeed();
	ncArgv.nc.nLine		= pData1->GetStrLine();
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
	return g_pDoc->DataOperation(pData1, &ncArgv, g_pDoc->GetNCsize()-1, NCINS);
}

double FeedAnalyze_Dot(const string& str)
{
	return fabs(GetNCValue(str));
}

double FeedAnalyze_Int(const string& str)
{
	return fabs(atof(str.c_str()));
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
		g_Cycle.bCycle = FALSE;
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

// 変数初期化
void InitialVariable(void)
{
	int		i;
	const CMCOption* pMCopt = AfxGetNCVCApp()->GetMCOption();

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
	for ( i=0; i<NCXYZ; i++ )
		g_ncArgv.nc.dValue[i] = pMCopt->GetInitialXYZ(i);
	for ( ; i<VALUESIZE; i++ )
		g_ncArgv.nc.dValue[i] = 0.0;
	g_ncArgv.bAbs = pMCopt->GetModalSetting(MODALGROUP3) == 0 ? TRUE : FALSE;
	g_ncArgv.bInitial = pMCopt->GetModalSetting(MODALGROUP4) == 0 ? TRUE : FALSE;;
	g_ncArgv.dFeed = pMCopt->GetFeed();

	g_Cycle.bCycle = FALSE;
	g_Cycle.dVal[0] = g_Cycle.dVal[1] = g_Cycle.dVal[2] = HUGE_VAL;

	g_dwValFlags = 0;
	g_nSubprog = 0;
	g_strComma.clear();

	g_pfnSearchMacro = pMCopt->IsMacroSearch() ? &NC_SearchMacroProgram : &NC_NoSearchMacro;
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
