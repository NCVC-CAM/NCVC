// TH_MakeNCD.cpp
// DXF->NC 生成
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "DXFdata.h"
#include "DXFshape.h"
#include "Layer.h"
#include "DXFDoc.h"
#include "NCdata.h"
#include "NCMakeClass.h"
#include "ThreadDlg.h"
#include "NCVCdefine.h"

#include <math.h>
#include <ctype.h>

/*
!!!ATTENTION!!!
生成時間の表示：ﾘﾘｰｽﾊﾞｰｼﾞｮﾝでは外すのを忘れずに
#define	_DBG_NCMAKE_TIME
*/

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
extern	CMagaDbg	g_dbg;
#endif

/*
	CDXFdata の GetType() と GetMakeType() の使い分けに注意！！
*/
// よく使う変数や呼び出しの簡略置換
using namespace boost;
#define	IsThread()	g_pParent->IsThreadContinue()
#define	GetFlg(a)	g_pMakeOpt->GetFlag(a)
#define	GetNum(a)	g_pMakeOpt->GetNum(a)
#define	GetDbl(a)	g_pMakeOpt->GetDbl(a)

// ｸﾞﾛｰﾊﾞﾙ変数定義
static	CThreadDlg*	g_pParent;
static	CDXFDoc*	g_pDoc;
static	CNCMakeOption*	g_pMakeOpt;

static	LPCTSTR	g_szWait = "ﾃﾞｰﾀ準備中!!!";
static	LPCTSTR	g_szFinal = "NCﾃﾞｰﾀ出力中!!!";

// NC生成に必要なﾃﾞｰﾀ群
static	CDXFarray	g_obDXFdata;	// 切削対象ﾃﾞｰﾀ(ﾚｲﾔ毎に抽出)
static	CDXFmap		g_mpDXFdata,	// 座標をｷｰにCDXFdataを格納
			g_mpDXFstarttext,				// 開始ﾚｲﾔ
			g_mpDXFmove, g_mpDXFmovetext,	// 移動ﾚｲﾔﾏｯﾌﾟ
			g_mpDXFtext, g_mpDXFcomment;	// 加工ﾃｷｽﾄ，ｺﾒﾝﾄ専用
static	CDXFarray	g_obPoint;		// 穴加工ﾃﾞｰﾀ
static	CDXFsort	g_obCircle;		// 円ﾃﾞｰﾀを穴加工するときの仮登録
static	CDXFsort	g_obDrillGroup;	// ｸﾞﾙｰﾌﾟ分けした穴加工ﾃﾞｰﾀ
static	CDXFsort	g_obDrillAxis;	// -> 軸座標で並べ替え
static	CDXFsort	g_obStartData;	// 加工開始位置指示ﾃﾞｰﾀ
static	CDXFmap		g_mpEuler;		// 一筆書きｵﾌﾞｼﾞｪｸﾄを格納
static	CDXFlist	g_ltDeepGlist(1024);// 深彫切削用の仮登録
static	CTypedPtrArrayEx<CPtrArray, CNCMake*>	g_obMakeGdata;	// 加工ﾃﾞｰﾀ

static	BOOL		g_bData;		// 各生成処理で生成されたか
static	double		g_dZCut;		// Z軸の切削座標 == RoundUp(GetDbl(MKNC_DBL_ZCUT))
static	double		g_dDeep;		// 深彫の切削座標 == RoundUp(GetDbl(MKNC_DBL_DEEP))
static	double		g_dZInitial;	// Z軸のｲﾆｼｬﾙ点 == RoundUp(GetDbl(MKNC_DBL_G92Z))
static	double		g_dZG0Stop;		// Z軸のR点 == RoundUp(GetDbl(MKNC_DBL_ZG0STOP))
static	double		g_dZReturn;		// Z軸の復帰座標

// ｻﾌﾞ関数
static	void	InitialVariable(void);		// 変数初期化
static	void	InitialCycleBaseVariable(void);	// 固定ｻｲｸﾙのﾍﾞｰｽ値初期化
static	BOOL	MakeNCD_MainThread(CString&);	// NC生成のﾒｲﾝﾙｰﾌﾟ
static	BOOL	SingleLayer(void);			// 単一ﾚｲﾔ処理
static	BOOL	MultiLayer(int);			// 複数ﾚｲﾔ処理(2ﾊﾟﾀｰﾝ兼用)
static	void	SetStaticOption(void);		// 静的変数の初期化
static	BOOL	SetStartData(void);			// 加工開始位置指示ﾚｲﾔの仮登録, 移動指示ﾚｲﾔの処理
static	void	SetGlobalArray_Single(void);
static	void	SetGlobalArray_Multi(CLayerData*);
static	void	SetGlobalArray_Sub(CDXFdata*);
static	void	SetGlobalMap_Other(void);
static	BOOL	MakeNCD_FinalFunc(LPCTSTR = NULL);	// 終了ｺｰﾄﾞ，ﾌｧｲﾙ出力など
static	BOOL	OutputNCcode(LPCTSTR);		// NCｺｰﾄﾞの出力

// 各ﾀｲﾌﾟ別の処理呼び出し
static	enum	ENMAKETYPE {MAKECUTTER, MAKEDRILLPOINT, MAKEDRILLCIRCLE};
static	BOOL	CallMakeDrill_Sub(CString&);
static	BOOL	CallMakeLoop(ENMAKETYPE, CString&);

// 原点調整(TRUE:IsMatch)
static	tuple<CDXFdata*, BOOL>	OrgTuningCutter(void);
static	tuple<CDXFdata*, BOOL>	OrgTuningDrillPoint(void);
static	CDXFdata*	OrgTuningDrillCircle(void);

// ﾃﾞｰﾀ解析関数
static	BOOL		MakeLoopEuler(BOOL, CDXFdata*);
static	BOOL		MakeLoopEulerSearch(CPointD&);
static	int			MakeLoopEulerAdd(void);
static	BOOL		MakeLoopEulerAdd_with_one_stroke(CPointD&, CDXFarray*, CDXFlist&, BOOL);
static	BOOL		MakeLoopDeepAdd(void);
typedef	CDXFdata*	(*PFNDEEPPROC)(BOOL);
static	PFNDEEPPROC	g_pfnDeepHead, g_pfnDeepTail;
static	CDXFdata*	MakeLoopAddDeepHead(BOOL);
static	CDXFdata*	MakeLoopAddDeepTail(BOOL);
static	CDXFdata*	MakeLoopAddDeepHeadAll(BOOL);
static	CDXFdata*	MakeLoopAddDeepTailAll(BOOL);
static	void		MakeLoopAddDeepZProc(BOOL, BOOL, CDXFdata*);
static	void		MakeLoopDeepZDown(void);
static	void		MakeLoopDeepZUp(void);
static	BOOL		MakeLoopDrillPoint(BOOL, CDXFdata*);
static	BOOL		MakeLoopDrillPointSeqChk(BOOL, CDXFsort&);
static	BOOL		MakeLoopDrillPointXY(BOOL);
static	BOOL		MakeLoopDrillPointXYRevers(BOOL);
static	BOOL		MakeLoopDrillCircle(void);
static	BOOL		MakeLoopAddDrill(BOOL, CDXFdata*);
static	BOOL		MakeLoopAddDrillSeq(int);
static	CDXFdata*	MakeLoopAddFirstMove(ENMAKETYPE);
static	BOOL		MakeLoopAddLastMove(void);

// 引数で指定したｵﾌﾞｼﾞｪｸﾄに一番近いｵﾌﾞｼﾞｪｸﾄを返す(戻り値:Z軸の移動が必要か)
static	tuple<CDXFdata*, BOOL>	GetNearPointCutter(CDXFdata*);
static	tuple<CDXFdata*, BOOL>	GetNearPointDrill(CDXFdata*);
typedef	BOOL	(*PFNGETMATCHPOINTMOVE)(CDXFdata*&);
static	PFNGETMATCHPOINTMOVE	g_pfnGetMatchPointMove;
static	BOOL	GetMatchPointMove_Target(CDXFdata*&);
static	BOOL	GetMatchPointMove_Exclude(CDXFdata*&);

// ﾍｯﾀﾞｰ,ﾌｯﾀﾞｰ等のｽﾍﾟｼｬﾙｺｰﾄﾞ生成
static	void	AddCustomCode(const CString&, const CDXFdata*);
static	BOOL	IsNCchar(LPCTSTR);

// ﾃｷｽﾄ情報
typedef	void	(*PFNADDTEXT)(CPointD&);
static	PFNADDTEXT	g_pfnAddCutterText, g_pfnAddCommentText,
					g_pfnAddStartText, g_pfnAddMoveText;
static	void	AddCutterText_Target(CPointD&);
static	void	AddStartText_Target(CPointD&);
static	void	AddMoveText_Target(CPointD&);
static	void	AddCommentText_Target(CPointD&);
static	void	AddText_Exclude(CPointD&);

// NCﾃﾞｰﾀ登録関数
static	void	AddMakeStart(void);		// 加工開始指示ﾃﾞｰﾀの生成
typedef void	(*PFNADDMOVE)(void);	// 移動指示ﾚｲﾔの移動時における動作関数
static	PFNADDMOVE	g_pfnAddMoveZ, g_pfnAddMoveCust_B, g_pfnAddMoveCust_A;
static	void	AddMoveZ_NotMove(void);
static	void	AddMoveZ_R(void);
static	void	AddMoveZ_Initial(void);
static	void	AddMoveCust_B(void);
static	void	AddMoveCust_A(void);
// ﾃｷｽﾄ情報(切削ﾚｲﾔとｺﾒﾝﾄﾚｲﾔ)の検索
inline	void	AddCutterTextIntegrated(CPointD& pt)
{
	(*g_pfnAddCommentText)(pt);
	(*g_pfnAddCutterText)(pt);
}
// ﾃｷｽﾄ情報(開始ﾚｲﾔとｺﾒﾝﾄﾚｲﾔ)の検索
inline	void	AddStartTextIntegrated(CPointD& pt)
{
	(*g_pfnAddCommentText)(pt);
	(*g_pfnAddStartText)(pt);
}
// ﾃｷｽﾄ情報(移動ﾚｲﾔとｺﾒﾝﾄﾚｲﾔ)の検索
inline	void	AddMoveTextIntegrated(CPointD& pt)
{
	(*g_pfnAddCommentText)(pt);
	(*g_pfnAddMoveText)(pt);
}
// Z軸の移動(切削)ﾃﾞｰﾀ生成
inline	void	AddMoveGdata(int nCode, double dZ, double dFeed)
{
	CNCMake*	mkNCD = new CNCMake(nCode, dZ, dFeed);
	ASSERT( mkNCD );
	g_obMakeGdata.Add(mkNCD);
}
// Z軸の上昇
inline	void	AddMoveGdataZup(void)
{
	CPointD	pt( CDXFdata::ms_pData->GetEndMakePoint() );
	// 終点座標でのｺﾒﾝﾄ生成
	AddCutterTextIntegrated(pt);
	// Z軸の上昇
	AddMoveGdata(0, g_dZReturn, -1.0);
}
// Z軸の下降
inline	void	AddMoveGdataZdown(void)
{
	// Z軸の現在位置がR点より大きい(高い)ならR点まで早送り
	if ( CNCMake::ms_xyz[NCA_Z] > g_dZG0Stop )
		AddMoveGdata(0, g_dZG0Stop, -1.0);
	// 加工済み深さへのZ軸切削移動
	double	dZValue;
	switch ( GetNum(MKNC_NUM_MAKEEND) ) {
	case 1:		// ｵﾌｾｯﾄ移動
		dZValue = g_dZCut + GetDbl(MKNC_DBL_MAKEEND);
		break;
	case 2:		// 固定Z値
		dZValue = GetDbl(MKNC_DBL_MAKEEND);
		break;
	default:
		dZValue = HUGE_VAL;
		break;
	}
	if ( CNCMake::ms_xyz[NCA_Z] > dZValue )
		AddMoveGdata(1, dZValue, GetDbl(MKNC_DBL_MAKEENDFEED));
	// 切削点まで切削送り
	if ( CNCMake::ms_xyz[NCA_Z] > g_dZCut )
		AddMoveGdata(1, g_dZCut, GetDbl(MKNC_DBL_ZFEED));
}
inline	void	AddMoveGdataG0(const CDXFdata* pData)
{
	// G0移動ﾃﾞｰﾀの生成
	CNCMake* mkNCD = new CNCMake(0, pData->GetStartMakePoint());
	ASSERT( mkNCD );
	g_obMakeGdata.Add(mkNCD);
	// Z軸の下降
	AddMoveGdataZdown();
}
inline	void	AddMoveGdataG1(const CDXFdata* pData)
{
	// Z軸の下降
	AddMoveGdataZdown();
	// G1移動ﾃﾞｰﾀの生成
	CNCMake* mkNCD = new CNCMake(1, pData->GetStartMakePoint());
	ASSERT( mkNCD );
	g_obMakeGdata.Add(mkNCD);
}
// 切削ﾃﾞｰﾀの生成
inline	void	AddMakeGdata(CDXFdata* pData)
{
	CPointD	pt( pData->GetStartMakePoint() );
	// 開始位置と等しいﾃｷｽﾄﾃﾞｰﾀの生成
	AddCutterTextIntegrated(pt);
	// 切削ﾃﾞｰﾀ生成
	CNCMake*	mkNCD = new CNCMake(pData,
		pData->GetMakeType() == DXFPOINTDATA ?
			GetDbl(MKNC_DBL_DRILLFEED) : GetDbl(MKNC_DBL_FEED) );
	ASSERT( mkNCD );
	g_obMakeGdata.Add(mkNCD);
	pData->SetMakeFlg();
}
// 切削ﾃﾞｰﾀの生成（深彫）
inline	void	AddMakeGdataDeep(CDXFdata* pData, BOOL bDeep)
{
	CPointD	pt( pData->GetStartMakePoint() );
	// 開始位置と等しいﾃｷｽﾄﾃﾞｰﾀの生成
	AddCutterTextIntegrated(pt);
	// 切削ﾃﾞｰﾀ生成
	CNCMake*	mkNCD = new CNCMake(pData,
		bDeep ? GetDbl(MKNC_DBL_DEEPFEED) : GetDbl(MKNC_DBL_FEED) );
	ASSERT( mkNCD );
	g_obMakeGdata.Add(mkNCD);
	pData->SetMakeFlg();
}
// 任意ﾃﾞｰﾀの生成
inline	void	AddMakeGdata(const CString& strData)
{
	CNCMake*	mkNCD = new CNCMake(strData);
	ASSERT( mkNCD );
	g_obMakeGdata.Add(mkNCD);
}
// 移動指示ﾚｲﾔのﾃﾞｰﾀ生成
inline	void	AddMakeMove(CDXFdata* pData)
{
	CPointD	pt( pData->GetStartMakePoint() );
	// 開始位置と等しいﾃｷｽﾄﾃﾞｰﾀの生成
	AddMoveTextIntegrated(pt);
	// 移動指示
	CNCMake*	mkNCD = new CNCMake(pData);
	ASSERT( mkNCD );
	g_obMakeGdata.Add(mkNCD);
	pData->SetMakeFlg();
}
// 固定ｻｲｸﾙｷｬﾝｾﾙｺｰﾄﾞ
inline	void	AddMakeGdataCycleCancel(void)
{
	if ( CDXFdata::ms_pData->GetMakeType() == DXFPOINTDATA ) {
		AddMakeGdata( CNCMake::MakeCustomString(80) );
		InitialCycleBaseVariable();
	}
}

// ﾌｪｰｽﾞ更新
static	int		g_nFase;			// ﾌｪｰｽﾞ№
static	void	SendFaseMessage(int = -1, LPCTSTR = NULL, LPCTSTR = NULL);
inline	void	SendProgressPos(int i)
{
	if ( (i & 0x003f) == 0 )	// 64回おき(下位6ﾋﾞｯﾄﾏｽｸ)
		g_pParent->m_ctReadProgress.SetPos(i);
}

