// TH_MakeNCD.cpp
// DXF->旋盤用NC生成
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "DXFdata.h"
#include "DXFshape.h"
#include "Layer.h"
#include "DXFDoc.h"
#include "NCdata.h"
#include "NCMakeLatheOpt.h"
#include "NCMakeLathe.h"
#include "MakeCustomCode.h"
#include "ThreadDlg.h"

/*
!!!ATTENTION!!!
生成時間の表示：正式ﾘﾘｰｽでは外すのを忘れずに
#define	_DBG_NCMAKE_TIME
*/

using namespace std;
using namespace boost;

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

// よく使う変数や呼び出しの簡略置換
#define	IsThread()	g_pParent->IsThreadContinue()
#define	GetFlg(a)	g_pMakeOpt->GetFlag(a)
#define	GetNum(a)	g_pMakeOpt->GetNum(a)
#define	GetDbl(a)	g_pMakeOpt->GetDbl(a)
#define	SetProgressPos(a)	g_pParent->m_ctReadProgress.SetPos(a)

// ｸﾞﾛｰﾊﾞﾙ変数定義
static	CThreadDlg*			g_pParent;
static	CDXFDoc*			g_pDoc;
static	CNCMakeLatheOpt*	g_pMakeOpt;

// NC生成に必要なﾃﾞｰﾀ群
static	CShapeArray	g_obShape;
static	CDXFarray	g_obOutsideTemp;
static	CDXFarray	g_obLathePass;
static	CTypedPtrArrayEx<CPtrArray, CNCMakeLathe*>	g_obMakeGdata;	// 加工ﾃﾞｰﾀ

// ｻﾌﾞ関数
static	void	InitialVariable(void);		// 変数初期化
static	void	SetStaticOption(void);		// 静的変数の初期化
static	BOOL	MakeLathe_MainFunc(void);	// NC生成のﾒｲﾝﾙｰﾌﾟ
static	BOOL	CreateShapeThread(void);	// 形状認識処理
static	void	InitialShapeData(void);		// 形状認識の初期化
static	BOOL	CreateOutsidePitch(void);	// 外径ｵﾌｾｯﾄを中心まで生成
static	BOOL	CreateRoughPass(void);		// 荒加工ﾃﾞｰﾀの生成
static	BOOL	MakeLatheCode(void);		// NCｺｰﾄﾞの生成
static	void	MoveLatheCode(const CDXFdata*, double, double);
static	BOOL	OutputNCcode(void);			// NCｺｰﾄﾞの出力

// ﾍｯﾀﾞｰ,ﾌｯﾀﾞｰ等のｽﾍﾟｼｬﾙｺｰﾄﾞ生成
static	void	AddCustomCode(const CString&);

// 任意ﾃﾞｰﾀの生成
inline	void	AddMakeLatheStr(const CString& strData)
{
	CNCMakeLathe*	mkNCD = new CNCMakeLathe(strData);
	ASSERT( mkNCD );
	g_obMakeGdata.Add(mkNCD);
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
		BOOL	bResult = FALSE;
		// NC生成ｵﾌﾟｼｮﾝｵﾌﾞｼﾞｪｸﾄの生成とｵﾌﾟｼｮﾝの読み込み
		g_pMakeOpt = new CNCMakeLatheOpt(
				AfxGetNCVCApp()->GetDXFOption()->GetLatheInitList()->GetHead());
		// NC生成のﾙｰﾌﾟ前に必要な初期化
		InitialVariable();
		// 条件ごとに変化するﾊﾟﾗﾒｰﾀを設定
		SetStaticOption();
		// 変数初期化ｽﾚｯﾄﾞの処理待ち
		g_csMakeAfter.Lock();		// ｽﾚｯﾄﾞ側でﾛｯｸ解除するまで待つ
		g_csMakeAfter.Unlock();
		// 増分割り当て
		g_obMakeGdata.SetSize(0, 1024);		// 旋盤ﾃﾞｰﾀ生成は未知数
		// 生成開始
		bResult = MakeLathe_MainFunc();
		if ( bResult )
			bResult = OutputNCcode();

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

	// CNCMakeMillの静的変数初期化
	CPointD	ptOrg(g_pDoc->GetCircleObject()->GetStartMakePoint());
	CNCMakeLathe::ms_xyz[NCA_X] = ptOrg.y;
	CNCMakeLathe::ms_xyz[NCA_Y] = 0;
	CNCMakeLathe::ms_xyz[NCA_Z] = ptOrg.x;

	// 生成ｵﾌﾟｼｮﾝによる静的変数の初期化
	CNCMakeLathe::SetStaticOption(g_pMakeOpt);
}

