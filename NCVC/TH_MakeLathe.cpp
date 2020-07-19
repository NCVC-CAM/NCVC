// TH_MakeLathe.cpp
//		旋盤用NC生成
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "DXFdata.h"
#include "DXFshape.h"
#include "Layer.h"
#include "DXFDoc.h"
#include "NCDoc.h"		// g_szNCcomment[]
#include "NCMakeLatheOpt.h"
#include "NCMakeLathe.h"
#include "MakeCustomCode.h"
#include "ThreadDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
//#define	_DBG_NCMAKE_TIME	//	生成時間の表示
#endif

using std::string;
using namespace boost;

extern	LPCTSTR	g_szNCcomment[];

// ｸﾞﾛｰﾊﾞﾙ変数定義
static	CThreadDlg*			g_pParent;
static	CDXFDoc*			g_pDoc;
static	CNCMakeLatheOpt*	g_pMakeOpt;

// よく使う変数や呼び出しの簡略置換
#define	MAXLAYER	3	// 内径・外径・突切
#define	IsThread()	g_pParent->IsThreadContinue()
#define	GetFlg(a)	g_pMakeOpt->GetFlag(a)
#define	GetNum(a)	g_pMakeOpt->GetNum(a)
#define	GetDbl(a)	g_pMakeOpt->GetDbl(a)
#define	GetStr(a)	g_pMakeOpt->GetStr(a)

// NC生成に必要なﾃﾞｰﾀ群
static	CDXFshape*	g_pShape[2];	// 0:Inside, 1:Outside
static	CLayerData*	g_pGrooveLayer;
static	CDXFarray	g_obLineTemp[2];
static	CDXFarray	g_obMakeLine[2];
static	CTypedPtrArrayEx<CPtrArray, CNCMakeLathe*>	g_obMakeData;	// 加工ﾃﾞｰﾀ

// ｻﾌﾞ関数
static	void	InitialVariable(void);			// 変数初期化
static	void	SetStaticOption(void);			// 静的変数の初期化
static	BOOL	MakeLathe_MainFunc(void);		// NC生成のﾒｲﾝﾙｰﾌﾟ
static	BOOL	CreateShapeThread(void);		// 形状認識処理
static	void	InitialShapeData(void);			// 形状認識の初期化
static	BOOL	CreateInsidePitch(float&);		// 内径ｵﾌｾｯﾄを生成
static	BOOL	CreateOutsidePitch(void);		// 外径ｵﾌｾｯﾄを生成
static	BOOL	CreateRoughPass(int);			// 荒加工ﾃﾞｰﾀの生成(内外共通)
static	BOOL	MakeInsideCode(const CPointF&);	// NCｺｰﾄﾞの生成
static	BOOL	MakeOutsideCode(const CPointF&);
static	BOOL	MakeGrooveCode(const CPointF&);
static	BOOL	CheckXZMove(int, const CPointF&, const CPointF&);
static	CPointF	MoveInsideCode(const CDXFchain*, const CPointF&, const CPointF&);
static	void	MoveOutsideCode(const CDXFdata*, const CPointF&, const CPointF&);
static	BOOL	OutputLatheCode(void);			// NCｺｰﾄﾞの出力

// ﾍｯﾀﾞｰ,ﾌｯﾀﾞｰ等のｽﾍﾟｼｬﾙｺｰﾄﾞ生成
static	void	AddCustomLatheCode(const CString&);

// 任意ﾃﾞｰﾀの生成
static inline	void	AddMakeLatheStr(const CString& strData)
{
	CNCMakeLathe*	pNCD = new CNCMakeLathe(strData);
	ASSERT( pNCD );
	g_obMakeData.Add(pNCD);
}

// ﾌｪｰｽﾞ更新
static	int		g_nFase;			// ﾌｪｰｽﾞ№
static	void	SendFaseMessage(INT_PTR = -1, int = -1, LPCTSTR = NULL);
static	inline	void	SetProgressPos(INT_PTR n)
{
	g_pParent->m_ctReadProgress.SetPos((int)n);
}

// 並べ替え補助関数
static	int		ShapeSortFunc(CDXFshape*, CDXFshape*);	// 形状認識ﾃﾞｰﾀの並べ替え
static	int		GrooveSortFunc(CDXFdata*, CDXFdata*);	// 突っ切り加工のﾃﾞｰﾀ並べ替え

// ｻﾌﾞｽﾚｯﾄﾞ関数
static	CCriticalSection	g_csMakeAfter;	// MakeLathe_AfterThread()ｽﾚｯﾄﾞﾛｯｸｵﾌﾞｼﾞｪｸﾄ
static	UINT	MakeLathe_AfterThread(LPVOID);	// 後始末ｽﾚｯﾄﾞ

//////////////////////////////////////////////////////////////////////
// 旋盤用NC生成ｽﾚｯﾄﾞ
//////////////////////////////////////////////////////////////////////

UINT MakeLathe_Thread(LPVOID pVoid)
{
#ifdef _DEBUG
	printf("MakeLathe_Thread() Start\n");
#endif
#ifdef _DBG_NCMAKE_TIME
	// 現在時刻を取得
	CTime t1 = CTime::GetCurrentTime();
#endif

	int		nResult = IDCANCEL;

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
		// NC生成ｵﾌﾟｼｮﾝｵﾌﾞｼﾞｪｸﾄの生成とｵﾌﾟｼｮﾝの読み込み
		g_pMakeOpt = new CNCMakeLatheOpt(
				AfxGetNCVCApp()->GetDXFOption()->GetInitList(NCMAKELATHE)->GetHead());
		// NC生成のﾙｰﾌﾟ前に必要な初期化
		InitialVariable();
		// 条件ごとに変化するﾊﾟﾗﾒｰﾀを設定
		SetStaticOption();
		// 変数初期化ｽﾚｯﾄﾞの処理待ち
		g_csMakeAfter.Lock();		// ｽﾚｯﾄﾞ側でﾛｯｸ解除するまで待つ
		g_csMakeAfter.Unlock();
		// 増分割り当て
		g_obMakeData.SetSize(0, 1024);		// 旋盤ﾃﾞｰﾀ生成は未知数
		// 生成開始
		BOOL bResult = MakeLathe_MainFunc();
		if ( bResult )
			bResult = OutputLatheCode();

		// 戻り値ｾｯﾄ
		if ( bResult && IsThread() )
			nResult = IDOK;
#ifdef _DEBUG
		printf("MakeLathe_Thread All Over!!!\n");
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

	// ここの情報ではないので終了前に先に消去しておく
	// MakeLathe_AfterThreadｽﾚｯﾄﾞ内での消去もNG
	for ( int i=0; i<SIZEOF(g_pShape); i++ ) {
		if ( g_pShape[i] )
			g_pShape[i]->CrearScanLine_Lathe();
	}

	// 終了処理
	g_pParent->PostMessage(WM_USERFINISH, nResult);	// このｽﾚｯﾄﾞからﾀﾞｲｱﾛｸﾞ終了
	// 生成したNCｺｰﾄﾞの消去ｽﾚｯﾄﾞ(優先度を下げる)
	AfxBeginThread(MakeLathe_AfterThread, NULL,
//		THREAD_PRIORITY_LOWEST);
		THREAD_PRIORITY_IDLE);
//		THREAD_PRIORITY_BELOW_NORMAL;

	// 条件ｵﾌﾞｼﾞｪｸﾄ削除
	if ( g_pMakeOpt )
		delete	g_pMakeOpt;

	return 0;
}

