// TH_NCRead.cpp
// ＮＣコードの内部変換
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "NCVC.h"
#include "MCOption.h"
#include "MainFrm.h"
#include "NCdata.h"
#include "NCDoc.h"
#include "ThreadDlg.h"
#include "NCVCdefine.h"
#include "NCVCdefine.h"
#include "Sstring.h"

#include <stdlib.h>
#include <math.h>
#include <memory.h>

// Boost Regex Library
#include <boost/regex.hpp>

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
extern	CMagaDbg	g_dbg;
#endif

#define	IsThread()	g_pParent->IsThreadContinue()
//
static	CThreadDlg*	g_pParent;
static	CNCDoc*		g_pDoc;
static	NCARGV		g_ncArgv;		// NCVCaddin.h
static	int			g_nSubprog;		// ｻﾌﾞﾌﾟﾛｸﾞﾗﾑ呼び出しの階層
static	CString		g_strComma;		// ｶﾝﾏ以降の文字列

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
inline	double		GetNCValue(const CString& strBuf)
{
	double	dResult = atof(strBuf);
	if ( strBuf.Find('.') < 0 )
		dResult /= 1000.0;		// 小数点がなければ 1/1000
	return dResult;
}
// G0～G3, G8x 切削，移動，固定ｻｲｸﾙ指定検査
inline	BOOL		IsGcodeCutter(int nCode)
{
	BOOL	bResult = FALSE;

	if ( nCode >= 0 && nCode <= 3 ) {
		g_Cycle.bCycle = FALSE;
		bResult = TRUE;
	}
	else if ( nCode >= 81 && nCode <= 89 ) {
		g_Cycle.bCycle = TRUE;
		bResult = TRUE;
	}
	return bResult;
}

// 解析関数
static	int		NC_GSeparater(int, CNCdata*&);
static	BOOL	NC_NSeparater(const CString&);
static	BOOL	CheckGcodeOther(int nCode);
static	int		NC_SearchSubProgram(int*);
typedef	int		(*PFNSEARCHMACRO)(CNCblock*);
static	int		NC_SearchMacroProgram(CNCblock*);
static	int		NC_NoSearchMacro(CNCblock*);
static	PFNSEARCHMACRO	g_pfnSearchMacro;
static	BOOL	MakeChamferingObject(CNCdata*, CNCdata*);

// Fﾊﾟﾗﾒｰﾀ, ﾄﾞｳｪﾙ時間の解釈
typedef double (*PFNFEEDANALYZE)(const CString&);
static	double		FeedAnalyze_Dot(const CString&);
static	double		FeedAnalyze_Int(const CString&);
static	PFNFEEDANALYZE	g_pfnFeedAnalyze;
inline	int			NC_Feed(const CString& strFeed, int nType)
{
	int	nGcode = atoi(strFeed);
	if ( nType == F_TYPE )
		g_ncArgv.dFeed = (*g_pfnFeedAnalyze)(strFeed);
	return nGcode;
}

// ｻﾌﾞﾌﾟﾛ，ﾏｸﾛの検索
static	CString		g_strSearchFolder[2];	// ｶﾚﾝﾄと指定ﾌｫﾙﾀﾞ
static	CString		SearchFolder(boost::regex&);
static	BOOL		SearchProgNo(LPCTSTR, boost::regex&);