BOOL OutputNCcode(void)
{
	CString	strPath, strFile,
			strNCFile(g_pDoc->GetNCFileName());
	Path_Name_From_FullPath(strNCFile, strPath, strFile);
	SendFaseMessage(g_obMakeGdata.GetSize(), IDS_ANA_DATAFINAL, strFile);
	try {
		CStdioFile	fp(strNCFile,
				CFile::modeCreate | CFile::modeWrite | CFile::shareExclusive | CFile::typeText);
#ifdef _DEBUG
		for ( int i=0; i<g_obMakeGdata.GetSize(); i++ ) {
#else
		for ( int i=0; i<g_obMakeGdata.GetSize() && IsThread(); i++ ) {
#endif
			g_obMakeGdata[i]->WriteGcode(fp);
			SetProgressPos(i+1);
		}
	}
	catch (	CFileException* e ) {
		strFile.Format(IDS_ERR_DATAWRITE, strNCFile);
		AfxMessageBox(strFile, MB_OK|MB_ICONSTOP);
		e->Delete();
		return FALSE;
	}

	SetProgressPos(g_obMakeGdata.GetSize());
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
	CMagaDbg	dbg("CreateShapeThread()\nStart");
	CDXFchain*	pChainDbg;
	CDXFdata*	pDataDbg;
	POSITION	posdbg;
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
		dbg.printf(" Layer=%s", pLayer->GetStrLayer());
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
				for ( posdbg=pChainDbg->GetHeadPosition(); posdbg; ) {
					pDataDbg = pChainDbg->GetNext(posdbg);
					ptsd = pDataDbg->GetNativePoint(0);
					pted = pDataDbg->GetNativePoint(1);
					dbg.printf("%d (%.3f, %.3f)-(%.3f, %.3f)", pDataDbg->GetType(),
						ptsd.x, ptsd.y, pted.x, pted.y);
				}
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
	int			i;
	POSITION	pos;
	CPointD		pts, pte;
	CDXFchain	ltTemp;
	CDXFchain*	pChain;

	for ( i=0; i<g_obShape.GetSize() && IsThread(); i++ ) {
		pChain = g_obShape[i]->GetShapeChain();
		ASSERT(pChain);
		pts = pChain->GetHead()->GetNativePoint(0);
		pte = pChain->GetTail()->GetNativePoint(1);
		// Z値(X)が大きい方を先頭ﾒﾝﾊﾞに
		if ( pts.x < pte.x ) {
#ifdef _DEBUG
			dbg.printf("(pts.x < pte.x) Object Reverse");
#endif
			ltTemp.RemoveAll();
			for ( pos=pChain->GetHeadPosition(); pos && IsThread(); )
				ltTemp.AddHead(pChain->GetNext(pos));
			pChain->RemoveAll();
			pChain->AddTail(&ltTemp);
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
	POSITION	pos;
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
		for ( pos=pChain->GetHeadPosition(); pos && IsThread(); ) {
			pData = pChain->GetNext(pos);
#ifdef _DEBUG
			ptsd = pData->GetNativePoint(0);
			pted = pData->GetNativePoint(1);
			rc = pData->GetMaxRect();
			dbg.printf("%d (%.3f, %.3f)-(%.3f, %.3f)", pData->GetType(),
				ptsd.x, ptsd.y, pted.x, pted.y);
#else
			rc = pData->GetMaxRect();
#endif
			if ( dLimit > rc.top )
				dLimit = rc.top;
		}
		// 所属ﾒﾝﾊﾞの原点調整
		for ( j=0; j<pOutline->GetSize() && IsThread(); j++ ) {
			pChain = pOutline->GetAt(j);
			ASSERT(pChain);
			pChain->OrgTuning();
		}
	}
#ifdef _DEBUG
	dbg.printf("Limit=%f", dLimit);
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
	POSITION	pos;
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
			qq += 360.0*RAD;
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
			for ( pos=pChain->GetHeadPosition(); pos && IsThread(); ) {
				bCreate = FALSE;
				pDataChain = pChain->GetNext(pos);
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
			}			// End of Chain Loop
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
					q += 360.0*RAD;
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
	POSITION	pos;
	double		dCutX;
	CPointD		pt;
	CDXFchain*	pChain;
	CDXFdata*	pData;
	COutlineData*	pOutline;
	CNCMakeLathe*	mkNCD;

	// 端面と外径の最大値を取得
	double		dMaxZ = g_pDoc->GetLatheLine(0)->GetStartMakePoint().x,
				dMaxX = g_pDoc->GetLatheLine(1)->GetStartMakePoint().y;

	// 先頭ﾃﾞｰﾀの始点に移動
	if ( nLoop > 0 ) {
		// Gｺｰﾄﾞﾍｯﾀﾞ(開始ｺｰﾄﾞ)
		AddCustomCode(g_pMakeOpt->GetStr(MKLA_STR_HEADER));
		//
		pData = g_obLathePass[0];
		pt = pData->GetStartMakePoint();
		if ( pt.x < CNCMakeLathe::ms_xyz[NCA_Z] ) {
			// 工具初期位置が端面の右側
			// →指定位置まで直線移動
			mkNCD = new CNCMakeLathe(pt);
		}
		else {
			// 工具初期位置が端面の左側
			// →Z軸移動後、X軸移動
			mkNCD = new CNCMakeLathe(ZXMOVE, pt);
		}
		ASSERT(mkNCD);
		g_obMakeGdata.Add(mkNCD);
		// 先頭ﾃﾞｰﾀの切削ﾊﾟｽ
		mkNCD = new CNCMakeLathe(pData);
		ASSERT(mkNCD);
		g_obMakeGdata.Add(mkNCD);
	}

	// 荒加工ﾊﾟｽﾙｰﾌﾟ
	for ( i=1; i<nLoop && IsThread(); i++ ) {
		SetProgressPos(i+1);
		pData = g_obLathePass[i];
		pt = pData->GetStartMakePoint();
		dCutX = pt.y;
		// pDataの始点が現在位置のどちらにあるかで引き代を変える
		if ( CNCMakeLathe::ms_xyz[NCA_Z] < pt.x ) {
			// 次の始点が右側 → 現在位置から
			pt.y = CNCMakeLathe::ms_xyz[NCA_X];
		}
		else {
			// 次の始点が左側 → 外径から
			pt.y = dMaxX; 
		}
		pt.y += GetDbl(MKLA_DBL_PULL_X);	// 引き代分引いて移動
		mkNCD = new CNCMakeLathe(XZMOVE, pt);
		ASSERT(mkNCD);
		g_obMakeGdata.Add(mkNCD);
		// 始点への切り込み
		if ( dMaxZ < pt.x ) {
			// 端面よりも右側 → 早送りでX軸方向に移動
			mkNCD = new CNCMakeLathe(0, NCA_X, dCutX);
		}
		else {
			// 端面よりも左側 → 切削送りでX軸方向に切り込み
			mkNCD = new CNCMakeLathe(1, NCA_X, dCutX);
		}
		ASSERT(mkNCD);
		g_obMakeGdata.Add(mkNCD);
		// 切削ﾊﾟｽ
		mkNCD = new CNCMakeLathe(pData);
		ASSERT(mkNCD);
		g_obMakeGdata.Add(mkNCD);
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
			for ( pos=pChain->GetHeadPosition(); pos && IsThread(); ) {
#ifdef _DEBUG
				pData = pChain->GetNext(pos);
				mkNCD = new CNCMakeLathe(pData);
#else
				mkNCD = new CNCMakeLathe(pChain->GetNext(pos));
#endif
				ASSERT(mkNCD);
				g_obMakeGdata.Add(mkNCD);
			}
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
		for ( pos=pChain->GetHeadPosition(); pos && IsThread(); ) {
#ifdef _DEBUG
			pData = pChain->GetNext(pos);
			mkNCD = new CNCMakeLathe(pData);
#else
			mkNCD = new CNCMakeLathe(pChain->GetNext(pos));
#endif
			ASSERT(mkNCD);
			g_obMakeGdata.Add(mkNCD);
		}
	}

	if ( !g_obMakeGdata.IsEmpty() ) {
		// 工具初期位置へ復帰
		dCutX = dMaxX + GetDbl(MKLA_DBL_PULL_X);
		pt = g_pDoc->GetCircleObject()->GetStartMakePoint();
		mkNCD = new CNCMakeLathe(1, NCA_X, dCutX);
		ASSERT(mkNCD);
		g_obMakeGdata.Add(mkNCD);
		if ( dCutX < pt.y ) {
			mkNCD = new CNCMakeLathe(0, NCA_X, pt.y);
			ASSERT(mkNCD);
			g_obMakeGdata.Add(mkNCD);
		}
		mkNCD = new CNCMakeLathe(0, NCA_Z, pt.x);
		ASSERT(mkNCD);
		g_obMakeGdata.Add(mkNCD);
		if ( pt.y < dCutX ) {
			mkNCD = new CNCMakeLathe(0, NCA_X, pt.y);
			ASSERT(mkNCD);
			g_obMakeGdata.Add(mkNCD);
		}
		// Gｺｰﾄﾞﾌｯﾀﾞ(終了ｺｰﾄﾞ)
		AddCustomCode(g_pMakeOpt->GetStr(MKLA_STR_FOOTER));
	}

	return IsThread();
}

void MoveLatheCode(const CDXFdata* pData, double dMaxZ, double dMaxX)
{
	CNCMakeLathe*	mkNCD;
	CPointD		pts(pData->GetStartMakePoint()),
				pt(pts);

	// X軸(Y)方向の離脱とZ軸(X)方向の移動
	if ( CNCMakeLathe::ms_xyz[NCA_Z] < pt.x ) {
		pt.x = dMaxZ + GetDbl(MKLA_DBL_PULL_Z);	// 端面＋引き代
		pt.y = CNCMakeLathe::ms_xyz[NCA_X];
	}
	else
		pt.y = dMaxX;	// Z軸はｵﾌﾞｼﾞｪｸﾄの開始位置, X軸は外径
	pt.y += GetDbl(MKLA_DBL_PULL_X);	// 引き代
	mkNCD = new CNCMakeLathe(XZMOVE, pt);
	ASSERT(mkNCD);
	g_obMakeGdata.Add(mkNCD);
	// X軸始点に移動
	mkNCD = new CNCMakeLathe(dMaxZ < pt.x ? 0 : 1, NCA_X, pts.y);
	ASSERT(mkNCD);
	g_obMakeGdata.Add(mkNCD);
	if ( dMaxZ < pt.x ) {
		// ｵﾌﾞｼﾞｪｸﾄの始点に移動
		mkNCD = new CNCMakeLathe(1, NCA_Z, pts.x);
		ASSERT(mkNCD);
		g_obMakeGdata.Add(mkNCD);
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

//	AddCustomCode() parse()関数から呼び出し
class CMakeCustomCode_Lathe : public CMakeCustomCode	// MakeCustomCode.h
{
public:
	CMakeCustomCode_Lathe(CString& r) :
				CMakeCustomCode(r, g_pDoc, NULL, g_pMakeOpt) {
		static	LPCTSTR	szCustomCode[] = {
			"ProgNo", 
			"G90orG91", "SPINDLE",
			"LatheDiameter", "LatheZmax", "LatheZmin",
			"ToolPosX", "ToolPosZ"
		};
		// ｵｰﾀﾞｰ追加
		m_strOrderIndex.AddElement(SIZEOF(szCustomCode), szCustomCode);
	}

	void	operator()(const char* s, const char* e) const {
		CString	strTmp;

		// 基底ｸﾗｽ呼び出し
		int		nTestCode = CMakeCustomCode::ReplaceCustomCode(s, e);

		// replace
		switch ( nTestCode ) {
		case 0:		// ProgNo
			if ( GetFlg(MKLA_FLG_PROG) ) {
				strTmp.Format(IDS_MAKENCD_PROG,
					GetFlg(MKLA_FLG_PROGAUTO) ? ::GetRandom(1,9999) : GetNum(MKLA_NUM_PROG));
				m_strResult += strTmp;
			}
			break;
		case 1:		// G90orG91
			m_strResult += CNCMakeBase::MakeCustomString(GetNum(MKLA_NUM_G90)+90);
			break;
		case 2:		// SPINDLE
			m_strResult += CNCMakeLathe::MakeSpindle();
			break;
		case 3:		// LatheDiameter
			strTmp.Format(IDS_MAKENCD_FORMAT, g_pDoc->GetLatheLine(1)->GetStartMakePoint().y * 2.0);
			m_strResult += strTmp;
			break;
		case 4:		// LatheZmax
			strTmp.Format(IDS_MAKENCD_FORMAT, g_pDoc->GetLatheLine(0)->GetStartMakePoint().x);
			m_strResult += strTmp;
			break;
		case 5:		// LatheZmin
			strTmp.Format(IDS_MAKENCD_FORMAT, g_pDoc->GetLatheLine(0)->GetEndMakePoint().x);
			m_strResult += strTmp;
			break;
		case 6:		// ToolPosX
			strTmp.Format(IDS_MAKENCD_FORMAT, g_pDoc->GetCircleObject()->GetStartMakePoint().y * 2.0);
			m_strResult += strTmp;
			break;
		case 7:		// ToolPosZ
			strTmp.Format(IDS_MAKENCD_FORMAT, g_pDoc->GetCircleObject()->GetStartMakePoint().x);
			m_strResult += strTmp;
			break;
		}
	}
};

void AddCustomCode(const CString& strFileName)
{
	using namespace boost::spirit::classic;

	CString	strBuf;
	CString	strResult;
	CMakeCustomCode_Lathe	custom(strResult);

	try {
		CStdioFile	fp(strFileName,
			CFile::modeRead | CFile::shareDenyWrite | CFile::typeText);
		while ( fp.ReadString(strBuf) && IsThread() ) {
			// 構文解析
			strResult.Empty();
			if ( parse((LPCTSTR)strBuf, *( *(anychar_p - '{')[custom] >> comment_p('{', '}')[custom] ) ).hit ) {
				if ( !strResult.IsEmpty() )
					AddMakeLatheStr( strResult );
			}
			else
				AddMakeLatheStr( strBuf );
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
	for ( i=0; i<g_obMakeGdata.GetSize(); i++ )
		delete	g_obMakeGdata[i];
	for ( i=0; i<g_pDoc->GetLayerCnt(); i++ )
		g_pDoc->GetLayerData(i)->RemoveAllShape();

	g_obOutsideTemp.RemoveAll();
	g_obLathePass.RemoveAll();
	g_obMakeGdata.RemoveAll();

	g_csMakeAfter.Unlock();

	return 0;
}
