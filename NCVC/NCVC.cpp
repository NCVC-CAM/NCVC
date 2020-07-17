// NCVC.cpp : アプリケーションのクラス動作を定義します。
//

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "AboutDlg.h"
#include "NCChild.h"
#include "NCDoc.h"
#include "NCViewTab.h"
#include "DXFChild.h"
#include "DXFDoc.h"
#include "DXFView.h"
#include "MCSetup.h"
#include "ViewSetup.h"
#include "DxfSetup.h"
#include "MKNCSetup.h"
#include "MKLASetup.h"
#include "ExecSetupDlg.h"
#include "ExtensionDlg.h"
#include "ThreadDlg.h"
#include "ThumbnailDlg.h"
#include "NCVCaddin.h"
#include "NCVCaddinIF.h"
#include "AddinDlg.h"
#include "SplashWnd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#ifdef _DEBUG_FILEOPEN
CTime	dbgtimeFileOpen;
#endif
#endif

extern	int		g_nProcesser = 1;		// ﾌﾟﾛｾｯｻ数(->検索ｽﾚｯﾄﾞ数)
extern	LPTSTR	g_pszDelimiter = NULL;	// g_szGdelimiter[] + g_szNdelimiter[]
extern	LPTSTR	g_pszExecDir = NULL;	// 実行ﾃﾞｨﾚｸﾄﾘ
static	LPCTSTR	g_szHelp = "NCVC.pdf";	// HelpFile
extern	DWORD	g_dwCamVer = NCVCSERIALVERSION;	// CAMﾌｧｲﾙ Ver.No.
extern	double	_TABLECOS[ARCCOUNT] = {	// 事前に計算された三角関数の結果
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
},
				_TABLESIN[ARCCOUNT] = {	// 初期化しないと extern エラー
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
/*
	ﾌﾟﾛﾊﾟﾃｨｼｰﾄのﾗｽﾄﾍﾟｰｼﾞ::小さい情報のため，(ﾚｼﾞｽﾄﾘへの)保存はしない
		NCVC稼働中に限り保持する
*/
extern	int		g_nLastPage_DXFSetup = 0;		// DXFｵﾌﾟｼｮﾝ
extern	int		g_nLastPage_MCSetup = 0;		// 工作機械ｵﾌﾟｼｮﾝ
extern	int		g_nLastPage_NCMake = 0;			// NC生成ｵﾌﾟｼｮﾝ
extern	int		g_nLastPage_NCMakeLathe = 0;	// 旋盤用NC生成ｵﾌﾟｼｮﾝ
extern	int		g_nLastPage_ViewSetup = 0;		// 表示系ｾｯﾄｱｯﾌﾟ

/////////////////////////////////////////////////////////////////////////////
// CNCVCApp

BEGIN_MESSAGE_MAP(CNCVCApp, CWinAppEx)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
	ON_COMMAND(ID_FILE_THUMBNAIL, OnFileThumbnail)
	ON_COMMAND(ID_FILE_CLANDOP, OnFileCloseAndOpen)
	ON_COMMAND(ID_HELP, OnHelp)
	ON_COMMAND(ID_HELP_ADDIN, OnHelpAddin)
	ON_COMMAND(ID_OPTION_MC, OnOptionMC)
	ON_COMMAND(ID_OPTION_EDITMC, OnOptionEditMC)
	ON_COMMAND(ID_OPTION_DXF, OnOptionDXF)
	ON_COMMAND(ID_OPTION_MAKENC, OnOptionMakeNC)
	ON_COMMAND(ID_OPTION_EDITNC, OnOptionEditNC)
	ON_COMMAND(ID_OPTION_EXEC, OnOptionExec)
	ON_COMMAND(ID_OPTION_EXT, OnOptionExt)
	ON_COMMAND(ID_OPTION_VIEW_SETUP, OnViewSetup)
	ON_COMMAND(ID_OPTION_VIEW_INPORT, OnViewSetupInport)
	ON_COMMAND(ID_OPTION_VIEW_EXPORT, OnViewSetupExport)
	ON_COMMAND(ID_WINDOW_ALLCLOSE, OnWindowAllClose)
	ON_UPDATE_COMMAND_UI(ID_OPTION_EDITNC, OnUpdateOptionEdit)
	ON_UPDATE_COMMAND_UI(ID_OPTION_EDITMC, OnUpdateOptionEdit)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNCVCApp コンストラクション

CNCVCApp::CNCVCApp()
{
	extern	LPCTSTR	g_szGdelimiter;
	extern	LPCTSTR	g_szNdelimiter;

	// ﾌﾟﾛｾｯｻ数の取得
	SYSTEM_INFO		si;
	::GetSystemInfo(&si);
	g_nProcesser = min(si.dwNumberOfProcessors, MAXIMUM_WAIT_OBJECTS);

	// 3D→2D変換の初期化
	CPoint3D::ChangeAngle(RX, RY, RZ);

	// 三角関数ﾃｰﾌﾞﾙの初期化
	double	q = 0;
	for ( int i=0; i<ARCCOUNT; i++, q+=ARCSTEP ) {
		_TABLECOS[i] = cos(q);
		_TABLESIN[i] = sin(q);
	}

	// ｱﾄﾞｲﾝIDの初期値
	m_wAddin = ADDINSTARTID;
	m_pActiveAddin = NULL;
	// EXEC-IDの初期値
	m_wExec = EXECSTARTID;

	// g_pszDelimiter への値ｾｯﾄ
	g_pszDelimiter = new TCHAR[lstrlen(g_szGdelimiter)+lstrlen(g_szNdelimiter)+1];
	lstrcat(lstrcpy(g_pszDelimiter, g_szGdelimiter), g_szNdelimiter);

	m_pOptMC		= NULL;
	m_pOptDXF		= NULL;
	m_pOptView		= NULL;
	m_pDefViewInfo	= NULL;
}

CNCVCApp::~CNCVCApp()
{
	if ( g_pszDelimiter )
		delete[]	g_pszDelimiter;
	if ( g_pszExecDir )
		delete[]	g_pszExecDir;
}

// 唯一の CNCVCApp オブジェクトです。

CNCVCApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CNCVCApp オーバーライド

BOOL CNCVCApp::InitInstance()
{
	// アプリケーション マニフェストが visual スタイルを有効にするために、
	// ComCtl32.dll Version 6 以降の使用を指定する場合は、
	// Windows XP に InitCommonControlsEx() が必要です。さもなければ、ウィンドウ作成はすべて失敗します。
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// アプリケーションで使用するすべてのコモン コントロール クラスを含めるには、
	// これを設定します。
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinAppEx::InitInstance();

	// OLE ライブラリを初期化します。
	if ( !AfxOleInit() ) {
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}
	AfxEnableControlContainer();
	// 標準初期化
	// 設定が格納されているレジストリ キーを変更します。
	SetRegistryKey(IDS_REGISTRY_KEY);
#ifdef _DEBUG
	g_dbg.printf("Processer Count=%d", g_nProcesser);
	g_dbg.printf("RegistryKey=%s", m_pszRegistryKey);
#endif
	LoadStdProfileSettings(10);	// 標準の INI ファイルのオプションをロードします (MRU を含む)
	InitialRecentViewList();	// MRUﾘｽﾄからCRecentViewInfo構築

	// VC++2008SP1以降のコントロール初期化
	InitContextMenuManager();
	InitKeyboardManager();
	InitShellManager();		// CMFCShellTreeCtrl

	// g_pszExecDir への値ｾｯﾄ
	{
		CString	strPath, strFile;
		::Path_Name_From_FullPath(m_pszHelpFilePath, strPath, strFile, FALSE);
		g_pszExecDir = new TCHAR[strPath.GetLength()+1];
		lstrcpy(g_pszExecDir, strPath);
	}

	// アプリケーション用のドキュメント テンプレートを登録します。ドキュメント テンプレート
	//  はドキュメント、フレーム ウィンドウとビューを結合するために機能します。
	m_pDocTemplate[TYPE_NCD] = new CNCVCDocTemplate(	// NC ﾄﾞｷｭﾒﾝﾄ(CMultiDocTemplate)
		IDR_NCTYPE,
		RUNTIME_CLASS(CNCDoc),
		RUNTIME_CLASS(CNCChild),
		RUNTIME_CLASS(CNCViewTab));	// CTabView
	m_pDocTemplate[TYPE_DXF] = new CNCVCDocTemplate(	// DXFﾄﾞｷｭﾒﾝﾄ
		IDR_DXFTYPE,
		RUNTIME_CLASS(CDXFDoc),
		RUNTIME_CLASS(CDXFChild),
		RUNTIME_CLASS(CDXFView));
	AddDocTemplate(m_pDocTemplate[TYPE_NCD]);
	AddDocTemplate(m_pDocTemplate[TYPE_DXF]);

	// DDE、file open など標準のシェル コマンドのコマンド ラインを解析します。
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);
	CSplashWnd::ms_bShowSplashWnd = cmdInfo.m_bShowSplash;	// CMainFrame::OnCreate()へ

	////////// CMainFrame作成前に実行
	// ﾚｼﾞｽﾄﾘ設定情報読み込み
	if ( !NCVCRegInit() )
		return FALSE;
	// ｱﾄﾞｲﾝ情報読み込み
	if ( !NCVCAddinInit(cmdInfo.m_nShellCommand) )
		return FALSE;
	//////////

	// メイン MDI フレーム ウィンドウを作成します。
	CMainFrame* pMainFrame = new CMainFrame;
	if ( !pMainFrame || !pMainFrame->LoadFrame(IDR_MAINFRAME) ) {
		delete pMainFrame;
		return FALSE;
	}
	m_pMainWnd = pMainFrame;

	// 接尾辞が存在する場合にのみ DragAcceptFiles を呼び出します。
	//  MDI アプリケーションでは、この呼び出しは、m_pMainWnd を設定した直後に発生しなければなりません。
	m_pMainWnd->DragAcceptFiles();
	EnableShellOpen();
	RegisterShellFileTypes(TRUE);

	////////// CMainFrame作成後に実行
	// 外部ｱﾌﾟﾘｹｰｼｮﾝ，ﾂｰﾙﾊﾞｰへの登録とﾒﾆｭｰの更新
	// (LoadFrame()の後でないとｱｻｰﾄｴﾗｰ)
	if ( m_liExec.GetCount() > 0 )
		AfxGetNCVCMainWnd()->SetExecButtons();
	// ｱﾄﾞｲﾝ情報に基づくﾒﾆｭｰの更新
	if ( m_obAddin.GetSize() > 0 ) {
		if ( !NCVCAddinMenu() )
			return FALSE;
		// ｱﾄﾞｲﾝ，ﾂｰﾙﾊﾞｰへの登録
		AfxGetNCVCMainWnd()->SetAddinButtons();
	}
	//////////

	// メイン ウィンドウが初期化されたので、表示と更新を行います。
	if ( !pMainFrame->RestoreWindowState() )		// CMainFrame
		m_pMainWnd->ShowWindow(m_nCmdShow);
	m_pMainWnd->UpdateWindow();

	// 全ての初期化処理が終了してから
	// コマンド ラインで指定されたディスパッチ コマンドです。アプリケーションが
	// /RegServer、/Register、/Unregserver または /Unregister で起動された場合、False を返します。

	// --- 超重要 ---
	// m_pMainWnd->UpdateWindow(); を実行してから
	// ProcessShellCommand() を処理しないと、
	// ショートカットへのＤ＆Ｄでエラーになる！！
	// ＭＦＣの雛形が間違っている
	// --------------
	if ( !ProcessShellCommand(cmdInfo) )
		return FALSE;

	return TRUE;
}

