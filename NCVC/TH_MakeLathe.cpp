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
#include "NCMakeLatheOpt.h"
#include "NCMakeLathe.h"
#include "MakeCustomCode.h"
#include "ThreadDlg.h"

using std::string;
using namespace boost;

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
//#define	_DBG_NCMAKE_TIME	//	生成時間の表示
#endif

// ｸﾞﾛｰﾊﾞﾙ変数定義
static	CThreadDlg*			g_pParent;
static	CDXFDoc*			g_pDoc;
static	CNCMakeLatheOpt*	g_pMakeOpt;

// よく使う変数や呼び出しの簡略置換
#define	IsThread()	g_pParent->IsThreadContinue()
#define	GetFlg(a)	g_pMakeOpt->GetFlag(a)
#define	GetNum(a)	g_pMakeOpt->GetNum(a)
#define	GetDbl(a)	g_pMakeOpt->GetDbl(a)
#define	GetStr(a)	g_pMakeOpt->GetStr(a)
#define	SetProgressPos(a)	g_pParent->m_ctReadProgress.SetPos(a)

// NC生成に必要なﾃﾞｰﾀ群
static	CShapeArray	g_obShape;
static	CDXFarray	g_obOutsideTemp;
static	CDXFarray	g_obLathePass;
static	CTypedPtrArrayEx<CPtrArray, CNCMakeLathe*>	g_obMakeData;	// 加工ﾃﾞｰﾀ

// ｻﾌﾞ関数
static	void	InitialVariable(void);		// 変数初期化
static	void	SetStaticOption(void);		// 静的変数の初期化
static	BOOL	MakeLathe_MainFunc(void);	// NC生成のﾒｲﾝﾙｰﾌﾟ
static	BOOL	CreateShapeThread(void);	// 形状認識処理
static	void	InitialShapeData(void);		// 形状認識の初期化
static	BOOL	CreateOutsidePitch(void);	// 外径ｵﾌｾｯﾄを中心まで生成
static	BOOL	CreateRoughPass(void);		// 荒加工ﾃﾞｰﾀの生成
static	BOOL	MakeLatheCode(void);		// NCｺｰﾄﾞの生成
static	BOOL	CheckXZMove(const CPointD&, const CPointD&);
static	void	MoveLatheCode(const CDXFdata*, double, double);
static	BOOL	OutputLatheCode(void);		// NCｺｰﾄﾞの出力

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
static	void	SendFaseMessage(int = -1, int = -1, LPCTSTR = NULL);

// ｻﾌﾞｽﾚｯﾄﾞ関数
static	CCriticalSection	g_csMakeAfter;	// MakeLathe_AfterThread()ｽﾚｯﾄﾞﾛｯｸｵﾌﾞｼﾞｪｸﾄ
static	UINT	MakeLathe_AfterThread(LPVOID);	// 後始末ｽﾚｯﾄﾞ

//////////////////////////////////////////////////////////////////////
// 旋盤用NC生成ｽﾚｯﾄﾞ
//////////////////////////////////////////////////////////////////////

UINT MakeLathe_Thread(LPVOID pVoid)
{
#ifdef _DEBUG
	CMagaDbg	dbg("MakeLathe_Thread()\nStart", DBG_GREEN);
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
		dbg.printf("MakeLathe_Thread All Over!!!");
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
	for ( int i=0; i<g_obShape.GetSize(); i++ )
		g_obShape[i]->CrearScanLine_Lathe();
	g_obShape.RemoveAll();

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
	// 端面の終点が原点
	CDXFdata::ms_ptOrg = g_pDoc->GetLatheLine(1)->GetNativePoint(1);
	// ORIGINﾃﾞｰﾀが工具初期位置
	CDXFdata::ms_pData = g_pDoc->GetCircleObject();
	// 各ｵﾌﾞｼﾞｪｸﾄの原点調整
	g_pDoc->GetCircleObject()->OrgTuning(FALSE);
	g_pDoc->GetLatheLine(0)->OrgTuning(FALSE);
	g_pDoc->GetLatheLine(1)->OrgTuning(FALSE);
	// 生成ｵﾌﾟｼｮﾝの初期化
	CNCMakeLathe::InitialVariable();
}