// 並べ替え補助関数
int		CircleSizeCompareFunc1(CDXFdata*, CDXFdata*);	// 円ﾃﾞｰﾀ昇順ｿｰﾄ
int		CircleSizeCompareFunc2(CDXFdata*, CDXFdata*);	// 円ﾃﾞｰﾀ降順ｿｰﾄ
int		DrillOptimaizeCompareFuncX(CDXFdata*, CDXFdata*);	// X軸
int		DrillOptimaizeCompareFuncY(CDXFdata*, CDXFdata*);	// Y軸

// ｻﾌﾞｽﾚｯﾄﾞ関数
static	CCriticalSection	g_csMakeAfter;	// MakeNCDAfterThread()ｽﾚｯﾄﾞﾛｯｸｵﾌﾞｼﾞｪｸﾄ
static	UINT	MakeNCDAfterThread(LPVOID);	// 後始末ｽﾚｯﾄﾞ
/*
	MAKENCDTHREADPARAM::evStart について
		特に SearchObjectThread() において
		１つのグローバルイベントで複数スレッドの同期処理を行う方が効率が良いが，
		実験の結果，うまくコントロールできないことが判明．
		よって，各起動スレッドごとにループ開始イベントを設定することにした．
*/
class	MAKENCDTHREADPARAM {
public:
	CEvent	evStart,		// ﾙｰﾌﾟ開始ｲﾍﾞﾝﾄ
			evEnd;			// 終了待ち確認
	BOOL	bThread,		// ｽﾚｯﾄﾞの継続ﾌﾗｸﾞ
			bResult;		// ｽﾚｯﾄﾞの結果
	int		nOrder,			// ｽﾚｯﾄﾞの処理対象
			nOrder2;
	// MAKENCDTHREADPARAM::CEvent を手動ｲﾍﾞﾝﾄにするためのｺﾝｽﾄﾗｸﾀ
	MAKENCDTHREADPARAM() : evStart(FALSE, TRUE), evEnd(FALSE, TRUE),
		bThread(TRUE), bResult(TRUE), nOrder(-1)
	{}
};
#define	LPMAKENCDTHREADPARAM	MAKENCDTHREADPARAM *
//
static	void	SetSearchRange(void);		// 各ｽﾚｯﾄﾞの検索範囲
static	UINT	SearchObjectThread(LPVOID);		// 次の切削ｵﾌﾞｼﾞｪｸﾄの検索(from GetNearPointCutter)
static	UINT	ClearPointMapThread(LPVOID);	// CMapｵﾌﾞｼﾞｪｸﾄｸﾘｱのﾊﾞｯｸｸﾞﾗｳﾝﾄﾞｽﾚｯﾄﾞ
typedef	struct	tagSEARCHRESULT {
	CDXFdata*	pData;
	double		dGap;
	BOOL		bMatch;
} SEARCHRESULT, *LPSEARCHRESULT;
extern	int		g_nProcesser;		// ﾌﾟﾛｾｯｻ数(NCVC.cpp)
static	LPMAKENCDTHREADPARAM	g_lpmSearch = NULL;
static	MAKENCDTHREADPARAM		g_pmClearMap;
static	LPSEARCHRESULT	g_lpSearchResult = NULL;
static	LPHANDLE		g_phSearchEnd = NULL;

//////////////////////////////////////////////////////////////////////
// NC生成ｽﾚｯﾄﾞ
//////////////////////////////////////////////////////////////////////

UINT MakeNCD_Thread(LPVOID pVoid)
{
#ifdef _DEBUG
	CMagaDbg	dbg("MakeNCD_Thread()\nStart", DBG_GREEN);
#endif
	int		i, nResult = IDCANCEL;

#ifdef _DBG_NCMAKE_TIME
	// 現在時刻を取得
	CTime t1 = CTime::GetCurrentTime();
#endif

	// ｸﾞﾛｰﾊﾞﾙ変数初期化
	LPNCVCTHREADPARAM	pParam = (LPNCVCTHREADPARAM)pVoid;
	g_pParent = pParam->pParent;
	g_pDoc = (CDXFDoc *)(pParam->pDoc);
	ASSERT(g_pParent);
	ASSERT(g_pDoc);

	// 準備中表示
	g_nFase = 0;
	SendFaseMessage(-1, g_szWait);
	g_pMakeOpt = NULL;

	// ｽﾚｯﾄﾞﾊﾝﾄﾞﾙ
	LPHANDLE	hSearchThread = NULL;
	HANDLE		hClearMapThread = NULL;

	// 下位の CMemoryException は全てここで集約
	try {
		int		nType;
		BOOL	bResult = FALSE;

		// ｽﾚｯﾄﾞﾊﾝﾄﾞﾙを必要分確保
		hSearchThread	= new HANDLE[g_nProcesser];

		// NC生成ｵﾌﾟｼｮﾝｵﾌﾞｼﾞｪｸﾄの生成
		g_pMakeOpt = new CNCMakeOption(NULL);	// 読み込みはしない
		CNCMake::ms_pMakeOpt = g_pMakeOpt;

		// NC生成ﾀｲﾌﾟ
		nType = (int)( ((LPNCVCTHREADPARAM)pParam)->pParam );

		// 変数初期化ｽﾚｯﾄﾞの処理待ち
		g_csMakeAfter.Lock();		// ｽﾚｯﾄﾞ側でﾛｯｸ解除するまで待つ
		g_csMakeAfter.Unlock();
#ifdef _DEBUG
		dbg.printf("g_csMakeAfter Unlock OK");
#endif

		// NC生成のﾙｰﾌﾟ前に必要な初期化
		CDXFdata::ms_ptOrg = g_pDoc->GetCutterOrigin();
		g_pDoc->GetCircleObject()->OrgTuning(FALSE);	// 結果的に原点がｾﾞﾛになる
		InitialVariable();
		// 増分割り当て
		i = g_pDoc->GetDxfLayerDataCnt(DXFCAMLAYER);
		g_obDXFdata.SetSize(0, i);
		g_obPoint.SetSize(0, i);
		g_obCircle.SetSize(0, i);
		g_obMakeGdata.SetSize(0, i*2);
		g_mpDXFdata.InitHashTable(max(17, GetPrimeNumber(i*2)));
		g_mpEuler.InitHashTable(max(17, GetPrimeNumber(i)));
		g_mpDXFtext.InitHashTable(max(17, GetPrimeNumber(i)));
		i = g_pDoc->GetDxfLayerDataCnt(DXFSTRLAYER);
		g_obStartData.SetSize(0, max(10, i*2));
		g_mpDXFstarttext.InitHashTable(max(17, GetPrimeNumber(i)));
		i = g_pDoc->GetDxfLayerDataCnt(DXFMOVLAYER);
		g_mpDXFmove.InitHashTable(max(17, GetPrimeNumber(i*2)));
		g_mpDXFmovetext.InitHashTable(max(17, GetPrimeNumber(i)));
		g_mpDXFcomment.InitHashTable(max(17, GetPrimeNumber(g_pDoc->GetDxfLayerDataCnt(DXFCOMLAYER))));

		// 関連ｽﾚｯﾄﾞ起動
		CWinThread*	pThread;
		for ( i=0; i<g_nProcesser; i++ ) {
			g_lpmSearch[i].bThread = TRUE;
			pThread = AfxBeginThread(SearchObjectThread, (LPVOID)i);
			hSearchThread[i] = NC_DuplicateHandle(pThread->m_hThread);
		}
		g_pmClearMap.bThread = TRUE;
		g_pmClearMap.evEnd.SetEvent();
		pThread = AfxBeginThread(ClearPointMapThread, NULL);
		hClearMapThread = NC_DuplicateHandle(pThread->m_hThread);

		// ﾒｲﾝﾙｰﾌﾟへ
		switch ( nType ) {
		case ID_FILE_DXF2NCD:		// 単一ﾚｲﾔ
			bResult = SingleLayer();
			break;

		case ID_FILE_DXF2NCD_EX1:	// 複数の切削条件ﾌｧｲﾙ
		case ID_FILE_DXF2NCD_EX2:	// 複数Z座標
			bResult = MultiLayer(nType);
			break;
		}

		// 戻り値ｾｯﾄ
		if ( bResult && IsThread() )
			nResult = IDOK;
#ifdef _DEBUG
		dbg.printf("MakeNCD_Thread All Over!!!");
#endif
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
	}

#ifdef _DBG_NCMAKE_TIME
	// 終了時刻を取得，生成時間の計算
	CTime t2 = CTime::GetCurrentTime();
	CTimeSpan ts = t2 - t1;
	CString	strTime( ts.Format("%H:%M:%S") );
	AfxMessageBox( strTime );
#endif

	// 関連ｽﾚｯﾄﾞ終了指示
	if ( hClearMapThread ) {
		g_pmClearMap.bThread = FALSE;
		g_pmClearMap.evStart.SetEvent();
		WaitForSingleObject(hClearMapThread, INFINITE);
		CloseHandle(hClearMapThread);
	}
	g_pmClearMap.evEnd.ResetEvent();
	if ( hSearchThread ) {
		for ( i=0; i<g_nProcesser; i++ ) {
			g_lpmSearch[i].bThread = FALSE;
			g_lpmSearch[i].evStart.SetEvent();
		}
		WaitForMultipleObjects(g_nProcesser, hSearchThread, TRUE, INFINITE);
		for ( i=0; i<g_nProcesser; i++ ) {
			CloseHandle(hSearchThread[i]);
			g_lpmSearch[i].evEnd.ResetEvent();
		}
	}

	// 終了処理
	CDXFmap::ms_dTolerance = NCMIN;		// 規定値に戻す
	g_pParent->PostMessage(WM_USERFINISH, nResult);	// このｽﾚｯﾄﾞからﾀﾞｲｱﾛｸﾞ終了
	// 生成したNCｺｰﾄﾞの消去ｽﾚｯﾄﾞ(優先度を下げる)
	AfxBeginThread(MakeNCDAfterThread, NULL,
		/*THREAD_PRIORITY_LOWEST*/
		THREAD_PRIORITY_IDLE
		/*THREAD_PRIORITY_BELOW_NORMAL*/);

	// 条件ｵﾌﾞｼﾞｪｸﾄ削除
	if ( g_pMakeOpt )
		delete	g_pMakeOpt;
	// ｽﾚｯﾄﾞﾊﾝﾄﾞﾙ削除
	if ( hSearchThread )
		delete[]	hSearchThread;

	return 0;
}

//////////////////////////////////////////////////////////////////////

void InitialVariable(void)
{
	g_bData = FALSE;
	CDXFdata::ms_pData = g_pDoc->GetCircleObject();
	CNCMake::InitialVariable();
}

void InitialCycleBaseVariable(void)
{
	CNCMake::ms_dCycleZ[1] =
		CNCMake::ms_dCycleR[1] =
			CNCMake::ms_dCycleP[1] = HUGE_VAL;
}

//////////////////////////////////////////////////////////////////////

BOOL SingleLayer(void)
{
#ifdef _DEBUG
	CMagaDbg	dbg("SingleLayer()\nStart", DBG_CYAN);
#endif
	// NC生成ｵﾌﾟｼｮﾝ読み込み
	g_pMakeOpt->ReadMakeOption(AfxGetNCVCApp()->GetDXFOption()->GetInitList()->GetHead());
	// 条件ごとに変化するﾊﾟﾗﾒｰﾀを設定
	SetStaticOption();
	// 加工原点開始位置指示ﾚｲﾔ処理
	if ( !SetStartData() )
		return FALSE;
	// Z軸の切削座標ｾｯﾄ
	g_dZCut = RoundUp(GetDbl(MKNC_DBL_ZCUT));
	g_dDeep = RoundUp(GetDbl(MKNC_DBL_DEEP));
	// 固定ｻｲｸﾙの切り込み座標ｾｯﾄ
	CNCMake::ms_dCycleZ[0] = RoundUp(GetDbl(MKNC_DBL_DRILLZ));
	CNCMake::ms_dCycleR[0] = RoundUp(GetDbl(MKNC_DBL_DRILLR));
	CNCMake::ms_dCycleP[0] = GetNum(MKNC_NUM_DWELL);
	InitialCycleBaseVariable();
	// ｸﾞﾛｰﾊﾞﾙ変数に生成対象ｵﾌﾞｼﾞｪｸﾄのｺﾋﾟｰ
	SetGlobalArray_Single();
	// 生成開始
	if ( !MakeNCD_MainThread(CString()) ) {
#ifdef _DEBUG
		dbg.printf("Error:MakeNCD_MainThread()");
#endif
		return FALSE;
	}

	// 最終ﾁｪｯｸ
	if ( g_obMakeGdata.GetSize() == 0 ) {
		AfxMessageBox(IDS_ERR_MAKENC, MB_OK|MB_ICONSTOP);
		return FALSE;
	}
	// 終了ｺｰﾄﾞ，ﾌｧｲﾙの出力など
	if ( !MakeNCD_FinalFunc() ) {
#ifdef _DEBUG
		dbg.printf("Error:MakeNCD_FinalFunc()");
#endif
		return FALSE;
	}

	return TRUE;
}

