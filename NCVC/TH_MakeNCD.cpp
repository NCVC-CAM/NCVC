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
#include "NCMakeMillOpt.h"
#include "NCMakeMill.h"
#include "MakeCustomCode.h"
#include "ThreadDlg.h"

using namespace std;
using namespace boost;

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
/* --------------
!!!ATTENTION!!!
	生成時間の表示：正式ﾘﾘｰｽでは外すのを忘れずに
----------------- */
//#define	_DBG_NCMAKE_TIME
#endif

// --- CDXFdata の GetType() と GetMakeType() の使い分けに注意！！

// よく使う変数や呼び出しの簡略置換
#define	IsThread()	g_pParent->IsThreadContinue()
#define	GetFlg(a)	g_pMakeOpt->GetFlag(a)
#define	GetNum(a)	g_pMakeOpt->GetNum(a)
#define	GetDbl(a)	g_pMakeOpt->GetDbl(a)

// ｸﾞﾛｰﾊﾞﾙ変数定義
static	CThreadDlg*		g_pParent;
static	CDXFDoc*		g_pDoc;
static	CNCMakeMillOpt*	g_pMakeOpt;

// NC生成に必要なﾃﾞｰﾀ群
static	CDXFmap		g_mpDXFdata,	// 座標をｷｰにCDXFdataを格納
			g_mpDXFstarttext,				// 開始ﾚｲﾔ
			g_mpDXFmove, g_mpDXFmovetext,	// 移動ﾚｲﾔﾏｯﾌﾟ
			g_mpDXFtext, g_mpDXFcomment;	// 加工ﾃｷｽﾄ，ｺﾒﾝﾄ専用
typedef	CTypedPtrMap<CMapStringToOb, CString, CDXFshape*>	CShapeHandleMap;
typedef	CTypedPtrArrayEx<CPtrArray, CDXFsort*>	CDrillAxis;
static	CDXFsort	g_obDrill;		// 穴加工ﾃﾞｰﾀ
static	CDXFsort	g_obCircle;		// 円ﾃﾞｰﾀを穴加工するときの仮登録
static	CDXFsort	g_obStartData;	// 加工開始位置指示ﾃﾞｰﾀ
static	CDXFlist	g_ltDeepData;	// 深彫切削用の仮登録
static	CTypedPtrArrayEx<CPtrArray, CNCMakeMill*>	g_obMakeData;	// 加工ﾃﾞｰﾀ

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
static	BOOL		MakeLoopShapeAdd_ChainList(CDXFshape*, CDXFchain*, CDXFdata*);
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
static	void		MakeLoopDrillPoint_MakeAxis(int, CDrillAxis&);
static	int			MakeLoopDrillPoint_EdgeChk(int, CDXFsort*);
static	int			MakeLoopDrillPoint_SeqChk(int, int, CDXFsort*);
static	BOOL		MakeLoopDrillCircle(void);
static	BOOL		MakeLoopAddDrill(CDXFdata*);
static	tuple<int, CDXFdata*>	MakeLoopAddDrillSeq(int, int, CDXFsort*);
static	CDXFdata*	MakeLoopAddFirstMove(ENMAKETYPE);
static	BOOL		MakeLoopAddLastMove(void);

// 引数で指定したｵﾌﾞｼﾞｪｸﾄに一番近いｵﾌﾞｼﾞｪｸﾄを返す(戻り値:Z軸の移動が必要か)
static	CDXFdata*	GetNearPointCutter(const CLayerData*, const CDXFdata*);
static	CDXFshape*	GetNearPointShape(const CPointD&);
static	tuple<CDXFdata*, BOOL>	GetNearPointDrill(const CDXFdata*);
static	tuple<int, int>			GetNearPointDrillAxis(const CDXFdata*, int, CDrillAxis&);
static	CDXFdata*	GetMatchPointMove(const CDXFdata*);

// ﾍｯﾀﾞｰ,ﾌｯﾀﾞｰ等のｽﾍﾟｼｬﾙｺｰﾄﾞ生成
static	void	AddCustomCode(const CString&, const CDXFdata*);

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
inline	void	AddMoveGdataZ(int nCode, double dZ, double dFeed)
{
	CNCMakeMill*	mkNCD = new CNCMakeMill(nCode, dZ, dFeed);
	ASSERT( mkNCD );
	g_obMakeData.Add(mkNCD);
}
// Z軸の上昇
inline	void	AddMoveGdataZup(void)
{
	// 終点座標でのｺﾒﾝﾄ生成
	AddCutterTextIntegrated( CDXFdata::ms_pData->GetEndCutterPoint() );
	// Z軸の上昇
	AddMoveGdataZ(0, g_dZReturn, -1.0);
}
// Z軸の下降
inline	void	AddMoveGdataZdown(void)
{
	// Z軸の現在位置がR点より大きい(高い)ならR点まで早送り
	if ( CNCMakeMill::ms_xyz[NCA_Z] > g_dZG0Stop )
		AddMoveGdataZ(0, g_dZG0Stop, -1.0);
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
	if ( CNCMakeMill::ms_xyz[NCA_Z] > dZValue )
		AddMoveGdataZ(1, dZValue, GetDbl(MKNC_DBL_MAKEENDFEED));
	// 切削点まで切削送り
	if ( CNCMakeMill::ms_xyz[NCA_Z] > g_dZCut )
		AddMoveGdataZ(1, g_dZCut, GetDbl(MKNC_DBL_ZFEED));
}
// 移動ﾃﾞｰﾀ
inline	void	AddMoveGdata(int nCode, const CDXFdata* pData)
{
	CNCMakeMill* mkNCD = new CNCMakeMill(nCode, pData->GetStartMakePoint());
	ASSERT( mkNCD );
	g_obMakeData.Add(mkNCD);
}
inline	void	AddMoveGdataG0(const CDXFdata* pData)
{
	// G0移動ﾃﾞｰﾀの生成
	AddMoveGdata(0, pData);
	// Z軸の下降
	AddMoveGdataZdown();
}
inline	void	AddMoveGdataG1(const CDXFdata* pData)
{
	// Z軸の下降
	AddMoveGdataZdown();
	// G1移動ﾃﾞｰﾀの生成
	AddMoveGdata(1, pData);
}
// 切削ﾃﾞｰﾀ
inline	void	AddMakeGdata(CDXFdata* pData, double dFeed)
{
	ASSERT( pData );
	// 開始位置と等しいﾃｷｽﾄﾃﾞｰﾀの生成
	AddCutterTextIntegrated( pData->GetStartCutterPoint() );
	// 切削ﾃﾞｰﾀ生成
	CNCMakeMill*	mkNCD = new CNCMakeMill(pData, dFeed);
	ASSERT( mkNCD );
	g_obMakeData.Add(mkNCD);
	pData->SetMakeFlg();
}
inline	void	AddMakeGdataCut(CDXFdata* pData)
{
	// 切削ﾃﾞｰﾀの生成(切削)
	AddMakeGdata(pData, GetDbl(MKNC_DBL_FEED));
}
inline	void	AddMakeGdataDeep(CDXFdata* pData, BOOL bDeep)
{
	// 切削ﾃﾞｰﾀの生成(深彫)
	AddMakeGdata(pData, bDeep ? GetDbl(MKNC_DBL_DEEPFEED) : GetDbl(MKNC_DBL_FEED));
}
inline	void	AddMakeGdataDrill(CDXFdata* pData)
{
	// 切削ﾃﾞｰﾀの生成(穴加工)
	AddMakeGdata(pData, GetDbl(MKNC_DBL_DRILLFEED));
}
inline	void	AddMakeGdataHelical(CDXFdata* pData)
{
	// 切削ﾃﾞｰﾀの生成(深彫円ﾃﾞｰﾀのﾍﾘｶﾙ切削)
	ASSERT( pData );
	AddCutterTextIntegrated( pData->GetStartCutterPoint() );
	CNCMakeMill*	mkNCD = new CNCMakeMill(pData, GetDbl(MKNC_DBL_FEED), &g_dZCut);
	ASSERT( mkNCD );
	g_obMakeData.Add(mkNCD);
	pData->SetMakeFlg();
}
// 任意ﾃﾞｰﾀの生成
inline	void	AddMakeGdataStr(const CString& strData)
{
	CNCMakeMill*	mkNCD = new CNCMakeMill(strData);
	ASSERT( mkNCD );
	g_obMakeData.Add(mkNCD);
}
// 移動指示ﾚｲﾔのﾃﾞｰﾀ生成
inline	void	AddMakeMove(CDXFdata* pData, BOOL bL0 = FALSE)
{
	// 開始位置と等しいﾃｷｽﾄﾃﾞｰﾀの生成
	AddMoveTextIntegrated( pData->GetStartCutterPoint() );
	// 移動指示
	CNCMakeMill*	mkNCD = new CNCMakeMill(pData, bL0);
	ASSERT( mkNCD );
	g_obMakeData.Add(mkNCD);
	pData->SetMakeFlg();
}
// 固定ｻｲｸﾙｷｬﾝｾﾙｺｰﾄﾞ
inline	void	AddMakeGdataCycleCancel(void)
{
	if ( CDXFdata::ms_pData->GetMakeType() == DXFPOINTDATA ) {
		AddMakeGdataStr( CNCMakeBase::MakeCustomString(80) );
		InitialCycleBaseVariable();
	}
}
// 文字情報出力
inline	void	AddMakeText(CDXFdata* pData)
{
	CString	strText( ((CDXFtext *)pData)->GetStrValue() );
	strText.Replace("\\n", "\n");
	AddMakeGdataStr(strText);
	pData->SetMakeFlg();
}

// ﾌｪｰｽﾞ更新
static	int		g_nFase;			// ﾌｪｰｽﾞ№
static	void	SendFaseMessage(int = -1, int = -1, LPCTSTR = NULL);
inline	void	SendProgressPos(int i)
{
	if ( (i & 0x003f) == 0 )	// 64回おき(下位6ﾋﾞｯﾄﾏｽｸ)
		g_pParent->m_ctReadProgress.SetPos(i);
}

// 並べ替え補助関数
static	int		CircleSizeCompareFunc1(CDXFdata*, CDXFdata*);	// 円ﾃﾞｰﾀ昇順ｿｰﾄ
static	int		CircleSizeCompareFunc2(CDXFdata*, CDXFdata*);	// 円ﾃﾞｰﾀ降順ｿｰﾄ
static	int		DrillOptimaizeCompareFuncX(CDXFdata*, CDXFdata*);	// X軸
static	int		DrillOptimaizeCompareFuncY(CDXFdata*, CDXFdata*);	// Y軸
static	int		AreaSizeCompareFunc(CDXFchain*, CDXFchain*);	// 形状集合の面積昇順ｿｰﾄ

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
#ifdef _DBG_NCMAKE_TIME
	// 現在時刻を取得
	CTime t1 = CTime::GetCurrentTime();
#endif

	int		i, nResult = IDCANCEL;

	// ｸﾞﾛｰﾊﾞﾙ変数初期化
	LPNCVCTHREADPARAM	pParam = reinterpret_cast<LPNCVCTHREADPARAM>(pVoid);
	g_pParent = pParam->pParent;
	g_pDoc = static_cast<CDXFDoc*>(pParam->pDoc);
	ASSERT(g_pParent);
	ASSERT(g_pDoc);

	// 準備中表示
	g_nFase = 0;
	SendFaseMessage(-1, IDS_ANA_DATAINIT);
	g_pMakeOpt = NULL;

	// 下位の CMemoryException は全てここで集約
	try {
		BOOL	bResult = FALSE;
		// NC生成ﾀｲﾌﾟ
		int		nID = (int)(pParam->wParam);

		// NC生成ｵﾌﾟｼｮﾝｵﾌﾞｼﾞｪｸﾄの生成
		g_pMakeOpt = new CNCMakeMillOpt(NULL);	// 読み込みはしない

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
		g_obDrill.SetSize(0, i);
		g_obCircle.SetSize(0, i);
		i *= 2;
		g_obMakeData.SetSize(0, i);
		i = GetPrimeNumber(i);
		i = max(17, i);
		g_mpDXFdata.InitHashTable(i);
		g_mpDXFtext.InitHashTable(i);
		i = g_pDoc->GetDxfLayerDataCnt(DXFSTRLAYER) * 2;
		g_obStartData.SetSize(0, max(10, i));
		i = GetPrimeNumber(i);
		g_mpDXFstarttext.InitHashTable(max(17, i));
		i = GetPrimeNumber(g_pDoc->GetDxfLayerDataCnt(DXFMOVLAYER)*2);
		i = max(17, i);
		g_mpDXFmove.InitHashTable(i);
		g_mpDXFmovetext.InitHashTable(i);
		i = GetPrimeNumber(g_pDoc->GetDxfLayerDataCnt(DXFCOMLAYER));
		g_mpDXFcomment.InitHashTable(max(17, i));

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
//		THREAD_PRIORITY_LOWEST);
		THREAD_PRIORITY_IDLE);