void SetStaticOption(void)
{
	// CDXFdataの静的変数初期化
	CDXFdata::ms_fXRev = CDXFdata::ms_fYRev = FALSE;

	// CNCMakeLatheの静的変数初期化
	CPointD	ptOrg(g_pDoc->GetCircleObject()->GetStartMakePoint());
	CNCMakeLathe::ms_xyz[NCA_X] = ptOrg.y;
	CNCMakeLathe::ms_xyz[NCA_Y] = 0.0;
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
		for ( int i=0; i<g_obMakeData.GetSize() && IsThread(); i++ ) {
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
	// 外径ﾃﾞｰﾀから切削準備ﾃﾞｰﾀを作成
	if ( !CreateOutsidePitch() )
		return FALSE;

	// ﾌｪｰｽﾞ1 : 形状のｵﾌｾｯﾄと外径のｵﾌｾｯﾄの交点を計算し荒加工のﾃﾞｰﾀを生成
	SendFaseMessage(g_obOutsideTemp.GetSize());
	if ( !CreateRoughPass() )
		return FALSE;

	// ﾌｪｰｽﾞ2 : NCｺｰﾄﾞの生成
	SendFaseMessage(g_obLathePass.GetSize()+GetNum(MKLA_NUM_MARGIN));
	if ( !MakeLatheCode() )
		return FALSE;

	return IsThread();
}

//////////////////////////////////////////////////////////////////////

BOOL CreateShapeThread(void)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CreateShapeThread() for TH_MakeLathe\nStart");
	CDXFchain*	pChainDbg;
	CDXFdata*	pDataDbg;
	CPointD		ptsd, pted;
#endif
	NCVCTHREADPARAM	param;
	param.pParent = NULL;
	param.pDoc    = g_pDoc;
	param.wParam  = NULL;
	param.lParam  = NULL;

	// 形状認識処理のｽﾚｯﾄﾞ生成
	CWinThread*	pThread = AfxBeginThread(ShapeSearch_Thread, &param,
			THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
	if ( !pThread )
		return FALSE;
	pThread->m_bAutoDelete = FALSE;
	pThread->ResumeThread();
	::WaitForSingleObject(pThread->m_hThread, INFINITE);
	delete	pThread;

	// 旋盤生成できる集合が検出されたか？
	int		i, j, nLoop;
	const int	nLayerLoop = g_pDoc->GetLayerCnt();
	CLayerData*	pLayer;
	CDXFshape*	pShape;
	CDXFmap*	pMap;

	for ( i=0; i<nLayerLoop && IsThread(); i++ ) {
		pLayer = g_pDoc->GetLayerData(i);
		if ( !pLayer->IsCutType() )
			continue;
#ifdef _DEBUG
		dbg.printf(" Layer=%s", pLayer->GetLayerName());
#endif
		nLoop = pLayer->GetShapeSize();
		for ( j=0; j<nLoop && IsThread(); j++ ) {
			pShape = pLayer->GetShapeData(j);
			pMap   = pShape->GetShapeMap();
			if ( !pMap || !(pShape->GetShapeFlag()&DXFMAPFLG_EDGE) )
				continue;	// この時点でCDXFchainならｱｳﾄ
			// CDXFmap -> CDXFchain 変換
			if ( pShape->ChangeCreate_MapToChain() ) {
#ifdef _DEBUG
				pChainDbg = pShape->GetShapeChain();
				dbg.printf("  ShapeNo.%d ChainCnt=%d", i, pChainDbg->GetCount());
				PLIST_FOREACH(pDataDbg, pChainDbg)
					ptsd = pDataDbg->GetNativePoint(0);
					pted = pDataDbg->GetNativePoint(1);
					dbg.printf("%d (%.3f, %.3f)-(%.3f, %.3f)", pDataDbg->GetType(),
						ptsd.x, ptsd.y, pted.x, pted.y);
				END_FOREACH
#endif
				delete	pMap;
				// 生成対象形状を保存
				g_obShape.Add(pShape);
			}
		}
	}

	return !g_obShape.IsEmpty();
}

void InitialShapeData(void)
{
#ifdef _DEBUG
	CMagaDbg	dbg("InitialShapeData()\nStart");
#endif
	CPointD		pts, pte;
	CDXFchain*	pChain;

	for ( int i=0; i<g_obShape.GetSize() && IsThread(); i++ ) {
		pChain = g_obShape[i]->GetShapeChain();
		ASSERT(pChain);
		pts = pChain->GetHead()->GetNativePoint(0);
		pte = pChain->GetTail()->GetNativePoint(1);
		// Z値(X)が大きい方を先頭ﾒﾝﾊﾞに
		if ( pts.x < pte.x ) {
#ifdef _DEBUG
			dbg.printf("(pts.x < pte.x) Object Reverse");
#endif
			pChain->Reverse();
			pChain->ReverseNativePt();
		}
		// 順序を正しくしてから原点調整
		pChain->OrgTuning();
	}
}

BOOL CreateOutsidePitch(void)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CreateOutsidePitch()\nStart");
	CPointD		ptsd, pted;
	int			nCntDbg = 0;
#endif
	int			i, j,
				n = GetNum(MKLA_NUM_MARGIN);
	double		d = GetDbl(MKLA_DBL_MARGIN);
	COutlineData*	pOutline;
	CDXFchain*	pChain;
	CDXFdata*	pData;
	CRectD		rc;
	double		dLimit = DBL_MAX,
				cutD  = fabs(GetDbl(MKLA_DBL_CUT));

	// 形状ｵﾌｾｯﾄ一番外側の下限値を求める
	for ( i=0; i<g_obShape.GetSize() && IsThread(); i++ ) {
		// 形状のｵﾌｾｯﾄを生成 + 一番外側のｵﾌｾｯﾄを取得
		g_obShape[i]->CreateScanLine_Lathe(n, d);
		pOutline = g_obShape[i]->GetLatheList();
		// n==0の対処
		pChain = pOutline->IsEmpty() ?
			g_obShape[i]->GetShapeChain() : pOutline->GetHead();
		PLIST_FOREACH(pData, pChain)
			rc = pData->GetMaxRect();
#ifdef _DEBUG
			ptsd = pData->GetNativePoint(0);
			pted = pData->GetNativePoint(1);
			dbg.printf("%d (%.3f, %.3f)-(%.3f, %.3f)", pData->GetType(),
				ptsd.x, ptsd.y, pted.x, pted.y);
			dbg.printf("rc=(%.3f, %.3f)-(%.3f, %.3f)",
				rc.left, rc.top, rc.right, rc.bottom);
#endif
			if ( dLimit > rc.top )
				dLimit = rc.top;
		END_FOREACH
		// 所属ﾒﾝﾊﾞの原点調整
		for ( j=0; j<pOutline->GetSize() && IsThread(); j++ ) {
			pChain = pOutline->GetAt(j);
			ASSERT(pChain);
			pChain->OrgTuning();
		}
	}
#ifdef _DEBUG
	dbg.printf("Limit(tuning)=%f", dLimit - CDXFdata::ms_ptOrg.y);
#endif

	// 外径ｵﾌｾｯﾄを中心まで生成
	CDXFline*	pLine = g_pDoc->GetLatheLine(0);	// 外径ｵﾌﾞｼﾞｪｸﾄ
	CPointD	pts(pLine->GetNativePoint(0)),
			pte(pLine->GetNativePoint(1));
	pts.x += GetDbl(MKLA_DBL_PULL_Z);		// 引き代分
	pts.y -= cutD;
	pte.y -= cutD;
	DXFLARGV	dxfLine;
	dxfLine.pLayer = NULL;

	// 原点(zero)までX方向(Y値)を切り込み
	while ( pts.y>dLimit && pte.y>dLimit && IsThread() ) {
		dxfLine.s = pts;
		dxfLine.e = pte;
		pLine = new CDXFline(&dxfLine);
		g_obOutsideTemp.Add(pLine);		// OrgTuning()は不要
		pts.y -= cutD;
		pte.y -= cutD;
#ifdef _DEBUG
		nCntDbg++;
#endif
	}
#ifdef _DEBUG
	dbg.printf("OffsetCnt=%d", nCntDbg);
#endif

	return IsThread();
}