BOOL MultiLayer(int nType)
{
#ifdef _DEBUG
	CMagaDbg	dbg("MultiLayer()\nStart", DBG_CYAN);
	CMagaDbg	dbgE("MultiLayer() Error", DBG_RED);
#endif
	extern	LPCTSTR	gg_szCat;
	int		i, j, nLayerCnt = g_pDoc->GetLayerCnt();
	BOOL	bPartOut = FALSE;	// １回でも個別出力があればTRUE
	CLayerData*	pLayer;
	CString	strLayer;

	if ( nType == ID_FILE_DXF2NCD_EX2 ) {
		// 基準となるNC生成ｵﾌﾟｼｮﾝ読み込み
		g_pMakeOpt->ReadMakeOption(AfxGetNCVCApp()->GetDXFOption()->GetInitList()->GetHead());
		// 条件ごとに変化するﾊﾟﾗﾒｰﾀを設定
		SetStaticOption();
	}
	// 加工原点開始位置指示ﾚｲﾔ処理
	if ( !SetStartData() )
		return FALSE;

	// ﾚｲﾔ毎のﾙｰﾌﾟ
	for ( i=0; i<nLayerCnt && IsThread(); i++ ) {
		pLayer = g_pDoc->GetLayerData(i);
		// 切削ﾚｲﾔで対象のﾚｲﾔだけ
		if ( !pLayer->IsCutType() || !pLayer->IsCutTarget() )
			continue;
		pLayer->SetLayerFlags();
		strLayer = pLayer->GetStrLayer();
#ifdef _DEBUG
		dbg.printf("No.%d ID=%d Name=%s", i+1,
			pLayer->GetListNo(), strLayer);
#endif
		// 処理ﾚｲﾔ名をﾀﾞｲｱﾛｸﾞに表示
		SendFaseMessage(-1, g_szWait, strLayer);
		//
		if ( nType == ID_FILE_DXF2NCD_EX1 ) {
			// NC生成ｵﾌﾟｼｮﾝ読み込み
			if ( pLayer->GetInitFile().CompareNoCase(g_pMakeOpt->GetInitFile()) != 0 ) {
				g_pMakeOpt->ReadMakeOption(pLayer->GetInitFile());	// 違うときだけ
				SetStaticOption();
			}
			// Z軸の切削座標ｾｯﾄ
			g_dZCut = RoundUp(GetDbl(MKNC_DBL_ZCUT));
			g_dDeep = RoundUp(GetDbl(MKNC_DBL_DEEP));
		}
		else {
			// 強制Z座標補正
			g_dZCut = g_dDeep = RoundUp(pLayer->GetZCut());
		}
		// 固定ｻｲｸﾙの切り込み座標ｾｯﾄ
		CNCMake::ms_dCycleZ[0] = pLayer->IsDrillZ() ?
					g_dZCut : RoundUp(GetDbl(MKNC_DBL_DRILLZ));
		CNCMake::ms_dCycleR[0] = RoundUp(GetDbl(MKNC_DBL_DRILLR));
		CNCMake::ms_dCycleP[0] = GetNum(MKNC_NUM_DWELL);
		InitialCycleBaseVariable();
		// ｸﾞﾛｰﾊﾞﾙ変数に生成対象ｵﾌﾞｼﾞｪｸﾄのｺﾋﾟｰ
		SetGlobalArray_Multi(pLayer);
		// 生成開始
#ifdef _DEBUG
		dbg.printf("Layer=%s Cut=%f Start", strLayer, g_dZCut);
#endif
		if ( !MakeNCD_MainThread(strLayer) ) {
#ifdef _DEBUG
			dbgE.printf("MakeNCD_MainThread() Error");
#endif
			return FALSE;
		}
		// 個別出力でないなら(並べ替えしているので途中に割り込むことはない)
		if ( !pLayer->IsPartOut() )
			continue;
		// --- 以下個別出力のみの処理
		if ( g_bData ) {	// NC生成ｱﾘ？
			// MakeNCD_FinalFunc(終了ｺｰﾄﾞ，ﾌｧｲﾙの出力)の実行
			if ( MakeNCD_FinalFunc(pLayer->GetNCFile()) ) {
				// ｸﾞﾛｰﾊﾞﾙ変数初期化
				InitialVariable();
				// NC生成済情報の削除
				for ( j=0; j<g_obMakeGdata.GetSize() && IsThread(); j++ )
					delete	g_obMakeGdata[j];
				g_obMakeGdata.RemoveAll();
			}
			else {
#ifdef _DEBUG
				dbgE.printf("MakeNCD_FinalFunc_Multi() Error");
#endif
				return FALSE;
			}
			bPartOut = TRUE;	// １回でも個別出力あり
		}
		else {
#ifdef _DEBUG
			dbg.printf("Layer=%s CDXFdata::ms_pData NULL", pLayer->GetStrLayer());
#endif
			// 該当ﾚｲﾔのﾃﾞｰﾀなし
			pLayer->SetLayerFlags(1);
		}
	}	// End of for main loop (Layer)

	// --- 最終ﾁｪｯｸ
	if ( g_bData ) {	// ﾙｰﾌﾟが全体出力で終了
		if ( g_obMakeGdata.GetSize() == 0 ) {
			if ( bPartOut ) {	// 個別出力があれば
				// 個別出力以外のﾚｲﾔ情報を取得し，ﾜｰﾆﾝｸﾞﾒｯｾｰｼﾞ出力へ
				for ( i=0; i<nLayerCnt; i++ ) {
					pLayer = g_pDoc->GetLayerData(i);
					if ( !pLayer->IsPartOut() )
						pLayer->SetLayerFlags(1);
				}
			}
			else {
				// ｴﾗｰﾒｯｾｰｼﾞ
				AfxMessageBox(IDS_ERR_MAKENC, MB_OK|MB_ICONEXCLAMATION);
				return FALSE;
			}
		}
		else {
			// 終了ｺｰﾄﾞ，ﾌｧｲﾙの出力など
			if ( !MakeNCD_FinalFunc() ) {
#ifdef _DEBUG
				dbgE.printf("MakeNCD_FinalFunc()");
#endif
				return FALSE;
			}
		}
	}

	// 個別出力のﾜｰﾆﾝｸﾞﾒｯｾｰｼﾞ
	CString	strMiss;
	for ( i=0; i<nLayerCnt; i++ ) {
		pLayer = g_pDoc->GetLayerData(i);
		if ( pLayer->GetLayerFlags() == 0 )
			continue;
		if ( !strMiss.IsEmpty() )
			strMiss += gg_szCat;
		strMiss += pLayer->GetStrLayer();
	}
	if ( !strMiss.IsEmpty() ) {
		CString	strMsg;
		strMsg.Format(IDS_ERR_FAILMULTILAYER, strMiss);
		AfxMessageBox(strMsg, MB_OK|MB_ICONINFORMATION);
	}

	return TRUE;
}

void SetStaticOption(void)
{
	g_dZInitial = RoundUp(GetDbl(MKNC_DBL_G92Z));		// ｲﾆｼｬﾙ点
	g_dZG0Stop  = RoundUp(GetDbl(MKNC_DBL_ZG0STOP));	// R点座標
	// AddMoveGdata(Z軸の上昇)で使用
	g_dZReturn = GetNum(MKNC_NUM_ZRETURN) == 0 ? g_dZInitial : g_dZG0Stop;
	// 移動指示ﾚｲﾔのZ軸
	switch ( GetNum(MKNC_NUM_MOVEZ) ) {
	case 1:		// R点
		g_pfnAddMoveZ = &AddMoveZ_R;
		break;
	case 2:		// ｲﾆｼｬﾙ点
		g_pfnAddMoveZ = &AddMoveZ_Initial;
		break;
	default:	// そのまま(他)
		g_pfnAddMoveZ = &AddMoveZ_NotMove;
		break;
	}
	// 移動指示ﾚｲﾔのｶｽﾀﾑｺｰﾄﾞ
	g_pfnAddMoveCust_B = g_pMakeOpt->GetStr(MKNC_STR_CUSTMOVE_B).IsEmpty() ?
		&AddMoveZ_NotMove : &AddMoveCust_B;
	g_pfnAddMoveCust_A = g_pMakeOpt->GetStr(MKNC_STR_CUSTMOVE_A).IsEmpty() ?
		&AddMoveZ_NotMove : &AddMoveCust_A;
	// 加工検索許容差
	CDXFmap::ms_dTolerance = GetFlg(MKNC_FLG_DEEP) ?
		NCMIN : GetDbl(MKNC_DBL_TOLERANCE);
	// 深彫の処理
	if ( GetNum(MKNC_NUM_DEEPAPROCESS) == 0 ) {
		g_pfnDeepHead = &MakeLoopAddDeepHeadAll;
		g_pfnDeepTail = &MakeLoopAddDeepTailAll;
	}
	else {
		g_pfnDeepHead = &MakeLoopAddDeepHead;
		g_pfnDeepTail = &MakeLoopAddDeepTail;
	}

	// CDXFdataの静的変数初期化
	CDXFdata::ms_fXRev = GetFlg(MKNC_FLG_XREV);
	CDXFdata::ms_fYRev = GetFlg(MKNC_FLG_YREV);
	CDXFpoint::ms_pfnOrgDrillTuning = GetNum(MKNC_NUM_OPTIMAIZEDRILL) == 0 ?
		&CDXFpoint::OrgTuning_Seq : &CDXFpoint::OrgTuning_XY;

	// CNCMakeの静的変数初期化
	if ( !g_bData ) {
		// ABS, INC 関係なく G92値で初期化
		for ( int i=0; i<NCXYZ; i++ )
			CNCMake::ms_xyz[i] = RoundUp(GetDbl(MKNC_DBL_G92X+i));
	}
	// 生成ｵﾌﾟｼｮﾝによる静的変数の初期化
	CNCMake::SetStaticOption();
}

BOOL SetStartData(void)
{
	ASSERT(CDXFdata::ms_pData);

	int		i, j, nLayerCnt = g_pDoc->GetLayerCnt(), nDataCnt;
	CDXFdata*	pData;
	CDXFdata*	pMatchData = NULL;
	CDXFdata*	pDataResult = NULL;
	CDXFcircleEx*	pStartCircle = NULL;
	double		dGap, dGapMin = HUGE_VAL;
	CString		strLayer;
	CLayerData*	pLayer;
	CDXFarray	obArray;

	obArray.SetSize(0, g_pDoc->GetDxfLayerDataCnt(DXFSTRLAYER));
	for ( i=0; i<nLayerCnt && IsThread(); i++ ) {
		pLayer = g_pDoc->GetLayerData(i);
		if ( !pLayer->IsCutTarget() || pLayer->GetLayerType()!=DXFSTRLAYER )
			continue;
		// 加工開始位置指示ﾚｲﾔのﾃｷｽﾄ情報原点調整
		nDataCnt = pLayer->GetDxfTextSize();
		for ( j=0; j<nDataCnt && IsThread(); j++ ) {
			pData = pLayer->GetDxfTextData(j);
			pData->OrgTuning(FALSE);
			g_mpDXFstarttext.SetMakePointMap(pData);
		}
		// 加工開始位置指示ﾚｲﾔの原点調整と移動開始位置検索ﾙｰﾌﾟ(OrgTuning)
		nDataCnt = pLayer->GetDxfSize();
		for ( j=0; j<nDataCnt && IsThread(); j++ ) {
			pData = pLayer->GetDxfData(j);
			if ( pData->IsKindOf(RUNTIME_CLASS(CDXFcircleEx)) )
				pStartCircle = (CDXFcircleEx *)pData;
			else {
				dGap = pData->OrgTuning();
				if ( dGap < dGapMin ) {
					dGapMin = dGap;
					pDataResult = pData;
				}
				obArray.Add(pData);
			}
		}
	}
	g_pfnAddStartText = g_mpDXFstarttext.IsEmpty() ?
		&AddText_Exclude : &AddStartText_Target;
	// 加工開始位置指示ﾚｲﾔのｵﾌﾞｼﾞｪｸﾄ仮登録(MakeLoop)
	while ( pDataResult && IsThread() ) {
		g_obStartData.Add(pDataResult);
		// ﾃﾞｰﾀﾏｰｸ
		pDataResult->SetSearchFlg();
		// 次の要素検索(GetNearPoint)
		pMatchData = pDataResult;
		pDataResult = NULL;
		dGapMin = HUGE_VAL;
		for ( i=0; i<obArray.GetSize() && IsThread(); i++ ) {
			pData = obArray[i];
			if ( pData->IsSearchFlg() )
				continue;
			if ( pData->IsMatchObject(pMatchData) ) {
				pDataResult = pData;
				break;
			}
			dGap = pData->GetEdgeGap(pMatchData);
			if ( dGap < dGapMin ) {
				pDataResult = pData;
				if ( sqrt(dGap) < NCMIN )	// 移動指示ﾚｲﾔはNCMIN固定
					break;
				dGapMin = dGap;
			}
		}	// End for loop
	}	// End while loop
	// 加工開始位置に円ﾃﾞｰﾀが使われていたら
	if ( pStartCircle && IsThread() ) {
		// 原点調整
		pStartCircle->OrgTuning(FALSE);	// CDXFcircleEx::OrgTuning()
		// 移動ﾃﾞｰﾀ仮登録
		if ( !pMatchData || pMatchData->GetEndMakePoint()!=pStartCircle->GetEndMakePoint() )
			g_obStartData.Add(pStartCircle);
	}

	return IsThread();
}

void SetGlobalArray_Single(void)
{
	int			i, j, nLayerCnt = g_pDoc->GetLayerCnt(), nDataCnt;
	CDXFdata*	pData;
	CLayerData*	pLayer;

	// 切削対象ﾚｲﾔ(表示ﾚｲﾔ)をｺﾋﾟｰ
	for ( i=0; i<nLayerCnt && IsThread(); i++ ) {
		pLayer = g_pDoc->GetLayerData(i);
		if ( pLayer->IsCutTarget() && pLayer->IsCutType() ) {
			// 切削ﾃﾞｰﾀをｺﾋﾟｰ
			nDataCnt = pLayer->GetDxfSize();
			for ( j=0; j<nDataCnt && IsThread(); j++ )
				SetGlobalArray_Sub(pLayer->GetDxfData(j));
			// ﾃｷｽﾄﾃﾞｰﾀのﾏｯﾌﾟ作成
			nDataCnt = pLayer->GetDxfTextSize();
			for ( j=0; j<nDataCnt && IsThread(); j++ ) {
				pData = pLayer->GetDxfTextData(j);
				pData->OrgTuning(FALSE);
				g_mpDXFtext.SetMakePointMap(pData);
			}
		}
	}
	// その他ﾚｲﾔﾃﾞｰﾀのﾏｯﾌﾟ作成
	SetGlobalMap_Other();
}

void SetGlobalArray_Multi(CLayerData* pLayer)
{
	int			i, nCnt;
	CDXFdata*	pData;

	g_obDXFdata.RemoveAll();
	g_mpDXFdata.RemoveAll();
	g_mpDXFmove.RemoveAll();
	g_mpDXFmovetext.RemoveAll();
	g_mpDXFtext.RemoveAll();
	g_mpDXFcomment.RemoveAll();
	g_obPoint.RemoveAll();
	g_obCircle.RemoveAll();
	g_ltDeepGlist.RemoveAll();
	g_obDrillGroup.RemoveAll();
	g_obDrillAxis.RemoveAll();
	g_mpEuler.RemoveAll();

	// 指定ﾚｲﾔのDXFﾃﾞｰﾀｵﾌﾞｼﾞｪｸﾄをｺﾋﾟｰ
	nCnt = pLayer->GetDxfSize();
	for ( i=0; i<nCnt && IsThread(); i++ )
		SetGlobalArray_Sub(pLayer->GetDxfData(i));
	// ﾃｷｽﾄﾃﾞｰﾀのﾏｯﾌﾟ作成
	nCnt = pLayer->GetDxfTextSize();
	for ( i=0; i<nCnt && IsThread(); i++ ) {
		pData = pLayer->GetDxfTextData(i);
		pData->OrgTuning(FALSE);
		g_mpDXFtext.SetMakePointMap(pData);
	}
	// その他ﾚｲﾔﾃﾞｰﾀのﾏｯﾌﾟ作成
	SetGlobalMap_Other();
}

void SetGlobalArray_Sub(CDXFdata* pData)
{
	// 楕円ﾃﾞｰﾀの変身(長径短径が等しい楕円なら)
	if ( GetFlg(MKNC_FLG_ELLIPSE) && pData->GetType()==DXFELLIPSEDATA &&
			fabs(1.0-((CDXFellipse *)pData)->GetShortMagni()) < EPS ) {	// 倍率1.0
		pData->ChangeMakeType( ((CDXFellipse *)pData)->GetArc() ?
			DXFARCDATA : DXFCIRCLEDATA);	// 円弧か円ﾃﾞｰﾀに変身
	}
	// 各ｵﾌﾞｼﾞｪｸﾄﾀｲﾌﾟごとの処理
	switch ( pData->GetMakeType() ) {
	case DXFPOINTDATA:
		// ここで重複ﾁｪｯｸを行う方が効率がよいが，
		// OrgTuning() 前では座標のﾁｪｯｸができない !!!
		g_obPoint.Add(pData);
		break;
	case DXFCIRCLEDATA:
		// 円ﾃﾞｰﾀの半径が指定値以下なら
		if ( GetFlg(MKNC_FLG_DRILLCIRCLE) &&
				((CDXFcircle *)pData)->GetMakeR() <= GetDbl(MKNC_DBL_DRILLCIRCLE) ) {
			// 穴加工ﾃﾞｰﾀに変身して登録
			pData->ChangeMakeType(DXFPOINTDATA);
			g_obCircle.Add(pData);
			break;
		}
		// through
	default:
		// 「穴加工のみ」以外なら
		if ( GetNum(MKNC_NUM_DRILLPROCESS) != 2 )
			g_obDXFdata.Add(pData);
		break;
	}
}