int CNCVCApp::ExitInstance() 
{
	int		i;
	POSITION	pos;

	// ﾚｼﾞｽﾄﾘ保存
	CString	strRegKey, strEntry;
	VERIFY(strRegKey.LoadString(IDS_REGKEY_NC));
	VERIFY(strEntry.LoadString(IDS_REG_NCV_VIEWPAGE));
	WriteProfileInt(strRegKey, strEntry, m_nNCTabPage);
	VERIFY(strEntry.LoadString(IDS_REG_NCV_TRACESPEED));
	WriteProfileInt(strRegKey, strEntry, m_nTraceSpeed - ID_NCVIEW_TRACE_FAST);

	// ﾃﾞﾌｫﾙﾄ描画情報の保存
	if ( m_pDefViewInfo ) {
		VERIFY(strEntry.LoadString(IDS_REG_NCV_DEFGLINFO));
		WriteProfileBinary(strRegKey, strEntry, (LPBYTE)&(m_pDefViewInfo->m), sizeof(CRecentViewInfo::VINFO));
	}

	// MRUﾘｽﾄに準拠した描画情報を保存
	WriteRecentViewList();
	for ( pos=m_liRecentView.GetHeadPosition(); pos; )
		delete	m_liRecentView.GetNext(pos);
	m_liRecentView.RemoveAll();

	// ｵﾌﾟｼｮﾝ関連の削除
	if ( m_pOptMC  )		delete	m_pOptMC;
	if ( m_pOptDXF )		delete	m_pOptDXF;
	if ( m_pOptView )		delete	m_pOptView;
	if ( m_pDefViewInfo )	delete	m_pDefViewInfo;

	// 外部ｱﾌﾟﾘｹｰｼｮﾝ情報削除
	for ( pos=m_liExec.GetHeadPosition(); pos; )
		delete	m_liExec.GetNext(pos);
	m_liExec.RemoveAll();
	m_mpExec.RemoveAll();

	// ｱﾄﾞｲﾝ情報削除
	for ( i=0; i<m_obAddin.GetSize(); i++ )
		delete	m_obAddin[i];
	WORD		wKey;
	CNCVCaddinMap*	pAddin;
	for ( pos=m_mpAddin.GetStartPosition(); pos; ) {
		m_mpAddin.GetNextAssoc(pos, wKey, pAddin);
		delete	pAddin;
	}
	m_obAddin.RemoveAll();
	m_mpAddin.RemoveAll();

	return CWinAppEx::ExitInstance();
}

void CNCVCApp::AddToRecentFileList(LPCTSTR lpszPathName)
{
	CWinAppEx::AddToRecentFileList(lpszPathName);
	
	if ( lpszPathName && lstrlen(lpszPathName)>0 )
		AddToRecentViewList(lpszPathName);
}

/////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG_FILEOPEN
CDocument* CNCVCApp::OpenDocumentFile(LPCTSTR lpszFileName)
{
	g_dbg.printf("CNCVCApp::OpenDocumentFile() Start");
	
	CTime	t1 = CTime::GetCurrentTime();
	CDocument* pDoc = CWinAppEx::OpenDocumentFile(lpszFileName);
	CTime	t2 = CTime::GetCurrentTime();

	CTimeSpan ts = t2 - t1;
	CString	strTime( ts.Format("%H:%M:%S") );
	g_dbg.printf("CWinAppEx::OpenDocumentFile() End = %s", strTime);
	dbgtimeFileOpen = t2;

	return pDoc;
}
#endif

/////////////////////////////////////////////////////////////////////////////
// CNCVCApp ﾒﾝﾊﾞ関数

BOOL CNCVCApp::NCVCRegInit(void)
{
#ifdef _DEBUG
	CMagaDbg	dbg("NCVCRegInit()\nStart");
#endif
	CString		strRegKey, strEntry;
	CRecentViewInfo::VINFO*	bi;
	UINT					n;

	// 外部ｱﾌﾟﾘｹｰｼｮﾝの数(数勘定は下位互換のため)
	VERIFY(strRegKey.LoadString(IDS_REGKEY_SETTINGS));
	VERIFY(strEntry.LoadString(IDS_REG_EXEC));
	int	nCnt = (int)GetProfileInt(strRegKey, strEntry, -1);
	if ( nCnt > 0 ) {
		if ( !NCVCRegInit_NewExec(strRegKey, nCnt) )
			return FALSE;
	}
	else {
		if ( !NCVCRegInit_OldExec(strRegKey) )
			return FALSE;
		SaveExecData();		// ﾃﾞｰﾀの移行
	}
	// 外部ｱﾌﾟﾘｹｰｼｮﾝのｺﾏﾝﾄﾞIDとﾏｯﾌﾟの作成
	if ( !CreateExecMap() )
		return FALSE;

	// NCViewTab管理情報
	// NCViewTab起動時ではなく，ﾌﾟﾛｸﾞﾗﾑ開始情報として取得
	VERIFY(strRegKey.LoadString(IDS_REGKEY_NC));
	VERIFY(strEntry.LoadString(IDS_REG_NCV_TRACESPEED));
	m_nTraceSpeed = GetProfileInt(strRegKey, strEntry, -1) + ID_NCVIEW_TRACE_FAST;
	if ( m_nTraceSpeed<ID_NCVIEW_TRACE_FAST || m_nTraceSpeed>ID_NCVIEW_TRACE_LOW )
		m_nTraceSpeed = ID_NCVIEW_TRACE_LOW;
	VERIFY(strEntry.LoadString(IDS_REG_NCV_VIEWPAGE));
	m_nNCTabPage = GetProfileInt(strRegKey, strEntry, 0);
	if ( m_nNCTabPage < 0 || m_nNCTabPage > NCVIEW_OPENGL/*GetPageCount()*/ )
		m_nNCTabPage = 0;

	try {
		// OpenGL Default View Info
		VERIFY(strEntry.LoadString(IDS_REG_NCV_DEFGLINFO));
		if ( GetProfileBinary(strRegKey, strEntry, (LPBYTE*)&bi, &n) ) {
			ASSERT( n==sizeof(CRecentViewInfo::VINFO) );
			m_pDefViewInfo = new CRecentViewInfo(NULL);
			m_pDefViewInfo->SetViewInfo(bi->objectXform, bi->rcView, bi->ptCenter);
			delete	bi;		// GetProfileBinary() Specification
		}

		// ｵﾌﾟｼｮﾝの構築
		// --- NC設定
		m_pOptMC = new CMCOption;
		// --- DXF設定
		m_pOptDXF = new CDXFOption;
		// --- View設定
		m_pOptView = new CViewOption;
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		return FALSE;
	}

	// ﾚｼﾞｽﾄﾘの旧ﾃﾞｰﾀ削除
	NCVCRegOld();

	return TRUE;
}

BOOL CNCVCApp::NCVCRegInit_OldExec(CString& strRegKey)
{
	CString	strEntry;
	CString	strEditor, strCAD;

	// 標準ｴﾃﾞｨﾀ
	VERIFY(strEntry.LoadString(IDS_REG_NOTEPAD));
	strEditor = GetProfileString(strRegKey, strEntry, strEntry);
	if ( strEditor == strEntry ) {
		TCHAR	szTmp[_MAX_PATH];
		::GetWindowsDirectory(szTmp, sizeof(szTmp));
		strEditor.Format("%s\\%s.exe", szTmp, strEntry);
	}
	// 標準CAD
	VERIFY(strEntry.LoadString(IDS_REG_CAD));
	strCAD = GetProfileString(strRegKey, strEntry);
	
	CExecOption*	pExec = NULL;
	try {
		pExec = new CExecOption(strEditor, TYPE_NCD);
		m_liExec.AddTail(pExec);
		if ( !strCAD.IsEmpty() ) {
			pExec = new CExecOption(strCAD, TYPE_DXF);
			m_liExec.AddTail(pExec);
		}
	}
	catch (CMemoryException* e) {
		if ( pExec )
			delete	pExec;
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		return FALSE;
	}

	return TRUE;
}