BOOL CreateRoughPass(void)
{
	int			i, j,
				nResult;
	BOOL		bCreate, bInter;
	ENDXFTYPE	enType;
	double		q, qq;
	CPointD		ptChk[4];
	optional<CPointD>	pts;
	CDXFdata*	pData;
	CDXFdata*	pDataChain;
	CDXFdata*	pDataNew;
	CDXFchain*	pChain;
	COutlineData*	pOutline;
	CDXFarc*	pArc;
	DXFLARGV	dxfLine;
	dxfLine.pLayer = NULL;

	// 外径基準線の傾きを計算
	if ( !g_obOutsideTemp.IsEmpty() ) {
		pData = g_obOutsideTemp[0];
		ptChk[0] = pData->GetNativePoint(1) - pData->GetNativePoint(0);
		if ( (qq=atan2(ptChk[0].y, ptChk[0].x)) < 0 )
			qq += PI2;
	}

	// 外径準備ﾃﾞｰﾀをﾙｰﾌﾟさせ荒加工ﾃﾞｰﾀを作成
	for ( i=0; i<g_obOutsideTemp.GetSize() && IsThread(); i++ ) {
		SetProgressPos(i+1);
		pData = g_obOutsideTemp[i];
		pts = pData->GetNativePoint(0);
		pDataChain = NULL;
		bInter = FALSE;		// 一度でも交点があればTRUE
		for ( j=0; j<g_obShape.GetSize() && IsThread(); j++ ) {
			// 一番外側の形状ｵﾌｾｯﾄと交点ﾁｪｯｸ
			pOutline = g_obShape[j]->GetLatheList();
			pChain = pOutline->IsEmpty() ?
				g_obShape[j]->GetShapeChain() : pOutline->GetHead();
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
					g_obLathePass.Add(pDataNew);
					bInter = TRUE;
				}
			END_FOREACH	// End of Chain Loop
		}				// End of Shape Loop
		// 交点端数処理
		if ( pts ) {
			if ( !bInter ) {
				// 交点がなければ外径終点まで生成
				dxfLine.s = *pts;
				dxfLine.e = pData->GetNativePoint(1);
				pDataNew = new CDXFline(&dxfLine);
				pDataNew->OrgTuning(FALSE);
				g_obLathePass.Add(pDataNew);
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
				if ( (q=atan2(ptChk[0].y, ptChk[0].x)) < 0 )
					q += PI2;
				if ( q >= qq ) {
					// 基準線より傾きが大きいときだけ
					// 端数交点から外径終点まで生成
					dxfLine.s = *pts;
					dxfLine.e = pData->GetNativePoint(1);
					pDataNew = new CDXFline(&dxfLine);
					pDataNew->OrgTuning(FALSE);
					g_obLathePass.Add(pDataNew);
				}
			}
		}
	}	// End of g_obOutsideTemp

	return IsThread();
}