void SetGlobalMap_Other(void)
{
	// ここから原点調整するｵﾌﾞｼﾞｪｸﾄは距離計算の必要なし
	int			i, j, nType, nLayerCnt = g_pDoc->GetLayerCnt(), nDataCnt;
	CLayerData*	pLayer;
	CDXFdata*	pData;

	for ( i=0; i<nLayerCnt && IsThread(); i++ ) {
		pLayer = g_pDoc->GetLayerData(i);
		if ( !pLayer->IsCutTarget() )
			continue;
		nType = pLayer->GetLayerType();
		if ( nType == DXFMOVLAYER ) {
			// 移動指示ﾚｲﾔのﾏｯﾌﾟ生成
			nDataCnt = pLayer->GetDxfSize();
			for ( j=0; j<nDataCnt && IsThread(); j++ ) {
				pData = pLayer->GetDxfData(j);
				pData->OrgTuning(FALSE);
				g_mpDXFmove.SetMakePointMap(pData);
			}
			// 移動指示ﾚｲﾔﾃｷｽﾄのﾏｯﾌﾟ生成
			nDataCnt = pLayer->GetDxfTextSize();
			for ( j=0; j<nDataCnt && IsThread(); j++ ) {
				pData = pLayer->GetDxfTextData(j);
				pData->OrgTuning(FALSE);
				g_mpDXFmovetext.SetMakePointMap(pData);
			}
		}
		else if ( nType == DXFCOMLAYER ) {
			// ｺﾒﾝﾄﾚｲﾔのﾏｯﾌﾟ生成
			nDataCnt = pLayer->GetDxfTextSize();
			for ( j=0; j<nDataCnt && IsThread(); j++ ) {
				pData = pLayer->GetDxfTextData(j);
				pData->OrgTuning(FALSE);
				g_mpDXFcomment.SetMakePointMap(pData);
			}
		}
	}

#ifdef _DEBUGOLD
	CDumpContext	dc;
	dc.SetDepth(1);
	g_mpDXFmove.Dump(dc);
#endif

	// 呼ぶべき関数の初期化
	g_pfnGetMatchPointMove = g_mpDXFmove.IsEmpty() ?
		&GetMatchPointMove_Exclude : &GetMatchPointMove_Target;
	g_pfnAddMoveText = g_mpDXFmovetext.IsEmpty() ?
		&AddText_Exclude : &AddMoveText_Target;
	g_pfnAddCutterText = g_mpDXFtext.IsEmpty() ?
		&AddText_Exclude : &AddCutterText_Target;
	g_pfnAddCommentText = g_mpDXFcomment.IsEmpty() ?
		&AddText_Exclude : &AddCommentText_Target;
}

BOOL MakeNCD_FinalFunc(LPCTSTR lpszFileName/*=NULL*/)
{
	// 最終ｲﾆｼｬﾙZ座標への復帰
	if ( GetNum(MKNC_NUM_ZRETURN)!=0 || CDXFdata::ms_pData->GetMakeType()!=DXFPOINTDATA )
		AddMoveGdata(0, g_dZInitial, -1);
	// Gｺｰﾄﾞﾌｯﾀﾞ(終了ｺｰﾄﾞ)
	AddCustomCode(g_pMakeOpt->GetStr(MKNC_STR_FOOTER), NULL);
	// ﾌｧｲﾙ出力ﾌｪｰｽﾞ
	return OutputNCcode(lpszFileName);
}

BOOL OutputNCcode(LPCTSTR lpszFileName)
{
	CString	strPath, strFile,
			strNCFile(lpszFileName ? lpszFileName : g_pDoc->GetNCFileName());
	Path_Name_From_FullPath(strNCFile, strPath, strFile);
	SendFaseMessage(g_obMakeGdata.GetSize(), g_szFinal, strFile);
	try {
		CStdioFile	fp(strNCFile,
				CFile::modeCreate | CFile::modeWrite | CFile::shareExclusive | CFile::typeText);
		for ( int i=0; i<g_obMakeGdata.GetSize() && IsThread(); i++ ) {
			g_obMakeGdata[i]->WriteGcode(fp);
			SendProgressPos(i);
		}
	}
	catch (	CFileException* e ) {
		strFile.Format(IDS_ERR_DATAWRITE, strNCFile);
		AfxMessageBox(strFile, MB_OK|MB_ICONSTOP);
		e->Delete();
		return FALSE;
	}

	g_pParent->m_ctReadProgress.SetPos(g_obMakeGdata.GetSize());
	return IsThread();
}

//////////////////////////////////////////////////////////////////////
// NC生成ﾒｲﾝｽﾚｯﾄﾞ
//////////////////////////////////////////////////////////////////////

// NC生成処理
BOOL MakeNCD_MainThread(CString& strLayer)
{
#ifdef _DEBUG
	CMagaDbg	dbg("MakeNCD_MainThread()\nStart", DBG_MAGENTA);
#endif

	// ﾌﾟﾛｸﾞﾚｽﾊﾞｰ初期化
	SetSearchRange();
	g_nFase = 1;

	// 半径で並べ替え
	if ( GetNum(MKNC_NUM_DRILLSORT) == 0 )
		g_obCircle.Sort(CircleSizeCompareFunc1);	// 昇順
	else
		g_obCircle.Sort(CircleSizeCompareFunc2);	// 降順
	// 加工開始位置指示ﾃﾞｰﾀのｾｯﾄ
	if ( !g_bData && !g_obStartData.IsEmpty() )
		CDXFdata::ms_pData = g_obStartData.GetTail();

	// ﾒｲﾝ分岐
	switch ( GetNum(MKNC_NUM_DRILLPROCESS) ) {
	case 0:		// 先に穴加工
		// 穴加工ﾃﾞｰﾀの準備と最短開始位置のﾃﾞｰﾀ解析
		if ( !CallMakeDrill_Sub(strLayer) )
			return FALSE;
		// 切削ﾃﾞｰﾀの準備と最短開始位置のﾃﾞｰﾀ解析
		// 穴加工ﾃﾞｰﾀの最後に近いﾃﾞｰﾀ検索
		if ( !CallMakeLoop(MAKECUTTER, strLayer) )
			return FALSE;
		break;

	case 1:		// 後で穴加工
		// 切削ﾃﾞｰﾀの準備と最短開始位置のﾃﾞｰﾀ解析
		if ( !CallMakeLoop(MAKECUTTER, strLayer) )
			return FALSE;
		// through
	case 2:		// 穴加工のみ
		// 穴加工ﾃﾞｰﾀの準備と最短開始位置のﾃﾞｰﾀ解析
		// 切削ﾃﾞｰﾀの最後に近いﾃﾞｰﾀ検索
		if ( !CallMakeDrill_Sub(strLayer) )
			return FALSE;
		break;

	default:
		return FALSE;
	}

	// 切削ﾃﾞｰﾀ終了後の移動指示ﾚｲﾔﾁｪｯｸ
	return MakeLoopAddLastMove();
}

//////////////////////////////////////////////////////////////////////
// NC生成ﾒｲﾝｽﾚｯﾄﾞ補助関数群
//////////////////////////////////////////////////////////////////////

BOOL CallMakeLoop(ENMAKETYPE enMake, CString& strLayer)
{
	CDXFdata*	pData;
	CString		strBuf;
	BOOL		bMatch;

	// ﾌｪｰｽﾞ1 ( OrgTuning_XXX)
	switch( enMake ) {
	case MAKECUTTER:
		tie(pData, bMatch) = OrgTuningCutter();
		break;
	case MAKEDRILLPOINT:
		tie(pData, bMatch) = OrgTuningDrillPoint();
		break;
	case MAKEDRILLCIRCLE:
		pData = OrgTuningDrillCircle();
		bMatch = FALSE;
		break;
	}

	if ( !IsThread() )
		return FALSE;
	if ( !pData )
		return TRUE;

	if ( !g_bData ) {
		g_bData = TRUE;
		// Gｺｰﾄﾞﾍｯﾀﾞ(開始ｺｰﾄﾞ)
		AddCustomCode(g_pMakeOpt->GetStr(MKNC_STR_HEADER), pData);
		// 加工開始位置指示ﾚｲﾔ処理
		AddMakeStart();
	}
	else {
		// 穴加工以外で前の切削終了位置と次の切削開始位置が違うなら
		if ( CDXFdata::ms_pData->GetMakeType() != DXFPOINTDATA &&
				CDXFdata::ms_pData->GetEndMakePoint() != pData->GetStartMakePoint() ) {
			// Z軸の上昇
			AddMoveGdata(0, g_dZReturn, -1.0);
		}
	}
	// ﾚｲﾔごとのｺﾒﾝﾄ
	if ( !strLayer.IsEmpty() && GetFlg(MKNC_FLG_LAYERCOMMENT) ) {
		strBuf.Format(IDS_MAKENCD_LAYERBREAK, strLayer);
		AddMakeGdata(strBuf);
		// 同じｺﾒﾝﾄを入れないようにﾚｲﾔ名をｸﾘｱ
		strLayer.Empty();
	}
	// 回転数
	strBuf = CNCMake::MakeSpindle(pData->GetMakeType());
	if ( !strBuf.IsEmpty() )
		AddMakeGdata(strBuf);

	// ﾌｪｰｽﾞ2
	SendFaseMessage();

	// NC生成ﾙｰﾌﾟ
	BOOL	bResult = FALSE;
	switch ( enMake ) {
	case MAKECUTTER:
		bResult = MakeLoopEuler(bMatch, pData);
		break;
	case MAKEDRILLPOINT:
		bResult = MakeLoopDrillPoint(bMatch, pData);
		break;
	case MAKEDRILLCIRCLE:
		if ( bResult=MakeLoopDrillCircle() ) {
			// 円ﾃﾞｰﾀの終了ｺﾒﾝﾄ
			if ( GetFlg(MKNC_FLG_DRILLBREAK) ) {
				VERIFY(strBuf.LoadString(IDS_MAKENCD_CIRCLEEND));
				AddMakeGdata(strBuf);
			}
		}
		break;
	}

	return bResult;
}

BOOL CallMakeDrill_Sub(CString& strLayer)
{
	if ( GetFlg(MKNC_FLG_DRILLCIRCLE) ) {
		// 円ﾃﾞｰﾀも穴加工ﾃﾞｰﾀとする場合
		switch ( GetNum(MKNC_NUM_DRILLCIRCLEPROCESS) ) {
		case 1:	// 円を先に
			if ( !CallMakeLoop(MAKEDRILLCIRCLE, strLayer) )
				return FALSE;
			if ( !CallMakeLoop(MAKEDRILLPOINT, strLayer) )
				return FALSE;
			break;

		case 2:	// 円を後で
			if ( !CallMakeLoop(MAKEDRILLPOINT, strLayer) )
				return FALSE;
			// through
		case 0:	// 実点無視
			if ( !CallMakeLoop(MAKEDRILLCIRCLE, strLayer) )
				return FALSE;
			break;
		}
	}
	else {
		// 実点ﾃﾞｰﾀだけ穴加工
		if ( !CallMakeLoop(MAKEDRILLPOINT, strLayer) )
			return FALSE;
	}

	// 固定ｻｲｸﾙｷｬﾝｾﾙ
	AddMakeGdataCycleCancel();

	return TRUE;
}

tuple<CDXFdata*, BOOL> OrgTuningCutter(void)
{
#ifdef _DEBUG
	CMagaDbg	dbg("OrgTuningCutter()", DBG_GREEN);
#endif
	if ( g_obDXFdata.IsEmpty() )
		return NULL;

	// ﾌｪｰｽﾞ1
	SendFaseMessage(g_obDXFdata.GetSize());

	// 原点調整と切削開始ﾎﾟｲﾝﾄ検索ﾙｰﾌﾟ
	BOOL	bMatch = FALSE;
	double	orgGap, orgGapMin = HUGE_VAL;	// 原点までの距離
	CDXFdata*	pDataResult = NULL;
	CDXFdata*	pData;
#ifdef _DEBUG
	int	nResult;
#endif

	for ( int i=0; i<g_obDXFdata.GetSize() && IsThread(); i++ ) {
		pData = g_obDXFdata[i];
		// 原点調整と距離計算 + NC生成ﾌﾗｸﾞの初期化
		orgGap = pData->OrgTuning();
		// 原点に一番近いﾎﾟｲﾝﾄを探す
		if ( orgGap < orgGapMin ) {
			orgGapMin = orgGap;
			pDataResult = pData;
#ifdef _DEBUG
			nResult = i;
#endif
			if ( sqrt(orgGap) < CDXFmap::ms_dTolerance )
				bMatch = TRUE;
		}
		// 座標ﾏｯﾌﾟに登録
		g_mpDXFdata.SetMakePointMap(pData);
		//
		SendProgressPos(i);
	}
#ifdef _DEBUG
	dbg.printf("FirstPoint %d Gap=%f", nResult, orgGapMin);
#endif
	g_pParent->m_ctReadProgress.SetPos(g_obDXFdata.GetSize());

	return make_tuple(pDataResult, bMatch);
}

tuple<CDXFdata*, BOOL> OrgTuningDrillPoint(void)
{
#ifdef _DEBUG
	CMagaDbg	dbg("OrgTuningDrillPoint()", DBG_GREEN);
#endif
	if ( g_obPoint.IsEmpty() )
		return NULL;

	// ﾌｪｰｽﾞ1
	SendFaseMessage(g_obPoint.GetSize());

	// 最適化に伴う配列のｺﾋﾟｰ先
	CDXFsort& obDrill = GetNum(MKNC_NUM_OPTIMAIZEDRILL) == 0 ? g_obDrillAxis : g_obDrillGroup;
	obDrill.SetSize(0, g_obPoint.GetSize());

	// 原点調整と切削開始ﾎﾟｲﾝﾄ検索ﾙｰﾌﾟ
	int		i, j;
	BOOL	bMatch = FALSE, bCalc = GetNum(MKNC_NUM_OPTIMAIZEDRILL) == 0 ? TRUE : FALSE;
	double	orgGap, orgGapMin = HUGE_VAL;	// 原点までの距離
	CDXFdata*	pDataResult = NULL;
	CDXFdata*	pData;
			// 基準軸が設定されていると「原点に一番近いﾎﾟｲﾝﾄを探す」はムダな処理
#ifdef _DEBUG
	int	nResult;
#endif

	// 穴加工ｵﾌﾞｼﾞｪｸﾄを g_obDrillGroup にｺﾋﾟｰ
	for ( i=0; i<g_obPoint.GetSize() && IsThread(); i++ ) {
		pData = g_obPoint[i];
		// 原点調整と距離計算 + NC生成ﾌﾗｸﾞの初期化
		orgGap = pData->OrgTuning(bCalc);
		// 重複座標のﾁｪｯｸ
		if ( GetFlg(MKNC_FLG_DRILLMATCH) ) {
			for ( j=0; j<obDrill.GetSize() && IsThread(); j++ ) {
				if ( obDrill[j]->IsMatchObject(pData) ||
						sqrt(obDrill[j]->GetEdgeGap(pData, FALSE)) < GetDbl(MKNC_DBL_TOLERANCE) ) {
					pData->SetMakeFlg();
					break;
				}
			}
		}
		// 原点に一番近いﾎﾟｲﾝﾄを探す
		if ( !pData->IsMakeFlg() ) {
			obDrill.Add(pData);
			if ( orgGap < orgGapMin ) {
				orgGapMin = orgGap;
				pDataResult = pData;
#ifdef _DEBUG
				nResult = i;
#endif
				if ( sqrt(orgGap) < GetDbl(MKNC_DBL_TOLERANCE) )
					bMatch = TRUE;
			}
		}
		SendProgressPos(i);
	}
#ifdef _DEBUG
	dbg.printf("FirstPoint %d Gap=%f", nResult, orgGapMin);
#endif

	g_pParent->m_ctReadProgress.SetPos(g_obPoint.GetSize());

	return make_tuple(pDataResult, bMatch);
}

CDXFdata* OrgTuningDrillCircle(void)
{
#ifdef _DEBUG
	CMagaDbg	dbg("OrgTuningDrillCircle()", DBG_GREEN);
#endif
	if ( g_obCircle.IsEmpty() )
		return NULL;

	// ﾌｪｰｽﾞ1
	SendFaseMessage(g_obCircle.GetSize());

	// 必ず並べ替えされる(半径ごとに処理)ので原点調整のみ
	for ( int i=0; i<g_obCircle.GetSize() && IsThread(); i++ ) {
		g_obCircle[i]->OrgTuning(FALSE);
		SendProgressPos(i);
	}
	g_pParent->m_ctReadProgress.SetPos(g_obCircle.GetSize());

	// ﾀﾞﾐｰﾃﾞｰﾀを返す
	return g_obCircle[0];
}

//////////////////////////////////////////////////////////////////////