//////////////////////////////////////////////////////////////////////

void InitialVariable(void)
{
	ZEROCLR(g_pShape);
	g_pGrooveLayer = NULL;
	// 端面の終点が原点
	CDXFdata::ms_ptOrg = g_pDoc->GetLatheLine(1)->GetNativePoint(1);
	// ORIGINﾃﾞｰﾀが工具初期位置
	CDXFdata::ms_pData = g_pDoc->GetCircleObject();
	// 各ｵﾌﾞｼﾞｪｸﾄの原点調整
	g_pDoc->GetCircleObject()->OrgTuning(FALSE);
	g_pDoc->GetLatheLine(0)->OrgTuning(FALSE);
	g_pDoc->GetLatheLine(1)->OrgTuning(FALSE);
	// 生成ｵﾌﾟｼｮﾝの初期化
	CNCMakeLathe::InitialVariable();		// CNCMakeBase::InitialVariable()
}

void SetStaticOption(void)
{
	// CDXFdataの静的変数初期化
	CDXFdata::ms_fXRev = CDXFdata::ms_fYRev = FALSE;

	// CNCMakeLatheの静的変数初期化
	CPointF	ptOrg(g_pDoc->GetCircleObject()->GetStartMakePoint());
	CNCMakeLathe::ms_xyz[NCA_X] = ptOrg.y;
	CNCMakeLathe::ms_xyz[NCA_Y] = 0.0f;
	CNCMakeLathe::ms_xyz[NCA_Z] = ptOrg.x;

	// 生成ｵﾌﾟｼｮﾝによる静的変数の初期化
	CNCMakeLathe::SetStaticOption(g_pMakeOpt);
}

BOOL OutputLatheCode(void)
{
	CString	strPath, strFile,
			strNCFile(g_pDoc->GetNCFileName());
	Path_Name_From_FullPath(strNCFile, strPath, strFile);
	SendFaseMessage(g_obMakeData.GetSize(), IDS_ANA_DATAFINAL, strFile);
	try {
		CStdioFile	fp(strNCFile,
			CFile::modeCreate | CFile::modeWrite |
			CFile::shareExclusive | CFile::typeText | CFile::osSequentialScan);
		for ( INT_PTR i=0; i<g_obMakeData.GetSize() && IsThread(); i++ ) {
			g_obMakeData[i]->WriteGcode(fp);
			SetProgressPos(i+1);
		}
	}
	catch (	CFileException* e ) {
		strFile.Format(IDS_ERR_DATAWRITE, strNCFile);
		AfxMessageBox(strFile, MB_OK|MB_ICONSTOP);
		e->Delete();
		return FALSE;
	}

	SetProgressPos(g_obMakeData.GetSize());
	return IsThread();
}

//////////////////////////////////////////////////////////////////////
// NC生成ﾒｲﾝｽﾚｯﾄﾞ
//////////////////////////////////////////////////////////////////////

BOOL MakeLathe_MainFunc(void)
{
	// 形状認識処理を用いて図形集合を作成
	if ( !CreateShapeThread() )
		return FALSE;
	// 旋盤加工形状の原点調整他
	InitialShapeData();

	// 内径と外径の切削準備ﾃﾞｰﾀを作成
	float	dHole = GetDbl(MKLA_DBL_HOLE);	// 既存下穴ｻｲｽﾞ
	if ( g_pShape[0] ) {
		if ( !CreateInsidePitch(dHole) )	// dHole更新
			return FALSE;
	}
	if ( g_pShape[1] ) {
		if ( !CreateOutsidePitch() )
			return FALSE;
	}

	// ﾌｪｰｽﾞ1 : 形状のｵﾌｾｯﾄと内外径のｵﾌｾｯﾄの交点を計算し荒加工のﾃﾞｰﾀを生成
	SendFaseMessage(g_obLineTemp[0].GetSize()+g_obLineTemp[1].GetSize());
	for ( int i=0; i<SIZEOF(g_obLineTemp); i++ ) {
		if ( !g_obLineTemp[i].IsEmpty() ) {
			if ( !CreateRoughPass(i) )
				return FALSE;
		}
	}

	// 端面と外径の最大値を取得
	CPointF		ptMax(g_pDoc->GetLatheLine(0)->GetStartMakePoint().x,	// Z軸
					  g_pDoc->GetLatheLine(1)->GetStartMakePoint().y);	// X軸

	// Gｺｰﾄﾞﾍｯﾀﾞ(開始ｺｰﾄﾞ)
	AddCustomLatheCode(GetStr(MKLA_STR_HEADER));

	// 端面処理
	if ( GetFlg(MKLA_FLG_ENDFACE) ) {
		CNCMakeLathe* pNCD = new CNCMakeLathe;
		ASSERT(pNCD);
		pNCD->CreateEndFace(ptMax);
		g_obMakeData.Add(pNCD);
	}

	// 下穴処理
	if ( !GetStr(MKLA_STR_DRILL).IsEmpty() ) {
		CNCMakeLathe* pNCD = new CNCMakeLathe;
		ASSERT(pNCD);
		pNCD->CreatePilotHole();
		g_obMakeData.Add(pNCD);
	}

	// ﾌｪｰｽﾞ2 : NCｺｰﾄﾞの生成
	SendFaseMessage(
		g_obMakeLine[0].GetSize()+GetNum(MKLA_NUM_I_MARGIN)+
		g_obMakeLine[1].GetSize()+GetNum(MKLA_NUM_O_MARGIN)
	);
	if ( g_pShape[0] && dHole>0.0f ) {
		if ( !MakeInsideCode(ptMax) )
			return FALSE;
	}
	if ( g_pShape[1] ) {
		if ( !MakeOutsideCode(ptMax) )
			return FALSE;
	}
	if ( g_pGrooveLayer ) {
		if ( !MakeGrooveCode(ptMax) )
			return FALSE;
	}

	// Gｺｰﾄﾞﾌｯﾀﾞ(終了ｺｰﾄﾞ)
	AddCustomLatheCode(GetStr(MKLA_STR_FOOTER));

	return IsThread();
}

//////////////////////////////////////////////////////////////////////

