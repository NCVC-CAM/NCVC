// NCDoc.cpp : CNCDoc クラスの動作の定義を行います。
//

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "NCChild.h"
#include "MCOption.h"
#include "NCdata.h"
#include "NCDoc.h"
#include "NCViewTab.h"
#include "NCInfoTab.h"
#include "NCListView.h"
#include "NCInfoView.h"
#include "NCWorkDlg.h"
#include "ThreadDlg.h"
#include "DXFMakeOption.h"
#include "DXFMakeClass.h"
#include "MakeDXFDlg.h"
#include "DXFkeyword.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif
//#define	_DEBUGOLD
#undef	_DEBUGOLD

// 文字列検査用(TH_NCRead.cpp, NCMakeClass.cpp からも参照)
extern	LPCTSTR	g_szGdelimiter = "GSMOF";
extern	LPCTSTR	g_szNdelimiter = "XYZRIJKPLDH";
extern	LPTSTR	g_pszDelimiter;		// g_szGdelimiter[] + g_szNdelimiter[] (NCVC.cppで生成)

// 指定された値のﾌﾗｸﾞ
extern	const	DWORD	g_dwSetValFlags[] = {
	NCD_X, NCD_Y, NCD_Z, 
	NCD_R, NCD_I, NCD_J, NCD_K,
	NCD_P, NCD_L,
	NCD_D, NCD_H
};

/////////////////////////////////////////////////////////////////////////////
// CNCDoc

IMPLEMENT_DYNCREATE(CNCDoc, CDocument)

BEGIN_MESSAGE_MAP(CNCDoc, CDocument)
	//{{AFX_MSG_MAP(CNCDoc)
	ON_UPDATE_COMMAND_UI(ID_FILE_NCINSERT, OnUpdateFileInsert)
	ON_COMMAND(ID_FILE_NCINSERT, OnFileInsert)
	ON_COMMAND(ID_FILE_NCD2DXF, OnFileNCD2DXF)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE, OnUpdateFileSave)
	ON_UPDATE_COMMAND_UI(ID_NCVIEW_WORKRECT, OnUpdateWorkRect)
	ON_COMMAND(ID_NCVIEW_WORKRECT, OnWorkRect)
	ON_UPDATE_COMMAND_UI(ID_NCVIEW_MAXRECT, OnUpdateMaxRect)
	ON_COMMAND(ID_NCVIEW_MAXRECT, OnMaxRect)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNCDoc クラスの構築/消滅

CNCDoc::CNCDoc()
{
	m_fError = TRUE;	// 初期状態はｴﾗｰ
	m_dMove[0] = m_dMove[1] = 0.0;
	m_dCutTime = -1.0;
	m_nTraceStart = m_nTraceDraw = 0;
	m_bWorkRect = m_bMaxRect = FALSE;
	m_hCutcalc = NULL;
	m_bCutcalc = m_bCorrect = FALSE;
	// ﾜｰｸ座標系取得
	const CMCOption* pMCopt = AfxGetNCVCApp()->GetMCOption();
	for ( int i=0; i<WORKOFFSET; i++ )
		m_ptNcWorkOrg[i] = pMCopt->GetWorkOffset(i);
	m_ptNcWorkOrg[i] = 0.0;		// G92の初期化
	m_nWorkOrg = pMCopt->GetModalSetting(MODALGROUP2);		// G54～G59
	if ( m_nWorkOrg<0 || SIZEOF(m_ptNcWorkOrg)<=m_nWorkOrg )
		m_nWorkOrg = 0;
	// ｵﾌﾞｼﾞｪｸﾄ矩形の初期化
	m_rcMax.SetRectMinimum();
	// 増分割り当てサイズ
	m_obBlock.SetSize(0, 1024);
	m_obGdata.SetSize(0, 1024);
}

CNCDoc::~CNCDoc()
{
	int	i;
	for ( i=0; i<m_obGdata.GetSize(); i++ )
		delete	m_obGdata[i];
	for ( i=0; i<m_obBlock.GetSize(); i++ )
		delete	m_obBlock[i];
	// 一時展開のﾏｸﾛﾌｧｲﾙを消去
	for ( i=0; i<m_obMacroFile.GetSize(); i++ ) {
#ifdef _DEBUG
		BOOL bResult = ::DeleteFile(m_obMacroFile[i]);
		if ( !bResult ) {
			g_dbg.printf("Delete File=%s", m_obMacroFile[i]);
			::NC_FormatMessage();
		}
#else
		::DeleteFile(m_obMacroFile[i]);
#endif
	}
}

/////////////////////////////////////////////////////////////////////////////
// ﾕｰｻﾞﾒﾝﾊﾞ関数

BOOL CNCDoc::RouteCmdToAllViews
	(CView* pActiveView, UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) 
{
	CView*	pView;

	for ( POSITION pos=GetFirstViewPosition(); pos; ) {
		pView = GetNextView(pos);
		// ｱｸﾃｨﾌﾞﾋﾞｭｰか CNC[View|Info][*] ならｺﾏﾝﾄﾞﾙｰﾃｨﾝｸﾞしない
		// CNC[View|Info][*] は CNC[View|Info]Tab からｱｸﾃｨﾌﾞﾋﾞｭｰに対してだけｺﾏﾝﾄﾞﾙｰﾃｨﾝｸﾞ
		if ( pView!=pActiveView &&
				( pView->IsKindOf(RUNTIME_CLASS(CNCListView)) ||
				  pView->IsKindOf(RUNTIME_CLASS(CNCInfoTab)) ||
				  pView->IsKindOf(RUNTIME_CLASS(CNCViewTab)) ) ) {
			if ( pView->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo) )
				return TRUE;
		}
	}
	return FALSE;
}