BOOL MakeLoopEuler(BOOL bMatch, CDXFdata* pData)
{
	// ---------------------------------------------------
	// MakeLoopEuler() の処理全般は，精度の高い座標を渡す
	// ---------------------------------------------------
	CDXFdata*	pDataMove;
	BOOL		bMove = FALSE, bCust = TRUE;
	int			i, nCnt, nPos = 0, nSetPos = 64;
	CPointD		pt;

	// OrgTuning()で現在位置と同一座標が見つからなかった
	if ( !bMatch ) {
		pDataMove = MakeLoopAddFirstMove(MAKECUTTER);
		if ( pDataMove )
			pData = pDataMove;
	}

	// GetNearPoint() の結果が NULL になるまで
	while ( pData && IsThread() ) {
		// CMapｸﾘｱｽﾚｯﾄﾞの終了待ち
		g_pmClearMap.evEnd.Lock();
		g_pmClearMap.evEnd.ResetEvent();
		// この pData を基点に
		g_mpEuler.SetMakePointMap(pData);
		pData->SetSearchFlg();
		// pData の始点・終点で一筆書き探索
		for ( i=0; i<pData->GetPointNumber(); i++ ) {
			pt = pData->GetTunPoint(i);
			if ( pt != HUGE_VAL ) {
				if ( !MakeLoopEulerSearch(pt) )
					return FALSE;
			}
		}
		// CMapｵﾌﾞｼﾞｪｸﾄのNC生成
		if ( (nCnt=MakeLoopEulerAdd()) < 0 )
			return FALSE;
		//
		nPos += nCnt;
		if ( nSetPos < nPos ) {
			g_pParent->m_ctReadProgress.SetPos(nPos);
			while ( nSetPos < nPos )
				nSetPos += nSetPos;
		}
		// 次の切削ﾎﾟｲﾝﾄ検索
		pData = CDXFdata::ms_pData;
		while ( IsThread() ) {
			// この要素に一番近い要素
			tie(pData, bMatch) = GetNearPointCutter(pData);
			if ( !pData )
				break;	// ﾒｲﾝﾙｰﾌﾟ終了条件
			// 等しい座標なし(==Z軸の移動が必要)
			if ( !bMatch ) {
				// 移動指示ﾚｲﾔのﾁｪｯｸ
				pDataMove = CDXFdata::ms_pData;
				if ( (*g_pfnGetMatchPointMove)(pDataMove) ) {	// 前回ｵﾌﾞｼﾞｪｸﾄで探索
					pData = pDataMove;
					if ( GetFlg(MKNC_FLG_DEEP) && GetNum(MKNC_NUM_DEEPAPROCESS)==0 ) {
						g_ltDeepGlist.AddTail(pData);
						pData->SetMakeFlg();
					}
					else {
						bMove = TRUE;
						// 移動ﾃﾞｰﾀ前のｶｽﾀﾑｺｰﾄﾞ挿入
						if ( bCust ) {
							(*g_pfnAddMoveCust_B)();
							bCust = FALSE;	// 連続して入らないように
						}
						// 指示されたZ位置で移動
						(*g_pfnAddMoveZ)();
						// 移動ﾃﾞｰﾀの生成
						AddMakeMove(pData);
						// 移動ﾃﾞｰﾀを待避
						CDXFdata::ms_pData = pData;
					}
					continue;	// 再探索
				}
			}
			// GetMatchPoint_Move() で continue する以外は
			break;
		}
		bCust = TRUE;
		// 移動ﾃﾞｰﾀ後のｶｽﾀﾑｺｰﾄﾞ挿入
		if ( bMove && IsThread() ) {
			// 移動ﾃﾞｰﾀの終点でﾃｷｽﾄﾃﾞｰﾀの生成
			pt = CDXFdata::ms_pData->GetEndMakePoint();
			AddMoveTextIntegrated(pt);
			// ｶｽﾀﾑｺｰﾄﾞ
			(*g_pfnAddMoveCust_A)();
			bMove = FALSE;
		}
		// Z軸の上昇
		if ( pData && !GetFlg(MKNC_FLG_DEEP) && GetNum(MKNC_NUM_TOLERANCE)==0 )
			AddMoveGdataZup();
	} // End of while
	g_pParent->m_ctReadProgress.SetPos(nPos);

	// 全体深彫の後処理
	if ( GetFlg(MKNC_FLG_DEEP) && GetNum(MKNC_NUM_DEEPAPROCESS)==0 ) {
		if ( !MakeLoopDeepAdd() )
			return FALSE;
	}

	return IsThread();
}

BOOL MakeLoopEulerSearch(CPointD& pt)
{
	int			i, j;
	BOOL		bResult = TRUE;
	CPointD		ptSrc;
	CDXFarray*	pobArray;
	CDXFdata*	pData;

	// 座標をｷｰにﾏｯﾌﾟ検索
	if ( g_mpDXFdata.Lookup(pt, pobArray) ) {
		for ( i=0; i<pobArray->GetSize() && bResult && IsThread(); i++ ) {
			pData = pobArray->GetAt(i);
			if ( !pData->IsMakeFlg() && !pData->IsSearchFlg() ) {
				g_mpEuler.SetMakePointMap(pData);
				pData->SetSearchFlg();
				// ｵﾌﾞｼﾞｪｸﾄの端点をさらに検索
				for ( j=0; j<pData->GetPointNumber() && IsThread(); j++ ) {
					ptSrc = pData->GetTunPoint(j);
					if ( ptSrc != HUGE_VAL && pt != ptSrc ) {
						if ( !MakeLoopEulerSearch(ptSrc) ) {
							bResult = FALSE;
							break;
						}
					}
				}
			}
		}
	}
	if ( !IsThread() )
		bResult = FALSE;
	return bResult;
}

int MakeLoopEulerAdd(void)
{
#ifdef _DEBUG
	CMagaDbg	dbg("MakeLoopEulerAdd()", DBG_MAGENTA);
#endif
	int			i, nObCnt, nOddCnt = 0;
	BOOL		bEuler = FALSE;	// 一筆書き要件を満たしているか
	double		dGap, dGapMin = HUGE_VAL, dGapMin2 = HUGE_VAL;
	POSITION	pos;
	CPointD		pt, ptStart, ptStart2,
				ptKey( CDXFdata::ms_pData->GetEndCutterPoint() );
	CDXFdata*	pData;
	CDXFarray*	pArray;
	CDXFarray*	pStartArray = NULL;
	CDXFarray*	pStartArray2;
	CDXFlist	ltEuler(1024);		// 一筆書き順

	// --- ﾃﾞｰﾀ準備
#ifdef _DEBUG
	dbg.printf("g_mpEuler.GetCount()=%d", g_mpEuler.GetCount());
	for ( pos = g_mpEuler.GetStartPosition(); pos; ) {
		g_mpEuler.GetNextAssoc(pos, pt, pArray);
		dbg.printf("pt.x=%f pt.y=%f", pt.x, pt.y);
		for ( i=0; i<pArray->GetSize(); i++ ) {
			pData = pArray->GetAt(i);
			dbg.printf("Type=%d", pData->GetMakeType());
		}
	}
#endif
	// ｵﾌﾞｼﾞｪｸﾄ登録数の奇数を探す + ｻｰﾁﾌﾗｸﾞのｸﾘｱ
	for ( pos = g_mpEuler.GetStartPosition(); pos && IsThread(); ) {
		g_mpEuler.GetNextAssoc(pos, pt, pArray);
		// 座標ｷｰに対する登録ｵﾌﾞｼﾞｪｸﾄ数が奇数の近接ｵﾌﾞｼﾞｪｸﾄを検索
		// (円ﾃﾞｰﾀはｶｳﾝﾄしないため obArray->GetSize() が使えない)
		for ( i=0, nObCnt=0; i<pArray->GetSize() && IsThread(); i++ ) {
			pData = pArray->GetAt(i);
			pData->ClearSearchFlg();
			if ( !pData->IsStartEqEnd() )	// 始点に戻るﾃﾞｰﾀ以外
				nObCnt++;
		}
		dGap = GAPCALC(ptKey - pt);
		if ( nObCnt & 0x01 ) {
			nOddCnt++;
			// 奇数を優先的に検索
			if ( dGap < dGapMin ) {
				dGapMin = dGap;
				pStartArray = pArray;
				ptStart = pt;
			}
		}
		else {
			// 偶数の場合でも一番近い座標を確保
			if ( dGap < dGapMin2 ) {
				dGapMin2 = dGap;
				pStartArray2 = pArray;
				ptStart2 = pt;
			}
		}
	}
	if ( !pStartArray ) {		// nOddCnt == 0
		bEuler = TRUE;		// 一筆書き要件を満たしている
		// 奇数座標がない場合は偶数座標の最も近いｵﾌﾞｼﾞｪｸﾄ配列を使う
		pStartArray = pStartArray2;
		ptStart = ptStart2;
	}
	else if ( nOddCnt == 2 ) {	// 奇点が２個
		bEuler = TRUE;		// 一筆書き要件を満たしている
	}
#ifdef _DEBUG
	dbg.printf("FirstPoint x=%f y=%f", ptStart.x, ptStart.y);
#endif
	if ( !IsThread() )
		return -1;

	// --- 一筆書きの生成(再帰呼び出しによる木構造解析)
	ASSERT( pStartArray );
	ASSERT( !pStartArray->IsEmpty() );
	MakeLoopEulerAdd_with_one_stroke(ptStart, pStartArray, ltEuler, bEuler);

	// --- 切削ﾃﾞｰﾀ生成
	ASSERT( !ltEuler.IsEmpty() );
	if ( GetFlg(MKNC_FLG_DEEP) ) {
		// 切削ﾃﾞｰﾀ生成
		for ( pos = ltEuler.GetHeadPosition(); pos; ) {
			pData = ltEuler.GetNext(pos);
			pData->SetMakeFlg();
		}
		g_ltDeepGlist.AddTail(&ltEuler);
		// 深彫が全体か否か
		if ( GetNum(MKNC_NUM_DEEPAPROCESS) == 0 ) {
			g_ltDeepGlist.AddTail((CDXFdata *)NULL);	// Z軸移動のﾏｰｶｰ
			// 最後に生成したﾃﾞｰﾀを待避
			CDXFdata::ms_pData = pData;
		}
		else {
			if ( !MakeLoopDeepAdd() )
				return -1;
		}
	}
	else {
		// 切削ﾃﾞｰﾀまでの移動
		pData = ltEuler.GetHead();
		if ( GetNum(MKNC_NUM_TOLERANCE) == 0 )
			AddMoveGdataG0(pData);
		else
			AddMoveGdataG1(pData);
		// 切削ﾃﾞｰﾀ生成
		for ( pos = ltEuler.GetHeadPosition(); pos; ) {
			pData = ltEuler.GetNext(pos);
			AddMakeGdata(pData);
		}
		// 最後に生成したﾃﾞｰﾀを待避
		CDXFdata::ms_pData = pData;
	}

	// 一筆書き要件を満たしていないときだけ
	if ( !bEuler ) {
		// 一筆書きに漏れたｵﾌﾞｼﾞｪｸﾄのｻｰﾁﾌﾗｸﾞを初期化
		for ( pos = g_mpEuler.GetStartPosition(); pos && IsThread(); ) {
			g_mpEuler.GetNextAssoc(pos, pt, pArray);
			for ( i=0; i<pArray->GetSize() && IsThread(); i++ ) {
				pData = pArray->GetAt(i);
				if ( !pData->IsMakeFlg() )
					pData->ClearSearchFlg();
			}
		}
	}
	// CMapｵﾌﾞｼﾞｪｸﾄｸﾘｱ
	g_pmClearMap.evStart.SetEvent();

	return ltEuler.GetCount();
}

BOOL MakeLoopEulerAdd_with_one_stroke
	(CPointD& pt, CDXFarray* pArray, CDXFlist& ltEuler, BOOL bEuler)
{
	int			i;
	CDXFdata*	pData;
	CDXFarray*	pNextArray;
	CPointD		ptNext;
	POSITION	pos, posTail = ltEuler.GetTailPosition();	// この時点での仮登録ﾘｽﾄの最後

	// まずこの座標配列の円(に準拠する)ﾃﾞｰﾀを仮登録
	for ( i=0; i<pArray->GetSize() && IsThread(); i++ ) {
		pData = pArray->GetAt(i);
		if ( !pData->IsSearchFlg() && pData->IsStartEqEnd() ) {
			pData->GetEdgeGap(pt);	// ｵﾌﾞｼﾞｪｸﾄのpt値をptStartの近い方に入れ替え
			ltEuler.AddTail( pData );
			pData->SetSearchFlg();
		}
	}

	// 円以外のﾃﾞｰﾀで木構造の次を検索
	for ( i=0; i<pArray->GetSize() && IsThread(); i++ ) {
		pData = pArray->GetAt(i);
		if ( !pData->IsSearchFlg() ) {
			pData->GetEdgeGap(pt);
			ltEuler.AddTail(pData);
			pData->SetSearchFlg();
			ptNext = pData->GetEndCutterPoint();
			VERIFY( g_mpEuler.Lookup(ptNext, pNextArray) );	// Lookup()で失敗することはない
			// 次の座標配列を検索
			if ( MakeLoopEulerAdd_with_one_stroke(ptNext, pNextArray, ltEuler, bEuler) )
				return TRUE;	// 再帰を抜ける
			// この座標配列のｉ番目のノードではなかったので
			// 今登録した仮登録ﾙｰﾄは解除
			ltEuler.RemoveTail();
			pData->ClearSearchFlg();
		}
	}
	// 検索対象が無くなった
	if ( !bEuler || !IsThread() ) {	// 一筆書きの要件を満たしていない
		// ホントは最長経路を探す
		return TRUE;		// 検索終了
	}
	else {
		// 全件仮登録出来たかどうか
		for ( pos = g_mpEuler.GetStartPosition(); pos && IsThread(); ) {
			g_mpEuler.GetNextAssoc(pos, pt, pArray);
			for ( i=pArray->GetSize(); --i>=0 && IsThread(); ) {
				if ( !pArray->GetAt(i)->IsSearchFlg() ) {
					pos = NULL;
					break;
				}
			}
		}
		if ( i < 0 )
			return TRUE;	// 全件終了
	}
	// この座標配列の検索が終了したので木構造の上位へ移動．
	// 円ﾃﾞｰﾀを含む全ての仮登録ﾘｽﾄを削除
	ltEuler.GetNext(posTail);
	for ( pos=posTail; pos && IsThread(); pos=posTail) {
		pData = ltEuler.GetNext(posTail);	// 先に次の要素を取得
		pData->ClearSearchFlg();			// ﾌﾗｸﾞを消して
		ltEuler.RemoveAt(pos);				// 要素削除
	}

	return FALSE;
}