BOOL CNCVCApp::NCVCRegInit_NewExec(CString& strRegKey, int nCnt)
{
	CString			strEntry, strResult, strFormat;
	CExecOption*	pExec = NULL;

	VERIFY(strEntry.LoadString(IDS_REG_EXEC));
	try {
		for ( int i=0; i<nCnt; i++ ) {
			strFormat.Format(IDS_COMMON_FORMAT, strEntry, i);
			strResult = GetProfileString(strRegKey, strFormat);
			if ( !strResult.IsEmpty() ) {
				pExec = new CExecOption(strResult);
				m_liExec.AddTail(pExec);
			}
		}
	}
	catch (CMemoryException* e) {
		if ( pExec )
			delete	pExec;
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		return FALSE;
	}

	return TRUE;
}

void CNCVCApp::NCVCRegOld(void)
{
	extern	LPCTSTR	gg_szRegKey;
	int		i;
	CRegKey	reg;

	// 通信関係の設定削除
	CString	strEntry;
	VERIFY(strEntry.LoadString(IDS_REGKEY_SETTINGS));
	// --- "Software\MNCT-S\NCVC\Settings"
	if ( reg.Open(HKEY_CURRENT_USER, gg_szRegKey+strEntry) == ERROR_SUCCESS ) {
		VERIFY(strEntry.LoadString(IDS_REG_COMPORT));
		reg.DeleteValue(strEntry);
		for ( i=0; i<4; i++ ) {
			strEntry.Format(IDS_REG_COMFORM, i);
			reg.DeleteValue(strEntry);
		}
		// 旧ﾊﾞｰｼﾞｮﾝの外部ｱﾌﾟﾘｹｰｼｮﾝ設定削除
		VERIFY(strEntry.LoadString(IDS_REG_NOTEPAD));
		reg.DeleteValue(strEntry);
		VERIFY(strEntry.LoadString(IDS_REG_CAD));
		reg.DeleteValue(strEntry);
		reg.Close();
	}
	// ﾋﾞｭｰの状態を保存するﾚｼﾞｽﾄﾘ関連(旧 MainFrame.cpp)の削除
	static	const	int		s_nRegViewKey[] = {
		IDS_REGKEY_NC, IDS_REGKEY_DXF
	};
	VERIFY(strEntry.LoadString(IDS_REGKEY_WINDOW));
	// --- "Software\MNCT-S\NCVC\Window"
	if ( reg.Open(HKEY_CURRENT_USER, gg_szRegKey+strEntry) == ERROR_SUCCESS ) {
		for ( i=0; i<SIZEOF(s_nRegViewKey); i++ ) {
			VERIFY(strEntry.LoadString(s_nRegViewKey[i]));
			reg.DeleteSubKey(strEntry);
		}
		reg.Close();
	}
}

void CNCVCApp::InitialRecentViewList(void)
{
	CString		strRegKey, strEntry;
	CRecentViewInfo*		pInfo;
	CRecentViewInfo::VINFO*	bi;
	UINT					n;

	VERIFY(strRegKey.LoadString(IDS_REGKEY_RECENTVIEWINFO));

	for ( int i=0; i<m_pRecentFileList->GetSize(); i++ ) {
		pInfo = new CRecentViewInfo(m_pRecentFileList->operator[](i));
		// Recent View List のﾚｼﾞｽﾄﾘ読み込み
		strEntry.Format(m_pRecentFileList->m_strEntryFormat, i+1);	// File%d
		if ( GetProfileBinary(strRegKey, strEntry, (LPBYTE*)&bi, &n) ) {
			ASSERT( n==sizeof(CRecentViewInfo::VINFO) );
			pInfo->SetViewInfo(bi->objectXform, bi->rcView, bi->ptCenter);
			delete	bi;		// GetProfileBinary() Specification
		}
		// ﾘｽﾄ登録
		m_liRecentView.AddTail(pInfo);
	}
}

void CNCVCApp::WriteRecentViewList(void)
{
	int			nCnt = 0;
	POSITION	pos;
	CString		strRegKey, strEntry;
	CRecentViewInfo*	pInfo;

	VERIFY(strRegKey.LoadString(IDS_REGKEY_RECENTVIEWINFO));

	for ( pos=m_liRecentView.GetHeadPosition(); pos && nCnt<10; nCnt++) {
		strEntry.Format(m_pRecentFileList->m_strEntryFormat, nCnt+1);
		pInfo = m_liRecentView.GetNext(pos);
		if ( pInfo->m_bInfo ) {
			WriteProfileBinary(strRegKey, strEntry, (LPBYTE)&(pInfo->m), sizeof(CRecentViewInfo::VINFO));
		}
		else {
			// ｷｰの消去
			WriteProfileString(strRegKey, strEntry, NULL);
		}
	}
}

void CNCVCApp::AddToRecentViewList(LPCTSTR lpszPathName)
{
	CRecentViewInfo*	pInfo;
	POSITION			pos1, pos2;

	for ( pos1=m_liRecentView.GetHeadPosition(); (pos2=pos1); ) {
		pInfo = m_liRecentView.GetNext(pos1);
		if ( pInfo->m_strFile.CompareNoCase(lpszPathName) == 0 ) {
			// 文字列があれば，それを消して先頭へ
			m_liRecentView.RemoveAt(pos2);
			m_liRecentView.AddHead(pInfo);
			return;
		}
	}
	
	// なければ先頭に追加
	pInfo = new CRecentViewInfo(lpszPathName);
	m_liRecentView.AddHead(pInfo);
}

void CNCVCApp::SetDefaultViewInfo(const GLdouble objectXform[4][4])
{
	CRect3D	rcRect;		// dummy
	CPointD	ptCenter;

	if ( !m_pDefViewInfo )
		m_pDefViewInfo = new CRecentViewInfo(NULL);

	m_pDefViewInfo->SetViewInfo(objectXform, rcRect, ptCenter);
}

void CNCVCApp::SaveExecData(void)
{
	CString		strRegKey, strEntry, strFormat;
	VERIFY(strRegKey.LoadString(IDS_REGKEY_SETTINGS));
	VERIFY(strEntry.LoadString(IDS_REG_EXEC));

	// 外部ｱﾌﾟﾘｹｰｼｮﾝ登録
	WriteProfileInt(strRegKey, strEntry, m_liExec.GetCount());
	POSITION	pos = m_liExec.GetHeadPosition();
	for ( int i=0; pos; i++ ) {
		strFormat.Format(IDS_COMMON_FORMAT, strEntry, i);
		WriteProfileString(strRegKey, strFormat, m_liExec.GetNext(pos)->GetStringData());
	}
}

BOOL CNCVCApp::CreateExecMap(void)
{
	CExecOption*	pExec;
	m_mpExec.RemoveAll();
	m_wExec = EXECSTARTID;

	try {
		for ( POSITION pos=m_liExec.GetHeadPosition(); pos; m_wExec++ ) {
			// ﾒﾆｭｰIDに対するﾏｯﾌﾟ作成
			pExec = m_liExec.GetNext(pos);
			m_mpExec.SetAt(m_wExec, pExec);
			pExec->SetMenuID(m_wExec);
		}
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		return FALSE;
	}

	return TRUE;
}