CNCdata* CNCDoc::DataOperation
	(const CNCdata* pDataSrc, LPNCARGV lpArgv, int nIndex/*=-1*/, ENNCOPERATION enOperation/*=NCADD*/)
{
	CMCOption*	pOpt = AfxGetNCVCApp()->GetMCOption();
	CNCdata*	pData = NULL;
	CNCblock*	pBlock;
	CPoint3D	pt( m_ptNcWorkOrg[m_nWorkOrg] + m_ptNcLocalOrg );
	int			i;

	// 例外ｽﾛｰは上位でｷｬｯﾁ
	if ( lpArgv->nc.nGtype == G_TYPE ) {
		switch ( lpArgv->nc.nGcode ) {
		case 0:		// 直線
		case 1:
			// ｵﾌﾞｼﾞｪｸﾄ生成
			pData = new CNCline(pDataSrc, lpArgv, pt);
			SetMaxRect(pData);		// 最小・最大値の更新
			if ( lpArgv->nc.dwValFlags & NCD_CORRECT )
				m_bCorrect = TRUE;
			break;
		case 2:		// 円弧
		case 3:
			pData = new CNCcircle(pDataSrc, lpArgv, pt);
			SetMaxRect(pData);
			if ( lpArgv->nc.dwValFlags & NCD_CORRECT )
				m_bCorrect = TRUE;
			break;
		case 81:	// 固定ｻｲｸﾙ
		case 82:
		case 83:
		case 84:
		case 85:
		case 86:
		case 87:
		case 88:
		case 89:
			pData = new CNCcycle(pDataSrc, lpArgv, pt, pOpt->GetFlag(MC_FLG_L0CYCLE));
			SetMaxRect(pData);
			break;
		case 10:	// ﾃﾞｰﾀ設定
			if ( lpArgv->nc.dwValFlags & (NCD_P|NCD_R) ) {	// G10P_R_
				// 工具情報の追加
				if ( pOpt->AddTool((int)lpArgv->nc.dValue[NCA_P], lpArgv->nc.dValue[NCA_R], lpArgv->bAbs) ) {
					pData = new CNCdata(pDataSrc, lpArgv, pt);
				}
				else {
					i = lpArgv->nc.nLine;
					if ( 0<=i && i<m_obBlock.GetSize() ) {	// 保険
						pBlock = GetNCblock(i);
						pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_G10ADDTOOL);
					}
				}
				break;
			}
			else if ( lpArgv->nc.dwValFlags & NCD_P ) {
				// ﾜｰｸ座標系の設定
				int nWork = (int)lpArgv->nc.dValue[NCA_P];
				if ( nWork>=0 && nWork<WORKOFFSET ) { 
					for ( i=0; i<NCXYZ; i++ ) {
						if ( lpArgv->nc.dwValFlags & g_dwSetValFlags[i] )
							m_ptNcWorkOrg[nWork][i] += lpArgv->nc.dValue[i];
					}
					pData = new CNCdata(pDataSrc, lpArgv, pt);
					break;
				}
			}
			// P値認識不能
			i = lpArgv->nc.nLine;
			if ( 0<=i && i<m_obBlock.GetSize() ) {
				pBlock = GetNCblock(i);
				pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_ORDER);
			}
			break;
		case 52:	// ﾛｰｶﾙ座標設定
			for ( i=0; i<NCXYZ; i++ ) {
				if ( lpArgv->nc.dwValFlags & g_dwSetValFlags[i] )
					m_ptNcLocalOrg[i] = lpArgv->nc.dValue[i];
			}
			pData = new CNCdata(pDataSrc, lpArgv, pt);
			break;
		case 92:
			// ﾛｰｶﾙ座標系ｸﾘｱとG92値取得
			for ( i=0; i<NCXYZ; i++ ) {
				m_ptNcLocalOrg[i] = 0.0;
				pt[i] = lpArgv->nc.dwValFlags & g_dwSetValFlags[i] ?
					lpArgv->nc.dValue[i] : pDataSrc->GetEndValue(i);
			}
			// 現在位置 - G92値 で、G92座標系原点を計算
			m_ptNcWorkOrg[WORKOFFSET] = pDataSrc->GetEndPoint() - pt;
			m_nWorkOrg = WORKOFFSET;	// G92座標系選択
			pt = m_ptNcWorkOrg[WORKOFFSET];
			// through
		default:	// G04 ...
			pData = new CNCdata(pDataSrc, lpArgv, pt);
		}
	}	// end of G_TYPE
	else {
		// M_TYPE, O_TYPE, etc.
		pData = new CNCdata(pDataSrc, lpArgv, pt);
	}

	// ｵﾌﾞｼﾞｪｸﾄ登録
	switch ( enOperation ) {
	case NCADD:
		m_obGdata.Add(pData);
		break;
	case NCINS:
		m_obGdata.InsertAt(nIndex, pData);
		break;
	case NCMOD:
		RemoveAt(nIndex, 1);
		m_obGdata.SetAt(nIndex, pData);
		break;
	}

	// 行番号にﾘﾝｸしたｴﾗｰﾌﾗｸﾞの設定
	UINT	nError = pData->GetNCObjErrorCode();
	if ( nError > 0 ) {
		i = pData->GetBlockLineNo();
		if ( 0<=i && i<m_obBlock.GetSize() ) {	// 保険
			pBlock = GetNCblock(i);
			pBlock->SetNCBlkErrorCode(nError);
		}
	}

	return pData;
}