//		THREAD_PRIORITY_BELOW_NORMAL);

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
	CNCMakeMill::InitialVariable();
}

void InitialCycleBaseVariable(void)
{
	CNCMakeMill::ms_dCycleZ[1] =
	CNCMakeMill::ms_dCycleR[1] =
	CNCMakeMill::ms_dCycleP[1] = HUGE_VAL;
}

//////////////////////////////////////////////////////////////////////

BOOL SingleLayer(int nID)
{
#ifdef _DEBUG
	CMagaDbg	dbg("SingleLayer()\nStart", DBG_CYAN);
#endif
	// NC生成ｵﾌﾟｼｮﾝ読み込み
	g_pMakeOpt->ReadMakeOption(AfxGetNCVCApp()->GetDXFOption()->GetMillInitList()->GetHead());
	// 条件ごとに変化するﾊﾟﾗﾒｰﾀを設定
	SetStaticOption();
	// 加工原点開始位置指示ﾚｲﾔ処理
	if ( !SetStartData() )
		return FALSE;
	// Z軸の切削座標ｾｯﾄ
	g_dZCut = RoundUp(GetDbl(MKNC_DBL_ZCUT));
	g_dDeep = RoundUp(GetDbl(MKNC_DBL_DEEP));
	// 固定ｻｲｸﾙの切り込み座標ｾｯﾄ
	CNCMakeMill::ms_dCycleZ[0] = RoundUp(GetDbl(MKNC_DBL_DRILLZ));
	CNCMakeMill::ms_dCycleR[0] = RoundUp(GetDbl(MKNC_DBL_DRILLR));
	CNCMakeMill::ms_dCycleP[0] = GetNum(MKNC_NUM_DWELL);
	InitialCycleBaseVariable();
	// ｸﾞﾛｰﾊﾞﾙ変数に生成対象ｵﾌﾞｼﾞｪｸﾄのｺﾋﾟｰ
	SetGlobalMap();

	// 生成開始
	if ( nID == ID_FILE_DXF2NCD ) {
		if ( !MakeNCD_MainFunc(NULL) ) {
#ifdef _DEBUG
			dbg.printf("Error:MakeNCD_MainFunc()");
			// どこまで処理できているかがわかる
			if ( AfxMessageBox("Output?", MB_YESNO|MB_ICONQUESTION) == IDYES )
				MakeNCD_FinalFunc();
#endif
			return FALSE;
		}
	}
	else {
		if ( !MakeNCD_ShapeFunc() ) {
#ifdef _DEBUG
			dbg.printf("Error:MakeNCD_ShapeFunc()");
			if ( AfxMessageBox("Output?", MB_YESNO|MB_ICONQUESTION) == IDYES )
				MakeNCD_FinalFunc();
#endif
			return FALSE;
		}
	}

	// 最終ﾁｪｯｸ
	if ( !g_bData ) {
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
	BOOL	bPartOut = FALSE,	// １回でも個別出力があればTRUE
			bNotPart = FALSE;	// 全体出力でﾙｰﾌﾟ終了なら  TRUE
	CLayerData*	pLayer;

	if ( nID == ID_FILE_DXF2NCD_EX2 ) {
		// 基準となるNC生成ｵﾌﾟｼｮﾝ読み込み
		g_pMakeOpt->ReadMakeOption(AfxGetNCVCApp()->GetDXFOption()->GetMillInitList()->GetHead());
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
		pLayer->SetLayerPartFlag();
		// 処理ﾚｲﾔ名をﾀﾞｲｱﾛｸﾞに表示
		g_nFase = 1;
		SendFaseMessage(-1, IDS_ANA_DATAINIT, pLayer->GetStrLayer());
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
			CNCMakeMill::ms_dCycleZ[0] = RoundUp(GetDbl(MKNC_DBL_DRILLZ));
		}
		else {
			// 強制Z座標補正
			g_dZCut = g_dDeep = RoundUp(pLayer->GetZCut());
			// 固定ｻｲｸﾙの切り込み座標ｾｯﾄ
			CNCMakeMill::ms_dCycleZ[0] = pLayer->IsLayerFlag(LAYER_DRILLZ) ?
						g_dZCut : RoundUp(GetDbl(MKNC_DBL_DRILLZ));
		}
		// 固定ｻｲｸﾙその他の座標ｾｯﾄ
		CNCMakeMill::ms_dCycleR[0] = RoundUp(GetDbl(MKNC_DBL_DRILLR));
		CNCMakeMill::ms_dCycleP[0] = GetNum(MKNC_NUM_DWELL);
		InitialCycleBaseVariable();
		// ｸﾞﾛｰﾊﾞﾙ変数に生成対象ｵﾌﾞｼﾞｪｸﾄのｺﾋﾟｰ
		g_mpDXFdata.RemoveAll();
		g_mpDXFtext.RemoveAll();
		g_obDrill.RemoveAll();
		g_obCircle.RemoveAll();
		g_ltDeepData.RemoveAll();
		SetGlobalMapToLayer(pLayer);
		// 一時領域で使用したﾏｯﾌﾟを削除
		g_mpDXFdata.RemoveAll();

		// 生成開始
#ifdef _DEBUG
		dbg.printf("No.%d ID=%d Name=%s Cut=%f", i+1,
			pLayer->GetLayerListNo(), pLayer->GetStrLayer(), g_dZCut);
#endif
		if ( !MakeNCD_MainFunc(pLayer) ) {
#ifdef _DEBUG
			dbgE.printf("MakeNCD_MainFunc() Error");
#endif
			return FALSE;
		}

		// 個別出力でないなら(並べ替えしているので途中に割り込むことはない)
		if ( !pLayer->IsLayerFlag(LAYER_PARTOUT) ) {
			bNotPart = TRUE;
			continue;
		}

		// --- 以下個別出力のみの処理
		if ( g_bData ) {	// NC生成ｱﾘ？
			// MakeNCD_FinalFunc(終了ｺｰﾄﾞ，ﾌｧｲﾙの出力)の実行
			if ( MakeNCD_FinalFunc(pLayer->GetNCFile()) ) {
				// ｸﾞﾛｰﾊﾞﾙ変数初期化
				InitialVariable();
				// NC生成済情報の削除
				for ( j=0; j<g_obMakeData.GetSize() && IsThread(); j++ )
					delete	g_obMakeData[j];
				g_obMakeData.RemoveAll();
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
			pLayer->SetLayerPartFlag(TRUE);
		}
	}	// End of for main loop (Layer)

	// --- 最終ﾁｪｯｸ
	if ( bNotPart ) {	// ﾙｰﾌﾟが全体出力で終了
		if ( g_bData ) {	// NC生成ｱﾘ？
			// MakeNCD_FinalFunc(終了ｺｰﾄﾞ，ﾌｧｲﾙの出力)の実行
			if ( !MakeNCD_FinalFunc() ) {
#ifdef _DEBUG
				dbgE.printf("MakeNCD_FinalFunc()");
#endif
				return FALSE;
			}
		}
		else {				// 出力ﾅｼ
			if ( bPartOut ) {	// 個別出力があれば
				// 個別出力以外のﾚｲﾔ情報を取得し，ﾜｰﾆﾝｸﾞﾒｯｾｰｼﾞ出力へ
				for ( i=0; i<nLayerCnt; i++ ) {
					pLayer = g_pDoc->GetLayerData(i);
					if ( !pLayer->IsLayerFlag(LAYER_PARTOUT) )
						pLayer->SetLayerPartFlag(TRUE);
				}
			}
			else {
				// ｴﾗｰﾒｯｾｰｼﾞ
				AfxMessageBox(IDS_ERR_MAKENC, MB_OK|MB_ICONEXCLAMATION);
				return FALSE;
			}
		}
	}

	// 個別出力のﾜｰﾆﾝｸﾞﾒｯｾｰｼﾞ
	CString	strMiss;
	for ( i=0; i<nLayerCnt; i++ ) {
		pLayer = g_pDoc->GetLayerData(i);
		if ( pLayer->IsLayerFlag(LAYER_PARTERROR) ) {
			if ( !strMiss.IsEmpty() )
				strMiss += gg_szCat;
			strMiss += pLayer->GetStrLayer();
		}
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
	// AddMoveGdataZ(Z軸の上昇)で使用
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
	g_pfnDeepProc = GetNum(MKNC_NUM_DEEPALL) == 0 ?
		&MakeLoopDeepAdd_All : &MakeLoopDeepAdd_Euler;

	// CDXFdataの静的変数初期化
	CDXFdata::ms_fXRev = GetFlg(MKNC_FLG_XREV);
	CDXFdata::ms_fYRev = GetFlg(MKNC_FLG_YREV);
	CDXFpoint::ms_pfnOrgDrillTuning = GetNum(MKNC_NUM_OPTIMAIZEDRILL) == 0 ?
		&CDXFpoint::OrgTuning_Seq : &CDXFpoint::OrgTuning_XY;

	// CNCMakeMillの静的変数初期化
	if ( !g_bData ) {
		// ABS, INC 関係なく G92値で初期化
		for ( int i=0; i<NCXYZ; i++ )
			CNCMakeMill::ms_xyz[i] = RoundUp(GetDbl(MKNC_DBL_G92X+i));
	}
	// 生成ｵﾌﾟｼｮﾝによる静的変数の初期化
	CNCMakeMill::SetStaticOption(g_pMakeOpt);
}

BOOL SetStartData(void)
{
	ASSERT(CDXFdata::ms_pData);

	int		i, j;
	CDXFdata*	pData;
	CDXFdata*	pMatchData = NULL;
	CDXFdata*	pDataResult = NULL;
	CDXFcircleEx*	pStartCircle = NULL;
	double		dGap, dGapMin = DBL_MAX;
	CString		strLayer;
	CLayerData*	pLayer;
	CDXFarray	obArray;
	obArray.SetSize(0, g_pDoc->GetDxfLayerDataCnt(DXFSTRLAYER));

	for ( i=0; i<g_pDoc->GetLayerCnt() && IsThread(); i++ ) {
		pLayer = g_pDoc->GetLayerData(i);
		if ( !pLayer->IsLayerFlag(LAYER_CUTTARGET) || pLayer->GetLayerType()!=DXFSTRLAYER )
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
				pStartCircle = static_cast<CDXFcircleEx*>(pData);
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
		dGapMin = DBL_MAX;
		for ( i=0; i<obArray.GetSize() && IsThread(); i++ ) {
			pData = obArray[i];
			if ( pData->IsSearchFlg() )
				continue;
			if ( pData->IsMakeMatchObject(pMatchData) ) {
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
	CPointD	pt;
	CDXFdata*	pData;
	CDXFellipse*	pEllipse;
	CDXFarray*	pDummy = NULL;

	// 指定ﾚｲﾔのDXFﾃﾞｰﾀｵﾌﾞｼﾞｪｸﾄ(穴加工のみ)をｺﾋﾟｰ
	for ( i=0; i<pLayer->GetDxfSize() && IsThread(); i++ ) {
		pData = pLayer->GetDxfData(i);
		// 楕円ﾃﾞｰﾀの変身
		if ( pData->GetType() == DXFELLIPSEDATA ) {
			pEllipse = static_cast<CDXFellipse*>(pData);
			if ( GetFlg(MKNC_FLG_ELLIPSE) &&
					fabs( RoundUp(pEllipse->GetLongLength()) - RoundUp(pEllipse->GetShortLength()) ) < NCMIN ) {
				// 長径短径が等しい楕円なら円弧か円ﾃﾞｰﾀに変身
				pData->ChangeMakeType( pEllipse->IsArc() ? DXFARCDATA : DXFCIRCLEDATA);
			}
		}
		// 各ｵﾌﾞｼﾞｪｸﾄﾀｲﾌﾟごとの処理
		switch ( pData->GetMakeType() ) {
		case DXFPOINTDATA:
			// 重複ﾁｪｯｸ
			if ( GetFlg(MKNC_FLG_DRILLMATCH) ) {
				pt = pData->GetNativePoint(0);
				if ( !g_mpDXFdata.Lookup(pt, pDummy) ) {
					g_obDrill.Add(pData);
					g_mpDXFdata.SetAt(pt, pDummy);
				}
			}
			else
				g_obDrill.Add(pData);
			break;
		case DXFCIRCLEDATA:
			// 円ﾃﾞｰﾀの半径が指定値以下なら
			if ( GetFlg(MKNC_FLG_DRILLCIRCLE) &&
					static_cast<CDXFcircle*>(pData)->GetMakeR() <= GetDbl(MKNC_DBL_DRILLCIRCLE) ) {
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

	// ﾃｷｽﾄﾃﾞｰﾀのﾏｯﾌﾟ作成
	for ( i=0; i<pLayer->GetDxfTextSize() && IsThread(); i++ ) {
		pData = pLayer->GetDxfTextData(i);
		pData->OrgTuning(FALSE);
		g_mpDXFtext.SetMakePointMap(pData);
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
		if ( !pLayer->IsLayerFlag(LAYER_CUTTARGET) )
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
		AddMoveGdataZ(0, g_dZInitial, -1);
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
	SendFaseMessage(g_obMakeData.GetSize(), IDS_ANA_DATAFINAL, strFile);
	try {
		CStdioFile	fp(strNCFile,
				CFile::modeCreate | CFile::modeWrite | CFile::shareExclusive | CFile::typeText);
#ifdef _DEBUG
		for ( int i=0; i<g_obMakeData.GetSize(); i++ ) {
#else
		for ( int i=0; i<g_obMakeData.GetSize() && IsThread(); i++ ) {
#endif
			g_obMakeData[i]->WriteGcode(fp);
			SendProgressPos(i);
		}
	}
	catch (	CFileException* e ) {
		strFile.Format(IDS_ERR_DATAWRITE, strNCFile);
		AfxMessageBox(strFile, MB_OK|MB_ICONSTOP);
		e->Delete();
		return FALSE;
	}

	g_pParent->m_ctReadProgress.SetPos(g_obMakeData.GetSize());
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
	CLayerData*	pLayer;
	CDXFshape*	pShape;
	CDXFchain*	pChain;
	int			i, j, nLoop, nMapCnt = 0;

	// 加工開始位置指示ﾃﾞｰﾀのｾｯﾄ
	if ( !g_bData && !g_obStartData.IsEmpty() )
		CDXFdata::ms_pData = g_obStartData.GetTail();

	// ﾌｪｰｽﾞ1
	switch ( GetNum(MKNC_NUM_DRILLPROCESS) ) {
	case 0:		// 先に穴加工
	case 2:		// 穴加工のみ
		if ( !CallMakeDrill(NULL, CString()) )
			return FALSE;
		break;
	}
	if ( GetNum(MKNC_NUM_DRILLPROCESS) == 2 )
		return MakeLoopAddLastMove();	// 穴加工のみなら、ここで終了

	// ﾌｪｰｽﾞ2
	// 原点調整とﾏｯﾌﾟの生成ﾌﾗｸﾞをｸﾘｱ、形状集合ﾏｯﾌﾟに登録
	for ( i=0; i<g_pDoc->GetLayerCnt() && IsThread(); i++ ) {
		pLayer = g_pDoc->GetLayerData(i);
		if ( !pLayer->IsMakeTarget() )
			continue;
		nLoop  = pLayer->GetShapeSize();
		for ( j=0; j<nLoop && IsThread(); j++ ) {
			pShape = pLayer->GetShapeData(j);
			pChain = pShape->GetShapeChain();
			if ( pChain && pChain->GetCount()==1 && pChain->GetHead()->GetMakeType()==DXFPOINTDATA )
				pShape->SetShapeFlag(DXFMAPFLG_MAKE|DXFMAPFLG_SEARCH);
			else {
				pShape->OrgTuning();
				pShape->ClearMakeFlg();
			}
		}
		nMapCnt += nLoop;
	}
	if ( !IsThread() )
		return FALSE;
	if ( nMapCnt <= 0 )
		return TRUE;

	if ( !g_bData ) {
		// Gｺｰﾄﾞﾍｯﾀﾞ(開始ｺｰﾄﾞ)
		AddCustomCode(g_pMakeOpt->GetStr(MKNC_STR_HEADER), NULL);
		// 加工開始位置指示ﾚｲﾔ処理
		AddMakeStart();
	}
	// 回転数
	CString	strSpindle( CNCMakeMill::MakeSpindle(DXFLINEDATA) );
	if ( !strSpindle.IsEmpty() )
		AddMakeGdataStr(strSpindle);
	if ( !g_bData ) {
		// 加工前移動指示の生成
		MakeLoopAddFirstMove(MAKECUTTER);
	}

	// 原点に近い座標ﾏｯﾌﾟを検索
	pShape = GetNearPointShape(CDXFdata::ms_pData->GetEndCutterPoint()+CDXFdata::ms_ptOrg);
	if ( !pShape )
		return MakeLoopAddLastMove();

	// ﾃﾞｰﾀ生成
	g_bData = TRUE;
	SendFaseMessage(nMapCnt);

	// NC生成ﾙｰﾌﾟ
	if ( !MakeLoopShape(pShape) )
		return FALSE;

	if ( GetNum(MKNC_NUM_DRILLPROCESS) == 1 ) {
		// 後で穴加工
		if ( !CallMakeDrill(NULL, CString()) )
			return FALSE;
	}

	// 切削ﾃﾞｰﾀ終了後の移動指示ﾚｲﾔﾁｪｯｸ
	return MakeLoopAddLastMove();
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
			AddMoveGdataZ(0, g_dZReturn, -1.0);
		}
	}
	// ﾚｲﾔごとのｺﾒﾝﾄと出力ｺｰﾄﾞ
	if ( !strLayer.IsEmpty() ) {
		// ｺﾒﾝﾄ処理
		if ( GetFlg(MKNC_FLG_LAYERCOMMENT) ) {
			strBuf = pLayer->GetLayerComment();
			if ( strBuf.IsEmpty() )
				strBuf.Format(IDS_MAKENCD_LAYERBREAK, strLayer);
			else
				strBuf = "(" + strBuf + ")";
			AddMakeGdataStr(strBuf);
		}
		// 出力ｺｰﾄﾞ
		strBuf = pLayer->GetLayerOutputCode();
		if ( !strBuf.IsEmpty() ) {
			strBuf.Replace("\\n", "\n");
			AddMakeGdataStr(strBuf);
		}
		// 同じｺﾒﾝﾄを入れないようにﾚｲﾔ名をｸﾘｱ
		strLayer.Empty();
	}
	// 回転数
	strBuf = CNCMakeMill::MakeSpindle(pData->GetMakeType());
	if ( !strBuf.IsEmpty() )
		AddMakeGdataStr(strBuf);

	// 加工前移動指示の生成
	if ( !bMatch && enMake!=MAKEDRILLCIRCLE ) {
		// OrgTuning()で現在位置と同一座標が見つからなかった
		CDXFdata* pDataMove = MakeLoopAddFirstMove(enMake);
		if ( pDataMove ) {
			// 移動先に近い切削ﾃﾞｰﾀを再検索
			if ( enMake == MAKECUTTER )
				pData = GetNearPointCutter(pLayer, pDataMove);
			else if ( enMake==MAKEDRILLPOINT && GetNum(MKNC_NUM_OPTIMAIZEDRILL)==0 )
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
				AddMakeGdataStr(strBuf);
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
	BOOL	bMatch = FALSE, bCalc = !g_bData,
			bRound = GetNum(MKNC_NUM_CIRCLECODE) == 0 ? FALSE : TRUE;
	double	dGap, dGapMin = DBL_MAX;	// 原点までの距離
	CDXFdata*	pDataResult = NULL;
	CDXFdata*	pData;
	CDXFellipse*	pEllipse;
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
		pLayer = pLayerTarget ? const_cast<CLayerData *>(pLayerTarget) : g_pDoc->GetLayerData(nLayerLoop);
		if ( !pLayer->IsMakeTarget() )
			continue;
		for ( i=0; i<pLayer->GetDxfSize() && IsThread(); i++, nCnt++ ) {
			pData = pLayer->GetDxfData(i);
			if ( pData->GetMakeType()!=DXFPOINTDATA && pData->IsMakeTarget() ) {	// 長さの短いﾃﾞｰﾀはここで除去
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
				// 楕円ﾃﾞｰﾀの調整
				if ( pData->GetMakeType() == DXFELLIPSEDATA ) {
					pEllipse = static_cast<CDXFellipse*>(pData);
					if ( !pEllipse->IsArc() ) {
						// 楕円の切削方向を設定
						pEllipse->SetRoundFixed(bRound);
					}
				}
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
	int			i;
	const int	nLoop = g_obDrill.GetSize();
	BOOL		bMatch = FALSE, bCalc;
	double		dGap, dGapMin = DBL_MAX;	// 原点までの距離
	CDXFdata*	pData;
	CDXFdata*	pDataResult = NULL;

	if ( g_obDrill.IsEmpty() )
		return make_tuple(pDataResult, bMatch);

	// ﾌｪｰｽﾞ1
	SendFaseMessage(nLoop);

	// 原点調整と切削開始ﾎﾟｲﾝﾄ検索ﾙｰﾌﾟ
#ifdef _DEBUG
	int	nDbg = -1;
#endif

	// 切削開始ﾎﾟｲﾝﾄの計算が必要かどうか
	if ( g_bData )
		bCalc = FALSE;
	else
		bCalc = GetNum(MKNC_NUM_OPTIMAIZEDRILL) == 0 ? TRUE : FALSE;

	for ( i=0; i<nLoop && IsThread(); i++ ) {
		pData = g_obDrill[i];
		// 原点調整と距離計算 + NC生成ﾌﾗｸﾞの初期化
		dGap = pData->OrgTuning(bCalc);
		// 原点に一番近いﾎﾟｲﾝﾄを探す
		if ( !bCalc ) {
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

	if ( !pDataResult )		// !bCalc
		pDataResult = g_obDrill[0];		// dummy

	return make_tuple(pDataResult, bMatch);
}

CDXFdata* OrgTuningDrillCircle(void)
{
#ifdef _DEBUG
	CMagaDbg	dbg("OrgTuningDrillCircle()", DBG_GREEN);
#endif
	if ( g_obCircle.IsEmpty() )
		return NULL;

	int			i, j;
	const int	nLoop = g_obCircle.GetSize();
	CPointD		pt;
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
	g_obCircle.Sort( GetNum(MKNC_NUM_DRILLSORT) == 0 ?
		CircleSizeCompareFunc1 : CircleSizeCompareFunc2 );	// 昇順・降順

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
	BOOL		bMove, bMakeHit, bCust;
	int			i, nCnt, nPos = 0, nSetPos = 64;
	CPointD		ptKey;
	CDXFarray*	pobArray;

	i = GetPrimeNumber( g_pDoc->GetDxfLayerDataCnt(DXFCAMLAYER)*2 );
	mpEuler.InitHashTable(max(17, i));

	// GetNearPoint() の結果が NULL になるまで
	while ( pData && IsThread() ) {
		mpEuler.RemoveAll();
		// この pData を基点に
		mpEuler.SetMakePointMap(pData);
		pData->SetSearchFlg();
		// pData の始点・終点で一筆書き探索
		for ( i=0; i<pData->GetPointNumber() && IsThread(); i++ ) {
			if ( !MakeLoopEulerSearch(pData->GetTunPoint(i), mpEuler) )
				return FALSE;
		}
		// CMapｵﾌﾞｼﾞｪｸﾄのNC生成
		if ( IsThread() ) {
			if ( (nCnt=MakeLoopEulerAdd(&mpEuler)) < 0 )
				return FALSE;
		}
		//
		nPos += nCnt;
		if ( nSetPos < nPos ) {
			g_pParent->m_ctReadProgress.SetPos(nPos);
			while ( nSetPos < nPos )
				nSetPos += nSetPos;
		}

		// 移動指示ﾚｲﾔのﾁｪｯｸ
		bCust = TRUE;
		bMove = bMakeHit = FALSE;
		while ( (pDataMove=GetMatchPointMove(CDXFdata::ms_pData)) && IsThread() ) {
			if ( GetFlg(MKNC_FLG_DEEP) && GetNum(MKNC_NUM_DEEPALL)==0 ) {
				// 最後にZ軸移動のﾏｰｶｰがあれば削除
				if ( !g_ltDeepData.IsEmpty() && g_ltDeepData.GetTail() == NULL )
					g_ltDeepData.RemoveTail();
				// 移動ﾃﾞｰﾀ登録
				g_ltDeepData.AddTail(pDataMove);
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
			// 移動ﾃﾞｰﾀの終点で切削ﾃﾞｰﾀがﾋｯﾄすれば
			ptKey = pDataMove->GetEndCutterPoint();
			if ( g_mpDXFdata.Lookup(ptKey, pobArray) ) {
				bMakeHit = TRUE;
				break;	// 切削ﾃﾞｰﾀ優先のため移動ﾃﾞｰﾀ処理を中断
			}
		}
		// 移動ﾃﾞｰﾀ後のｶｽﾀﾑｺｰﾄﾞ挿入
		if ( bMove && IsThread() ) {
			// 移動ﾃﾞｰﾀの終点でﾃｷｽﾄﾃﾞｰﾀの生成
			AddMoveTextIntegrated(CDXFdata::ms_pData->GetEndCutterPoint());
			// ｶｽﾀﾑｺｰﾄﾞ
			(*g_pfnAddMoveCust_A)();
		}

		if ( bMakeHit ) {
			pData = NULL;
			// pobArray から次の切削ﾃﾞｰﾀを検索
			for ( i=0; i<pobArray->GetSize() && IsThread(); i++ ) {
				pDataMove = pobArray->GetAt(i);
				if ( !pDataMove->IsMakeFlg() ) {
					pData = pDataMove;
					break;
				}
			}
			if ( !pData )
				bMakeHit = FALSE;	// 次の切削ﾎﾟｲﾝﾄ検索処理へ
		}
		if ( !bMakeHit ) {		// else ではダメ
			// 次の切削ﾎﾟｲﾝﾄ検索
			if ( IsThread() )
				pData = GetNearPointCutter(pLayer, CDXFdata::ms_pData);
			// Z軸の上昇
			if ( pData && !GetFlg(MKNC_FLG_DEEP) && GetNum(MKNC_NUM_TOLERANCE)==0 )
				AddMoveGdataZup();
		}

	} // End of while

	g_pParent->m_ctReadProgress.SetPos(nPos);

	// 全体深彫の後処理
	if ( GetFlg(MKNC_FLG_DEEP) && GetNum(MKNC_NUM_DEEPALL)==0 && IsThread() ) {
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
	dbg.printf("FirstPoint x=%f y=%f EulerFlg=%d Cnt=%d", pt.x, pt.y, bEuler, pEuler->GetCount());
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
		g_ltDeepData.AddTail(&ltEuler);
		// 深彫が全体か否か
		if ( GetNum(MKNC_NUM_DEEPALL) == 0 ) {
			g_ltDeepData.AddTail((CDXFdata *)NULL);	// Z軸移動のﾏｰｶｰ
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
			AddMakeGdataCut(pData);
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
	int			i;
	const int	nLoop = pArray->GetSize();
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

//////////////////////////////////////////////////////////////////////

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
		// 形状集合の内側から生成
		pShape->SetShapeFlag(DXFMAPFLG_SEARCH);	// 親(外側)形状集合は検索対象外
		if ( (nCnt=MakeLoopShapeSearch(pShape)) < 0 )
			return FALSE;
		if ( pShape->IsOutlineList() )
			pData = NULL;
		else {
			// 親形状集合から現在位置に近いｵﾌﾞｼﾞｪｸﾄを検索
			pt = CDXFdata::ms_pData->GetEndCutterPoint() + ptOrg;
			pShape->GetSelectObjectFromShape(pt, NULL, &pData);	// 戻り値不要
		}
		// 親形状集合自身の生成
		if ( !MakeLoopShapeAdd(pShape, pData) )
			return FALSE;
		// ﾌﾟﾛｸﾞﾚｽﾊﾞｰの更新
		nPos += nCnt+1;
		g_pParent->m_ctReadProgress.SetPos(nPos);
		// 次の形状集合を検索
		pt = CDXFdata::ms_pData->GetEndCutterPoint() + ptOrg;
		pShape = GetNearPointShape(pt);		// ﾈｲﾃｨﾌﾞ座標で検索
	}

	// 全体深彫の後処理
	if ( GetFlg(MKNC_FLG_DEEP) && GetNum(MKNC_NUM_DEEPALL)==0 && IsThread() ) {
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
	CShapeArray	obShape;	// 内側の形状集合一覧
	CRectD	rcBase( pShapeBase->GetMaxRect() );
	int		i, j, nCnt = 0;
	obShape.SetSize(0, 64);

	// pShapeBaseより内側の矩形を持つ形状集合を検索
	for ( i=0; i<g_pDoc->GetLayerCnt() && IsThread(); i++ ) {
		pLayer = g_pDoc->GetLayerData(i);
		for ( j=0; j<pLayer->GetShapeSize() && IsThread(); j++ ) {
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
		dGapMin = DBL_MAX;
		pShapeResult = NULL;
		// ptに一番近いﾈｲﾃｨﾌﾞの形状集合を検索
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
			// 形状集合のNC生成
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
	if ( pShape->IsMakeFlg() )	// 併合輪郭等で、既に生成済みの場合がある
		return TRUE;

	pShape->SetShapeFlag(DXFMAPFLG_MAKE);

	// 処理の分岐
	CDXFchain*	pChain = pShape->GetShapeChain();
	if ( pChain ) {
		if ( !pShape->IsOutlineList() )
			return MakeLoopShapeAdd_ChainList(pShape, pChain, pData);
	}
	else
		return MakeLoopShapeAdd_EulerMap(pShape);

	// 輪郭処理
	int		i;
	BOOL	bResult = TRUE;
	POSITION		pos;
	COutlineData	obMerge;
	COutlineList*	pOutlineList = pShape->GetOutlineList();
	CDXFworkingOutline*	pOutline;
	obMerge.SetSize(0, 64);

	// 形状に属する全ての輪郭ｵﾌﾞｼﾞｪｸﾄを統合
	for ( pos=pOutlineList->GetHeadPosition(); pos && IsThread(); ) {
		pOutline = pOutlineList->GetNext(pos);
		for ( i=0; i<pOutline->GetOutlineSize() && IsThread(); i++ ) {
			pChain = pOutline->GetOutlineObject(i);
			pChain->OrgTuning();
			obMerge.Add(pChain);
		}
	}

	// 統合した輪郭ｵﾌﾞｼﾞｪｸﾄを面積で並べ替え
	obMerge.Sort(AreaSizeCompareFunc);

	// NC生成
	for ( i=0; i<obMerge.GetSize() && bResult && IsThread(); i++ ) {
		pChain  = obMerge[i];
		bResult = MakeLoopShapeAdd_ChainList(pShape, pChain, NULL);
	}

	return bResult;
}

BOOL MakeLoopShapeAdd_ChainList(CDXFshape* pShape, CDXFchain* pChain, CDXFdata* pData)
{
#ifdef _DEBUG
	optional<CPointD>	ptDbg;
	CPointD		ptDbg1, ptDbg2;
	CDXFdata*	pDataDbg;
	POSITION	posDbg;
#endif
	BOOL	bReverse = FALSE, bNext = FALSE;
	double	dGap1, dGap2;
	const CPointD	ptOrg( CDXFdata::ms_ptOrg );
	CPointD	ptNow;
	CDXFdata*		pDataFix;
	CDXFworking*	pWork;

#ifdef _DEBUG
	g_dbg.printf("--- pChain Cnt = %d", pChain->GetCount());
	for ( posDbg=pChain->GetHeadPosition(); posDbg; ) {
		pDataDbg = pChain->GetNext(posDbg);
		ptDbg1 = pDataDbg->GetTunPoint(0);		// GetNativePoint() ではない
		ptDbg2 = pDataDbg->GetTunPoint(1);
		g_dbg.printf("pt=(%.3f, %.3f) - (%.3f, %.3f) %s",
			ptDbg1.x, ptDbg1.y, ptDbg2.x, ptDbg2.y,
			ptDbg && sqrt(GAPCALC(*ptDbg-ptDbg1))>=NCMIN ? "X" : " ");
		ptDbg = ptDbg2;
	}
#endif
	if ( pData ) {
		// pShape が輪郭ｵﾌﾞｼﾞｪｸﾄを持たない場合

		// 開始位置指示
		tie(pWork, pDataFix) = pShape->GetStartObject();
		if ( pDataFix ) {
			// 先頭ｵﾌﾞｼﾞｪｸﾄと現在位置を更新
			pData = pDataFix;
			ptNow = static_cast<CDXFworkingStart*>(pWork)->GetStartPoint() - ptOrg;
		}
		else
			ptNow = CDXFdata::ms_pData->GetEndCutterPoint();

		// 方向指示および生成順のﾁｪｯｸ
		tie(pWork, pDataFix) = pShape->GetDirectionObject();
		if ( pChain->GetCount()==1 &&
				(pData->GetMakeType()==DXFCIRCLEDATA ||
					(pData->GetMakeType()==DXFELLIPSEDATA && !static_cast<CDXFellipse*>(pData)->IsArc())) ) {
			if ( pDataFix ) {
				CDXFcircle*	pCircle = static_cast<CDXFcircle*>(pData);
				CPointD	pts( static_cast<CDXFworkingDirection*>(pWork)->GetStartPoint() ),
						pte( static_cast<CDXFworkingDirection*>(pWork)->GetArrowPoint() );
				BOOL	bRound = pCircle->GetRound();
				// 回転設定
				pCircle->SetRoundFixed(pts, pte);
				if ( pData->GetMakeType() == DXFELLIPSEDATA )					// 楕円の場合は
					static_cast<CDXFellipse*>(pData)->SetRoundFixed(bRound);	// 追加で独自処理
			}
			pData->GetEdgeGap(ptNow);	// 輪郭ｵﾌﾞｼﾞｪｸﾄの近接座標計算
		}
		else {
			dGap1 = GAPCALC(pData->GetStartCutterPoint() - ptNow);
			dGap2 = GAPCALC(pData->GetEndCutterPoint()   - ptNow);
			if ( pDataFix ) {
				CPointD	ptFix( static_cast<CDXFworkingDirection*>(pWork)->GetArrowPoint() - ptOrg );
				if ( pDataFix->GetEndCutterPoint().IsMatchPoint(&ptFix) ) {
					// 開始ｵﾌﾞｼﾞｪｸﾄの終点の方が近い場合は、
					// 次のｵﾌﾞｼﾞｪｸﾄから開始
					if ( dGap1 > dGap2 )
						bNext = TRUE;
				}
				else {
					bReverse = TRUE;
					// 開始ｵﾌﾞｼﾞｪｸﾄの終点は bReverse なので始点で判断
					if ( dGap1 < dGap2 )
						bNext = TRUE;
				}
			}
			else {
				if ( dGap1 > dGap2 )
					bReverse = TRUE;
			}
		}
	}
	else {
		// pShape が輪郭ｵﾌﾞｼﾞｪｸﾄを持つ場合
		ptNow = CDXFdata::ms_pData->GetEndCutterPoint();

		if ( pChain->IsLoop() ) {
			if ( pChain->GetCount() == 1 ) {
				pData = pChain->GetHead();
				pData->GetEdgeGap(ptNow);	// 輪郭ｵﾌﾞｼﾞｪｸﾄの近接座標計算
			}
			else {
				// 現在位置に近いｵﾌﾞｼﾞｪｸﾄから開始
				pChain->GetSelectObjectFromShape(ptNow+ptOrg, NULL, &pData);
				dGap1 = GAPCALC(pData->GetStartCutterPoint() - ptNow);
				dGap2 = GAPCALC(pData->GetEndCutterPoint()   - ptNow);
				if ( dGap1 > dGap2 )
					bReverse = TRUE;
			}
		}
		else {
			// 端点に近い方から開始
			dGap1 = GAPCALC(pChain->GetHead()->GetStartCutterPoint() - ptNow);
			dGap2 = GAPCALC(pChain->GetTail()->GetEndCutterPoint()   - ptNow);
			if ( dGap1 > dGap2 ) {
				bReverse = TRUE;
				pData = pChain->GetTail();
			}
			else
				pData = pChain->GetHead();
		}
	}

	POSITION	pos1, pos2,
				(CDXFchain::*pfnGetPosition)(void) const;
	CDXFdata*& (CDXFchain::*pfnGetData)(POSITION&);
	if ( bReverse ) {
		pChain->ReverseMakePt();
		pfnGetPosition	= &(CDXFchain::GetTailPosition);
		pfnGetData		= &(CDXFchain::GetPrev);
	}
	else {
		pfnGetPosition	= &(CDXFchain::GetHeadPosition);
		pfnGetData		= &(CDXFchain::GetNext);
	}

	// 開始ｵﾌﾞｼﾞｪｸﾄの検索
	if ( pChain->IsLoop() ) {
		for ( pos1=(pChain->*pfnGetPosition)(); (pos2=pos1) && IsThread(); ) {
			if ( pData == (pChain->*pfnGetData)(pos1) )
				break;
		}
		pos1 = pos2;	// pData のﾎﾟｼﾞｼｮﾝ
		// 開始ｵﾌﾞｼﾞｪｸﾄの最終調整
		if ( bNext ) {
			for ( int i=0; i<2; i++ ) {	// 自分自身と次のｵﾌﾞｼﾞｪｸﾄの[ﾎﾟｼﾞｼｮﾝ]
				pos2 = pos1;
				pData = (pChain->*pfnGetData)(pos1);
				if ( !pos1 )
					pos1 = (pChain->*pfnGetPosition)();
			}
			pos1 = pos2;
		}
	}
	else
		pos1 = pos2 = (pChain->*pfnGetPosition)();

#ifdef _DEBUG
	// CDXFchainﾘｽﾄ構造が順序良く並んでいるか
	g_dbg.printf("--- pChain Fin");
	ptDbg.reset();

	for ( posDbg=pos1; posDbg; ) {
		pDataDbg = (pChain->*pfnGetData)(posDbg);
		ptDbg1 = pDataDbg->GetTunPoint(0);
		ptDbg2 = pDataDbg->GetTunPoint(1);
		g_dbg.printf("pt=(%.3f, %.3f) - (%.3f, %.3f) %s",
			ptDbg1.x, ptDbg1.y, ptDbg2.x, ptDbg2.y,
			ptDbg && sqrt(GAPCALC(*ptDbg-ptDbg1))>=NCMIN ? "X" : " ");
		ptDbg = ptDbg2;
	}
	for ( posDbg=(pChain->*pfnGetPosition)(); posDbg!=pos2; ) {
		pDataDbg = (pChain->*pfnGetData)(posDbg);
		ptDbg1 = pDataDbg->GetTunPoint(0);
		ptDbg2 = pDataDbg->GetTunPoint(1);
		g_dbg.printf("pt=(%.3f, %.3f) - (%.3f, %.3f) %s",
			ptDbg1.x, ptDbg1.y, ptDbg2.x, ptDbg2.y,
			ptDbg && sqrt(GAPCALC(*ptDbg-ptDbg1))>=NCMIN ? "X" : " ");
		ptDbg = ptDbg2;
	}
#endif

	// 切削ﾃﾞｰﾀ生成
	if ( GetFlg(MKNC_FLG_DEEP) ) {
		// 開始ﾎﾟｼﾞｼｮﾝからﾙｰﾌﾟ
		while ( pos1 && IsThread() ) {
			pData = (pChain->*pfnGetData)(pos1);
			g_ltDeepData.AddTail(pData);
		}
		// [先頭|終端]から開始ｵﾌﾞｼﾞｪｸﾄまで
		for ( pos1=(pChain->*pfnGetPosition)(); pos1!=pos2 && IsThread(); ) {
			pData = (pChain->*pfnGetData)(pos1);
			g_ltDeepData.AddTail(pData);
		}
		// 深彫が全体か否か
		if ( GetNum(MKNC_NUM_DEEPALL) == 0 ) {
			// 最後に生成したﾃﾞｰﾀを待避
			CDXFdata::ms_pData = pData;
			// Z軸移動のﾏｰｶｰ
			g_ltDeepData.AddTail((CDXFdata *)NULL);
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
			AddMakeGdataCut(pData);
		}
		// [先頭|終端]から開始ｵﾌﾞｼﾞｪｸﾄまで
		for ( pos1=(pChain->*pfnGetPosition)(); pos1!=pos2 && IsThread(); ) {
			pData = (pChain->*pfnGetData)(pos1);
			AddMakeGdataCut(pData);
		}
		// 最後に生成したﾃﾞｰﾀを待避
		CDXFdata::ms_pData = pData;
		// Z軸上昇
		AddMoveGdataZup();
	}

	return IsThread();
}

BOOL MakeLoopShapeAdd_EulerMap(CDXFshape* pShape)
{
	BOOL		bEuler = FALSE;
	CDXFmap*	pEuler = pShape->GetShapeMap();
	ASSERT( pEuler );
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
		dGapMin = DBL_MAX;
		pt = CDXFdata::ms_pData->GetEndCutterPoint();
		for ( pos=pEuler->GetStartPosition(); pos && IsThread(); ) {
			pEuler->GetNextAssoc(pos, ptKey, pArray);
			for ( i=0; i<pArray->GetSize() && IsThread(); i++ ) {
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
		if ( !pDataResult || !IsThread() )	// ﾙｰﾌﾟ終了条件
			break;
		// 仮座標ﾏｯﾌﾟを生成
		mpLeak.RemoveAll();
		mpLeak.SetPointMap(pDataResult);	// SetMakePointMap() ではない
		pDataResult->SetSearchFlg();
		for ( i=0; i<pDataResult->GetPointNumber() && IsThread(); i++ ) {
			pt = pDataResult->GetNativePoint(i);
			if ( !MakeLoopShapeAdd_EulerMap_Search(pt, pEuler, &mpLeak) )
				return FALSE;
		}
		if ( !IsThread() )
			return FALSE;
		// ２回目以降の生成処理
		bEuler = FALSE;
		if ( !MakeLoopShapeAdd_EulerMap_Make( pShape, &mpLeak, bEuler ) )
			return FALSE;
	}

	return IsThread();
}

BOOL MakeLoopShapeAdd_EulerMap_Make(CDXFshape* pShape, CDXFmap* pEuler, BOOL& bEuler)
{
	// MakeLoopEulerAdd() 参考
	BOOL		bReverse = FALSE, bRound;
	POSITION	pos;
	CPointD		pt;
	CDXFdata*		pData;
	CDXFdata*		pDataFix;
	CDXFcircle*		pCircle;
	CDXFarray*		pArray;
	CDXFworking*	pWork;
	CDXFchain		ltEuler;
	const	CPointD	ptOrg(CDXFdata::ms_ptOrg);

	// 開始位置指示
	tie(pWork, pDataFix) = pShape->GetStartObject();
	const	CPointD		ptNow( pDataFix ?
		static_cast<CDXFworkingStart*>(pWork)->GetStartPoint() :	// 現在位置を更新
		CDXFdata::ms_pData->GetEndCutterPoint()+ptOrg );

	// この座標ﾏｯﾌﾟが一筆書き要件を満たしているか、かつ、
	// 現在位置（加工開始位置）に近いところから一時集合を生成
	tie(bEuler, pArray, pt) = pEuler->IsEulerRequirement(ptNow);
	if ( !IsThread() )
		return -1;
	pt -= ptOrg;
	ASSERT( pArray );
	ASSERT( !pArray->IsEmpty() );

	// --- 一筆書きの生成(再帰呼び出しによる木構造解析)
	if ( !MakeLoopEulerAdd_with_one_stroke(pEuler, bEuler, TRUE, pt, pArray, ltEuler) ) {
		// 一筆書きできるハズやけど失敗したら条件緩和してやり直し
		bEuler = FALSE;
		MakeLoopEulerAdd_with_one_stroke(pEuler, bEuler, TRUE, pt, pArray, ltEuler);
	}
	ASSERT( !ltEuler.IsEmpty() );

	// 方向指示および生成順のﾁｪｯｸ
	tie(pWork, pDataFix) = pShape->GetDirectionObject();
	if ( pDataFix ) {
		// 方向指示がltEulerに含まれる場合だけﾁｪｯｸ
		for ( pos=ltEuler.GetHeadPosition(); pos && IsThread(); ) {
			pData = ltEuler.GetNext(pos);
			if ( pDataFix == pData ) {
				if ( pData->GetMakeType()==DXFCIRCLEDATA || pData->GetMakeType()==DXFELLIPSEDATA ) {
					pCircle = static_cast<CDXFcircle*>(pData);
					CPointD	pts( static_cast<CDXFworkingDirection*>(pWork)->GetStartPoint() ),
							pte( static_cast<CDXFworkingDirection*>(pWork)->GetArrowPoint() );
					bRound = pCircle->GetRound();
					pCircle->SetRoundFixed(pts, pte);
					if ( pData->GetMakeType() == DXFELLIPSEDATA )
						static_cast<CDXFellipse*>(pData)->SetRoundFixed(bRound);
				}
				else {
					pt = static_cast<CDXFworkingDirection*>(pWork)->GetArrowPoint() - ptOrg;
					if ( !pData->GetEndCutterPoint().IsMatchPoint(&pt) )
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
		ltEuler.ReverseMakePt();
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
	if ( GetFlg(MKNC_FLG_DEEP) ) {
		// 切削ﾃﾞｰﾀ生成
		for ( pos=(ltEuler.*pfnGetPosition)(); pos && IsThread(); ) {
			pData = (ltEuler.*pfnGetData)(pos);
			if ( pData )
				pData->SetMakeFlg();
		}
		g_ltDeepData.AddTail(&ltEuler);
		// 深彫が全体か否か
		if ( GetNum(MKNC_NUM_DEEPALL) == 0 ) {
			// 最後に生成したﾃﾞｰﾀを待避
			CDXFdata::ms_pData = g_ltDeepData.GetTail();
			// Z軸移動のﾏｰｶｰ
			g_ltDeepData.AddTail((CDXFdata *)NULL);
		}
		else
			return MakeLoopDeepAdd();
	}
	else {
		BOOL	bNext = FALSE;
		// 切削ﾃﾞｰﾀまでの移動
		pData = (ltEuler.*pfnGetFirst)();
		if ( GetNum(MKNC_NUM_TOLERANCE) == 0 )
			AddMoveGdataG0(pData);
		else
			AddMoveGdataG1(pData);
		// 切削ﾃﾞｰﾀ生成
		for ( pos=(ltEuler.*pfnGetPosition)(); pos && IsThread(); ) {
			pData = (ltEuler.*pfnGetData)(pos);
			if ( pData ) {
				if ( bNext ) {
					AddMoveGdataZup();
					AddMoveGdataG0(pData);
					bNext = FALSE;
				}
				AddMakeGdataCut(pData);
			}
			else
				bNext = TRUE;
		}
		// 最後に生成したﾃﾞｰﾀを待避
		CDXFdata::ms_pData = pData;
		// Z軸上昇
		AddMoveGdataZup();
	}

	return IsThread();
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
				pResultMap->SetPointMap(pData);		// SetMakePointMap() ではない
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

	if ( g_ltDeepData.IsEmpty() )
		return TRUE;

	// 最後のZ軸移動ﾏｰｶは削除
	if ( g_ltDeepData.GetTail() == NULL )
		g_ltDeepData.RemoveTail();

#ifdef _DEBUG
	int	n;
	g_dbg.printf("LayerName=%s", g_ltDeepData.GetHead()->GetParentLayer()->GetStrLayer());
	for ( POSITION p=g_ltDeepData.GetHeadPosition(); p; ) {
		pData = g_ltDeepData.GetNext(p);
		if ( pData )
			n = pData->GetParentLayer()->IsCutType() ? 1 : 2;
		else
			n = 0;
		g_dbg.printf("ListType=%d", n);
	}
#endif

	// 回転数
	CString	strSpindle( CNCMakeMill::MakeSpindle(DXFLINEDATA) );
	if ( !strSpindle.IsEmpty() )
		AddMakeGdataStr(strSpindle);
	// 切削ﾃﾞｰﾀまでの移動
	AddMoveGdataG0( g_ltDeepData.GetHead() );

	// 深彫が「全体」の場合，ﾄｰﾀﾙ件数*深彫ｽﾃｯﾌﾟでﾌﾟﾛｸﾞﾚｽｺﾝﾄﾛｰﾙの再設定
	if ( GetNum(MKNC_NUM_DEEPALL) == 0 ) {
		// ﾄｰﾀﾙ件数(深彫ｽﾃｯﾌﾟｶｳﾝﾄは切り上げ)
		nCnt = (int)ceil(fabs((g_dDeep - g_dZCut) / GetDbl(MKNC_DBL_ZSTEP)))
			* g_ltDeepData.GetCount();
		SendFaseMessage( nCnt );
	}

	nCnt  = 0;
	pData = g_ltDeepData.GetHead();
	// 深彫最終位置まで仮登録ﾃﾞｰﾀのNC生成
	if ( GetNum(MKNC_NUM_DEEPALL)==1 && GetFlg(MKNC_FLG_HELICAL) &&
			g_ltDeepData.GetCount()==1 && pData->GetMakeType()==DXFCIRCLEDATA ) {
		if ( g_dZCut - g_dDeep > NCMIN ) {
			// 円ﾃﾞｰﾀのﾍﾘｶﾙ切削
			g_dZCut += GetDbl(MKNC_DBL_ZSTEP);
			if ( g_dZCut < g_dDeep )
				g_dZCut = g_dDeep;
			do {
				AddMakeGdataHelical(pData);
				g_dZCut += GetDbl(MKNC_DBL_ZSTEP);
			} while ( g_dZCut>=g_dDeep && IsThread() );
		}
	}
	else {
		// 深彫生成処理
		// g_dZCut > g_dDeep での条件では数値誤差が発生したときﾙｰﾌﾟ脱出しないため
		// g_dZCut - g_dDeep > NCMIN とした
		while ( g_dZCut - g_dDeep > NCMIN && IsThread() ) {
			pData = (*g_pfnDeepProc)(bAction, FALSE);
			CDXFdata::ms_pData = pData;
			// ｱｸｼｮﾝの切り替え(往復切削のみ)
			if ( GetNum(MKNC_NUM_DEEPROUND) == 0 ) {
				bAction = !bAction;
				// 各ｵﾌﾞｼﾞｪｸﾄの始点終点を入れ替え
				for ( pos=g_ltDeepData.GetHeadPosition(); pos && IsThread(); ) {
					pData = g_ltDeepData.GetNext(pos);
					if ( pData && pData->GetMakeType()!=DXFCIRCLEDATA ) {
						// 円ﾃﾞｰﾀ以外の座標入れ替え
						if ( pData->GetMakeType() == DXFELLIPSEDATA ) {
							CDXFellipse* pEllipse = static_cast<CDXFellipse*>(pData);
							if ( pEllipse->IsArc() )
								pEllipse->SwapMakePt(0);
							else
								pEllipse->SetRoundFixed(!pEllipse->GetRound());
						}
						else
							pData->SwapMakePt(0);
					}
				}
			}
			// Z軸の下降
			g_dZCut += GetDbl(MKNC_DBL_ZSTEP);
			if ( pData->GetParentLayer()->IsCutType() && g_dZCut-g_dDeep > NCMIN ) {
				// 一方通行切削のﾁｪｯｸ
				MakeLoopDeepZDown();
			}
			// ﾌﾟﾛｸﾞﾚｽﾊﾞｰの更新
			if ( GetNum(MKNC_NUM_DEEPALL) == 0 )
				g_pParent->m_ctReadProgress.SetPos(++nCnt * g_ltDeepData.GetCount());
		}
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
			AddMakeGdataStr( CNCMakeMill::MakeSpindle(DXFLINEDATA, TRUE) );
			// ｵﾌﾞｼﾞｪｸﾄ切削位置へ移動
			pData = bAction ? g_ltDeepData.GetHead() : g_ltDeepData.GetTail();
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
	g_ltDeepData.RemoveAll();
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
	for ( POSITION pos=(g_ltDeepData.*pfnGetPosition)(); pos && IsThread(); ) {
		pData = (g_ltDeepData.*pfnGetData)(pos);
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
	for ( POSITION pos=(g_ltDeepData.*pfnGetPosition)(); pos && IsThread(); ) {
		// ｵﾌﾞｼﾞｪｸﾄ取り出し
		pData = (g_ltDeepData.*pfnGetData)(pos);
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
	const CDXFdata* pDataHead = g_ltDeepData.GetHead();
	const CDXFdata* pDataTail = g_ltDeepData.GetTail();

	// 往復切削か一連のｵﾌﾞｼﾞｪｸﾄが閉ﾙｰﾌﾟなら
	if ( GetNum(MKNC_NUM_DEEPROUND)==0 ||
				pDataHead->GetStartMakePoint()==pDataTail->GetEndMakePoint() ) {
		// 次の深彫座標へ，Z軸の降下のみ
		AddMoveGdataZ(1, g_dZCut, GetDbl(MKNC_DBL_ZFEED));
	}
	else {
		// 一方通行切削の場合
		// まずZ軸の上昇
		MakeLoopDeepZUp();
		// 先頭ｵﾌﾞｼﾞｪｸﾄに移動
		AddMoveGdataG0(pDataHead);
	}
}

void MakeLoopDeepZUp(void)
{
	if ( GetNum(MKNC_NUM_DEEPRETURN) == 0 ) {
		// 早送りでZ軸復帰
		AddMoveGdataZup();
	}
	else {
		// R点まで切削送りでZ軸復帰
		AddMoveGdataZ(1, g_dZG0Stop, GetDbl(MKNC_DBL_MAKEENDFEED));
		// ｲﾆｼｬﾙ点復帰なら
		if ( GetNum(MKNC_NUM_ZRETURN) == 0 )
			AddMoveGdataZup();
	}
}

BOOL MakeLoopDrillPoint(CDXFdata* pData)
{
	int		nAxis = GetNum(MKNC_NUM_OPTIMAIZEDRILL);

	if ( nAxis == 0 )	// 優先軸なし
		return MakeLoopAddDrill(pData);

	int		i, nCnt, nIndex, nMatch, nProgress = 0;
	BOOL	bMatch, bMove, bCust;
	CDrillAxis	obAxis;	// 軸集合
	CDXFsort*	pAxis;
	CDXFdata*	pDataDummy;
	CDXFdata*	pDataMove;

	nAxis = nAxis - 1;	// X=0, Y=1
	obAxis.SetSize(0, g_obDrill.GetSize());

	// 軸集合の生成
	MakeLoopDrillPoint_MakeAxis(nAxis, obAxis);

	// 切削ﾃﾞｰﾀの生成
	while ( IsThread() ) {
		bCust = TRUE;
		bMove = FALSE;
		// 現在位置に近い軸集合を検索
		tie(nIndex, nMatch) = GetNearPointDrillAxis(CDXFdata::ms_pData, 1-nAxis, obAxis);
		if ( nIndex < 0 )
			break;		// 終了条件
		pAxis = obAxis[nIndex];
		if ( nMatch < 0 ) {
			// 軸の開始位置と開始方向を調整
			nMatch = MakeLoopDrillPoint_EdgeChk(nAxis, pAxis);
		}
		else if ( nMatch > 0 ) {	// ｾﾞﾛはﾁｪｯｸ不要
			// 一致したｵﾌﾞｼﾞｪｸﾄの前後どちらに近いか
			nMatch = MakeLoopDrillPoint_SeqChk(nMatch, nAxis, pAxis);
		}
		tie(nCnt, pDataMove) = MakeLoopAddDrillSeq(nProgress, nMatch, pAxis);
		nProgress += nCnt;
		// 移動ﾃﾞｰﾀﾁｪｯｸ
		while ( pDataMove && IsThread() ) {
			// 移動ﾃﾞｰﾀの１つ目の終端に穴加工ﾃﾞｰﾀがHit
			tie(pDataDummy, bMatch) = GetNearPointDrill(pDataMove);	// g_obDrill全体から検索
			if ( bMatch ) {
				// 移動ﾚｲﾔﾃﾞｰﾀは処理したことに
				pDataMove->SetMakeFlg();
				CDXFdata::ms_pData = pDataMove;
				break;
			}
			// 固定ｻｲｸﾙｷｬﾝｾﾙ
			if ( !bMove && !GetFlg(MKNC_FLG_L0CYCLE) )
				AddMakeGdataCycleCancel();
			// 移動ﾌﾗｸﾞON
			bMove = TRUE;
			// 移動ﾃﾞｰﾀ前のｶｽﾀﾑｺｰﾄﾞ挿入
			if ( bCust ) {
				(*g_pfnAddMoveCust_B)();
				bCust = FALSE;
			}
			if ( GetFlg(MKNC_FLG_L0CYCLE) ) {
				// 移動ﾃﾞｰﾀの生成(L0付き)
				AddMakeMove(pDataMove, TRUE);
			}
			else {
				// 指示されたZ位置で移動
				(*g_pfnAddMoveZ)();
				// 移動ﾃﾞｰﾀの生成
				AddMakeMove(pDataMove);
			}
			CDXFdata::ms_pData = pDataMove;
			// 次の移動ﾃﾞｰﾀを検索
			pDataMove = GetMatchPointMove(pDataMove);
		}
		// 移動ﾃﾞｰﾀの後処理
		if ( bMove && IsThread() ) {
			// 移動ﾃﾞｰﾀの終点でﾃｷｽﾄﾃﾞｰﾀの生成
			AddMoveTextIntegrated(CDXFdata::ms_pData->GetEndCutterPoint());
			// ｶｽﾀﾑｺｰﾄﾞ
			(*g_pfnAddMoveCust_A)();
		}
	}

	// 後片づけ
	for ( i=0; i<obAxis.GetSize(); i++ )
		delete	obAxis[i];

	return IsThread();
}

void MakeLoopDrillPoint_MakeAxis(int nAxis, CDrillAxis& obAxis)
{
	CDXFsort*	pAxis;
	int			i = 0;
	const int	nLoop = g_obDrill.GetSize();
	double		dBase, dMargin = fabs(GetDbl(MKNC_DBL_DRILLMARGIN)) * 2.0;
	CDXFsort::PFNCOMPARE	pfnCompare;
	
	// 基準軸で並べ替え
	if ( nAxis == 0 ) {
		g_obDrill.Sort(DrillOptimaizeCompareFuncY);
		pfnCompare = DrillOptimaizeCompareFuncX;
	}
	else {
		g_obDrill.Sort(DrillOptimaizeCompareFuncX);
		pfnCompare = DrillOptimaizeCompareFuncY;
	}

	nAxis = 1 - nAxis;

	// 軸ｸﾞﾙｰﾌﾟの生成
	while ( i < nLoop && IsThread() ) {
		dBase = g_obDrill[i]->GetEndCutterPoint()[nAxis] + dMargin;
		pAxis = new CDXFsort;
		while ( i < nLoop &&
				g_obDrill[i]->GetEndCutterPoint()[nAxis] <= dBase && IsThread() ) {
			pAxis->Add(g_obDrill[i++]);
		}
		pAxis->Sort(pfnCompare);
		obAxis.Add(pAxis);
	}
}

int MakeLoopDrillPoint_EdgeChk(int nAxis, CDXFsort* pAxis)
{
	int			i, nFirst = -1, nLast = -1;
	const int	nLoop = pAxis->GetSize();
	CPointD		pts, pte,
				ptNow(CDXFdata::ms_pData->GetEndCutterPoint());
	CDXFdata*	pData;

	// 先頭の未生成ﾃﾞｰﾀを検索
	for ( i=0; i<nLoop; i++ ) {
		pData = pAxis->GetAt(i);
		if ( !pData->IsMakeFlg() ) {
			pts = pData->GetEndCutterPoint();
			nFirst = i;
			break;
		}
	}
	ASSERT( nFirst >= 0 );

	// 末尾の未生成ﾃﾞｰﾀを検索
	for ( i=nLoop-1; i>=0; i-- ) {
		pData = pAxis->GetAt(i);
		if ( !pData->IsMakeFlg() ) {
			pte = pData->GetEndCutterPoint();
			nLast = i;
			break;
		}
	}

	// 現在位置からのどちらが近いか(距離ではなく基準軸で)
	if ( nFirst!=nLast &&
			fabs(pts[nAxis] - ptNow[nAxis]) > fabs(pte[nAxis] - ptNow[nAxis]) ) {
		pAxis->Reverse();
		nFirst = nLoop - nLast - 1;	// 逆順にしたときの新しい開始位置
	}
	
	return nFirst;
}

int MakeLoopDrillPoint_SeqChk(int nMatch, int nAxis, CDXFsort* pAxis)
{
	int			i, nFirst = -1, nLast = -1, nCntF = 0, nCntL = 0;
	const int	nLoop = pAxis->GetSize();
	CPointD		pts, pte,
				ptNow(pAxis->GetAt(nMatch)->GetEndCutterPoint());
	CDXFdata*	pData;

	// 先頭方向へ
	for ( i=nMatch-1; i>=0; i-- ) {
		pData = pAxis->GetAt(i);
		if ( !pData->IsMakeFlg() ) {
			if ( nFirst < 0 ) {
				pts = pData->GetEndCutterPoint();
				nFirst = i;
			}
			nCntF++;	// 先頭方向への残り件数
		}
	}
	if ( nFirst < 0 )
		return nMatch;		// 順序入れ替え不要

	// 末尾方向へ
	for ( i=nMatch+1; i<nLoop; i++ ) {
		pData = pAxis->GetAt(i);
		if ( !pData->IsMakeFlg() ) {
			if ( nLast < 0 ) {
				pte = pData->GetEndCutterPoint();
				nLast = i;
			}
			nCntL++;	// 末尾方向への残り件数
		}
	}
	if ( nLast < 0 ) {
		pAxis->Reverse();	// 末尾方向にﾃﾞｰﾀ無し
		nMatch = nLoop - nMatch - 1;	// 逆順にしたときの新しい開始位置
		return nMatch;
	}

	double	dGap = fabs(pts[nAxis]-ptNow[nAxis]) - fabs(pte[nAxis]-ptNow[nAxis]);

	if ( fabs(dGap) < NCMIN ) {
		// 前後の距離が等しいので残り件数の多い方へ
		if ( nCntF > nCntL ) {
			pAxis->Reverse();
			nMatch = nLoop - nMatch - 1;
		}
	}
	else if ( dGap < 0 ) {	// pts < pte
		// 先頭方向の方が近い
		pAxis->Reverse();
		nMatch = nLoop - nMatch - 1;
	}

	return nMatch;
}

BOOL MakeLoopDrillCircle(void)
{
	CDXFdata*	pData;
	CDXFdata*	pDataResult;
	int			i;
	const int	nLoop = g_obCircle.GetSize();
	BOOL		bMatch;
	double		r, dGap, dGapMin;
	CString		strBreak;

	// ﾙｰﾌﾟ開始
	g_obDrill.SetSize(0, nLoop);
	for ( i=0; i<nLoop && IsThread(); ) {
		g_obDrill.RemoveAll();
		// 円をｸﾞﾙｰﾌﾟ(半径)ごとに処理
		r = fabs( static_cast<CDXFcircle*>(g_obCircle[i])->GetMakeR() );
		bMatch = FALSE;
		pDataResult = NULL;
		dGapMin = DBL_MAX;
		for ( ; i<nLoop &&
				r==fabs(static_cast<CDXFcircle*>(g_obCircle[i])->GetMakeR()) && IsThread(); i++ ) {
			pData = g_obCircle[i];
			if ( !pData->IsMakeFlg() ) {
				g_obDrill.Add(pData);
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
				strBreak.Format(IDS_MAKENCD_CIRCLEBREAK, static_cast<CDXFcircle*>(pDataResult)->GetMakeR());
				AddMakeGdataStr(strBreak);
			}
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

	// 消去しておかないと、「円ﾃﾞｰﾀを先に」のときに
	// 次の実点穴加工でﾃﾞｰﾀが処理される
	g_obDrill.RemoveAll();

	return IsThread();
}

BOOL MakeLoopAddDrill(CDXFdata* pData)
{
	CDXFdata*	pDataMove;
	int			nPos = 0;
	BOOL		bMatch,
				bMove = FALSE,		// 移動ﾚｲﾔHit
				bCust = TRUE;		// 移動ﾃﾞｰﾀ前のｶｽﾀﾑｺｰﾄﾞ挿入

	// ﾃﾞｰﾀ生成
	AddMakeGdataDrill(pData);
	CDXFdata::ms_pData = pData;

	// GetNearPoint() の結果が NULL になるまで
	while ( IsThread() ) {
		// この要素に一番近い要素
		tie(pData, bMatch) = GetNearPointDrill(CDXFdata::ms_pData);
		if ( !pData )
			break;
		if ( !bMatch ) {
			// 移動指示ﾚｲﾔのﾁｪｯｸ
			if ( (pDataMove=GetMatchPointMove(CDXFdata::ms_pData)) ) {
				// 移動ﾚｲﾔ１つ目の終端に穴加工ﾃﾞｰﾀがHitすれば
				tie(pData, bMatch) = GetNearPointDrill(pDataMove);
				if ( bMatch ) {
					// 移動ﾚｲﾔﾃﾞｰﾀは処理したことにして，この穴加工ﾃﾞｰﾀを生成
					pDataMove->SetMakeFlg();
				}
				else {
					bMove = TRUE;
					// 固定ｻｲｸﾙｷｬﾝｾﾙ
					if ( !GetFlg(MKNC_FLG_L0CYCLE) )
						AddMakeGdataCycleCancel();
					// 移動ﾃﾞｰﾀ前のｶｽﾀﾑｺｰﾄﾞ挿入
					if ( bCust ) {
						(*g_pfnAddMoveCust_B)();
						bCust = FALSE;
					}
					if ( GetFlg(MKNC_FLG_L0CYCLE) ) {
						// 移動ﾃﾞｰﾀの生成(L0付き)
						AddMakeMove(pDataMove, TRUE);
					}
					else {
						// 指示されたZ位置で移動
						(*g_pfnAddMoveZ)();
						// 移動ﾃﾞｰﾀの生成
						AddMakeMove(pDataMove);
					}
					CDXFdata::ms_pData = pDataMove;
					continue;	// 再探索
				}
			}
		}
		bCust = TRUE;
		// 移動ﾃﾞｰﾀ後のｶｽﾀﾑｺｰﾄﾞ挿入
		if ( bMove && IsThread() ) {
			// 移動ﾃﾞｰﾀの終点でﾃｷｽﾄﾃﾞｰﾀの生成
			AddMoveTextIntegrated(CDXFdata::ms_pData->GetEndCutterPoint());
			// ｶｽﾀﾑｺｰﾄﾞ
			(*g_pfnAddMoveCust_A)();
			bMove = FALSE;
		}
		// ﾃﾞｰﾀ生成
		AddMakeGdataDrill(pData);
		CDXFdata::ms_pData = pData;
		SendProgressPos(++nPos);
	} // End of while

	g_pParent->m_ctReadProgress.SetPos(nPos);

	return IsThread();
}

tuple<int, CDXFdata*> MakeLoopAddDrillSeq(int nProgress, int nStart, CDXFsort* pAxis)
{
	int		i, nCnt = 0;
	CDXFdata*	pData;
	CDXFdata*	pDataMove = NULL;

	for ( i=nStart; i<pAxis->GetSize() && IsThread(); i++ ) {
		pData = pAxis->GetAt(i);
		if ( pData->IsMakeFlg() )
			continue;
		// ﾃﾞｰﾀ生成
		AddMakeGdataDrill(pData);
		SendProgressPos(i+nProgress);
		nCnt++;
		// 移動ﾚｲﾔのﾁｪｯｸ
		if ( (pDataMove=GetMatchPointMove(pData)) )
			break;
	}
	// 最後に生成したﾃﾞｰﾀを待避
	CDXFdata::ms_pData = pData;

	return make_tuple(nCnt, pDataMove);
}

CDXFdata* MakeLoopAddFirstMove(ENMAKETYPE enType)
{
	CDXFarray*	pobArray;
	CDXFdata*	pDataMove;
	CDXFdata*	pDataResult = NULL;
	CDXFdata*	pData;
	BOOL		bMatch, bCust = FALSE;
	CPointD		pt;

	// 移動指示ﾚｲﾔのﾁｪｯｸ
	while ( (pDataMove=GetMatchPointMove(CDXFdata::ms_pData)) && IsThread() ) {
		if ( !bCust ) {
			// 移動ﾃﾞｰﾀ前のｶｽﾀﾑｺｰﾄﾞ挿入(１回だけ)
			(*g_pfnAddMoveCust_B)();
			bCust = TRUE;
		}
		// 指示されたZ位置で移動
		(*g_pfnAddMoveZ)();
		// 移動ﾃﾞｰﾀの生成
		AddMakeMove(pDataMove);
		CDXFdata::ms_pData = pDataResult = pDataMove;
		// 同一座標で切削ﾃﾞｰﾀが見つかれば break
		if ( enType == MAKECUTTER ) {
			pt = pDataMove->GetEndCutterPoint();
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
			tie(pData, bMatch) = GetNearPointDrill(pDataMove);
			if ( bMatch ) {
				pDataResult = pData;
				break;
			}
		}
	}
	// 移動ﾃﾞｰﾀ後のｶｽﾀﾑｺｰﾄﾞ挿入
	if ( bCust && IsThread() ) {
		// 移動ﾃﾞｰﾀの終点でﾃｷｽﾄﾃﾞｰﾀの生成
		AddMoveTextIntegrated(CDXFdata::ms_pData->GetEndCutterPoint());
		// ｶｽﾀﾑｺｰﾄﾞ
		(*g_pfnAddMoveCust_A)();
	}

	return pDataResult;
}

BOOL MakeLoopAddLastMove(void)
{
	CDXFdata*	pDataMove;
	BOOL		bCust = FALSE;

	// 終点座標でのｺﾒﾝﾄ生成
	CPointD	pt( CDXFdata::ms_pData->GetEndCutterPoint() );
	AddCutterTextIntegrated(pt);	// 切削ﾚｲﾔ
	AddMoveTextIntegrated(pt);		// 移動ﾚｲﾔ

	// 最後の移動ｺｰﾄﾞを検索
	while ( (pDataMove=GetMatchPointMove(CDXFdata::ms_pData)) && IsThread() ) {
		if ( !bCust ) {
			(*g_pfnAddMoveCust_B)();
			bCust = TRUE;
		}
		(*g_pfnAddMoveZ)();
		AddMakeMove(pDataMove);
		CDXFdata::ms_pData = pDataMove;
	}
	if ( bCust && IsThread() ) {
		AddMoveTextIntegrated(CDXFdata::ms_pData->GetEndCutterPoint());
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
	double		dGap, dGapMin = DBL_MAX;

	while ( nLayerLoop-- > 0 && IsThread() ) {
		pLayer = pLayerTarget ? const_cast<CLayerData*>(pLayerTarget) : g_pDoc->GetLayerData(nLayerLoop);
		if ( !pLayer->IsMakeTarget() )
			continue;
		for ( i=0; i<pLayer->GetDxfSize() && IsThread(); i++ ) {
			pData = pLayer->GetDxfData(i);
			if ( !pData->IsMakeFlg() && pData->GetMakeType()!=DXFPOINTDATA && pData->IsMakeTarget() ) {
				dGap = pData->GetEdgeGap(pDataTarget);
				if ( dGap < dGapMin ) {
					pDataResult = pData;
					dGapMin = dGap;
				}
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
	int			i, j, nLoop1 = g_pDoc->GetLayerCnt(), nLoop2;
	double		dGap, dGapMin = DBL_MAX;

	for ( i=0; i<nLoop1 && IsThread(); i++ ) {
		pLayer = g_pDoc->GetLayerData(i);
		if ( !pLayer->IsMakeTarget() )
			continue;
		nLoop2 = pLayer->GetShapeSize();
		for ( j=0; j<nLoop2 && IsThread(); j++ ) {
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

tuple<CDXFdata*, BOOL> GetNearPointDrill(const CDXFdata* pDataTarget)
{
	CDXFdata*	pData;
	CDXFdata*	pDataResult = NULL;
	BOOL	bMatch = FALSE;
	double	dGap, dGapMin = DBL_MAX;	// 指定点との距離

	// 現在位置と等しい，または近い要素を検索
	for ( int i=0; i<g_obDrill.GetSize() && IsThread(); i++ ) {
		pData = g_obDrill[i];
		if ( pData->IsMakeFlg() )
			continue;
		// 条件判断
		if ( pData->IsMakeMatchObject(pDataTarget) ) {
			pDataResult = pData;
			bMatch = TRUE;
			break;
		}
		// 現在位置との距離計算
		dGap = pData->GetEdgeGap(pDataTarget);
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

tuple<int, int> GetNearPointDrillAxis(const CDXFdata* pDataTarget, int nAxis, CDrillAxis& obAxis)
{
	CDXFsort*	pAxis;
	CDXFdata*	pData;
	int		i, j, nMatch = -1, nResult = -1;
	double	dGap, dGapMin = DBL_MAX;
	CPointD	pt1(pDataTarget->GetEndMakePoint()), pt2;

	for ( i=0; i<obAxis.GetSize() && nMatch<0 && IsThread(); i++ ) {
		pAxis = obAxis[i];
		for ( j=0; j<pAxis->GetSize(); j++ ) {
			pData = pAxis->GetAt(j);
			if ( pData->IsMakeFlg() )
				continue;
			if ( pData->IsMakeMatchPoint(pt1) ) {	// pData->IsMakeMatchObject(pDataTarget)
				nResult = i;
				nMatch  = j;		// 外側ﾙｰﾌﾟもbreak
				break;
			}
			pt2 = pData->GetEndMakePoint();
			dGap = fabs(pt2[nAxis] - pt1[nAxis]);	// 指定軸の距離で判断
			if ( dGap < dGapMin ) {
				nResult = i;
				dGapMin = dGap;
			}
		}
	}

	return make_tuple(nResult, nMatch);
}

CDXFdata* GetMatchPointMove(const CDXFdata* pDataTarget)
{
	// 現在位置と等しい要素だけを検索
	CPointD	pt( pDataTarget->GetEndCutterPoint() );
	CDXFarray*	pobArray;
	CDXFdata*	pDataResult = NULL;
	
	if ( g_mpDXFmove.Lookup(pt, pobArray) ) {
		CDXFdata*	pData;
		for ( int i=0; i<pobArray->GetSize() && IsThread(); i++ ) {
			pData = pobArray->GetAt(i);
			if ( !pData->IsMakeFlg() ) {
				pData->GetEdgeGap(pt);	// 始点を調整
				pDataResult = pData;
				break;
			}
		}
	}

	return pDataResult;
}

//	AddCustomCode() parse()関数から呼び出し
class CMakeCustomCode_Mill : public CMakeCustomCode	// MakeCustomCode.h
{
public:
	CMakeCustomCode_Mill(CString& r, const CDXFdata* pData) :
				CMakeCustomCode(r, g_pDoc, pData, g_pMakeOpt) {
		static	LPCTSTR	szCustomCode[] = {
			"ProgNo", 
			"G90orG91", "G92_INITIAL", "G92X", "G92Y", "G92Z",
			"SPINDLE", "G0XY_INITIAL"
		};
		// ｵｰﾀﾞｰ追加
		m_strOrderIndex.AddElement(SIZEOF(szCustomCode), szCustomCode);
	}

	void	operator()(const char* s, const char* e) const {
		extern	const	DWORD	g_dwSetValFlags[];
		double	dValue[VALUESIZE];

		// 基底ｸﾗｽ呼び出し
		int		nTestCode = CMakeCustomCode::ReplaceCustomCode(s, e);

		// replace
		switch ( nTestCode ) {
		case 0:		// ProgNo
			if ( GetFlg(MKNC_FLG_PROG) ) {
				CString	strTmp;
				strTmp.Format(IDS_MAKENCD_PROG,
					GetFlg(MKNC_FLG_PROGAUTO) ? ::GetRandom(1,9999) : GetNum(MKNC_NUM_PROG));
				m_strResult += strTmp;
			}
			break;
		case 1:		// G90orG91
			m_strResult += CNCMakeBase::MakeCustomString(GetNum(MKNC_NUM_G90)+90);
			break;
		case 2:		// G92_INITIAL
			dValue[NCA_X] = GetDbl(MKNC_DBL_G92X);
			dValue[NCA_Y] = GetDbl(MKNC_DBL_G92Y);
			dValue[NCA_Z] = GetDbl(MKNC_DBL_G92Z);
			m_strResult += CNCMakeBase::MakeCustomString(92, NCD_X|NCD_Y|NCD_Z, dValue, FALSE);
			break;
		case 3:		// G92X
		case 4:		// G92Y
		case 5:		// G92Z
			nTestCode -= 3;
			dValue[nTestCode] = GetDbl(MKNC_DBL_G92X+nTestCode);
			m_strResult += CNCMakeBase::MakeCustomString(-1, g_dwSetValFlags[nTestCode], dValue, FALSE);
			break;
		case 6:		// SPINDLE
			if ( m_pData )					// Header
				m_strResult += CNCMakeMill::MakeSpindle(m_pData->GetMakeType());
			else if ( CDXFdata::ms_pData )	// Footer
				m_strResult += CNCMakeMill::MakeSpindle(CDXFdata::ms_pData->GetMakeType());
			break;
		case 7:		// G0XY_INITIAL
			dValue[NCA_X] = GetDbl(MKNC_DBL_G92X);
			dValue[NCA_Y] = GetDbl(MKNC_DBL_G92Y);
			m_strResult += CNCMakeBase::MakeCustomString(0, NCD_X|NCD_Y, dValue);
			break;
		}
	}
};

void AddCustomCode(const CString& strFileName, const CDXFdata* pData)
{
	using namespace boost::spirit::classic;

	CString	strBuf;
	CString	strResult;
	CMakeCustomCode_Mill	custom(strResult, pData);

	try {
		CStdioFile	fp(strFileName,
			CFile::modeRead | CFile::shareDenyWrite | CFile::typeText);
		while ( fp.ReadString(strBuf) && IsThread() ) {
			// 構文解析
			strResult.Empty();
			if ( parse((LPCTSTR)strBuf, *( *(anychar_p - '{')[custom] >> comment_p('{', '}')[custom] ) ).hit ) {
				if ( !strResult.IsEmpty() )
					AddMakeGdataStr( strResult );
			}
			else
				AddMakeGdataStr( strBuf );
		}
	}
	catch (CFileException* e) {
		AfxMessageBox(IDS_ERR_MAKECUSTOM, MB_OK|MB_ICONEXCLAMATION);
		e->Delete();
		// このｴﾗｰは正常ﾘﾀｰﾝ(警告のみ)
	}
}

// 加工開始指示ﾃﾞｰﾀの生成
void AddMakeStart(void)
{
	// 機械原点でのﾃｷｽﾄﾃﾞｰﾀ生成
	AddStartTextIntegrated(CPointD());	// (0, 0)
	
	CDXFdata*	pData = NULL;
	CNCMakeMill*	mkNCD;
	// 特定条件しか呼ばれないので IsMakeFlg() の判断は必要なし
	for ( int i=0; i<g_obStartData.GetSize() && IsThread(); i++ ) {
		pData = g_obStartData[i];
		// 開始位置と等しいﾃｷｽﾄﾃﾞｰﾀの生成
		AddStartTextIntegrated(pData->GetStartCutterPoint());
		// 移動指示
		mkNCD = new CNCMakeMill(pData, FALSE);
		ASSERT( mkNCD );
		g_obMakeData.Add(mkNCD);
	}
	if ( pData ) {
		// 最後の生成ﾃﾞｰﾀ
		CDXFdata::ms_pData = pData;
		// 終了位置と等しいﾃｷｽﾄﾃﾞｰﾀの生成
		AddStartTextIntegrated(pData->GetEndCutterPoint());
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
	if ( CNCMakeMill::ms_xyz[NCA_Z] < g_dZG0Stop )
		AddMoveGdataZ(0, g_dZG0Stop, -1);
}

void AddMoveZ_Initial(void)
{
	// Z軸の現在位置がｲﾆｼｬﾙ点より小さい(低い)なら
	if ( CNCMakeMill::ms_xyz[NCA_Z] < g_dZInitial )
		AddMoveGdataZ(0, g_dZInitial, -1);
}

void AddMoveCust_B(void)
{
	AddMakeGdataStr(g_pMakeOpt->GetStr(MKNC_STR_CUSTMOVE_B));
}

void AddMoveCust_A(void)
{
	AddMakeGdataStr(g_pMakeOpt->GetStr(MKNC_STR_CUSTMOVE_A));
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
				AddMakeGdataStr( "(" + ((CDXFtext *)pData)->GetStrValue() + ")" );
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
	(int nRange/*=-1*/, int nMsgID/*=-1*/, LPCTSTR lpszMsg/*=NULL*/)
{
#ifdef _DEBUG
	CMagaDbg	dbg("MakeNCD_Thread()", DBG_GREEN);
	dbg.printf("Phase%d Start", g_nFase);
#endif
	if ( nRange > 0 )
		g_pParent->m_ctReadProgress.SetRange32(0, nRange);

	CString	strMsg;
	if ( nMsgID > 0 )
		VERIFY(strMsg.LoadString(nMsgID));
	else
		strMsg.Format(IDS_MAKENCD_FASE, g_nFase);
	g_pParent->SetFaseMessage(strMsg, lpszMsg);
	g_nFase++;
}

//////////////////////////////////////////////////////////////////////
// 並べ替え補助関数
//////////////////////////////////////////////////////////////////////

int CircleSizeCompareFunc1(CDXFdata* pFirst, CDXFdata* pSecond)
{
	int		nResult;
	double	dResult = fabs(static_cast<CDXFcircle*>(pFirst)->GetMakeR()) -
						fabs(static_cast<CDXFcircle*>(pSecond)->GetMakeR());
	if ( dResult == 0.0 )
		nResult = 0;
	else if ( dResult > 0.0 )
		nResult = 1;
	else
		nResult = -1;
	return nResult;
}

int CircleSizeCompareFunc2(CDXFdata* pFirst, CDXFdata* pSecond)
{
	int		nResult;
	double	dResult = fabs(static_cast<CDXFcircle*>(pSecond)->GetMakeR()) -
						fabs(static_cast<CDXFcircle*>(pFirst)->GetMakeR());
	if ( dResult == 0.0 )
		nResult = 0;
	else if ( dResult > 0.0 )
		nResult = 1;
	else
		nResult = -1;
	return nResult;
}

int DrillOptimaizeCompareFuncX(CDXFdata* pFirst, CDXFdata* pSecond)
{
	int		nResult;
	double	dResult = pFirst->GetEndCutterPoint().x - pSecond->GetEndCutterPoint().x;
	if ( dResult == 0.0 )
		nResult = 0;
	else if ( dResult > 0.0 )
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
	else if ( dResult > 0.0 )
		nResult = 1;
	else
		nResult = -1;
	return nResult;
}

int AreaSizeCompareFunc(CDXFchain* pFirst, CDXFchain* pSecond)
{
	int		nResult;
	CRectD	rc1(pFirst->GetMaxRect()), rc2(pSecond->GetMaxRect());
	double	dResult = rc1.Width() * rc1.Height() - rc2.Width() * rc2.Height();
	if ( dResult == 0.0 )
		nResult = 0;
	else if ( dResult > 0.0 )
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
	for ( int i=0; i<g_obMakeData.GetSize(); i++ )
		delete	g_obMakeData[i];
	g_obMakeData.RemoveAll();
	g_mpDXFdata.RemoveAll();
	g_mpDXFstarttext.RemoveAll();
	g_mpDXFmove.RemoveAll();
	g_mpDXFmovetext.RemoveAll();
	g_mpDXFtext.RemoveAll();
	g_mpDXFcomment.RemoveAll();
	g_obStartData.RemoveAll();
	g_ltDeepData.RemoveAll();
	g_obDrill.RemoveAll();
	g_obCircle.RemoveAll();

	g_csMakeAfter.Unlock();
	return 0;
}
