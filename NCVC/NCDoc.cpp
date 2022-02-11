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
#include "DXFkeyword.h"
#include "DXFOption.h"
#include "DXFMakeOption.h"
#include "DXFMakeClass.h"
#include "MakeDXFDlg.h"
#ifdef USE_KODATUNO
#include "Kodatuno/IGES_Parser.h"
#include "Kodatuno/STL_Parser.h"
#undef PI	// Use NCVC (MyTemplate.h)
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
//#define	_DEBUGOLD
#undef	_DEBUGOLD
#endif

using namespace boost;

// 文字列検査用(TH_NCRead.cpp, NCMakeClass.cpp からも参照)
extern	LPCTSTR	g_szLineDelimiter = "%N0123456789";	// 行番号とGｺｰﾄﾞの分別
extern	LPCTSTR	g_szGdelimiter = "GSMOF";
extern	LPCTSTR	g_szNdelimiter = "XYZUVWIJKRPLDH";
extern	LPTSTR	g_pszDelimiter;	// g_szGdelimiter[] + g_szNdelimiter[] (NCVC.cppで生成)
extern	LPCTSTR	g_szNCcomment[] = {
	"Endmill", "Drill", "Tap", "Reamer", "GroovingTool",
	"WorkRect", "WorkCylinder", "WorkFile", "MCFile",
	"LatheView", "WireView",
	"ToolPos", "LatheHole",
	"Inside", "EndInside", "EndDrill", "EndGrooving"
};

// 指定された値のﾌﾗｸﾞ
extern	const	DWORD	g_dwSetValFlags[] = {
	NCD_X, NCD_Y, NCD_Z, 
	NCD_U, NCD_V, NCD_W, 
	NCD_I, NCD_J, NCD_K, NCD_R,
	NCD_P, NCD_L,
	NCD_D, NCD_H
};

// ｻﾑﾈｲﾙﾓｰﾄﾞのときに読み込む最大ﾌﾞﾛｯｸ数
static	const INT_PTR	THUMBNAIL_MAXREADBLOCK = 5000;
#define	IsThumbnail()	m_bDocFlg[NCDOC_THUMBNAIL]

// 空のNCプログラムのときのダミー矩形
static	const CRect3F	g_rcDefRect(-50.0f, -50.0f, 50.0f, 50.0f, 50.0f, -50.0f);

/////////////////////////////////////////////////////////////////////////////
// CNCDoc

IMPLEMENT_DYNCREATE(CNCDoc, CDocument)

BEGIN_MESSAGE_MAP(CNCDoc, CDocument)
	//{{AFX_MSG_MAP(CNCDoc)
	ON_COMMAND(ID_FILE_NCD2DXF, &CNCDoc::OnFileNCD2DXF)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE, &CNCDoc::OnUpdateFileSave)
	ON_UPDATE_COMMAND_UI(ID_NCVIEW_WORKRECT, &CNCDoc::OnUpdateWorkRect)
	ON_COMMAND(ID_NCVIEW_WORKRECT, &CNCDoc::OnWorkRect)
	ON_UPDATE_COMMAND_UI(ID_NCVIEW_MAXRECT, &CNCDoc::OnUpdateMaxRect)
	ON_COMMAND(ID_NCVIEW_MAXRECT, &CNCDoc::OnMaxRect)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNCDoc クラスの構築/消滅

CNCDoc::CNCDoc()
{
#ifdef _DEBUG_FILEOPEN
	printf("CNCDoc::CNCDoc() Start\n");
#endif
	int		i;

	m_bDocFlg.set(NCDOC_ERROR);	// 初期状態はｴﾗｰﾌﾗｸﾞだけ立てる
	ZEROCLR(m_dMove);
	m_dCutTime = -1.0f;
	m_nDecimalID = IDCV_VALFORMAT3;
	m_nTrace = ID_NCVIEW_TRACE_STOP;
	m_nTraceStart = m_nTraceDraw = 0;
	m_pCutcalcThread  = NULL;
	m_pRecentViewInfo = NULL;
	// ﾜｰｸ座標系取得
	const CMCOption* pMCopt = AfxGetNCVCApp()->GetMCOption();
	for ( i=0; i<WORKOFFSET; i++ )
		m_ptNcWorkOrg[i] = pMCopt->GetWorkOffset(i);
	m_ptNcWorkOrg[i] = 0.0f;		// G92の初期化
	m_nWorkOrg = pMCopt->GetModalSetting(MODALGROUP2);		// G54～G59
	if ( m_nWorkOrg<0 || SIZEOF(m_ptNcWorkOrg)<=m_nWorkOrg )
		m_nWorkOrg = 0;
	// ｵﾌﾞｼﾞｪｸﾄ矩形の初期化
	m_rcMax.SetRectMinimum();
	m_rcWork.SetRectMinimum();
	// 増分割り当てサイズ
	m_obBlock.SetSize(0, 4096);
	m_obGdata.SetSize(0, 4096);
#ifdef USE_KODATUNO
	m_kBody  = NULL;
	m_kbList = NULL;
#endif
}

CNCDoc::~CNCDoc()
{
	// NCﾃﾞｰﾀの消去
	for ( int i=0; i<m_obGdata.GetSize(); i++ )
		delete	m_obGdata[i];
	m_obGdata.RemoveAll();
#ifdef USE_KODATUNO
	// IGES Body
	if ( m_kBody ) {
		//m_kBody->DeleteBody(m_kbList);
		m_kBody->DelBodyElem();
		delete	m_kBody;
	}
	if ( m_kbList ) {
		m_kbList->clear();
		delete	m_kbList;
	}
#endif
	// ﾌﾞﾛｯｸﾃﾞｰﾀの消去
	ClearBlockData();
	// 一時展開のﾏｸﾛﾌｧｲﾙを消去
	DeleteMacroFile();
}

/////////////////////////////////////////////////////////////////////////////
// ﾕｰｻﾞﾒﾝﾊﾞ関数