void CNCDoc::StrOperation(LPCSTR pszTmp, int nIndex/*=-1*/, ENNCOPERATION enOperation/*=NCADD*/)
{
	int	nBuf = (int)strcspn(pszTmp, g_pszDelimiter);
	CString		str(pszTmp);
	CNCblock*	pBlock = NULL;

	// 例外ｽﾛｰは上位でｷｬｯﾁ
	pBlock = new CNCblock(str.Left(nBuf).Trim(), str.Mid(nBuf).Trim());
	switch ( enOperation ) {
	case NCADD:
		m_obBlock.Add(pBlock);
		break;
	case NCINS:
		m_obBlock.InsertAt(nIndex, pBlock);
		break;
	case NCMOD:
		delete	m_obBlock[nIndex];
		m_obBlock.SetAt(nIndex, pBlock);
		break;
	}
}

void CNCDoc::RemoveAt(int nIndex, int nCnt)
{
	nCnt = min(nCnt, m_obGdata.GetSize() - nIndex);
	CNCdata*	pData;
	for ( int i=nIndex; i<nIndex+nCnt; i++ ) {
		pData = m_obGdata[i];
		if ( pData->GetGtype() == G_TYPE ) {
			switch ( pData->GetType() ) {
			case NCDLINEDATA:
				m_dMove[pData->GetGcode()] -= pData->GetCutLength();
				break;
			case NCDCYCLEDATA:
				m_dMove[0] -= static_cast<CNCcycle *>(pData)->GetCycleMove();
				m_dMove[1] -= pData->GetCutLength();
				break;
			case NCDARCDATA:
				m_dMove[1] -= pData->GetCutLength();
				break;
			}
		}
		delete pData;
	}
	m_obGdata.RemoveAt(nIndex, nCnt);
}

void CNCDoc::RemoveStr(int nIndex, int nCnt)
{
	nCnt = min(nCnt, m_obBlock.GetSize() - nIndex);
	for ( int i=nIndex; i<nIndex+nCnt; i++ )
		delete	m_obBlock[i];
	m_obBlock.RemoveAt(nIndex, nCnt);
}

void CNCDoc::AllChangeFactor(double f) const
{
	f *= LOMETRICFACTOR;
	for ( int i=0; i<GetNCsize(); i++ )
		GetNCdata(i)->DrawTuning(f);
}

void CNCDoc::AllChangeFactorXY(double f) const
{
	f *= LOMETRICFACTOR;
	for ( int i=0; i<GetNCsize(); i++ )
		GetNCdata(i)->DrawTuningXY(f);
}

void CNCDoc::AllChangeFactorXZ(double f) const
{
	f *= LOMETRICFACTOR;
	for ( int i=0; i<GetNCsize(); i++ )
		GetNCdata(i)->DrawTuningXZ(f);
}

void CNCDoc::AllChangeFactorYZ(double f) const
{
	f *= LOMETRICFACTOR;
	for ( int i=0; i<GetNCsize(); i++ )
		GetNCdata(i)->DrawTuningYZ(f);
}