BOOL CreateShapeThread(void)
{
#ifdef _DEBUG
	printf("CreateShapeThread() for TH_MakeLathe Start\n");
	CDXFchain*	pChainDbg;
	CDXFdata*	pDataDbg;
	CPointF		ptsd, pted;
#endif
	const INT_PTR	nLayerLoop = g_pDoc->GetLayerCnt();
	if ( nLayerLoop > MAXLAYER ) {
		AfxMessageBox(IDS_ERR_LATHE_LAYER, MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}

	NCVCTHREADPARAM	param;
	param.pParent = NULL;
	param.pDoc    = g_pDoc;
	param.wParam  = NULL;
	param.lParam  = NULL;

	// 形状認識処理のｽﾚｯﾄﾞ生成
	CWinThread*	pThread = AfxBeginThread(ShapeSearch_Thread, &param,	// TH_ShapeSearch.cpp
			THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
	if ( !pThread )
		return FALSE;
	pThread->m_bAutoDelete = FALSE;
	pThread->ResumeThread();
	::WaitForSingleObject(pThread->m_hThread, INFINITE);
	delete	pThread;

	// 旋盤生成できる集合が検出されたか？
	INT_PTR		i, j, n, nLoop;
	CLayerData*	pLayer;
	CDXFshape*	pShape;
	CDXFmap*	pMap;
	CString		strInside(INSIDE_S),	// g_szNCcomment[LATHEINSIDE]
				strGroove("GROOVE"),
				strLayer;
	strInside.MakeUpper();

	for ( i=0; i<nLayerLoop && IsThread(); i++ ) {
		pLayer = g_pDoc->GetLayerData(i);
		if ( !pLayer->IsMakeTarget() )
			continue;
		strLayer = pLayer->GetLayerName();
#ifdef _DEBUG
		printf(" Layer=%s\n", LPCTSTR(strLayer));
#endif
		strLayer.MakeUpper();
		if ( strLayer.Find(strGroove) >= 0 ) {
			// 突っ切り溝加工データ
			g_pGrooveLayer = pLayer;
			continue;	// 形状処理しない
		}
		nLoop = pLayer->GetShapeSize();
		// [内|外]径データは図形集合を１つに限定
		if ( nLoop > 1 ) {
			CString	strMsg;
			strMsg.Format(IDS_ERR_LATHE_SHAPE, pLayer->GetLayerName());
			AfxMessageBox(strMsg, MB_OK|MB_ICONEXCLAMATION);
			return FALSE;
		}
		// 内側か外側かの判定
		n = strLayer.Find(strInside) >= 0 ? 0 : 1; 
		//
		for ( j=0; j<nLoop && IsThread(); j++ ) {
			pShape = pLayer->GetShapeData(j);
			pMap   = pShape->GetShapeMap();
			if ( !pMap || !(pShape->GetShapeFlag()&DXFMAPFLG_EDGE) )
				continue;	// この時点でCDXFchainならｱｳﾄ
			// CDXFmap -> CDXFchain 変換
			if ( pShape->ChangeCreate_MapToChain() ) {
#ifdef _DEBUG
				pChainDbg = pShape->GetShapeChain();
				printf("  ShapeNo.%d ChainCnt=%d\n", i, pChainDbg->GetCount());
				PLIST_FOREACH(pDataDbg, pChainDbg)
					ptsd = pDataDbg->GetNativePoint(0);
					pted = pDataDbg->GetNativePoint(1);
					printf("%d (%.3f, %.3f)-(%.3f, %.3f)\n", pDataDbg->GetType(),
						ptsd.x, ptsd.y, pted.x, pted.y);
				END_FOREACH
#endif
				delete	pMap;
				// 生成対象形状を保存
				g_pShape[n] = pShape;
			}
		}
	}

	return TRUE;
}

void InitialShapeData(void)
{
#ifdef _DEBUG
	printf("InitialShapeData() Start\n");
#endif
	CPointF		pts, pte;
	CDXFchain*	pChain;

	for ( int i=0; i<SIZEOF(g_pShape) && IsThread(); i++ ) {
		if ( !g_pShape[i] )
			continue;
		pChain = g_pShape[i]->GetShapeChain();
		ASSERT(pChain);
		pts = pChain->GetHead()->GetNativePoint(0);
		pte = pChain->GetTail()->GetNativePoint(1);
		// Z値(X)が大きい方を先頭ﾒﾝﾊﾞに
		if ( pts.x < pte.x ) {
#ifdef _DEBUG
			printf("(pts.x < pte.x) Object Reverse\n");
#endif
			pChain->Reverse();
			pChain->ReverseNativePt();
		}
		// 順序を正しくしてから原点調整
		pChain->OrgTuning();
	}
}

BOOL CreateInsidePitch(float& dHole)
{
#ifdef _DEBUG
	printf("CreateInsidePitch() Start\n");
//	CPointF		ptsd, pted;
	int			nCntDbg = 0;
#endif
	int			i,
				n = GetNum(MKLA_NUM_I_MARGIN);
	float		d = GetDbl(MKLA_DBL_I_MARGIN),
			cutD  = fabs(GetDbl(MKLA_DBL_I_CUT)),
			dLimit;
	COutlineData*	pOutline;
	CDXFchain*	pChain;
	CDXFdata*	pData;
	VLATHEDRILLINFO	v;

	if ( g_pMakeOpt->GetDrillInfo(v) ) {
		if ( dHole < v.back().d )	// 最後のドリルサイズを採用
			dHole = v.back().d;		// （並べ替えの必要ないと思うけど）
	}
	if ( dHole <= 0.0f ) {
		AfxMessageBox(IDS_ERR_LATHE_INSIDE, MB_OK|MB_ICONEXCLAMATION);
		return TRUE;
	}
	dHole /= 2.0f;

	// --- 形状ｵﾌｾｯﾄ内側の上限値を求める
	// 形状のｵﾌｾｯﾄを生成 + 一番外側のｵﾌｾｯﾄを取得
	g_pShape[0]->CreateScanLine_Lathe(n, -d);
	pOutline = g_pShape[0]->GetLatheList();
	// n==0の対処
	pChain = pOutline->IsEmpty() ?
		g_pShape[0]->GetShapeChain() : pOutline->GetHead();
	CRectF rc = pChain->GetMaxRect();
	dLimit = max(rc.top, rc.bottom);
#ifdef _DEBUG
	printf("Limit(tuning)=%f\n", dLimit - CDXFdata::ms_ptOrg.y);
#endif

	// 所属ﾒﾝﾊﾞの原点調整
	for ( i=0; i<pOutline->GetSize() && IsThread(); i++ ) {
		pChain = pOutline->GetAt(i);
		ASSERT(pChain);
		pChain->OrgTuning();
	}

	// 内径ｵﾌｾｯﾄの生成準備
	CPointF	pts, pte;
	pts.x = rc.right + GetDbl(MKLA_DBL_I_PULLZ);	// 引き代分
	pte.x = rc.left;
	pts.y = pte.y = dHole + cutD + CDXFdata::ms_ptOrg.y;	// Y値は既存下穴ｻｲｽﾞかﾄﾞﾘﾙ径から(Naitive座標)
	DXFLARGV	dxfLine;
	dxfLine.pLayer = NULL;

	// dLimitまでX方向(Y値)を切り込み
	while ( pts.y<dLimit && IsThread() ) {
		dxfLine.s = pts;
		dxfLine.e = pte;
		pData = new CDXFline(&dxfLine);
		g_obLineTemp[0].Add(pData);		// OrgTuning()は不要
		pts.y += cutD;
		pte.y += cutD;
#ifdef _DEBUG
		nCntDbg++;
#endif
	}
#ifdef _DEBUG
	printf("OffsetCnt=%d\n", nCntDbg);
#endif

	return IsThread();
}

BOOL CreateOutsidePitch(void)
{
#ifdef _DEBUG
	printf("CreateOutsidePitch() Start\n");
	int			nCntDbg = 0;
#endif
	int			i,
				n = GetNum(MKLA_NUM_O_MARGIN);
	float		d = GetDbl(MKLA_DBL_O_MARGIN),
			cutD  = fabs(GetDbl(MKLA_DBL_O_CUT)),
			dLimit;
	COutlineData*	pOutline;
	CDXFchain*	pChain;
	CDXFdata*	pData;

	// --- 形状ｵﾌｾｯﾄ外側の下限値を求める
	// 形状のｵﾌｾｯﾄを生成 + 一番外側のｵﾌｾｯﾄを取得
	g_pShape[1]->CreateScanLine_Lathe(n, d);
	pOutline = g_pShape[1]->GetLatheList();
	// n==0の対処
	pChain = pOutline->IsEmpty() ?
		g_pShape[1]->GetShapeChain() : pOutline->GetHead();
	CRectF rc = pChain->GetMaxRect();
	dLimit = min(rc.top, rc.bottom);
#ifdef _DEBUG
	printf("Limit(tuning)=%f\n", dLimit - CDXFdata::ms_ptOrg.y);
#endif

	// 所属ﾒﾝﾊﾞの原点調整
	for ( i=0; i<pOutline->GetSize() && IsThread(); i++ ) {
		pChain = pOutline->GetAt(i);
		ASSERT(pChain);
		pChain->OrgTuning();
	}

	// 外径ｵﾌｾｯﾄの生成準備
	pData = g_pDoc->GetLatheLine(0);	// 外径ｵﾌﾞｼﾞｪｸﾄ
	CPointF	pts(pData->GetNativePoint(0)),
			pte(pData->GetNativePoint(1));
	pts.x += GetDbl(MKLA_DBL_O_PULLZ);	// 引き代分
	pts.y -= cutD;
	pte.y -= cutD;
	DXFLARGV	dxfLine;
	dxfLine.pLayer = NULL;

	// dLimitまでX方向(Y値)を切り込み
	while ( pts.y>dLimit && pte.y>dLimit && IsThread() ) {
		dxfLine.s = pts;
		dxfLine.e = pte;
		pData = new CDXFline(&dxfLine);
		g_obLineTemp[1].Add(pData);		// OrgTuning()は不要
		pts.y -= cutD;
		pte.y -= cutD;
#ifdef _DEBUG
		nCntDbg++;
#endif
	}
#ifdef _DEBUG
	printf("OffsetCnt=%d\n", nCntDbg);
#endif

	return IsThread();
}

BOOL CreateRoughPass(int io)
{
	INT_PTR		i, iPosBase = io==0 ? 0 : g_obLineTemp[1].GetSize(),
				nResult;
	BOOL		bCreate, bInter;
	ENDXFTYPE	enType;
	float		q, qq;
	CPointF		ptChk[4];
	optional<CPointF>	pts;
	CDXFdata*	pData;
	CDXFdata*	pDataChain;
	CDXFdata*	pDataNew;
	CDXFchain*	pChain;
	COutlineData*	pOutline;
	CDXFarc*	pArc;
	DXFLARGV	dxfLine;
	dxfLine.pLayer = NULL;

	// 外径基準線の傾きを計算
	pData = g_obLineTemp[io][0];
	ptChk[0] = pData->GetNativePoint(1) - pData->GetNativePoint(0);
	if ( (qq=ptChk[0].arctan()) < 0.0f )
		qq += PI2;

	// 外径準備ﾃﾞｰﾀをﾙｰﾌﾟさせ荒加工ﾃﾞｰﾀを作成
	for ( i=0; i<g_obLineTemp[io].GetSize() && IsThread(); i++ ) {
		SetProgressPos(i+iPosBase+1);
		pData = g_obLineTemp[io][i];
		pts = pData->GetNativePoint(0);
		pDataChain = NULL;
		bInter = FALSE;		// 一度でも交点があればTRUE
		// 一番外側の形状ｵﾌｾｯﾄと交点ﾁｪｯｸ
		pOutline = g_pShape[io]->GetLatheList();
		pChain = pOutline->IsEmpty() ?
			g_pShape[io]->GetShapeChain() : pOutline->GetHead();
		PLIST_FOREACH(pDataChain, pChain)
			bCreate = FALSE;
			enType = pDataChain->GetType();
			nResult = pData->GetIntersectionPoint(pDataChain, ptChk, FALSE);
			if ( nResult > 1 ) {
				// 交点が２個以上
				if ( ptChk[0].x < ptChk[1].x )
					swap(ptChk[0], ptChk[1]);
				if ( enType==DXFARCDATA || enType==DXFELLIPSEDATA ) {
					pArc = static_cast<CDXFarc*>(pDataChain);
					if ( pArc->GetRound() ) {
						// 反時計回り
						if ( pts ) {
							dxfLine.s = *pts;
							dxfLine.e = ptChk[0];
							bCreate = TRUE;
						}
						pts = ptChk[1];
					}
					else {
						// 時計回り
						dxfLine.s = ptChk[0];
						dxfLine.e = ptChk[1];
						pts.reset();
						bCreate = TRUE;
					}
				}
				else {
					// 円弧以外で交点２個以上はありえないが、
					// ２個目で次の処理へ
					if ( pts ) {
						dxfLine.s = *pts;
						dxfLine.e = ptChk[0];
						bCreate = TRUE;
					}
					pts = ptChk[1];
				}
			}
			else if ( nResult > 0 ) {
				// 交点が１つ
				if ( ptChk[0] == pDataChain->GetNativePoint(0) ) {
					// ｵﾌﾞｼﾞｪｸﾄの始点と等しい場合は、
					// 直前のｵﾌﾞｼﾞｪｸﾄで処理済み
					pts.reset();
				}
				else {
					if ( pts ) {
						if ( ptChk[0] != (*pts) ) {
							// 前回の交点と違う時だけ生成
							dxfLine.s = *pts;
							dxfLine.e = ptChk[0];
							pts.reset();
							bCreate = TRUE;
						}
					}
					else {
						pts = ptChk[0];	// 次の処理へ
					}
				}
			}
			// 荒加工ﾃﾞｰﾀ生成
			if ( bCreate ) {
				pDataNew = new CDXFline(&dxfLine);
				pDataNew->OrgTuning(FALSE);
				g_obMakeLine[io].Add(pDataNew);
				bInter = TRUE;
			}
		END_FOREACH	// End of Chain Loop

		// 交点端数処理
		if ( pts ) {
			if ( !bInter ) {
				// 交点がなければ外径終点まで生成
				dxfLine.s = *pts;
				dxfLine.e = pData->GetNativePoint(1);
				pDataNew = new CDXFline(&dxfLine);
				pDataNew->OrgTuning(FALSE);
				g_obMakeLine[io].Add(pDataNew);
			}
			else if ( pDataChain ) {
				// 最後のｵﾌﾞｼﾞｪｸﾄの傾きをﾁｪｯｸ
				if ( enType==DXFARCDATA || enType==DXFELLIPSEDATA ) {
					pArc = static_cast<CDXFarc*>(pDataChain);
					// 交点を原点に円の中心を90°回転
					ptChk[1] = pArc->GetCenter() - (*pts);
					if ( pArc->GetRound() ) {
						// 反時計回り(-90°)
						ptChk[0].x =  ptChk[1].y;
						ptChk[0].y = -ptChk[1].x;
					}
					else {
						// 時計回り(+90°)
						ptChk[0].x = -ptChk[1].y;
						ptChk[0].y =  ptChk[1].x;
					}
				}
				else {
					// 直線
					ptChk[0] = pDataChain->GetNativePoint(1) -
								pDataChain->GetNativePoint(0);
				}
				if ( (q=ptChk[0].arctan()) < 0.0f )
					q += PI2;
				if ( q >= qq ) {
					// 基準線より傾きが大きいときだけ
					// 端数交点から外径終点まで生成
					dxfLine.s = *pts;
					dxfLine.e = pData->GetNativePoint(1);
					pDataNew = new CDXFline(&dxfLine);
					pDataNew->OrgTuning(FALSE);
					g_obMakeLine[io].Add(pDataNew);
				}
			}
		}
	}	// End of g_obLineTemp

	return IsThread();
}

BOOL MakeInsideCode(const CPointF& ptMax)
{
	INT_PTR		i, nLoop = g_obMakeLine[0].GetSize();
	float		dCutX;
	CPointF		pt, pts, pte, ptPull, ptMov;
	CDXFdata*	pData;
	CDXFchain*	pChain;
	COutlineData*	pOutline;
	CNCMakeLathe*	pNCD;

	// 初期設定
	CString	strCode( CNCMakeLathe::MakeSpindle(GetNum(MKLA_NUM_I_SPINDLE)) );
	if ( !strCode.IsEmpty() )
		AddMakeLatheStr(strCode);
	ptPull.x = GetDbl(MKLA_DBL_I_PULLZ);
	ptPull.y = GetDbl(MKLA_DBL_I_PULLX);

	// ｶｽﾀﾑｺｰﾄﾞ
	if ( !GetStr(MKLA_STR_I_CUSTOM).IsEmpty() )
		AddMakeLatheStr(GetStr(MKLA_STR_I_CUSTOM));
	// NCVCの内径切削指示
	AddMakeLatheStr( '('+CString(INSIDE_S)+')' );

	// 先頭ﾃﾞｰﾀの始点に移動
	if ( nLoop > 0 ) {
		pData = g_obMakeLine[0][0];
		pt = pData->GetStartMakePoint();
		if ( pt.x < CNCMakeLathe::ms_xyz[NCA_Z] ) {
			// 工具初期位置が端面の右側
			// →指定位置まで直線移動
			pNCD = new CNCMakeLathe(0, pt, 0.0f);
		}
		else {
			// 工具初期位置が端面の左側
			// →Z軸移動後、X軸移動
			pNCD = new CNCMakeLathe(ZXMOVE, pt, GetDbl(MKLA_DBL_I_FEED));
		}
		ASSERT(pNCD);
		g_obMakeData.Add(pNCD);
		// 先頭ﾃﾞｰﾀの切削ﾊﾟｽ
		pNCD = new CNCMakeLathe(pData, GetDbl(MKLA_DBL_I_FEED));
		ASSERT(pNCD);
		g_obMakeData.Add(pNCD);
	}

	// 荒加工ﾊﾟｽﾙｰﾌﾟ
	for ( i=1; i<nLoop && IsThread(); i++ ) {
		SetProgressPos(i+1);
		pData = g_obMakeLine[0][i];
		pt = pte = pData->GetStartMakePoint();
		dCutX = pt.y;
		pts.x = CNCMakeLathe::ms_xyz[NCA_Z];
		pts.y = CNCMakeLathe::ms_xyz[NCA_X];
		pts += CDXFdata::ms_ptOrg;		// 現在位置
		pte += CDXFdata::ms_ptOrg;		// 次の移動位置
		pte.y += ptPull.y;
		// pDataの始点が現在位置のどちらにあるかで引き代を変える
		if ( CNCMakeLathe::ms_xyz[NCA_Z]<pt.x && !CheckXZMove(0, pts, pte) ) {
			// 次の始点が右側、かつ、輪郭ｵﾌｾｯﾄに衝突しない → 現在位置から
			pt.y = CNCMakeLathe::ms_xyz[NCA_X];
		}
		pt.y -= ptPull.y;				// 引き代分引いて移動
		pNCD = new CNCMakeLathe(XZMOVE, pt, GetDbl(MKLA_DBL_I_FEEDX));
		ASSERT(pNCD);
		g_obMakeData.Add(pNCD);
		// 始点への切り込み
		if ( ptMax.x < pt.x ) {
			// 端面よりも右側 → 早送りでX軸方向に移動
			pNCD = new CNCMakeLathe(0, NCA_X, dCutX, 0.0f);
		}
		else {
			// 端面よりも左側 → 切削送りでX軸方向に切り込み
			pNCD = new CNCMakeLathe(1, NCA_X, dCutX, GetDbl(MKLA_DBL_I_FEEDX));
		}
		ASSERT(pNCD);
		g_obMakeData.Add(pNCD);
		// 切削ﾊﾟｽ
		pNCD = new CNCMakeLathe(pData, GetDbl(MKLA_DBL_I_FEED));
		ASSERT(pNCD);
		g_obMakeData.Add(pNCD);
	}

	// 仕上げしろﾊﾟｽﾙｰﾌﾟ
	pOutline = g_pShape[0]->GetLatheList();
	for ( i=0; i<pOutline->GetSize() && IsThread(); i++ ) {
		pChain = pOutline->GetAt(i);
		// 移動ﾊﾟｽ
		if ( !pChain->IsEmpty() )
			MoveInsideCode(pChain, ptMax, ptPull);
		// 切削ﾊﾟｽ
		PLIST_FOREACH(pData, pChain)
			pNCD = new CNCMakeLathe(pData, GetDbl(MKLA_DBL_I_FEED));
			ASSERT(pNCD);
			g_obMakeData.Add(pNCD);
		END_FOREACH
	}

	// 形状仕上げ
	pChain = g_pShape[0]->GetShapeChain();
	ASSERT(pChain);
	if ( !pChain->IsEmpty() )
		ptMov = MoveInsideCode(pChain, ptMax, ptPull);
	PLIST_FOREACH(pData, pChain)
		pNCD = new CNCMakeLathe(pData, GetDbl(MKLA_DBL_I_FEED));
		ASSERT(pNCD);
		g_obMakeData.Add(pNCD);
	END_FOREACH

	// 工具初期位置へ復帰
	if ( !pChain->IsEmpty() ) {
		// X軸(Y)方向の離脱とZ軸(X)方向の移動
		pNCD = new CNCMakeLathe(XZMOVE, ptMov, GetDbl(MKLA_DBL_I_FEEDX));
		ASSERT(pNCD);
		g_obMakeData.Add(pNCD);
		pNCD = new CNCMakeLathe(0, g_pDoc->GetCircleObject()->GetStartMakePoint(), 0.0f);
		ASSERT(pNCD);
		g_obMakeData.Add(pNCD);
	}

	// 内径切削終了ｺﾒﾝﾄ
	AddMakeLatheStr( '('+CString(ENDINSIDE_S)+')' );

	return IsThread();
}

BOOL MakeOutsideCode(const CPointF& ptMax)
{
	INT_PTR		i, nLoop = g_obMakeLine[1].GetSize(),
				iPosBase = g_obMakeLine[0].GetSize();
	float		dCutX;
	CPointF		pt, pts, pte, ptPull;
	CDXFdata*	pData;
	CDXFchain*	pChain;
	COutlineData*	pOutline;
	CNCMakeLathe*	pNCD;

	// 初期設定
	CString	strCode( CNCMakeLathe::MakeSpindle(GetNum(MKLA_NUM_O_SPINDLE)) );
	if ( !strCode.IsEmpty() )
		AddMakeLatheStr(strCode);
	ptPull.x = GetDbl(MKLA_DBL_O_PULLZ);
	ptPull.y = GetDbl(MKLA_DBL_O_PULLX);

	// ｶｽﾀﾑｺｰﾄﾞ
	if ( !GetStr(MKLA_STR_O_CUSTOM).IsEmpty() )
		AddMakeLatheStr(GetStr(MKLA_STR_O_CUSTOM));

	// 先頭ﾃﾞｰﾀの始点に移動
	if ( nLoop > 0 ) {
		if ( CNCMakeLathe::ms_xyz[NCA_X] < ptMax.y ) {
			// 端面の下側
			if ( CNCMakeLathe::ms_xyz[NCA_Z] < ptMax.x ) {
				// 端面の左側(下穴か内径後を想定)
				pNCD = new CNCMakeLathe(0, NCA_Z, ptMax.x+GetDbl(MKLA_DBL_O_PULLZ), 0.0f);	// Z軸退避
				ASSERT(pNCD);
				g_obMakeData.Add(pNCD);
			}
			pNCD = new CNCMakeLathe(0, NCA_X, ptMax.y+GetDbl(MKLA_DBL_O_PULLX), 0.0f);		// X軸退避
			ASSERT(pNCD);
			g_obMakeData.Add(pNCD);
		}
		pData = g_obMakeLine[1][0];
		pts = pData->GetStartMakePoint();
		pNCD = new CNCMakeLathe(0, NCA_Z, pts.x, 0.0f);
		ASSERT(pNCD);
		g_obMakeData.Add(pNCD);
		if ( CNCMakeLathe::ms_xyz[NCA_X] > ptMax.y ) {
			pNCD = new CNCMakeLathe(0, NCA_X, ptMax.y+GetDbl(MKLA_DBL_O_PULLX), 0.0f);
			ASSERT(pNCD);
			g_obMakeData.Add(pNCD);
		}
		pNCD = new CNCMakeLathe(1, NCA_X, pts.y, GetDbl(MKLA_DBL_O_FEEDX));
		ASSERT(pNCD);
		g_obMakeData.Add(pNCD);
		// 先頭ﾃﾞｰﾀの切削ﾊﾟｽ
		pNCD = new CNCMakeLathe(pData, GetDbl(MKLA_DBL_O_FEED));
		ASSERT(pNCD);
		g_obMakeData.Add(pNCD);
	}

	// 荒加工ﾊﾟｽﾙｰﾌﾟ
	for ( i=1; i<nLoop && IsThread(); i++ ) {
		SetProgressPos(i+iPosBase+1);
		pData = g_obMakeLine[1][i];
		pt = pte = pData->GetStartMakePoint();
		dCutX = pt.y;
		pts.x = CNCMakeLathe::ms_xyz[NCA_Z];
		pts.y = CNCMakeLathe::ms_xyz[NCA_X];
		pts += CDXFdata::ms_ptOrg;		// 現在位置
		pte += CDXFdata::ms_ptOrg;		// 次の移動位置
		pte.y += ptPull.y;
		// pDataの始点が現在位置のどちらにあるかで引き代を変える
		if ( CNCMakeLathe::ms_xyz[NCA_Z]<pt.x && !CheckXZMove(1, pts, pte) ) {
			// 次の始点が右側、かつ、輪郭ｵﾌｾｯﾄに衝突しない → 現在位置から
			pt.y = CNCMakeLathe::ms_xyz[NCA_X];
		}
		else {
			// 次の始点が左側 → 外径から
			pt.y = ptMax.y; 
		}
		pt.y += ptPull.y;				// 引き代分引いて移動
		pNCD = new CNCMakeLathe(XZMOVE, pt, GetDbl(MKLA_DBL_O_FEEDX));
		ASSERT(pNCD);
		g_obMakeData.Add(pNCD);
		// 始点への切り込み
		if ( ptMax.x < pt.x ) {
			// 端面よりも右側 → 早送りでX軸方向に移動
			pNCD = new CNCMakeLathe(0, NCA_X, dCutX, 0.0f);
		}
		else {
			// 端面よりも左側 → 切削送りでX軸方向に切り込み
			pNCD = new CNCMakeLathe(1, NCA_X, dCutX, GetDbl(MKLA_DBL_O_FEEDX));
		}
		ASSERT(pNCD);
		g_obMakeData.Add(pNCD);
		// 切削ﾊﾟｽ
		pNCD = new CNCMakeLathe(pData, GetDbl(MKLA_DBL_O_FEED));
		ASSERT(pNCD);
		g_obMakeData.Add(pNCD);
	}

	// 仕上げしろﾊﾟｽﾙｰﾌﾟ
	pOutline = g_pShape[1]->GetLatheList();
	for ( i=0; i<pOutline->GetSize() && IsThread(); i++ ) {
		pChain = pOutline->GetAt(i);
		// 移動ﾊﾟｽ
		if ( !pChain->IsEmpty() )
			MoveOutsideCode(pChain->GetHead(), ptMax, ptPull);
		// 切削ﾊﾟｽ
		PLIST_FOREACH(pData, pChain)
			pNCD = new CNCMakeLathe(pData, GetDbl(MKLA_DBL_O_FEED));
			ASSERT(pNCD);
			g_obMakeData.Add(pNCD);
		END_FOREACH
	}

	// 形状仕上げ
	pChain = g_pShape[1]->GetShapeChain();
	ASSERT(pChain);
	if ( !pChain->IsEmpty() )
		MoveOutsideCode(pChain->GetHead(), ptMax, ptPull);
	PLIST_FOREACH(pData, pChain)
		pNCD = new CNCMakeLathe(pData, GetDbl(MKLA_DBL_O_FEED));
		ASSERT(pNCD);
		g_obMakeData.Add(pNCD);
	END_FOREACH

	// 工具初期位置へ復帰
	if ( !pChain->IsEmpty() ) {
		dCutX = ptMax.y + ptPull.y;
		pt = g_pDoc->GetCircleObject()->GetStartMakePoint();
		pNCD = new CNCMakeLathe(1, NCA_X, dCutX, GetDbl(MKLA_DBL_O_FEEDX));
		ASSERT(pNCD);
		g_obMakeData.Add(pNCD);
		if ( dCutX < pt.y ) {
			pNCD = new CNCMakeLathe(0, NCA_X, pt.y, 0.0f);
			ASSERT(pNCD);
			g_obMakeData.Add(pNCD);
		}
		pNCD = new CNCMakeLathe(0, NCA_Z, pt.x, 0.0f);
		ASSERT(pNCD);
		g_obMakeData.Add(pNCD);
		if ( pt.y < dCutX ) {
			pNCD = new CNCMakeLathe(0, NCA_X, pt.y, 0.0f);
			ASSERT(pNCD);
			g_obMakeData.Add(pNCD);
		}
	}

	return IsThread();
}

BOOL MakeGrooveCode(const CPointF& ptMax)
{
	INT_PTR		i;
	CDXFsort	arDXFdata;
	CDXFdata*	pData;
	CNCMakeLathe*	pNCD;
	CPointF		pt, pts, pte;
	float		dPullX = ptMax.y + GetDbl(MKLA_DBL_G_PULLX),
				dWidth = GetDbl(MKLA_DBL_GROOVEWIDTH);

	// データ準備
	for ( i=0; i<g_pGrooveLayer->GetDxfSize(); i++ ) {
		pData = g_pGrooveLayer->GetDxfData(i);
		if ( pData->GetType() == DXFLINEDATA ) {
			pData->OrgTuning(FALSE);
			arDXFdata.Add(pData);
		}
	}
	if ( arDXFdata.IsEmpty() )
		return IsThread();
	arDXFdata.Sort(GrooveSortFunc);	// Z軸(X値)の大きい順に並べ替え

	// 初期設定
	CString	strCode( CNCMakeLathe::MakeSpindle(GetNum(MKLA_NUM_G_SPINDLE)) );
	if ( !strCode.IsEmpty() )
		AddMakeLatheStr(strCode);

	// ｶｽﾀﾑｺｰﾄﾞ
	if ( !GetStr(MKLA_STR_G_CUSTOM).IsEmpty() )
		AddMakeLatheStr(GetStr(MKLA_STR_G_CUSTOM));
	// NCVCの突っ切り切削指示
	CString	strTool;
	strTool.Format(IDS_MAKENCD_FORMAT, GetDbl(MKLA_DBL_GROOVEWIDTH));
	switch ( GetNum(MKLA_NUM_GROOVETOOL) ) {
	case 1:		// 工具基準点中央
		strTool = 'C'+strTool;
		break;
	case 2:		// 右
		strTool = 'R'+strTool;
		break;
	}
	AddMakeLatheStr( '('+CString(GROOVE_S)+'='+strTool+')' );

	// 突っ切りデータ生成
	for ( i=0; i<arDXFdata.GetSize(); i++ ) {
		pData = arDXFdata[i];
		pts = pData->GetStartMakePoint();
		pte = pData->GetEndMakePoint();
		if ( pts.x < pte.x )
			swap(pts, pte);
		// X軸の長さによって生成方法を変える
		if ( dWidth < pts.x-pte.x ) {
			// Z軸スライド
			switch ( GetNum(MKLA_NUM_GROOVETOOL) ) {
			case 1:
				pts.x -= dWidth / 2.0f;
				pte.x += dWidth / 2.0f;
				break;
			case 2:
				pte.x += dWidth;
				break;
			default:
				pts.x -= dWidth;
			}
			pNCD = new CNCMakeLathe;
			ASSERT(pNCD);
			pNCD->CreateGroove(pts, pte, dPullX);
			g_obMakeData.Add(pNCD);
		}
		else {
			// X軸のみ(溝)
			switch ( GetNum(MKLA_NUM_GROOVETOOL) ) {
			case 1:
				pt.x = (pts.x - pte.x) / 2.0f + pte.x;
				pt.y = (pts.y - pte.y) / 2.0f + pte.y;
				break;
			case 2:
				pt = pts;
				break;
			default:
				pt = pte;
			}
			pNCD = new CNCMakeLathe;
			ASSERT(pNCD);
			pNCD->CreateGroove(pt, dPullX);
			g_obMakeData.Add(pNCD);
		}
	}

	// 工具初期位置へ復帰
	pt = g_pDoc->GetCircleObject()->GetStartMakePoint();
	pNCD = new CNCMakeLathe(0, NCA_X, pt.y, 0.0f);
	ASSERT(pNCD);
	g_obMakeData.Add(pNCD);
	pNCD = new CNCMakeLathe(0, NCA_Z, pt.x, 0.0f);
	ASSERT(pNCD);
	g_obMakeData.Add(pNCD);

	// 突っ切り切削終了ｺﾒﾝﾄ
	AddMakeLatheStr( '('+CString(ENDGROOVE_S)+')' );

	return IsThread();
}

BOOL CheckXZMove(int io, const CPointF& pts, const CPointF& pte)
{
	INT_PTR		i, nLoop;
	BOOL		bResult = FALSE;
	CPointF		pt[4];
	COutlineData*	pOutline;
	CDXFchain*		pChain;
	CDXFdata*		pData;

	// 輪郭ｵﾌｾｯﾄとの交点をﾁｪｯｸ
	if ( g_pShape[io] ) {
		pOutline = g_pShape[io]->GetLatheList();
		nLoop = pOutline->IsEmpty() ? 1 : pOutline->GetSize();
		for ( i=0; i<nLoop && !bResult && IsThread(); i++ ) {
			pChain = pOutline->IsEmpty() ? g_pShape[io]->GetShapeChain() : pOutline->GetAt(i);
			PLIST_FOREACH(pData, pChain)
				if ( pData->GetIntersectionPoint(pts, pte, pt, FALSE) > 0 ) {
					bResult = TRUE;
					break;
				}
			END_FOREACH
		}
	}

	return bResult;
}

CPointF MoveInsideCode(const CDXFchain* pChain, const CPointF& ptMax, const CPointF& ptPull)
{
	CNCMakeLathe*	pNCD;

	// 形状の一番下側＋引き代の座標計算
	CRect3F	rc( pChain->GetMaxRect() );
	CPointF	pt1(rc.TopLeft()), pt2(rc.BottomRight()), pt;
	pt1 -= CDXFdata::ms_ptOrg;
	pt2 -= CDXFdata::ms_ptOrg;
	pt.x = ptMax.x + ptPull.x;
	pt.y = min(pt1.y, pt2.y) - ptPull.y;

	// X軸(Y)方向の離脱とZ軸(X)方向の移動
	pNCD = new CNCMakeLathe(XZMOVE, pt, GetDbl(MKLA_DBL_I_FEEDX));
	ASSERT(pNCD);
	g_obMakeData.Add(pNCD);

	// 先頭データのX軸に移動
	CDXFdata*	pData = pChain->GetHead();
	CPointF	pts(pData->GetStartMakePoint());
	pNCD = new CNCMakeLathe(1, NCA_X, pts.y, 0.0f);
	ASSERT(pNCD);
	g_obMakeData.Add(pNCD);

	return pt;
}

void MoveOutsideCode(const CDXFdata* pData, const CPointF& ptMax, const CPointF& ptPull)
{
	CNCMakeLathe*	pNCD;
	CPointF		pts(pData->GetStartMakePoint()),
				pt(pts);

	// X軸(Y)方向の離脱とZ軸(X)方向の移動
	if ( CNCMakeLathe::ms_xyz[NCA_Z] <= pt.x )
		pt.x = ptMax.x + ptPull.x;		// 端面＋引き代
	pt.y = ptMax.y + ptPull.y;	// Z軸はｵﾌﾞｼﾞｪｸﾄの開始位置, X軸は外径
	pNCD = new CNCMakeLathe(XZMOVE, pt, GetDbl(MKLA_DBL_O_FEEDX));
	ASSERT(pNCD);
	g_obMakeData.Add(pNCD);

	// X軸始点に移動
	pNCD = new CNCMakeLathe(ptMax.x < pt.x ? 0 : 1, NCA_X, pts.y, GetDbl(MKLA_DBL_O_FEEDX));
	ASSERT(pNCD);
	g_obMakeData.Add(pNCD);
	if ( ptMax.x < pt.x ) {
		// ｵﾌﾞｼﾞｪｸﾄの始点に移動
		pNCD = new CNCMakeLathe(1, NCA_Z, pts.x, GetDbl(MKLA_DBL_O_FEED));
		ASSERT(pNCD);
		g_obMakeData.Add(pNCD);
	}
}

//////////////////////////////////////////////////////////////////////

// ﾌｪｰｽﾞ出力
void SendFaseMessage
	(INT_PTR nRange/*=-1*/, int nMsgID/*=-1*/, LPCTSTR lpszMsg/*=NULL*/)
{
#ifdef _DEBUG
	printf("MakeLathe_Thread() Phase%d Start\n", g_nFase);
#endif
	if ( nRange > 0 )
		g_pParent->m_ctReadProgress.SetRange32(0, (int)nRange);

	CString	strMsg;
	if ( nMsgID > 0 )
		VERIFY(strMsg.LoadString(nMsgID));
	else
		strMsg.Format(IDS_MAKENCD_FASE, g_nFase);
	g_pParent->SetFaseMessage(strMsg, lpszMsg);
	g_nFase++;
}

//	AddCustomLatheCode() から呼び出し
class CMakeCustomCode_Lathe : public CMakeCustomCode	// MakeCustomCode.h
{
public:
	CMakeCustomCode_Lathe(void) :
				CMakeCustomCode(g_pDoc, NULL, g_pMakeOpt) {
		static	LPCTSTR	szCustomCode[] = {
			"ProgNo", 
			"G90orG91", "SPINDLE",
			"LatheDiameter", "LatheZmax", "LatheZmin",
			"ToolPosX", "ToolPosZ"
		};
		// ｵｰﾀﾞｰ追加
		m_strOrderIndex.AddElement(SIZEOF(szCustomCode), szCustomCode);
	}

	CString	ReplaceCustomCode(const string& str) {
		int		nTestCode;
		CString	strResult;

		// 基底ｸﾗｽ呼び出し
		tie(nTestCode, strResult) = CMakeCustomCode::ReplaceCustomCode(str);
		if ( !strResult.IsEmpty() )
			return strResult;

		// 派生replace
		switch ( nTestCode ) {
		case 0:		// ProgNo
			if ( GetFlg(MKLA_FLG_PROG) )
				strResult.Format(IDS_MAKENCD_PROG,
					GetFlg(MKLA_FLG_PROGAUTO) ? ::GetRandom(1,9999) : GetNum(MKLA_NUM_PROG));
			break;
		case 1:		// G90orG91
			strResult = CNCMakeBase::MakeCustomString(GetNum(MKLA_NUM_G90)+90);
			break;
		case 2:		// SPINDLE
			// -- 初期工程の回転数で生成
			if ( GetFlg(MKLA_FLG_ENDFACE) ) {
				// 端面回転数
				strResult = CNCMakeLathe::MakeSpindle(GetNum(MKLA_NUM_E_SPINDLE));
			}
			else if ( !GetStr(MKLA_STR_DRILL).IsEmpty() ) {
				// 下穴回転数
				VLATHEDRILLINFO	v;
				if ( g_pMakeOpt->GetDrillInfo(v) )
					strResult = CNCMakeLathe::MakeSpindle(v[0].s);
			}
			else if ( g_pShape[0] ) {
				// 内径回転数
				strResult = CNCMakeLathe::MakeSpindle(GetNum(MKLA_NUM_I_SPINDLE));
			}
			else if ( g_pShape[1] ) {	// 最初のﾍｯﾀﾞｰﾙｰﾌﾟなので else if でOK
				// 外径回転数
				strResult = CNCMakeLathe::MakeSpindle(GetNum(MKLA_NUM_O_SPINDLE));
			}
			else if ( g_pGrooveLayer ) {
				// 突切回転数
				strResult = CNCMakeLathe::MakeSpindle(GetNum(MKLA_NUM_G_SPINDLE));
			}
			break;
		case 3:		// LatheDiameter
			strResult.Format(IDS_MAKENCD_FORMAT, g_pDoc->GetLatheLine(1)->GetStartMakePoint().y * 2.0);
			break;
		case 4:		// LatheZmax
			strResult.Format(IDS_MAKENCD_FORMAT, g_pDoc->GetLatheLine(0)->GetStartMakePoint().x);
			break;
		case 5:		// LatheZmin
			strResult.Format(IDS_MAKENCD_FORMAT, g_pDoc->GetLatheLine(0)->GetEndMakePoint().x);
			break;
		case 6:		// ToolPosX
			strResult.Format(IDS_MAKENCD_FORMAT, g_pDoc->GetCircleObject()->GetStartMakePoint().y * 2.0);
			break;
		case 7:		// ToolPosZ
			strResult.Format(IDS_MAKENCD_FORMAT, g_pDoc->GetCircleObject()->GetStartMakePoint().x);
			break;
		default:
			strResult = str.c_str();
		}

		return strResult;
	}
};

void AddCustomLatheCode(const CString& strFileName)
{
	CString	strBuf, strResult;
	CMakeCustomCode_Lathe	custom;
	string	str, strTok;
	tokenizer<custom_separator>	tokens(str);

	try {
		CStdioFile	fp(strFileName,
			CFile::modeRead | CFile::shareDenyWrite | CFile::typeText | CFile::osSequentialScan);
		while ( fp.ReadString(strBuf) && IsThread() ) {
			str = strBuf;
			tokens.assign(str);
			strResult.Empty();
			BOOST_FOREACH(strTok, tokens) {
				strResult += custom.ReplaceCustomCode(strTok);
			}
			if ( !strResult.IsEmpty() )
				AddMakeLatheStr(strResult);
		}
	}
	catch (CFileException* e) {
		AfxMessageBox(IDS_ERR_MAKECUSTOM, MB_OK|MB_ICONEXCLAMATION);
		e->Delete();
		// このｴﾗｰは正常ﾘﾀｰﾝ(警告のみ)
	}
}

int ShapeSortFunc(CDXFshape* pFirst, CDXFshape* pSecond)
{
	int		nResult;
	CPointF	pt1( pFirst->GetShapeChain()->GetHead()->GetNativePoint(0) ),
			pt2( pSecond->GetShapeChain()->GetHead()->GetNativePoint(0) );
	float	x = RoundUp(pt1.x - pt2.x);
	if ( x == 0.0f )
		nResult = 0;
	else if ( x > 0.0f )
		nResult = -1;
	else
		nResult = 1;
	return nResult;
}

int GrooveSortFunc(CDXFdata* pFirst, CDXFdata* pSecond)
{
	int		nResult;
	CPointF	pt1s( pFirst->GetStartMakePoint() ),  pt1e( pFirst->GetEndMakePoint() ),
			pt2s( pSecond->GetStartMakePoint() ), pt2e( pSecond->GetEndMakePoint() );
	float	sx = pt1s.x > pt1e.x ? pt1s.x : pt1e.x,
			ex = pt2s.x > pt2e.x ? pt2s.x : pt2e.x;
	if ( sx == ex )
		nResult = 0;
	else if ( sx > ex )
		nResult = -1;
	else
		nResult = 1;
	return nResult;
}

//////////////////////////////////////////////////////////////////////

// NC生成のｸﾞﾛｰﾊﾞﾙ変数初期化(後始末)ｽﾚｯﾄﾞ
UINT MakeLathe_AfterThread(LPVOID)
{
	g_csMakeAfter.Lock();

	int			i, j;
#ifdef _DEBUG
	printf("MakeLathe_AfterThread() Start\n");
#endif

	for ( j=0; j<SIZEOF(g_obLineTemp); j++ ) {
		for ( i=0; i<g_obLineTemp[j].GetSize(); i++ )
			delete	g_obLineTemp[j][i];
		g_obLineTemp[j].RemoveAll();
	}
	for ( j=0; j<SIZEOF(g_obMakeLine); j++ ) {
		for ( i=0; i<g_obMakeLine[j].GetSize(); i++ )
			delete	g_obMakeLine[j][i];
		g_obMakeLine[j].RemoveAll();
	}
	for ( i=0; i<g_obMakeData.GetSize(); i++ )
		delete	g_obMakeData[i];
	g_obMakeData.RemoveAll();
	for ( i=0; i<g_pDoc->GetLayerCnt(); i++ )
		g_pDoc->GetLayerData(i)->RemoveAllShape();

	g_csMakeAfter.Unlock();

	return 0;
}