// ﾌｪｰｽﾞ更新
static	void		SendFaseMessage(void);

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

	// ﾌｪｰｽﾞ1 Gｺｰﾄﾞ分解
	SendFaseMessage();
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
// Gｺｰﾄﾞの分割(再帰関数)
// ｴﾗｰﾃﾞｰﾀは，NCF_ERROR ﾋﾞｯﾄﾌﾗｸﾞで対処
int NC_GSeparater(int i, CNCdata*& pDataResult)
{
	extern	LPCTSTR	gg_szComma;		// StdAfx.cpp
	extern	LPCTSTR	g_szGdelimiter;	// "GSMOF" NCDoc.cpp
	extern	LPCTSTR	g_szGtester[];
	extern	const	DWORD	g_dwSetValFlags[];
	static	CString	ss_strExclude("[({%");
	static	STRING_CUTTER_EX	cutGcode(CString(gg_szComma)+g_szGdelimiter);
	static	STRING_TESTER_EX	tstGcode(GTYPESIZE, g_szGtester, FALSE);

#ifdef _DEBUG
	CMagaDbg	dbg("NC_Gseparate()", DBG_BLUE);
	CMagaDbg	dbg1(DBG_GREEN);
	dbg.printf("No.%004d Line=%s", i+1, g_pDoc->GetNCblock(i)->GetStrGcode());
#endif
	int			ii, nType, nCode, nResult, nIndex, nRepeat;
	BOOL		bNCadd = FALSE;
	CString		strGPiece, strComma;
	CNCdata*	pData = NULL;
	CNCblock*	pBlock = g_pDoc->GetNCblock(i);
	// 変数初期化
	g_ncArgv.nc.nLine		= i;
	g_ncArgv.nc.dwFlags		= 0;
	g_ncArgv.nc.dwValFlags	= 0;

	// ﾏｸﾛ置換解析
	if ( (nIndex=(*g_pfnSearchMacro)(pBlock)) >= 0 ) {
		g_nSubprog++;
		for ( ii=nIndex; ii<g_pDoc->GetNCBlockSize() && IsThread(); ii++ ) {
			nResult = NC_GSeparater(ii, pDataResult);	// 再帰
			if ( nResult == 30 )
				return 30;
			else if ( nResult == 99 )
				break;
			else if ( nResult != 0 ) {
				pBlock->SetBlockFlag(NCF_ERROR);
				continue;
			}
		}
		return 0;
	}

	// Gｺｰﾄﾞ解析
	cutGcode.Set(pBlock->GetStrGcode());
	while ( cutGcode.GiveAPieceEx(strGPiece) && IsThread() ) {
#ifdef _DEBUG
		dbg1.printf("G Cut=%s", strGPiece);
#endif
		// 除外文字のﾁｪｯｸ
		if ( strGPiece.GetLength()<=0 || ss_strExclude.Find(strGPiece[0])>=0 )
			continue;

		// ｶﾝﾏﾁｪｯｸ
		if ( strGPiece[0] == gg_szComma[0] ) {
			strComma = strGPiece.Mid(1);	// ｶﾝﾏ以降を取得
			strComma.TrimLeft();
#ifdef _DEBUG
			dbg1.printf("strComma=%s", strComma);
#endif
			continue;
		}

		// ｺｰﾄﾞﾁｪｯｸ
		nType = tstGcode.TestEx(strGPiece) - 1;
		if ( nType > G_TYPE ) {
			if ( (nCode=NC_Feed(strGPiece.Mid(1), nType)) <= 0 ) {
				pBlock->SetBlockFlag(NCF_ERROR);
				continue;
			}
			if ( nType != M_TYPE )
				continue;
			// Mｺｰﾄﾞ処理
			switch ( nCode ) {
			case 2:
			case 30:
				return 30;
			case 98:
				// 5階層以上の呼び出しはｴﾗｰ(4階層まで)
				if ( g_nSubprog+1 >= 5 ) {
					pBlock->SetBlockFlag(NCF_ERROR);
					continue;
				}
				if ( !NC_NSeparater(strGPiece) ||
					 !(g_ncArgv.nc.dwValFlags & g_dwSetValFlags[NCA_P]) ||
					 (nIndex=NC_SearchSubProgram(&nRepeat)) < 0 ) {
					pBlock->SetBlockFlag(NCF_ERROR);
					continue;
				}
				g_nSubprog++;
				// nRepeat分繰り返し
				while ( nRepeat-- > 0 && IsThread() ) {
					for ( ii=nIndex; ii<g_pDoc->GetNCBlockSize() && IsThread(); ii++ ) {
						nResult = NC_GSeparater(ii, pDataResult);	// 再帰
						if ( nResult == 30 )
							return 30;
						else if ( nResult == 99 )
							break;
						else if ( nResult != 0 ) {
							pBlock->SetBlockFlag(NCF_ERROR);
							continue;
						}
					}
				}
				break;
			case 99:
				if ( g_nSubprog > 0 )
					g_nSubprog--;
				return 99;
			}
			continue;
		}
		// Gｺｰﾄﾞ処理
		if ( nType == G_TYPE ) {
			nCode = atoi(strGPiece.Mid(1));
			if ( IsGcodeCutter(nCode) ) {
				g_ncArgv.nc.nGcode = nCode;
				bNCadd = TRUE;
			}
			else if ( CheckGcodeOther(nCode) ) {
				g_ncArgv.nc.nGcode = nCode;
				bNCadd = TRUE;
			}
		}
		// Gｺｰﾄﾞに続く座標値，または座標単独指示の処理
		if ( NC_NSeparater(strGPiece) )
			bNCadd = TRUE;

	} // End of while()

	if ( bNCadd ) {
		// 固定ｻｲｸﾙのﾓｰﾀﾞﾙ補間
		if ( g_Cycle.bCycle )
			CycleInterpolate();
		// NCﾃﾞｰﾀの登録
		pData = g_pDoc->DataOperation(pDataResult, &g_ncArgv);
		// 面取りｵﾌﾞｼﾞｪｸﾄの登録
		if ( !g_strComma.IsEmpty() && !MakeChamferingObject(pDataResult, pData) )
			g_pDoc->GetNCblock(pDataResult->GetStrLine())->SetBlockFlag(NCF_ERROR);
		//
		pBlock->SetBlockToNCdata(pData, g_pDoc->GetNCsize());
		pDataResult = pData;
	}

	if ( pData )
		g_strComma = strComma;		// 次の処理に備えてｶﾝﾏ文字列を静的変数へ

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Gｺｰﾄﾞに続く値の分割
BOOL NC_NSeparater(const CString& strGPiece)
{
	extern	LPCTSTR	g_szNdelimiter;		// "XYZRIJKPLDH" NCDoc.cpp
	extern	LPCTSTR	g_szNtester[];
	extern	const	DWORD	g_dwSetValFlags[];
	static	STRING_CUTTER_EX	cutNcode(g_szNdelimiter);
	static	STRING_TESTER_EX	tstNcode(VALUESIZE, g_szNtester, FALSE);

#ifdef _DEBUG
	CMagaDbg	dbg("NC_NSeparater()", DBG_BLUE);
	CMagaDbg	dbg1(DBG_CYAN);
#endif
	CString	strNPiece;
	DWORD	dwValFlags = 0;
	int		nType, nPiece = strGPiece.FindOneOf(g_szNdelimiter);

	if ( nPiece < 0 )
		return FALSE;

#ifdef _DEBUG
	dbg.printf("%s", strGPiece.Mid(nPiece));
#endif
	cutNcode.Set(strGPiece.Mid(nPiece));
	while ( cutNcode.GiveAPieceEx(strNPiece) && IsThread() ) {
		nType = tstNcode.TestEx(strNPiece) - 1;
#ifdef _DEBUG
		dbg1.printf("N Cut=%s Type=%d", strNPiece, nType);
#endif
		if ( nType<0 || nType>=VALUESIZE )
			continue;
		else if ( nType < GVALSIZE /*NCA_P*/) {
			// 固定ｻｲｸﾙのKはﾈｲﾃｨﾌﾞで
			if ( g_ncArgv.nc.nGcode>=81 && g_ncArgv.nc.nGcode<=89 && nType==NCA_K )
				g_ncArgv.nc.dValue[NCA_K] = atoi(strNPiece.Mid(1));
			else
				g_ncArgv.nc.dValue[nType] = GetNCValue(strNPiece.Mid(1));
		}
		else {
			// 固定ｻｲｸﾙのP(ﾄﾞｳｪﾙ時間)は送り速度と同じ判定
			if ( g_ncArgv.nc.nGcode>=81 && g_ncArgv.nc.nGcode<=89 && nType==NCA_P )
				g_ncArgv.nc.dValue[NCA_P] = (*g_pfnFeedAnalyze)(strNPiece.Mid(1));
			else
				g_ncArgv.nc.dValue[nType] = atoi(strNPiece.Mid(1));
		}
		g_ncArgv.nc.dwValFlags |= g_dwSetValFlags[nType];
		dwValFlags |= g_dwSetValFlags[nType];
	}

	return dwValFlags == 0 ? FALSE : TRUE;
}

//////////////////////////////////////////////////////////////////////
// 補助関数

// 切削ｺｰﾄﾞ以外の重要なGｺｰﾄﾞ検査
//		FALSE : Modal and coordinates
//		TRUE  : CreateObject(itself)
BOOL CheckGcodeOther(int nCode)
{
	BOOL	bResult = FALSE;

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
	// ﾜｰｸ座標系
	case 54:
	case 55:
	case 56:
	case 57:
	case 58:
	case 59:
		g_pDoc->SelectWorkOffset(nCode - 54);
		break;
	// ﾛｰｶﾙ座標系(切削ｺｰﾄﾞ以外で唯一のｵﾌﾞｼﾞｪｸﾄ生成)
	case 52:
	case 92:
		bResult = TRUE;
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

	return bResult;
}

// ｻﾌﾞﾌﾟﾛｸﾞﾗﾑの検索
int NC_SearchSubProgram(int *pRepeat)
{
	int		nProg, i, n;
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
	boost::regex	r(strProg);

	// 同じﾒﾓﾘﾌﾞﾛｯｸから検索
	for ( i=0; i<g_pDoc->GetNCBlockSize() && IsThread(); i++ ) {
		if ( boost::regex_search(g_pDoc->GetNCblock(i)->GetStrGcode(), r) )
			return i;
	}

	// 機械情報ﾌｫﾙﾀﾞからﾌｧｲﾙ検索
	CString	strFile( SearchFolder(r) );
	if ( strFile.IsEmpty() )
		return -1;

	// Ｏ番号が存在すればNCﾌﾞﾛｯｸの挿入 ->「ｶｰｿﾙ位置に読み込み」でﾌﾞﾛｯｸ追加
	n = g_pDoc->GetNCBlockSize();
	if ( g_pDoc->SerializeInsertBlock(strFile, n, NCF_AUTOREAD, FALSE) ) {
		// ﾌﾞﾛｯｸが挿入されていない場合があるので念のためﾁｪｯｸ
		if ( n < g_pDoc->GetNCBlockSize() )
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
	CMCOption* pMCopt = AfxGetNCVCApp()->GetMCOption();

	// pMCopt->IsMacroSearch() はﾁｪｯｸ済み
	boost::regex	r(pMCopt->GetMacroStr(MCMACROCODE));
	if ( !boost::regex_search(strBlock, r) )
		return -1;
	// 5階層以上の呼び出しはｴﾗｰ(4階層まで)
	if ( g_nSubprog+1 >= 5 ) {
		pBlock->SetBlockFlag(NCF_ERROR);
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
		// ﾌﾞﾛｯｸが挿入されていない場合があるので念のためﾁｪｯｸ
		if ( n < g_pDoc->GetNCBlockSize() )
			g_pDoc->GetNCblock(n)->SetBlockFlag(NCF_FOLDER);
		return n;
	}

	return -1;
}

int NC_NoSearchMacro(CNCblock*)
{
	return -1;
}

BOOL MakeChamferingObject(CNCdata* pData1, CNCdata* pData2)
{
#ifdef _DEBUG
	CMagaDbg	dbg("MakeChamferingObject()", DBG_BLUE);
#endif
	// ﾃﾞｰﾀﾁｪｯｸ
	if ( pData1->GetGtype() != G_TYPE || pData2->GetGtype() != G_TYPE ||
			pData1->GetGcode() < 1 || pData1->GetGcode() > 3 ||
			pData2->GetGcode() < 1 || pData2->GetGcode() > 3 ||
			pData1->GetPlane() != pData2->GetPlane() )
		return FALSE;
	TCHAR	cCham = g_strComma[0];
	if ( cCham != 'R' && cCham != 'C' )
		return FALSE;

	double	r1, r2, cr = fabs(atof(g_strComma.Mid(1)));
	CPointD	pts, pte, ptOrg;
	// 計算開始
	if ( cCham == 'C' )
		r1 = r2 = cr;
	else {
		// ｺｰﾅｰRの場合は，面取りに相当するC値の計算
		if ( !pData1->CalcRoundPoint(pData2, cr, ptOrg, r1, r2) )
			return FALSE;
	}

	// pData1(前のｵﾌﾞｼﾞｪｸﾄ)の終点を補正
	if ( !pData1->SetChamferingPoint(FALSE, r1, pts) )
		return FALSE;
	// pData2(次のｵﾌﾞｼﾞｪｸﾄ)の始点を補正
	if ( !pData2->SetChamferingPoint(TRUE,  r2, pte) )
		return FALSE;

#ifdef _DEBUG
	dbg.printf("%c=%f, %f", cCham, r1, r2);
	dbg.printf("pts=(%f, %f)", pts.x, pts.y);
	dbg.printf("pte=(%f, %f)", pte.x, pte.y);
#endif

	// pts, pte で面取りｵﾌﾞｼﾞｪｸﾄの生成
	NCARGV	ncArgv;
	memcpy((void *)&ncArgv, &g_ncArgv, sizeof(NCARGV));	// ｸﾞﾛｰﾊﾞﾙ変数からｺﾋﾟｰ
	ncArgv.bAbs		= TRUE;		// 絶対値指定
	ncArgv.dFeed	= pData1->GetFeed();
	ncArgv.nc.nLine	= pData1->GetStrLine();
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
	default:
		return FALSE;
	}
	if ( cCham == 'C' )
		ncArgv.nc.nGcode = 1;
	else {
		// ｺｰﾅｰRの場合は，求めたｺｰﾅｰRの中心(ptOrg)から回転方向を計算
		double	pa, pb;
		pts -= ptOrg;	pte -= ptOrg;
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
	pData2 = g_pDoc->DataOperation(pData1, &ncArgv, g_pDoc->GetNCsize()-1, NCINS);

	return TRUE;
}

double FeedAnalyze_Dot(const CString& strBuf)
{
	return fabs(GetNCValue(strBuf));
}

double FeedAnalyze_Int(const CString& strBuf)
{
	return fabs(atof(strBuf));
}

inline	CString	SearchFolder_Sub(int n, LPCTSTR lpszFind, boost::regex& r)
{
	CString	strFile;
	HANDLE	hFind;
	WIN32_FIND_DATA	fd;
	DWORD	dwFlags = FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_SYSTEM;

	if ( (hFind=::FindFirstFile(g_strSearchFolder[n]+lpszFind, &fd)) != INVALID_HANDLE_VALUE ) {
		do {
			strFile = g_strSearchFolder[n] + fd.cFileName;
			if ( !(fd.dwFileAttributes & dwFlags) &&
					boost::regex_search(fd.cFileName, r) &&
					SearchProgNo(strFile, r) )
				return strFile;
		} while ( ::FindNextFile(hFind, &fd) );
		::FindClose(hFind);
	}

	return CString();
}

CString SearchFolder(boost::regex& r)
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
			for ( POSITION pos = pMap->GetStartPosition(); pos; ) {
				pMap->GetNextAssoc(pos, strExt, pFunc);
				strResult = SearchFolder_Sub(i, gg_szWild + strExt, r);
				if ( !strResult.IsEmpty() )
					return strResult;
			}
		}
	}

	return CString();
}