void CNCDoc::CreateCutcalcThread(void)
{
	WaitCalcThread();	// 現在計算中なら中断

	LPNCVCTHREADPARAM	pParam = new NCVCTHREADPARAM;
	pParam->pParent = NULL;
	pParam->pDoc = this;
	pParam->wParam = NULL;		// CNCInfoView1
	pParam->lParam = NULL;

	CView*	pView;
	for ( POSITION pos=GetFirstViewPosition(); pos; ) {
		pView = GetNextView(pos);
		if ( pView->IsKindOf(RUNTIME_CLASS(CNCInfoView1)) ) {
			pParam->wParam = (WPARAM)pView;
			break;
		}
	}

	CWinThread* pThread = AfxBeginThread(CuttimeCalc_Thread, pParam,
			THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
	m_hCutcalc = ::NCVC_DuplicateHandle(pThread->m_hThread);
	if ( !m_hCutcalc )
		::NCVC_CriticalErrorMsg(__FILE__, __LINE__);
	m_bCutcalc = TRUE;
	pThread->ResumeThread();
}

void CNCDoc::WaitCalcThread(void)
{
	m_bCutcalc = FALSE;
	if ( m_hCutcalc ) {
#ifdef _DEBUG
		CMagaDbg	dbg("CNCDoc::WaitCalcThread()", DBG_BLUE);
		if ( ::WaitForSingleObject(m_hCutcalc, INFINITE) == WAIT_FAILED ) {
			dbg.printf("WaitForSingleObject() Fail!");
			::NC_FormatMessage();
		}
		else
			dbg.printf("WaitForSingleObject() OK");
		if ( !::CloseHandle(m_hCutcalc) ) {
			dbg.printf("CloseHandle() Fail!");
			::NC_FormatMessage();
		}
		else
			dbg.printf("CloseHandle() OK");
#else
		::WaitForSingleObject(m_hCutcalc, INFINITE);
		::CloseHandle(m_hCutcalc);
#endif
		m_hCutcalc = NULL;
	}
}

int CNCDoc::SearchBlockRegex(boost::regex& r, int nStart/*=0*/, BOOL bReverse/*=FALSE*/)
{
	int		i;

	if ( bReverse ) {
		for (i=nStart; i>=0; i--) {
			if ( regex_search((LPCTSTR)(GetNCblock(i)->GetStrGcode()), r) )
				return i;
		}
	}
	else {
		for (i=nStart; i<GetNCBlockSize(); i++) {
			if ( regex_search((LPCTSTR)(GetNCblock(i)->GetStrGcode()), r) )
				return i;
		}
	}

	return -1;
}

void CNCDoc::ClearBreakPoint(void)
{
	CNCblock*	pBlock;
	for (int i=0; i<GetNCBlockSize(); i++) {
		pBlock = GetNCblock(i);
		pBlock->SetBlockFlag(pBlock->GetBlockFlag() & ~NCF_BREAK, FALSE);
	}
}

BOOL CNCDoc::IncrementTrace(int& nTraceDraw)
{
	int			nMax = GetNCsize(), nLine1, nLine2;
	BOOL		bResult = FALSE, bBreakChk;
	CNCdata*	pData;

	m_csTraceDraw.Lock();
	if ( m_nTraceDraw > 0 ) {
		pData = GetNCdata(m_nTraceDraw-1);
		nLine1 = pData->GetBlockLineNo();		// ｲﾝｸﾘﾒﾝﾄ前のﾌﾞﾛｯｸ行
		bBreakChk = pData->GetGtype()==M_TYPE ? FALSE : TRUE;
	}
	else {
		nLine1 = 0;
		bBreakChk = FALSE;
	}
	// NCｵﾌﾞｼﾞｪｸﾄ単位のｲﾝｸﾘﾒﾝﾄ
	m_nTraceDraw++;
	if ( nMax < m_nTraceDraw ) {
		nTraceDraw = -1;
		m_nTraceDraw = nMax;
		m_csTraceDraw.Unlock();
		return FALSE;
	}
	pData = GetNCdata(m_nTraceDraw-1);
	// ｲﾝｸﾘﾒﾝﾄ後のｵﾌﾞｼﾞｪｸﾄがM_TYPEで、ｲﾝｸﾘﾒﾝﾄ前と同じﾌﾞﾛｯｸなら
	if ( pData->GetGtype()==M_TYPE && nLine1==pData->GetBlockLineNo() ) {
		// さらにｲﾝｸﾘﾒﾝﾄ
		m_nTraceDraw++;
		pData = GetNCdata(m_nTraceDraw-1);
	}
	nLine2 = pData->GetBlockLineNo();			// ｲﾝｸﾘﾒﾝﾄ後のﾌﾞﾛｯｸ行
	nTraceDraw = m_nTraceDraw;
	m_csTraceDraw.Unlock();

	// ﾌﾞﾚｲｸﾎﾟｲﾝﾄのﾁｪｯｸ
	if ( bBreakChk ) {
		while ( ++nLine1 <= nLine2 ) {
			if ( IsBreakPoint(nLine1) ) {
				bResult = TRUE;
				break;
			}
		}
	}
	else
		bResult = IsBreakPoint( GetNCdata(nTraceDraw-1)->GetBlockLineNo() );

	return bResult;
}

BOOL CNCDoc::SetLineToTrace(BOOL bStart, int nLine)
{
	for ( int i=nLine; i<GetNCBlockSize(); i++ ) {
		if ( GetNCblock(i)->GetBlockToNCdata() )
			break;
	}
	if ( i >= GetNCBlockSize() ) {
		m_csTraceDraw.Lock();
		m_nTraceDraw = GetNCsize();
		if ( bStart )
			m_nTraceStart = m_nTraceDraw;
		m_csTraceDraw.Unlock();
		return FALSE;
	}
	m_csTraceDraw.Lock();
	m_nTraceDraw = GetNCblock(i)->GetBlockToNCdataArrayNo();
	if ( bStart ) {
		int n =m_nTraceDraw - 1;
		m_nTraceStart = max(0, n);
	}
	m_csTraceDraw.Unlock();

	return TRUE;
}

void CNCDoc::GetWorkRectPP(int a, double dTmp[])
{
	ASSERT(a>=NCA_X && a<=NCA_Z);
	switch (a) {
	case NCA_X:
		dTmp[0] = m_rcMax.left;
		dTmp[1] = m_rcMax.right;
		break;
	case NCA_Y:
		dTmp[0] = m_rcMax.top;
		dTmp[1] = m_rcMax.bottom;
		break;
	case NCA_Z:
		dTmp[0] = m_rcMax.high;
		dTmp[1] = m_rcMax.low;
		break;
	}
}

void CNCDoc::MakeDXF(const CDXFMakeOption* pDXFMake)
{
	CWaitCursor		wait;
	CProgressCtrl*	pProgress;
	CTypedPtrArrayEx<CPtrArray, CDXFMake*>	obDXFdata;// DXF出力ｲﾒｰｼﾞ
	CDXFMake*	pMake;
	CNCdata*	pData;
	CNCdata*	pDataBase;
	int			i, j, nLoop = m_obGdata.GetSize(), nCorrect;
	BOOL		bOrigin = TRUE;
	DWORD		dwValFlag;
	CString		strMsg;

	obDXFdata.SetSize(0, nLoop);
	// 静的変数初期化
	CDXFMake::SetStaticOption(pDXFMake);
	// 平面指定による検査ﾌﾗｸﾞの設定
	switch ( pDXFMake->GetNum(MKDX_NUM_PLANE) ) {
	case 1:		// XZ
		dwValFlag = NCD_X | NCD_Z;
		break;
	case 2:		// YZ
		dwValFlag = NCD_Y | NCD_Z;
		break;
	default:	// XY
		dwValFlag = NCD_X | NCD_Y;
		break;
	}
	dwValFlag |= (NCD_I | NCD_J | NCD_K);	// 円弧補間用(どの平面からも見える)

	// ﾒｲﾝﾌﾚｰﾑのﾌﾟﾛｸﾞﾚｽﾊﾞｰ準備
	pProgress = AfxGetNCVCMainWnd()->GetProgressCtrl();
	pProgress->SetRange32(0, nLoop);
	pProgress->SetPos(0);

	// 下位の CMemoryException は全てここで集約
	try {
		// 各ｾｸｼｮﾝ生成(ENTITIESのｾｸｼｮﾝ名まで)
		for ( i=SEC_HEADER; i<=SEC_ENTITIES; i++ ) {
			pMake = new CDXFMake(i, this);
			obDXFdata.Add(pMake);
		}
		// NCｵﾌﾞｼﾞｪｸﾄのDXF出力
		for ( i=0; i<nLoop; i++ ) {
			pData = m_obGdata[i];
			if ( pData->GetGtype()==G_TYPE && pData->GetType()!=NCDBASEDATA &&
					pData->GetValFlags()&dwValFlag ) {
				if ( bOrigin ) {
					// 最初の対象ｺｰﾄﾞの開始座標を原点とする
					pMake = new CDXFMake(pData->GetStartPoint());
					obDXFdata.Add(pMake);
					bOrigin = FALSE;		// 原点出力済み
				}
				// ｵﾌﾞｼﾞｪｸﾄ生成
				pMake = new CDXFMake(pData);
				obDXFdata.Add(pMake);
				// 補正ｵﾌﾞｼﾞｪｸﾄ
				if ( m_bCorrect ) {
					pDataBase = pData;
					nCorrect = pDataBase->GetCorrectArray()->GetSize();
					for ( j=0; j<nCorrect; j++ ) {
						pData = pDataBase->GetCorrectArray()->GetAt(j);
						pMake = new CDXFMake(pData, TRUE);
						obDXFdata.Add(pMake);
					}
				}
			}
			if ( (i & 0x003f) == 0 )	// 64回おき(下位6ﾋﾞｯﾄﾏｽｸ)
				pProgress->SetPos(i);		// ﾌﾟﾛｸﾞﾚｽﾊﾞｰ
		}
		// ENTITIESｾｸｼｮﾝ終了とEOF出力
		pMake = new CDXFMake(SEC_NOSECTION);
		obDXFdata.Add(pMake);
		// 出力
		CStdioFile	fp(m_strDXFFileName,
				CFile::modeCreate | CFile::modeWrite | CFile::shareExclusive | CFile::typeText);
		for ( i=0; i<obDXFdata.GetSize(); i++ )
			obDXFdata[i]->WriteDXF(fp);
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
	}
	catch (CFileException* e) {
		strMsg.Format(IDS_ERR_DATAWRITE, m_strDXFFileName);
		AfxMessageBox(strMsg, MB_OK|MB_ICONSTOP);
		e->Delete();
	}

	// ｵﾌﾞｼﾞｪｸﾄ消去
	for ( i=0; i<obDXFdata.GetSize(); i++ )
		delete	obDXFdata[i];
	obDXFdata.RemoveAll();

	pProgress->SetPos(0);
}

void CNCDoc::AddMacroFile(const CString& strMacroFile)
{
	try {
		if ( !strMacroFile.IsEmpty() )
			m_obMacroFile.Add(strMacroFile);
	}
	catch (CMemoryException* e) {
		e->Delete();	// ｴﾗｰ処理(Msg)は特に必要としない
	}
}

/////////////////////////////////////////////////////////////////////////////
// CNCDoc クラスの診断

#ifdef _DEBUG
void CNCDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CNCDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CNCDoc クラスのオーバライド関数

BOOL CNCDoc::OnOpenDocument(LPCTSTR lpszPathName) 
{
	BOOL	bResult;
	PFNNCVCSERIALIZEFUNC	pSerialFunc = AfxGetNCVCApp()->GetSerializeFunc();

	if ( pSerialFunc ) {
		// ｱﾄﾞｲﾝｼﾘｱﾙ関数を保存．ﾌｧｲﾙ変更通知などに使用
		m_pfnSerialFunc = pSerialFunc;	// DocBase.h
		// ｱﾄﾞｲﾝのｼﾘｱﾙ関数を呼び出し
		bResult = (*pSerialFunc)(this, lpszPathName);
		// ｼﾘｱﾙ関数の初期化
		AfxGetNCVCApp()->SetSerializeFunc((PFNNCVCSERIALIZEFUNC)NULL);
	}
	else {
		// 通常のｼﾘｱﾙ関数呼び出し
		bResult = CDocument::OnOpenDocument(lpszPathName);
		if ( bResult ) {
			// CDocument::GetPathName() は
			// OnOpenDocument() 終了後にﾌﾚｰﾑﾜｰｸが設定するので使えない
			m_strCurrentFile = lpszPathName;
		}
	}

	if ( bResult ) {
		// ﾌｧｲﾙ読み込み後のﾁｪｯｸ
		bResult = SerializeAfterCheck();
		if ( bResult ) {
			// ﾄﾞｷｭﾒﾝﾄ変更通知ｽﾚｯﾄﾞの生成
			POSITION	pos = GetFirstViewPosition();
			CDocBase::OnOpenDocument(lpszPathName, GetNextView(pos)->GetParentFrame());
		}
	}

	// ﾒｲﾝﾌﾚｰﾑのﾌﾟﾛｸﾞﾚｽﾊﾞｰ初期化
	AfxGetNCVCMainWnd()->GetProgressCtrl()->SetPos(0);

	return bResult;
}

BOOL CNCDoc::OnSaveDocument(LPCTSTR lpszPathName) 
{
	// ﾄﾞｷｭﾒﾝﾄ変更通知ｽﾚｯﾄﾞの終了
	CDocBase::OnCloseDocument();

	if ( GetPathName().CompareNoCase(lpszPathName) != 0 ) {
		CNCDoc* pDoc = AfxGetNCVCApp()->GetAlreadyNCDocument(lpszPathName);
		if ( pDoc )
			pDoc->OnCloseDocument();	// 既に開いているﾄﾞｷｭﾒﾝﾄを閉じる
	}

	// 保存処理
	BOOL bResult = CDocument::OnSaveDocument(lpszPathName);

	// ﾄﾞｷｭﾒﾝﾄ変更通知ｽﾚｯﾄﾞの生成
	if ( bResult ) {
		POSITION	pos = GetFirstViewPosition();
		CFrameWnd*	pFrame = GetNextView(pos)->GetParentFrame();
		CDocBase::OnOpenDocument(lpszPathName, pFrame);
	}

	return bResult;
}

void CNCDoc::OnCloseDocument() 
{
/*
	各ﾋﾞｭｰの OnDestroy() よりも先に呼ばれる
*/
#ifdef _DEBUG
	CMagaDbg	dbg("CNCDoc::OnCloseDocument()\nStart", DBG_GREEN);
#endif
	// ﾛｯｸｱﾄﾞｲﾝのﾁｪｯｸ
	if ( !IsLockThread() ) {	// CDocBase
#ifdef _DEBUG
		dbg.printf("AddinLock FALSE");
#endif
		return;
	}

	// 処理中のｽﾚｯﾄﾞを中断させる
	CDocBase::OnCloseDocument();	// ﾌｧｲﾙ変更通知ｽﾚｯﾄﾞ
	WaitCalcThread();				// 切削時間計算ｽﾚｯﾄﾞ

	CDocument::OnCloseDocument();
}

void CNCDoc::ReportSaveLoadException
	(LPCTSTR lpszPathName, CException* e, BOOL bSaving, UINT nIDPDefault) 
{
	if ( e->IsKindOf(RUNTIME_CLASS(CUserException)) ) {
		AfxGetNCVCMainWnd()->GetProgressCtrl()->SetPos(0);
		return;	// 標準ｴﾗｰﾒｯｾｰｼﾞを出さない
	}
	CDocument::ReportSaveLoadException(lpszPathName, e, bSaving, nIDPDefault);
}

void CNCDoc::SetModifiedFlag(BOOL bModified)
{
	CString	strTitle( GetTitle() );
	if ( UpdateModifiedTitle(bModified, strTitle) )		// DocBase.cpp
		SetTitle(strTitle);
	CDocument::SetModifiedFlag(bModified);
}

/////////////////////////////////////////////////////////////////////////////
// CNCDoc シリアライゼーション

void CNCDoc::Serialize(CArchive& ar)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CNCDoc::Serialize()\nStart");
#endif

	if ( ar.IsStoring() ) {
		// ﾌｧｲﾙ保存
		AfxGetNCVCMainWnd()->GetProgressCtrl()->SetRange32(0, GetNCBlockSize());
		AfxGetNCVCMainWnd()->GetProgressCtrl()->SetPos(0);
		for ( int i=0; i<GetNCBlockSize(); i++ ) {
			ar.WriteString(GetNCblock(i)->GetStrBlock()+"\r\n");
			if ( (i & 0x003f) == 0 )		// 64回おき(下位6ﾋﾞｯﾄﾏｽｸ)
				AfxGetNCVCMainWnd()->GetProgressCtrl()->SetPos(i);
		}
		AfxGetNCVCMainWnd()->GetProgressCtrl()->SetPos(0);
		return;
	}

	// ﾌｧｲﾙ読み込み
	SerializeBlock(ar, m_obBlock);
}