BOOL MakeLatheCode(void)
{
	int			i, j, nLoop = g_obLathePass.GetSize();
	double		dCutX;
	CPointD		pt, pts, pte;
	CDXFchain*	pChain;
	CDXFdata*	pData;
	COutlineData*	pOutline;
	CNCMakeLathe*	pNCD;

	// 端面と外径の最大値を取得
	double		dMaxZ = g_pDoc->GetLatheLine(0)->GetStartMakePoint().x,
				dMaxX = g_pDoc->GetLatheLine(1)->GetStartMakePoint().y;

	// 先頭ﾃﾞｰﾀの始点に移動
	if ( nLoop > 0 ) {
		// Gｺｰﾄﾞﾍｯﾀﾞ(開始ｺｰﾄﾞ)
		AddCustomLatheCode(GetStr(MKLA_STR_HEADER));
		//
		pData = g_obLathePass[0];
		pt = pData->GetStartMakePoint();
		if ( pt.x < CNCMakeLathe::ms_xyz[NCA_Z] ) {
			// 工具初期位置が端面の右側
			// →指定位置まで直線移動
			pNCD = new CNCMakeLathe(pt);
		}
		else {
			// 工具初期位置が端面の左側
			// →Z軸移動後、X軸移動
			pNCD = new CNCMakeLathe(ZXMOVE, pt);
		}
		ASSERT(pNCD);
		g_obMakeData.Add(pNCD);
		// 先頭ﾃﾞｰﾀの切削ﾊﾟｽ
		pNCD = new CNCMakeLathe(pData);
		ASSERT(pNCD);
		g_obMakeData.Add(pNCD);
	}

	// 荒加工ﾊﾟｽﾙｰﾌﾟ
	for ( i=1; i<nLoop && IsThread(); i++ ) {
		SetProgressPos(i+1);
		pData = g_obLathePass[i];
		pt = pte = pData->GetStartMakePoint();
		dCutX = pt.y;
		pts.x = CNCMakeLathe::ms_xyz[NCA_Z];
		pts.y = CNCMakeLathe::ms_xyz[NCA_X];
		pts += CDXFdata::ms_ptOrg;		// 現在位置
		pte += CDXFdata::ms_ptOrg;		// 次の移動位置
		pte.y += GetDbl(MKLA_DBL_PULL_X);
		// pDataの始点が現在位置のどちらにあるかで引き代を変える
		if ( CNCMakeLathe::ms_xyz[NCA_Z]<pt.x && !CheckXZMove(pts, pte) ) {
			// 次の始点が右側、かつ、輪郭ｵﾌｾｯﾄに衝突しない → 現在位置から
			pt.y = CNCMakeLathe::ms_xyz[NCA_X];
		}
		else {
			// 次の始点が左側 → 外径から
			pt.y = dMaxX; 
		}
		pt.y += GetDbl(MKLA_DBL_PULL_X);	// 引き代分引いて移動
		pNCD = new CNCMakeLathe(XZMOVE, pt);
		ASSERT(pNCD);
		g_obMakeData.Add(pNCD);
		// 始点への切り込み
		if ( dMaxZ < pt.x ) {
			// 端面よりも右側 → 早送りでX軸方向に移動
			pNCD = new CNCMakeLathe(0, NCA_X, dCutX);
		}
		else {
			// 端面よりも左側 → 切削送りでX軸方向に切り込み
			pNCD = new CNCMakeLathe(1, NCA_X, dCutX);
		}
		ASSERT(pNCD);
		g_obMakeData.Add(pNCD);
		// 切削ﾊﾟｽ
		pNCD = new CNCMakeLathe(pData);
		ASSERT(pNCD);
		g_obMakeData.Add(pNCD);
	}

	// 仕上げしろﾊﾟｽﾙｰﾌﾟ
	for ( i=0; i<g_obShape.GetSize() && IsThread(); i++ ) {
		pOutline = g_obShape[i]->GetLatheList();
		for ( j=0; j<pOutline->GetSize() && IsThread(); j++ ) {
			pChain = pOutline->GetAt(j);
			// 移動ﾊﾟｽ
			if ( !pChain->IsEmpty() )
				MoveLatheCode(pChain->GetHead(), dMaxZ, dMaxX);
			// 切削ﾊﾟｽ
			PLIST_FOREACH(pData, pChain)
				pNCD = new CNCMakeLathe(pData);
				ASSERT(pNCD);
				g_obMakeData.Add(pNCD);
			END_FOREACH
		}
	}

	// 仕上げﾊﾟｽﾙｰﾌﾟ
	for ( i=0; i<g_obShape.GetSize() && IsThread(); i++ ) {
		pChain = g_obShape[i]->GetShapeChain();
		ASSERT(pChain);
		// 移動ﾊﾟｽ
		if ( !pChain->IsEmpty() )
			MoveLatheCode(pChain->GetHead(), dMaxZ, dMaxX);
		// 切削ﾊﾟｽ
		PLIST_FOREACH(pData, pChain)
			pNCD = new CNCMakeLathe(pData);
			ASSERT(pNCD);
			g_obMakeData.Add(pNCD);
		END_FOREACH
	}

	if ( !g_obMakeData.IsEmpty() ) {
		// 工具初期位置へ復帰
		dCutX = dMaxX + GetDbl(MKLA_DBL_PULL_X);
		pt = g_pDoc->GetCircleObject()->GetStartMakePoint();
		pNCD = new CNCMakeLathe(1, NCA_X, dCutX);
		ASSERT(pNCD);
		g_obMakeData.Add(pNCD);
		if ( dCutX < pt.y ) {
			pNCD = new CNCMakeLathe(0, NCA_X, pt.y);
			ASSERT(pNCD);
			g_obMakeData.Add(pNCD);
		}
		pNCD = new CNCMakeLathe(0, NCA_Z, pt.x);
		ASSERT(pNCD);
		g_obMakeData.Add(pNCD);
		if ( pt.y < dCutX ) {
			pNCD = new CNCMakeLathe(0, NCA_X, pt.y);
			ASSERT(pNCD);
			g_obMakeData.Add(pNCD);
		}
		// Gｺｰﾄﾞﾌｯﾀﾞ(終了ｺｰﾄﾞ)
		AddCustomLatheCode(GetStr(MKLA_STR_FOOTER));
	}

	return IsThread();
}

