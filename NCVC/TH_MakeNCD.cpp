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

/*
!!!ATTENTION!!!
生成時間の表示：正式ﾘﾘｰｽでは外すのを忘れずに
#define	_DBG_NCMAKE_TIME
*/

using namespace boost;

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

/*
	CDXFdata の GetType() と GetMakeType() の使い分けに注意！！
*/
// よく使う変数や呼び出しの簡略置換
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
static	CDXFmap		g_mpDXFdata,	// 座標をｷｰにCDXFdataを格納
			g_mpDXFstarttext,				// 開始ﾚｲﾔ
			g_mpDXFmove, g_mpDXFmovetext,	// 移動ﾚｲﾔﾏｯﾌﾟ
			g_mpDXFtext, g_mpDXFcomment;	// 加工ﾃｷｽﾄ，ｺﾒﾝﾄ専用
static	CDXFarray	g_obPoint;		// 穴加工ﾃﾞｰﾀ
static	CDXFsort	g_obCircle;		// 円ﾃﾞｰﾀを穴加工するときの仮登録
static	CDXFsort	g_obDrillGroup;	// ｸﾞﾙｰﾌﾟ分けした穴加工ﾃﾞｰﾀ
static	CDXFsort	g_obDrillAxis;	// -> 軸座標で並べ替え
static	CDXFsort	g_obStartData;	// 加工開始位置指示ﾃﾞｰﾀ
static	CDXFlist	g_ltDeepGlist;	// 深彫切削用の仮登録
static	CTypedPtrArrayEx<CPtrArray, CNCMake*>	g_obMakeGdata;	// 加工ﾃﾞｰﾀ

static	BOOL		g_bData;		// 各生成処理で生成されたか
static	double		g_dZCut;		// Z軸の切削座標 == RoundUp(GetDbl(MKNC_DBL_ZCUT))
static	double		g_dDeep;		// 深彫の切削座標 == RoundUp(GetDbl(MKNC_DBL_DEEP))
static	double		g_dZInitial;	// Z軸のｲﾆｼｬﾙ点 == RoundUp(GetDbl(MKNC_DBL_G92Z))
static	double		g_dZG0Stop;		// Z軸のR点 == RoundUp(GetDbl(MKNC_DBL_ZG0STOP))
static	double		g_dZReturn;		// Z軸の復帰座標
static	int			g_nCorrect;		// 補正ｺｰﾄﾞ

// ｻﾌﾞ関数
static	void	InitialVariable(void);		// 変数初期化
static	void	InitialCycleBaseVariable(void);	// 固定ｻｲｸﾙのﾍﾞｰｽ値初期化
static	BOOL	SingleLayer(int);			// 単一ﾚｲﾔ, 形状処理
static	BOOL	MultiLayer(int);			// 複数ﾚｲﾔ処理(2ﾊﾟﾀｰﾝ兼用)
static	void	SetStaticOption(void);		// 静的変数の初期化
static	BOOL	SetStartData(void);			// 加工開始位置指示ﾚｲﾔの仮登録, 移動指示ﾚｲﾔの処理
static	void	SetGlobalMap(void);
static	void	SetGlobalMapToLayer(const CLayerData*);
static	void	SetGlobalMapToLayer_Drill(CDXFdata*);
static	void	SetGlobalMapToOther(void);
static	BOOL	MakeNCD_MainFunc(const CLayerData*);		// NC生成のﾒｲﾝﾙｰﾌﾟ
static	BOOL	MakeNCD_ShapeFunc(void);
static	BOOL	MakeNCD_FinalFunc(LPCTSTR = NULL);	// 終了ｺｰﾄﾞ，ﾌｧｲﾙ出力など
static	BOOL	OutputNCcode(LPCTSTR);		// NCｺｰﾄﾞの出力

// 各ﾀｲﾌﾟ別の処理呼び出し
static	enum	ENMAKETYPE {MAKECUTTER, MAKECORRECT, MAKEDRILLPOINT, MAKEDRILLCIRCLE};
static	BOOL	CallMakeDrill(const CLayerData*, CString&);
static	BOOL	CallMakeLoop(ENMAKETYPE, const CLayerData*, CString&);

// 原点調整(TRUE:IsMatch)
static	tuple<CDXFdata*, BOOL>	OrgTuningCutter(const CLayerData*);
static	tuple<CDXFdata*, BOOL>	OrgTuningDrillPoint(void);
static	CDXFdata*	OrgTuningDrillCircle(void);

// ﾃﾞｰﾀ解析関数
static	BOOL		MakeLoopEuler(const CLayerData*, CDXFdata*);
static	BOOL		MakeLoopEulerSearch(const CPointD&, CDXFmap&);
static	int			MakeLoopEulerAdd(const CDXFmap*);
static	BOOL		MakeLoopEulerAdd_with_one_stroke(const CDXFmap*, BOOL, BOOL, const CPointD&, const CDXFarray*, CDXFlist&);
static	BOOL		MakeLoopShape(CDXFshape*);
static	int			MakeLoopShapeSearch(const CDXFshape*);
static	BOOL		MakeLoopShapeAdd(CDXFshape*, CDXFdata*);
static	BOOL		MakeLoopShapeAdd_ChainList(CDXFshape*, CDXFdata*);
static	BOOL		MakeLoopShapeAdd_EulerMap(CDXFshape*);
static	BOOL		MakeLoopShapeAdd_EulerMap_Make(CDXFshape*, CDXFmap*, BOOL&);
static	BOOL		MakeLoopShapeAdd_EulerMap_Search(const CPointD&, CDXFmap*, CDXFmap*);
static	BOOL		MakeLoopDeepAdd(void);
typedef	CDXFdata*	(*PFNDEEPPROC)(BOOL, BOOL);
static	PFNDEEPPROC	g_pfnDeepProc;
static	CDXFdata*	MakeLoopDeepAdd_Euler(BOOL, BOOL);
static	CDXFdata*	MakeLoopDeepAdd_All(BOOL, BOOL);
static	void		MakeLoopDeepZDown(void);
static	void		MakeLoopDeepZUp(void);
static	BOOL		MakeLoopDrillPoint(CDXFdata*);
static	BOOL		MakeLoopDrillPointSeqChk(BOOL, CDXFsort&);
static	BOOL		MakeLoopDrillPointXY(BOOL);
static	BOOL		MakeLoopDrillPointXYRevers(BOOL);
static	BOOL		MakeLoopDrillCircle(void);
static	BOOL		MakeLoopAddDrill(CDXFdata*);
static	BOOL		MakeLoopAddDrillSeq(int);
static	CDXFdata*	MakeLoopAddFirstMove(ENMAKETYPE);
static	BOOL		MakeLoopAddLastMove(void);

// 引数で指定したｵﾌﾞｼﾞｪｸﾄに一番近いｵﾌﾞｼﾞｪｸﾄを返す(戻り値:Z軸の移動が必要か)
static	CDXFdata*	GetNearPointCutter(const CLayerData*, const CDXFdata*);
static	CDXFshape*	GetNearPointShape(const CPointD&);
static	tuple<CDXFdata*, BOOL>	GetNearPointDrill(CDXFdata*);
static	BOOL	GetMatchPointMove(CDXFdata*&);

// ﾍｯﾀﾞｰ,ﾌｯﾀﾞｰ等のｽﾍﾟｼｬﾙｺｰﾄﾞ生成
static	void	AddCustomCode(const CString&, const CDXFdata*);
static	BOOL	IsNCchar(LPCTSTR);

// ﾃｷｽﾄ情報
static	void	AddCommentText(const CPointD&);
static	void	AddStartTextIntegrated(const CPointD& pt);
static	void	AddMoveTextIntegrated(const CPointD& pt);
static	void	AddCutterTextIntegrated(const CPointD& pt);