BOOL MakeLoopDeepAdd(void)
{
	int			nCnt;
	BOOL		bAction = TRUE;		// まずは正方向
	double		dZCut = g_dZCut;	// g_dZCutﾊﾞｯｸｱｯﾌﾟ
	CDXFdata*	pData;
	POSITION	pos;

	if ( g_ltDeepGlist.IsEmpty() )
		return TRUE;

	// 最後のZ軸移動ﾏｰｶは削除
	if ( g_ltDeepGlist.GetTail() == NULL )
		g_ltDeepGlist.RemoveTail();
	// 回転数
	CString	strSpindle( CNCMake::MakeSpindle(DXFLINEDATA) );
	if ( !strSpindle.IsEmpty() )
		AddMakeGdata(strSpindle);
	// 切削ﾃﾞｰﾀまでの移動
	ASSERT( g_ltDeepGlist.GetHead() );
	AddMoveGdataG0( g_ltDeepGlist.GetHead() );

	// 深彫が「全体」の場合，ﾄｰﾀﾙ件数*深彫ｽﾃｯﾌﾟでﾌﾟﾛｸﾞﾚｽｺﾝﾄﾛｰﾙの再設定
	if ( GetNum(MKNC_NUM_DEEPAPROCESS) == 0 ) {
		// ﾄｰﾀﾙ件数(深彫ｽﾃｯﾌﾟｶｳﾝﾄは切り上げ)
		nCnt = (int)ceil(fabs((g_dDeep - g_dZCut) / GetDbl(MKNC_DBL_ZSTEP)))
			* g_ltDeepGlist.GetCount();
		SendFaseMessage( nCnt );
	}

	nCnt = 0;
	// 深彫最終位置まで仮登録ﾃﾞｰﾀのNC生成
	// g_dZCut > g_dDeep での条件では数値誤差が発生したときﾙｰﾌﾟ脱出しないため
	// g_dZCut - g_dDeep > EPS とした
	while ( g_dZCut - g_dDeep > EPS && IsThread() ) {
		pData = bAction ? (*g_pfnDeepHead)(FALSE) : (*g_pfnDeepTail)(FALSE);
		if ( !IsThread() )
			return FALSE;
		CDXFdata::ms_pData = pData;
		// ｱｸｼｮﾝの切り替え(往復切削のみ)
		if ( GetNum(MKNC_NUM_DEEPCPROCESS) == 0 ) {
			bAction = !bAction;
			// 各ｵﾌﾞｼﾞｪｸﾄの始点終点を入れ替え
			for ( pos = g_ltDeepGlist.GetHeadPosition(); pos && IsThread(); ) {
				pData = g_ltDeepGlist.GetNext(pos);
				if ( pData )
					pData->ReversePt();
			}
		}
		// Z軸の下降
		g_dZCut += GetDbl(MKNC_DBL_ZSTEP);
		if ( g_dZCut - g_dDeep > EPS ) {
			// 一方通行切削のﾁｪｯｸ
			MakeLoopDeepZDown();
		}
		// ﾌﾟﾛｸﾞﾚｽﾊﾞｰの更新
		if ( GetNum(MKNC_NUM_DEEPAPROCESS) == 0 )
			g_pParent->m_ctReadProgress.SetPos(++nCnt * g_ltDeepGlist.GetCount());
	}
	if ( !IsThread() )
		return FALSE;

	// 最終Z値
	g_dZCut = g_dDeep;

	// 最終仕上げｵﾌﾟｼｮﾝの適用
	BOOL	bFinish;
	if ( GetFlg(MKNC_FLG_DEEPFINISH) ) {
		bFinish = TRUE;
		// 通常回転数と仕上げ回転数が違うとき
		if ( GetNum(MKNC_NUM_SPINDLE) != GetNum(MKNC_NUM_DEEPSPINDLE) ) {
			// Z軸上昇
			MakeLoopDeepZUp();
			// 仕上げ用回転数に変更
			AddMakeGdata( CNCMake::MakeSpindle(DXFLINEDATA, TRUE) );
			// ｵﾌﾞｼﾞｪｸﾄ切削位置へ移動
			pData = bAction ? g_ltDeepGlist.GetHead() : g_ltDeepGlist.GetTail();
			AddMoveGdataG0(pData);
		}
		else {
			// 仕上げ面へのZ軸下降
			MakeLoopDeepZDown();
		}
	}
	else {
		bFinish = FALSE;
		// 仕上げ面へのZ軸下降
		MakeLoopDeepZDown();
	}

	// 仕上げ面のﾃﾞｰﾀ生成
	CDXFdata::ms_pData = bAction ? (*g_pfnDeepHead)(bFinish) : (*g_pfnDeepTail)(bFinish);

	// 深彫切削におけるZ軸の上昇
	MakeLoopDeepZUp();

	// 後始末
	g_ltDeepGlist.RemoveAll();
	g_dZCut = dZCut;

	return IsThread();
}

CDXFdata* MakeLoopAddDeepHead(BOOL bDeep)
{
	CDXFdata*	pData;
	for ( POSITION pos = g_ltDeepGlist.GetHeadPosition(); pos && IsThread(); ) {
		pData = g_ltDeepGlist.GetNext(pos);
		AddMakeGdataDeep(pData, bDeep);
	}
	return pData;
}

CDXFdata* MakeLoopAddDeepTail(BOOL bDeep)
{
	CDXFdata*	pData;
	for ( POSITION pos = g_ltDeepGlist.GetTailPosition(); pos && IsThread(); ) {
		pData = g_ltDeepGlist.GetPrev(pos);
		AddMakeGdataDeep(pData, bDeep);
	}
	return pData;
}

CDXFdata* MakeLoopAddDeepHeadAll(BOOL bDeep)
{
	BOOL		bMoveZ = FALSE;
	CDXFdata*	pData;
	CDXFdata*	pDataResult = NULL;
	for ( POSITION pos = g_ltDeepGlist.GetHeadPosition(); pos && IsThread(); ) {
		pData = g_ltDeepGlist.GetNext(pos);
		if ( pData ) {
			// １つ前のｵﾌﾞｼﾞｪｸﾄが移動ﾃﾞｰﾀなら
			if ( pDataResult && !pDataResult->GetLayerData()->IsCutType() )
				AddMoveGdataZdown();	// Z軸の下降
			MakeLoopAddDeepZProc(bMoveZ, bDeep, pData);
			bMoveZ = FALSE;
			pDataResult = pData;
		}
		else
			bMoveZ = TRUE;
	}
	ASSERT(pDataResult);
	return pDataResult;
}

CDXFdata* MakeLoopAddDeepTailAll(BOOL bDeep)
{
	BOOL		bMoveZ = FALSE;
	CDXFdata*	pData;
	CDXFdata*	pDataResult = NULL;
	for ( POSITION pos = g_ltDeepGlist.GetTailPosition(); pos && IsThread(); ) {
		pData = g_ltDeepGlist.GetPrev(pos);
		if ( pData ) {
/*
			// 逆回りの場合は NULLﾌﾞﾚｲｸ が有効に働くので，以下の処理は不要
			if ( pDataResult && !pDataResult->GetLayerData()->IsCutType() )
				AddMoveGdataZdown();
*/
			MakeLoopAddDeepZProc(bMoveZ, bDeep, pData);
			bMoveZ = FALSE;
			pDataResult = pData;
		}
		else
			bMoveZ = TRUE;
	}
	ASSERT(pDataResult);
	return pDataResult;
}

void MakeLoopAddDeepZProc(BOOL bMoveZ, BOOL bDeep, CDXFdata* pData)
{
	if ( pData->GetLayerData()->IsCutType() ) {
		if ( bMoveZ ) {		// Z軸移動ﾏｰｶ
			MakeLoopDeepZUp();	// Z軸の上昇
			AddMoveGdataG0(pData);	// pDataまで移動
		}
		// 切削ﾃﾞｰﾀの生成
		AddMakeGdataDeep(pData, bDeep);
	}
	else {
		// Z軸の復帰
		if ( GetNum(MKNC_NUM_MOVEZ) != 0 )		// 「そのまま」以外なら
			MakeLoopDeepZUp();	// Z軸の上昇
		// 移動ﾃﾞｰﾀの生成
		AddMakeMove(pData);
	}
}

void MakeLoopDeepZDown(void)
{
	const CDXFdata* pDataHead = g_ltDeepGlist.GetHead();
	const CDXFdata* pDataTail = g_ltDeepGlist.GetTail();

	// 往復切削か一連のｵﾌﾞｼﾞｪｸﾄが閉ﾙｰﾌﾟなら
	if ( GetNum(MKNC_NUM_DEEPCPROCESS)==0 ||
			pDataHead->GetStartMakePoint()==pDataTail->GetEndMakePoint() ) {
		// 次の深彫座標へ，Z軸の降下のみ
		AddMoveGdata(1, g_dZCut, GetDbl(MKNC_DBL_ZFEED));
		return;
	}

	// 一方通行切削の場合
	// まずZ軸の上昇
	MakeLoopDeepZUp();
	// 先頭ｵﾌﾞｼﾞｪｸﾄに移動
	AddMoveGdataG0(pDataHead);
}

void MakeLoopDeepZUp(void)
{
	if ( GetNum(MKNC_NUM_DEEPZPROCESS) == 0 ) {
		// 早送りでZ軸復帰
		AddMoveGdataZup();
	}
	else {
		// R点まで切削送りでZ軸復帰
		AddMoveGdata(1, g_dZG0Stop, GetDbl(MKNC_DBL_MAKEENDFEED));
		// ｲﾆｼｬﾙ点復帰なら
		if ( GetNum(MKNC_NUM_ZRETURN) == 0 )
			AddMoveGdataZup();
	}
}

BOOL MakeLoopDrillPoint(BOOL bMatch, CDXFdata* pData)
{
	BOOL	bResult = FALSE;

	// 基準軸で並べ替え，同一線上にあるﾃﾞｰﾀを抽出
	switch ( GetNum(MKNC_NUM_OPTIMAIZEDRILL) ) {
	case 1:		// X軸基準
		g_obDrillAxis.SetSize(0, g_obDrillGroup.GetSize());
		g_obDrillGroup.Sort(DrillOptimaizeCompareFuncY);
		// 現在位置に近い方から
		bResult = MakeLoopDrillPointSeqChk(FALSE, g_obDrillGroup) ?
			MakeLoopDrillPointXY(TRUE) : MakeLoopDrillPointXYRevers(TRUE);
		break;

	case 2:		// Y軸基準
		g_obDrillAxis.SetSize(0, g_obDrillGroup.GetSize());
		g_obDrillGroup.Sort(DrillOptimaizeCompareFuncX);
		bResult = MakeLoopDrillPointSeqChk(TRUE, g_obDrillGroup) ?
			MakeLoopDrillPointXY(FALSE) : MakeLoopDrillPointXYRevers(FALSE);
		break;

	default:	// なし
		bResult = MakeLoopAddDrill(bMatch, pData);
		break;
	}

	return bResult;
}

BOOL MakeLoopDrillPointSeqChk(BOOL bXY, CDXFsort& pObArray)	// bXY==TRUE -> X軸
{
	if ( pObArray.GetSize() <= 1 )	// １件なら比べるまでもない
		return TRUE;

	CDXFdata*	pData1 = pObArray.GetHead();
	CDXFdata*	pData2 = pObArray.GetTail();

#ifdef _DEBUG
	g_dbg.printf("DrillSeqChk(): ArraySize=%d", pObArray.GetSize());
	g_dbg.printf("[ 0 ] x=%f y=%f",
		pData1->GetEndCutterPoint().x, pData1->GetEndCutterPoint().y );
	g_dbg.printf("[MAX] x=%f y=%f",
		pData2->GetEndCutterPoint().x, pData2->GetEndCutterPoint().y );
#endif

	// 現在位置からのどちらが近いか(距離ではなく基準軸で)
	if ( bXY ) {
		if ( fabs(pData1->GetEndCutterPoint().x - CDXFdata::ms_pData->GetEndCutterPoint().x) <=
			 fabs(pData2->GetEndCutterPoint().x - CDXFdata::ms_pData->GetEndCutterPoint().x) )
			return TRUE;	// 先頭が近い
		else
			return FALSE;	// 末尾が近い
	}
	else {
		if ( fabs(pData1->GetEndCutterPoint().y - CDXFdata::ms_pData->GetEndCutterPoint().y) <=
			 fabs(pData2->GetEndCutterPoint().y - CDXFdata::ms_pData->GetEndCutterPoint().y) )
			return TRUE;	// 先頭が近い
		else
			return FALSE;	// 末尾が近い
	}
}

BOOL MakeLoopDrillPointXY(BOOL bXY)
{
	int		i = 0, nPos = 0, n;
	double	dBase, dMargin = fabs(GetDbl(MKNC_DBL_DRILLMARGIN)) * 2.0;
	CDXFsort::PFNCOMPARE	pfnCompare;

	if ( bXY ) {
		n = 1;
		pfnCompare = DrillOptimaizeCompareFuncX;
	}
	else {
		n = 0;
		pfnCompare = DrillOptimaizeCompareFuncY;
	}

	while ( i < g_obDrillGroup.GetSize() && IsThread() ) {
		dBase = g_obDrillGroup[i]->GetEndCutterPoint()[n] + dMargin;
#ifdef _DEBUG
		g_dbg.printf("BasePoint=%f", dBase);
#endif
		g_obDrillAxis.RemoveAll();
		while ( i < g_obDrillGroup.GetSize() &&
				g_obDrillGroup[i]->GetEndCutterPoint()[n] <= dBase && IsThread() ) {
#ifdef _DEBUG
			g_dbg.printf("NowPoint=%f", g_obDrillGroup[i]->GetEndCutterPoint()[n]);
#endif
			g_obDrillAxis.Add(g_obDrillGroup[i++]);
		}
		if ( g_obDrillAxis.IsEmpty() )
			continue;
		g_obDrillAxis.Sort(pfnCompare);
		if ( !MakeLoopDrillPointSeqChk(bXY, g_obDrillAxis) )
			g_obDrillAxis.MakeReverse();
		if ( !MakeLoopAddDrillSeq(nPos) )
			return FALSE;
		nPos = i;
	}

	return IsThread();
}

BOOL MakeLoopDrillPointXYRevers(BOOL bXY)
{
	int		i = g_obDrillGroup.GetUpperBound(), nPos = 0, n;
	double	dBase, dMargin = fabs(GetDbl(MKNC_DBL_DRILLMARGIN)) * 2.0;
	CDXFsort::PFNCOMPARE	pfnCompare;

	if ( bXY ) {
		n = 1;
		pfnCompare = DrillOptimaizeCompareFuncX;
	}
	else {
		n = 0;
		pfnCompare = DrillOptimaizeCompareFuncY;
	}

	while ( i >= 0 && IsThread() ) {
		dBase = g_obDrillGroup[i]->GetEndCutterPoint()[n] - dMargin;
#ifdef _DEBUG
		g_dbg.printf("BasePoint=%f", dBase);
#endif
		g_obDrillAxis.RemoveAll();
		while ( i >= 0 &&
				g_obDrillGroup[i]->GetEndCutterPoint()[n] >= dBase && IsThread() ) {
#ifdef _DEBUG
			g_dbg.printf("NowPoint=%f", g_obDrillGroup[i]->GetEndCutterPoint()[n]);
#endif
			g_obDrillAxis.Add(g_obDrillGroup[i--]);
		}
		if ( g_obDrillAxis.IsEmpty() )
			continue;
		g_obDrillAxis.Sort(pfnCompare);
		if ( !MakeLoopDrillPointSeqChk(bXY, g_obDrillAxis) )
			g_obDrillAxis.MakeReverse();
		if ( !MakeLoopAddDrillSeq(nPos) )
			return FALSE;
		nPos = g_obDrillAxis.GetSize();
	}

	return IsThread();
}

BOOL MakeLoopDrillCircle(void)
{
	BOOL	bMatch;
	CDXFdata*	pData;
	CDXFdata*	pDataResult;
	int		i, j;
	double	r, orgGap, orgGapMin;
	CString	strBreak;

	// g_obCircle の重複座標を削除
	if ( GetFlg(MKNC_FLG_DRILLMATCH) ) {
		for ( i=0; i<g_obCircle.GetSize() && IsThread(); i++ ) {
			pData = g_obCircle[i];
			for ( j=i+1; j<g_obCircle.GetSize() && IsThread(); j++ ) {
				pDataResult = g_obCircle[j];
				if ( pDataResult->IsMatchObject(pData) ||
						sqrt(pDataResult->GetEdgeGap(pData, FALSE)) < GetDbl(MKNC_DBL_TOLERANCE) )
					pDataResult->SetMakeFlg();
			}
		}
	}

	// ﾙｰﾌﾟ開始
	g_obDrillGroup.SetSize(0, g_obCircle.GetSize());
	for ( i=0; i<g_obCircle.GetSize() && IsThread(); ) {
		g_obDrillGroup.RemoveAll();
		// 円をｸﾞﾙｰﾌﾟ(半径)ごとに処理
		r = fabs( ((CDXFcircle *)g_obCircle[i])->GetMakeR() );
		bMatch = FALSE;
		pDataResult = NULL;
		orgGapMin = HUGE_VAL;
		for ( ; i<g_obCircle.GetSize() &&
				r==fabs(((CDXFcircle *)g_obCircle[i])->GetMakeR()) && IsThread(); i++ ) {
			pData = g_obCircle[i];
			if ( !pData->IsMakeFlg() ) {
				g_obDrillGroup.Add(pData);
				orgGap = pData->GetEdgeGap(CDXFdata::ms_pData);
				if ( orgGap < orgGapMin ) {
					orgGapMin = orgGap;
					pDataResult = pData;
					if ( sqrt(orgGap) < GetDbl(MKNC_DBL_TOLERANCE) )
						bMatch = TRUE;
				}
			}
		}
		// 同一半径のﾌﾞﾚｲｸ
		if ( pDataResult ) {
			// 大きさごとにｺﾒﾝﾄを埋め込む
			if ( GetFlg(MKNC_FLG_DRILLBREAK) ) {
				strBreak.Format(IDS_MAKENCD_CIRCLEBREAK, ((CDXFcircle *)pDataResult)->GetMakeR());
				AddMakeGdata(strBreak);
			}
			// ﾃﾞｰﾀ生成
			if ( GetNum(MKNC_NUM_OPTIMAIZEDRILL) == 0 )
				g_obDrillAxis.Copy(g_obDrillGroup);	// 基準軸ないので全て対象
			if ( !MakeLoopDrillPoint(bMatch, pDataResult) )
				return FALSE;
		}
	}

	return IsThread();
}