void CNCDoc::SerializeBlock
	(CArchive& ar, CNCblockArray& obBlock, DWORD dwFlags/*=0*/, BOOL bProgress/*=TRUE*/)
{
	// 行番号とGｺｰﾄﾞの分別
	static	LPCTSTR	ss_szLineDelimiter = "%N0123456789";

#ifdef _DEBUG
	CMagaDbg	dbg;
#endif
	CString		strBlock, strLine;
	CNCblock*	pBlock = NULL;
	int			n, nCnt = 0;

	ULONGLONG	dwSize = ar.GetFile()->GetLength();		// ﾌｧｲﾙｻｲｽﾞ取得
	DWORD		dwPosition = 0;
	CProgressCtrl* pProgress = AfxGetNCVCMainWnd()->GetProgressCtrl();
	if ( bProgress ) {
		// ﾒｲﾝﾌﾚｰﾑのﾌﾟﾛｸﾞﾚｽﾊﾞｰ準備
		pProgress->SetRange32(0, 100);
		pProgress->SetPos(0);
	}

	try {
		while ( ar.ReadString(strBlock) ) {
			// ﾌﾟﾛｸﾞﾚｽﾊﾞｰの表示
			dwPosition += strBlock.GetLength() + 2;	// 改行ｺｰﾄﾞ分
			if ( bProgress && (++nCnt & 0x003f)==0 ) {	// 64回おき(下位6ﾋﾞｯﾄﾏｽｸ)
				n = (int)(dwPosition*100/dwSize);
				pProgress->SetPos(min(100, n));
			}
			// 行番号とGｺｰﾄﾞの分別(XYZ...含む)
			strLine = strBlock.SpanIncluding(ss_szLineDelimiter);
			pBlock = new CNCblock(strLine.Trim(), strBlock.Mid(strLine.GetLength()).Trim(), dwFlags);
			obBlock.Add(pBlock);
			pBlock = NULL;
#ifdef _DEBUGOLD
			dbg.printf("LineCnt=%d Line=%s Gcode=%s", nCnt,
				strLine, strBlock.Mid(strLine.GetLength()) );
#endif
		}
		if ( bProgress )
			pProgress->SetPos(100);
	}
	catch ( CMemoryException* e ) {
		if ( pBlock )
			delete	pBlock;
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		AfxThrowUserException();	// 暗黙の ReportSaveLoadException() 呼び出し
	}
}