BOOL CNCVCApp::NCVCAddinInit(int nShellCommand)
{
	extern	LPCTSTR	gg_szCat;
#ifdef _DEBUG_OLD
	CMagaDbg	dbg("NCVCAddinInit()\nStart");
	dbg.print("nShellCommand=", FALSE);
	switch ( nShellCommand ) {
	case CCommandLineInfo::FileNew:
		dbg.print("FileNew");
		break;
	case CCommandLineInfo::FileOpen:
		dbg.print("FileOpen");
		break;
	case CCommandLineInfo::FilePrint:
		dbg.print("FilePrint");
		break;
	case CCommandLineInfo::FilePrintTo:
		dbg.print("FilePrintTo");
		break;
	case CCommandLineInfo::FileDDE:
		dbg.print("FileDDE");
		break;
	case CCommandLineInfo::FileNothing:
		dbg.print("FileNothing");
		break;
	default:
		dbg.print("???");
		break;
	}
#endif
	// 外部ｱﾄﾞｲﾝ登録前に，ReadDXF()を登録
	CString	strFilter;
	VERIFY(strFilter.LoadString(IDS_DXF_FILTER));
	m_pDocTemplate[TYPE_DXF]->AddExtensionFunc(strFilter.Left(3), ReadDXF);	// "DXF"

	struct NCVCINITIALIZE_BUF {
		DWORD		dwSize;
		char		szDummy[4096];
	};
	struct NCVCINITIALIZE_OLD {
		DWORD		dwSize;
		DWORD		dwType;
		LPCTSTR		lpszMenuName[NCVCADIN_TYPESIZE];
		LPCTSTR		lpszFuncName[NCVCADIN_TYPESIZE];
		LPCTSTR		lpszAddinName;
		LPCTSTR		lpszCopyright;
		LPCTSTR		lpszSupport;
		LPCTSTR		lpszComment;
	};

	int		i;
	WIN32_FIND_DATA	fd;
	HANDLE			hFind;
	HMODULE			hLib;
	CNCVCaddinIF*	pAddin;
	PFNNCVCINITIALIZE	pfnInitialize;
	NCVCINITIALIZE_BUF	ncibuf;
	NCVCINITIALIZE		nci;
	NCVCINITIALIZE_OLD	nciold;

	CString	strDLLPath(g_pszExecDir),
			strDLL(strDLLPath+"*.dll"), strErrDLL;

	DWORD	dwFlags = FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_SYSTEM;
	if ( (hFind=::FindFirstFile(strDLL, &fd)) == INVALID_HANDLE_VALUE )
		return TRUE;	// 正常終了

	// DLL検査(fd.cFileNameにはﾌｧｲﾙ名のみ入るがｶﾚﾝﾄ検索には問題ない)
	try {
		do {
			if ( fd.dwFileAttributes & dwFlags )
				continue;
#ifdef _DEBUG
			g_dbg.printf("DLL File = %s", fd.cFileName);
#endif
			hLib = (HMODULE)::LoadLibrary(fd.cFileName);
			DBGASSERT(hLib, "LoadLibrary() NULL!");	
			if ( !hLib )
				continue;
			// 初期化関数のｱﾄﾞﾚｽ取得
			pfnInitialize = (PFNNCVCINITIALIZE)::GetProcAddress(hLib, "NCVC_Initialize");
			if ( !pfnInitialize ) {
#ifdef _DEBUG
				g_dbg.printf("GetProcAddress() NULL");
				::NC_FormatMessage();	// GetLastError()
#endif
				::FreeLibrary(hLib);
				continue;
			}
			// DLL のｲﾆｼｬﾙ関数呼び出し
			::ZeroMemory(&ncibuf, sizeof(NCVCINITIALIZE_BUF));
			if ( !(*pfnInitialize)( (LPNCVCINITIALIZE)(&ncibuf) ) ) {
#ifdef _DEBUG
				g_dbg.printf("NC-Addin Initialize Faild");
#endif
				::FreeLibrary(hLib);
				continue;
			}
			// 初期化構造体のｻｲｽﾞﾁｪｯｸ
			switch ( ncibuf.dwSize ) {
			case sizeof(NCVCINITIALIZE_OLD):
				memcpy(&nciold, &ncibuf, sizeof(NCVCINITIALIZE_OLD));
				// ﾒﾝﾊﾞごとにｺﾋﾟｰ
				nci.dwType		= nciold.dwType;
				nci.nToolBar	= -1;	// new
				for ( i=0; i<NCVCADIN_TYPESIZE; i++ ) {
					nci.lpszMenuName[i] = nciold.lpszMenuName[i];
					nci.lpszFuncName[i] = nciold.lpszFuncName[i];
				}
				nci.lpszAddinName	= nciold.lpszAddinName;
				nci.lpszCopyright	= nciold.lpszCopyright;
				nci.lpszSupport		= nciold.lpszSupport;
				nci.lpszComment		= nciold.lpszComment;
				break;
			case sizeof(NCVCINITIALIZE):
				memcpy(&nci, &ncibuf, sizeof(NCVCINITIALIZE));
				break;
			default:
				if ( !strErrDLL.IsEmpty() )
					strErrDLL += gg_szCat;
				strErrDLL += fd.cFileName;
				::FreeLibrary(hLib);
				continue;
			}
#ifdef _DEBUG
			g_dbg.printf("DLL Type = 0x%04x", nci.dwType);
#endif
			// ｱﾄﾞｲﾝ登録
			pAddin = new CNCVCaddinIF(hLib, &nci, strDLLPath+fd.cFileName);
			m_obAddin.Add(pAddin);
		} while ( ::FindNextFile(hFind, &fd) );
	}
	catch (CMemoryException* e) {
		if ( pAddin )
			delete	pAddin;
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		::FreeLibrary(hLib);
		::FindClose(hFind);
		return FALSE;
	}

	::FindClose(hFind);

//	if ( !strErrDLL.IsEmpty() && nShellCommand != CCommandLineInfo::FileDDE ) {
//		strDLL.Format(IDS_ERR_DLL, strErrDLL);
//		AfxMessageBox(strDLL, MB_OK|MB_ICONEXCLAMATION);
//	}

	return TRUE;
}

BOOL CNCVCApp::NCVCAddinMenu(void)
{
	static	LPCTSTR	lpszAddin = "ｱﾄﾞｲﾝ(&A)";
	extern	const	DWORD	g_dwAddinType[];	// NCVCaddinIF.cpp
#ifdef _DEBUG
	CMagaDbg	dbg("NCVCAddinMenu()\nStart");
#endif
	int				i, j;
	CNCVCaddinIF*	pAddin;
	CNCVCaddinMap*	pMap;
	CMenu*			pMenu;
	CMenu			menuPop[NCVCADIN_TYPESIZE];
	for ( i=0; i<NCVCADIN_TYPESIZE; i++ )
		menuPop[i].CreatePopupMenu();

	try {
		for ( i=0; i<m_obAddin.GetSize(); i++ ) {
			pAddin = m_obAddin[i];
			for ( j=0; j<NCVCADIN_TYPESIZE; j++ ) {
				if ( !pAddin->GetMenuName(j).IsEmpty() && pAddin->GetAddinFunc(j) &&
						pAddin->GetAddinType() & g_dwAddinType[j] ) {
					menuPop[j].AppendMenu(MF_STRING, m_wAddin, pAddin->GetMenuName(j));
					pMap = new CNCVCaddinMap(pAddin, pAddin->GetAddinFunc(j));
					m_mpAddin.SetAt(m_wAddin, pMap);
					pAddin->SetMenuID(j, m_wAddin++);
				}
			}
		}
	}
	catch (CMemoryException* e) {
		if ( pMap )
			delete	pMap;
		for ( i=0; i<NCVCADIN_TYPESIZE; i++ )
			menuPop[i].DestroyMenu();
		e->Delete();
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		return FALSE;
	}

	for ( i=0; i<NCVCADIN_TYPESIZE; i++ ) {
		if ( menuPop[i].GetMenuItemCount() <= 0 ) {
			menuPop[i].DestroyMenu();
			continue;
		}
		if ( i == NCVCADIN_ARY_APPFILE )		// ﾒｲﾝﾌﾚｰﾑのﾌｧｲﾙ
			pMenu = AfxGetMainWnd()->GetMenu()->GetSubMenu(0);
		else if ( i == NCVCADIN_ARY_APPOPTION )	// ﾒｲﾝﾌﾚｰﾑのｵﾌﾟｼｮﾝ
			pMenu = AfxGetMainWnd()->GetMenu()->GetSubMenu(2);
		else if ( i <= NCVCADIN_ARY_NCDOPTION )	// NCﾄﾞｷｭﾒﾝﾄ
			pMenu = CMenu::FromHandle(m_pDocTemplate[TYPE_NCD]->m_hMenuShared)->GetSubMenu(i-NCVCADIN_ARY_NCDFILE);
		else				// DXFﾄﾞｷｭﾒﾝﾄ
			pMenu = CMenu::FromHandle(m_pDocTemplate[TYPE_DXF]->m_hMenuShared)->GetSubMenu(i-NCVCADIN_ARY_DXFFILE);
		// 「ﾌｧｲﾙ」ﾒﾆｭｰの場合は，ﾒﾆｭｰ順番指定
		switch ( i ) {
		case NCVCADIN_ARY_APPFILE:	// ﾒｲﾝﾌﾚｰﾑの「ﾌｧｲﾙ」ﾒﾆｭｰ
			pMenu->InsertMenu(2, MF_BYPOSITION|MF_SEPARATOR);
			pMenu->InsertMenu(3, MF_BYPOSITION|MF_POPUP, (UINT)menuPop[i].Detach(), lpszAddin);
			break;
		case NCVCADIN_ARY_NCDFILE:	// NCD の「ﾌｧｲﾙ」ﾒﾆｭｰ
			pMenu->InsertMenu(9,  MF_BYPOSITION|MF_SEPARATOR);
			pMenu->InsertMenu(10, MF_BYPOSITION|MF_POPUP, (UINT)menuPop[i].Detach(), lpszAddin);
			break;
		case NCVCADIN_ARY_DXFFILE:	// DXF の「ﾌｧｲﾙ」ﾒﾆｭｰ
			pMenu->InsertMenu(7, MF_BYPOSITION|MF_SEPARATOR);
			pMenu->InsertMenu(8, MF_BYPOSITION|MF_POPUP, (UINT)menuPop[i].Detach(), lpszAddin);
			break;
		default:	// それ以外は追加
			pMenu->AppendMenu(MF_SEPARATOR);
			pMenu->AppendMenu(MF_POPUP, (UINT)menuPop[i].Detach(), lpszAddin);
		}
	}

	return TRUE;
}

void CNCVCApp::SaveWindowState(const CString& strRegKey, const WINDOWPLACEMENT& wp)
{
	CString	strEntry;
	int		i = IDS_REG_WINFLAGS;
	VERIFY(strEntry.LoadString(i++));
	WriteProfileInt(strRegKey, strEntry, wp.flags);
	VERIFY(strEntry.LoadString(i++));
	WriteProfileInt(strRegKey, strEntry, wp.showCmd);
	VERIFY(strEntry.LoadString(i++));
	WriteProfileInt(strRegKey, strEntry, wp.rcNormalPosition.left);
	VERIFY(strEntry.LoadString(i++));
	WriteProfileInt(strRegKey, strEntry, wp.rcNormalPosition.top);
	VERIFY(strEntry.LoadString(i++));
	WriteProfileInt(strRegKey, strEntry, wp.rcNormalPosition.right);
	VERIFY(strEntry.LoadString(i++));
	WriteProfileInt(strRegKey, strEntry, wp.rcNormalPosition.bottom);
}

BOOL CNCVCApp::GetWindowState(const CString& strRegKey, WINDOWPLACEMENT* lpwp)
{
	CString	strEntry;
	int		i = IDS_REG_WINFLAGS;
	VERIFY(strEntry.LoadString(i++));
	lpwp->flags = GetProfileInt(strRegKey, strEntry, 0);
	VERIFY(strEntry.LoadString(i++));
	lpwp->showCmd = GetProfileInt(strRegKey, strEntry, -1);
	VERIFY(strEntry.LoadString(i++));
	lpwp->rcNormalPosition.left = GetProfileInt(strRegKey, strEntry, -1);
	VERIFY(strEntry.LoadString(i++));
	lpwp->rcNormalPosition.top = GetProfileInt(strRegKey, strEntry, -1);
	VERIFY(strEntry.LoadString(i++));
	lpwp->rcNormalPosition.right = GetProfileInt(strRegKey, strEntry, -1);
	VERIFY(strEntry.LoadString(i++));
	lpwp->rcNormalPosition.bottom = GetProfileInt(strRegKey, strEntry, -1);

	if ( lpwp->showCmd==-1 ||
			lpwp->rcNormalPosition.left==-1 || lpwp->rcNormalPosition.top==-1 ||
			lpwp->rcNormalPosition.right==-1 || lpwp->rcNormalPosition.bottom==-1 )
		return FALSE;

	int	n;
	n = ::GetSystemMetrics(SM_CXSCREEN) - ::GetSystemMetrics(SM_CXICON);
	lpwp->rcNormalPosition.left = min(lpwp->rcNormalPosition.left, n);
	n = ::GetSystemMetrics(SM_CYSCREEN) - ::GetSystemMetrics(SM_CYICON);
	lpwp->rcNormalPosition.top = min(lpwp->rcNormalPosition.top, n);

	return TRUE;
}

