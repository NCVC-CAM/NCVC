// TH_MakeWire.cpp
//		ワイヤ放電加工機用NC生成
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "DXFOption.h"
#include "DXFdata.h"
#include "DXFshape.h"
#include "Layer.h"
#include "DXFDoc.h"
#include "NCMakeWireOpt.h"
#include "NCMakeWire.h"
#include "MakeCustomCode.h"
#include "ThreadDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
//#define	_DBG_NCMAKE_TIME	//	生成時間の表示
#endif

using std::string;
using namespace boost;

// ｸﾞﾛｰﾊﾞﾙ変数定義
static	CThreadDlg*			g_pParent;
static	CDXFDoc*			g_pDoc;
static	CNCMakeWireOpt*		g_pMakeOpt;
static	int					g_nFase;	// ﾌｪｰｽﾞ№

// よく使う変数や呼び出しの簡略置換
#define	IsThread()	g_pParent->IsThreadContinue()
#define	GetFlg(a)	g_pMakeOpt->GetFlag(a)
#define	GetNum(a)	g_pMakeOpt->GetNum(a)
#define	GetDbl(a)	g_pMakeOpt->GetDbl(a)
#define	GetStr(a)	g_pMakeOpt->GetStr(a)

// NC生成に必要なﾃﾞｰﾀ群
static	CSortArray<CObArray, CDXFcircle*>
					g_obAWFinside, g_obAWFoutside;	// AWFﾃﾞｰﾀ(CDXFcircle*)
static	CDXFmap		g_mpPause;		// 一時停止指示点
static	CTypedPtrArrayEx<CPtrArray, CNCMakeWire*>	g_obMakeData;	// 加工ﾃﾞｰﾀ

static	WORD	g_wBindOperator;	// Bind時のﾌｧｲﾙ出力指示
static	BOOL	g_bAWF;				// AWF接続状況

// ｻﾌﾞ関数
static	void	InitialVariable(void);		// 変数初期化
static	void	SetStaticOption(void);		// 静的変数の初期化
static	BOOL	MakeWire_MainFunc(void);	// NC生成のﾒｲﾝﾙｰﾌﾟ
static	BOOL	MakeLoopWire(CDXFshape*);
static	INT_PTR	MakeLoopWireSearch(CDXFshape*, int);
static	BOOL	MakeLoopWireAdd(CDXFshape*, CDXFshape*, BOOL);
static	BOOL	MakeLoopWireAdd_Hetero(CDXFshape*, CDXFshape*);
static	BOOL	MakeLoopWireAdd_ChainList(CDXFshape*, CDXFchain*);
static	BOOL	MakeLoopWireAdd_EulerMap(CDXFshape*);
static	BOOL	MakeLoopWireAdd_EulerMap_Make(CDXFshape*, CDXFmap*, BOOL&);
static	BOOL	MakeLoopWireAdd_EulerMap_Search(const CPointF&, CDXFmap*, CDXFmap*);
static	BOOL	MakeLoopWireAdd_with_one_stroke(const CDXFmap*, BOOL, BOOL, const CPointF&, const CDXFarray*, CDXFlist&);
static	CDXFcircle*	SetAWFandPAUSEdata(void);	// AWFと一時停止ﾎﾟｲﾝﾄの登録処理
static	BOOL	CreateShapeThread(void);	// 形状認識処理
static	BOOL	OutputWireCode(void);		// NCｺｰﾄﾞの出力

// 引数で指定したｵﾌﾞｼﾞｪｸﾄに一番近いｵﾌﾞｼﾞｪｸﾄを返す
static	CDXFshape*	GetNearPointWire(const CPointF&);
static	CDXFcircle*	GetInsideAWF(const CDXFshape*);
static	CDXFcircle*	GetOutsideAWF(void);

// ﾍｯﾀﾞｰ,ﾌｯﾀﾞｰ等のｽﾍﾟｼｬﾙｺｰﾄﾞ生成
static	void	AddCustomWireCode(const CString&);

// 任意ﾃﾞｰﾀの生成
static inline	void	_AddMakeWireStr(const CString& strData)
{
	CNCMakeWire*	pNCD = new CNCMakeWire(strData);
	ASSERT( pNCD );
	g_obMakeData.Add(pNCD);
}
// AWF結線
static inline	void	_AddAWFconnect(void)
{
	if ( !g_bAWF ) {
		_AddMakeWireStr(GetStr(MKWI_STR_AWFCNT));
		g_bAWF = TRUE;
	}
}
// AWF切断
static inline	void	_AddAWFcut(void)
{
	if ( g_bAWF ) {
		_AddMakeWireStr(GetStr(MKWI_STR_AWFCUT));
		g_bAWF = FALSE;
	}
}
// AWFﾎﾟｲﾝﾄへの移動(G00)
static inline	void	_AddMoveAWFpoint(CDXFcircle* pCircle)
{
	CNCMakeWire*	pNCD = new CNCMakeWire(0, pCircle->GetMakeCenter(),
			0.0f, GetDbl(MKWI_DBL_TAPER));
	ASSERT( pNCD );
	g_obMakeData.Add(pNCD);
	pCircle->SetMakeFlg();
	_AddAWFconnect();
}
// 切削ﾃﾞｰﾀへの移動(G01)
static inline	void	_AddMoveGdata(const CDXFdata* pData)
{
	CNCMakeWire*	pNCD = new CNCMakeWire(1, pData->GetStartMakePoint(),
			GetDbl(MKWI_DBL_FEED), GetDbl(MKWI_DBL_TAPER));
	ASSERT( pNCD );
	g_obMakeData.Add(pNCD);
}
// 切削ﾃﾞｰﾀへの移動(G01)
static inline	void	_AddMoveGdata(const CDXFdata* pDataXY, const CDXFdata* pDataUV)
{
	CNCMakeWire*	pNCD = new CNCMakeWire(
			pDataXY->GetStartMakePoint(), pDataUV->GetStartMakePoint(),
			GetDbl(MKWI_DBL_FEED));
	ASSERT( pNCD );
	g_obMakeData.Add(pNCD);
}
// 切削ﾃﾞｰﾀ
static inline	void	_AddMakeGdata(CDXFdata* pData)
{
	CNCMakeWire*	pNCD = new CNCMakeWire(pData, GetDbl(MKWI_DBL_FEED));
	ASSERT( pNCD );
	g_obMakeData.Add(pNCD);
	pData->SetMakeFlg();
}
// 切削ﾃﾞｰﾀ（上下異形状）
static inline	void	_AddMakeGdata(CDXFdata* pDataXY, CDXFdata* pDataUV)
{
	CNCMakeWire*	pNCD = new CNCMakeWire(pDataXY, pDataUV, GetDbl(MKWI_DBL_FEED));
	ASSERT( pNCD );
	g_obMakeData.Add(pNCD);
	pDataXY->SetMakeFlg();
	pDataUV->SetMakeFlg();
}
// 切削ﾃﾞｰﾀ（上下異形状微細線分）
static inline	void	_AddMakeGdata(const CVPointF& vptXY, const CVPointF& vptUV)
{
	CNCMakeWire*	pNCD = new CNCMakeWire(vptXY, vptUV, GetDbl(MKWI_DBL_FEED));
	ASSERT( pNCD );
	g_obMakeData.Add(pNCD);
}