BOOL CNCDoc::RouteCmdToAllViews
	(CView* pActiveView, UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) 
{
#ifdef _DEBUG_CMDMSG
	printf("CNCDoc::RouteCmdToAllViews()\n");
#endif
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

void CNCDoc::SetLatheViewMode(void)
{
	if ( !m_bDocFlg[NCDOC_LATHE] ) {
		m_bDocFlg.set(NCDOC_LATHE);	// NC旋盤ﾓｰﾄﾞ
		// 座標系のXZを入れ替え
		for ( int i=0; i<WORKOFFSET+1; i++ ) {
			m_ptNcWorkOrg[i].x /= 2.0;
			swap(m_ptNcWorkOrg[i].x, m_ptNcWorkOrg[i].z);
		}
		m_ptNcLocalOrg.x /= 2.0;
		swap(m_ptNcLocalOrg.x, m_ptNcLocalOrg.z);
	}
}

BOOL CNCDoc::ReadWorkFile(LPCTSTR strFile)
{
#ifdef USE_KODATUNO
	CString	strPath, strName, strExt;
	TCHAR	szFileName[_MAX_FNAME],
			szExt[_MAX_EXT];

	// ﾌｧｲﾙ名の検査
	if ( ::PathIsRelative(strFile) ) {
		CString	strCurrent;
		TCHAR	pBuf[MAX_PATH];
		::Path_Name_From_FullPath(m_strCurrentFile, strCurrent, strName);	// GetPathName()不可
		strCurrent += strFile;
		if ( ::PathCanonicalize(pBuf, strCurrent) )
			strPath = pBuf;
	}
	else {
		strPath = strFile;
	}
	_tsplitpath_s(strPath, NULL, 0, NULL, 0,
		szFileName, SIZEOF(szFileName), szExt, SIZEOF(szExt));
	if ( lstrlen(szFileName)<=0 || lstrlen(szExt)<=0 )
		return FALSE;
	strExt = szExt + 1;		// ドットを除く

	// WorkFile読み込み準備
	if ( m_kBody ) {
		if ( m_kbList )
			m_kbList->clear();
		m_kBody->DelBodyElem();
		delete	m_kBody;
	}
	m_kBody = new BODY;

	// 拡張子で判別
	int	nResult = KOD_FALSE;
	if ( strExt.CompareNoCase("igs")==0 || strExt.CompareNoCase("iges")==0 ) {
		IGES_PARSER	iges;
		if ( (nResult=iges.IGES_Parser_Main(m_kBody, strPath)) == KOD_TRUE )
			iges.Optimize4OpenGL(m_kBody);
	}
	else if ( strExt.CompareNoCase("stl") == 0 ) {
		STL_PARSER	stl;
		nResult = stl.STL_Parser_Main(m_kBody, strPath);
	}
	if ( nResult != KOD_TRUE ) {
		delete	m_kBody;
		m_kBody = NULL;
		return FALSE;
	}

	// Kodatuno BODY 登録
	if ( !m_kbList )
		m_kbList = new BODYList;
	m_kBody->RegistBody(m_kbList, strFile);
#endif

	m_bDocFlg.set(NCDOC_WORKFILE);

	return TRUE;
}

#ifdef USE_KODATUNO
void CNCDoc::SetWorkFileOffset(const Coord& sft)
{
	if ( m_kBody )
		m_kBody->ShiftBody(sft);
}
#endif

BOOL CNCDoc::ReadMCFile(LPCTSTR strFile)
{
	CString	strPath, strName, strExt;
	TCHAR	szFileName[_MAX_FNAME],
			szExt[_MAX_EXT];

	// ﾌｧｲﾙ名の検査
	if ( ::PathIsRelative(strFile) ) {
		CString	strCurrent;
		TCHAR	pBuf[MAX_PATH];
		::Path_Name_From_FullPath(m_strCurrentFile, strCurrent, strName);	// GetPathName()不可
		strCurrent += strFile;
		if ( ::PathCanonicalize(pBuf, strCurrent) )
			strPath = pBuf;
	}
	else {
		strPath = strFile;
	}
	_tsplitpath_s(strPath, NULL, 0, NULL, 0,
		szFileName, SIZEOF(szFileName), szExt, SIZEOF(szExt));
	if ( lstrlen(szFileName)<=0 )
		return FALSE;
	if ( lstrlen(szExt)<=0 ) {
		// 拡張子の補間
		VERIFY(strExt.LoadString(IDS_MC_FILTER));
		strPath += '.' + strExt.Left(3);	// .mnc
	}

	// 機械情報の読み込み
	// -- TH_NCRead.cpp からの呼び出しでﾂｰﾙﾊﾞｰの更新は別スレッドなのでダメ
	// -- AfxGetNCVCMainWnd()->ChangeMachine() はできない
	return  AfxGetNCVCApp()->GetMCOption()->ReadMCoption(strPath);
}

CNCdata* CNCDoc::DataOperation
	(const CNCdata* pData, LPNCARGV lpArgv, INT_PTR nIndex/*=-1*/, ENNCOPERATION enOperation/*=NCADD*/)
{
	CMCOption*	pOpt = AfxGetNCVCApp()->GetMCOption();
	CNCdata*	pDataResult = NULL;
	CNCblock*	pBlock;
//	CPoint3F	ptOffset( m_ptNcWorkOrg[m_nWorkOrg] + m_ptNcLocalOrg );	// GetOffsetOrig()
	INT_PTR		i;
	BOOL		bResult = TRUE;
	enMAKETYPE	enMakeType;

	// 円弧補間用
	if ( m_bDocFlg[NCDOC_LATHE] )
		enMakeType = NCMAKELATHE;
	else if ( m_bDocFlg[NCDOC_WIRE] )
		enMakeType = NCMAKEWIRE;
	else
		enMakeType = NCMAKEMILL;

	// 例外ｽﾛｰは上位でｷｬｯﾁ
	if ( lpArgv->nc.nGtype == G_TYPE ) {
		switch ( lpArgv->nc.nGcode ) {
		case 0:		// 直線
		case 1:
			// ｵﾌﾞｼﾞｪｸﾄ生成
			pDataResult = new CNCline(pData, lpArgv, GetOffsetOrig());
			SetMaxRect(pDataResult);		// 最小・最大値の更新
			if ( lpArgv->nc.dwValFlags & NCD_CORRECT )
				m_bDocFlg.set(NCDOC_REVISEING);	// 補正ﾓｰﾄﾞ
			break;
		case 2:		// 円弧
		case 3:
			pDataResult = new CNCcircle(pData, lpArgv, GetOffsetOrig(), enMakeType);
			SetMaxRect(pDataResult);
			if ( lpArgv->nc.dwValFlags & NCD_CORRECT )
				m_bDocFlg.set(NCDOC_REVISEING);
			break;
		case 4:		// ﾄﾞｳｪﾙ
			if ( lpArgv->nc.dwValFlags & NCD_X ) {
				// 移動ｺｰﾄﾞを考慮し、Ｐ値に強制変換
				lpArgv->nc.dValue[NCA_P] = lpArgv->nc.dValue[NCA_X] * 1000.0;	// sec -> msec
				lpArgv->nc.dValue[NCA_X] = 0.0f;
				lpArgv->nc.dwValFlags &= ~NCD_X;
				lpArgv->nc.dwValFlags |=  NCD_P;
			}
			pDataResult = new CNCdata(pData, lpArgv, GetOffsetOrig());
			break;
		case 73:	case 74:	case 76:	// 固定ｻｲｸﾙ
		case 81:	case 82:	case 83:	case 84:	case 85:
		case 86:	case 87:	case 88:	case 89:
			pDataResult = new CNCcycle(pData, lpArgv, GetOffsetOrig(), pOpt->GetFlag(MC_FLG_L0CYCLE), enMakeType);
			SetMaxRect(pDataResult);
			break;
		case 10:	// ﾃﾞｰﾀ設定
			if ( lpArgv->nc.dwValFlags & (NCD_P|NCD_R) ) {	// G10P_R_
				if ( !IsThumbnail() ) {
					// 工具情報の追加
					if ( !pOpt->AddTool((int)lpArgv->nc.dValue[NCA_P], (float)lpArgv->nc.dValue[NCA_R], lpArgv->bAbs) ) {
						i = lpArgv->nc.nLine;
						if ( 0<=i && i<GetNCBlockSize() ) {	// 保険
							pBlock = m_obBlock[i];
							pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_G10ADDTOOL);
						}
						// このｴﾗｰはｽﾙｰ
					}
				}
			}
			else if ( lpArgv->nc.dwValFlags & NCD_P ) {
				// ﾜｰｸ座標系の設定
				int nWork = (int)lpArgv->nc.dValue[NCA_P];
				if ( nWork>=0 && nWork<WORKOFFSET ) { 
					for ( i=0; i<NCXYZ; i++ ) {
						if ( lpArgv->nc.dwValFlags & g_dwSetValFlags[i] )
							m_ptNcWorkOrg[nWork][i] += (float)lpArgv->nc.dValue[i];
					}
				}
				else {
					bResult = FALSE;
				}
			}
			else {
				bResult = FALSE;
			}
			pDataResult = new CNCdata(pData, lpArgv, GetOffsetOrig());
			if ( !bResult ) {
				// P値認識不能など
				i = lpArgv->nc.nLine;
				if ( 0<=i && i<GetNCBlockSize() ) {
					pBlock = m_obBlock[i];
					pBlock->SetNCBlkErrorCode(IDS_ERR_NCBLK_ORDER);
				}
			}
			break;
		case 52:	// Mill
		case 93:	// Wire
			// ﾛｰｶﾙ座標設定
			for ( i=0; i<NCXYZ; i++ ) {
				if ( lpArgv->nc.dwValFlags & g_dwSetValFlags[i] )
					m_ptNcLocalOrg[i] = (float)lpArgv->nc.dValue[i];
			}
			pDataResult = new CNCdata(pData, lpArgv, GetOffsetOrig());
			break;
		case 92:
			if ( !m_bDocFlg[NCDOC_LATHE] ) {
				// ﾛｰｶﾙ座標系ｸﾘｱとG92値取得
				for ( i=0; i<NCXYZ; i++ ) {
					m_ptNcLocalOrg[i] = 0.0f;
					if ( lpArgv->nc.dwValFlags & g_dwSetValFlags[i] ) {
						// 座標指定のあるところだけ、現在位置からの減算で
						// G92座標系原点を計算
						m_ptNcWorkOrg[WORKOFFSET][i] = pData->GetEndValue(i) - (float)lpArgv->nc.dValue[i];
					}
				}
				// 現在位置 - G92値 で、G92座標系原点を計算
				m_nWorkOrg = WORKOFFSET;	// G92座標系選択
				// ptOffset = m_ptNcWorkOrg[WORKOFFSET];
			}
			if ( m_bDocFlg[NCDOC_WIRE] ) {
				// ﾜｰｸ厚さとﾌﾟﾛｸﾞﾗﾑ面の指示
				if ( lpArgv->nc.dwValFlags & NCD_I )
					m_rcWorkCo.high = (float)lpArgv->nc.dValue[NCA_I];
				if ( lpArgv->nc.dwValFlags & NCD_J )
					m_rcWorkCo.low  = (float)lpArgv->nc.dValue[NCA_J];
			}
			// through
		default:
			pDataResult = new CNCdata(pData, lpArgv, GetOffsetOrig());
		}
	}	// end of G_TYPE
	else {
		// M_TYPE, O_TYPE, etc.
		pDataResult = new CNCdata(pData, lpArgv, GetOffsetOrig());
	}

	ASSERT( pDataResult );

	// ｵﾌﾞｼﾞｪｸﾄ登録
	switch ( enOperation ) {
	case NCADD:
		m_obGdata.Add(pDataResult);
		break;
	case NCINS:
		m_obGdata.InsertAt(nIndex, pDataResult);
		break;
	case NCMOD:
		RemoveAt(nIndex, 1);
		m_obGdata.SetAt(nIndex, pDataResult);
		break;
	}

	// 行番号にﾘﾝｸしたｴﾗｰﾌﾗｸﾞの設定
	UINT	nError = pDataResult->GetNCObjErrorCode();
	if ( nError > 0 ) {
		i = pDataResult->GetBlockLineNo();
		if ( 0<=i && i<GetNCBlockSize() ) {	// 保険
			pBlock = m_obBlock[i];
			pBlock->SetNCBlkErrorCode(nError);
		}
	}

	return pDataResult;
}