void CNCVCApp::SaveDlgWindow(int nID, const CWnd* pWnd)
{
	CRect	rc;
	pWnd->GetWindowRect(&rc);
	CString	strSection(GetSubTreeRegKey(IDS_REGKEY_WINDOW, nID)), strEntry;
	VERIFY(strEntry.LoadString(IDS_REG_LEFT));
	WriteProfileInt(strSection, strEntry, rc.left);
	VERIFY(strEntry.LoadString(IDS_REG_TOP));
	WriteProfileInt(strSection, strEntry, rc.top);
}

BOOL CNCVCApp::GetDlgWindow(int nID, CPoint* lpt)
{
	CString	strSection(GetSubTreeRegKey(IDS_REGKEY_WINDOW, nID)), strEntry;
	VERIFY(strEntry.LoadString(IDS_REG_LEFT));
	lpt->x = GetProfileInt(strSection, strEntry, -1);
	VERIFY(strEntry.LoadString(IDS_REG_TOP));
	lpt->y = GetProfileInt(strSection, strEntry, -1);

	if ( lpt->x==-1 || lpt->y==-1 )
		return FALSE;

	int	n;
	n = ::GetSystemMetrics(SM_CXSCREEN) - ::GetSystemMetrics(SM_CXICON);
	lpt->x = min(lpt->x, n);
	n = ::GetSystemMetrics(SM_CYSCREEN) - ::GetSystemMetrics(SM_CYICON);
	lpt->y = min(lpt->y, n);
	return TRUE;
}

CNCDoc* CNCVCApp::GetAlreadyNCDocument(LPCTSTR strPathName)
{
	CNCDoc*		pDoc;
	for ( POSITION pos=m_pDocTemplate[TYPE_NCD]->GetFirstDocPosition(); pos; ) {
		pDoc = (CNCDoc *)(m_pDocTemplate[TYPE_NCD]->GetNextDoc(pos));
#ifdef _DEBUG
		g_dbg.printf("GetAlreadyNCDocument() DocPathName=%s", pDoc->GetPathName());
#endif
		if ( !strPathName || pDoc->GetPathName().CompareNoCase(strPathName)==0 )
			return pDoc;
	}

	return NULL;
}

CDXFDoc* CNCVCApp::GetAlreadyDXFDocument(LPCTSTR strPathName)
{
	CDXFDoc*	pDoc;
	for ( POSITION pos=m_pDocTemplate[TYPE_DXF]->GetFirstDocPosition(); pos; ) {
		pDoc = (CDXFDoc *)(m_pDocTemplate[TYPE_DXF]->GetNextDoc(pos));
#ifdef _DEBUG
		g_dbg.printf("GetAlreadyDXFDocument() DocPathName=%s", pDoc->GetPathName());
#endif
		if ( !strPathName || pDoc->GetPathName().CompareNoCase(strPathName)==0 )
			return pDoc;
	}

	return NULL;
}

int CNCVCApp::GetDXFOpenDocumentCount(void)
{
	int		nResult = 0;
	for ( POSITION pos=m_pDocTemplate[TYPE_DXF]->GetFirstDocPosition(); pos; nResult++ )
		m_pDocTemplate[TYPE_DXF]->GetNextDoc(pos);

	return nResult;
}

void CNCVCApp::ReloadDXFDocument(void)
{
	CDXFDoc*	pDoc;
	CTypedPtrList<CObList, CDXFDoc*>	ltDoc;
	PFNNCVCSERIALIZEFUNC	pfnSerialFunc;
	CString		strFileName;
	POSITION	pos;

	try {
		// Reloadﾌﾗｸﾞのﾄﾞｷｭﾒﾝﾄを取得 from CDxfSetup::OnReload()
		for ( pos=m_pDocTemplate[TYPE_DXF]->GetFirstDocPosition(); pos; ) {
			pDoc = (CDXFDoc *)(m_pDocTemplate[TYPE_DXF]->GetNextDoc(pos));
			if ( pDoc->IsDXFDocFlag(DXFDOC_RELOAD) )
				ltDoc.AddTail(pDoc);
		}
		// 必要な情報を取得後，閉じて開く
		for ( pos=ltDoc.GetHeadPosition(); pos; ) {
			pDoc = ltDoc.GetNext(pos);
			pfnSerialFunc = pDoc->GetSerializeFunc();
			strFileName = pDoc->GetPathName();
			pDoc->OnCloseDocument();
			SetSerializeFunc(pfnSerialFunc);
			m_pDocTemplate[TYPE_DXF]->OpenDocumentFile(strFileName);
		}
		SetSerializeFunc((PFNNCVCSERIALIZEFUNC)NULL);
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
	}
}

void CNCVCApp::ChangeViewOption(void)
{
	// ﾌﾞﾗｼ・ﾍﾟﾝ作成，ﾌｫﾝﾄ変更，など
	AfxGetNCVCMainWnd()->ChangeViewOption();

	// 各NCﾋﾞｭｰへの変更通知
	POSITION	pos;
	CDocument*	pDoc;
	for ( int i=0; i<SIZEOF(m_pDocTemplate); i++ ) {
		for ( pos=m_pDocTemplate[i]->GetFirstDocPosition(); pos; ) {
			pDoc = m_pDocTemplate[i]->GetNextDoc(pos);
			pDoc->UpdateAllViews(NULL, UAV_CHANGEFONT);
		}
	}
}

BOOL CNCVCApp::ChangeMachine(int nIndex)
{
	const	CStringList*	pList = m_pOptMC->GetMCList();
	CString	strFileName;

	// 「参照...」を選択？
	if ( pList->GetCount() <= nIndex ) {
		if ( !pList->IsEmpty() )
			strFileName = pList->GetHead();
		if ( ::NCVC_FileDlgCommon(IDS_OPTION_MC, IDS_MC_FILTER, FALSE, strFileName) != IDOK )
			return FALSE;
	}
	else {
		POSITION pos = pList->GetHeadPosition();
		// ｺﾝﾎﾞﾎﾞｯｸｽの順番からﾌﾙﾊﾟｽ情報を取得
		while ( nIndex-- > 0 && pos )
			pList->GetNext(pos);
		if ( pos )
			strFileName = pList->GetAt(pos);
		else
			return FALSE;
	}

	// 各種更新
	return ChangeMachine(strFileName);
}

BOOL CNCVCApp::ChangeMachine(LPCTSTR lpszMachineFile)
{
	// 機械情報読み込み
	if ( !m_pOptMC->ReadMCoption(lpszMachineFile) )
		return FALSE;

	// ﾂｰﾙﾊﾞｰの更新
	AfxGetNCVCMainWnd()->ChangeMachine();

	// 再計算
	for ( POSITION pos=m_pDocTemplate[TYPE_NCD]->GetFirstDocPosition(); pos; )
		((CNCDoc *)(m_pDocTemplate[TYPE_NCD]->GetNextDoc(pos)))->CreateCutcalcThread();

	return TRUE;
}

void CNCVCApp::CallAddinFunc(WORD wID)
{
	CNCVCaddinMap*	pMap;
	BOOL	bResult = m_mpAddin.Lookup(wID, pMap);
	if ( bResult ) {
		m_pActiveAddin = pMap->GetAddinIF();
		PFNNCVCADDINFUNC pfnFunc = pMap->GetAddinFunc();
		if ( pfnFunc )	// GetProcAddress()でNULLの可能性もある
			(*pfnFunc)();
	}
	else
		m_pActiveAddin = NULL;
}

const CNCVCaddinIF* CNCVCApp::GetLookupAddinID(WORD wID)
{
	CNCVCaddinMap*	pMap;
	if ( m_mpAddin.Lookup(wID, pMap) )
		return pMap->GetAddinIF();
	return NULL;
}

BOOL CNCVCApp::AddExtensionFunc
	(DOCTYPE enType, LPCTSTR lpszExt, LPCTSTR pszModuleName, LPCTSTR pszSerialFunc)
{
	HMODULE	hLib = ::GetModuleHandle(pszModuleName);
	if ( !hLib )
		return FALSE;
	PFNNCVCSERIALIZEFUNC pFunc = (PFNNCVCSERIALIZEFUNC)::GetProcAddress(hLib, pszSerialFunc);
	if ( !pFunc )
		return FALSE;

	return m_pDocTemplate[enType]->AddExtensionFunc(lpszExt, pFunc);
}

const CExecOption* CNCVCApp::GetLookupExecID(WORD wID)
{
	CExecOption*	pExec;
	if ( m_mpExec.Lookup(wID, pExec) )
		return pExec;
	return NULL;
}