// ｻﾌﾞｽﾚｯﾄﾞ関数
static	CCriticalSection	g_csMakeAfter;	// MakeWire_AfterThread()ｽﾚｯﾄﾞﾛｯｸｵﾌﾞｼﾞｪｸﾄ
static	UINT	MakeWire_AfterThread(LPVOID);	// 後始末ｽﾚｯﾄﾞ

//////////////////////////////////////////////////////////////////////
// ワイヤ放電加工機用NC生成ｽﾚｯﾄﾞ
//////////////////////////////////////////////////////////////////////

UINT MakeWire_Thread(LPVOID pVoid)
{
#ifdef _DEBUG
	printf("MakeWire_Thread() Start\n");
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
	int i = (int)(pParam->lParam);
	//	= 0 : Normal
	//	= 1 : ﾌｯﾀ出力なし
	//	> 1 : ﾌｧｲﾙ追加ﾓｰﾄﾞ, ﾍｯﾀﾞ/ﾌｯﾀ出力なし
	//	=-1 : ﾌｧｲﾙ追加ﾓｰﾄﾞ, ﾍｯﾀﾞ出力なし
	//	=-2 : one bind
	if ( i == 1 )
		g_wBindOperator = TH_HEADER;
	else if ( i > 1 )
		g_wBindOperator = TH_APPEND;
	else if ( i == -1 )
		g_wBindOperator = TH_FOOTER | TH_APPEND;
	else
		g_wBindOperator = TH_HEADER | TH_FOOTER;

	// 準備中表示
	g_nFase = 0;
	SendFaseMessage(g_pParent, g_nFase, -1, IDS_ANA_DATAINIT);
	g_pMakeOpt = NULL;

	// 下位の CMemoryException は全てここで集約
	try {
		// NC生成ｵﾌﾟｼｮﾝｵﾌﾞｼﾞｪｸﾄの生成とｵﾌﾟｼｮﾝの読み込み
		g_pMakeOpt = new CNCMakeWireOpt(
				AfxGetNCVCApp()->GetDXFOption()->GetInitList(NCMAKEWIRE)->GetHead());
		// NC生成のﾙｰﾌﾟ前に必要な初期化
		{
			optional<CPointF>	ptResult = g_pDoc->GetCutterOrigin();
			CDXFdata::ms_ptOrg = ptResult ? *ptResult : 0.0f;
		}
		g_pDoc->GetCircleObject()->OrgTuning(FALSE);	// 結果的に原点がｾﾞﾛになる
		InitialVariable();
		// 条件ごとに変化するﾊﾟﾗﾒｰﾀを設定
		SetStaticOption();
		// 変数初期化ｽﾚｯﾄﾞの処理待ち
		g_csMakeAfter.Lock();		// ｽﾚｯﾄﾞ側でﾛｯｸ解除するまで待つ
		g_csMakeAfter.Unlock();
		// 増分割り当て
		g_obMakeData.SetSize(0, 1024);
		// 生成開始
		BOOL bResult = MakeWire_MainFunc();
		if ( bResult )
			bResult = OutputWireCode();

		// 戻り値ｾｯﾄ
		if ( bResult && IsThread() )
			nResult = IDOK;
#ifdef _DEBUG
		printf("MakeWire_Thread All Over!!!\n");
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
	_dp.SetDecimal3();
	g_pParent->PostMessage(WM_USERFINISH, nResult);	// このｽﾚｯﾄﾞからﾀﾞｲｱﾛｸﾞ終了

	// 生成したNCｺｰﾄﾞの消去ｽﾚｯﾄﾞ(優先度を下げる)
	AfxBeginThread(MakeWire_AfterThread, NULL,
		THREAD_PRIORITY_IDLE);

	return 0;
}

//////////////////////////////////////////////////////////////////////

void InitialVariable(void)
{
	if ( !(g_wBindOperator & TH_APPEND) )
		CDXFdata::ms_pData = g_pDoc->GetCircleObject();
	CNCMakeWire::InitialVariable();		// CNCMakeBase::InitialVariable()
}

void SetStaticOption(void)
{
	g_bAWF = GetFlg(MKWI_FLG_AWFSTART);

	// CDXFdataの静的変数初期化
	CDXFdata::ms_fXRev = CDXFdata::ms_fYRev = FALSE;

	// CNCMakeWireの静的変数初期化
	CNCMakeWire::ms_xyz[NCA_X] = 0.0f;
	CNCMakeWire::ms_xyz[NCA_Y] = 0.0f;
	CNCMakeWire::ms_xyz[NCA_Z] = 0.0f;

	// 生成ｵﾌﾟｼｮﾝによる静的変数の初期化
	CNCMakeWire::SetStaticOption(g_pMakeOpt);
}

BOOL OutputWireCode(void)
{
	CString	strPath, strFile,
			strNCFile(g_pDoc->GetNCFileName());
	Path_Name_From_FullPath(strNCFile, strPath, strFile);
	SendFaseMessage(g_pParent, g_nFase, g_obMakeData.GetSize(), IDS_ANA_DATAFINAL, strFile);
	try {
		UINT	nOpenFlg = CFile::modeCreate | CFile::modeWrite |
			CFile::shareExclusive | CFile::typeText | CFile::osSequentialScan;
		if ( g_wBindOperator & TH_APPEND )
			nOpenFlg |= CFile::modeNoTruncate;
		CStdioFile	fp(strNCFile, nOpenFlg);
		if ( g_wBindOperator & TH_APPEND )
			fp.SeekToEnd();
		for ( INT_PTR i=0; i<g_obMakeData.GetSize() && IsThread(); i++ ) {
			g_obMakeData[i]->WriteGcode(fp);
			SetProgressPos(g_pParent, i+1);
		}
	}
	catch (	CFileException* e ) {
		strFile.Format(IDS_ERR_DATAWRITE, strNCFile);
		AfxMessageBox(strFile, MB_OK|MB_ICONSTOP);
		e->Delete();
		return FALSE;
	}

	SetProgressPos(g_pParent, g_obMakeData.GetSize());
	return IsThread();
}

//////////////////////////////////////////////////////////////////////
// NC生成ﾒｲﾝｽﾚｯﾄﾞ
//////////////////////////////////////////////////////////////////////

BOOL MakeWire_MainFunc(void)
{
	if ( !g_pDoc->IsDocFlag(DXFDOC_SHAPE) ) {
		// 形状認識処理を用いて図形集合を作成
		if ( !CreateShapeThread() )
			return FALSE;
	}

	INT_PTR		i, j, nLoop;
	CDXFcircle*	pCircle;
	CLayerData*	pLayer;
	CDXFshape*	pShape;

	// 原点調整とﾏｯﾌﾟの生成ﾌﾗｸﾞをｸﾘｱ
	for ( i=0; i<g_pDoc->GetLayerCnt() && IsThread(); i++ ) {
		pLayer = g_pDoc->GetLayerData(i);
		if ( !pLayer->IsMakeTarget() )
			continue;
		nLoop = pLayer->GetShapeSize();
		for ( j=0; j<nLoop && IsThread(); j++ ) {
			pShape = pLayer->GetShapeData(j);
			pShape->OrgTuning();
			pShape->ClearMakeFlg();
		}
	}

	// AWFﾎﾟｲﾝﾄ
	pCircle = SetAWFandPAUSEdata();

	// AWFに近い座標ﾏｯﾌﾟを検索
	CPointF	pt( pCircle ? pCircle->GetCenter() :
			(CDXFdata::ms_pData->GetEndCutterPoint() + CDXFdata::ms_ptOrg) );
	pShape = GetNearPointWire(pt);

	// NC生成ﾙｰﾌﾟ
	if ( !MakeLoopWire(pShape) )
		return FALSE;

	if ( pShape ) {
		// AWF切断
		if ( GetFlg(MKWI_FLG_AWFEND) )
			_AddAWFcut();
		// Gｺｰﾄﾞﾌｯﾀﾞ(終了ｺｰﾄﾞ)
		if ( g_wBindOperator & TH_FOOTER )
			AddCustomWireCode(GetStr(MKWI_STR_FOOTER));
	}

	return IsThread();
}

//////////////////////////////////////////////////////////////////////

BOOL MakeLoopWire(CDXFshape* pShape)
{
#ifdef _DEBUG
	printf("MakeLoopWire()\n");
#endif
	INT_PTR		nCnt, nPos = 0;

	while ( pShape && IsThread() ) {
#ifdef _DEBUG
		printf("ParentMapName=%s\n", LPCTSTR(pShape->GetShapeName()));
#endif
		// 形状集合の内側から生成
		pShape->SetShapeFlag(DXFMAPFLG_SEARCH);	// 親(外側)形状集合は検索対象外
		if ( (nCnt=MakeLoopWireSearch(pShape, 0)) < 0 )
			return FALSE;
		// 親形状集合自身の生成
		if ( !MakeLoopWireAdd(pShape, NULL, TRUE) )
			return FALSE;
		// ﾌﾟﾛｸﾞﾚｽﾊﾞｰの更新
		nPos += nCnt+1;
		SetProgressPos(g_pParent, nPos);
		// 次の形状集合を検索
		pShape = GetNearPointWire(CDXFdata::ms_pData->GetEndCutterPoint() + CDXFdata::ms_ptOrg);
	}

	return IsThread();
}

INT_PTR MakeLoopWireSearch(CDXFshape* pShapeBase, int nRef)
{
#ifdef _DEBUG
	printf("MakeLoopWireSearch()\n");
#endif

	INT_PTR		i, j, nCnt = 0;
	CLayerData*	pLayer;
	CDXFshape*	pShape;
	CShapeArray	obShape;	// 内側の形状集合一覧
	CRectF		rcBase(pShapeBase->GetMaxRect() ), rc;
	rcBase.InflateRect(rcBase.Width()*0.05f, rcBase.Height()*0.05f);	// 計算誤差緩和のため10%拡大
	obShape.SetSize(0, 64);

	// pShapeBaseより内側の矩形を持つ形状集合を検索
	for ( i=0; i<g_pDoc->GetLayerCnt() && IsThread(); i++ ) {
		pLayer = g_pDoc->GetLayerData(i);
		for ( j=0; j<pLayer->GetShapeSize() && IsThread(); j++ ) {
			pShape = pLayer->GetShapeData(j);
			rc = pShape->GetMaxRect();
			if ( !pShape->IsMakeFlg() && !pShape->IsSearchFlg() &&
					pShape->GetShapeAssemble()!=DXFSHAPE_EXCLUDE &&
					rcBase.PtInRect(rc.TopLeft().RoundUp()) && rcBase.PtInRect(rc.BottomRight().RoundUp()) ) {
				pShape->SetShapeFlag(DXFMAPFLG_SEARCH);
				obShape.Add(pShape);
				nCnt += MakeLoopWireSearch(pShape, nRef+1);
			}
		}
	}

	if ( obShape.IsEmpty() || !IsThread() )
		return 0;

	// obShapeに蓄積されたﾃﾞｰﾀの生成
	CDXFshape*	pShapeResult = obShape[0];
	j = obShape.GetSize();

	if ( j == 1 ) {
		// 内側ｵﾌﾞｼﾞｪｸﾄが1つ
		BOOL	bResult;
		if ( pShapeBase->GetShapeType()==0 && pShapeResult->GetShapeType()==0 ) {
			// 両方とも輪郭集合の場合のみ
			CString	strLayer1(pShapeBase->GetParentLayer()->GetLayerName()),
					strLayer2(pShapeResult->GetParentLayer()->GetLayerName());
			int		nCmp = strLayer1.CompareNoCase(strLayer2);
			if ( nCmp > 0 ) {
				// 上下異形状生成処理(ﾚｲﾔ名の大きい方を第2引数UV扱いで)
				bResult =MakeLoopWireAdd(pShapeResult, pShapeBase, nRef>0 ? FALSE : TRUE);
			}
			else if ( nCmp < 0 ) {
				bResult =MakeLoopWireAdd(pShapeBase, pShapeResult, nRef>0 ? FALSE : TRUE);
			}
			else {
				// 通常処理
				bResult = MakeLoopWireAdd(pShapeResult, NULL, FALSE);
			}
		}
		else {
			// 通常処理
			bResult = MakeLoopWireAdd(pShapeResult, NULL, FALSE);
		}
		return bResult ? nCnt : -1;
	}

	// 内側に複数のｵﾌﾞｼﾞｪｸﾄ
	const	CPointF		ptOrg(CDXFdata::ms_ptOrg);
			CPointF		pt(CDXFdata::ms_pData->GetEndCutterPoint() + ptOrg);
	float	dGap, dGapMin;

	while ( pShapeResult && IsThread() ) {
		dGapMin = FLT_MAX;
		pShapeResult = NULL;
		// ptに一番近いﾈｲﾃｨﾌﾞの形状集合を検索
		for ( i=0; i<j && IsThread(); i++ ) {
			pShape = obShape[i];
			if ( !pShape->IsMakeFlg() ) {
				dGap = pShape->GetSelectObjectFromShape(pt);
				if ( dGap < dGapMin ) {
					dGapMin = dGap;
					pShapeResult = pShape;
				}
			}
		}
		if ( pShapeResult ) {
			// 形状集合のNC生成
			if ( !MakeLoopWireAdd(pShapeResult, NULL, FALSE) )
				return -1;
			// pt座標の更新
			pt = CDXFdata::ms_pData->GetEndCutterPoint() + ptOrg;
		}
	}

	return nCnt;
}

BOOL MakeLoopWireAdd(CDXFshape* pShapeXY, CDXFshape* pShapeUV, BOOL bParent)
{
#ifdef _DEBUG
	printf("MakeLoopWireAdd() MapName=%s\n", LPCTSTR(pShapeXY->GetShapeName()));
	if ( pShapeUV )
		printf(" +MapName(UV)=%s\n", LPCTSTR(pShapeUV->GetShapeName()));
#endif
	if ( pShapeXY->IsMakeFlg() )	// 併合輪郭等で、既に生成済みの場合がある
		return TRUE;
	pShapeXY->SetShapeFlag(DXFMAPFLG_MAKE);

	if ( pShapeUV ) {
		if ( pShapeUV->IsMakeFlg() )
			pShapeUV = NULL;
		else
			pShapeUV->SetShapeFlag(DXFMAPFLG_MAKE);
	}

	// Gｺｰﾄﾞﾍｯﾀﾞ(開始ｺｰﾄﾞ)
	if ( g_obMakeData.IsEmpty() ) {
		// Gｺｰﾄﾞﾍｯﾀﾞ(開始ｺｰﾄﾞ)
		if ( g_wBindOperator & TH_HEADER )
			AddCustomWireCode(GetStr(MKWI_STR_HEADER));
		// ﾌｧｲﾙ名のｺﾒﾝﾄ
		if ( g_pDoc->IsDocFlag(DXFDOC_BIND) && AfxGetNCVCApp()->GetDXFOption()->GetDxfOptFlg(DXFOPT_FILECOMMENT) ) {
			CString	strBuf;
			strBuf.Format(IDS_MAKENCD_BINDFILE, g_pDoc->GetTitle());
			_AddMakeWireStr(strBuf);
		}
	}

	// AWFﾎﾟｲﾝﾄの検索
	CDXFcircle* pCircle = bParent ? GetOutsideAWF() : GetInsideAWF(pShapeXY);
	if ( pCircle ) {
		// AWF切断
		_AddAWFcut();
		// AWFﾎﾟｲﾝﾄまでの移動と結線
		_AddMoveAWFpoint(pCircle);
		CDXFdata::ms_pData = pCircle;
	}
	else {
		// AWF結線
		_AddAWFconnect();
	}

	// 処理の分岐
	if ( pShapeUV )
		return MakeLoopWireAdd_Hetero(pShapeXY, pShapeUV);
	else {
		CDXFchain*	pChain = pShapeXY->GetShapeChain();
		return pChain ?
			MakeLoopWireAdd_ChainList(pShapeXY, pChain) :
			MakeLoopWireAdd_EulerMap(pShapeXY);
	}
}

//////////////////////////////////////////////////////////////////////

BOOL MakeLoopWireAdd_Hetero(CDXFshape* pShapeXY, CDXFshape* pShapeUV)
{
	POSITION	posXY, posUV, posXYb, posUVb;
	CDXFchain*	pChainXY = pShapeXY->GetShapeChain();
	CDXFchain*	pChainUV = pShapeUV->GetShapeChain();
	CDXFdata*	pDataXY;
	CDXFdata*	pDataUV;
	CVPointF	vptXY, vptUV;
	size_t		nCntXY = pChainXY->GetObjectCount(),
				nCntUV = pChainUV->GetObjectCount();
	int			nXYUV;		// 1:XY処理で終了, 2:UV処理で終了, 0:両方同時

	ASSERT(pChainXY);	ASSERT(pChainUV);
	posXY = posXYb = pShapeXY->GetFirstChainPosition();
	posUV = posUVb = pShapeUV->GetFirstChainPosition();
	ASSERT(posXY);		ASSERT(posUV);

	pDataXY = pChainXY->GetAt(posXY);
	pDataUV = pChainUV->GetAt(posUV);
	_AddMoveGdata(pDataXY, pDataUV);

	if ( !g_mpPause.IsEmpty() ) {
		// XY,UV連携 一時停止ﾓｰﾄﾞ（ｵﾌﾞｼﾞｪｸﾄ数が正しく連携できることが条件）
		CDXFarray*	pobArray;
		CPointF		ptsXY, ptsUV;
		size_t		i, nCnt;
		BOOL		bSeqXY = TRUE, bSeqUV = TRUE;
		do {
			if ( bSeqXY ) {
				pDataXY = pChainXY->GetSeqData(posXY);
				if ( !posXY )
					posXY = pChainXY->GetFirstPosition();
				ptsXY = pDataXY->GetStartCutterPoint();
			}
			if ( bSeqUV ) {
				pDataUV = pChainUV->GetSeqData(posUV);
				if ( !posUV )
					posUV = pChainUV->GetFirstPosition();
				ptsUV = pDataUV->GetStartCutterPoint();
			}
			// 一時停止点検索
			if ( bSeqUV && g_mpPause.Lookup(ptsUV, pobArray) ) {
				// XY軸で座標生成して
				nCnt = pDataXY->SetVectorPoint(vptXY, GetDbl(MKWI_DBL_ELLIPSE));
				// その数だけUV座標を登録
				ptsUV = pDataUV->GetStartMakePoint();
				for ( i=0; i<nCnt; i++ )
					vptUV.push_back(ptsUV);
				_AddMakeGdata(vptXY, vptUV);
				pDataXY->SetMakeFlg();
				// 次のﾃﾞｰﾀ読み込み
				bSeqXY = posXY==posXYb ? FALSE : TRUE;	// 読む
				bSeqUV = FALSE;							// 読まない
				nXYUV = 1;
			}
			else if ( bSeqXY && g_mpPause.Lookup(ptsXY, pobArray) ) {
				nCnt = pDataUV->SetVectorPoint(vptUV, GetDbl(MKWI_DBL_ELLIPSE));
				ptsXY = pDataXY->GetStartMakePoint();
				for ( i=0; i<nCnt; i++ )
					vptXY.push_back(ptsXY);
				_AddMakeGdata(vptXY, vptUV);
				pDataUV->SetMakeFlg();
				bSeqXY = FALSE;
				bSeqUV = posUV==posUVb ? FALSE : TRUE;
				nXYUV = 2;
			}
			else {
				bSeqXY = posXY==posXYb ? FALSE : TRUE;
				bSeqUV = posUV==posUVb ? FALSE : TRUE;
				// ｵﾌﾞｼﾞｪｸﾄ数が同じと仮定した上下異形状の座標登録
				pDataXY->SetWireHeteroData(pDataUV, vptXY, vptUV, GetDbl(MKWI_DBL_ELLIPSE));
				if ( vptXY.empty() ) {
					// 座標分割必要なし -> ｵﾌﾞｼﾞｪｸﾄ座標から直接生成
					_AddMakeGdata(pDataXY, pDataUV);
				}
				else {
					// 分割座標からG01生成
					_AddMakeGdata(vptXY, vptUV);
					pDataXY->SetMakeFlg();
					pDataUV->SetMakeFlg();
				}
				nXYUV = 0;
			}
			vptXY.clear();
			vptUV.clear();
		} while ( (bSeqXY || bSeqUV) && IsThread() );
		// 終点処理
		switch ( nXYUV ) {
		case 1:
			// 残ったUVﾃﾞｰﾀを処理	
			nCnt = pDataUV->SetVectorPoint(vptUV, GetDbl(MKWI_DBL_ELLIPSE));
			ptsXY = pDataXY->GetEndMakePoint();		// 終点
			for ( i=0; i<nCnt; i++ )
				vptXY.push_back(ptsXY);
			_AddMakeGdata(vptXY, vptUV);
			pDataUV->SetMakeFlg();
			break;
		case 2:
			// 残ったXYﾃﾞｰﾀを処理
			nCnt = pDataXY->SetVectorPoint(vptXY, GetDbl(MKWI_DBL_ELLIPSE));
			ptsUV = pDataUV->GetEndMakePoint();
			for ( i=0; i<nCnt; i++ )
				vptUV.push_back(ptsUV);
			_AddMakeGdata(vptXY, vptUV);
			pDataXY->SetMakeFlg();
			break;
		}
	}
	else if ( nCntXY == nCntUV ) {
		do {
			pDataXY = pChainXY->GetSeqData(posXY);
			pDataUV = pChainUV->GetSeqData(posUV);
			if ( !posXY )
				posXY = pChainXY->GetFirstPosition();
			if ( !posUV )
				posUV = pChainUV->GetFirstPosition();
			// 上下異形状の座標登録
			pDataXY->SetWireHeteroData(pDataUV, vptXY, vptUV, GetDbl(MKWI_DBL_ELLIPSE));
			if ( vptXY.empty() ) {
				// 座標分割必要なし -> ｵﾌﾞｼﾞｪｸﾄ座標から直接生成
				_AddMakeGdata(pDataXY, pDataUV);
			}
			else {
				// 分割座標からG01生成
				_AddMakeGdata(vptXY, vptUV);
				pDataXY->SetMakeFlg();
				pDataUV->SetMakeFlg();
			}
			vptXY.clear();
			vptUV.clear();
		} while ( posXY!=posXYb && IsThread() );
	}
	else if ( nCntXY==1 && pChainXY->GetHead()->GetMakeType()==DXFCIRCLEDATA ) {
		CDXFcircle*	pCircle = static_cast<CDXFcircle*>(pChainXY->GetHead());
		// 分割数の予想値
		size_t	n = (size_t)ceil(PI2*pCircle->GetR() / GetDbl(MKWI_DBL_ELLIPSE));
		// 分割予想数をｵﾌﾞｼﾞｪｸﾄ長さで均等割
		pChainUV->SetVectorPoint(posUV, vptUV, n);
		// 均等割で得られた分割数で円を分割
		n = vptUV.size();		// 端数が出るので分割数を再取得
		pChainXY->SetVectorPoint(posXY, vptXY, n);
		// 座標生成
		_AddMakeGdata(vptXY, vptUV);
		// 生成済みﾌﾗｸﾞ
		pChainXY->SetMakeFlags();
		pChainUV->SetMakeFlags();
	}
	else if ( nCntUV==1 && pChainUV->GetHead()->GetMakeType()==DXFCIRCLEDATA ) {
		CDXFcircle*	pCircle = static_cast<CDXFcircle*>(pChainUV->GetHead());
		size_t	n = (size_t)ceil(PI2*pCircle->GetR() / GetDbl(MKWI_DBL_ELLIPSE));
		pChainXY->SetVectorPoint(posXY, vptXY, n);
		n = vptXY.size();
		pChainUV->SetVectorPoint(posUV, vptUV, n);
		_AddMakeGdata(vptXY, vptUV);
		pChainXY->SetMakeFlags();
		pChainUV->SetMakeFlags();
	}
	else if ( nCntXY > nCntUV ) {
		// ｵﾌﾞｼﾞｪｸﾄの数が多い（複雑な形状を想定）を基準に分割を行う
		pChainXY->SetVectorPoint(posXY, vptXY, GetDbl(MKWI_DBL_ELLIPSE));
		pChainUV->SetVectorPoint(posUV, vptUV, vptXY.size());
		_AddMakeGdata(vptXY, vptUV);
		pChainXY->SetMakeFlags();
		pChainUV->SetMakeFlags();
	}
	else {
		pChainUV->SetVectorPoint(posUV, vptUV, GetDbl(MKWI_DBL_ELLIPSE));
		pChainXY->SetVectorPoint(posXY, vptXY, vptUV.size());
		_AddMakeGdata(vptXY, vptUV);
		pChainXY->SetMakeFlags();
		pChainUV->SetMakeFlags();
	}

	return IsThread();
}

BOOL MakeLoopWireAdd_ChainList(CDXFshape* pShape, CDXFchain* pChain)
{
	POSITION	pos1, pos2;
	CDXFdata*	pData;

	// 加工指示に伴う開始位置や方向等の設定
	pos1 = pos2 = pShape->GetFirstChainPosition();
	ASSERT(pos1);
	pData = pChain->GetAt(pos1);

	// 切削ﾃﾞｰﾀ生成
	_AddMoveGdata(pData);
	// 開始ﾎﾟｼﾞｼｮﾝからﾙｰﾌﾟ
	do {
		pData = pChain->GetSeqData(pos1);
		_AddMakeGdata(pData);
		if ( !pos1 )
			pos1 = pChain->GetFirstPosition();
	} while ( pos1!=pos2 && IsThread() ); 
	// 最後に生成したﾃﾞｰﾀを待避
	CDXFdata::ms_pData = pData;

	return IsThread();
}

BOOL MakeLoopWireAdd_EulerMap(CDXFshape* pShape)
{
	BOOL		bEuler = FALSE;
	CDXFmap*	pEuler = pShape->GetShapeMap();
	ASSERT( pEuler );
	// １回目の生成処理
	if ( !MakeLoopWireAdd_EulerMap_Make( pShape, pEuler, bEuler ) )
		return FALSE;
	if ( bEuler )
		return TRUE;	// １回目で全て生成完了

	// 生成漏れのﾃﾞｰﾀ処理
	int			i;
	CDXFdata*	pData;
	CDXFdata*	pDataResult;
	CDXFarray*	pArray;
	CDXFmap		mpLeak;
	float		dGap, dGapMin;
	CPointF		pt, ptKey;
	while ( IsThread() ) {
		// 現在位置に近いｵﾌﾞｼﾞｪｸﾄ検索
		pDataResult = NULL;
		dGapMin = FLT_MAX;
		pt = CDXFdata::ms_pData->GetEndCutterPoint();
		PMAP_FOREACH(ptKey, pArray, pEuler)
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
		END_FOREACH
		if ( !pDataResult || !IsThread() )	// ﾙｰﾌﾟ終了条件
			break;
		// 仮座標ﾏｯﾌﾟを生成
		mpLeak.RemoveAll();
		mpLeak.SetPointMap(pDataResult);	// SetMakePointMap() ではない
		pDataResult->SetSearchFlg();
		for ( i=0; i<pDataResult->GetPointNumber() && IsThread(); i++ ) {
			pt = pDataResult->GetNativePoint(i);
			if ( !MakeLoopWireAdd_EulerMap_Search(pt, pEuler, &mpLeak) )
				return FALSE;
		}
		if ( !IsThread() )
			return FALSE;
		// ２回目以降の生成処理
		bEuler = FALSE;
		if ( !MakeLoopWireAdd_EulerMap_Make( pShape, &mpLeak, bEuler ) )
			return FALSE;
	}

	return IsThread();
}

BOOL MakeLoopWireAdd_EulerMap_Make(CDXFshape* pShape, CDXFmap* pEuler, BOOL& bEuler)
{
	// MakeLoopEulerAdd() 参考
	BOOL		bReverse = FALSE;
	POSITION	pos;
	CPointF		pt;
	CDXFdata*		pData;
	CDXFdata*		pDataFix;
	CDXFarray*		pArray;
	CDXFworking*	pWork;
	CDXFchain		ltEuler;
	const CPointF	ptOrg(CDXFdata::ms_ptOrg);

	// 開始位置指示
	tie(pWork, pDataFix) = pShape->GetStartObject();
	const	CPointF		ptNow( pDataFix ?
		static_cast<CDXFworkingStart*>(pWork)->GetStartPoint() :	// 現在位置を更新
		CDXFdata::ms_pData->GetEndCutterPoint()+ptOrg );

	// この座標ﾏｯﾌﾟが一筆書き要件を満たしているか、かつ、
	// 現在位置（加工開始位置）に近いところから一時集合を生成
	tie(bEuler, pArray, pt) = pEuler->IsEulerRequirement(ptNow);
	if ( !IsThread() )
		return FALSE;
	pt -= ptOrg;
	ASSERT( pArray );
	ASSERT( !pArray->IsEmpty() );

	// --- 一筆書きの生成(再帰呼び出しによる木構造解析)
	if ( !MakeLoopWireAdd_with_one_stroke(pEuler, bEuler, TRUE, pt, pArray, ltEuler) ) {
		// 一筆書きできるハズやけど失敗したら条件緩和してやり直し
		bEuler = FALSE;
		MakeLoopWireAdd_with_one_stroke(pEuler, bEuler, TRUE, pt, pArray, ltEuler);
	}
	ASSERT( !ltEuler.IsEmpty() );

	// 方向指示および生成順のﾁｪｯｸ
	tie(pWork, pDataFix) = pShape->GetDirectionObject();
	if ( pDataFix ) {
		// 方向指示がltEulerに含まれる場合だけﾁｪｯｸ
		PLIST_FOREACH(pData, &ltEuler)
			if ( pDataFix == pData ) {
				CPointF	pts( static_cast<CDXFworkingDirection*>(pWork)->GetStartPoint() - ptOrg ),
						pte( static_cast<CDXFworkingDirection*>(pWork)->GetArrowPoint() - ptOrg );
				bReverse = pData->IsDirectionPoint(pts, pte);
				break;
			}
		END_FOREACH
	}

	// 生成順序の設定
	ltEuler.SetLoopFunc(NULL, bReverse, FALSE);

	BOOL	bNext = FALSE;
	// 切削ﾃﾞｰﾀまでの移動
	pData = ltEuler.GetFirstData();
	_AddMoveGdata(pData);
	// 切削ﾃﾞｰﾀ生成
	for ( pos=ltEuler.GetFirstPosition(); pos && IsThread(); ) {
		pData = ltEuler.GetSeqData(pos);
		if ( pData ) {
			if ( bNext ) {
				_AddMoveGdata(pData);
				bNext = FALSE;
			}
			_AddMakeGdata(pData);
		}
		else
			bNext = TRUE;
	}
	// 最後に生成したﾃﾞｰﾀを待避
	CDXFdata::ms_pData = pData;

	return IsThread();
}

BOOL MakeLoopWireAdd_EulerMap_Search
	(const CPointF& ptKey, CDXFmap* pOrgMap, CDXFmap* pResultMap)
{
	int			i, j;
	CPointF		pt;
	CDXFarray*	pobArray;
	CDXFdata*	pData;

	// 座標をｷｰに全体のﾏｯﾌﾟ検索
	if ( pOrgMap->Lookup(const_cast<CPointF&>(ptKey), pobArray) ) {
		for ( i=0; i<pobArray->GetSize() && IsThread(); i++ ) {
			pData = pobArray->GetAt(i);
			if ( !pData->IsMakeFlg() && !pData->IsSearchFlg() ) {
				pResultMap->SetPointMap(pData);		// SetMakePointMap() ではない
				pData->SetSearchFlg();
				// ｵﾌﾞｼﾞｪｸﾄの端点をさらに検索
				for ( j=0; j<pData->GetPointNumber() && IsThread(); j++ ) {
					pt = pData->GetNativePoint(j);
					if ( ptKey != pt ) {
						if ( !MakeLoopWireAdd_EulerMap_Search(pt, pOrgMap, pResultMap) )
							break;
					}
				}
			}
		}
	}

	return IsThread();
}

BOOL MakeLoopWireAdd_with_one_stroke
	(const CDXFmap* pEuler, BOOL bEuler, BOOL bMakeShape, 
		const CPointF& pt, const CDXFarray* pArray, CDXFlist& ltEuler)
{
	INT_PTR		i;
	const INT_PTR	nLoop = pArray->GetSize();
	CDXFdata*	pData;
	CDXFarray*	pNextArray;
	CPointF		ptNext;
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
			if ( MakeLoopWireAdd_with_one_stroke(pEuler, bEuler, bMakeShape, ptNext, pNextArray, ltEuler) )
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

//	AddCustomWireCode() から呼び出し
class CMakeCustomWireCode : public CMakeCustomCode	// MakeCustomCode.h
{
public:
	CMakeCustomWireCode() :
				CMakeCustomCode(g_pDoc, NULL, g_pMakeOpt) {
		static	LPCTSTR	szCustomCode[] = {
			"ProgNo", "WorkDepth", "TaperMode",
			"G90orG91", "G92_INITIAL", "G92X", "G92Y",
			"G0XY_INITIAL"
		};
		// ｵｰﾀﾞｰ追加
		m_strOrderIndex.AddElement(SIZEOF(szCustomCode), szCustomCode);
	}

	CString	ReplaceCustomCode(const string& str) {
		extern	const	DWORD	g_dwSetValFlags[];
		int		nTestCode;
		float	dValue[VALUESIZE];
		CString	strResult;

		// 基底ｸﾗｽ呼び出し
		tie(nTestCode, strResult) = CMakeCustomCode::ReplaceCustomCode(str);
		if ( !strResult.IsEmpty() )
			return strResult;

		// 派生replace
		switch ( nTestCode ) {
		case 0:		// ProgNo
			if ( GetFlg(MKWI_FLG_PROG) )
				strResult.Format(IDS_MAKENCD_PROG,
					GetFlg(MKWI_FLG_PROGAUTO) ? ::GetRandom(1,9999) : GetNum(MKWI_NUM_PROG));
			break;
		case 1:		// WorkDepth
			strResult.Format(IDS_MAKENCD_FORMAT, GetDbl(MKWI_DBL_DEPTH));
			break;
		case 2:		// TaperMode
			strResult = GetStr(MKWI_STR_TAPERMODE);
			break;
		case 3:		// G90orG91
			strResult = CNCMakeBase::MakeCustomString(GetNum(MKWI_NUM_G90)+90);
			break;
		case 4:		// G92_INITIAL
			dValue[NCA_X] = GetDbl(MKWI_DBL_G92X);
			dValue[NCA_Y] = GetDbl(MKWI_DBL_G92Y);
			strResult = CNCMakeBase::MakeCustomString(92, NCD_X|NCD_Y, dValue, FALSE);
			break;
		case 5:		// G92X
		case 6:		// G92Y
			nTestCode -= 3;
			dValue[nTestCode] = GetDbl(MKWI_DBL_G92X+nTestCode);
			strResult = CNCMakeBase::MakeCustomString(-1, g_dwSetValFlags[nTestCode], dValue, FALSE);
			break;
		case 7:		// G0XY_INITIAL
			dValue[NCA_X] = GetDbl(MKWI_DBL_G92X);
			dValue[NCA_Y] = GetDbl(MKWI_DBL_G92Y);
			strResult = CNCMakeBase::MakeCustomString(0, NCD_X|NCD_Y, dValue);
			break;
		default:
			strResult = str.c_str();
		}

		return strResult;
	}
};

void AddCustomWireCode(const CString& strFileName)
{
	CString	strBuf, strResult;
	CMakeCustomWireCode			custom;
	string	str, strTok;
	tokenizer<custom_separator>	tokens(str);

	try {
		CStdioFile	fp(strFileName,
			CFile::modeRead | CFile::shareDenyWrite | CFile::typeText);
		while ( fp.ReadString(strBuf) && IsThread() ) {
			str = strBuf;
			tokens.assign(str);
			strResult.Empty();
			BOOST_FOREACH(strTok, tokens) {
				strResult += custom.ReplaceCustomCode(strTok);
			}
			if ( !strResult.IsEmpty() )
				_AddMakeWireStr(strResult);
		}
	}
	catch (CFileException* e) {
		AfxMessageBox(IDS_ERR_MAKECUSTOM, MB_OK|MB_ICONEXCLAMATION);
		e->Delete();
		// このｴﾗｰは正常ﾘﾀｰﾝ(警告のみ)
	}
}

//////////////////////////////////////////////////////////////////////

BOOL CreateShapeThread(void)
{
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

	return IsThread();
}

CDXFcircle* SetAWFandPAUSEdata(void)
{
	INT_PTR		i, j, k, nLoop1 = g_pDoc->GetLayerCnt(), nLoop2;
	CLayerData*	pLayer;
	CDXFshape*	pShape;
	CDXFdata*	pData;
	CDXFcircle*	pDataResult = NULL;
	CDXFcircle*	pCircle;
	CPointF		ptOrg(0, 0),				// 加工原点
				pt;
	float		dGap, dGapMin = FLT_MAX;	// 指定点との距離
	CSortArray<CObArray, CDXFcircle*>	obAWFdata;		// AWFﾃﾞｰﾀ仮置き場

	// AWFと一時停止ﾎﾟｲﾝﾄの検索
	for ( i=0; i<nLoop1 && IsThread(); i++ ) {
		pLayer = g_pDoc->GetLayerData(i);
		if ( !pLayer->IsMakeTarget() )
			continue;
		nLoop2 = pLayer->GetDxfSize();
		for ( j=0; j<nLoop2 && IsThread(); j++ ) {
			pData = pLayer->GetDxfData(j);
			switch ( pData->GetMakeType() ) {
			case DXFPOINTDATA:
				pData->OrgTuning(FALSE);
				g_mpPause.SetMakePointMap(pData);
				break;
			case DXFCIRCLEDATA:
				pCircle = static_cast<CDXFcircle*>(pData);
				if ( g_pMakeOpt->IsAWFcircle(pCircle->GetMakeR()) ) {
					// AWFﾃﾞｰﾀの抽出
					pData->SetSearchFlg();
					pData->ChangeMakeType(DXFPOINTDATA);
					obAWFdata.Add(pCircle);
					// 親を生成済み
					pData->GetParentMap()->SetShapeFlag(DXFMAPFLG_MAKE);
				}
				break;
			}
		}
	}
	if ( obAWFdata.IsEmpty() )
		return NULL;

	// 形状の内側か外側かの振り分け
	for ( i=0; i<obAWFdata.GetSize() && IsThread(); i++ ) {
		pCircle = obAWFdata[i];
		pt = pCircle->GetCenter();
		for ( j=0; j<nLoop1 && IsThread(); j++ ) {
			pLayer = g_pDoc->GetLayerData(j);
			if ( !pLayer->IsMakeTarget() )
				continue;
			nLoop2 = pLayer->GetShapeSize();
			for ( k=0; k<nLoop2 && IsThread(); k++ ) {
				pShape = pLayer->GetShapeData(k);
				if ( !pShape->IsMakeFlg() && pShape->GetShapeAssemble()!=DXFSHAPE_EXCLUDE &&
							pShape->GetMaxRect().PtInRect(pt) ) {
					g_obAWFinside.Add(pCircle);
					break;
				}
			}
			if ( k >= nLoop2 )
				g_obAWFoutside.Add(pCircle);
		}
	}

	// 外側を優先的に、一番近いAWFﾎﾟｲﾝﾄを検索
	CSortArray<CObArray, CDXFcircle*>* pArray = g_obAWFoutside.IsEmpty() ?
		&g_obAWFinside : &g_obAWFoutside;
	for ( i=0; i<pArray->GetSize() && IsThread(); i++ ) {
		pCircle = pArray->GetAt(i);
		dGap = pCircle->GetEdgeGap(ptOrg);
		if ( dGap < dGapMin ) {
			dGapMin = dGap;
			pDataResult = pCircle;
		}
	}

	return pDataResult;
}

CDXFshape* GetNearPointWire(const CPointF& pt)
{
	CLayerData*	pLayer;
	CDXFshape*	pShape;
	CDXFshape*	pShapeResult = NULL;
	INT_PTR		i, j, nLoop1 = g_pDoc->GetLayerCnt(), nLoop2;
	float		dGap, dGapMin = FLT_MAX;

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

CDXFcircle*	GetInsideAWF(const CDXFshape* pShape)
{
	CRectF	rc(pShape->GetMaxRect());
	CDXFcircle*	pCircle;

	for ( int i=0; i<g_obAWFinside.GetSize() && IsThread(); i++ ) {
		pCircle = g_obAWFinside[i];
		if ( !pCircle->IsMakeFlg() && rc.PtInRect(pCircle->GetCenter()) )
			return pCircle;
	}

	return NULL;
}

CDXFcircle*	GetOutsideAWF(void)
{
	int		i;
	CPointF	pt(CDXFdata::ms_pData->GetEndCutterPoint());
	float	dGap, dGapMin = FLT_MAX;
	CDXFcircle*	pCircle;
	CDXFcircle*	pDataResult = NULL;

	for ( i=0; i<g_obAWFoutside.GetSize() && IsThread(); i++ ) {
		pCircle = g_obAWFoutside[i];
		if ( !pCircle->IsMakeFlg() ) {
			dGap = pCircle->GetEdgeGap(pt);
			if ( dGap < dGapMin ) {
				dGapMin = dGap;
				pDataResult = pCircle;
			}
		}
	}

	if ( pDataResult )
		return pDataResult;

	for ( i=0; i<g_obAWFinside.GetSize() && IsThread(); i++ ) {
		pCircle = g_obAWFinside[i];
		if ( !pCircle->IsMakeFlg() ) {
			dGap = pCircle->GetEdgeGap(pt);
			if ( dGap < dGapMin ) {
				dGapMin = dGap;
				pDataResult = pCircle;
			}
		}
	}

	return pDataResult;
}

//////////////////////////////////////////////////////////////////////

// NC生成のｸﾞﾛｰﾊﾞﾙ変数初期化(後始末)ｽﾚｯﾄﾞ
UINT MakeWire_AfterThread(LPVOID)
{
	g_csMakeAfter.Lock();

#ifdef _DEBUG
	printf("MakeWire_AfterThread() Start\n");
#endif
	for ( int i=0; i<g_obMakeData.GetSize(); i++ )
		delete	g_obMakeData[i];
	g_obMakeData.RemoveAll();
	g_obAWFinside.RemoveAll();
	g_obAWFoutside.RemoveAll();
	g_mpPause.RemoveAll();

	if ( g_pMakeOpt )
		delete	g_pMakeOpt;

	g_csMakeAfter.Unlock();

	return 0;
}