// NCﾃﾞｰﾀ登録関数
static	void	AddMakeStart(void);		// 加工開始指示ﾃﾞｰﾀの生成
typedef void	(*PFNADDMOVE)(void);	// 移動指示ﾚｲﾔの移動時における動作関数
static	PFNADDMOVE	g_pfnAddMoveZ, g_pfnAddMoveCust_B, g_pfnAddMoveCust_A;
static	void	AddMoveZ_NotMove(void);
static	void	AddMoveZ_R(void);
static	void	AddMoveZ_Initial(void);
static	void	AddMoveCust_B(void);
static	void	AddMoveCust_A(void);

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
	// 終点座標でのｺﾒﾝﾄ生成
	AddCutterTextIntegrated( CDXFdata::ms_pData->GetEndMakePoint() );
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
	// 開始位置と等しいﾃｷｽﾄﾃﾞｰﾀの生成
	AddCutterTextIntegrated( pData->GetStartMakePoint() );
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
	// 開始位置と等しいﾃｷｽﾄﾃﾞｰﾀの生成
	AddCutterTextIntegrated( pData->GetStartMakePoint() );
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
	// 開始位置と等しいﾃｷｽﾄﾃﾞｰﾀの生成
	AddMoveTextIntegrated( pData->GetStartMakePoint() );
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
// 文字情報出力
inline	void	AddMakeText(CDXFdata* pData)
{
	CString	strText( ((CDXFtext *)pData)->GetStrValue() );
	strText.Replace("\\n", "\n");
	AddMakeGdata(strText);
	pData->SetMakeFlg();
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
static	CCriticalSection	g_csMakeAfter;	// MakeNCD_AfterThread()ｽﾚｯﾄﾞﾛｯｸｵﾌﾞｼﾞｪｸﾄ
static	UINT	MakeNCD_AfterThread(LPVOID);	// 後始末ｽﾚｯﾄﾞ

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

	// 下位の CMemoryException は全てここで集約
	try {
		BOOL	bResult = FALSE;
		// NC生成ﾀｲﾌﾟ
		int		nID = (int)(pParam->wParam);

		// NC生成ｵﾌﾟｼｮﾝｵﾌﾞｼﾞｪｸﾄの生成
		g_pMakeOpt = new CNCMakeOption(NULL);	// 読み込みはしない
		CNCMake::ms_pMakeOpt = g_pMakeOpt;

		// 変数初期化ｽﾚｯﾄﾞの処理待ち
		g_csMakeAfter.Lock();		// ｽﾚｯﾄﾞ側でﾛｯｸ解除するまで待つ
		g_csMakeAfter.Unlock();
#ifdef _DEBUG
		dbg.printf("g_csMakeAfter Unlock OK");
#endif

		// NC生成のﾙｰﾌﾟ前に必要な初期化
		{
			optional<CPointD>	ptResult = g_pDoc->GetCutterOrigin();
			CDXFdata::ms_ptOrg = ptResult ? *ptResult : 0.0;
		}
		g_pDoc->GetCircleObject()->OrgTuning(FALSE);	// 結果的に原点がｾﾞﾛになる
		InitialVariable();
		// 増分割り当て
		i = g_pDoc->GetDxfLayerDataCnt(DXFCAMLAYER);
		g_obPoint.SetSize(0, i);
		g_obCircle.SetSize(0, i);
		i *= 2;
		g_obMakeGdata.SetSize(0, i);
		i = max(17, GetPrimeNumber(i));
		g_mpDXFdata.InitHashTable(i);
		g_mpDXFtext.InitHashTable(i);
		i = g_pDoc->GetDxfLayerDataCnt(DXFSTRLAYER);
		g_obStartData.SetSize(0, max(10, i*2));
		g_mpDXFstarttext.InitHashTable(max(17, GetPrimeNumber(i)));
		i = max(17, GetPrimeNumber(g_pDoc->GetDxfLayerDataCnt(DXFMOVLAYER)*2));
		g_mpDXFmove.InitHashTable(i);
		g_mpDXFmovetext.InitHashTable(i);
		g_mpDXFcomment.InitHashTable(max(17, GetPrimeNumber(g_pDoc->GetDxfLayerDataCnt(DXFCOMLAYER))));

		// ﾒｲﾝﾙｰﾌﾟへ
		switch ( nID ) {
		case ID_FILE_DXF2NCD:		// 単一ﾚｲﾔ
		case ID_FILE_DXF2NCD_SHAPE:	// 形状加工
			bResult = SingleLayer(nID);
			break;

		case ID_FILE_DXF2NCD_EX1:	// 複数の切削条件ﾌｧｲﾙ
		case ID_FILE_DXF2NCD_EX2:	// 複数Z座標
			bResult = MultiLayer(nID);
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

	// 終了処理
	CDXFmap::ms_dTolerance = NCMIN;		// 規定値に戻す
	g_pParent->PostMessage(WM_USERFINISH, nResult);	// このｽﾚｯﾄﾞからﾀﾞｲｱﾛｸﾞ終了
	// 生成したNCｺｰﾄﾞの消去ｽﾚｯﾄﾞ(優先度を下げる)
	AfxBeginThread(MakeNCD_AfterThread, NULL,
		/*THREAD_PRIORITY_LOWEST*/
		THREAD_PRIORITY_IDLE
		/*THREAD_PRIORITY_BELOW_NORMAL*/);

	// 条件ｵﾌﾞｼﾞｪｸﾄ削除
	if ( g_pMakeOpt )
		delete	g_pMakeOpt;

	return 0;
}

//////////////////////////////////////////////////////////////////////

void InitialVariable(void)
{
	g_bData = FALSE;
	g_nCorrect = -1;
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

BOOL SingleLayer(int nID)
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
	SetGlobalMap();

	// 生成開始
	if ( nID == ID_FILE_DXF2NCD ) {
		if ( !MakeNCD_MainFunc(NULL) ) {
#ifdef _DEBUG
			dbg.printf("Error:MakeNCD_MainFunc()");
#endif
			return FALSE;
		}
	}
	else {
		if ( !MakeNCD_ShapeFunc() ) {
#ifdef _DEBUG
			dbg.printf("Error:MakeNCD_ShapeFunc()");
#endif
			return FALSE;
		}
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

BOOL MultiLayer(int nID)
{
#ifdef _DEBUG
	CMagaDbg	dbg("MultiLayer()\nStart", DBG_CYAN);
	CMagaDbg	dbgE("MultiLayer() Error", DBG_RED);
#endif
	extern	LPCTSTR	gg_szCat;
	int		i, j, nLayerCnt = g_pDoc->GetLayerCnt();
	BOOL	bPartOut = FALSE;	// １回でも個別出力があればTRUE
	CLayerData*	pLayer;

	if ( nID == ID_FILE_DXF2NCD_EX2 ) {
		// 基準となるNC生成ｵﾌﾟｼｮﾝ読み込み
		g_pMakeOpt->ReadMakeOption(AfxGetNCVCApp()->GetDXFOption()->GetInitList()->GetHead());
		// 条件ごとに変化するﾊﾟﾗﾒｰﾀを設定
		SetStaticOption();
	}
	// 加工原点開始位置指示ﾚｲﾔ処理
	if ( !SetStartData() )
		return FALSE;

	// 切削ﾚｲﾔ以外のﾏｯﾌﾟ作成
	SetGlobalMapToOther();

	// ﾚｲﾔ毎のﾙｰﾌﾟ
	for ( i=0; i<nLayerCnt && IsThread(); i++ ) {
		pLayer = g_pDoc->GetLayerData(i);
		// 切削ﾚｲﾔで対象のﾚｲﾔだけ
		if ( !pLayer->IsMakeTarget() )
			continue;
		pLayer->SetLayerFlags();
		// 処理ﾚｲﾔ名をﾀﾞｲｱﾛｸﾞに表示
		g_nFase = 1;
		SendFaseMessage(-1, g_szWait, pLayer->GetStrLayer());
		//
		if ( nID == ID_FILE_DXF2NCD_EX1 ) {
			// NC生成ｵﾌﾟｼｮﾝ読み込み
			if ( pLayer->GetInitFile().CompareNoCase(g_pMakeOpt->GetInitFile()) != 0 ) {
				g_pMakeOpt->ReadMakeOption(pLayer->GetInitFile());	// 違うときだけ
				SetStaticOption();
			}
			// Z軸の切削座標ｾｯﾄ
			g_dZCut = RoundUp(GetDbl(MKNC_DBL_ZCUT));
			g_dDeep = RoundUp(GetDbl(MKNC_DBL_DEEP));
			// 固定ｻｲｸﾙの切り込み座標ｾｯﾄ
			CNCMake::ms_dCycleZ[0] = RoundUp(GetDbl(MKNC_DBL_DRILLZ));
		}
		else {
			// 強制Z座標補正
			g_dZCut = g_dDeep = RoundUp(pLayer->GetZCut());
			// 固定ｻｲｸﾙの切り込み座標ｾｯﾄ
			CNCMake::ms_dCycleZ[0] = pLayer->IsDrillZ() ?
						g_dZCut : RoundUp(GetDbl(MKNC_DBL_DRILLZ));
		}
		// 固定ｻｲｸﾙその他の座標ｾｯﾄ
		CNCMake::ms_dCycleR[0] = RoundUp(GetDbl(MKNC_DBL_DRILLR));
		CNCMake::ms_dCycleP[0] = GetNum(MKNC_NUM_DWELL);
		InitialCycleBaseVariable();
		// ｸﾞﾛｰﾊﾞﾙ変数に生成対象ｵﾌﾞｼﾞｪｸﾄのｺﾋﾟｰ
		g_mpDXFdata.RemoveAll();
		g_mpDXFtext.RemoveAll();
		g_obPoint.RemoveAll();
		g_obCircle.RemoveAll();
		g_ltDeepGlist.RemoveAll();
		g_obDrillGroup.RemoveAll();
		g_obDrillAxis.RemoveAll();
		SetGlobalMapToLayer(pLayer);
		// 一時領域で使用したﾏｯﾌﾟを削除
		g_mpDXFdata.RemoveAll();

		// 生成開始
#ifdef _DEBUG
		dbg.printf("No.%d ID=%d Name=%s Cut=%f", i+1,
			pLayer->GetListNo(), pLayer->GetStrLayer(), g_dZCut);
#endif
		if ( !MakeNCD_MainFunc(pLayer) ) {
#ifdef _DEBUG
			dbgE.printf("MakeNCD_MainFunc() Error");
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
	g_pfnDeepProc = GetNum(MKNC_NUM_DEEPAPROCESS) == 0 ?
		&MakeLoopDeepAdd_All : &MakeLoopDeepAdd_Euler;

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

	int		i, j;
	CDXFdata*	pData;
	CDXFdata*	pMatchData = NULL;
	CDXFdata*	pDataResult = NULL;
	CDXFcircleEx*	pStartCircle = NULL;
	double		dGap, dGapMin = HUGE_VAL;
	CString		strLayer;
	CLayerData*	pLayer;
	CDXFarray	obArray;
	obArray.SetSize(0, g_pDoc->GetDxfLayerDataCnt(DXFSTRLAYER));

	for ( i=0; i<g_pDoc->GetLayerCnt() && IsThread(); i++ ) {
		pLayer = g_pDoc->GetLayerData(i);
		if ( !pLayer->IsCutTarget() || pLayer->GetLayerType()!=DXFSTRLAYER )
			continue;
		// 加工開始位置指示ﾚｲﾔのﾃｷｽﾄ情報原点調整
		for ( j=0; j<pLayer->GetDxfTextSize() && IsThread(); j++ ) {
			pData = pLayer->GetDxfTextData(j);
			pData->OrgTuning(FALSE);
			g_mpDXFstarttext.SetMakePointMap(pData);
		}
		// 加工開始位置指示ﾚｲﾔの原点調整と移動開始位置検索ﾙｰﾌﾟ(OrgTuning)
		for ( j=0; j<pLayer->GetDxfSize() && IsThread(); j++ ) {
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

void SetGlobalMap(void)
{
	CLayerData*	pLayer;
	// 切削対象ﾚｲﾔ(表示ﾚｲﾔ)をｺﾋﾟｰ
	for ( int i=0; i<g_pDoc->GetLayerCnt() && IsThread(); i++ ) {
		pLayer = g_pDoc->GetLayerData(i);
		if ( pLayer->IsMakeTarget() )
			SetGlobalMapToLayer(pLayer);
	}
	// 一時領域で使用したﾏｯﾌﾟを削除
	g_mpDXFdata.RemoveAll();

	// その他ﾚｲﾔﾃﾞｰﾀのﾏｯﾌﾟ作成
	SetGlobalMapToOther();
}

void SetGlobalMapToLayer(const CLayerData* pLayer)
{
	int		i;
	CDXFdata*	pData;

	// 指定ﾚｲﾔのDXFﾃﾞｰﾀｵﾌﾞｼﾞｪｸﾄ(穴加工のみ)をｺﾋﾟｰ
	for ( i=0; i<pLayer->GetDxfSize() && IsThread(); i++ )
		SetGlobalMapToLayer_Drill(pLayer->GetDxfData(i));

	// ﾃｷｽﾄﾃﾞｰﾀのﾏｯﾌﾟ作成
	for ( i=0; i<pLayer->GetDxfTextSize() && IsThread(); i++ ) {
		pData = pLayer->GetDxfTextData(i);
		pData->OrgTuning(FALSE);
		g_mpDXFtext.SetMakePointMap(pData);
	}
}

void SetGlobalMapToLayer_Drill(CDXFdata* pData)
{
	// 楕円ﾃﾞｰﾀの変身(長径短径が等しい楕円なら)
	if ( GetFlg(MKNC_FLG_ELLIPSE) && pData->GetType()==DXFELLIPSEDATA ) {
		CDXFellipse* pEllipse = (CDXFellipse *)pData;
		if ( fabs( RoundUp(pEllipse->GetLongLength()) - RoundUp(pEllipse->GetShortLength()) ) < EPS )
			pData->ChangeMakeType( ((CDXFellipse *)pData)->GetArc() ?
				DXFARCDATA : DXFCIRCLEDATA);	// 円弧か円ﾃﾞｰﾀに変身
	}

	CDXFarray*	pDummy = NULL;
	CPointD		pt;
	// 各ｵﾌﾞｼﾞｪｸﾄﾀｲﾌﾟごとの処理
	switch ( pData->GetMakeType() ) {
	case DXFPOINTDATA:
		// 重複ﾁｪｯｸ
		if ( GetFlg(MKNC_FLG_DRILLMATCH) ) {
			pt = pData->GetNativePoint(0);
			if ( !g_mpDXFdata.Lookup(pt, pDummy) ) {
				g_obPoint.Add(pData);
				g_mpDXFdata.SetAt(pt, pDummy);
			}
		}
		else
			g_obPoint.Add(pData);
		break;
	case DXFCIRCLEDATA:
		// 円ﾃﾞｰﾀの半径が指定値以下なら
		if ( GetFlg(MKNC_FLG_DRILLCIRCLE) &&
				((CDXFcircle *)pData)->GetMakeR() <= GetDbl(MKNC_DBL_DRILLCIRCLE) ) {
			// 穴加工ﾃﾞｰﾀに変身して登録
			pData->ChangeMakeType(DXFPOINTDATA);
			// ここでは重複ﾁｪｯｸを行わない。
			// 並べ替えた後(OrgTuningDrillCircle)に行う
			g_obCircle.Add(pData);
			break;
		}
		break;
	}
}

void SetGlobalMapToOther(void)
{
	// ここから原点調整するｵﾌﾞｼﾞｪｸﾄは距離計算の必要なし
	int			i, j, nType;
	CLayerData*	pLayer;
	CDXFdata*	pData;

	for ( i=0; i<g_pDoc->GetLayerCnt() && IsThread(); i++ ) {
		pLayer = g_pDoc->GetLayerData(i);
		if ( !pLayer->IsCutTarget() )
			continue;
		nType = pLayer->GetLayerType();
		if ( nType == DXFMOVLAYER ) {
			// 移動指示ﾚｲﾔのﾏｯﾌﾟ生成
			for ( j=0; j<pLayer->GetDxfSize() && IsThread(); j++ ) {
				pData = pLayer->GetDxfData(j);
				pData->OrgTuning(FALSE);
				g_mpDXFmove.SetMakePointMap(pData);
			}
			// 移動指示ﾚｲﾔﾃｷｽﾄのﾏｯﾌﾟ生成
			for ( j=0; j<pLayer->GetDxfTextSize() && IsThread(); j++ ) {
				pData = pLayer->GetDxfTextData(j);
				pData->OrgTuning(FALSE);
				g_mpDXFmovetext.SetMakePointMap(pData);
			}
		}
		else if ( nType == DXFCOMLAYER ) {
			// ｺﾒﾝﾄﾚｲﾔのﾏｯﾌﾟ生成
			for ( j=0; j<pLayer->GetDxfTextSize() && IsThread(); j++ ) {
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

BOOL MakeNCD_MainFunc(const CLayerData* pLayer)
{
#ifdef _DEBUG
	CMagaDbg	dbg("MakeNCD_MainFunc()\nStart", DBG_MAGENTA);
#endif
	CString	strLayer;
	if ( pLayer )
		strLayer = pLayer->GetStrLayer();

	// 加工開始位置指示ﾃﾞｰﾀのｾｯﾄ
	if ( !g_bData && !g_obStartData.IsEmpty() )
		CDXFdata::ms_pData = g_obStartData.GetTail();

	// ﾒｲﾝ分岐
	switch ( GetNum(MKNC_NUM_DRILLPROCESS) ) {
	case 0:		// 先に穴加工
		// 穴加工ﾃﾞｰﾀの準備と最短開始位置のﾃﾞｰﾀ解析
		if ( !CallMakeDrill(pLayer, strLayer) )
			return FALSE;
		// 切削ﾃﾞｰﾀの準備と最短開始位置のﾃﾞｰﾀ解析
		// 穴加工ﾃﾞｰﾀの最後に近いﾃﾞｰﾀ検索
		if ( !CallMakeLoop(MAKECUTTER, pLayer, strLayer) )
			return FALSE;
		break;

	case 1:		// 後で穴加工
		// 切削ﾃﾞｰﾀの準備と最短開始位置のﾃﾞｰﾀ解析
		if ( !CallMakeLoop(MAKECUTTER, pLayer, strLayer) )
			return FALSE;
		// through
	case 2:		// 穴加工のみ
		// 穴加工ﾃﾞｰﾀの準備と最短開始位置のﾃﾞｰﾀ解析
		// 切削ﾃﾞｰﾀの最後に近いﾃﾞｰﾀ検索
		if ( !CallMakeDrill(pLayer, strLayer) )
			return FALSE;
		break;

	default:
		return FALSE;
	}

	// 切削ﾃﾞｰﾀ終了後の移動指示ﾚｲﾔﾁｪｯｸ
	return MakeLoopAddLastMove();
}

BOOL MakeNCD_ShapeFunc(void)
{
	CDXFshape*	pShape;
	CString		strBuf;
	int			nMapCnt = 0;

	// ﾌｪｰｽﾞ1
	// 原点調整とﾏｯﾌﾟの生成ﾌﾗｸﾞをｸﾘｱ
	for ( int i=0; i<g_pDoc->GetLayerCnt() && IsThread(); i++ )
		nMapCnt += g_pDoc->GetLayerData(i)->AllShape_OrgTuning();
	if ( !IsThread() )
		return FALSE;

	// Gｺｰﾄﾞﾍｯﾀﾞ(開始ｺｰﾄﾞ)
	AddCustomCode(g_pMakeOpt->GetStr(MKNC_STR_HEADER), NULL);
	// 加工開始位置指示ﾚｲﾔ処理
	AddMakeStart();
	if ( !g_obStartData.IsEmpty() )
		CDXFdata::ms_pData = g_obStartData.GetTail();
	// 回転数
	strBuf = CNCMake::MakeSpindle(DXFLINEDATA);
	if ( !strBuf.IsEmpty() )
		AddMakeGdata(strBuf);
	// 加工前移動指示の生成
	MakeLoopAddFirstMove(MAKECUTTER);

	// 原点に近い座標ﾏｯﾌﾟを検索
	pShape = GetNearPointShape(CDXFdata::ms_pData->GetEndCutterPoint()+CDXFdata::ms_ptOrg);
	if ( !pShape )
		return TRUE;

	// ﾌｪｰｽﾞ2
	SendFaseMessage(nMapCnt);

	// NC生成ﾙｰﾌﾟ
	return MakeLoopShape(pShape);
}

//////////////////////////////////////////////////////////////////////
// NC生成ﾒｲﾝｽﾚｯﾄﾞ補助関数群
//////////////////////////////////////////////////////////////////////

BOOL CallMakeDrill(const CLayerData* pLayer, CString& strLayer)
{
	// 穴加工処理ﾀｲﾌﾟ
	if ( GetFlg(MKNC_FLG_DRILLCIRCLE) ) {
		// 円ﾃﾞｰﾀも穴加工ﾃﾞｰﾀとする場合
		switch ( GetNum(MKNC_NUM_DRILLCIRCLEPROCESS) ) {
		case 1:	// 円を先に
			if ( !CallMakeLoop(MAKEDRILLCIRCLE, pLayer, strLayer) )
				return FALSE;
			if ( !CallMakeLoop(MAKEDRILLPOINT, pLayer, strLayer) )
				return FALSE;
			break;

		case 2:	// 円を後で
			if ( !CallMakeLoop(MAKEDRILLPOINT, pLayer, strLayer) )
				return FALSE;
			// through
		case 0:	// 実点無視
			if ( !CallMakeLoop(MAKEDRILLCIRCLE, pLayer, strLayer) )
				return FALSE;
			break;
		}
	}
	else {
		// 実点ﾃﾞｰﾀだけ穴加工
		if ( !CallMakeLoop(MAKEDRILLPOINT, pLayer, strLayer) )
			return FALSE;
	}

	// 固定ｻｲｸﾙｷｬﾝｾﾙ
	AddMakeGdataCycleCancel();

	return TRUE;
}

BOOL CallMakeLoop(ENMAKETYPE enMake, const CLayerData* pLayer, CString& strLayer)
{
	CDXFdata*	pData;
	BOOL		bMatch;
	CString		strBuf;

	// ﾌｪｰｽﾞ1 ( OrgTuning_XXX)
	switch( enMake ) {
	case MAKECUTTER:
		tie(pData, bMatch) = OrgTuningCutter(pLayer);
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

	// 加工前移動指示の生成
	if ( !bMatch && enMake!=MAKEDRILLCIRCLE ) {
		// OrgTuning()で現在位置と同一座標が見つからなかった
		CDXFdata* pDataMove = MakeLoopAddFirstMove(enMake);
		if ( pDataMove ) {
			// 移動先に近い切削ﾃﾞｰﾀを再検索
			if ( enMake == MAKECUTTER )
				pData = GetNearPointCutter(pLayer, pDataMove);
			else
				tie(pData, bMatch) = GetNearPointDrill(pDataMove);
		}
	}

	// ﾌｪｰｽﾞ2
	SendFaseMessage();

	// NC生成ﾙｰﾌﾟ
	BOOL	bResult = FALSE;
	switch ( enMake ) {
	case MAKECUTTER:
		bResult = MakeLoopEuler(pLayer, pData);
		break;
	case MAKEDRILLPOINT:
		bResult = MakeLoopDrillPoint(pData);
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

tuple<CDXFdata*, BOOL> OrgTuningCutter(const CLayerData* pLayerTarget)
{
#ifdef _DEBUG
	CMagaDbg	dbg("OrgTuningCutter()", DBG_GREEN);
	int			nDbg = -1;
	CLayerData*	pLayerDbg = NULL;
#endif
	int		i, nCnt = 0, nLayerLoop, nDataLoop;
	BOOL	bMatch = FALSE, bCalc = !g_bData;
	double	dGap, dGapMin = HUGE_VAL;	// 原点までの距離
	CDXFdata*	pDataResult = NULL;
	CDXFdata*	pData;
	CLayerData*	pLayer;

	// ﾌｪｰｽﾞ1(処理件数ｶｳﾝﾄ)
	if ( pLayerTarget ) {
		nLayerLoop = 1;
		nDataLoop  = pLayerTarget->GetDxfSize();
	}
	else {
		nLayerLoop = g_pDoc->GetLayerCnt();
		nDataLoop  = 0;
		for ( i=0; i<nLayerLoop; i++ ) {
			pLayer = g_pDoc->GetLayerData(i);
			if ( pLayer->IsMakeTarget() )
				nDataLoop += pLayer->GetDxfSize();
		}
	}
	SendFaseMessage(nDataLoop);

	// 原点調整と切削開始ﾎﾟｲﾝﾄ検索ﾙｰﾌﾟ
	while ( nLayerLoop-- > 0 && IsThread() ) {
		pLayer = pLayerTarget ? (CLayerData *)pLayerTarget : g_pDoc->GetLayerData(nLayerLoop);
		if ( !pLayer->IsMakeTarget() )
			continue;
		for ( i=0; i<pLayer->GetDxfSize() && IsThread(); i++, nCnt++ ) {
			pData = pLayer->GetDxfData(i);
			if ( pData->GetMakeType() != DXFPOINTDATA ) {
				// 原点調整と距離計算 + NC生成ﾌﾗｸﾞの初期化
				dGap = pData->OrgTuning(bCalc);
				if ( !bCalc )
					dGap = pData->GetEdgeGap(CDXFdata::ms_pData);
				// 原点(または現在位置)に一番近いﾎﾟｲﾝﾄを探す
				if ( dGap < dGapMin ) {
					dGapMin = dGap;
					pDataResult = pData;
#ifdef _DEBUG
					nDbg = i;
					pLayerDbg = pLayer;
#endif
					if ( sqrt(dGap) < CDXFmap::ms_dTolerance )
						bMatch = TRUE;
				}
				// 座標ﾏｯﾌﾟに登録
				g_mpDXFdata.SetMakePointMap(pData);
			}
			SendProgressPos(nCnt);
		}
	}

#ifdef _DEBUG
	if ( pLayerDbg ) {
		dbg.printf("FirstPoint Layer=%s Cnt=%d Gap=%f",
			pLayerDbg->GetStrLayer(), nDbg, dGapMin);
	}
	else {
		dbg.printf("FirstPoint Cnt=%d Gap=%f",
			nDbg, dGapMin);
	}
#endif
	g_pParent->m_ctReadProgress.SetPos(nDataLoop);

	return make_tuple(pDataResult, bMatch);
}

tuple<CDXFdata*, BOOL> OrgTuningDrillPoint(void)
{
#ifdef _DEBUG
	CMagaDbg	dbg("OrgTuningDrillPoint()", DBG_GREEN);
#endif
	CDXFdata*	pDataResult = NULL;
	if ( g_obPoint.IsEmpty() )
		return make_tuple(pDataResult, FALSE);

	int		i, nLoop = g_obPoint.GetSize();
	BOOL	bMatch = FALSE, bCalc;
	double	dGap, dGapMin = HUGE_VAL;	// 原点までの距離
	CDXFdata*	pData;

	// ﾌｪｰｽﾞ1
	SendFaseMessage(nLoop);

	// 最適化に伴う配列のｺﾋﾟｰ先
	CDXFsort& obDrill = GetNum(MKNC_NUM_OPTIMAIZEDRILL) == 0 ? g_obDrillAxis : g_obDrillGroup;
	obDrill.SetSize(0, nLoop);

	// 原点調整と切削開始ﾎﾟｲﾝﾄ検索ﾙｰﾌﾟ
#ifdef _DEBUG
	int	nDbg = -1;
#endif

	// 切削開始ﾎﾟｲﾝﾄの計算が必要かどうか
	if ( g_bData )
		bCalc = FALSE;
	else
		bCalc = GetNum(MKNC_NUM_OPTIMAIZEDRILL) == 0 ? TRUE : FALSE;

	// 穴加工ｵﾌﾞｼﾞｪｸﾄを g_obDrillGroup にｺﾋﾟｰ
	for ( i=0; i<nLoop && IsThread(); i++ ) {
		pData = g_obPoint[i];
		// 原点調整と距離計算 + NC生成ﾌﾗｸﾞの初期化
		dGap = pData->OrgTuning(bCalc);
		// 原点に一番近いﾎﾟｲﾝﾄを探す
		if ( !pData->IsMakeFlg() ) {
			obDrill.Add(pData);
			if ( !bCalc )
				dGap = pData->GetEdgeGap(CDXFdata::ms_pData);
			if ( dGap < dGapMin ) {
				dGapMin = dGap;
				pDataResult = pData;
#ifdef _DEBUG
				nDbg = i;
#endif
				if ( sqrt(dGap) < GetDbl(MKNC_DBL_TOLERANCE) )
					bMatch = TRUE;
			}
		}
		SendProgressPos(i);
	}
#ifdef _DEBUG
	dbg.printf("FirstPoint %d Gap=%f", nDbg, dGapMin);
#endif

	g_pParent->m_ctReadProgress.SetPos(nLoop);

	return make_tuple(pDataResult, bMatch);
}

CDXFdata* OrgTuningDrillCircle(void)
{
#ifdef _DEBUG
	CMagaDbg	dbg("OrgTuningDrillCircle()", DBG_GREEN);
#endif
	if ( g_obCircle.IsEmpty() )
		return NULL;

	int		i, j, nLoop = g_obCircle.GetSize();
	CPointD	pt;
	CDXFdata*	pData1;
	CDXFdata*	pData2;
	SendFaseMessage(nLoop);

	// ﾌｪｰｽﾞ1 原点調整
	for ( i=0; i<nLoop && IsThread(); i++ ) {
		g_obCircle[i]->OrgTuning(FALSE);	// 並べ替えるので近接計算は不要
		SendProgressPos(i);
	}
	g_pParent->m_ctReadProgress.SetPos(nLoop);
	// ﾌｪｰｽﾞ2 並べ替え
	if ( GetNum(MKNC_NUM_DRILLSORT) == 0 )
		g_obCircle.Sort(CircleSizeCompareFunc1);	// 昇順
	else
		g_obCircle.Sort(CircleSizeCompareFunc2);	// 降順

	if ( GetFlg(MKNC_FLG_DRILLMATCH) ) {
		// ﾌｪｰｽﾞ3 重複ﾁｪｯｸ
		SendFaseMessage(nLoop);
		for ( i=0; i<nLoop && IsThread(); i++ ) {
			pData1 = g_obCircle[i];
			if ( !pData1->IsMakeFlg() ) {
				pt = pData1->GetStartMakePoint();
				for ( j=i+1; j<nLoop && IsThread(); j++ ) {
					pData2 = g_obCircle[j];
					if ( pt == pData2->GetStartMakePoint() ) 
						pData2->SetMakeFlg();
				}
			}
			SendProgressPos(i);
		}
		g_pParent->m_ctReadProgress.SetPos(nLoop);
	}

	// ﾀﾞﾐｰﾃﾞｰﾀを返す
	return g_obCircle[0];
}

//////////////////////////////////////////////////////////////////////

BOOL MakeLoopEuler(const CLayerData* pLayer, CDXFdata* pData)
{
	// ---------------------------------------------------
	// MakeLoopEuler() の処理全般は，精度の高い座標を渡す
	// ---------------------------------------------------
	CDXFdata*	pDataMove;
	CDXFmap		mpEuler;		// 一筆書きｵﾌﾞｼﾞｪｸﾄを格納
	BOOL		bMatch, bMove, bCust;
	int			i, nCnt, nPos = 0, nSetPos = 64;
	CPointD		pt;

	i = GetPrimeNumber( g_pDoc->GetDxfLayerDataCnt(DXFCAMLAYER)*2 );
	mpEuler.InitHashTable(max(17, i));

	// GetNearPoint() の結果が NULL になるまで
	while ( pData && IsThread() ) {
		mpEuler.RemoveAll();
		// この pData を基点に
		mpEuler.SetMakePointMap(pData);
		pData->SetSearchFlg();
		// pData の始点・終点で一筆書き探索
		for ( i=0; i<pData->GetPointNumber(); i++ ) {
			pt = pData->GetTunPoint(i);
			if ( !MakeLoopEulerSearch(pt, mpEuler) )
				return FALSE;
		}
		// CMapｵﾌﾞｼﾞｪｸﾄのNC生成
		if ( (nCnt=MakeLoopEulerAdd(&mpEuler)) < 0 )
			return FALSE;
		//
		nPos += nCnt;
		if ( nSetPos < nPos ) {
			g_pParent->m_ctReadProgress.SetPos(nPos);
			while ( nSetPos < nPos )
				nSetPos += nSetPos;
		}

		// 移動指示ﾚｲﾔのﾁｪｯｸ
		bCust = TRUE;
		bMove = FALSE;
		pDataMove = CDXFdata::ms_pData;
		bMatch = GetMatchPointMove(pDataMove);
		while ( bMatch && IsThread() ) {
			if ( GetFlg(MKNC_FLG_DEEP) && GetNum(MKNC_NUM_DEEPAPROCESS)==0 ) {
				// 最後にZ軸移動のﾏｰｶｰがあれば削除
				if ( !g_ltDeepGlist.IsEmpty() && g_ltDeepGlist.GetTail() == NULL )
					g_ltDeepGlist.RemoveTail();
				// 移動ﾃﾞｰﾀ登録
				g_ltDeepGlist.AddTail(pDataMove);
				pDataMove->SetMakeFlg();
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
				AddMakeMove(pDataMove);
			}
			// 移動ﾃﾞｰﾀを待避
			CDXFdata::ms_pData = pDataMove;
			// 次の移動ﾃﾞｰﾀ検索
			bMatch = GetMatchPointMove(pDataMove);
		}
		// 移動ﾃﾞｰﾀ後のｶｽﾀﾑｺｰﾄﾞ挿入
		if ( bMove && IsThread() ) {
			// 移動ﾃﾞｰﾀの終点でﾃｷｽﾄﾃﾞｰﾀの生成
			pt = pDataMove->GetEndMakePoint();
			AddMoveTextIntegrated(pt);
			// ｶｽﾀﾑｺｰﾄﾞ
			(*g_pfnAddMoveCust_A)();
		}

		// 次の切削ﾎﾟｲﾝﾄ検索
		pData = GetNearPointCutter(pLayer, pDataMove);

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

BOOL MakeLoopEulerSearch(const CPointD& ptKey, CDXFmap& mpEuler)
{
	int			i, j;
	CPointD		pt;
	CDXFarray*	pobArray;
	CDXFdata*	pData;

	// 座標をｷｰに全体のﾏｯﾌﾟ検索
	if ( g_mpDXFdata.Lookup(const_cast<CPointD&>(ptKey), pobArray) ) {
		for ( i=0; i<pobArray->GetSize() && IsThread(); i++ ) {
			pData = pobArray->GetAt(i);
			if ( !pData->IsMakeFlg() && !pData->IsSearchFlg() ) {
				mpEuler.SetMakePointMap(pData);
				pData->SetSearchFlg();
				// ｵﾌﾞｼﾞｪｸﾄの端点をさらに検索
				for ( j=0; j<pData->GetPointNumber() && IsThread(); j++ ) {
					pt = pData->GetTunPoint(j);
					if ( ptKey != pt ) {
						if ( !MakeLoopEulerSearch(pt, mpEuler) )
							break;
					}
				}
			}
		}
	}

	return IsThread();
}

int MakeLoopEulerAdd(const CDXFmap* pEuler)
{
#ifdef _DEBUG
	CMagaDbg	dbg("MakeLoopEulerAdd()", DBG_MAGENTA);
#endif
	BOOL		bEuler = FALSE;		// 一筆書き要件を満たしているか
	POSITION	pos;
	CPointD		pt, ptNow(CDXFdata::ms_pData->GetEndCutterPoint());
	CDXFdata*	pData;
	CDXFarray*	pStartArray;
	CDXFlist	ltEuler;			// 一筆書き順

#ifdef _DEBUGOLD
	CDumpContext	dc;
	dc.SetDepth(1);
	pEuler->Dump(dc);
#endif

	// この座標ﾏｯﾌﾟが一筆書き要件を満たしているか
	tie(bEuler, pStartArray, pt) = pEuler->IsEulerRequirement(ptNow);
	if ( !IsThread() )
		return -1;
#ifdef _DEBUG
	dbg.printf("FirstPoint x=%f y=%f Euler=%d", pt.x, pt.y, bEuler);
#endif

	// --- 一筆書きの生成(再帰呼び出しによる木構造解析)
	ASSERT( pStartArray );
	ASSERT( !pStartArray->IsEmpty() );
	if ( !MakeLoopEulerAdd_with_one_stroke(pEuler, bEuler, FALSE, pt, pStartArray, ltEuler) ) {
		// 一筆書きできるハズやけど失敗したら条件緩和してやり直し
		bEuler = FALSE;
		MakeLoopEulerAdd_with_one_stroke(pEuler, bEuler, FALSE, pt, pStartArray, ltEuler);
	}

	// --- 切削ﾃﾞｰﾀ生成
	ASSERT( !ltEuler.IsEmpty() );
	if ( GetFlg(MKNC_FLG_DEEP) ) {
		// 切削ﾃﾞｰﾀ生成
		for ( pos=ltEuler.GetHeadPosition(); pos && IsThread(); ) {
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
		for ( pos=ltEuler.GetHeadPosition(); pos && IsThread(); ) {
			pData = ltEuler.GetNext(pos);
			AddMakeGdata(pData);
		}
		// 最後に生成したﾃﾞｰﾀを待避
		CDXFdata::ms_pData = pData;
	}

	// 一筆書き要件を満たしていないときだけ
	if ( !bEuler )
		pEuler->AllMapObject_ClearSearchFlg();	// 一筆書きに漏れたｵﾌﾞｼﾞｪｸﾄのｻｰﾁﾌﾗｸﾞを初期化

	return ltEuler.GetCount();
}

BOOL MakeLoopEulerAdd_with_one_stroke
	(const CDXFmap* pEuler, BOOL bEuler, BOOL bMakeShape, 
		const CPointD& pt, const CDXFarray* pArray, CDXFlist& ltEuler)
{
	int			i, nLoop = pArray->GetSize();
	CDXFdata*	pData;
	CDXFarray*	pNextArray;
	CPointD		ptNext;
	POSITION	pos, posTail = ltEuler.GetTailPosition();	// この時点での仮登録ﾘｽﾄの最後

	// まずこの座標配列の円(に準拠する)ﾃﾞｰﾀを仮登録
	for ( i=0; i<nLoop && IsThread(); i++ ) {
		pData = pArray->GetAt(i);
		if ( !pData->IsSearchFlg() && pData->IsStartEqEnd() ) {
			pData->GetEdgeGap(pt);	// pt値に近い方をｵﾌﾞｼﾞｪｸﾄの始点に入れ替え
			ltEuler.AddTail( pData );
			pData->SetSearchFlg();
		}
	}

	// 円以外のﾃﾞｰﾀで木構造の次を検索
	for ( i=0; i<nLoop && IsThread(); i++ ) {
		pData = pArray->GetAt(i);
		if ( !pData->IsSearchFlg() ) {
			pData->GetEdgeGap(pt);
			ltEuler.AddTail(pData);
			pData->SetSearchFlg();
			ptNext = pData->GetEndCutterPoint();
			if ( bMakeShape ) {
				if ( !pEuler->Lookup(ptNext+CDXFdata::ms_ptOrg, pNextArray) )
					NCVC_CriticalErrorMsg(__FILE__, __LINE__);
			}
			else {
				if ( !pEuler->Lookup(ptNext, pNextArray) )
					NCVC_CriticalErrorMsg(__FILE__, __LINE__);
			}
			// 次の座標配列を検索
			if ( MakeLoopEulerAdd_with_one_stroke(pEuler, bEuler, bMakeShape, ptNext, pNextArray, ltEuler) )
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
		if ( pEuler->IsAllSearchFlg() )
			return TRUE;	// 全件終了
	}
	// この座標配列の検索が終了したので木構造の上位へ移動．
	// 円ﾃﾞｰﾀを含む全ての仮登録ﾘｽﾄを削除
	if ( posTail ) {
		ltEuler.GetNext(posTail);	// posTailの次から
		for ( pos=posTail; pos && IsThread(); pos=posTail) {
			pData = ltEuler.GetNext(posTail);	// 先に次の要素を取得
			pData->ClearSearchFlg();			// ﾌﾗｸﾞを消して
			ltEuler.RemoveAt(pos);				// 要素削除
		}
	}

	return FALSE;
}

BOOL MakeLoopShape(CDXFshape* pShape)
{
#ifdef _DEBUG
	CMagaDbg	dbg("MakeLoopShape()", DBG_RED);
#endif
	CDXFdata*	pData;
	const	CPointD		ptOrg(CDXFdata::ms_ptOrg);
			CPointD		pt;
	int		nCnt, nPos = 0;

	while ( pShape && IsThread() ) {
#ifdef _DEBUG
		dbg.printf("ParentMapName=%s", pShape->GetShapeName());
#endif
		// ﾏｯﾌﾟの内側から生成
		pShape->SetShapeFlag(DXFMAPFLG_SEARCH);	// 親ﾏｯﾌﾟは検索対象外
		if ( (nCnt=MakeLoopShapeSearch(pShape)) < 0 )
			return FALSE;
		// 親ﾏｯﾌﾟ自身の生成
		pt = CDXFdata::ms_pData->GetEndCutterPoint() + ptOrg;
		pShape->GetSelectObjectFromShape(pt, NULL, &pData);	// 戻り値不要
		if ( !MakeLoopShapeAdd(pShape, pData) )
			return FALSE;
		// ﾌﾟﾛｸﾞﾚｽﾊﾞｰの更新
		nPos += nCnt+1;
		g_pParent->m_ctReadProgress.SetPos(nPos);
		// 次のﾏｯﾌﾟを検索
		pt = CDXFdata::ms_pData->GetEndCutterPoint() + ptOrg;
		pShape = GetNearPointShape(pt);		// ﾈｲﾃｨﾌﾞ座標で検索
	}

	// 全体深彫の後処理
	if ( GetFlg(MKNC_FLG_DEEP) && GetNum(MKNC_NUM_DEEPAPROCESS)==0 ) {
		if ( !MakeLoopDeepAdd() )
			return FALSE;
	}

	return IsThread();
}

int MakeLoopShapeSearch(const CDXFshape* pShapeBase)
{
#ifdef _DEBUG
	CMagaDbg	dbg("MakeLoopShapeSearch()", DBG_RED);
#endif

	CLayerData*	pLayer;
	CDXFshape*	pShape;
	CShapeArray	obShape;	// 内側の座標ﾏｯﾌﾟ一覧
	CRectD	rcBase( pShapeBase->GetMaxRect() );
	int		i, j, nCnt = 0;
	obShape.SetSize(0, 64);

	// pShapeBaseより内側の矩形を持つ座標ﾏｯﾌﾟを検索
	for ( i=0; i<g_pDoc->GetLayerCnt() && IsThread(); i++ ) {
		pLayer = g_pDoc->GetLayerData(i);
		for ( j=0; j<pLayer->GetShapeSize(); j++ ) {
			pShape = pLayer->GetShapeData(j);
			if ( !pShape->IsMakeFlg() && !pShape->IsSearchFlg() &&
					pShape->GetShapeAssemble()!=DXFSHAPE_EXCLUDE &&
					rcBase.PtInRect(pShape->GetMaxRect()) ) {
				pShape->SetShapeFlag(DXFMAPFLG_SEARCH);
				obShape.Add(pShape);
				nCnt += MakeLoopShapeSearch(pShape);
			}
		}
	}

	if ( obShape.IsEmpty() || !IsThread() )
		return 0;

	// obShapeに蓄積されたﾃﾞｰﾀの生成
	const	CPointD		ptOrg(CDXFdata::ms_ptOrg);
			CPointD		pt(CDXFdata::ms_pData->GetEndCutterPoint() + ptOrg);
	double		dGap, dGapMin;
	CDXFdata*	pData;
	CDXFdata*	pDataResult;
	CDXFshape*	pShapeResult;

	pShapeResult = obShape[0];
	j = obShape.GetSize();

	while ( pShapeResult && IsThread() ) {
		dGapMin = HUGE_VAL;
		pShapeResult = NULL;
		// ptに一番近いﾈｲﾃｨﾌﾞの座標ﾏｯﾌﾟを検索
		for ( i=0; i<j && IsThread(); i++ ) {
			pShape = obShape[i];
			if ( !pShape->IsMakeFlg() ) {
				dGap = pShape->GetSelectObjectFromShape(pt, NULL, &pData);
				if ( dGap < dGapMin ) {
					dGapMin = dGap;
					pDataResult = pData;
					pShapeResult = pShape;
				}
			}
		}
		if ( pShapeResult ) {
			// 座標ﾏｯﾌﾟのNC生成
			if ( !MakeLoopShapeAdd(pShapeResult, pDataResult) )
				return -1;
			// pt座標の更新
			pt = CDXFdata::ms_pData->GetEndCutterPoint() + ptOrg;
		}
	}

	return nCnt;
}

BOOL MakeLoopShapeAdd(CDXFshape* pShape, CDXFdata* pData)
{
#ifdef _DEBUG
	CMagaDbg	dbg("MakeLoopShapeAdd()", DBG_RED);
	dbg.printf("MapName=%s", pShape->GetShapeName());
#endif
	BOOL	bResult;

	bResult = pShape->GetShapeType() == 0 ?
		MakeLoopShapeAdd_ChainList( pShape, pData ) :
		MakeLoopShapeAdd_EulerMap ( pShape );

	pShape->SetShapeFlag(DXFMAPFLG_MAKE);

	return bResult;
}

BOOL MakeLoopShapeAdd_ChainList(CDXFshape* pShape, CDXFdata* pData)
{
	const	CPointD		ptOrg( CDXFdata::ms_ptOrg );
	const	CPointD		ptNow( CDXFdata::ms_pData->GetEndCutterPoint() );
	double	dGap1, dGap2;
	BOOL	bReverse = FALSE, bNext = FALSE;
	CDXFworking*	pWork;
	CDXFdata*		pDataFix;
	CDXFchain*		pChain = pShape->GetOutlineObject();

	if ( pChain ) {
		// 輪郭ｵﾌﾞｼﾞｪｸﾄ用原点調整
		pChain->OrgTuning();
		// 輪郭ｵﾌﾞｼﾞｪｸﾄで近いﾃﾞｰﾀを検索
		pChain->GetSelectObjectFromShape(ptNow+ptOrg, NULL, &pData);
	}
	else {
		pChain = pShape->GetShapeChain();	// ｵﾘｼﾞﾅﾙｵﾌﾞｼﾞｪｸﾄ
		// 近接ｵﾌﾞｼﾞｪｸﾄ(pData)は、上位で検索済み
	}
	ASSERT( pData );

	// 方向指示および生成順のﾁｪｯｸ
	tie(pWork, pDataFix) = pShape->GetDirectionObject();
	if ( pChain->GetCount()==1 && pData->GetMakeType()==DXFCIRCLEDATA ) {
		if ( pDataFix ) {
			CPointD	pts( ((CDXFworkingDirection *)pWork)->GetStartPoint() ),
					pte( ((CDXFworkingDirection *)pWork)->GetArrowPoint() );
			// ｵﾘｼﾞﾅﾙｵﾌﾞｼﾞｪｸﾄで回転設定
			CDXFcircle*	pDataOrig = (CDXFcircle *)(pShape->GetShapeChain()->GetHead());
			pDataOrig->SetRoundFixed(pts, pte);
			// 輪郭ｵﾌﾞｼﾞｪｸﾄがあれば、同じ回転設定
			if ( pShape->GetOutlineObject() )
				((CDXFcircle *)pData)->SetRoundFixed(pDataOrig->GetRound());
		}
		pData->GetEdgeGap(ptNow);	// 輪郭ｵﾌﾞｼﾞｪｸﾄの近接座標計算
	}
	else {
		dGap1 = GAPCALC(pData->GetStartCutterPoint() - ptNow);
		dGap2 = GAPCALC(pData->GetEndCutterPoint()   - ptNow);
		if ( pDataFix ) {
			CPointD	ptFix( ((CDXFworkingDirection *)pWork)->GetArrowPoint() - ptOrg );
			if ( GAPCALC(pDataFix->GetEndCutterPoint() - ptFix) > EPS ) {
				bReverse = TRUE;
				// 開始ｵﾌﾞｼﾞｪｸﾄの終点(bReverseなので始点で判断)
				// の方が近い場合は、次のｵﾌﾞｼﾞｪｸﾄから開始
				if ( dGap1 < dGap2 )
					bNext = TRUE;
			}
			else {
				if ( dGap1 > dGap2 )
					bNext = TRUE;
			}
		}
		else {
			if ( dGap1 > dGap2 )
				bReverse = TRUE;
		}
		if ( bReverse )
			pChain->ReversPoint();
	}

	// 開始ｵﾌﾞｼﾞｪｸﾄの検索
	POSITION	pos1, pos2,
				(CDXFchain::*pfnGetPosition)(void) const;
	CDXFdata*&	(CDXFchain::*pfnGetData)(POSITION&);
	for ( pos1=pChain->GetHeadPosition(); (pos2=pos1) && IsThread(); ) {
		if ( pData == pChain->GetNext(pos1) )
			break;
	}
	pos1 = pos2;

	if ( bReverse ) {
		pfnGetPosition	= &(CDXFchain::GetTailPosition);
		pfnGetData		= &(CDXFchain::GetPrev);
	}
	else {
		pfnGetPosition	= &(CDXFchain::GetHeadPosition);
		pfnGetData		= &(CDXFchain::GetNext);
	}

	// 開始ｵﾌﾞｼﾞｪｸﾄの最終調整
	if ( bNext ) {
		for ( int i=0; i<2; i++ ) {
			pos2 = pos1;
			pData = (pChain->*pfnGetData)(pos1);
			if ( !pos1 )
				pos1 = (pChain->*pfnGetPosition)();
		}
		pos1 = pos2;
	}

	// 切削ﾃﾞｰﾀ生成
	if ( GetFlg(MKNC_FLG_DEEP) ) {
		// 開始ﾎﾟｼﾞｼｮﾝからﾙｰﾌﾟ
		while ( pos1 && IsThread() ) {
			pData = (pChain->*pfnGetData)(pos1);
			g_ltDeepGlist.AddTail(pData);
		}
		// [先頭|終端]から開始ｵﾌﾞｼﾞｪｸﾄまで
		for ( pos1=(pChain->*pfnGetPosition)(); pos1!=pos2 && IsThread(); ) {
			pData = (pChain->*pfnGetData)(pos1);
			g_ltDeepGlist.AddTail(pData);
		}
		// 深彫が全体か否か
		if ( GetNum(MKNC_NUM_DEEPAPROCESS) == 0 ) {
			// 最後に生成したﾃﾞｰﾀを待避
			CDXFdata::ms_pData = pData;
			// Z軸移動のﾏｰｶｰ
			g_ltDeepGlist.AddTail((CDXFdata *)NULL);
		}
		else
			return MakeLoopDeepAdd();
	}
	else {
		// 切削ﾃﾞｰﾀまでの移動
		if ( GetNum(MKNC_NUM_TOLERANCE) == 0 )
			AddMoveGdataG0(pData);
		else
			AddMoveGdataG1(pData);
		// 開始ﾎﾟｼﾞｼｮﾝからﾙｰﾌﾟ
		while ( pos1 && IsThread() ) {
			pData = (pChain->*pfnGetData)(pos1);
			AddMakeGdata(pData);
		}
		// [先頭|終端]から開始ｵﾌﾞｼﾞｪｸﾄまで
		for ( pos1=(pChain->*pfnGetPosition)(); pos1!=pos2 && IsThread(); ) {
			pData = (pChain->*pfnGetData)(pos1);
			AddMakeGdata(pData);
		}
		// 最後に生成したﾃﾞｰﾀを待避
		CDXFdata::ms_pData = pData;
		// Z軸上昇
		AddMoveGdataZup();
	}

	return TRUE;
}

BOOL MakeLoopShapeAdd_EulerMap(CDXFshape* pShape)
{
	BOOL		bEuler = FALSE;
	CDXFmap*	pEuler = pShape->GetShapeMap();
	// １回目の生成処理
	if ( !MakeLoopShapeAdd_EulerMap_Make( pShape, pEuler, bEuler ) )
		return FALSE;
	if ( bEuler )
		return TRUE;	// １回目で全て生成完了

	// 生成漏れのﾃﾞｰﾀ処理
	int			i;
	POSITION	pos;
	CDXFdata*	pData;
	CDXFdata*	pDataResult;
	CDXFarray*	pArray;
	CDXFmap		mpLeak;
	double		dGap, dGapMin;
	CPointD		pt, ptKey;
	while ( IsThread() ) {
		// 現在位置に近いｵﾌﾞｼﾞｪｸﾄ検索
		pDataResult = NULL;
		dGapMin = HUGE_VAL;
		pt = CDXFdata::ms_pData->GetEndCutterPoint();
		for ( pos=pEuler->GetStartPosition(); pos; ) {
			pEuler->GetNextAssoc(pos, ptKey, pArray);
			for ( i=0; i<pArray->GetSize(); i++ ) {
				pData = pArray->GetAt(i);
				if ( pData->IsMakeFlg() )
					continue;
				pData->ClearSearchFlg();
				dGap = pData->GetEdgeGap(pt);
				if ( dGap < dGapMin ) {
					dGap = dGapMin;
					pDataResult = pData;
				}
			}
		}
		if ( !pDataResult )		// ﾙｰﾌﾟ終了条件
			break;
		// 仮座標ﾏｯﾌﾟを生成
		mpLeak.RemoveAll();
		mpLeak.SetMakePointMap(pDataResult);
		pDataResult->SetSearchFlg();
		for ( i=0; i<pDataResult->GetPointNumber(); i++ ) {
			pt = pDataResult->GetNativePoint(i);
			if ( !MakeLoopShapeAdd_EulerMap_Search(pt, pEuler, &mpLeak) )
				return FALSE;
		}
		// ２回目以降の生成処理
		bEuler = FALSE;
		if ( !MakeLoopShapeAdd_EulerMap_Make( pShape, &mpLeak, bEuler ) )
			return FALSE;
	}
	return TRUE;
}

BOOL MakeLoopShapeAdd_EulerMap_Make(CDXFshape* pShape, CDXFmap* pEuler, BOOL& bEuler)
{
	// MakeLoopEulerAdd() 参考
	BOOL		bReverse = FALSE;
	POSITION	pos;
	const	CPointD	ptOrg(CDXFdata::ms_ptOrg);
	CPointD		pt;
	CDXFdata*	pData;
	CDXFdata*	pDataFix;
	CDXFarray*	pArray;
	CDXFworking*	pWork;
	CDXFchain	ltEuler;

	// この座標ﾏｯﾌﾟが一筆書き要件を満たしているか
	tie(bEuler, pArray, pt) = pEuler->IsEulerRequirement(CDXFdata::ms_pData->GetEndCutterPoint()+ptOrg);
	if ( !IsThread() )
		return -1;
	pt -= ptOrg;

	// --- 一筆書きの生成(再帰呼び出しによる木構造解析)
	ASSERT( pArray );
	ASSERT( !pArray->IsEmpty() );
	if ( !MakeLoopEulerAdd_with_one_stroke(pEuler, bEuler, TRUE, pt, pArray, ltEuler) ) {
		// 一筆書きできるハズやけど失敗したら条件緩和してやり直し
		bEuler = FALSE;
		MakeLoopEulerAdd_with_one_stroke(pEuler, bEuler, TRUE, pt, pArray, ltEuler);
	}

	// 方向指示および生成順のﾁｪｯｸ
	tie(pWork, pDataFix) = pShape->GetDirectionObject();
	if ( pDataFix ) {
		// 方向指示がltEulerに含まれる場合だけﾁｪｯｸ
		for ( pos=ltEuler.GetHeadPosition(); pos; ) {
			pData = ltEuler.GetNext(pos);
			if ( pDataFix == pData ) {
				if ( pData->GetMakeType() == DXFCIRCLEDATA ) {
					CPointD	pts( ((CDXFworkingDirection *)pWork)->GetStartPoint() ),
							pte( ((CDXFworkingDirection *)pWork)->GetArrowPoint() );
					((CDXFcircle *)pData)->SetRoundFixed(pts, pte);
				}
				else {
					CPointD	ptFix( ((CDXFworkingDirection *)pWork)->GetArrowPoint() - ptOrg );
					if ( GAPCALC(pData->GetEndCutterPoint() - ptFix) > EPS )
						bReverse = TRUE;
				}
				break;
			}
		}
	}

	POSITION	(CDXFchain::*pfnGetPosition)(void) const;
	CDXFdata*&	(CDXFchain::*pfnGetData)(POSITION&);
	CDXFdata*&	(CDXFchain::*pfnGetFirst)();
	if ( bReverse ) {
		ltEuler.ReversPoint();
		pfnGetPosition	= &(CDXFchain::GetTailPosition);
		pfnGetData		= &(CDXFchain::GetPrev);
		pfnGetFirst		= &(CDXFchain::GetTail);
	}
	else {
		pfnGetPosition	= &(CDXFchain::GetHeadPosition);
		pfnGetData		= &(CDXFchain::GetNext);
		pfnGetFirst		= &(CDXFchain::GetHead);
	}

	// --- 切削ﾃﾞｰﾀ生成
	ASSERT( !ltEuler.IsEmpty() );
	if ( GetFlg(MKNC_FLG_DEEP) ) {
		// 切削ﾃﾞｰﾀ生成
		for ( pos=(ltEuler.*pfnGetPosition)(); pos && IsThread(); ) {
			pData = (ltEuler.*pfnGetData)(pos);
			pData->SetMakeFlg();
		}
		g_ltDeepGlist.AddTail(&ltEuler);
		// 深彫が全体か否か
		if ( GetNum(MKNC_NUM_DEEPAPROCESS) == 0 ) {
			// 最後に生成したﾃﾞｰﾀを待避
			CDXFdata::ms_pData = g_ltDeepGlist.GetTail();
			// Z軸移動のﾏｰｶｰ
			g_ltDeepGlist.AddTail((CDXFdata *)NULL);
		}
		else {
			if ( !MakeLoopDeepAdd() )
				return FALSE;
		}
	}
	else {
		// 切削ﾃﾞｰﾀまでの移動
		pData = (ltEuler.*pfnGetFirst)();
		if ( GetNum(MKNC_NUM_TOLERANCE) == 0 )
			AddMoveGdataG0(pData);
		else
			AddMoveGdataG1(pData);
		// 切削ﾃﾞｰﾀ生成
		for ( pos=(ltEuler.*pfnGetPosition)(); pos && IsThread(); ) {
			pData = (ltEuler.*pfnGetData)(pos);
			AddMakeGdata(pData);
		}
		// 最後に生成したﾃﾞｰﾀを待避
		CDXFdata::ms_pData = pData;
		// Z軸上昇
		AddMoveGdataZup();
	}

	return TRUE;
}

BOOL MakeLoopShapeAdd_EulerMap_Search
	(const CPointD& ptKey, CDXFmap* pOrgMap, CDXFmap* pResultMap)
{
	int			i, j;
	CPointD		pt;
	CDXFarray*	pobArray;
	CDXFdata*	pData;

	// 座標をｷｰに全体のﾏｯﾌﾟ検索
	if ( pOrgMap->Lookup(const_cast<CPointD&>(ptKey), pobArray) ) {
		for ( i=0; i<pobArray->GetSize() && IsThread(); i++ ) {
			pData = pobArray->GetAt(i);
			if ( !pData->IsMakeFlg() && !pData->IsSearchFlg() ) {
				pResultMap->SetMakePointMap(pData);
				pData->SetSearchFlg();
				// ｵﾌﾞｼﾞｪｸﾄの端点をさらに検索
				for ( j=0; j<pData->GetPointNumber() && IsThread(); j++ ) {
					pt = pData->GetNativePoint(j);
					if ( ptKey != pt ) {
						if ( !MakeLoopShapeAdd_EulerMap_Search(pt, pOrgMap, pResultMap) )
							break;
					}
				}
			}
		}
	}

	return IsThread();
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

#ifdef _DEBUG
	int	n;
	g_dbg.printf("LayerName=%s", g_ltDeepGlist.GetHead()->GetParentLayer()->GetStrLayer());
	for ( POSITION p=g_ltDeepGlist.GetHeadPosition(); p; ) {
		pData = g_ltDeepGlist.GetNext(p);
		if ( pData )
			n = pData->GetParentLayer()->IsCutType() ? 1 : 2;
		else
			n = 0;
		g_dbg.printf("ListType=%d", n);
	}
#endif

	// 回転数
	CString	strSpindle( CNCMake::MakeSpindle(DXFLINEDATA) );
	if ( !strSpindle.IsEmpty() )
		AddMakeGdata(strSpindle);
	// 切削ﾃﾞｰﾀまでの移動
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
	// g_dZCut - g_dDeep > NCMIN とした
	while ( g_dZCut - g_dDeep > NCMIN && IsThread() ) {
		// 深彫生成処理
		pData = (*g_pfnDeepProc)(bAction, FALSE);
		CDXFdata::ms_pData = pData;
		// ｱｸｼｮﾝの切り替え(往復切削のみ)
		if ( GetNum(MKNC_NUM_DEEPCPROCESS) == 0 ) {
			bAction = !bAction;
			// 各ｵﾌﾞｼﾞｪｸﾄの始点終点を入れ替え
			for ( pos=g_ltDeepGlist.GetHeadPosition(); pos && IsThread(); ) {
				pData = g_ltDeepGlist.GetNext(pos);
				if ( pData )
					pData->ReversePt();
			}
		}
		// Z軸の下降
		g_dZCut += GetDbl(MKNC_DBL_ZSTEP);
		if ( pData->GetParentLayer()->IsCutType() && g_dZCut-g_dDeep > NCMIN ) {
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
	CDXFdata::ms_pData = (*g_pfnDeepProc)(bAction, bFinish);

	// 深彫切削におけるZ軸の上昇
	MakeLoopDeepZUp();

	// 後始末
	g_ltDeepGlist.RemoveAll();
	g_dZCut = dZCut;

	return IsThread();
}

CDXFdata* MakeLoopDeepAdd_Euler(BOOL bAction, BOOL bDeep)
{
	POSITION	(CDXFlist::*pfnGetPosition)(void) const;
	CDXFdata*&	(CDXFlist::*pfnGetData)(POSITION&);
	if ( bAction ) {
		pfnGetPosition	= &(CDXFlist::GetHeadPosition);
		pfnGetData		= &(CDXFlist::GetNext);
	}
	else {
		pfnGetPosition	= &(CDXFlist::GetTailPosition);
		pfnGetData		= &(CDXFlist::GetPrev);
	}

	// ﾃﾞｰﾀ生成ﾙｰﾌﾟ(正転逆転)
	CDXFdata*	pData;
	for ( POSITION pos=(g_ltDeepGlist.*pfnGetPosition)(); pos && IsThread(); ) {
		pData = (g_ltDeepGlist.*pfnGetData)(pos);
		AddMakeGdataDeep(pData, bDeep);
	}

	return pData;
}

CDXFdata* MakeLoopDeepAdd_All(BOOL bAction, BOOL bDeep)
{
	POSITION	(CDXFlist::*pfnGetPosition)(void) const;
	CDXFdata*&	(CDXFlist::*pfnGetData)(POSITION&);
	if ( bAction ) {
		pfnGetPosition	= &(CDXFlist::GetHeadPosition);
		pfnGetData		= &(CDXFlist::GetNext);
	}
	else {
		pfnGetPosition	= &(CDXFlist::GetTailPosition);
		pfnGetData		= &(CDXFlist::GetPrev);
	}
	BOOL		bMove = FALSE, bBreak = FALSE;
	CDXFdata*	pData;
	CDXFdata*	pDataResult = NULL;

	// ﾃﾞｰﾀ生成ﾙｰﾌﾟ(正転逆転)
	for ( POSITION pos=(g_ltDeepGlist.*pfnGetPosition)(); pos && IsThread(); ) {
		// ｵﾌﾞｼﾞｪｸﾄ取り出し
		pData = (g_ltDeepGlist.*pfnGetData)(pos);
		if ( pData ) {
			if ( pData->GetParentLayer()->IsCutType() ) {
				// 切削ﾃﾞｰﾀ
				if ( bBreak ) {
					if ( !bMove )	// 移動なければ
						MakeLoopDeepZUp();
					// pDataまで移動(Z軸下降込み)
					AddMoveGdataG0(pData);
				}
				if ( bMove ) {
					// 移動ﾃﾞｰﾀ処理中
					AddMoveGdataZdown();
					bMove = FALSE;
				}
				AddMakeGdataDeep(pData, bDeep);
			}
			else {
				// 移動ﾃﾞｰﾀ
				ASSERT( !bBreak );
				if ( !bMove )
					MakeLoopDeepZUp();
				AddMakeMove(pData);
				bMove = TRUE;
			}
			bBreak = FALSE;
			pDataResult = pData;
		}
		else
			bBreak = TRUE;
	}

	ASSERT(pDataResult);
	return pDataResult;
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

BOOL MakeLoopDrillPoint(CDXFdata* pData)
{
	BOOL	bResult = FALSE;

	// 基準軸で並べ替え，同一線上にあるﾃﾞｰﾀを抽出
	switch ( GetNum(MKNC_NUM_OPTIMAIZEDRILL) ) {
	case 1:		// X軸基準
		g_obDrillAxis.SetSize(0, g_obDrillGroup.GetSize());
		g_obDrillGroup.Sort(DrillOptimaizeCompareFuncY);
		// 現在位置に近い方から
		bResult = MakeLoopDrillPointSeqChk(FALSE, g_obDrillGroup) ?
			MakeLoopDrillPointXYRevers(TRUE) : MakeLoopDrillPointXY(TRUE);
		break;

	case 2:		// Y軸基準
		g_obDrillAxis.SetSize(0, g_obDrillGroup.GetSize());
		g_obDrillGroup.Sort(DrillOptimaizeCompareFuncX);
		bResult = MakeLoopDrillPointSeqChk(TRUE, g_obDrillGroup) ?
			MakeLoopDrillPointXYRevers(FALSE) : MakeLoopDrillPointXY(FALSE);
		break;

	default:	// なし
		bResult = MakeLoopAddDrill(pData);
		break;
	}

	return bResult;
}

BOOL MakeLoopDrillPointSeqChk(BOOL bXY, CDXFsort& pObArray)	// bXY==TRUE -> X軸
{
	if ( pObArray.GetSize() <= 1 )	// １件なら比べるまでもない
		return FALSE;

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
			return FALSE;	// 先頭が近い
		else
			return TRUE;	// 末尾が近い(反転が必要)
	}
	else {
		if ( fabs(pData1->GetEndCutterPoint().y - CDXFdata::ms_pData->GetEndCutterPoint().y) <=
			 fabs(pData2->GetEndCutterPoint().y - CDXFdata::ms_pData->GetEndCutterPoint().y) )
			return FALSE;	// 先頭が近い
		else
			return TRUE;	// 末尾が近い(反転が必要)
	}
}

BOOL MakeLoopDrillPointXY(BOOL bXY)
{
	int		i = 0, nPos = 0, nLoop = g_obDrillGroup.GetSize(), n;
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

	while ( i < nLoop && IsThread() ) {
		dBase = g_obDrillGroup[i]->GetEndCutterPoint()[n] + dMargin;
#ifdef _DEBUG
		g_dbg.printf("BasePoint=%f", dBase);
#endif
		g_obDrillAxis.RemoveAll();
		while ( i < nLoop &&
				g_obDrillGroup[i]->GetEndCutterPoint()[n] <= dBase && IsThread() ) {
#ifdef _DEBUG
			g_dbg.printf("NowPoint=%f", g_obDrillGroup[i]->GetEndCutterPoint()[n]);
#endif
			g_obDrillAxis.Add(g_obDrillGroup[i++]);
		}
		if ( g_obDrillAxis.IsEmpty() )
			continue;
		g_obDrillAxis.Sort(pfnCompare);
		if ( MakeLoopDrillPointSeqChk(bXY, g_obDrillAxis) )	// 反転必要？
			g_obDrillAxis.Reverse();
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
		if ( MakeLoopDrillPointSeqChk(bXY, g_obDrillAxis) )	// 反転必要？
			g_obDrillAxis.Reverse();
		if ( !MakeLoopAddDrillSeq(nPos) )
			return FALSE;
		nPos = g_obDrillAxis.GetSize();
	}

	return IsThread();
}

BOOL MakeLoopDrillCircle(void)
{
	CDXFdata*	pData;
	CDXFdata*	pDataResult;
	int		i, nLoop = g_obCircle.GetSize();
	BOOL	bMatch;
	double	r, dGap, dGapMin;
	CString	strBreak;

	// ﾙｰﾌﾟ開始
	g_obDrillGroup.SetSize(0, nLoop);
	for ( i=0; i<nLoop && IsThread(); ) {
		g_obDrillGroup.RemoveAll();
		// 円をｸﾞﾙｰﾌﾟ(半径)ごとに処理
		r = fabs( ((CDXFcircle *)g_obCircle[i])->GetMakeR() );
		bMatch = FALSE;
		pDataResult = NULL;
		dGapMin = HUGE_VAL;
		for ( ; i<nLoop &&
				r==fabs(((CDXFcircle *)g_obCircle[i])->GetMakeR()) && IsThread(); i++ ) {
			pData = g_obCircle[i];
			if ( !pData->IsMakeFlg() ) {
				g_obDrillGroup.Add(pData);
				dGap = pData->GetEdgeGap(CDXFdata::ms_pData);
				if ( dGap < dGapMin ) {
					dGapMin = dGap;
					pDataResult = pData;
					if ( sqrt(dGap) < GetDbl(MKNC_DBL_TOLERANCE) )
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
			if ( GetNum(MKNC_NUM_OPTIMAIZEDRILL) == 0 )
				g_obDrillAxis.Copy(g_obDrillGroup);	// 基準軸ないので全て対象
			// 加工前移動指示の生成
			if ( !bMatch ) {
				pData = MakeLoopAddFirstMove(MAKEDRILLPOINT);
				if ( pData )
					pDataResult = pData;
			}
			// ﾃﾞｰﾀ生成
			if ( !MakeLoopDrillPoint(pDataResult) )
				return FALSE;
		}
	}

	return IsThread();
}

BOOL MakeLoopAddDrill(CDXFdata* pData)
{
	CDXFdata*	pDataMove;
	CPointD		pt;
	int			nPos = 0;
	BOOL		bMatch,
				bMove = FALSE,		// 移動ﾚｲﾔHit
				bCust = TRUE;		// 移動ﾃﾞｰﾀ前のｶｽﾀﾑｺｰﾄﾞ挿入

	// ﾃﾞｰﾀ生成
	AddMakeGdata(pData);
	CDXFdata::ms_pData = pData;

	// GetNearPoint() の結果が NULL になるまで
	while ( IsThread() ) {
		// この要素に一番近い要素
		tie(pData, bMatch) = GetNearPointDrill(CDXFdata::ms_pData);
		if ( !pData )
			break;
		if ( !bMatch ) {
			// 移動指示ﾚｲﾔのﾁｪｯｸ
			pDataMove = CDXFdata::ms_pData;		// 前回ｵﾌﾞｼﾞｪｸﾄで探索
			if ( GetMatchPointMove(pDataMove) ) {
				// 移動ﾚｲﾔ１つ目の終端に穴加工ﾃﾞｰﾀがHitすれば
				tie(pData, bMatch) = GetNearPointDrill(pDataMove);
				if ( !bMove && bMatch ) {
					// 移動ﾚｲﾔﾃﾞｰﾀは処理したことにして，この穴加工ﾃﾞｰﾀを生成
					pDataMove->SetMakeFlg();
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
					CDXFdata::ms_pData = pDataMove;
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
		CDXFdata::ms_pData = pData;
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
	CDXFarray*	pobArray;
	CDXFdata*	pDataMove = CDXFdata::ms_pData;
	CDXFdata*	pDataResult = NULL;
	CDXFdata*	pData;
	BOOL		bMatch, bCust = FALSE;

	// 移動指示ﾚｲﾔのﾁｪｯｸ
	while ( GetMatchPointMove(pDataMove) && IsThread() ) {
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
			CPointD	pt(pDataResult->GetEndMakePoint());
			if ( g_mpDXFdata.Lookup(pt, pobArray) ) {
				bMatch = FALSE;
				for ( int i=0; i<pobArray->GetSize() && IsThread(); i++ ) {
					pData = pobArray->GetAt(i);
					if ( !pData->IsMakeFlg() ) {
						pDataResult = pData;
						bMatch = TRUE;
						break;
					}
				}
				if ( bMatch )
					break;
			}
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
	while ( GetMatchPointMove(pDataMove) && IsThread() ) {
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

CDXFdata* GetNearPointCutter(const CLayerData* pLayerTarget, const CDXFdata* pDataTarget)
{
	CDXFdata*	pDataResult = NULL;
	CDXFdata*	pData;
	CLayerData*	pLayer;
	int			i, nLayerLoop = pLayerTarget ? 1 : g_pDoc->GetLayerCnt();
	double		dGap, dGapMin = HUGE_VAL;

	while ( nLayerLoop-- > 0 && IsThread() ) {
		pLayer = pLayerTarget ? const_cast<CLayerData*>(pLayerTarget) : g_pDoc->GetLayerData(nLayerLoop);
		if ( !pLayer->IsMakeTarget() )
			continue;
		for ( i=0; i<pLayer->GetDxfSize() && IsThread(); i++ ) {
			pData = pLayer->GetDxfData(i);
			if ( pData->IsMakeFlg() || pData->GetMakeType()==DXFPOINTDATA )
				continue;
			dGap = pData->GetEdgeGap(pDataTarget);
			if ( dGap < dGapMin ) {
				pDataResult = pData;
				dGapMin = dGap;
			}
		}
	}

	return pDataResult;
}

CDXFshape* GetNearPointShape(const CPointD& pt)
{
	CLayerData*	pLayer;
	CDXFshape*	pShape;
	CDXFshape*	pShapeResult = NULL;
	int			i, j;
	double		dGap, dGapMin = HUGE_VAL;

	for ( i=0; i<g_pDoc->GetLayerCnt() && IsThread(); i++ ) {
		pLayer = g_pDoc->GetLayerData(i);
		if ( !pLayer->IsMakeTarget() )
			continue;
		for ( j=0; j<pLayer->GetShapeSize(); j++ ) {
			pShape = pLayer->GetShapeData(j);
			if ( !pShape->IsMakeFlg() && pShape->GetShapeAssemble()!=DXFSHAPE_EXCLUDE ) {
				dGap = pShape->GetSelectObjectFromShape(pt);
				if ( dGap < dGapMin ) {
					dGapMin = dGap;
					pShapeResult = pShape;
				}
			}
		}
	}

	return pShapeResult;
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

BOOL GetMatchPointMove(CDXFdata*& pDataResult)
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

		if ( *s != '{' ) {
			m_strResult += string(s, e);
			return;
		}

		// 前後の "{}" 除去して解析
		int		nTestCode = stOrder.GetIndex(string(s+1, e-1).c_str());
		CString	strPath, strFile;
		TCHAR	szUserName[_MAX_PATH];
		DWORD	dwResult;
		CTime	time;
		double	dValue[VALUESIZE];
		// replace
		switch ( nTestCode ) {
		case 0:		// G90orG91
			m_strResult += CNCMake::MakeCustomString(GetNum(MKNC_NUM_G90)+90);
			break;
		case 1:		// G92_INITIAL
			dValue[NCA_X] = GetDbl(MKNC_DBL_G92X);
			dValue[NCA_Y] = GetDbl(MKNC_DBL_G92Y);
			dValue[NCA_Z] = GetDbl(MKNC_DBL_G92Z);
			m_strResult += CNCMake::MakeCustomString(92, NCD_X|NCD_Y|NCD_Z, dValue, FALSE);
			break;
		case 2:		// G92X
		case 3:		// G92Y
		case 4:		// G92Z
			dValue[nTestCode-2] = GetDbl(MKNC_DBL_G92X+nTestCode-2);
			m_strResult += CNCMake::MakeCustomString(-1, g_dwSetValFlags[nTestCode-2], dValue, FALSE);
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
			m_strResult += CNCMake::MakeCustomString(0, NCD_X|NCD_Y, dValue);
			break;
		case 7:		// MakeUser
			dwResult = _MAX_PATH;
			// ﾕｰｻﾞ名に漢字が含まれていると生成しない
			m_strResult += GetUserName(szUserName, &dwResult) && IsNCchar(szUserName) ?
				szUserName : szReplaceErr;
			break;
		case 8:		// MakeDate
			time = CTime::GetCurrentTime();
			VERIFY(strPath.LoadString(ID_INDICATOR_DATE_F2));	// %y/%m/%d
			m_strResult += time.Format(strPath);
			break;
		case 9:		// MakeTime
			time = CTime::GetCurrentTime();
			VERIFY(strPath.LoadString(ID_INDICATOR_TIME_F));	// %H:%M
			m_strResult += time.Format(strPath);
			break;
		case 10:	// MakeNCD
			Path_Name_From_FullPath(g_pDoc->GetNCFileName(), strPath, strFile);
			m_strResult += IsNCchar(strFile) ? strFile : szReplaceErr;
			break;
		case 11:	// MakeDXF
			Path_Name_From_FullPath(g_pDoc->GetPathName(), strPath, strFile);	// GetTitle()には"*"が付く可能性あり
			m_strResult += IsNCchar(strFile) ? strFile : szReplaceErr;
			break;
		case 12:	// MakeCondition
			Path_Name_From_FullPath(g_pMakeOpt->GetInitFile(), strPath, strFile);
			m_strResult += IsNCchar(strFile) ? strFile : szReplaceErr;
			break;
		}
	}
};

void AddCustomCode(const CString& strFileName, const CDXFdata* pData)
{
	using namespace boost::spirit;

	CString	strBuf;
	string	strResult;
	CMakeCustomCode		custom(strResult, pData);

	try {
		CStdioFile	fp(strFileName,
			CFile::modeRead | CFile::shareDenyWrite | CFile::typeText);
		while ( fp.ReadString(strBuf) ) {
			// 構文解析
			strResult.clear();
			if ( parse((LPCTSTR)strBuf, *( *(anychar_p - '{')[custom] >> comment_p('{', '}')[custom] ) ).hit ) {
				if ( !strResult.empty() )
					AddMakeGdata( strResult.c_str() );
			}
			else
				AddMakeGdata( strBuf );
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
	for ( int i=0; i<lstrlen(lpsz); i++ ) {
//		if ( isprint(lpsz[i]) == 0 )
		if ( IsDBCSLeadByte(lpsz[i]) )
			return FALSE;
	}
	return TRUE;
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
void AddCommentText(const CPointD& pt)
{
	CDXFarray*	pobArray;
	
	if ( g_mpDXFcomment.Lookup(const_cast<CPointD&>(pt), pobArray) ) {
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

void AddStartTextIntegrated(const CPointD& pt)
{
	AddCommentText(pt);

	CDXFarray*	pobArray;
	if ( g_mpDXFstarttext.Lookup(const_cast<CPointD&>(pt), pobArray) ) {
		CDXFdata*	pData;
		for ( int i=0; i<pobArray->GetSize() && IsThread(); i++ ) {
			pData = pobArray->GetAt(i);
			if ( !pData->IsMakeFlg() ) {
				AddMakeText(pData);
				return;
			}
		}
	}
}

void AddMoveTextIntegrated(const CPointD& pt)
{
	AddCommentText(pt);

	CDXFarray*	pobArray;
	if ( g_mpDXFmovetext.Lookup(const_cast<CPointD&>(pt), pobArray) ) {
		CDXFdata*	pData;
		for ( int i=0; i<pobArray->GetSize() && IsThread(); i++ ) {
			pData = pobArray->GetAt(i);
			if ( !pData->IsMakeFlg() ) {
				AddMakeText(pData);
				return;
			}
		}
	}
}

void AddCutterTextIntegrated(const CPointD& pt)
{
	AddCommentText(pt);

	CDXFarray*	pobArray;
	if ( g_mpDXFtext.Lookup(const_cast<CPointD&>(pt), pobArray) ) {
		CDXFdata*	pData;
		for ( int i=0; i<pobArray->GetSize() && IsThread(); i++ ) {
			pData = pobArray->GetAt(i);
			if ( !pData->IsMakeFlg() ) {
				AddMakeText(pData);
				return;
			}
		}
	}
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
		strMsg.Format(IDS_MAKENCD_FASE, g_nFase);
	g_pParent->SetFaseMessage(strMsg, lpszMsg2);
	g_nFase++;
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

// NC生成のｸﾞﾛｰﾊﾞﾙ変数初期化(後始末)ｽﾚｯﾄﾞ
UINT MakeNCD_AfterThread(LPVOID)
{
	g_csMakeAfter.Lock();

#ifdef _DEBUG
	CMagaDbg	dbg("MakeNCD_AfterThread()\nStart", TRUE, DBG_RED);
#endif
	for ( int i=0; i<g_obMakeGdata.GetSize(); i++ )
		delete	g_obMakeGdata[i];
	g_obMakeGdata.RemoveAll();
	g_mpDXFdata.RemoveAll();
	g_mpDXFstarttext.RemoveAll();
	g_mpDXFmove.RemoveAll();
	g_mpDXFmovetext.RemoveAll();
	g_mpDXFtext.RemoveAll();
	g_mpDXFcomment.RemoveAll();
	g_obStartData.RemoveAll();
	g_ltDeepGlist.RemoveAll();
	g_obPoint.RemoveAll();
	g_obCircle.RemoveAll();
	g_obDrillGroup.RemoveAll();
	g_obDrillAxis.RemoveAll();

	g_csMakeAfter.Unlock();
	return 0;
}