BOOL CNCDoc::SerializeAfterCheck(void)
{
#ifdef _DEBUG
	CMagaDbg	dbg;
#endif
	int			i;
#ifdef _DEBUGOLD
	for ( i=0; i<GetNCBlockSize(); i++ )
		dbg.printf("%4d:%s", i, GetNCblock(i)->GetStrBlock());
#endif
	// ﾃﾞｰﾀﾁｪｯｸ
	for ( i=0; i<GetNCBlockSize(); i++ ) {
		if ( GetNCblock(i)->GetStrGcode().GetLength() > 0 )
			break;
	}
	if ( i >= GetNCBlockSize() ) {
		AfxMessageBox(IDS_ERR_NCDATA, MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}

	// 変換状況案内ﾀﾞｲｱﾛｸﾞ(変換ｽﾚｯﾄﾞ生成)
	CThreadDlg	dlg(IDS_READ_NCD, this);
	if ( dlg.DoModal() != IDOK )
		return FALSE;
	if ( m_obGdata.GetSize() < 1 ) {
		AfxMessageBox(IDS_ERR_NCDATA, MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}
	m_rcMax.NormalizeRect();
#ifdef _DEBUG
	dbg.printf("m_rcMax left =%f top   =%f", m_rcMax.left, m_rcMax.top);
	dbg.printf("m_rcMax right=%f bottom=%f", m_rcMax.right, m_rcMax.bottom);
	dbg.printf("m_rcMax.low  =%f high  =%f", m_rcMax.low, m_rcMax.high);
#endif
	// 補正座標計算
	if ( m_bCorrect ) {
		CThreadDlg	dlg(IDS_CORRECT_NCD, this);
		if ( dlg.DoModal() != IDOK )
			return FALSE;
	}

	// 切削時間計算ｽﾚｯﾄﾞ開始
	CreateCutcalcThread();
	// NCﾃﾞｰﾀの最大値取得
	m_nTraceDraw = GetNCsize();
	// ｴﾗｰﾌﾗｸﾞ解除
	m_fError = FALSE;
	// G10で仮登録された工具情報を削除
	AfxGetNCVCApp()->GetMCOption()->ReductionTools(TRUE);

	return TRUE;
}

BOOL CNCDoc::SerializeInsertBlock
	(LPCTSTR lpszFileName, int nInsert, DWORD dwFlags/*=0*/, BOOL bProgress/*=TRUE*/)
{
/*
	MFC\SRC\DOCCORE.CPP の CDocument::OnOpenDocument を参考
*/
	CFileException	fe;
	CFile*	pFile = GetFile(lpszFileName, CFile::modeRead|CFile::shareDenyWrite, &fe);
	if ( !pFile ) {
		ReportSaveLoadException(lpszFileName, &fe, FALSE, AFX_IDP_FAILED_TO_OPEN_DOC);
		return FALSE;
	}
	CArchive	ar(pFile, CArchive::load|CArchive::bNoFlushOnDelete);
	ar.m_pDocument = this;
	ar.m_bForceFlat = FALSE;

	BOOL	bResult = TRUE;
	if ( pFile->GetLength() > 0 ) {
		CNCblockArray	obBlock;	// 一時領域
		obBlock.SetSize(0, 1024);
		// ﾌｧｲﾙ読み込み
		SerializeBlock(ar, obBlock, dwFlags, bProgress);
		// ﾌﾞﾛｯｸ挿入
		m_obBlock.InsertAt(nInsert, &obBlock);
	}
	else
		bResult = FALSE;

	ar.Close();
	ReleaseFile(pFile, FALSE);

	return bResult;
}

/////////////////////////////////////////////////////////////////////////////
// CNCDoc コマンド

void CNCDoc::OnUpdateFileInsert(CCmdUI* pCmdUI) 
{
	CNCChild*		pFrame = static_cast<CNCChild *>(AfxGetNCVCMainWnd()->MDIGetActive());
	CNCListView*	pList = pFrame->GetListView();
	pCmdUI->Enable(pList->GetListCtrl().GetFirstSelectedItemPosition() ? TRUE : FALSE);
}

void CNCDoc::OnFileInsert()
{
	int		i;
	CNCChild*		pFrame = static_cast<CNCChild *>(AfxGetNCVCMainWnd()->MDIGetActive());
	CNCListView*	pList = pFrame->GetListView();
	POSITION pos;
	if ( !(pos=pList->GetListCtrl().GetFirstSelectedItemPosition()) )
		return;
	int	nInsert = pList->GetListCtrl().GetNextSelectedItem(pos);

	CString	strFileName(AfxGetNCVCApp()->GetRecentFileName());
	if ( ::NCVC_FileDlgCommon(ID_FILE_NCINSERT,
				AfxGetNCVCApp()->GetFilterString(TYPE_NCD), strFileName) != IDOK )
		return;

#ifdef _DEBUG
	CMagaDbg	dbg("CNCDoc::OnFileInsert()\nStart");
#endif

	// 「ｶｰｿﾙ位置に読み込み」の準備
	WaitCalcThread();			// 切削時間計算の中断

	// 再変換を行うため m_obGdata を削除
	for ( i=0; i<m_obGdata.GetSize(); i++ )
		delete	m_obGdata[i];
	m_obGdata.RemoveAll();
	m_bCorrect = FALSE; 
	// ﾌﾞﾛｯｸﾃﾞｰﾀのﾌﾗｸﾞをｸﾘｱ
	for ( i=0; i<GetNCBlockSize(); i++ )
		GetNCblock(i)->SetNCBlkErrorCode(0);
	// 変数初期化
	m_fError = TRUE;
	m_nTraceDraw = 0;

	// ﾌｧｲﾙ(NCﾌﾞﾛｯｸの挿入)
	if ( SerializeInsertBlock(strFileName, nInsert) ) {
		// 処理中のﾌｧｲﾙを挿入ﾌｧｲﾙ名に設定
		m_strCurrentFile = strFileName;
		// ﾌｧｲﾙ読み込み後のﾁｪｯｸ
		SerializeAfterCheck();	// 戻り値ﾁｪｯｸしてもどうにもならない
		// 更新ﾌﾗｸﾞON
		SetModifiedFlag();
		// 各ﾋﾞｭｰの設定 & 描画更新
		UpdateAllViews(NULL, UAV_FILEINSERT);
	}

	// ﾒｲﾝﾌﾚｰﾑのﾌﾟﾛｸﾞﾚｽﾊﾞｰ初期化
	AfxGetNCVCMainWnd()->GetProgressCtrl()->SetPos(0);
}

void CNCDoc::OnUpdateFileSave(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(IsModified());
}

void CNCDoc::OnFileNCD2DXF() 
{
	CMakeDXFDlg	ps(this);
	if ( ps.DoModal() != IDOK )
		return;
	if ( !ps.GetDXFMakeOption()->SaveDXFMakeOption() )
		AfxMessageBox(IDS_ERR_REGISTRY, MB_OK|MB_ICONEXCLAMATION);

	// ﾌｧｲﾙ名取得と上書き確認
	m_strDXFFileName = ps.m_dlg1.GetDXFFileName();
	if ( ::IsFileExist(m_strDXFFileName, FALSE) )	// NCVC.cpp
		MakeDXF(ps.GetDXFMakeOption());	// DXF出力
}

void CNCDoc::OnUpdateWorkRect(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck( AfxGetNCVCMainWnd()->GetModelessDlg(MLD_NCWORK) != NULL );
}

void CNCDoc::OnWorkRect() 
{
	if ( AfxGetNCVCMainWnd()->GetModelessDlg(MLD_NCWORK) ) {
		// CNCWorkDlg::OnCancel() の間接呼び出し
		AfxGetNCVCMainWnd()->GetModelessDlg(MLD_NCWORK)->PostMessage(WM_CLOSE);
		return;
	}
	try {
		CNCWorkDlg*	pDlg = new CNCWorkDlg;
		pDlg->Create(IDD_NCVIEW_WORK);
		AfxGetNCVCMainWnd()->SetModelessDlg(MLD_NCWORK, pDlg);
	}
	catch ( CMemoryException* e ) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
	}
}

void CNCDoc::OnUpdateMaxRect(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(GetDocError() ? FALSE : TRUE);
	pCmdUI->SetCheck(m_bMaxRect);
}

void CNCDoc::OnMaxRect() 
{
	m_bMaxRect = !m_bMaxRect;
	// ﾋﾞｭｰの更新
	UpdateAllViews(NULL, UAV_DRAWMAXRECT);
}