BOOL SearchProgNo(LPCTSTR lpszFile, boost::regex& r)
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
				if ( boost::regex_search(pMap, r) )
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
		g_Cycle.bCycle = FALSE;
		return;
	}

	if ( g_ncArgv.nc.dwValFlags & NCD_Z )
		g_Cycle.dVal[0] = g_ncArgv.nc.dValue[NCA_Z];
	else if ( g_Cycle.dVal[0] != HUGE_VAL ) {
		g_ncArgv.nc.dValue[NCA_Z] = g_Cycle.dVal[0];
		g_ncArgv.nc.dwValFlags |= NCD_Z;
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
	CMCOption* pMCopt = AfxGetNCVCApp()->GetMCOption();

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
	g_ncArgv.nc.dwFlags = g_ncArgv.nc.dwValFlags = 0;
	for ( i=0; i<NCXYZ; i++ )
		g_ncArgv.nc.dValue[i] = pMCopt->GetInitialXYZ(i);
	for ( ; i<VALUESIZE; i++ )
		g_ncArgv.nc.dValue[i] = 0.0;
	g_ncArgv.bAbs = pMCopt->GetModalSetting(MODALGROUP3) == 0 ? TRUE : FALSE;
	g_ncArgv.bInitial = pMCopt->GetModalSetting(MODALGROUP4) == 0 ? TRUE : FALSE;;
	g_ncArgv.dFeed = pMCopt->GetFeed();

	g_Cycle.bCycle = FALSE;
	g_Cycle.dVal[0] = g_Cycle.dVal[1] = g_Cycle.dVal[2] = HUGE_VAL;

	g_nSubprog = 0;
	g_strComma.Empty();

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

// ﾌｪｰｽﾞ更新
void SendFaseMessage(void)
{
#ifdef _DEBUG
	CMagaDbg	dbg("NCDtoXYZ_Thread()", DBG_GREEN);
#endif
	CString	strMsg;
	VERIFY(strMsg.LoadString(IDS_READ_NCD));
	g_pParent->SetFaseMessage(strMsg);
}