BOOL CNCVCApp::DoPromptFileNameEx(CString& strFileName)
{
#ifdef _DEBUG_FILEOPEN
	g_dbg.printf("CNCVCApp::DoPromptFileNameEx() Start");
#endif
	int		i, nResult, nExt;
	CString	strAllFilter, strFilter[SIZEOF(m_pDocTemplate)], strExt[SIZEOF(m_pDocTemplate)], strTmp;

	// NCVCﾌｨﾙﾀ設定
	for ( i=0; i<SIZEOF(m_pDocTemplate); i++ ) {
		strFilter[i] = m_pDocTemplate[i]->GetFilterString();
		if ( !strTmp.IsEmpty() )
			strTmp += ';';
		strTmp += strFilter[i];
	}
	strAllFilter.Format(IDS_NCVC_FILTER, strTmp, strTmp);
	// NC, DXFﾌｨﾙﾀ
	for ( i=0; i<SIZEOF(m_pDocTemplate); i++ ) {
		strTmp.Format(i+IDS_NCD_FILTER, strFilter[i], strFilter[i]);
		strExt[i] = strTmp.Left(3);				// ncd or dxf
		strAllFilter += '|' + strTmp.Mid(4);	// ﾃﾞﾌｫﾙﾄ拡張子除く
	}
	// ここで「All Files」は不要
	strAllFilter += "||";
	// 現在ｱｸﾃｨﾌﾞなﾄﾞｷｭﾒﾝﾄﾀｲﾌﾟに合わせてﾃﾞﾌｫﾙﾄ拡張子を決定
	CMDIChildWnd*	pChild = AfxGetNCVCMainWnd()->MDIGetActive();
	CDocument*		pDoc   = pChild ? pChild->GetActiveDocument() : NULL;
	if ( pDoc )
		nExt = pDoc->IsKindOf(RUNTIME_CLASS(CNCDoc)) ? 0 : 1;
	else	// ﾌｧｲﾙが１つも開かれていないときはﾃﾞﾌｫﾙﾄ拡張子を「DXF」にする
		nExt = 1;

	// ﾌｧｲﾙ開くﾀﾞｲｱﾛｸﾞ
	CFileDialog	dlg(TRUE, strExt[nExt], strFileName,
		OFN_FILEMUSTEXIST|OFN_HIDEREADONLY, strAllFilter);
	// ﾀﾞｲｱﾛｸﾞ表示
	nResult = dlg.DoModal();
	if ( nResult == IDOK )
		strFileName = dlg.GetPathName();

	return nResult == IDOK;
}

/////////////////////////////////////////////////////////////////////////////
// CNCVCApp メッセージ ハンドラ

void CNCVCApp::OnFileOpen() 
{
#ifdef _DEBUG_FILEOPEN
	g_dbg.printf("CNCVCApp::OnFileOpen() Start");
#endif
	// MRU最新ﾘｽﾄのﾊﾟｽを有効にする
	CString	newName(GetRecentFileName());
	if ( !DoPromptFileNameEx(newName) )		// ｶｽﾀﾑﾌｧｲﾙｵｰﾌﾟﾝ
		return;

	OpenDocumentFile(newName);
}

void CNCVCApp::OnFileThumbnail()
{
	CString	strRegKey, strSort, strPlane;
	VERIFY(strRegKey.LoadString(IDS_REGKEY_SETTINGS));
	VERIFY(strSort.LoadString(IDS_REG_THUMBNAIL_SORT));
	VERIFY(strPlane.LoadString(IDS_REG_THUMBNAIL_PLANE));
	int	nSort  = (int)GetProfileInt(strRegKey, strSort, 0),
		nPlane = (int)GetProfileInt(strRegKey, strPlane, 1);
	CThumbnailDlg	dlg(nSort, (ENNCVPLANE)nPlane);
	if ( dlg.DoModal() != IDOK )
		return;

	WriteProfileInt(strRegKey, strSort,  dlg.m_nSort);
	WriteProfileInt(strRegKey, strPlane, dlg.m_nPlane);
	OpenDocumentFile(dlg.m_strFile);
}

void CNCVCApp::OnFileCloseAndOpen() 
{
	CString		newName(GetRecentFileName());
	if ( !DoPromptFileNameEx(newName) )
		return;

	CMDIChildWnd*	pChild = AfxGetNCVCMainWnd()->MDIGetActive();
	CDocument*		pDoc   = pChild ? pChild->GetActiveDocument() : NULL;
	if ( pDoc )
		pDoc->OnCloseDocument();
	OpenDocumentFile(newName);
}

void CNCVCApp::OnViewSetup() 
{
	CViewSetup	ps;
	if ( ps.DoModal() != IDOK )
		return;

	// ｵﾌﾟｼｮﾝの保存
	if ( !m_pOptView->SaveViewOption() )
		AfxMessageBox(IDS_ERR_REGISTRY, MB_OK|MB_ICONEXCLAMATION);

	// ﾒｲﾝﾌﾚｰﾑ，各ﾋﾞｭｰへの更新通知
	ChangeViewOption();
}

void CNCVCApp::OnViewSetupInport() 
{
	CString	strFile;
	if ( ::NCVC_FileDlgCommon(IDS_VIEW_SETUP_INPORT, IDS_INI_FILTER, TRUE, strFile) == IDOK ) {
		m_pOptView->Inport(strFile);
		if ( !m_pOptView->SaveViewOption() )
			AfxMessageBox(IDS_ERR_REGISTRY, MB_OK|MB_ICONEXCLAMATION);
		ChangeViewOption();
	}
}

void CNCVCApp::OnViewSetupExport() 
{
	CString	strFile;
	if ( ::NCVC_FileDlgCommon(IDS_VIEW_SETUP_EXPORT, IDS_INI_FILTER, TRUE,
			strFile, NULL, FALSE, OFN_HIDEREADONLY|OFN_PATHMUSTEXIST) == IDOK ) {
		CString	strMsg;
		if ( m_pOptView->Export(strFile) ) {
			strMsg.Format(IDS_ANA_FILEOUTPUT, strFile);
			AfxMessageBox(strMsg, MB_OK|MB_ICONINFORMATION);
		}
		else {
			strMsg.Format(IDS_ERR_WRITESETTING, strFile);
			AfxMessageBox(strMsg, MB_OK|MB_ICONEXCLAMATION);
		}
	}
}

void CNCVCApp::OnWindowAllClose() 
{
	for ( int i=0; i<SIZEOF(m_pDocTemplate); i++ )
		m_pDocTemplate[i]->CloseAllDocuments(FALSE);
	// 表示中のﾓｰﾄﾞﾚｽﾀﾞｲｱﾛｸﾞにも指示
	AfxGetNCVCMainWnd()->AllModelessDlg_PostSwitchMessage();
}

void CNCVCApp::OnOptionMC() 
{
	BOOL	bNewSelect = FALSE;
	CString	strFileName(m_pOptMC->GetMCHeadFileName());
	if ( strFileName.IsEmpty() )
		bNewSelect = TRUE;	// 新規選択の場合は再計算

	if ( ::NCVC_FileDlgCommon(IDS_OPTION_MC, IDS_MC_FILTER, FALSE, strFileName) != IDOK )
		return;

	CString	strCaption;
	VERIFY(strCaption.LoadString(IDS_SETUP_MC));
	CMCSetup	ps(::AddDialogTitle2File(strCaption, strFileName), strFileName);
	if ( ps.DoModal() != IDOK ) {
		if ( !strFileName.IsEmpty() )
			m_pOptMC->ReadMCoption(strFileName, FALSE);	// 設定前のﾌｧｲﾙで読み直し
		return;
	}

	// 新規の場合，ｵﾌﾟｼｮﾝの保存
	strFileName.Empty();
	if ( ps.m_strFileName.IsEmpty() )
		::NCVC_FileDlgCommon(IDS_OPTION_MCSAVE, IDS_MC_FILTER, FALSE, 
				strFileName, g_pszExecDir, FALSE, OFN_OVERWRITEPROMPT|OFN_HIDEREADONLY);
	else
		strFileName = ps.m_strFileName;
	if ( !strFileName.IsEmpty() && !m_pOptMC->SaveMCoption(strFileName) ) {
		CString	strMsg;
		strMsg.Format(IDS_ERR_WRITESETTING, strFileName);
		AfxMessageBox(strMsg, MB_OK|MB_ICONEXCLAMATION);
		return;
	}

	// ﾂｰﾙﾊﾞｰの更新
	AfxGetNCVCMainWnd()->ChangeMachine();

	// ｵﾌﾟｼｮﾝの変更があれば，再読込か各ﾄﾞｷｭﾒﾝﾄごとに切削時間の計算ｽﾚｯﾄﾞ生成
	POSITION	pos = m_pDocTemplate[TYPE_NCD]->GetFirstDocPosition();
	if ( !pos )
		return;
	// 再読込のﾁｪｯｸ
	if ( ps.m_bReload && AfxMessageBox(IDS_ANA_RELOAD, MB_YESNO|MB_ICONQUESTION)==IDYES ) {
		CStringList	listStrFileName;
		try {
			while ( pos )
				listStrFileName.AddTail(m_pDocTemplate[TYPE_NCD]->GetNextDoc(pos)->GetPathName());
			m_pDocTemplate[TYPE_NCD]->CloseAllDocuments(FALSE);
			for ( pos=listStrFileName.GetHeadPosition(); pos; )
				OpenDocumentFile(listStrFileName.GetNext(pos));
		}
		catch (CMemoryException* e) {
			AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
			e->Delete();
		}
		return;
	}
	// 再計算のﾁｪｯｸ
	if ( bNewSelect || ps.m_bCalcThread ) {
		while ( pos )
			((CNCDoc *)(m_pDocTemplate[TYPE_NCD]->GetNextDoc(pos)))->CreateCutcalcThread();
	}
}

void CNCVCApp::OnOptionEditMC() 
{
	if ( m_liExec.GetCount() <= 0 )
		return;

	CString	strFileName(m_pOptMC->GetMCHeadFileName());
	if ( ::NCVC_FileDlgCommon(IDS_OPTION_MC, IDS_MC_FILTER, FALSE, strFileName) == IDOK )
		AfxGetNCVCMainWnd()->CreateOutsideProcess(m_liExec.GetHead()->GetFileName(), "\""+strFileName+"\"");
}