void CNCDoc::StrOperation(LPCTSTR pszTmp, INT_PTR nIndex/*=-1*/, ENNCOPERATION enOperation/*=NCADD*/)
{
	// 例外ｽﾛｰは上位でｷｬｯﾁ
	CNCblock* pBlock = new CNCblock( CString(pszTmp) );
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

void CNCDoc::RemoveAt(INT_PTR nIndex, INT_PTR nCnt)
{
	nCnt = min(nCnt, m_obGdata.GetSize() - nIndex);
	CNCdata*	pData;
	for ( INT_PTR i=nIndex; i<nIndex+nCnt; i++ ) {
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

void CNCDoc::RemoveStr(INT_PTR nIndex, INT_PTR nCnt)
{
	nCnt = min(nCnt, GetNCBlockSize() - nIndex);
	for ( INT_PTR i=nIndex; i<nIndex+nCnt; i++ )
		delete	m_obBlock[i];
	m_obBlock.RemoveAt(nIndex, nCnt);
}

void CNCDoc::ClearBlockData(void)
{
	for ( int i=0; i<GetNCBlockSize(); i++ )
		delete	m_obBlock[i];
	m_obBlock.RemoveAll();
}

void CNCDoc::SetWorkRect(BOOL bShow, const CRect3F& rc)
{
	m_bDocFlg.set(NCDOC_WRKRECT, bShow);
	if ( bShow ) {
		m_rcWork = rc;
		m_bDocFlg.reset(NCDOC_CYLINDER);
		m_bDocFlg.reset(NCDOC_WORKFILE);
	}
	UpdateAllViews(NULL, UAV_DRAWWORKRECT, (CObject *)(INT_PTR)bShow);
}

void CNCDoc::SetWorkCylinder(BOOL bShow, float d, float h, const CPoint3F& ptOffset)
{
	m_bDocFlg.set(NCDOC_CYLINDER, bShow);
	if ( bShow ) {
		d /= 2.0;
		CRect3F	rc(-d, -d, d, d, h, 0);
		rc.OffsetRect(ptOffset);
		m_rcWork = rc;
		m_bDocFlg.reset(NCDOC_WRKRECT);
		m_bDocFlg.reset(NCDOC_WORKFILE);
	}
	UpdateAllViews(NULL, UAV_DRAWWORKRECT, (CObject *)(INT_PTR)bShow);
}

void CNCDoc::SetCommentStr(const CString& strComment)
{
	// 既存のｺﾒﾝﾄ行を検索(hoge=ddd.dd, ddd.d, ...にﾏｯﾁ)
	size_t	n;
	if ( m_bDocFlg[NCDOC_LATHE] )
		n = LATHEVIEW;
	else if ( m_bDocFlg[NCDOC_CYLINDER] )
		n = WORKCYLINDER;
	else if ( m_bDocFlg[NCDOC_WRKRECT] )
		n = WORKRECT;
	else
		return;

	CString	strKey(g_szNCcomment[n]),
			strDouble("[\\+\\-]?\\d+\\.?\\d*"),
			strRegex(strKey+"\\s*=\\s*("+strDouble+")*(\\s*,\\s*"+strDouble+")*");

	regex	r1(strRegex, regex::icase);
	INT_PTR	nIndex = SearchBlockRegex(r1, FALSE), i;
	BOOL	bInsert = TRUE;
	
	if ( nIndex >= 0 ) {
		// 複数のｺﾒﾝﾄﾜｰﾄﾞが記述されている可能性があるので
		// 該当部分を置換で除去する
		std::string	strReplace(m_obBlock[nIndex]->GetStrBlock());
		strReplace = regex_replace(strReplace, r1, "");
		// ｶｯｺだけが残ったらｶｯｺも除去
		regex	rc("\\(\\)");
		strReplace = regex_replace(strReplace, rc, "");
		if ( strReplace.empty() )
			bInsert = FALSE;
		else
			StrOperation(strReplace.c_str(), nIndex, NCMOD);
	}
	if ( bInsert ) {
		// ｺﾒﾝﾄ行を新規挿入
		regex	r2("^%");
		for ( i=0; i<GetNCBlockSize(); i++ ) {
			// "%"の次行
			if ( regex_search(LPCTSTR(m_obBlock[i]->GetStrBlock()), r2) ) {
				i++;
				break;
			}
			// または最初のGｺｰﾄﾞの直前に挿入
			if ( m_obBlock[i]->GetBlockToNCdata() )
				break;
		}
		StrOperation(strComment, i, NCINS);
	}
	else
		StrOperation(strComment, nIndex, NCMOD);

	SetModifiedFlag();
}

void CNCDoc::DeleteMacroFile(void)
{
	// 一時展開のﾏｸﾛﾌｧｲﾙを消去
	for ( int i=0; i<m_obMacroFile.GetSize(); i++ ) {
#ifdef _DEBUG
		BOOL bResult = ::DeleteFile(m_obMacroFile[i]);
		if ( !bResult ) {
			printf("Delete File=%s\n", LPCTSTR(m_obMacroFile[i]));
			::NC_FormatMessage();
		}
#else
		::DeleteFile(m_obMacroFile[i]);
#endif
	}
	m_obMacroFile.RemoveAll();
}

void CNCDoc::AllChangeFactor(ENNCDRAWVIEW enType, float f) const
{
	typedef	void(CNCdata::*PFNDRAWPROC)(float);
	PFNDRAWPROC	pfnDrawProc;
	switch (enType) {
	case NCDRAWVIEW_XY:
		pfnDrawProc = &(CNCdata::DrawTuningXY);
		break;
	case NCDRAWVIEW_XZ:
		pfnDrawProc = &(CNCdata::DrawTuningXZ);
		break;
	case NCDRAWVIEW_YZ:
		pfnDrawProc = &(CNCdata::DrawTuningYZ);
		break;
	default:	// NCDRAWVIEW_XYZ
		pfnDrawProc = &(CNCdata::DrawTuning);
		break;
	}
	for ( int i=0; i<GetNCsize(); i++ )
		(GetNCdata(i)->*pfnDrawProc)(f);
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

	m_pCutcalcThread = AfxBeginThread(CuttimeCalc_Thread, pParam,
			THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
	if ( m_pCutcalcThread ) {
		m_bDocFlg.set(NCDOC_CUTCALC);
		m_pCutcalcThread->m_bAutoDelete = FALSE;
		m_pCutcalcThread->ResumeThread();
	}
	else {
		m_bDocFlg.reset(NCDOC_CUTCALC);
		delete	pParam;
	}
}

void CNCDoc::WaitCalcThread(BOOL bWaitOnly/*=FALSE*/)
{
	if ( !bWaitOnly )
		m_bDocFlg.reset(NCDOC_CUTCALC);	// 終了ﾌﾗｸﾞ

	if ( m_pCutcalcThread ) {
#ifdef _DEBUG
		if ( ::WaitForSingleObject(m_pCutcalcThread->m_hThread, INFINITE) == WAIT_FAILED ) {
			printf("CNCDoc::WaitForSingleObject() Fail!\n");
			::NC_FormatMessage();
		}
		else
			printf("CNCDoc::WaitForSingleObject() OK\n");
#else
		::WaitForSingleObject(m_pCutcalcThread->m_hThread, INFINITE);
#endif
		delete	m_pCutcalcThread;
		m_pCutcalcThread = NULL;
	}
}

INT_PTR CNCDoc::SearchBlockRegex(regex& r,
	BOOL bCommentThrough/*=TRUE*/, INT_PTR nStart/*=0*/, BOOL bReverse/*=FALSE*/)
{
	INT_PTR		i;
	std::string	str;
	regex		c("\\(.*\\)");	// (コメント)

	if ( bReverse ) {
		for (i=nStart; i>=0; i--) {
			str = LPCTSTR(m_obBlock[i]->GetStrBlock());
			if ( bCommentThrough )
				str = regex_replace(str, c, "");	// ｺﾒﾝﾄを除く
			if ( !str.empty() && regex_search(str, r) )
				return i;
		}
	}
	else {
		for (i=nStart; i<GetNCBlockSize(); i++) {
			str = LPCTSTR(m_obBlock[i]->GetStrBlock());
			if ( bCommentThrough )
				str = regex_replace(str, c, "");
			if ( !str.empty() && regex_search(str, r) )
				return i;
		}
	}

	return -1;
}

void CNCDoc::ClearBreakPoint(void)
{
	CNCblock*	pBlock;
	for (int i=0; i<GetNCBlockSize(); i++) {
		pBlock = m_obBlock[i];
		pBlock->SetBlockFlag(pBlock->GetBlockFlag() & ~NCF_BREAK, FALSE);
	}
}

BOOL CNCDoc::IncrementTrace(INT_PTR& nTraceDraw)
{
	INT_PTR		nMax = GetNCsize(), nLine1, nLine2;
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
	// bStart==TRUE  -> ｶｰｿﾙ位置から実行
	// bStart==FALSE -> ｶｰｿﾙ位置まで実行
	INT_PTR		i;
	CNCdata*	pData;

	for ( i=nLine; i<GetNCBlockSize(); i++ ) {
		pData = m_obBlock[i]->GetBlockToNCdata();
		if ( pData )
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
	// pDataを検索
	for ( i=0; i<m_obGdata.GetSize(); i++ ) {
		if ( pData == m_obGdata[i] )
			break;
	}
	m_nTraceDraw = i;

	if ( bStart ) {
		INT_PTR n = m_nTraceDraw - 1;
		m_nTraceStart = max(0, n);
	}
	else if ( m_nTrace == ID_NCVIEW_TRACE_PAUSE ) {
		m_nTraceStart = m_nTraceDraw;
	}
	else
		m_nTraceStart = 0;
	m_csTraceDraw.Unlock();

	return TRUE;
}

void CNCDoc::InsertBlock(int nInsert, const CString& strFileName)
{
#ifdef _DEBUG
	printf("CNCDoc::OnFileInsert() Start\n");
#endif
	int		i;

	// 「ｶｰｿﾙ位置に読み込み」の準備
	WaitCalcThread();		// 切削時間計算の中断

	// 再変換を行うため m_obGdata を削除
	for ( i=0; i<m_obGdata.GetSize(); i++ )
		delete	m_obGdata[i];
	m_obGdata.RemoveAll();
	m_bDocFlg.reset(NCDOC_REVISEING); 
	// ﾌﾞﾛｯｸﾃﾞｰﾀのﾌﾗｸﾞをｸﾘｱ
	for ( i=0; i<GetNCBlockSize(); i++ )
		m_obBlock[i]->SetNCBlkErrorCode(0);
	// 変数初期化
	m_bDocFlg.set(NCDOC_ERROR);
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

void CNCDoc::GetWorkRectPP(int a, float dResult[])
{
	ASSERT(a>=NCA_X && a<=NCA_Z);
	switch (a) {
	case NCA_X:
		dResult[0] = m_rcMax.left;
		dResult[1] = m_rcMax.right;
		break;
	case NCA_Y:
		dResult[0] = m_rcMax.top;
		dResult[1] = m_rcMax.bottom;
		break;
	case NCA_Z:
		dResult[0] = m_rcMax.high;
		dResult[1] = m_rcMax.low;
		break;
	}
}

void CNCDoc::MakeDXF(const CDXFMakeOption* pDXFMake)
{
	CWaitCursor		wait;
	CProgressCtrl*	pProgress = AfxGetNCVCMainWnd()->GetProgressCtrl();
	CDxfMakeArray	obDXFdata;// DXF出力ｲﾒｰｼﾞ
	CDXFMake*	pMake;
	CNCdata*	pData;
	CNCdata*	pDataBase;
	INT_PTR			i, j, nCorrect;
	const INT_PTR	nLoop = m_obGdata.GetSize();
	int			p = 0;
	double		n;
	BOOL		bOrigin = TRUE, bResult = TRUE;
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
	pProgress->SetRange32(0, 100);
	pProgress->SetPos(0);

	// 下位の CMemoryException は全てここで集約
	try {
		// 各ｾｸｼｮﾝ生成(ENTITIESのｾｸｼｮﾝ名まで)
		for ( i=SEC_HEADER; i<=SEC_ENTITIES; i++ ) {
			pMake = new CDXFMake((enSECNAME)i, this);
			obDXFdata.Add(pMake);
		}
		// NCｵﾌﾞｼﾞｪｸﾄのDXF出力
		for ( i=0; i<nLoop; i++ ) {
			pData = m_obGdata[i];
			if ( pData->GetType()!=NCDBASEDATA && pData->GetValFlags()&dwValFlag ) {
				if ( bOrigin ) {
					if ( pDXFMake->GetFlag(MKDX_FLG_OUT_O) ) {
						// 最初の対象ｺｰﾄﾞの開始座標を原点とする
						pMake = new CDXFMake(pData->GetStartPoint());
						obDXFdata.Add(pMake);
					}
					bOrigin = FALSE;	// 原点出力済み
				}
				// ｵﾌﾞｼﾞｪｸﾄ生成
				pMake = new CDXFMake(pData);
				obDXFdata.Add(pMake);
				// 補正ｵﾌﾞｼﾞｪｸﾄ
				if ( m_bDocFlg[NCDOC_REVISEING] && pDXFMake->GetFlag(MKDX_FLG_OUT_H) ) {
					pDataBase = pData;
					nCorrect = pDataBase->GetCorrectArray()->GetSize();
					for ( j=0; j<nCorrect; j++ ) {
						pData = pDataBase->GetCorrectArray()->GetAt(j);
						pMake = new CDXFMake(pData, TRUE);
						obDXFdata.Add(pMake);
					}
				}
			}
			n = (double)i/nLoop*100.0;
			while ( n >= p )
				p += 10;
			pProgress->SetPos(p);
		}
		// ENTITIESｾｸｼｮﾝ終了とEOF出力
		pMake = new CDXFMake(SEC_NOSECNAME);
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
		bResult = FALSE;
	}
	catch (CFileException* e) {
		strMsg.Format(IDS_ERR_DATAWRITE, m_strDXFFileName);
		AfxMessageBox(strMsg, MB_OK|MB_ICONSTOP);
		e->Delete();
		bResult = FALSE;
	}

	// ｵﾌﾞｼﾞｪｸﾄ消去
	for ( i=0; i<obDXFdata.GetSize(); i++ )
		delete	obDXFdata[i];
	obDXFdata.RemoveAll();

	if ( bResult ) {
		strMsg.Format(IDS_ANA_FILEOUTPUT, m_strDXFFileName);
		AfxMessageBox(strMsg, MB_OK|MB_ICONINFORMATION);
	}

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

void CNCDoc::ReadThumbnail(LPCTSTR lpszPathName)
{
	// ﾌｧｲﾙを開く
	OnOpenDocument(lpszPathName);
	SetPathName(lpszPathName, FALSE);
#ifdef _DEBUG
	printf("CNCDoc::ReadThumbnail() File =%s\n", lpszPathName);
	printf("Block=%Id\n", GetNCBlockSize());
#endif
	if ( !ValidBlockCheck() ) {
#ifdef _DEBUG
		printf("ValidBlockCheck() Error\n");
#endif
		return;
	}
	// 変換ｽﾚｯﾄﾞ起動
	NCVCTHREADPARAM	param;
	param.pParent	= NULL;
	param.pDoc		= this;
	param.wParam	= NULL;
	param.lParam	= NULL;
	NCDtoXYZ_Thread(&param);	// ｽﾚｯﾄﾞで呼び出す必要なし

	if ( !ValidDataCheck() ) {
		m_rcMax.SetRectEmpty();
		m_bDocFlg.set(NCDOC_ERROR);
		// error through
	}
	else {
		// 占有矩形調整
		m_rcMax.NormalizeRect();
		// ｴﾗｰﾌﾗｸﾞ解除
		m_bDocFlg.reset(NCDOC_ERROR);
	}

	// 変換後のﾃﾞｰﾀ設定
	m_nTraceDraw = GetNCsize();
	// ｻﾑﾈｲﾙ表示に不要なﾃﾞｰﾀの消去
	ClearBlockData();
	DeleteMacroFile();
}

/////////////////////////////////////////////////////////////////////////////
// CNCDoc クラスのオーバライド関数

BOOL CNCDoc::OnOpenDocument(LPCTSTR lpszPathName) 
{
#ifdef _DEBUG_FILEOPEN		// NCVC.h
	extern	CTime	dbgtimeFileOpen;	// NCVC.cpp
	CTime	t2 = CTime::GetCurrentTime();
	CTimeSpan ts = t2 - dbgtimeFileOpen;
	CString	strTime( ts.Format("%H:%M:%S") );
	printf("CNCDoc::OnOpenDocument() Start %s\n", LPCTSTR(strTime));
	dbgtimeFileOpen = t2;
#endif
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
		bResult = __super::OnOpenDocument(lpszPathName);
		if ( bResult ) {
			// __super::GetPathName() は
			// OnOpenDocument() 終了後にﾌﾚｰﾑﾜｰｸが設定するので使えない
			m_strCurrentFile = lpszPathName;
		}
	}

	if ( bResult && !IsThumbnail() ) {
		// ﾌｧｲﾙ読み込み後のﾁｪｯｸ
		bResult = SerializeAfterCheck();
		if ( bResult ) {
			// ﾄﾞｷｭﾒﾝﾄ変更通知ｽﾚｯﾄﾞの生成
			POSITION	pos = GetFirstViewPosition();
			ASSERT( pos );
			OnOpenDocumentSP(lpszPathName, GetNextView(pos)->GetParentFrame());	// CDocBase
		}
	}

	if ( !IsThumbnail() ) {
		// ﾒｲﾝﾌﾚｰﾑのﾌﾟﾛｸﾞﾚｽﾊﾞｰ初期化
		AfxGetNCVCMainWnd()->GetProgressCtrl()->SetPos(0);
	}

	return bResult;
}

BOOL CNCDoc::OnSaveDocument(LPCTSTR lpszPathName) 
{
	// ﾄﾞｷｭﾒﾝﾄ変更通知ｽﾚｯﾄﾞの終了
	OnCloseDocumentSP();	// CDocBase

	if ( GetPathName().CompareNoCase(lpszPathName) != 0 ) {
		CDocument* pDoc = AfxGetNCVCApp()->GetAlreadyDocument(TYPE_NCD, lpszPathName);
		if ( pDoc )
			pDoc->OnCloseDocument();	// 既に開いているﾄﾞｷｭﾒﾝﾄを閉じる
	}

	// 保存処理
	BOOL bResult = __super::OnSaveDocument(lpszPathName);

	// ﾄﾞｷｭﾒﾝﾄ変更通知ｽﾚｯﾄﾞの生成
	if ( bResult ) {
		POSITION	pos = GetFirstViewPosition();
		ASSERT( pos );
		OnOpenDocumentSP(lpszPathName, GetNextView(pos)->GetParentFrame());
	}

	return bResult;
}

void CNCDoc::OnCloseDocument() 
{
/*
	各ﾋﾞｭｰの OnDestroy() よりも先に呼ばれる
*/
#ifdef _DEBUG
	printf("CNCDoc::OnCloseDocument() Start\n");
#endif
	// ﾛｯｸｱﾄﾞｲﾝのﾁｪｯｸ
	if ( !IsLockThread() ) {	// CDocBase
#ifdef _DEBUG
		printf("AddinLock FALSE\n");
#endif
		return;
	}

	// 処理中のｽﾚｯﾄﾞを中断させる
	OnCloseDocumentSP();		// ﾌｧｲﾙ変更通知ｽﾚｯﾄﾞ
	WaitCalcThread();			// 切削時間計算ｽﾚｯﾄﾞ

	__super::OnCloseDocument();
}

void CNCDoc::SetPathName(LPCTSTR lpszPathName, BOOL bAddToMRU)
{
	if ( IsThumbnail() ) {
		// 自前で用意しないとASSERT_VALIDに引っかかる
		CString	strPath;
		::Path_Name_From_FullPath(lpszPathName, strPath, m_strTitle);
		m_strPathName = lpszPathName;
	}
	else {
		__super::SetPathName(lpszPathName, bAddToMRU);
		// --> to be CNCVCApp::AddToRecentFileList()
		m_pRecentViewInfo = AfxGetNCVCApp()->GetRecentViewInfo();
		ASSERT( m_pRecentViewInfo );
#ifdef _DEBUG
		printf("CNCDoc::SetPathName() %s OK\n", lpszPathName);
#endif
	}
}

void CNCDoc::OnChangedViewList()
{
	// ｻﾑﾈｲﾙ表示ﾓｰﾄﾞでﾋﾞｭｰを切り替えるとき、
	// ﾄﾞｷｭﾒﾝﾄが delete this してしまうのを防止する
	if ( !IsThumbnail() )
		__super::OnChangedViewList();
}

/////////////////////////////////////////////////////////////////////////////
// CNCDoc シリアライゼーション

void CNCDoc::Serialize(CArchive& ar)
{
	extern	LPCTSTR	gg_szCRLF;	// "\r\n"
#ifdef _DEBUG
	printf("CNCDoc::Serialize() Start\n");
#endif

	if ( ar.IsStoring() ) {
		CProgressCtrl*	pProgress = AfxGetNCVCMainWnd()->GetProgressCtrl();
		// ﾌｧｲﾙ保存
		pProgress->SetRange32(0, 100);
		pProgress->SetPos(0);
		int		p = 0;
		INT_PTR	n;
		INT_PTR nLoop = GetNCBlockSize();
		for ( INT_PTR i=0; i<nLoop; i++ ) {
			ar.WriteString(m_obBlock[i]->GetStrBlock()+gg_szCRLF);
			n = i*100/nLoop;
			if ( n >= p ) {
				while ( n >= p )
					p += 10;
				pProgress->SetPos(p);	// 10%ずつ
			}
		}
		pProgress->SetPos(0);
		return;
	}

	// ﾌｧｲﾙ読み込み
	SerializeBlock(ar, m_obBlock, 0);
}

void CNCDoc::SerializeBlock
	(CArchive& ar, CNCblockArray& obBlock, DWORD dwFlags)
{
	CString		strBlock;
	CNCblock*	pBlock = NULL;
	int			p = 0;
	ULONGLONG	n;

	ULONGLONG	dwSize = ar.GetFile()->GetLength();		// ﾌｧｲﾙｻｲｽﾞ取得
	ULONGLONG	dwPosition = 0;
	CProgressCtrl* pProgress = IsThumbnail() || dwFlags&NCF_AUTOREAD ?
						NULL : AfxGetNCVCMainWnd()->GetProgressCtrl();

	if ( pProgress ) {
		// ﾒｲﾝﾌﾚｰﾑのﾌﾟﾛｸﾞﾚｽﾊﾞｰ準備
		pProgress->SetRange32(0, 100);
		pProgress->SetPos(0);
	}

	try {
		while ( ar.ReadString(strBlock) ) {
			if ( pProgress ) {
				// ﾌﾟﾛｸﾞﾚｽﾊﾞｰの表示
				dwPosition += strBlock.GetLength() + 2;	// 改行ｺｰﾄﾞ分
				n = dwPosition*100/dwSize;
				if ( n >= p ) {
					while ( n >= p )
						p += 10;
					pProgress->SetPos(p);	// 10%ずつ
				}
			}
			// 行番号とGｺｰﾄﾞの分別(XYZ...含む)
			pBlock = new CNCblock(strBlock, dwFlags);
			obBlock.Add(pBlock);
#ifdef _DEBUGOLD
			printf("LineCnt=%d Line=%s Gcode=%s\n", nCnt,
				LPCTSTR(pBlock->GetStrLine()), LPCTSTR(pBlock->GetStrGcode()) );
#endif
			// ｻﾑﾈｲﾙﾓｰﾄﾞで規程件数を超えたら、読み込みを中断
			if ( IsThumbnail() && GetNCBlockSize()>=THUMBNAIL_MAXREADBLOCK )
				break;
			pBlock = NULL;
		}
		if ( pProgress )
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
	printf("CNCDoc::SerializeAfterCheck()\n");
#endif
	// ﾃﾞｰﾀﾁｪｯｸ
#ifdef _DEBUGOLD
	for ( i=0; i<GetNCBlockSize(); i++ )
		printf("%4d:%s\n", i, LPCTSTR(m_obBlock[i]->GetStrBlock()));
#endif
	// 変換状況案内ﾀﾞｲｱﾛｸﾞ(変換ｽﾚｯﾄﾞ生成)
	CThreadDlg	dlg(IDS_READ_NCD, this);
	if ( dlg.DoModal() != IDOK )
		return FALSE;

	// 機械情報の変更
	if ( m_bDocFlg[NCDOC_MC_CHANGE] ) {
		AfxGetNCVCMainWnd()->ChangeMachine();
		m_bDocFlg.reset(NCDOC_MC_CHANGE);	// one shot
	}

	// ﾜｲﾔﾓｰﾄﾞにおけるUV軸ｵﾌﾞｼﾞｪｸﾄの生成
	if ( m_bDocFlg[NCDOC_WIRE] ) {
		CThreadDlg	dlg(IDS_UVTAPER_NCD, this);
		if ( dlg.DoModal() != IDOK )
			return FALSE;
	}

	// 補正座標計算
	if ( m_bDocFlg[NCDOC_REVISEING] ) {
		CThreadDlg	dlg(IDS_CORRECT_NCD, this);
		if ( dlg.DoModal() != IDOK )
			return FALSE;
	}

	// 切削時間計算ｽﾚｯﾄﾞ開始
	CreateCutcalcThread();

	// NC情報の小数点表記
	if ( m_bDocFlg[NCDOC_DECIMAL4] ) {
		m_nDecimalID = IDCV_VALFORMAT4;
	}

	// 占有矩形調整
	m_rcMax.NormalizeRect();
	m_rcWork.NormalizeRect();
	if ( m_bDocFlg[NCDOC_LATHE] ) {
		if ( m_bDocFlg[NCDOC_COMMENTWORK_R] ) {
			m_rcWork.high = m_rcWorkCo.high;
			m_rcWork.low  = m_rcWorkCo.low;
		}
		if ( m_bDocFlg[NCDOC_COMMENTWORK_Z] ) {
			m_rcWork.left  = m_rcWorkCo.left;
			m_rcWork.right = m_rcWorkCo.right;
		}
	}
	else {
#ifdef USE_KODATUNO
		if ( m_bDocFlg[NCDOC_WORKFILE] && m_kbList ) {
			CalcWorkFileRect();		// IGES/STEPの占有矩形を計算
			m_rcWork = m_rcWorkCo;
		}
		else if ( m_bDocFlg[NCDOC_COMMENTWORK] )
#else
		if ( m_bDocFlg[NCDOC_COMMENTWORK] )
#endif
			m_rcWork = m_rcWorkCo;
		else
			m_rcWorkCo = m_rcWork;	// 指示がなければﾃﾞｰﾀを保存
	}

	// 最終ﾁｪｯｸ
	if ( !ValidDataCheck() ) {
		// デフォルト矩形をセット
		m_rcMax = m_rcWork = m_rcWorkCo = g_rcDefRect;
		m_bDocFlg.set(NCDOC_ERROR);
		// error through
	}
	else {
		// ｴﾗｰﾌﾗｸﾞ解除
		m_bDocFlg.reset(NCDOC_ERROR);
	}

#ifdef _DEBUG
	printf("m_rcMax  left =%f top   =%f\n", m_rcMax.left, m_rcMax.top);
	printf("m_rcMax  right=%f bottom=%f\n", m_rcMax.right, m_rcMax.bottom);
	printf("m_rcMax  low  =%f high  =%f\n", m_rcMax.low, m_rcMax.high);
	printf("--- cut\n");
	printf("m_rcWork left =%f top   =%f\n", m_rcWork.left, m_rcWork.top);
	printf("m_rcWork right=%f bottom=%f\n", m_rcWork.right, m_rcWork.bottom);
	printf("m_rcWork low  =%f high  =%f\n", m_rcWork.low, m_rcWork.high);
#endif

	// NCﾃﾞｰﾀの最大値取得
	m_nTraceDraw = GetNCsize();

	// G10で仮登録された工具情報を削除
	AfxGetNCVCApp()->GetMCOption()->ReductionTools(TRUE);
	// 一時展開のﾏｸﾛﾌｧｲﾙを消去
	DeleteMacroFile();

	return TRUE;
}

BOOL CNCDoc::ValidBlockCheck(void)
{
	for ( int i=0; i<GetNCBlockSize(); i++ ) {
		if ( m_obBlock[i]->GetStrGcode().GetLength() > 0 )
			return TRUE;
	}
	return FALSE;
}

BOOL CNCDoc::ValidDataCheck(void)
{
	for ( int i=0; i<GetNCsize(); i++ ) {
		if ( GetNCdata(i)->GetType() != NCDBASEDATA )
			return TRUE;
	}
	return FALSE;
}

#ifdef USE_KODATUNO
void CNCDoc::CalcWorkFileRect(void)
{
	BODY*		pBody;
	CPoint3F	pt;
//	Coord		cd;		// Kodatuno座標クラス
//	int		i, j, k, nLoop = m_kbList->getNum();
	int		i, nLoop = m_kbList->getNum();

	m_rcWorkCo.SetRectMinimum();

	// NURBS曲線のみを対象にｺﾝﾄﾛｰﾙﾎﾟｲﾝﾄから占有矩形を推測
	for ( i=0; i<nLoop; i++ ) {
		pBody = (BODY *)m_kbList->getData(i);
//		for ( j=0; j<pBody->TypeNum[_NURBSC]; j++ ) {
//			for ( k=0; k<pBody->NurbsC[j].K; k++ ) {
//				cd = pBody->NurbsC[j].cp[k];
//				pt.SetPoint((float)cd.x, (float)cd.y, (float)cd.z);
//				m_rcWorkCo |= pt;
//			}
//		}
#ifdef _DEBUG
		printf("CNCDoc::CalcWorkFileRect() body->MaxCoord=%f\n", pBody->MaxCoord);
		printf(" minCoord=(%f,%f,%f)\n", pBody->minmaxCoord[0].x, pBody->minmaxCoord[0].y, pBody->minmaxCoord[0].z);
		printf(" maxCoord=(%f,%f,%f)\n", pBody->minmaxCoord[1].x, pBody->minmaxCoord[1].y, pBody->minmaxCoord[1].z);
#endif
		// ライブラリ側を少し改造
		pt.SetPoint((float)pBody->minmaxCoord[0].x, (float)pBody->minmaxCoord[0].y, (float)pBody->minmaxCoord[0].z);
		m_rcWorkCo |= pt;
		pt.SetPoint((float)pBody->minmaxCoord[1].x, (float)pBody->minmaxCoord[1].y, (float)pBody->minmaxCoord[1].z);
		m_rcWorkCo |= pt;
	}
#ifdef _DEBUG
	printf("(%f,%f)-(%f,%f)\n", m_rcWorkCo.left, m_rcWorkCo.top, m_rcWorkCo.right, m_rcWorkCo.bottom);
	printf("(%f,%f)\n", m_rcWorkCo.low, m_rcWorkCo.high);
#endif
}
#endif

BOOL CNCDoc::SerializeInsertBlock
	(LPCTSTR lpszFileName, INT_PTR nInsert, DWORD dwFlags/*=0*/)
{
/*
	MFC\SRC\DOCCORE.CPP の __super::OnOpenDocument を参考
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
		obBlock.SetSize(0, 4096);
		// ﾌｧｲﾙ読み込み
		SerializeBlock(ar, obBlock, dwFlags);
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
		CNCWorkDlg*	pDlg = new CNCWorkDlg(ID_NCVIEW_WORKRECT, m_bDocFlg[NCDOC_CYLINDER] ? 1 : 0);
		pDlg->Create(AfxGetMainWnd());
		AfxGetNCVCMainWnd()->SetModelessDlg(MLD_NCWORK, pDlg);
	}
	catch ( CMemoryException* e ) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
	}
}

void CNCDoc::OnUpdateMaxRect(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_bDocFlg[NCDOC_ERROR] ? FALSE : TRUE);
	pCmdUI->SetCheck(m_bDocFlg[NCDOC_MAXRECT]);
}

void CNCDoc::OnMaxRect() 
{
	m_bDocFlg.flip(NCDOC_MAXRECT);
	// ﾋﾞｭｰの更新
	UpdateAllViews(NULL, UAV_DRAWMAXRECT);
}