BOOL MakeLoopAddDrill(BOOL bMatch, CDXFdata* pData)
{
	CDXFdata*	pDataMove;
	CPointD		pt;
	int			nPos = 0;
	BOOL		bMove = FALSE,		// 移動ﾚｲﾔHit
				bCust = TRUE;		// 移動ﾃﾞｰﾀ前のｶｽﾀﾑｺｰﾄﾞ挿入

	// OrgTuning()で現在位置と同一座標が見つからなかった
	// DrillCircle系は並べ替えしてから処理する必要があるのでココで処理
	if ( !bMatch ) {
		pDataMove = MakeLoopAddFirstMove(MAKEDRILLPOINT);
		if ( pDataMove )
			pData = pDataMove;
	}

	// ﾃﾞｰﾀ生成
	AddMakeGdata(pData);

	// GetNearPoint() の結果が NULL になるまで
	while ( IsThread() ) {
		// 最後に生成したﾃﾞｰﾀを待避
		CDXFdata::ms_pData = pData;
		// この要素に一番近い要素
		tie(pData, bMatch) = GetNearPointDrill(pData);
		if ( !pData )
			break;
		if ( !bMatch ) {
			// 移動指示ﾚｲﾔのﾁｪｯｸ
			pDataMove = CDXFdata::ms_pData;
			if ( (*g_pfnGetMatchPointMove)(pDataMove) ) {	// 前回ｵﾌﾞｼﾞｪｸﾄで探索
				// 移動ﾚｲﾔ１つ目の終端に穴加工ﾃﾞｰﾀがHitすれば
				tie(pData, bMatch) = GetNearPointDrill(pDataMove);
				if ( !bMove && bMatch ) {
					// 移動ﾚｲﾔﾃﾞｰﾀは処理したことにして，この穴加工ﾃﾞｰﾀを生成
					pData->SetMakeFlg();
				}
				else {
					bMove = TRUE;
					// 固定ｻｲｸﾙｷｬﾝｾﾙ
					AddMakeGdataCycleCancel();
					// 移動ﾃﾞｰﾀ前のｶｽﾀﾑｺｰﾄﾞ挿入
					if ( bCust ) {
						(*g_pfnAddMoveCust_B)();
						bCust = FALSE;
					}
					// 指示されたZ位置で移動
					(*g_pfnAddMoveZ)();
					// 移動ﾃﾞｰﾀの生成
					AddMakeMove(pDataMove);
					pData = pDataMove;
					continue;	// 再探索
				}
			}
		}
		bCust = TRUE;
		// 移動ﾃﾞｰﾀ後のｶｽﾀﾑｺｰﾄﾞ挿入
		if ( bMove && IsThread() ) {
			// 移動ﾃﾞｰﾀの終点でﾃｷｽﾄﾃﾞｰﾀの生成
			pt = CDXFdata::ms_pData->GetEndMakePoint();
			AddMoveTextIntegrated(pt);
			// ｶｽﾀﾑｺｰﾄﾞ
			(*g_pfnAddMoveCust_A)();
			bMove = FALSE;
		}
		// ﾃﾞｰﾀ生成
		AddMakeGdata(pData);
		SendProgressPos(++nPos);
	} // End of while

	g_pParent->m_ctReadProgress.SetPos(nPos);

	return IsThread();
}

BOOL MakeLoopAddDrillSeq(int nPos)
{
	CDXFdata*	pData;
	// g_obDrillAxisの順番で
	for ( int i=0; i<g_obDrillAxis.GetSize() && IsThread(); i++ ) {
		pData = g_obDrillAxis[i];
		// ﾃﾞｰﾀ生成
		AddMakeGdata(pData);
		SendProgressPos(i+nPos);
	}
	g_pParent->m_ctReadProgress.SetPos(i+nPos);
	// 最後に生成したﾃﾞｰﾀを待避
	CDXFdata::ms_pData = pData;

	return IsThread();
}

CDXFdata* MakeLoopAddFirstMove(ENMAKETYPE enType)
{
	CDXFdata*	pDataMove = CDXFdata::ms_pData;
	CDXFdata*	pDataResult = NULL;
	BOOL		bMatch, bCust = FALSE;

	// 移動指示ﾚｲﾔのﾁｪｯｸ
	while ( (*g_pfnGetMatchPointMove)(pDataMove) && IsThread() ) {
		if ( !bCust ) {
			// 移動ﾃﾞｰﾀ前のｶｽﾀﾑｺｰﾄﾞ挿入(１回だけ)
			(*g_pfnAddMoveCust_B)();
			bCust = TRUE;
		}
		// 指示されたZ位置で移動
		(*g_pfnAddMoveZ)();
		// 移動ﾃﾞｰﾀの生成
		pDataResult = pDataMove;
		AddMakeMove(pDataResult);
		CDXFdata::ms_pData = pDataResult;
		// 同一座標で切削ﾃﾞｰﾀが見つかれば break
		if ( enType == MAKECUTTER ) {
			tie(pDataResult, bMatch) = GetNearPointCutter(pDataResult);
			if ( bMatch )
				break;
		}
		else {
			tie(pDataResult, bMatch) = GetNearPointDrill(pDataResult);
			if ( bMatch )
				break;
		}
	}
	// 移動ﾃﾞｰﾀ後のｶｽﾀﾑｺｰﾄﾞ挿入
	if ( bCust && IsThread() ) {
		// 移動ﾃﾞｰﾀの終点でﾃｷｽﾄﾃﾞｰﾀの生成
		CPointD	pt( CDXFdata::ms_pData->GetEndMakePoint() );
		AddMoveTextIntegrated(pt);
		// ｶｽﾀﾑｺｰﾄﾞ
		(*g_pfnAddMoveCust_A)();
	}

	return pDataResult;
}

BOOL MakeLoopAddLastMove(void)
{
	CDXFdata*	pDataMove = CDXFdata::ms_pData;
	BOOL		bCust = FALSE;

	// 終点座標でのｺﾒﾝﾄ生成
	CPointD	pt( pDataMove->GetEndMakePoint() );
	AddCutterTextIntegrated(pt);	// 切削ﾚｲﾔ
	AddMoveTextIntegrated(pt);		// 移動ﾚｲﾔ

	// 最後の移動ｺｰﾄﾞを検索
	while ( (*g_pfnGetMatchPointMove)(pDataMove) && IsThread() ) {
		if ( !bCust ) {
			(*g_pfnAddMoveCust_B)();
			bCust = TRUE;
		}
		(*g_pfnAddMoveZ)();
		AddMakeMove(pDataMove);
		CDXFdata::ms_pData = pDataMove;
	}
	if ( bCust && IsThread() ) {
		CPointD	pt( CDXFdata::ms_pData->GetEndMakePoint() );
		AddMoveTextIntegrated(pt);
		(*g_pfnAddMoveCust_A)();
	}

	return IsThread();
}

tuple<CDXFdata*, BOOL> GetNearPointCutter(CDXFdata* pData)
{
	CDXFdata*	pDataResult = NULL;
	BOOL		bMatch;
	int			i, nResult, nThreadCnt = g_nProcesser;
	CWordArray	obThread;	// 終了ｽﾚｯﾄﾞ一覧
	if ( g_nProcesser > 1 )
		obThread.SetSize(0, g_nProcesser);

	// ｵﾌﾞｼﾞｪｸﾄ検索開始
	for ( i=0; i<g_nProcesser; i++ ) {
		g_lpSearchResult[i].pData  = pData;
		g_lpSearchResult[i].bMatch = FALSE;
		g_lpmSearch[i].bResult = TRUE;
		g_lpmSearch[i].evStart.SetEvent();
	}

	while ( --nThreadCnt >= 0 ) {
		// ｵﾌﾞｼﾞｪｸﾄ検索終了待ち(どれかが終了すれば良い)
		// CMultiLockではg_lpmSearch[n].evEndの引数が渡せないので
		// 代わりにWaitForMultipleObjects()を使う
		nResult = WaitForMultipleObjects(g_nProcesser, g_phSearchEnd, FALSE, INFINITE) - WAIT_OBJECT_0;
		// 終了したｽﾚｯﾄﾞの結果検証
		if ( nResult < 0 || nResult >= g_nProcesser )
			break;
		// 各ｽﾚｯﾄﾞ結果の検証(一方で見つかれば他方は中断)
		if ( g_lpSearchResult[nResult].bMatch /*&&g_lpSearchResult[nResult].pData*/ ) {
			pDataResult = g_lpSearchResult[nResult].pData;
			for ( i=0; i<g_nProcesser; i++ )
				g_lpmSearch[i].bResult = FALSE;
			// 既に終了したｽﾚｯﾄﾞｲﾍﾞﾝﾄ処理
			for ( i=0; i<obThread.GetSize(); i++ )
				g_lpmSearch[obThread[i]].evEnd.SetEvent();
			// 全てのｽﾚｯﾄﾞが終了するまで待つ
			WaitForMultipleObjects(g_nProcesser, g_phSearchEnd, TRUE, INFINITE);
			for ( i=0; i<g_nProcesser; i++ )
				g_lpmSearch[i].evEnd.ResetEvent();
			bMatch = TRUE;
			break;
		}
		// 次のWaitForMultipleObjects()では nResult 以外のｽﾚｯﾄﾞで待つ必要がある
		g_lpmSearch[nResult].evEnd.ResetEvent();
		if ( g_nProcesser > 1 )
			obThread.Add(nResult);
	}

	if ( nThreadCnt < 0 ) {
		// 各ｽﾚｯﾄﾞの結果を比較
		double	dGap = HUGE_VAL;	// 指定点との距離
		for ( i=0; i<g_nProcesser; i++ ) {
			g_lpmSearch[i].evEnd.ResetEvent();
			if ( g_lpSearchResult[i].pData && dGap > g_lpSearchResult[i].dGap ) {
				dGap = g_lpSearchResult[i].dGap;
				pDataResult = g_lpSearchResult[i].pData;
			}
		}
		bMatch = FALSE;
	}

	return make_tuple(pDataResult, bMatch);
}

tuple<CDXFdata*, BOOL> GetNearPointDrill(CDXFdata* pData)
{
	CDXFdata*	pDataSrc = pData;
	CDXFdata*	pDataResult = NULL;
	BOOL	bMatch = FALSE;
	double	dGap, dGapMin = HUGE_VAL;	// 指定点との距離

	// 現在位置と等しい，または近い要素を検索
	for ( int i=0; i<g_obDrillAxis.GetSize() && IsThread(); i++ ) {
		pData = g_obDrillAxis[i];
		if ( pData->IsMakeFlg() )
			continue;
		// 条件判断
		if ( pData->IsMatchObject(pDataSrc) ) {
			pDataResult = pData;
			bMatch = TRUE;
			break;
		}
		// 現在位置との距離計算
		dGap = pData->GetEdgeGap(pDataSrc);
		if ( dGap < dGapMin ) {
			pDataResult = pData;
			if ( sqrt(dGap) < GetDbl(MKNC_DBL_TOLERANCE) ) {
				bMatch = TRUE;
				break;
			}
			dGapMin = dGap;
		}
	}

	return make_tuple(pDataResult, bMatch);
}

BOOL GetMatchPointMove_Target(CDXFdata*& pDataResult)
{
	// 現在位置と等しい要素だけを検索
	CPointD	pt( pDataResult->GetEndMakePoint() );
	CDXFarray*	pobArray;
	BOOL		bMatch = FALSE;
	
	if ( g_mpDXFmove.Lookup(pt, pobArray) ) {
		CDXFdata*	pData;
		for ( int i=0; i<pobArray->GetSize() && IsThread(); i++ ) {
			pData = pobArray->GetAt(i);
			if ( !pData->IsMakeFlg() ) {
				pData->GetEdgeGap(pt);	// 始点を調整
				pDataResult = pData;
				bMatch = TRUE;
				break;
			}
		}
	}

	return bMatch;
}

BOOL GetMatchPointMove_Exclude(CDXFdata*&)
{
	return FALSE;
}

// ｶｽﾀﾑﾍｯﾀﾞｰ, ﾌｯﾀﾞｰ処理
struct CMakeCustomCode	// parse() から呼び出し
{
	string&	m_strResult;
	const CDXFdata*	m_pData;

	CMakeCustomCode(string& r, const CDXFdata* pData) : m_strResult(r), m_pData(pData) {}

	void operator()(const char* s, const char* e) const
	{
		extern	const	DWORD	g_dwSetValFlags[];
		static	LPCTSTR	szCustomCode[] = {
			"G90orG91", "G92_INITIAL", "G92X", "G92Y", "G92Z",
			"SPINDLE", "G0XY_INITIAL",
			"MakeUser", "MakeDate", "MakeTime", "MakeNCD", "MakeDXF", "MakeCondition"
		};
		static	CStringKeyIndex	stOrder(SIZEOF(szCustomCode), szCustomCode);
		static	LPCTSTR	szReplaceErr = "???";

		string	str(s+1, e-1);	// 前後の "{}" 除去
		CString	strBuf;
		TCHAR	szUserName[_MAX_PATH];
		DWORD	dwResult;
		CTime	time;
		double	dValue[VALUESIZE];
		int		nTestCode = stOrder.GetIndex(str.c_str());
		// replace
		switch ( nTestCode ) {
		case 0:		// G90orG91
			m_strResult += CNCMake::MakeCustomString(GetNum(MKNC_NUM_G90)+90);
			break;
		case 1:		// G92_INITIAL
			dValue[NCA_X] = GetDbl(MKNC_DBL_G92X);
			dValue[NCA_Y] = GetDbl(MKNC_DBL_G92Y);
			dValue[NCA_Z] = GetDbl(MKNC_DBL_G92Z);
			m_strResult += CNCMake::MakeCustomString(92, dValue, NCD_X|NCD_Y|NCD_Z, FALSE);
			break;
		case 2:		// G92X
		case 3:		// G92Y
		case 4:		// G92Z
			dValue[nTestCode-2] = GetDbl(MKNC_DBL_G92X+nTestCode-2);
			m_strResult += CNCMake::MakeCustomString(-1, dValue,
								g_dwSetValFlags[nTestCode-2], FALSE);
			break;
		case 5:		// SPINDLE
			if ( m_pData )					// Header
				m_strResult += CNCMake::MakeSpindle(m_pData->GetMakeType());
			else if ( CDXFdata::ms_pData )	// Footer
				m_strResult += CNCMake::MakeSpindle(CDXFdata::ms_pData->GetMakeType());
			break;
		case 6:		// G0XY_INITIAL
			dValue[NCA_X] = GetDbl(MKNC_DBL_G92X);
			dValue[NCA_Y] = GetDbl(MKNC_DBL_G92Y);
			m_strResult += CNCMake::MakeCustomString(0, dValue, NCD_X|NCD_Y);
			break;
		case 7:		// MakeUser
			dwResult = _MAX_PATH;
			// ﾕｰｻﾞ名に漢字が含まれていると生成しない
			m_strResult += GetUserName(szUserName, &dwResult) && IsNCchar(szUserName) ?
				szUserName : szReplaceErr;
			break;
		case 8:		// MakeDate
			time = CTime::GetCurrentTime();
			VERIFY(strBuf.LoadString(ID_INDICATOR_DATE_F2));// %y/%m/%d
			m_strResult += time.Format(strBuf);
			break;
		case 9:		// MakeTime
			time = CTime::GetCurrentTime();
			VERIFY(strBuf.LoadString(ID_INDICATOR_TIME_F));	// %H:%M
			m_strResult += time.Format(strBuf);
			break;
		case 10:	// MakeNCD
			Path_Name_From_FullPath(g_pDoc->GetNCFileName(), CString(), strBuf);
			m_strResult += IsNCchar(strBuf) ? strBuf : szReplaceErr;
			break;
		case 11:	// MakeDXF
			strBuf = g_pDoc->GetTitle();
			m_strResult += IsNCchar(strBuf) ? strBuf : szReplaceErr;
			break;
		case 12:	// MakeCondition
			Path_Name_From_FullPath(g_pMakeOpt->GetInitFile(), CString(), strBuf);
			m_strResult += IsNCchar(strBuf) ? strBuf : szReplaceErr;
			break;
		}
	}
};