void CNCVCApp::OnOptionDXF() 
{
	CDxfSetup	ps(IDS_SETUP_CADLAYER);
	if ( ps.DoModal() != IDOK )
		return;

	// ｵﾌﾟｼｮﾝの保存
	if ( !m_pOptDXF->SaveDXFoption() )
		AfxMessageBox(IDS_ERR_REGISTRY, MB_OK|MB_ICONEXCLAMATION);
}

void CNCVCApp::OnOptionMakeNC() 
{
	CString		strFileName;
	const CStringList*	pList = NULL;
	switch ( m_pOptDXF->GetNCMakeType() ) {
	case NCMAKEMILL:
		pList = m_pOptDXF->GetMillInitList();
		break;
	case NCMAKELATHE:
		pList = m_pOptDXF->GetLatheInitList();
		break;
	}
	if ( pList && !pList->IsEmpty() )
		strFileName = pList->GetHead();
	if ( ::NCVC_FileDlgCommon(IDS_OPTION_INIT, IDS_NCI_FILTER, FALSE, strFileName) != IDOK ||
				strFileName.IsEmpty() )
		return;

	// 拡張子の判断
	CString	strCaption, strTmp, strMill, strLathe;

	VERIFY(strCaption.LoadString(IDS_MAKE_NCD));

	VERIFY(strTmp.LoadString(IDS_NCIM_FILTER));
	strMill = strTmp.Left(3);		// nci
	VERIFY(strTmp.LoadString(IDS_NCIL_FILTER));
	strLathe = strTmp.Left(3);		// ncj
	strTmp = strFileName.Right(3);	// 指定ﾌｧｲﾙの拡張子

	if ( strMill.CompareNoCase(strTmp) == 0 ) {
		CMKNCSetup	ps(::AddDialogTitle2File(strCaption, strFileName), strFileName);
		if ( ps.DoModal() == IDOK ) {
			// ｵﾌﾟｼｮﾝの保存
			ps.GetNCMakeOption()->SaveMakeOption();
#ifdef _DEBUGOLD
			ps.GetNCMakeOption()->DbgDump();
#endif
			// 切削条件ﾌｧｲﾙの履歴更新
			m_pOptDXF->AddMillInitHistory(ps.GetNCMakeOption()->GetInitFile());
		}
	}
	else if ( strLathe.CompareNoCase(strTmp) == 0 ) {
		CMKLASetup	ps(::AddDialogTitle2File(strCaption, strFileName), strFileName);
		if ( ps.DoModal() == IDOK ) {
			ps.GetNCMakeOption()->SaveMakeOption();
#ifdef _DEBUGOLD
			ps.GetNCMakeOption()->DbgDump();
#endif
			m_pOptDXF->AddLatheInitHistory(ps.GetNCMakeOption()->GetInitFile());
		}
	}
}

void CNCVCApp::OnUpdateOptionEdit(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_liExec.GetCount() < 1 ? FALSE : TRUE);
}

void CNCVCApp::OnOptionEditNC() 
{
	if ( m_liExec.GetCount() <= 0 )
		return;

	CString		strFileName;
	const CStringList*	pList = NULL;
	switch ( m_pOptDXF->GetNCMakeType() ) {
	case NCMAKEMILL:
		pList = m_pOptDXF->GetMillInitList();
		break;
	case NCMAKELATHE:
		pList = m_pOptDXF->GetLatheInitList();
		break;
	}
	if ( pList && !pList->IsEmpty() )
		strFileName = pList->GetHead();
	if ( ::NCVC_FileDlgCommon(IDS_OPTION_INIT, IDS_NCI_FILTER, TRUE, strFileName) == IDOK )
		AfxGetNCVCMainWnd()->CreateOutsideProcess(m_liExec.GetHead()->GetFileName(), "\""+strFileName+"\"");
}

void CNCVCApp::OnOptionExec() 
{
	extern	LPCTSTR	gg_szRegKey;

	// 設定前に現在の情報を保存
	CExecOption*	pExec;
	CStringArray	strArray;
	LPWORD	pMenuID = NULL;
	int		i=0, nBtnCnt = m_liExec.GetCount();
	try {
		pMenuID = new WORD[nBtnCnt];
		for ( POSITION pos=m_liExec.GetHeadPosition(); pos; i++ ) {
			pExec = m_liExec.GetNext(pos);
			strArray.Add( pExec->GetToolTip() );
			pMenuID[i] = pExec->GetMenuID();
		}
	}
	catch (CMemoryException* e) {
		if ( pMenuID )
			delete[]	pMenuID;
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		return;
	}

	CExecSetupDlg	dlg;
	if ( dlg.DoModal() != IDOK ) {
		delete[]	pMenuID;
		return;
	}

	// ﾚｼﾞｽﾄﾘへの保存
	CString	strRegKey, strEntry;
	VERIFY(strRegKey.LoadString(IDS_REGKEY_SETTINGS));
	VERIFY(strEntry.LoadString(IDS_REG_EXEC));
	int nRegExec = (int)GetProfileInt(strRegKey, strEntry, 0);
	nBtnCnt = m_liExec.GetCount();
	if ( nRegExec > nBtnCnt ) {
		// 減少分だけを消去
		CString	strFormat;
		CRegKey	reg;
		// --- "Software\MNCT-S\NCVC\Settings"
		if ( reg.Open(HKEY_CURRENT_USER, gg_szRegKey+strRegKey) == ERROR_SUCCESS ) {
			for ( i=nBtnCnt; i<nRegExec; i++ ) {
				strFormat.Format(IDS_COMMON_FORMAT, strEntry, i);
				reg.DeleteValue(strFormat);
			}
		}
	}
	// ｶｽﾀﾑﾒﾆｭｰに登録されたﾒﾆｭｰ文字列とｲﾒｰｼﾞﾏｯﾌﾟを消去
	AfxGetNCVCMainWnd()->RemoveCustomMenu(strArray, pMenuID);
	delete[]	pMenuID;
	// ﾃﾞｰﾀ再登録
	SaveExecData();
	CreateExecMap();

	// ﾂｰﾙﾊﾞｰへの通知
	AfxGetNCVCMainWnd()->SetExecButtons(FALSE);	// ﾘｽﾄｱなし
}

void CNCVCApp::OnOptionExt() 
{
	CExtensionDlg	dlg;
	if ( dlg.DoModal() == IDOK ) {
		for ( int i=0; i<SIZEOF(m_pDocTemplate); i++ )
			m_pDocTemplate[i]->SaveExt();
	}
}

void CNCVCApp::OnHelp() 
{
	CString	strHelp(g_pszExecDir);
	strHelp += g_szHelp;
	::ShellExecute(NULL, NULL, strHelp, NULL, NULL, SW_SHOWNORMAL);
}

void CNCVCApp::OnHelpAddin() 
{
	CAddinDlg	dlg;
	dlg.DoModal();
}

void CNCVCApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// ﾌﾟﾛｼﾞｪｸﾄ広域関数
/////////////////////////////////////////////////////////////////////////////

void NCVC_CriticalErrorMsg(LPCTSTR lpszProg, int nLine)
{
	CString	strMsg, strPath, strName;
	::Path_Name_From_FullPath(lpszProg, strPath, strName);
	strMsg.Format(IDS_ERR_CRITICAL, strName, nLine);
	AfxMessageBox(strMsg, MB_OK|MB_ICONSTOP);
	ExitProcess(-1);
}