BOOL CheckXZMove(const CPointD& pts, const CPointD& pte)
{
	int			i, j, nLoop;
	BOOL		bResult = FALSE;
	CPointD		pt[4];
	COutlineData*	pOutline;
	CDXFchain*		pChain;
	CDXFdata*		pData;

	// 輪郭ｵﾌｾｯﾄとの交点をﾁｪｯｸ
	for ( i=0; i<g_obShape.GetSize() && !bResult && IsThread(); i++ ) {
		pOutline = g_obShape[i]->GetLatheList();
		nLoop = pOutline->IsEmpty() ? 1 : pOutline->GetSize();
		for ( j=0; j<nLoop && !bResult && IsThread(); j++ ) {
			pChain = pOutline->IsEmpty() ? g_obShape[i]->GetShapeChain() : pOutline->GetAt(j);
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

void MoveLatheCode(const CDXFdata* pData, double dMaxZ, double dMaxX)
{
	CNCMakeLathe*	pNCD;
	CPointD		pts(pData->GetStartMakePoint()),
				pt(pts);

	// X軸(Y)方向の離脱とZ軸(X)方向の移動
	if ( CNCMakeLathe::ms_xyz[NCA_Z] <= pt.x ) {
		pt.x = dMaxZ + GetDbl(MKLA_DBL_PULL_Z);	// 端面＋引き代
		pt.y = CNCMakeLathe::ms_xyz[NCA_X];
	}
	else
		pt.y = dMaxX;	// Z軸はｵﾌﾞｼﾞｪｸﾄの開始位置, X軸は外径
	pt.y += GetDbl(MKLA_DBL_PULL_X);	// 引き代
	pNCD = new CNCMakeLathe(XZMOVE, pt);
	ASSERT(pNCD);
	g_obMakeData.Add(pNCD);
	// X軸始点に移動
	pNCD = new CNCMakeLathe(dMaxZ < pt.x ? 0 : 1, NCA_X, pts.y);
	ASSERT(pNCD);
	g_obMakeData.Add(pNCD);
	if ( dMaxZ < pt.x ) {
		// ｵﾌﾞｼﾞｪｸﾄの始点に移動
		pNCD = new CNCMakeLathe(1, NCA_Z, pts.x);
		ASSERT(pNCD);
		g_obMakeData.Add(pNCD);
	}
}

//////////////////////////////////////////////////////////////////////

// ﾌｪｰｽﾞ出力
void SendFaseMessage
	(int nRange/*=-1*/, int nMsgID/*=-1*/, LPCTSTR lpszMsg/*=NULL*/)
{
#ifdef _DEBUG
	CMagaDbg	dbg("MakeLathe_Thread()", DBG_GREEN);
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
			strResult = CNCMakeLathe::MakeSpindle();
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
	tokenizer<tag_separator>	tokens(str);

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

//////////////////////////////////////////////////////////////////////

// NC生成のｸﾞﾛｰﾊﾞﾙ変数初期化(後始末)ｽﾚｯﾄﾞ
UINT MakeLathe_AfterThread(LPVOID)
{
	g_csMakeAfter.Lock();

	int			i;
#ifdef _DEBUG
	CMagaDbg	dbg("MakeLathe_AfterThread()\nStart", TRUE, DBG_RED);
#endif

	for ( i=0; i<g_obOutsideTemp.GetSize(); i++ )
		delete	g_obOutsideTemp[i];
	for ( i=0; i<g_obLathePass.GetSize(); i++ )
		delete	g_obLathePass[i];
	for ( i=0; i<g_obMakeData.GetSize(); i++ )
		delete	g_obMakeData[i];
	for ( i=0; i<g_pDoc->GetLayerCnt(); i++ )
		g_pDoc->GetLayerData(i)->RemoveAllShape();

	g_obOutsideTemp.RemoveAll();
	g_obLathePass.RemoveAll();
	g_obMakeData.RemoveAll();

	g_csMakeAfter.Unlock();

	return 0;
}