struct CMakeCustomCode2	// parse() から呼び出し
{
	string&	strResult;
	CMakeCustomCode2(string& r) : strResult(r) {}

	void operator()(const char* s, const char* e) const
	{
		string  str(s, e);
		strResult += str;
	}
};

void AddCustomCode(const CString& strFileName, const CDXFdata* pData)
{
	using namespace boost::spirit;

	CString	strBuf;
	string	strResult;
	CMakeCustomCode		custom1(strResult, pData);
	CMakeCustomCode2	custom2(strResult);

	try {
		CStdioFile	fp(strFileName,
			CFile::modeRead | CFile::shareDenyWrite | CFile::typeText);
		while ( fp.ReadString(strBuf) ) {
			// 構文解析
			strResult.clear();
			parse((LPCTSTR)strBuf,
				*( *(anychar_p - '{')[custom2] >> comment_p('{', '}')[custom1] )
			);
			AddMakeGdata( strResult.c_str() );
		}
	}
	catch (CFileException* e) {
		AfxMessageBox(IDS_ERR_MAKECUSTOM, MB_OK|MB_ICONEXCLAMATION);
		e->Delete();
		// このｴﾗｰは正常ﾘﾀｰﾝ(警告のみ)
	}
}

BOOL IsNCchar(LPCTSTR lpsz)
{
	BOOL	bResult = TRUE;
	for ( int i=0; i<lstrlen(lpsz); i++ ) {
		if ( isprint(lpsz[i]) == 0 ) {
			bResult = FALSE;
			break;
		}
	}
	return bResult;
}

// 加工開始指示ﾃﾞｰﾀの生成
void AddMakeStart(void)
{
	CPointD	pt;
	// 機械原点でのﾃｷｽﾄﾃﾞｰﾀ生成
	AddStartTextIntegrated(pt);	// (0, 0)
	
	CDXFdata*	pData = NULL;
	CNCMake*	mkNCD;
	// 特定条件しか呼ばれないので IsMakeFlg() の判断は必要なし
	for ( int i=0; i<g_obStartData.GetSize() && IsThread(); i++ ) {
		pData = g_obStartData[i];
		// 開始位置と等しいﾃｷｽﾄﾃﾞｰﾀの生成
		pt = pData->GetStartMakePoint();
		AddStartTextIntegrated(pt);
		// 移動指示
		mkNCD = new CNCMake(pData);
		ASSERT( mkNCD );
		g_obMakeGdata.Add(mkNCD);
	}
	if ( pData ) {
		// 終了位置と等しいﾃｷｽﾄﾃﾞｰﾀの生成
		pt = pData->GetEndMakePoint();
		AddStartTextIntegrated(pt);
	}
}

// 移動指示ﾚｲﾔのZ軸復帰処理
void AddMoveZ_NotMove(void)
{
	// そのまま
}

void AddMoveZ_R(void)
{
	// Z軸の現在位置がR点より小さい(低い)なら
	if ( CNCMake::ms_xyz[NCA_Z] < g_dZG0Stop )
		AddMoveGdata(0, g_dZG0Stop, -1);
}

void AddMoveZ_Initial(void)
{
	// Z軸の現在位置がｲﾆｼｬﾙ点より小さい(低い)なら
	if ( CNCMake::ms_xyz[NCA_Z] < g_dZInitial )
		AddMoveGdata(0, g_dZInitial, -1);
}

void AddMoveCust_B(void)
{
	AddMakeGdata(g_pMakeOpt->GetStr(MKNC_STR_CUSTMOVE_B));
}

void AddMoveCust_A(void)
{
	AddMakeGdata(g_pMakeOpt->GetStr(MKNC_STR_CUSTMOVE_A));
}

// ﾃｷｽﾄ情報の生成
void AddCutterText_Target(CPointD& pt)
{
	CDXFarray*	pobArray;
	
	if ( g_mpDXFtext.Lookup(pt, pobArray) ) {
		CDXFdata*	pData;
		for ( int i=0; i<pobArray->GetSize() && IsThread(); i++ ) {
			pData = pobArray->GetAt(i);
			if ( !pData->IsMakeFlg() ) {
				CString	strText( ((CDXFtext *)pData)->GetStrValue() );
				strText.Replace("\\n", "\n");
				AddMakeGdata(strText);
				pData->SetMakeFlg();
				break;
			}
		}
	}
}

void AddStartText_Target(CPointD& pt)
{
	CDXFarray*	pobArray;
	
	if ( g_mpDXFstarttext.Lookup(pt, pobArray) ) {
		CDXFdata*	pData;
		for ( int i=0; i<pobArray->GetSize() && IsThread(); i++ ) {
			pData = pobArray->GetAt(i);
			if ( !pData->IsMakeFlg() ) {
				CString	strText( ((CDXFtext *)pData)->GetStrValue() );
				strText.Replace("\\n", "\n");
				AddMakeGdata(strText);
				pData->SetMakeFlg();
				break;
			}
		}
	}
}

void AddMoveText_Target(CPointD& pt)
{
	CDXFarray*	pobArray;

	if ( g_mpDXFmovetext.Lookup(pt, pobArray) ) {
		CDXFdata*	pData;
		for ( int i=0; i<pobArray->GetSize() && IsThread(); i++ ) {
			pData = pobArray->GetAt(i);
			if ( !pData->IsMakeFlg() ) {
				CString	strText( ((CDXFtext *)pData)->GetStrValue() );
				strText.Replace("\\n", "\n");
				AddMakeGdata(strText);
				pData->SetMakeFlg();
				return;
			}
		}
	}
}

void AddCommentText_Target(CPointD& pt)
{
	CDXFarray*	pobArray;
	
	if ( g_mpDXFcomment.Lookup(pt, pobArray) ) {
		CDXFdata*	pData;
		for ( int i=0; i<pobArray->GetSize() && IsThread(); i++ ) {
			pData = pobArray->GetAt(i);
			if ( !pData->IsMakeFlg() ) {
				AddMakeGdata( "(" + ((CDXFtext *)pData)->GetStrValue() + ")" );
				pData->SetMakeFlg();
				break;
			}
		}
	}
}

void AddText_Exclude(CPointD&)
{
	// 何もしない
}

// ﾌｪｰｽﾞ出力
void SendFaseMessage
	(int nRange/*=-1*/, LPCTSTR lpszMsg1/*=NULL*/, LPCTSTR lpszMsg2/*=NULL*/)
{
#ifdef _DEBUG
	CMagaDbg	dbg("MakeNCD_Thread()", DBG_GREEN);
	dbg.printf("Phase%d Start", g_nFase);
#endif
	if ( nRange > 0 )
		g_pParent->m_ctReadProgress.SetRange32(0, nRange);

	CString	strMsg;
	if ( lpszMsg1 )
		strMsg = lpszMsg1;
	else
		strMsg.Format(IDS_MAKENCD_FASE, g_nFase++);
	g_pParent->SetFaseMessage(strMsg, lpszMsg2);
}

//////////////////////////////////////////////////////////////////////
// 並べ替え補助関数
//////////////////////////////////////////////////////////////////////

int CircleSizeCompareFunc1(CDXFdata* pFirst, CDXFdata* pSecond)
{
	return (int)( (fabs(((CDXFcircle *)pFirst)->GetMakeR()) -
						fabs(((CDXFcircle *)pSecond)->GetMakeR())) * 1000.0 );
}

int CircleSizeCompareFunc2(CDXFdata* pFirst, CDXFdata* pSecond)
{
	return (int)( (fabs(((CDXFcircle *)pSecond)->GetMakeR()) -
						fabs(((CDXFcircle *)pFirst)->GetMakeR())) * 1000.0 );
}

int DrillOptimaizeCompareFuncX(CDXFdata* pFirst, CDXFdata* pSecond)
{
	// 値が大きくなる可能性がある
	int		nResult;
	double	dResult = pFirst->GetEndCutterPoint().x - pSecond->GetEndCutterPoint().x;
	if ( dResult == 0.0 )
		nResult = 0;
	else if ( dResult > 0 )
		nResult = 1;
	else
		nResult = -1;
	return nResult;
}

int DrillOptimaizeCompareFuncY(CDXFdata* pFirst, CDXFdata* pSecond)
{
	int		nResult;
	double	dResult = pFirst->GetEndCutterPoint().y - pSecond->GetEndCutterPoint().y;
	if ( dResult == 0.0 )
		nResult = 0;
	else if ( dResult > 0 )
		nResult = 1;
	else
		nResult = -1;
	return nResult;
}

//////////////////////////////////////////////////////////////////////
// ｻﾌﾞｽﾚｯﾄﾞ
//////////////////////////////////////////////////////////////////////

// 各ｽﾚｯﾄﾞの検索範囲
void SetSearchRange(void)
{
	int	i, nCnt = 0, nStep = max(1, g_obDXFdata.GetSize()/g_nProcesser);
	for ( i=0; i<g_nProcesser-1; i++ ) {
		g_lpmSearch[i].nOrder = nCnt;
		nCnt = min(nCnt+nStep, g_obDXFdata.GetSize());
		g_lpmSearch[i].nOrder2 = nCnt;
	}
	g_lpmSearch[i].nOrder = nCnt;
	nCnt = min(nCnt+nStep, g_obDXFdata.GetSize());
	g_lpmSearch[i].nOrder2 = max(nCnt, g_obDXFdata.GetSize());
}

// 次の切削ｵﾌﾞｼﾞｪｸﾄの検索(from GetNearPointCutter)
UINT SearchObjectThread(LPVOID lpVoid)
{
#ifdef _DEBUG
	CMagaDbg	dbg("SearchObjectThread()\nStart", DBG_BLUE);
#endif
	int	i, s, e, n = (int)lpVoid;	// ｸﾞﾛｰﾊﾞﾙ変数のどれを使うか
	CDXFdata*	pDataSrc;
	CDXFdata*	pData;
	double	dGap, dGapMin;

	while ( IsThread() ) {
		// 上位の実行許可が下りるまでｳｪｲﾄ
		g_lpmSearch[n].evStart.Lock();
		g_lpmSearch[n].evStart.ResetEvent();
		// 継続ﾌﾗｸﾞﾁｪｯｸ
		if ( !g_lpmSearch[n].bThread )
			break;
		pDataSrc = g_lpSearchResult[n].pData;
		g_lpSearchResult[n].pData  = NULL;
		dGapMin = HUGE_VAL;
		s = g_lpmSearch[n].nOrder;
		e = g_lpmSearch[n].nOrder2;
		// 検索開始
#ifdef _DEBUG
		dbg.printf("ID=%d Start s=%d e=%d", n, s, e);
#endif
		// 現在位置と等しい，または近い要素を検索
		// 他のｽﾚｯﾄﾞで等しい座標が存在したとき g_lpmSearch[n].bResult が偽で検索中断
		for ( i=s; i<e && g_lpmSearch[n].bResult && g_lpmSearch[n].bThread && IsThread(); i++ ) {
			pData = g_obDXFdata[i];
			if ( pData->IsMakeFlg() )
				continue;
			// 条件判断
//			if ( pData->IsMatchObject(pDataSrc) ) {
//				g_lpSearchResult[n].pData = pData;
//				g_lpSearchResult[n].bMatch = TRUE;
//				break;
//			}
			// 現在位置との距離計算(許容差内は座標ﾏｯﾌﾟにて)
			dGap = pData->GetEdgeGap(pDataSrc);
			if ( dGap < dGapMin ) {
				g_lpSearchResult[n].pData = pData;
//				if ( sqrt(dGap) < CDXFmap::ms_dTolerance ) {
//					g_lpSearchResult[n].bMatch = TRUE;
//					break;
//				}
				dGapMin = dGap;
			}
		}
		// 検索終了
#ifdef _DEBUG
		dbg.printf("ID=%d End", n);
#endif
		g_lpSearchResult[n].dGap = dGapMin;
		g_lpmSearch[n].evEnd.SetEvent();
	}

	g_lpmSearch[n].evEnd.SetEvent();
#ifdef _DEBUG
	dbg.printf("ID=%d Thread End", n);
#endif
	return 0;
}

// CMapｵﾌﾞｼﾞｪｸﾄｸﾘｱのﾊﾞｯｸｸﾞﾗｳﾝﾄﾞｽﾚｯﾄﾞ
UINT ClearPointMapThread(LPVOID lpVoid)
{
#ifdef _DEBUG
	CMagaDbg	dbg("ClearPointMapThread()\nStart", DBG_BLUE);
#endif

	while ( IsThread() ) {
		// 上位の実行許可が下りるまでｳｪｲﾄ
		g_pmClearMap.evStart.Lock();
		g_pmClearMap.evStart.ResetEvent();
		// 継続ﾌﾗｸﾞﾁｪｯｸ
		if ( !g_pmClearMap.bThread )
			break;
		// 関数実行
		g_mpEuler.RemoveAll();	// 時間がかかる場合がある
		// 終了
#ifdef _DEBUG
		dbg.printf("End");
#endif
		g_pmClearMap.evEnd.SetEvent();
	}

	g_pmClearMap.evEnd.SetEvent();
#ifdef _DEBUG
	dbg.printf("Thread End");
#endif
	return 0;
}

// NC生成のｸﾞﾛｰﾊﾞﾙ変数初期化(後始末)ｽﾚｯﾄﾞ
UINT MakeNCDAfterThread(LPVOID)
{
	g_csMakeAfter.Lock();

	int		i;
#ifdef _DEBUG
	CMagaDbg	dbg("MakeNCDAfterThread()\nStart", TRUE, DBG_RED);
#endif
	g_obDXFdata.RemoveAll();
	g_mpDXFdata.RemoveAll();
	g_mpDXFstarttext.RemoveAll();
	g_mpDXFmove.RemoveAll();
	g_mpDXFmovetext.RemoveAll();
	g_mpDXFtext.RemoveAll();
	g_mpDXFcomment.RemoveAll();
	for ( i=0; i<g_obMakeGdata.GetSize(); i++ )
		delete	g_obMakeGdata[i];
	g_obMakeGdata.RemoveAll();
	g_obStartData.RemoveAll();
	g_ltDeepGlist.RemoveAll();
	g_obPoint.RemoveAll();
	g_obCircle.RemoveAll();
	g_obDrillGroup.RemoveAll();
	g_obDrillAxis.RemoveAll();
	g_mpEuler.RemoveAll();

	g_csMakeAfter.Unlock();
	return 0;
}

// ﾌﾟﾛｾｯｻ数に応じたｽﾚｯﾄﾞの呼び出し(from CNCVCApp::InitInstance())
void SetMakeThreadFunction(void)
{
	// ﾌﾟﾛｾｯｻ数に応じた各変数領域の確保
	ASSERT( g_nProcesser > 0 );
	g_lpmSearch			= new MAKENCDTHREADPARAM[g_nProcesser];
	g_lpSearchResult	= new SEARCHRESULT[g_nProcesser];
	g_phSearchEnd		= new HANDLE[g_nProcesser];
	for ( int i=0; i<g_nProcesser; i++ )
		g_phSearchEnd[i] = HANDLE(g_lpmSearch[i].evEnd);
}

void DestructThreadFunction(void)
{
	if ( g_lpmSearch )
		delete[]	g_lpmSearch;
	if ( g_lpSearchResult )
		delete[]	g_lpSearchResult;
	if ( g_phSearchEnd )
		delete[]	g_phSearchEnd;
}