// ﾌｧｲﾙの存在ﾁｪｯｸ
BOOL IsFileExist(LPCTSTR lpszFile, BOOL bExist/*=TRUE*/, BOOL bMsg/*=TRUE*/)
{
	if ( lstrlen(lpszFile) <= 0 ) {
		AfxMessageBox(IDS_ERR_FILEPATH, MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}

	DWORD	dwErrFlg = (FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_SYSTEM);

	// ﾌｧｲﾙの存在ﾁｪｯｸ
	DWORD	dwAt = ::GetFileAttributes(lpszFile);
	if ( bExist ) {		// ﾌｧｲﾙが存在していなければならない
		if ( (int)dwAt == -1 || dwAt & dwErrFlg ) {	// -1 == INVALID_HANDLE_VALUE
			if ( bMsg )
				AfxMessageBox(IDS_ERR_FILEPATH, MB_OK|MB_ICONEXCLAMATION);
			return FALSE;
		}
	}
	else {				// ﾌｧｲﾙが存在していれば上書き確認
		if ( (int)dwAt != -1 ) {
			// 無効ﾊﾟｽ名のﾁｪｯｸ
			if ( dwAt & (dwErrFlg|FILE_ATTRIBUTE_READONLY) ) {
				AfxMessageBox(IDS_ERR_FILEPATH, MB_OK|MB_ICONEXCLAMATION);
				return FALSE;
			}
			// ﾌｧｲﾙの上書き確認
			CString		strMsg;
			strMsg.Format(IDS_ANA_FILEEXIST, lpszFile);
			if ( AfxMessageBox(strMsg, MB_YESNO|MB_ICONQUESTION) != IDYES )
				return FALSE;
		}
	}

	return TRUE;
}

// CFileDialogの呼び出し
int NCVC_FileDlgCommon
	(int nIDtitle, const CString& strFilter, BOOL bAll,
	 CString& strFileName, LPCTSTR lpszInitialDir, BOOL bRead, DWORD dwFlags)
{
	extern	LPCTSTR	gg_szReturn;

	int		nIndex, nResult;
	CString	strAllFilter, strTitle, strTmp;

	// ﾌｨﾙﾀ設定とﾃﾞﾌｫﾙﾄ拡張子の取得
	VERIFY(strTmp.LoadString(IDS_ALL_FILTER));
	strAllFilter = bAll ? strTmp : strTmp.Right(2);
	nIndex = strFilter.Find(gg_szReturn);
	CFileDialog	dlg(bRead, strFilter.Left(nIndex), strFileName,
		dwFlags, strFilter.Mid(nIndex+1)+strAllFilter);
	// ﾀｲﾄﾙの呼び出し
	if ( nIDtitle > 0 ) {
		VERIFY(strTitle.LoadString(nIDtitle));
		dlg.m_ofn.lpstrTitle = strTitle;
	}
	// 初期ﾃﾞｨﾚｸﾄﾘ
	if ( lpszInitialDir && lstrlen(lpszInitialDir)>0 )
		dlg.m_ofn.lpstrInitialDir = lpszInitialDir;
	// ﾀﾞｲｱﾛｸﾞ
	nResult = dlg.DoModal();
	if ( nResult == IDOK )
		strFileName = dlg.GetPathName();
	else
		strFileName.Empty();
	return	nResult;
}

//////////////////////////////////////////////////////////////////////
// CRecentViewInfo クラス
//////////////////////////////////////////////////////////////////////

CRecentViewInfo::CRecentViewInfo(LPCTSTR lpszPathName)
{
	m_bInfo = FALSE;
	if ( lpszPathName && lstrlen(lpszPathName)>0 )
		m_strFile = lpszPathName;
	for ( int i=0; i<SIZEOF(m.objectXform); i++ ) {
		for ( int j=0; j<SIZEOF(m.objectXform[0]); j++ )
			m.objectXform[i][j] = 0.0;
	}
}

void CRecentViewInfo::SetViewInfo
	(const GLdouble objectXform[4][4], const CRect3D& rcView, const CPointD& ptCenter)
{
	m_bInfo = TRUE;
	for ( int i=0; i<SIZEOF(m.objectXform); i++ ) {
		for ( int j=0; j<SIZEOF(m.objectXform[0]); j++ )
			m.objectXform[i][j] = objectXform[i][j];
	}
	m.rcView   = rcView;
	m.ptCenter = ptCenter;
}

BOOL CRecentViewInfo::GetViewInfo(GLdouble objectXform[4][4], CRect3D& rcView, CPointD& ptCenter) const
{
	if ( m_bInfo ) {
		// 値がｾｯﾄされているときだけ更新
		for ( int i=0; i<SIZEOF(m.objectXform); i++ ) {
			for ( int j=0; j<SIZEOF(m.objectXform[0]); j++ )
				objectXform[i][j] = m.objectXform[i][j];
		}
		rcView   = m.rcView;
		ptCenter = m.ptCenter;
	}

	return m_bInfo;
}

//////////////////////////////////////////////////////////////////////
// CNCVCDocTemplate クラス
//////////////////////////////////////////////////////////////////////

CNCVCDocTemplate::CNCVCDocTemplate(UINT nIDResource, CRuntimeClass* pDocClass,
	CRuntimeClass* pFrameClass, CRuntimeClass* pViewClass) :
			CMultiDocTemplate(nIDResource, pDocClass, pFrameClass, pViewClass)
{
	// ﾚｼﾞｽﾄﾘから拡張子情報を取得
	extern	LPCTSTR		gg_szComma;		// StdAfx.cpp
	typedef boost::tokenizer< boost::char_separator<TCHAR> > tokenizer;
	static	boost::char_separator<TCHAR> sep(gg_szComma);

	CString		strRegKey, strEntry, strResult;
	VERIFY(strRegKey.LoadString(IDS_REGKEY_SETTINGS));
	VERIFY(strEntry.LoadString(IDS_REG_EXTENSION));
	GetDocString(strResult, CDocTemplate::filterExt);
	strEntry += strResult;

	std::string	str( AfxGetApp()->GetProfileString(strRegKey, strEntry) );
	tokenizer	tok( str, sep );
	try {
		for ( tokenizer::iterator it=tok.begin(); it!=tok.end(); ++it ) {
			strResult = ::Trim(*it).c_str();
			strResult.MakeUpper();		// 大文字登録
			m_mpExt[EXT_DLG].SetAt(strResult, NULL);
		}
	}
	catch (CMemoryException* e) {
		e->Delete();
	}
}

BOOL CNCVCDocTemplate::AddExtensionFunc(LPCTSTR lpszExt, LPVOID pAddFunc)
{
	CString	strExt(lpszExt);
	strExt.MakeUpper();
	// ﾀﾞｲｱﾛｸﾞ用に登録されていればそれを削除(ｱﾄﾞｲﾝ優先)
	LPVOID	pFunc;
	if ( m_mpExt[EXT_DLG].Lookup(strExt, pFunc) )
		m_mpExt[EXT_DLG].RemoveKey(strExt);

	// ｱﾄﾞｲﾝ用拡張子の登録
	try {
		// 既に登録されていればｴﾗｰ(早い順)
		if ( m_mpExt[EXT_ADN].Lookup(strExt, pFunc) )
			return FALSE;
		m_mpExt[EXT_ADN].SetAt(strExt, pAddFunc);
	}
	catch (CMemoryException* e) {
		e->Delete();
		return FALSE;	// ｴﾗｰﾒｯｾｰｼﾞは不要
	}
	return TRUE;
}

CDocTemplate::Confidence
	CNCVCDocTemplate::MatchDocType(LPCTSTR lpszPathName, CDocument*& rpDocMatch)
{
	Confidence match = CDocTemplate::MatchDocType(lpszPathName, rpDocMatch);
	if ( match!=yesAlreadyOpen && match!=yesAttemptNative ) {
		match = noAttempt;	// 登録拡張子以外は開かないようにする
		LPCTSTR	lpszExt = ::PathFindExtension(lpszPathName);
		if ( lpszExt ) {
			CString	strExt(lstrlen(lpszExt)>0 ? lpszExt : ".*");
			if ( strExt[0] == '.' ) {
				// ｶｽﾀﾑ拡張子の検索
				LPVOID	pFunc;
				if ( IsExtension(strExt.Mid(1), &pFunc) ) {
					AfxGetNCVCApp()->SetSerializeFunc((PFNNCVCSERIALIZEFUNC)pFunc);
					match = yesAttemptForeign;
				}
			}
		}
	}
	return match;
}

#ifdef _DEBUG_FILEOPEN
CDocument* CNCVCDocTemplate::OpenDocumentFile(LPCTSTR lpszPathName, BOOL bMakeVisible/*=TRUE*/)
{
	g_dbg.printf("CNCVCDocTemplate::OpenDocumentFile() Start");

	CTime	t1 = dbgtimeFileOpen = CTime::GetCurrentTime();
	CDocument* pDoc = CMultiDocTemplate::OpenDocumentFile(lpszPathName, bMakeVisible);
	CTime	t2 = CTime::GetCurrentTime();

	CTimeSpan ts = t2 - t1;
	CString	strTime( ts.Format("%H:%M:%S") );
	g_dbg.printf("CMultiDocTemplate::OpenDocumentFile() = %s", strTime);

	return pDoc;
}
#endif

BOOL CNCVCDocTemplate::IsExtension(LPCTSTR lpszExt, LPVOID* pFuncResult/*=NULL*/)
{
	CString	strFilter, strExt(lpszExt);

	// 標準拡張子とのﾁｪｯｸ
	GetDocString(strFilter, CDocTemplate::filterExt);	// .ncd or .cam
	if ( strExt.CompareNoCase(strFilter.Right(3)) == 0 ) {
		if ( pFuncResult )
			*pFuncResult = NULL;	// ﾃﾞﾌｫﾙﾄｼﾘｱﾙ関数
		return TRUE;
	}

	// ｶｽﾀﾑ拡張子の検索
	// pFunc は NULL もあり得るので，戻り値に出来ない
	LPVOID	pFunc;
	strExt.MakeUpper();
	for ( int i=0; i<SIZEOF(m_mpExt); i++ ) {
		if ( m_mpExt[i].Lookup(strExt, pFunc) ) {
			if ( pFuncResult )
				*pFuncResult = pFunc;
			return TRUE;
		}
	}

	return FALSE;
}

BOOL CNCVCDocTemplate::SaveExt(void)
{
	extern	LPCTSTR		gg_szComma;

	CString		strRegKey, strEntry, strResult, strKey;
	VERIFY(strRegKey.LoadString(IDS_REGKEY_SETTINGS));
	VERIFY(strEntry.LoadString(IDS_REG_EXTENSION));
	GetDocString(strResult, CDocTemplate::filterExt);
	strEntry += strResult;

	LPVOID	pDummy;
	strResult.Empty();
	for ( POSITION pos=m_mpExt[EXT_DLG].GetStartPosition(); pos; ) {
		m_mpExt[EXT_DLG].GetNextAssoc(pos, strKey, pDummy);
		if ( !strResult.IsEmpty() )
			strResult += gg_szComma;
		strResult += strKey;
	}

	if ( !AfxGetApp()->WriteProfileString(strRegKey, strEntry, strResult) ) {
		AfxMessageBox(IDS_ERR_REGISTRY, MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}

	return TRUE;
}

CString CNCVCDocTemplate::GetFilterString(void)
{
	extern	LPCTSTR	gg_szWild;	// "*.";
	static	const	TCHAR	ss_cSplt = ';';
	POSITION	pos;
	CString		strResult, strKey;
	LPVOID	pDummy;

	// 基本拡張子
	GetDocString(strResult, CDocTemplate::filterExt);
	strResult.MakeLower();
	strResult = gg_szWild + strResult.Right(3);

	// 登録拡張子
	for ( int i=0; i<SIZEOF(m_mpExt); i++ ) {
		for ( pos=m_mpExt[i].GetStartPosition(); pos; ) {
			m_mpExt[i].GetNextAssoc(pos, strKey, pDummy);
			strKey.MakeLower();
			strResult += ss_cSplt;
			strResult += gg_szWild + strKey;
		}
	}

	return strResult;
}

IMPLEMENT_DYNAMIC(CNCVCDocTemplate, CMultiDocTemplate)
