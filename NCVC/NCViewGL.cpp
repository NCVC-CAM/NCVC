// NCViewGL.cpp : 実装ファイル
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "NCChild.h"
#include "NCdata.h"
#include "NCDoc.h"
#include "NCViewTab.h"
#include "NCInfoTab.h"
#include "NCViewGL.h"
#include "NCListView.h"
#include "ViewOption.h"
#include <numeric>				// accumulate()
#include "boost/array.hpp"
#include "../Kodatuno/Describe_BODY.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
//#define	_DEBUG_FILEOUT_		// Depth File out
//#define	_DEBUG_DRAWTEST_
//#define	_DEBUG_POLYGONLINE_
extern	CMagaDbg	g_dbg;
#include <mmsystem.h>			// timeGetTime()
#endif

using std::vector;
using namespace boost;
extern	int				g_nProcesser;	// ﾌﾟﾛｾｯｻ数(NCVC.cpp)
extern	const PENSTYLE	g_penStyle[];	// ViewOption.cpp

#define	IsWireMode()	GetDocument()->IsDocFlag(NCDOC_WIRE)
#define	IsLatheMode()	GetDocument()->IsDocFlag(NCDOC_LATHE)

// 底面描画座標生成ｽﾚｯﾄﾞ用
struct CREATEBOTTOMVERTEXPARAM {
#ifdef _DEBUG
	int		dbgThread;		// ｽﾚｯﾄﾞID
#endif
	CEvent		evStart,
				evEnd;
	BOOL		bThread,
				bResult;
	CNCDoc*		pDoc;
	size_t		s, e;
	CVBtmDraw	vBD;		// from NCdata.h
	// CEvent を手動ｲﾍﾞﾝﾄにするためのｺﾝｽﾄﾗｸﾀ
	CREATEBOTTOMVERTEXPARAM() : evStart(FALSE, TRUE), evEnd(FALSE, TRUE),
		bThread(TRUE), bResult(TRUE)
	{}
};
typedef	CREATEBOTTOMVERTEXPARAM*	LPCREATEBOTTOMVERTEXPARAM ;

#ifdef _WIN64
const size_t	MAXOBJ = 400;	// １つのｽﾚｯﾄﾞが一度に処理するｵﾌﾞｼﾞｪｸﾄ数
#else
const size_t	MAXOBJ = 200;
#endif

// 頂点配列生成ｽﾚｯﾄﾞ用
struct CREATEELEMENTPARAM {
#ifdef _DEBUG
	int		dbgThread;
#endif
	const GLfloat*	pfXYZ;
	GLfloat*		pfNOR;
	const GLbyte*	pbStl;
	BOOL	bResult;
	int		cx, cy,
			cs, ce;
	GLfloat	h, l;
	// 頂点配列ｲﾝﾃﾞｯｸｽ(可変長2次元配列)
	vector<CVelement>	vvElementCut,	// from NCdata.h
						vvElementWrk;
};
typedef	CREATEELEMENTPARAM*		LPCREATEELEMENTPARAM;

static	void	OutputGLErrorMessage(GLenum, UINT);
static	UINT	AddBottomVertexThread(LPVOID);
static	UINT	CreateElementThread(LPVOID);
static	void	CreateElementCut(LPCREATEELEMENTPARAM);
static	void	CreateElementTop(LPCREATEELEMENTPARAM);
static	void	CreateElementBtm(LPCREATEELEMENTPARAM);
static	BOOL	CreateElementSide(LPCREATEELEMENTPARAM, BOOL);

//	拡大率表示のための変換単位
//	1ｲﾝﾁあたりのﾋﾟｸｾﾙ数 GetDeviceCaps(LOGPIXELS[X|Y]) = 96dpi
//	1ｲﾝﾁ == 25.4mm
static	const float	LOGPIXEL = 96 / 25.4f;
//	円柱表示の円分割数
static	const int		CYCLECOUNT = ARCCOUNT*8+1;	// 512分割
static	const float		CYCLESTEP  = PI2/(CYCLECOUNT-1);

// ｴﾝﾄﾞﾐﾙ描画用の面法線(初期化は起動時CNCVCApp::CNCVCApp()から)
static	boost::array<GLfloat, (ARCCOUNT+2)*NCXYZ>	GLMillUpNor;	// 上面
static	boost::array<GLfloat, (ARCCOUNT+2)*NCXYZ>	GLMillDwNor;	// 下面
static	boost::array<GLfloat, (ARCCOUNT+1)*2*NCXYZ>	GLMillSdNor;	// 側面
static	boost::array<GLfloat, ((ARCCOUNT+1)*2*(ARCCOUNT/4-2)+(ARCCOUNT+2))*NCXYZ>
													GLMillPhNor;	// ﾎﾞｰﾙｴﾝﾄﾞﾐﾙ

// CNCViewGL
IMPLEMENT_DYNCREATE(CNCViewGL, CView)

BEGIN_MESSAGE_MAP(CNCViewGL, CView)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_WM_CONTEXTMENU()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MBUTTONDOWN()
	ON_WM_MBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEWHEEL()
	ON_WM_KEYDOWN()
	ON_WM_TIMER()
	// ﾍﾟｰｼﾞ切替ｲﾍﾞﾝﾄ
	ON_MESSAGE (WM_USERACTIVATEPAGE, &CNCViewGL::OnUserActivatePage)
	// 各ﾋﾞｭｰへのﾌｨｯﾄﾒｯｾｰｼﾞ
	ON_MESSAGE (WM_USERVIEWFITMSG, &CNCViewGL::OnUserViewFitMsg)
	// ﾒﾆｭｰｺﾏﾝﾄﾞ
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, &CNCViewGL::OnUpdateEditCopy)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_UP,  ID_VIEW_RT,  &CNCViewGL::OnUpdateMoveRoundKey)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_RUP, ID_VIEW_RRT, &CNCViewGL::OnUpdateMoveRoundKey)
	ON_COMMAND_RANGE(ID_VIEW_UP,  ID_VIEW_RT,    &CNCViewGL::OnMoveKey)
	ON_COMMAND_RANGE(ID_VIEW_RUP, ID_VIEW_RRT,   &CNCViewGL::OnRoundKey)
	ON_COMMAND_RANGE(ID_VIEW_FIT, ID_VIEW_LENSN, &CNCViewGL::OnLensKey)
	ON_COMMAND(ID_OPTION_DEFVIEWINFO, &CNCViewGL::OnDefViewInfo)
	//
	ON_MESSAGE(WM_USERTRACESELECT, &CNCViewGL::OnSelectTrace)
END_MESSAGE_MAP()

//	ｳｨﾝﾄﾞｳﾒｯｾｰｼﾞを受信し、OpenGL命令を操作するときは
//	::wglMakeCurrent( pDC->GetSafeHdc(), m_hRC );
//	-- OpenGL命令 --
//	::wglMakeCurrent( NULL, NULL );
//	を必ず入れておく

/////////////////////////////////////////////////////////////////////////////
// CNCViewGL クラスの構築/消滅

CNCViewGL::CNCViewGL()
{
#ifdef _DEBUG_FILEOPEN
	g_dbg.printf("CNCViewGL::CNCViewGL() Start");
#endif
	m_bActive = m_bSizeChg = FALSE;
	m_cx = m_cy = m_icx = m_icy = 0;
	m_dRate = m_dRoundAngle = m_dRoundStep = 0.0f;
	m_hRC = NULL;
	m_glCode = 0;

	m_pfDepth = m_pfXYZ = m_pfNOR = NULL;
	m_pbStencil = NULL;
	m_nVBOsize = 0;
	m_nVertexID[0] = m_nVertexID[1] =
		m_nPictureID = m_nTextureID = 0;
	m_pSolidElement = m_pLocusElement = NULL;

	m_enTrackingMode = TM_NONE;
	ClearObjectForm();
}

CNCViewGL::~CNCViewGL()
{
	if ( m_pfDepth )
		delete[]	m_pfDepth;
	if ( m_pfXYZ )
		delete[]	m_pfXYZ;
	if ( m_pfNOR )
		delete[]	m_pfNOR;
	if ( m_pbStencil )
		delete[]	m_pbStencil;
	ClearVBO();
}

/////////////////////////////////////////////////////////////////////////////
// CNCViewGL クラスのオーバライド関数

BOOL CNCViewGL::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.style |= WS_CLIPSIBLINGS|WS_CLIPCHILDREN;
	return __super::PreCreateWindow(cs);
}

void CNCViewGL::OnInitialUpdate() 
{
	extern	LPCTSTR	g_szNdelimiter;	// "XYZUVWIJKRPLDH" from NCDoc.cpp

#ifdef _DEBUG
	CMagaDbg	dbg("CNCViewGL::OnInitialUpdate()\nStart", DBG_CYAN);
#endif
	__super::OnInitialUpdate();

	// ｶﾞｲﾄﾞ表示
	if ( IsLatheMode() ) {
		m_strGuide  = g_szNdelimiter[NCA_Z];	// [ZYX]
		m_strGuide += g_szNdelimiter[NCA_Y];
		m_strGuide += g_szNdelimiter[NCA_X];
	}
	else {
		m_strGuide = g_szNdelimiter;
		m_strGuide.Delete(NCXYZ, 999);			// [XYZ]
	}

	// まだｳｨﾝﾄﾞｳがｱｸﾃｨﾌﾞでない可能性があるので
	// ここでの OpenGLｺﾏﾝﾄﾞ は使用しない
}

void CNCViewGL::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();

	switch ( lHint ) {
	case UAV_DRAWMAXRECT:
		return;		// 無視
	case UAV_TRACECURSOR:
		if ( (UINT_PTR)pHint == ID_NCVIEW_TRACE_CURSOR ) {
			// ｶｰｿﾙ位置まで実行（範囲制限）
			CClientDC	dc(this);
			::wglMakeCurrent( dc.GetSafeHdc(), m_hRC );
			if ( IsLatheMode() )
				CreateLathe(TRUE);
			else
				CreateBoxel(TRUE);
			::wglMakeCurrent( NULL, NULL );
		}
		else {
			// ｶｰｿﾙ位置から実行
			OnSelectTrace(NULL, NULL);	// ﾄﾚｰｽ実行の開始準備
			OnSelectTrace((WPARAM)(GetDocument()->GetNCdata(GetDocument()->GetTraceStart())), NULL);
		}
		Invalidate(FALSE);
		return;
	case UAV_STARTSTOPTRACE:
		if ( GetDocument()->GetTraceMode() != ID_NCVIEW_TRACE_STOP ) {
			OnSelectTrace(NULL, NULL);
			Invalidate(FALSE);
			return;
		}
		// if (GetTraceMode()==ID_NCVIEW_TRACE_STOP) through
	case UAV_FILEINSERT:	// 占有矩形の変更
		pOpt->m_dwUpdateFlg = VIEWUPDATE_DISPLAYLIST | VIEWUPDATE_BOXEL;
		pHint = (CObject *)1;	// dummy
		// through
	case UAV_DRAWWORKRECT:	// ﾜｰｸ矩形変更
		if ( pOpt->GetNCViewFlg(NCVIEWFLG_SOLIDVIEW) )
			pOpt->m_dwUpdateFlg |= VIEWUPDATE_BOXEL;
		// through
	case UAV_CHANGEFONT:	// 色の変更 etc.
		if ( m_bActive ) {
			GLdouble	objXform[4][4];
			CPointF		ptCenter(m_ptCenter);
			if ( pOpt->m_dwUpdateFlg & VIEWUPDATE_BOXEL ) {
				// 行列ﾏﾄﾘｸｽのﾊﾞｯｸｱｯﾌﾟ
				memcpy(objXform, m_objXform, sizeof(objXform));
				// 行列ﾏﾄﾘｸｽの初期化
				OnLensKey(ID_VIEW_FIT);
			}
			// 表示情報の更新
			UpdateViewOption();
			if ( pOpt->m_dwUpdateFlg & VIEWUPDATE_BOXEL ) {
				// 行列ﾏﾄﾘｸｽのﾘｽﾄｱ
				memcpy(m_objXform, objXform, sizeof(objXform));
				m_ptCenter = ptCenter;
				// 回転の復元
				CClientDC	dc(this);
				::wglMakeCurrent( dc.GetSafeHdc(), m_hRC );
				SetupViewingTransform();
				::wglMakeCurrent( NULL, NULL );
			}
		}
		pOpt->m_dwUpdateFlg = 0;
		// through
	case UAV_ADDINREDRAW:
		Invalidate(FALSE);
		return;
	}
	__super::OnUpdate(pSender, lHint, pHint);
}

void CNCViewGL::UpdateViewOption(void)
{
	CWaitCursor		wait;
	CViewOption*	pOpt = AfxGetNCVCApp()->GetViewOption();

	CClientDC	dc(this);
	::wglMakeCurrent( dc.GetSafeHdc(), m_hRC );

	// ﾃｸｽﾁｬ画像の読み込み
	if ( pOpt->GetNCViewFlg(NCVIEWFLG_SOLIDVIEW) &&
					pOpt->m_dwUpdateFlg & VIEWUPDATE_TEXTURE ) {
		ClearTexture();
		if ( pOpt->GetNCViewFlg(NCVIEWFLG_TEXTURE) ) {
			if ( ReadTexture(pOpt->GetTextureFile()) &&	// m_nPictureID値ｾｯﾄ
					!(pOpt->m_dwUpdateFlg & VIEWUPDATE_BOXEL) ) {
				// 画像だけ変更
				if ( IsLatheMode() )
					CreateTextureLathe();
				else if ( IsWireMode() )
					CreateTextureWire();
				else
					CreateTextureMill();
			}
		}
	}

	// 光源の色設定
	if ( pOpt->m_dwUpdateFlg & (VIEWUPDATE_LIGHT|VIEWUPDATE_TEXTURE) ) {
		COLORREF	col;
		GLfloat light_Wrk[] = {0.0f, 0.0f, 0.0f, 1.0f},
				light_Cut[] = {0.0f, 0.0f, 0.0f, 1.0f},
				light_Mil[] = {0.0f, 0.0f, 0.0f, 1.0f};
		GLfloat light_Position0[] = {-1.0f, -1.0f, -1.0f,  0.0f},
				light_Position1[] = { 1.0f,  1.0f,  1.0f,  0.0f};
		if ( m_nPictureID > 0 ) {
			// ﾃｸｽﾁｬ色を有効にするため白色光源
			for ( int i=0; i<NCXYZ; i++ )
				light_Wrk[i] = light_Cut[i] = 1.0f;
		}
		else {
			// 表示設定に基づく光源
			col = pOpt->GetNcDrawColor(NCCOL_GL_WRK);
			light_Wrk[0] = (GLfloat)GetRValue(col) / 255;	// 255 -> 1.0
			light_Wrk[1] = (GLfloat)GetGValue(col) / 255;
			light_Wrk[2] = (GLfloat)GetBValue(col) / 255;
			col = pOpt->GetNcDrawColor(NCCOL_GL_CUT);
			light_Cut[0] = (GLfloat)GetRValue(col) / 255;
			light_Cut[1] = (GLfloat)GetGValue(col) / 255;
			light_Cut[2] = (GLfloat)GetBValue(col) / 255;
		}
		col = pOpt->GetNcDrawColor(NCCOL_GL_MILL);
		light_Mil[0] = (GLfloat)GetRValue(col) / 255;
		light_Mil[1] = (GLfloat)GetGValue(col) / 255;
		light_Mil[2] = (GLfloat)GetBValue(col) / 255;
		//
		::glLightfv(GL_LIGHT0, GL_DIFFUSE,  light_Wrk);
		::glLightfv(GL_LIGHT1, GL_DIFFUSE,  light_Wrk);
		::glLightfv(GL_LIGHT2, GL_DIFFUSE,  light_Cut);
		::glLightfv(GL_LIGHT3, GL_DIFFUSE,  light_Cut);
		::glLightfv(GL_LIGHT4, GL_DIFFUSE,  light_Mil);
		::glLightfv(GL_LIGHT5, GL_DIFFUSE,  light_Mil);
		::glLightfv(GL_LIGHT0, GL_POSITION, light_Position0);
		::glLightfv(GL_LIGHT1, GL_POSITION, light_Position1);
		::glLightfv(GL_LIGHT2, GL_POSITION, light_Position0);
		::glLightfv(GL_LIGHT3, GL_POSITION, light_Position1);
		::glLightfv(GL_LIGHT4, GL_POSITION, light_Position0);
		::glLightfv(GL_LIGHT5, GL_POSITION, light_Position1);
	}

	// ﾜｲﾔｰ(ﾃﾞｨｽﾌﾟﾚｲﾘｽﾄ)
	if ( pOpt->m_dwUpdateFlg & VIEWUPDATE_DISPLAYLIST ) {
		if ( m_glCode > 0 )
			::glDeleteLists(m_glCode, 1);
		if ( !IsWireMode() )
			CreateDisplayList();
	}

	// ソリッドモデルの表示
	if ( pOpt->m_dwUpdateFlg & VIEWUPDATE_BOXEL ) {
		ClearVBO();
		// ﾜｰｸ矩形の描画用座標
		if ( pOpt->GetNCViewFlg(NCVIEWFLG_SOLIDVIEW) ) {
			BOOL	bResult;
			// 切削領域の設定
			m_rcDraw = GetDocument()->GetWorkRect();
			if ( IsLatheMode() ) {
				// 旋盤用回転モデルの生成
				bResult = CreateLathe();
				if ( bResult && m_nPictureID > 0 )
					CreateTextureLathe();	// ﾃｸｽﾁｬ座標の生成
			}
			else if ( IsWireMode() ) {
				// ﾜｲﾔ加工
				bResult = CreateWire();
				if ( bResult && m_nPictureID > 0 )
					CreateTextureWire();
			}
			else {
				// ﾌﾗｲｽ用ﾎﾞｸｾﾙの生成
				bResult = CreateBoxel();
				if ( bResult && m_nPictureID > 0 )
					CreateTextureMill();
			}
		}
	}

	::wglMakeCurrent( NULL, NULL );
	Invalidate(FALSE);
}

BOOL CNCViewGL::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) 
{
#ifdef _DEBUG_CMDMSG
	g_dbg.printf("CNCViewGL::OnCmdMsg()");
#endif
	// ここから CDocument::OnCmdMsg() を呼ばないようにする
//	return __super::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
	return CWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

/////////////////////////////////////////////////////////////////////////////
// CNCViewGL クラスのメンバ関数

BOOL CNCViewGL::SetupPixelFormat(CDC* pDC)
{
	static PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,
		32,
		0, 0, 0, 0, 0, 0,
		0,
		0,
		0,
		0, 0, 0, 0,
		24,		// Depth   Buffer
		8,		// Stencil Buffer
		0,
		PFD_MAIN_PLANE,
		0,
		0, 0, 0
	};

	int	iPixelFormat;
	if( !(iPixelFormat = ::ChoosePixelFormat(pDC->GetSafeHdc(), &pfd)) ) {
		TRACE0( "ChoosePixelFormat is failed" );
		return FALSE;
	}

	if( !::SetPixelFormat(pDC->GetSafeHdc(), iPixelFormat, &pfd) ) {
		TRACE0( "SetPixelFormat is failed" );
		return FALSE;
	}

    return TRUE;
}

void CNCViewGL::ClearObjectForm(void)
{
	m_ptCenter = 0.0f;
	m_objXform[0][0] = 1.0; m_objXform[0][1] = 0.0; m_objXform[0][2] = 0.0; m_objXform[0][3] = 0.0;
	m_objXform[1][0] = 0.0; m_objXform[1][1] = 1.0; m_objXform[1][2] = 0.0; m_objXform[1][3] = 0.0;
	m_objXform[2][0] = 0.0; m_objXform[2][1] = 0.0; m_objXform[2][2] = 1.0; m_objXform[2][3] = 0.0;
	m_objXform[3][0] = 0.0; m_objXform[3][1] = 0.0; m_objXform[3][2] = 0.0; m_objXform[3][3] = 1.0;
}

void CNCViewGL::CreateDisplayList(void)
{
	GLenum err = ::glGetError();		// ｴﾗｰをﾌﾗｯｼｭ

	m_glCode = ::glGenLists(1);
	if( m_glCode > 0 ) {
		// NCﾃﾞｰﾀ描画のﾃﾞｨｽﾌﾟﾚｲﾘｽﾄ生成
		::glNewList( m_glCode, GL_COMPILE );
			RenderCode();
		::glEndList();
		if ( ::glGetError() != GL_NO_ERROR ) {
			::glDeleteLists(m_glCode, 1);
			m_glCode = 0;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
//	ﾌﾗｲｽ加工表示ﾓﾃﾞﾘﾝｸﾞ
/////////////////////////////////////////////////////////////////////////////

BOOL CNCViewGL::CreateBoxel(BOOL bRange)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CreateBoxel()\nStart");
#endif
	BOOL	bResult;
	CProgressCtrl*	pProgress = AfxGetNCVCMainWnd()->GetProgressCtrl();
	pProgress->SetRange32(0, 100);		// 100%表記
	pProgress->SetPos(0);

	// ﾎﾞｸｾﾙ生成のための初期設定
	InitialBoxel();
	// 画面いっぱいに描画
	::glMatrixMode(GL_PROJECTION);
	::glPushMatrix();
	::glLoadIdentity();
	::glOrtho(m_rcDraw.left, m_rcDraw.right, m_rcDraw.top, m_rcDraw.bottom,
		m_rcView.low, m_rcView.high);	// m_rcDraw ではｷﾞﾘｷﾞﾘなので m_rcView を使う
	::glMatrixMode(GL_MODELVIEW);
#ifdef _DEBUG
	dbg.printf("(%f,%f)-(%f,%f)", m_rcDraw.left, m_rcDraw.top, m_rcDraw.right, m_rcDraw.bottom);
	dbg.printf("(%f,%f) m_rcView(l,h)", m_rcView.low, m_rcView.high);
	dbg.printf("(%f,%f) m_rcDraw(l,h)", m_rcDraw.low, m_rcDraw.high);
#endif

	if ( GetDocument()->IsDocFlag(NCDOC_WORKFILE) ) {
		// 図形ファイルと重ねるとき
		bResult = CreateBoxel_fromIGES();
		// ｽﾃﾝｼﾙﾊﾞｯﾌｧは削除
		if ( m_pbStencil ) {
			delete[]	m_pbStencil;
			m_pbStencil = NULL;
		}
	}
	else {
		// 切削底面の描画（デプス値の更新）
#ifdef _DEBUG
		DWORD	t1 = ::timeGetTime();
#endif
		bResult = CreateBottomFaceThread(bRange, 80);
		if ( bResult ) {
#ifdef _DEBUG
			DWORD	t2 = ::timeGetTime();
			dbg.printf( "CreateBottomFaceThread()=%d[ms]", t2 - t1 );
#endif
			// デプス値の取得
			bResult = GetDocument()->IsDocFlag(NCDOC_CYLINDER) ?
				GetClipDepthCylinder(bRange) : GetClipDepthMill(bRange);
		}
	}

	if ( !bResult )
		ClearVBO();

	// 終了処理
	m_bSizeChg = FALSE;
	FinalBoxel();
	pProgress->SetPos(0);

	return bResult;
}

BOOL CNCViewGL::CreateBoxel_fromIGES(void)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CreateBoxel_fromIGES()\nStart");
	DWORD	t1 = ::timeGetTime();
#endif
	int		i;
	Describe_BODY	bd;
	BODYList*	kbl = GetDocument()->GetKodatunoBodyList();
	if ( !kbl ) {
#ifdef _DEBUG
		dbg.printf("GetKodatunoBodyList() error");
#endif
		return FALSE;
	}

	::glPushAttrib(GL_ALL_ATTRIB_BITS);

	// 図形モデルの描画（深さ優先でデプス値の更新＋ステンシルビットの更新）
	::glEnable(GL_STENCIL_TEST);
	::glClear(GL_STENCIL_BUFFER_BIT);
	::glStencilFunc(GL_ALWAYS, 0x01, 0xff);
	::glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
	for ( i=0; i<kbl->getNum(); i++ ) {
		bd.DrawBody( (BODY *)kbl->getData(i) );
	}
#ifdef _DEBUG
	DWORD	t2 = ::timeGetTime();
	dbg.printf( "DrawBody(first)=%d[ms]", t2 - t1 );
#endif

	// 切削底面の描画（ステンシルビットが１のとこだけデプス値を更新）
	::glStencilFunc(GL_EQUAL, 0x01, 0xff);
	::glStencilOp(GL_KEEP, GL_KEEP, GL_DECR);	// Zpassだけ-1 → 貫通
	if ( !CreateBottomFaceThread(TRUE, 40) ) {
		::glDisable(GL_STENCIL_TEST);
		::glPopAttrib();
		return FALSE;
	}
#ifdef _DEBUG
	DWORD	t3 = ::timeGetTime();
	dbg.printf( "CreateBottomFaceThread(first)=%d[ms]", t3 - t2 );
#endif

	// 底面のボクセルを構築
	if ( !GetClipDepthMill(TRUE, DP_BottomStencil) ) {
		::glDisable(GL_STENCIL_TEST);
		::glPopAttrib();
		return FALSE;
	}
#ifdef _DEBUG
	DWORD	t4 = ::timeGetTime();
	dbg.printf( "GetClipDepthMill(DP_BottomStencil)=%d[ms]", t4 - t3 );
#endif

	// 手前優先で図形モデルを描画（ステンシルビットが１のとこだけ描画）
	::glDepthFunc(GL_LESS);
	::glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	for ( i=0; i<kbl->getNum(); i++ ) {
		bd.DrawBody( (BODY *)kbl->getData(i) );
	}
#ifdef _DEBUG
	DWORD	t5 = ::timeGetTime();
	dbg.printf( "DrawBody(second)=%d[ms]", t5 - t4 );
#endif

	// 切削底面の描画（深さ優先でステンシルビットが１のとこだけ描画）
	::glDepthFunc(GL_GREATER);
	::glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);	// Zpassだけ+1 → 切削面
	if ( !CreateBottomFaceThread(TRUE, 80) ) {
		::glDisable(GL_STENCIL_TEST);
		::glPopAttrib();
		return FALSE;
	}
#ifdef _DEBUG
	DWORD	t6 = ::timeGetTime();
	dbg.printf( "CreateBottomFaceThread(second)=%d[ms]", t6 - t5 );
#endif

	// 上面のボクセルを構築
	if ( !GetClipDepthMill(FALSE, DP_TopStencil) ) {
		::glDisable(GL_STENCIL_TEST);
		::glPopAttrib();
		return FALSE;
	}
#ifdef _DEBUG
	DWORD	t7 = ::timeGetTime();
	dbg.printf( "GetClipDepthMill(DP_TopStencil)=%d[ms]", t7 - t6 );
#endif

	::glDisable(GL_STENCIL_TEST);
	::glPopAttrib();

	// 頂点配列ﾊﾞｯﾌｧｵﾌﾞｼﾞｪｸﾄ生成
	BOOL	bResult = CreateVBOMill();
#ifdef _DEBUG
	DWORD	t8 = ::timeGetTime();
	dbg.printf( "CreateVBOMill()=%d[ms]", t8 - t7 );
#endif

	return bResult;
}

BOOL CNCViewGL::CreateBottomFaceThread(BOOL bRange, int nProgress)
{
	BOOL	bResult = TRUE;
	size_t	i, n, e, p, nLoop, proc;
	UINT	pp = 0;		// progress position
	DWORD	dwResult, id;

	if ( bRange ) {
		e = GetDocument()->GetTraceStart();
		nLoop = GetDocument()->GetTraceDraw();
	}
	else {
		e = 0;
		nLoop = GetDocument()->GetNCsize();
	}
	if ( nLoop > 0 ) {
		proc  = min(nLoop, (size_t)g_nProcesser);
//		proc  = min(MAXIMUM_WAIT_OBJECTS, min(nLoop, (size_t)g_nProcesser*2));
#ifdef _DEBUG
//		proc = 1;
#endif
		n = min(nLoop/proc, MAXOBJ);	// 1つのｽﾚｯﾄﾞが処理する最大ﾌﾞﾛｯｸ数
		LPCREATEBOTTOMVERTEXPARAM			pParam;
		// WaitForMultipleObjects(FALSE)で待つには
		// CWinThread::m_bAutoDeleteが邪魔をして不具合発生するので
		// 終了イベントオブジェクトCREATEBOTTOMVERTEXPARAM::evEnd.m_hObjectで待つ
		vector<HANDLE>						vThread;
		vector<LPCREATEBOTTOMVERTEXPARAM>	vParam, vParamEnd;
		// CPUの数だけｽﾚｯﾄﾞ生成と実行
		for ( i=0; i<proc; i++ ) {
			pParam = new CREATEBOTTOMVERTEXPARAM;
#ifdef _DEBUG
			pParam->dbgThread = i;
#endif
			pParam->pDoc = GetDocument();
			pParam->s = e;
			pParam->e = e = min(e+n, nLoop);
			AfxBeginThread(AddBottomVertexThread, pParam);
			vThread.push_back(pParam->evEnd.m_hObject);
			vParam.push_back(pParam);
			pParam->evStart.SetEvent();
		}
		// 切削底面描画
		::glEnableClientState(GL_VERTEX_ARRAY);
		while ( !vThread.empty() ) {
			// ｽﾚｯﾄﾞ1つ終わるごとに描画処理
			dwResult = ::WaitForMultipleObjects((DWORD)vThread.size(), &vThread[0], FALSE, INFINITE);
			id = dwResult - WAIT_OBJECT_0;
			if ( id < 0 || id >= vThread.size() || !vParam[id]->bResult ) {
				bResult = FALSE;
				break;
			}
			for ( const auto& v : vParam[id]->vBD ) {
				if ( !v.vpt.empty() )
					::glVertexPointer(NCXYZ, GL_FLOAT, 0, &(v.vpt[0]));
				if ( v.re == 0 )
					::glDrawArrays(v.mode, 0, (GLsizei)(v.vpt.size()/NCXYZ));
				else
					::glDrawRangeElements(v.mode, v.rs, v.re,
						(GLsizei)(v.vel.size()), GL_UNSIGNED_INT, &(v.vel[0]));
			}
			if ( e < nLoop ) {
				for ( auto& v : vParam[id]->vBD ) {
					v.vpt.clear();
					v.vel.clear();
				}
				vParam[id]->vBD.clear();
				vParam[id]->s = e;
				vParam[id]->e = e = min(e+n, nLoop);
				vParam[id]->evEnd.ResetEvent();
				vParam[id]->evStart.SetEvent();
			}
			else {
				vParam[id]->bThread = FALSE;
				vParam[id]->evStart.SetEvent();
				vParamEnd.push_back(vParam[id]);// delete vParam[id];
				vThread.erase(vThread.begin()+id);
				vParam.erase(vParam.begin()+id);
			}
			p = e*nProgress / nLoop;	// MAX 70% or 35%
			if ( p >= pp ) {
				while ( p >= pp )
					pp += 10;
				AfxGetNCVCMainWnd()->GetProgressCtrl()->SetPos(pp);	// 10%ずつ
			}
		}
		::glDisableClientState(GL_VERTEX_ARRAY);
		GLenum glError = ::glGetError();
		::glFinish();
		for ( auto v : vParamEnd )
			delete	v;
		if ( !bResult ) {
			for ( auto v : vParam )
				delete	v;
			return FALSE;
		}
	}	// end of Loop>0

	return bResult;
}

UINT AddBottomVertexThread(LPVOID pVoid)
{
	LPCREATEBOTTOMVERTEXPARAM pParam = reinterpret_cast<LPCREATEBOTTOMVERTEXPARAM>(pVoid);
#ifdef _DEBUG
	CMagaDbg	dbg(DBG_BLUE);
	DWORD		t1, t2;
	size_t		s;
#endif
	BOOL		bStartDraw;	// 始点の描画が必要かどうか
	try {
		while ( TRUE ) {
			// 実行許可待ち
			pParam->evStart.Lock();
			pParam->evStart.ResetEvent();
			if ( !pParam->bThread )
				break;
			bStartDraw = TRUE;
#ifdef _DEBUG
			dbg.printf("AddBottomVertexThread() ThreadID=%d s=%d e=%d Start!",
				pParam->dbgThread, pParam->s, pParam->e);
			t1 = ::timeGetTime();
#endif
			for ( size_t i=pParam->s; i<pParam->e; i++ )
				bStartDraw = pParam->pDoc->GetNCdata(i)->AddGLBottomFaceVertex(pParam->vBD, bStartDraw);
#ifdef _DEBUG
			t2 = ::timeGetTime();
			s = 0;
			for ( const auto& v : pParam->vBD )
				s += v.vpt.size();
			dbg.printf("                           ThreadID=%d End %d[ms] DrawCount=%d VertexSize=%d(/3)",
				pParam->dbgThread, t2 - t1, pParam->vBD.size(), s);
#endif
			// 終了イベント
			pParam->evEnd.SetEvent();
		}
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		pParam->bResult = FALSE;
	}

#ifdef _DEBUG
	dbg.printf("AddBottomVertexThread() ThreadID=%d thread end.",
		pParam->dbgThread);
#endif
	return 0;
}

BOOL CNCViewGL::GetClipDepthMill(BOOL bDepthKeep, ENCLIPDEPTH enStencil)
{
#ifdef _DEBUG
	CMagaDbg	dbg("GetClipDepthMill()\nStart");
#endif
	BOOL		bRecalc;
	size_t		i, j, n, nnu, nnd, nSize;
	int			icx, icy;
	GLint		viewPort[4];
	GLdouble	mvMatrix[16], pjMatrix[16],
				wx1, wy1, wz1, wx2, wy2, wz2;
	GLenum		glError;

	::glGetIntegerv(GL_VIEWPORT, viewPort);
	::glGetDoublev (GL_MODELVIEW_MATRIX,  mvMatrix);
	::glGetDoublev (GL_PROJECTION_MATRIX, pjMatrix);

	// 矩形領域をﾋﾟｸｾﾙ座標に変換
	::gluProject(m_rcDraw.left,  m_rcDraw.top,    0.0, mvMatrix, pjMatrix, viewPort,
		&wx1, &wy1, &wz1);
	::gluProject(m_rcDraw.right, m_rcDraw.bottom, 0.0, mvMatrix, pjMatrix, viewPort,
		&wx2, &wy2, &wz2);
	icx = (int)(wx2 - wx1);
	icy = (int)(wy2 - wy1);
	if ( icx<=0 || icy<=0 )
		return FALSE;
	m_wx = (GLint)wx1;
	m_wy = (GLint)wy1;
#ifdef _DEBUG
	dbg.printf("left,  top   =(%f, %f)", m_rcDraw.left,  m_rcDraw.top);
	dbg.printf("right, bottom=(%f, %f)", m_rcDraw.right, m_rcDraw.bottom);
	dbg.printf("wx1,   wy1   =(%f, %f)", wx1, wy1);
	dbg.printf("wx2,   wy2   =(%f, %f)", wx2, wy2);
	dbg.printf("icx=%d icy=%d", icx, icy);
	DWORD	t1 = ::timeGetTime();
#endif

	// 領域確保
//	if ( m_pfDepth &&
//			(m_icx!=icx || m_icy!=icy || GetDocument()->GetTraceMode()==ID_NCVIEW_TRACE_STOP) ) {
//		delete[]	m_pfDepth;
//		delete[]	m_pfXYZ;
//		delete[]	m_pfNOR;
//		m_pfDepth = m_pfXYZ = m_pfNOR = NULL;
//	}
	if ( m_icx!=icx || m_icy!=icy ) {
		if ( m_pfDepth ) {
			delete[]	m_pfDepth;
			delete[]	m_pfXYZ;
			delete[]	m_pfNOR;
			m_pfDepth = m_pfXYZ = m_pfNOR = NULL;
		}
		if ( m_pbStencil ) {
			delete[]	m_pbStencil;
			m_pbStencil = NULL;
		}
	}
	m_icx = icx;
	m_icy = icy;
	nSize = m_icx * m_icy;
	try {
		if ( !m_pfDepth ) {
			m_pfDepth = new GLfloat[nSize];
			nSize  *= NCXYZ;				// XYZ座標格納
			m_pfXYZ = new GLfloat[nSize*2];	// 上面と底面
			m_pfNOR = new GLfloat[nSize*2];
			bRecalc = TRUE;
		}
		else {
			nSize  *= NCXYZ;
			bRecalc = FALSE;
		}
		if ( enStencil!=DP_NoStencil && !m_pbStencil ) {
			m_pbStencil = new GLbyte[m_icx*m_icy];
		}
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		if ( m_pfDepth )
			delete[]	m_pfDepth;
		if ( m_pfXYZ )
			delete[]	m_pfXYZ;
		if ( m_pfNOR )
			delete[]	m_pfNOR;
		if ( m_pbStencil )
			delete[]	m_pbStencil;
		m_pfDepth = m_pfXYZ = m_pfNOR = NULL;
		m_pbStencil = NULL;
		return FALSE;
	}
#ifdef _DEBUG
	DWORD	t2 = ::timeGetTime();
	dbg.printf( "Memory alloc=%d[ms]", t2 - t1 );
#endif

	// ｸﾗｲｱﾝﾄ領域のﾃﾞﾌﾟｽ値を取得（ﾋﾟｸｾﾙ単位）
	::glPixelStorei(GL_PACK_ALIGNMENT, 1);
	::glReadPixels(m_wx, m_wy, m_icx, m_icy,
					GL_DEPTH_COMPONENT, GL_FLOAT, m_pfDepth);
//	ASSERT( ::glGetError() == GL_NO_ERROR );
	glError = ::glGetError();
#ifdef _DEBUG
	DWORD	t3 = ::timeGetTime();
	dbg.printf( "glReadPixels(GL_DEPTH_COMPONENT)=%d[ms] error=%d", t3 - t2, glError);
#endif
	if ( enStencil != DP_NoStencil ) {
		// ｸﾗｲｱﾝﾄ領域のｽﾃﾝｼﾙ値を取得
		::glReadPixels(m_wx, m_wy, m_icx, m_icy,
					GL_STENCIL_INDEX, GL_BYTE, m_pbStencil);
//		ASSERT( ::glGetError() == GL_NO_ERROR );
		glError = ::glGetError();
#ifdef _DEBUG
		DWORD	t31 = ::timeGetTime();
		dbg.printf( "glReadPixels(GL_STENCIL_INDEX)=%d[ms] error=%d", t31 - t3, glError);
		t3 = t31;
#endif
	}

	// ﾜｰｸ上面底面と切削面の座標登録
	PFN_GETCLIPDEPTHMILL	pfnGetClipDepthMill;
	switch ( enStencil ) {
	case DP_BottomStencil:
		ASSERT( m_pbStencil );
		pfnGetClipDepthMill = &CNCViewGL::GetClipDepthMill_BottomStencil;
		break;
	case DP_TopStencil:
		ASSERT( m_pbStencil );
		pfnGetClipDepthMill = &CNCViewGL::GetClipDepthMill_TopStencil;
		break;
	default:
		pfnGetClipDepthMill = bRecalc ? &CNCViewGL::GetClipDepthMill_All : &CNCViewGL::GetClipDepthMill_Zonly;
	}
	for ( j=0, n=0, nnu=0, nnd=nSize; j<(size_t)m_icy; j++ ) {
		for ( i=0; i<(size_t)m_icx; i++, n++, nnu+=NCXYZ, nnd+=NCXYZ ) {
			::gluUnProject(i+wx1, j+wy1, m_pfDepth[n],
					mvMatrix, pjMatrix, viewPort,
					&wx2, &wy2, &wz2);	// それほど遅くないので自作変換は中止
			// それぞれの座標登録へ
			(this->*pfnGetClipDepthMill)(wx2, wy2, wz2, nnu, nnd);
		}
	}
#ifdef _DEBUG
	DWORD	t4 = ::timeGetTime();
	dbg.printf( "AddMatrix=%d[ms]", t4 - t3 );
#endif

#ifdef _DEBUG_FILEOUT_
	extern	LPCTSTR	gg_szCat;		// ", "
	extern	LPCTSTR	gg_szReturn;	// "\n"
	CStdioFile	dbg_fx("C:\\Users\\magara\\Documents\\tmp\\depth_x.csv", CFile::typeText|CFile::modeCreate|CFile::modeWrite);
	CStdioFile	dbg_fy("C:\\Users\\magara\\Documents\\tmp\\depth_y.csv", CFile::typeText|CFile::modeCreate|CFile::modeWrite);
	CStdioFile	dbg_fz;
	CStdioFile	dbg_fd;
	CStdioFile	dbg_fs;
	if ( enStencil == DP_BottomStencil ) {
		dbg_fz.Open("C:\\Users\\magara\\Documents\\tmp\\depth_z_bottom.csv", CFile::typeText|CFile::modeCreate|CFile::modeWrite);
		dbg_fd.Open("C:\\Users\\magara\\Documents\\tmp\\depth_f_bottom.csv", CFile::typeText|CFile::modeCreate|CFile::modeWrite);
		dbg_fs.Open("C:\\Users\\magara\\Documents\\tmp\\stencil_bottom.csv", CFile::typeText|CFile::modeCreate|CFile::modeWrite);
	}
	else if ( enStencil == DP_TopStencil ) {
		dbg_fz.Open("C:\\Users\\magara\\Documents\\tmp\\depth_z_top.csv", CFile::typeText|CFile::modeCreate|CFile::modeWrite);
		dbg_fd.Open("C:\\Users\\magara\\Documents\\tmp\\depth_f_top.csv", CFile::typeText|CFile::modeCreate|CFile::modeWrite);
		dbg_fs.Open("C:\\Users\\magara\\Documents\\tmp\\stencil_top.csv", CFile::typeText|CFile::modeCreate|CFile::modeWrite);
	}
	else {
		dbg_fz.Open("C:\\Users\\magara\\Documents\\tmp\\depth_z.csv", CFile::typeText|CFile::modeCreate|CFile::modeWrite);
		dbg_fd.Open("C:\\Users\\magara\\Documents\\tmp\\depth_f.csv", CFile::typeText|CFile::modeCreate|CFile::modeWrite);
	}
	CString		r,
				sd, sx, sy, sz, ss;
	for ( j=0, n=0; j<m_icy; j++ ) {
		sd.Empty();
		sx.Empty();	sy.Empty();	sz.Empty();	ss.Empty();
		for ( i=0; i<m_icx; i++, n++ ) {
			if ( !sd.IsEmpty() ) {
				sd += gg_szCat;
				sx += gg_szCat;
				sy += gg_szCat;
				sz += gg_szCat;
				ss += gg_szCat;
			}
			r.Format(IDS_MAKENCD_FORMAT, m_pfDepth[n]);
			sd += r;
			r.Format(IDS_MAKENCD_FORMAT, m_pfXYZ[n*NCXYZ+NCA_X]);
			sx += r;
			r.Format(IDS_MAKENCD_FORMAT, m_pfXYZ[n*NCXYZ+NCA_Y]);
			sy += r;
			r.Format(IDS_MAKENCD_FORMAT, m_pfXYZ[n*NCXYZ+NCA_Z]);
			sz += r;
			if ( enStencil != DP_NoStencil ) {
				r.Format("%d", m_pbStencil[n]);
				ss += r;
			}
		}
		dbg_fd.WriteString(sd + gg_szReturn);
		dbg_fx.WriteString(sx + gg_szReturn);
		dbg_fy.WriteString(sy + gg_szReturn);
		dbg_fz.WriteString(sz + gg_szReturn);
		if ( enStencil != DP_NoStencil )
			dbg_fs.WriteString(ss + gg_szReturn);
	}
#endif	// _DEBUG_FILEOUT_

	if ( enStencil != DP_NoStencil )
		return TRUE;	// ステンシル処理の場合はここまで

	//
	if ( GetDocument()->GetTraceMode() == ID_NCVIEW_TRACE_STOP )
		AfxGetNCVCMainWnd()->GetProgressCtrl()->SetPos(90);	

	// 頂点配列ﾊﾞｯﾌｧｵﾌﾞｼﾞｪｸﾄ生成
	BOOL	bResult = CreateVBOMill();
#ifdef _DEBUG
	DWORD	t5 = ::timeGetTime();
	dbg.printf( "CreateVBOMill() total=%d[ms]", t5 - t4 );
#endif

	if ( !bDepthKeep ) {
		delete[]	m_pfDepth;
		delete[]	m_pfXYZ;
		delete[]	m_pfNOR;
		m_pfDepth = m_pfXYZ = m_pfNOR = NULL;
	}

	return bResult;
}

void CNCViewGL::GetClipDepthMill_All(GLdouble wx, GLdouble wy, GLdouble wz, size_t tu, size_t td)
{
	// ﾜｰﾙﾄﾞ座標
	m_pfXYZ[tu+NCA_X] = m_pfXYZ[td+NCA_X] = (GLfloat)wx;
	m_pfXYZ[tu+NCA_Y] = m_pfXYZ[td+NCA_Y] = (GLfloat)wy;
	m_pfXYZ[tu+NCA_Z] = min((GLfloat)wz, m_rcDraw.high);	// 上面を超えない
	m_pfXYZ[td+NCA_Z] = m_rcDraw.low;	// 底面はm_rcDraw.low座標で敷き詰める
	// ﾃﾞﾌｫﾙﾄの法線ﾍﾞｸﾄﾙ
	m_pfNOR[tu+NCA_X] =  0.0f;
	m_pfNOR[tu+NCA_Y] =  0.0f;
	m_pfNOR[tu+NCA_Z] =  1.0f;
	m_pfNOR[td+NCA_X] =  0.0f;
	m_pfNOR[td+NCA_Y] =  0.0f;
	m_pfNOR[td+NCA_Z] = -1.0f;
}

void CNCViewGL::GetClipDepthMill_Zonly(GLdouble wx, GLdouble wy, GLdouble wz, size_t tu, size_t td)
{
	// 上面Z値のみ
	m_pfXYZ[tu+NCA_Z] = min((GLfloat)wz, m_rcDraw.high);
}

void CNCViewGL::GetClipDepthMill_BottomStencil(GLdouble wx, GLdouble wy, GLdouble wz, size_t tu, size_t td)
{
	// ステンシル値が"1"のとこだけボクセル取得
	// そうでないところは -FLT_MAX をセットして貫通を示す
	m_pfXYZ[td+NCA_X] = (GLfloat)wx;
	m_pfXYZ[td+NCA_Y] = (GLfloat)wy;
	m_pfXYZ[td+NCA_Z] = m_pbStencil[tu/NCXYZ]==1 ? (GLfloat)wz : -FLT_MAX;
	m_pfNOR[td+NCA_X] =  0.0f;
	m_pfNOR[td+NCA_Y] =  0.0f;
	m_pfNOR[td+NCA_Z] = -1.0f;
}

void CNCViewGL::GetClipDepthMill_TopStencil(GLdouble wx, GLdouble wy, GLdouble wz, size_t tu, size_t td)
{
	// ステンシル値が"0"より大きいとこだけボクセル取得
	// そうでないところは FLT_MAX をセットして貫通を示す
	m_pfXYZ[tu+NCA_X] = (GLfloat)wx;
	m_pfXYZ[tu+NCA_Y] = (GLfloat)wy;
	m_pfXYZ[tu+NCA_Z] = m_pbStencil[tu/NCXYZ]>0 ? (GLfloat)wz : FLT_MAX;
	m_pfNOR[tu+NCA_X] =  0.0f;
	m_pfNOR[tu+NCA_Y] =  0.0f;
	m_pfNOR[tu+NCA_Z] =  1.0f;
}

BOOL CNCViewGL::CreateVBOMill(void)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CreateVBOMill()", DBG_BLUE);
#endif

	CWinThread*				hThread;
	HANDLE*					pThread;
	LPCREATEELEMENTPARAM	pParam;

	int		i, cx, cy, n, proc;
	GLsizeiptr	nVBOsize;
	BOOL	bResult = TRUE;
	GLenum	errCode;
	UINT	errLine;

	// 頂点ｲﾝﾃﾞｯｸｽの消去
	if ( m_pSolidElement ) {
		::glDeleteBuffers((GLsizei)(m_vElementWrk.size()+m_vElementCut.size()), m_pSolidElement);
		delete[]	m_pSolidElement;
		m_pSolidElement = NULL;
	}
	m_vElementWrk.clear();
	m_vElementCut.clear();

	// 準備
	if ( GetDocument()->IsDocFlag(NCDOC_CYLINDER) ) {
		cx = CYCLECOUNT;
		cy = (int)(max(m_icx, m_icy) / 2.0 + 0.5);
	}
	else {
		cx = m_icx;
		cy = m_icy;
	}
	nVBOsize = cx * cy * NCXYZ * 2 * sizeof(GLfloat);
	proc = min(cy, g_nProcesser);
//	proc = min(MAXIMUM_WAIT_OBJECTS, min(cy, g_nProcesser*2));
	pThread	= new HANDLE[proc];
	pParam	= new CREATEELEMENTPARAM[proc];

	// CPU数ｽﾚｯﾄﾞ起動
	n = cy / proc;	// 1CPU当たりの処理数
	for ( i=0; i<proc; i++ ) {
#ifdef _DEBUG
		pParam[i].dbgThread = i;
#endif
		pParam[i].pfXYZ = m_pfXYZ;
		pParam[i].pfNOR = m_pfNOR;
		pParam[i].pbStl = GetDocument()->IsDocFlag(NCDOC_WORKFILE) ? m_pbStencil : NULL;
		pParam[i].bResult = TRUE;
		pParam[i].cx = cx;
		pParam[i].cy = cy;
		pParam[i].cs = n * i;
		pParam[i].ce = i==proc-1 ? (cy-1) : min(n*i+n, cy-1);
		pParam[i].h  = m_rcDraw.high;
		pParam[i].l  = m_rcDraw.low;
		pParam[i].vvElementCut.reserve(n+1);
		pParam[i].vvElementWrk.reserve((n+1)*2);
		hThread = AfxBeginThread(CreateElementThread, &pParam[i]);
		ASSERT( hThread );
		pThread[i] = hThread->m_hThread;
	}
	::WaitForMultipleObjects(proc, pThread, TRUE, INFINITE);
	for ( i=0, n=1; i<proc; i++ ) {
		if ( !pParam[i].bResult )
			n = 0;
	}

	// ﾜｰｸ側面は法線ﾍﾞｸﾄﾙの更新があり
	// 切削面処理との排他制御を考慮
	if ( !n || !CreateElementSide(&pParam[0], GetDocument()->IsDocFlag(NCDOC_CYLINDER)) ) {
		delete[]	pThread;
		delete[]	pParam;
		return FALSE;
	}

	errCode = ::glGetError();		// ｴﾗｰをﾌﾗｯｼｭ

	if ( GetDocument()->GetTraceMode() == ID_NCVIEW_TRACE_STOP )
		AfxGetNCVCMainWnd()->GetProgressCtrl()->SetPos(100);	

	// 頂点配列と法線ﾍﾞｸﾄﾙをGPUﾒﾓﾘに転送
	if ( m_nVBOsize==nVBOsize && m_nVertexID[0]>0 ) {
		::glBindBuffer(GL_ARRAY_BUFFER, m_nVertexID[0]);
		::glBufferSubData(GL_ARRAY_BUFFER, 0, nVBOsize, m_pfXYZ);
		::glBindBuffer(GL_ARRAY_BUFFER, m_nVertexID[1]);
		::glBufferSubData(GL_ARRAY_BUFFER, 0, nVBOsize, m_pfNOR);
	}
	else {
		if ( m_nVertexID[0] > 0 )
			::glDeleteBuffers(SIZEOF(m_nVertexID), m_nVertexID);
		::glGenBuffers(SIZEOF(m_nVertexID), m_nVertexID);
		::glBindBuffer(GL_ARRAY_BUFFER, m_nVertexID[0]);
		::glBufferData(GL_ARRAY_BUFFER,
				nVBOsize, m_pfXYZ,
				GL_STATIC_DRAW);
//				GL_DYNAMIC_DRAW);
		::glBindBuffer(GL_ARRAY_BUFFER, m_nVertexID[1]);
		::glBufferData(GL_ARRAY_BUFFER,
				nVBOsize, m_pfNOR,
				GL_STATIC_DRAW);
//				GL_DYNAMIC_DRAW);
		m_nVBOsize = nVBOsize;
	}
	if ( (errCode=::glGetError()) != GL_NO_ERROR ) {	// GL_OUT_OF_MEMORY
		::glBindBuffer(GL_ARRAY_BUFFER, 0);
		ClearVBO();
		delete[]	pThread;
		delete[]	pParam;
		OutputGLErrorMessage(errCode, __LINE__);
		return FALSE;
	}
	::glBindBuffer(GL_ARRAY_BUFFER, 0);

	// 頂点ｲﾝﾃﾞｯｸｽをGPUﾒﾓﾘに転送
	try {
		size_t	jj = 0,
				nCutSize = 0, nWrkSize = 0,
				nElement;
		for ( i=0; i<proc; i++ ) {
			nCutSize += pParam[i].vvElementCut.size();
			nWrkSize += pParam[i].vvElementWrk.size();
		}

		m_pSolidElement = new GLuint[nWrkSize+nCutSize];
		::glGenBuffers((GLsizei)(nWrkSize+nCutSize), m_pSolidElement);
		if ( (errCode=::glGetError()) != GL_NO_ERROR ) {
			::glBindBuffer(GL_ARRAY_BUFFER, 0);
			ClearVBO();
			delete[]	pThread;
			delete[]	pParam;
			OutputGLErrorMessage(errCode, __LINE__);
			return FALSE;
		}

		m_vElementWrk.reserve(nWrkSize+1);
		m_vElementCut.reserve(nCutSize+1);

#ifdef _DEBUG
		int		dbgTriangleWrk = 0, dbgTriangleCut = 0;
#endif
		// ﾜｰｸ矩形用
		for ( i=0; i<proc; i++ ) {
			for ( const auto& v : pParam[i].vvElementWrk ) {
				nElement = v.size();
				::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_pSolidElement[jj++]);
				if ( (errCode=::glGetError()) == GL_NO_ERROR ) {
					::glBufferData(GL_ELEMENT_ARRAY_BUFFER,
						nElement*sizeof(GLuint), &(v[0]),
						GL_STATIC_DRAW);
//						GL_DYNAMIC_DRAW);
					errCode = ::glGetError();
					errLine = __LINE__;
				}
				else
					errLine = __LINE__;
				if ( errCode != GL_NO_ERROR ) {
					::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
					ClearVBO();
					delete[]	pThread;
					delete[]	pParam;
					OutputGLErrorMessage(errCode, errLine);
					return FALSE;
				}
				m_vElementWrk.push_back((GLuint)nElement);
#ifdef _DEBUG
				dbgTriangleWrk += nElement;
#endif
			}
		}
		// 切削面用
		for ( i=0; i<proc; i++ ) {
			for ( const auto& v : pParam[i].vvElementCut ) {
				nElement = v.size();
				::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_pSolidElement[jj++]);
				if ( (errCode=::glGetError()) == GL_NO_ERROR ) {
					::glBufferData(GL_ELEMENT_ARRAY_BUFFER,
						nElement*sizeof(GLuint), &(v[0]),
						GL_STATIC_DRAW);
//						GL_DYNAMIC_DRAW);
					errCode = ::glGetError();
					errLine = __LINE__;
				}
				else
					errLine = __LINE__;
				if ( errCode != GL_NO_ERROR ) {
					::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
					ClearVBO();
					delete[]	pThread;
					delete[]	pParam;
					OutputGLErrorMessage(errCode, errLine);
					return FALSE;
				}
				m_vElementCut.push_back((GLuint)nElement);
#ifdef _DEBUG
				dbgTriangleCut += nElement;
#endif
			}
		}

		::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

#ifdef _DEBUG
		dbg.printf("VertexCount=%d size=%d",
			cx*cy*2, cx*cy*NCXYZ*2*sizeof(GLfloat));
		dbg.printf("Work IndexCount=%d Triangle=%d",
			nWrkSize, dbgTriangleWrk/3);
		dbg.printf("Cut  IndexCount=%d Triangle=%d",
			nCutSize, dbgTriangleCut/3);
#endif
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		ClearVBO();
		bResult = FALSE;
	}

	delete[]	pThread;
	delete[]	pParam;

//	if ( GetDocument()->GetTraceMode() == ID_NCVIEW_TRACE_STOP )
//		AfxGetNCVCMainWnd()->GetProgressCtrl()->SetPos(100);	

	return bResult;
}

UINT CreateElementThread(LPVOID pVoid)
{
	LPCREATEELEMENTPARAM pParam = reinterpret_cast<LPCREATEELEMENTPARAM>(pVoid);
#ifdef _DEBUG
	CMagaDbg	dbg(DBG_BLUE);
	dbg.printf("CreateElementThread() ThreadID=%d s=%d e=%d Start!",
		pParam->dbgThread, pParam->cs, pParam->ce);
	DWORD	t1 = ::timeGetTime(), t2, tt = 0;
#endif

	try {
		// 切削面の頂点ｲﾝﾃﾞｯｸｽ処理
		CreateElementCut(pParam);
#ifdef _DEBUG
		t2 = ::timeGetTime();
		tt += t2 - t1;
		dbg.printf("--- ThreadID=%d CreateElementCut() End %d[ms]",
			pParam->dbgThread, t2 - t1);
		t1 = t2;
#endif
		// ﾜｰｸ上面の頂点ｲﾝﾃﾞｯｸｽ処理
		CreateElementTop(pParam);
#ifdef _DEBUG
		t2 = ::timeGetTime();
		tt += t2 - t1;
		dbg.printf("--- ThreadID=%d CreateElementTop() End %d[ms]",
			pParam->dbgThread, t2 - t1);
		t1 = t2;
#endif
		// ﾜｰｸ底面の頂点ｲﾝﾃﾞｯｸｽ処理
		CreateElementBtm(pParam);
#ifdef _DEBUG
		t2 = ::timeGetTime();
		tt += t2 - t1;
		dbg.printf("--- ThreadID=%d CreateElementBtm() End %d[ms] Total %d[ms]",
			pParam->dbgThread, t2 - t1, tt);
#endif
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		pParam->bResult = FALSE;
	}

	return 0;
}

void CreateElementTop(LPCREATEELEMENTPARAM pParam)
{
	// ﾜｰｸ上面処理
	int			i, j;
	BOOL		bReverse, bSingle;
	GLuint		n0, n1,	nn;
	UINT		z0, z1;
	CVelement	vElement;

	vElement.reserve(pParam->cx * 2 + 1);

	// 切削面以外は m_rcDraw.high を代入しているので
	// 等しいか否かの判断でOK
	for ( j=pParam->cs; j<pParam->ce; j++ ) {
		bReverse = bSingle = FALSE;
		vElement.clear();
		n0 =  j   *pParam->cx;
		n1 = (j+1)*pParam->cx;
		for ( i=0; i<pParam->cx; i++, n0++, n1++ ) {
			z0 = n0*NCXYZ + NCA_Z;
			z1 = n1*NCXYZ + NCA_Z;
			if ( (pParam->pbStl&&pParam->pbStl[n0]!=1) || (!pParam->pbStl&&pParam->pfXYZ[z0]!=pParam->h) ) {
				if ( (pParam->pbStl&&pParam->pbStl[n1]!=1) || (!pParam->pbStl&&pParam->pfXYZ[z1]!=pParam->h) ) {
					// z0:×, z1:×
					if ( vElement.size() > 3 )
						pParam->vvElementWrk.push_back(vElement);
					bReverse = bSingle = FALSE;
					vElement.clear();
				}
				else {
					// z0:×, z1:○
					if ( bSingle ) {
						// 片方だけが連続するならそこでbreak
						if ( vElement.size() > 3 )
							pParam->vvElementWrk.push_back(vElement);
						bReverse = bSingle = FALSE;
						vElement.clear();
					}
					else {
						vElement.push_back(n1);
						bReverse = FALSE;	// 次はn0から登録
						bSingle  = TRUE;
					}
				}
			}
			else if ( (pParam->pbStl&&pParam->pbStl[n1]!=1) || (!pParam->pbStl&&pParam->pfXYZ[z1]!=pParam->h) ) {
				// z0:○, z1:×
				if ( bSingle ) {
					if ( vElement.size() > 3 )
						pParam->vvElementWrk.push_back(vElement);
					bReverse = bSingle = FALSE;
					vElement.clear();
				}
				else {
					vElement.push_back(n0);
					bReverse = TRUE;		// 次はn1から登録
					bSingle  = TRUE;
				}
			}
			else {
				// z0:○, z1:○
				if ( bSingle ) {	// 前回が片方だけ
					// 凹形状防止
					if ( vElement.size() > 3 ) {
						pParam->vvElementWrk.push_back(vElement);
						// 前回の座標を再登録
						nn = vElement.back();
						vElement.clear();
						vElement.push_back(nn);
					}
				}
				bSingle = FALSE;
				if ( bReverse ) {
					vElement.push_back(n1);
					vElement.push_back(n0);
				}
				else {
					vElement.push_back(n0);
					vElement.push_back(n1);
				}
			}
		}
		// Ｘ方向のﾙｰﾌﾟが終了
		if ( vElement.size() > 3 )
			pParam->vvElementWrk.push_back(vElement);
	}
}

void CreateElementBtm(LPCREATEELEMENTPARAM pParam)
{
	// ﾜｰｸ底面処理
	int			i, j, cxcy = pParam->cx * pParam->cy;
	BOOL		bReverse, bSingle;
	GLuint		n0, n1,	b0, b1, nn;
	UINT		z0, z1;
	CVelement	vElement;

	vElement.reserve(pParam->cx * 2 + 1);

	for ( j=pParam->cs; j<pParam->ce; j++ ) {
		bReverse = bSingle = FALSE;
		vElement.clear();
		n0 =  j   *pParam->cx;
		n1 = (j+1)*pParam->cx;
		for ( i=0; i<pParam->cx; i++, n0++, n1++ ) {
			z0 = n0*NCXYZ + NCA_Z;	// 上面のﾃﾞﾌﾟｽ情報で描画判断
			z1 = n1*NCXYZ + NCA_Z;
			b0 = n0 + cxcy;			// 底面座標番号
			b1 = n1 + cxcy;
			if ( (pParam->pbStl&&pParam->pbStl[n0]<1) || (!pParam->pbStl&&pParam->pfXYZ[z0]<pParam->l) ) {
				if ( (pParam->pbStl&&pParam->pbStl[n1]<1) || (!pParam->pbStl&&pParam->pfXYZ[z1]<pParam->l) ) {
					// z0:×, z1:×
					if ( vElement.size() > 3 )
						pParam->vvElementWrk.push_back(vElement);
					bReverse = bSingle = FALSE;
					vElement.clear();
				}
				else {
					// z0:×, z1:○
					if ( bSingle ) {
						// 片方だけが連続するならそこでbreak
						if ( vElement.size() > 3 )
							pParam->vvElementWrk.push_back(vElement);
						bReverse = bSingle = FALSE;
						vElement.clear();
					}
					else {
						vElement.push_back(b1);
						bReverse = FALSE;	// 次はb0から登録
						bSingle  = TRUE;
					}
				}
			}
			else if ( (pParam->pbStl&&pParam->pbStl[n1]<1) || (!pParam->pbStl&&pParam->pfXYZ[z1]<pParam->l) ) {
				// z0:○, z1:×
				if ( bSingle ) {
					if ( vElement.size() > 3 )
						pParam->vvElementWrk.push_back(vElement);
					bReverse = bSingle = FALSE;
					vElement.clear();
				}
				else {
					vElement.push_back(b0);
					bReverse = TRUE;		// 次はb1から登録
					bSingle  = TRUE;
				}
			}
			else {
				// z0:○, z1:○
				if ( bSingle ) {	// 前回が片方だけ
					// 凹形状防止
					if ( vElement.size() > 3 ) {
						pParam->vvElementWrk.push_back(vElement);
						// 前回の座標を再登録
						nn = vElement.back();
						vElement.clear();
						vElement.push_back(nn);
					}
				}
				bSingle = FALSE;
				if ( bReverse ) {
					vElement.push_back(b1);
					vElement.push_back(b0);
				}
				else {
					vElement.push_back(b0);
					vElement.push_back(b1);
				}
			}
		}
		// Ｘ方向のﾙｰﾌﾟが終了
		if ( vElement.size() > 3 )
			pParam->vvElementWrk.push_back(vElement);
	}
}

void CreateElementCut(LPCREATEELEMENTPARAM pParam)
{
	// 切削面（上面・側面兼用）
	int			i, j;
	BOOL		bWorkrct,	// 前回××かどうかを判断するﾌﾗｸﾞ
				bThrough;	// 前回△△　　〃
	GLuint		n0, n1;
	UINT		z0, z1, z0b, z1b;
	GLfloat		z0z, z1z;
	float		q;
	CVelement	vElement;

	vElement.reserve(pParam->cx * 2 + 1);

	// ○：切削面，△：貫通，×：ﾜｰｸ上面
	for ( j=pParam->cs; j<pParam->ce; j++ ) {
		bWorkrct = bThrough = FALSE;
		vElement.clear();
		n0 =  j   *pParam->cx;
		n1 = (j+1)*pParam->cx;
		for ( i=0; i<pParam->cx; i++, n0++, n1++ ) {
			z0  = n0*NCXYZ;
			z1  = n1*NCXYZ;
			z0z = pParam->pfXYZ[z0+NCA_Z];
			z1z = pParam->pfXYZ[z1+NCA_Z];
			if ( (pParam->pbStl&&pParam->pbStl[n0]<=1) || (!pParam->pbStl&&z0z>=pParam->h) ) {
				if ( (pParam->pbStl&&pParam->pbStl[n1]<=1) || (!pParam->pbStl&&z1z>=pParam->h) ) {
					// z0:×, z1:×
					if ( bWorkrct ) {
						// 前回も z0:×, z1:×
						if ( vElement.size() > 3 ) {
							// break
							pParam->vvElementCut.push_back(vElement);
							// 前回の法線を左向き(終点)に変更
							pParam->pfNOR[z0b+NCA_X] = -1.0;
							pParam->pfNOR[z0b+NCA_Z] = 0;
							pParam->pfNOR[z1b+NCA_X] = -1.0;
							pParam->pfNOR[z1b+NCA_Z] = 0;
						}
						vElement.clear();
					}
					bWorkrct = TRUE;
				}
				else {
					// z0:×, z1:△
					// z0:×, z1:○
					if ( bWorkrct ) {
						// 前回 z0:×, z1:× なら
						// 前回の法線を右向き(始点)に変更
						pParam->pfNOR[z0b+NCA_X] = 1.0;
						pParam->pfNOR[z0b+NCA_Z] = 0;
						pParam->pfNOR[z1b+NCA_X] = 1.0;
						pParam->pfNOR[z1b+NCA_Z] = 0;
					}
					// z0をY方向上向きの法線に
					pParam->pfNOR[z0+NCA_Y] = 1.0;
					pParam->pfNOR[z0+NCA_Z] = 0;
					if ( z1z < pParam->l ) {
						// z1（△：切れ目側）をY方向下向きの法線
						pParam->pfNOR[z1+NCA_Y] = -1.0;
						pParam->pfNOR[z1+NCA_Z] = 0;
					}
					bWorkrct = FALSE;
				}
				bThrough = FALSE;
			}
			else if ( (pParam->pbStl&&pParam->pbStl[n0]<1) || (!pParam->pbStl&&z0z<pParam->l) ) {
				// z0:△
				if ( bWorkrct ) {
					// 前回 z0:×, z1:×
					// 前回の法線を右向き(始点)に変更
					pParam->pfNOR[z0b+NCA_X] = 1.0;
					pParam->pfNOR[z0b+NCA_Z] = 0;
					pParam->pfNOR[z1b+NCA_X] = 1.0;
					pParam->pfNOR[z1b+NCA_Z] = 0;
				}
				bWorkrct = FALSE;
				if ( (pParam->pbStl&&pParam->pbStl[n1]<1) || (!pParam->pbStl&&z1z<pParam->l) ) {
					// z0:△, z1:△
					if ( bThrough ) {
						// 前回も z0:△, z1:△
						if ( vElement.size() > 3 ) {
							// break
							pParam->vvElementCut.push_back(vElement);
							// 前回の法線を左向き(切れ目)に変更
							pParam->pfNOR[z0b+NCA_X] = -1.0;
							pParam->pfNOR[z0b+NCA_Z] = 0;
							pParam->pfNOR[z1b+NCA_X] = -1.0;
							pParam->pfNOR[z1b+NCA_Z] = 0;
						}
						vElement.clear();
					}
					bThrough = TRUE;
				}
				else {
					// z0:△, z1:×
					// z0:△, z1:○
					bThrough = FALSE;
					// z0を下向き, z1を上向きの法線
					pParam->pfNOR[z0+NCA_Y] = -1.0;
					pParam->pfNOR[z0+NCA_Z] = 0;
					pParam->pfNOR[z1+NCA_Y] = 1.0;
					pParam->pfNOR[z1+NCA_Z] = 0;
				}
			}
			else {
				// z0:○
				if ( bWorkrct ) {
					// 前回 z0:×, z1:×
					// 前回の法線を右向きに変更
					pParam->pfNOR[z0b+NCA_X] = 1.0;
					pParam->pfNOR[z0b+NCA_Z] = 0;
					pParam->pfNOR[z1b+NCA_X] = 1.0;
					pParam->pfNOR[z1b+NCA_Z] = 0;
				}
				if ( bThrough ) {
					// 前回 z0:△, z1:△
					// 前回の法線を左向きに変更
					pParam->pfNOR[z0b+NCA_X] = -1.0;
					pParam->pfNOR[z0b+NCA_Z] = 0;
					pParam->pfNOR[z1b+NCA_X] = -1.0;
					pParam->pfNOR[z1b+NCA_Z] = 0;
				}
				bWorkrct = bThrough = FALSE;
				if ( (pParam->pbStl&&pParam->pbStl[n1]>=1) || (!pParam->pbStl&&z1z>=pParam->h) ) {
					// z0:○, z1:×
					pParam->pfNOR[z1+NCA_Y] = -1.0;		// 下向きの法線
					pParam->pfNOR[z1+NCA_Z] = 0;
				}
				else if ( (pParam->pbStl&&pParam->pbStl[n1]<1) || (!pParam->pbStl&&z1z<pParam->l) ) {
					// z0:○, z1:△
					pParam->pfNOR[z1+NCA_Y] = 1.0;		// 上向きの法線
					pParam->pfNOR[z1+NCA_Z] = 0;
				}
				else {
					// YZ平面での法線ﾍﾞｸﾄﾙ計算
					q = atan2(z1z - z0z, 
						pParam->pfXYZ[z1+NCA_Y] - pParam->pfXYZ[z0+NCA_Y]);
					pParam->pfNOR[z1+NCA_Y] = -sin(q);	// 式の簡略化
					pParam->pfNOR[z1+NCA_Z] =  cos(q);
				}
				// z0に対する法線計算
				if ( i>0 && pParam->pfXYZ[z0b+NCA_Z] >= pParam->l ) {
					// 前回の座標との角度を求め
					// XZ平面での法線ﾍﾞｸﾄﾙを計算
					q = atan2(z0z - pParam->pfXYZ[z0b+NCA_Z],
						pParam->pfXYZ[z0+NCA_X] - pParam->pfXYZ[z0b+NCA_X]);
					pParam->pfNOR[z0+NCA_X] = -sin(q);
					pParam->pfNOR[z0+NCA_Z] =  cos(q);
				}
			}
			// 全座標をつなぐ
			// ××|△△が連続する所だけclear
			vElement.push_back(n1);
			vElement.push_back(n0);
			// 
			z0b = z0;
			z1b = z1;
		}
		// Ｘ方向のﾙｰﾌﾟが終了
		if ( vElement.size() > 3 )
			pParam->vvElementCut.push_back(vElement);
	}
}

BOOL CreateElementSide(LPCREATEELEMENTPARAM pParam, BOOL bCylinder)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CreateElementSide()\nStart", DBG_BLUE);
	DWORD	t1 = ::timeGetTime();
#endif

	int			i, j;
	UINT		kh, kl;
	GLuint		nh, nl, nn;
	CVelement	vElement;

	try {
		if ( bCylinder ) {
			float	q;
			vElement.reserve(CYCLECOUNT*2);
			nh = CYCLECOUNT * (pParam->cy -1);
			nl = nh + CYCLECOUNT * pParam->cy;
			for ( i=0, q=0; i<CYCLECOUNT; i++, nh++, nl++, q+=CYCLESTEP ) {
				kh = nh*NCXYZ;
				kl = nl*NCXYZ;
				if ( pParam->l >= pParam->pfXYZ[kh+NCA_Z] ) {
					if ( vElement.size() > 3 )
						pParam->vvElementWrk.push_back(vElement);
					vElement.clear();
				}
				else {
					vElement.push_back(nh);
					vElement.push_back(nl);
				}
				// 法線ﾍﾞｸﾄﾙの修正
				pParam->pfNOR[kh+NCA_X] =
				pParam->pfNOR[kl+NCA_X] = cos(q);
				pParam->pfNOR[kh+NCA_Y] =
				pParam->pfNOR[kl+NCA_Y] = sin(q);
				pParam->pfNOR[kh+NCA_Z] =
				pParam->pfNOR[kl+NCA_Z] = 0;
			}
			if ( vElement.size() > 3 )
				pParam->vvElementWrk.push_back(vElement);
		}
		else {
			GLfloat		nor = -1.0;
			vElement.reserve(pParam->cx *2);
			nn = pParam->cy * pParam->cx;	// 底面座標へのｵﾌｾｯﾄ
			// ﾜｰｸ矩形側面（X方向手前と奥）
			for ( j=0; j<pParam->cy; j+=pParam->cy-1 ) {	// 0とm_icy-1
				vElement.clear();
				for ( i=0; i<pParam->cx; i++ ) {
					nh = j*pParam->cx+i;	// 上面座標
					nl = nh + nn;			// 底面座標
					kh = nh*NCXYZ;
					kl = nl*NCXYZ;
					if ( (pParam->pbStl&&pParam->pbStl[nh]<1) || (!pParam->pbStl&&pParam->l>=pParam->pfXYZ[kh+NCA_Z]) ) {
						if ( vElement.size() > 3 )
							pParam->vvElementWrk.push_back(vElement);
						vElement.clear();
					}
					else {
						vElement.push_back(nh);
						vElement.push_back(nl);
					}
					// 法線ﾍﾞｸﾄﾙの修正
					pParam->pfNOR[kh+NCA_Y] = nor;	// 上or下向きの法線
					pParam->pfNOR[kh+NCA_Z] = 0;
					pParam->pfNOR[kl+NCA_Y] = nor;
					pParam->pfNOR[kl+NCA_Z] = 0;
				}
				if ( vElement.size() > 3 )
					pParam->vvElementWrk.push_back(vElement);
				nor *= -1.0;	// 符号反転
			}
			// ﾜｰｸ矩形側面（Y方向左と右）
			for ( i=0; i<pParam->cx; i+=pParam->cx-1 ) {
				vElement.clear();
				for ( j=0; j<pParam->cy; j++ ) {
					nh = j*pParam->cx+i;
					nl = nh + nn;
					kh = nh*NCXYZ;
					kl = nl*NCXYZ;
					if ( (pParam->pbStl&&pParam->pbStl[nh]<1) || (!pParam->pbStl&&pParam->l>=pParam->pfXYZ[kh+NCA_Z]) ) {
						if ( vElement.size() > 3 )
							pParam->vvElementWrk.push_back(vElement);
						vElement.clear();
					}
					else {
						vElement.push_back(nh);
						vElement.push_back(nl);
					}
					pParam->pfNOR[kh+NCA_X] = nor;	// 左or右向きの法線
					pParam->pfNOR[kh+NCA_Z] = 0;
					pParam->pfNOR[kl+NCA_X] = nor;
					pParam->pfNOR[kl+NCA_Z] = 0;
				}
				if ( vElement.size() > 3 )
					pParam->vvElementWrk.push_back(vElement);
				nor *= -1.0;
			}
		}
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		return FALSE;
	}

#ifdef _DEBUG
	DWORD	t2 = ::timeGetTime();
	dbg.printf("end %d[ms]", t2 - t1);
#endif

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
//	ﾌﾗｲｽ加工の円柱ﾓﾃﾞﾘﾝｸﾞ
/////////////////////////////////////////////////////////////////////////////

BOOL CNCViewGL::GetClipDepthCylinder(BOOL bDepthKeep)
{
#ifdef _DEBUG
	CMagaDbg	dbg("GetClipDepthCylinder()\nStart");
#endif
	BOOL		bRecalc;
	size_t		j, n, px, py, nnu, nnd, nnu0, nnd0, nSize;
	int			icx, icy;
	float		q, ox, oy, rx, ry, sx, sy;
	GLint		viewPort[4];
	GLdouble	mvMatrix[16], pjMatrix[16],
				wx1, wy1, wz1, wx2, wy2, wz2;

	::glGetIntegerv(GL_VIEWPORT, viewPort);
	::glGetDoublev (GL_MODELVIEW_MATRIX,  mvMatrix);
	::glGetDoublev (GL_PROJECTION_MATRIX, pjMatrix);

	// 矩形領域をﾋﾟｸｾﾙ座標に変換
	::gluProject(m_rcDraw.left,  m_rcDraw.top,    0.0, mvMatrix, pjMatrix, viewPort,
		&wx1, &wy1, &wz1);
	::gluProject(m_rcDraw.right, m_rcDraw.bottom, 0.0, mvMatrix, pjMatrix, viewPort,
		&wx2, &wy2, &wz2);

	icx = (int)(wx2 - wx1);
	icy = (int)(wy2 - wy1);
	if ( icx<=0 || icy<=0 )
		return FALSE;
	m_wx = (GLint)wx1;
	m_wy = (GLint)wy1;
	// 中心==半径やら進め具合やら
	ox = icx / 2.0f;
	oy = icy / 2.0f;
	if ( icx > icy ) {
		sx = 1.0f;
		sy = (float)icy / icx;
	}
	else {
		sx = (float)icx / icy;
		sy = 1.0f;
	}
#ifdef _DEBUG
	dbg.printf("left,  top   =(%f, %f)", m_rcDraw.left,  m_rcDraw.top);
	dbg.printf("right, bottom=(%f, %f)", m_rcDraw.right, m_rcDraw.bottom);
	dbg.printf("wx1,   wy1   =(%f, %f)", wx1, wy1);
	dbg.printf("wx2,   wy2   =(%f, %f)", wx2, wy2);
	dbg.printf("icx=%d icy=%d", icx, icy);
	DWORD	t1 = ::timeGetTime();
#endif

	// 領域確保
	if ( m_pfDepth &&
			(m_icx!=icx || m_icy!=icy || GetDocument()->GetTraceMode()==ID_NCVIEW_TRACE_STOP) ) {
		delete[]	m_pfDepth;
		delete[]	m_pfXYZ;
		delete[]	m_pfNOR;
		m_pfDepth = m_pfXYZ = m_pfNOR = NULL;
	}
	m_icx = icx;
	m_icy = icy;
	nSize = CYCLECOUNT * (int)(max(ox,oy)+0.5) * NCXYZ;
	if ( !m_pfDepth ) {
		try {
			m_pfDepth = new GLfloat[m_icx * m_icy];
			m_pfXYZ   = new GLfloat[nSize*2];
			m_pfNOR   = new GLfloat[nSize*2];
		}
		catch (CMemoryException* e) {
			AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
			e->Delete();
			if ( m_pfDepth )
				delete[]	m_pfDepth;
			if ( m_pfXYZ )
				delete[]	m_pfXYZ;
			if ( m_pfNOR )
				delete[]	m_pfNOR;
			m_pfDepth = m_pfXYZ = m_pfNOR = NULL;
			return FALSE;
		}
		bRecalc = TRUE;
	}
	else {
		bRecalc = FALSE;
	}
#ifdef _DEBUG
	DWORD	t2 = ::timeGetTime();
	dbg.printf( "Memory alloc=%d[ms]", t2 - t1 );
#endif

	// ｸﾗｲｱﾝﾄ領域のﾃﾞﾌﾟｽ値を取得（ﾋﾟｸｾﾙ単位）
	::glPixelStorei(GL_PACK_ALIGNMENT, 1);
	::glReadPixels(m_wx, m_wy, m_icx, m_icy,
					GL_DEPTH_COMPONENT, GL_FLOAT, m_pfDepth);
//	ASSERT( ::glGetError() == GL_NO_ERROR );
	GLenum glError = ::glGetError();
#ifdef _DEBUG
	DWORD	t3 = ::timeGetTime();
	dbg.printf( "glReadPixels()=%d[ms]", t3 - t2 );
#endif

	// ﾜｰｸ上面底面と切削面の座標登録
	if ( bRecalc ) {
		for ( rx=0, ry=0, nnu=0, nnd=nSize; rx<ox; rx+=sx, ry+=sy ) {
			nnu0 = nnu;
			nnd0 = nnd;
			for ( j=0, q=0; j<CYCLECOUNT-1; j++, q+=CYCLESTEP, nnu+=NCXYZ, nnd+=NCXYZ ) {
				px = (size_t)(rx * cos(q) + ox);	// size_tで受ける
				py = (size_t)(ry * sin(q) + oy);
				n  = min(py*m_icx+px, (size_t)(m_icx*m_icy));
				::gluUnProject((GLdouble)px, (GLdouble)py, m_pfDepth[n],
						mvMatrix, pjMatrix, viewPort,
						&wx2, &wy2, &wz2);
				// ﾜｰﾙﾄﾞ座標
				m_pfXYZ[nnu+NCA_X] = m_pfXYZ[nnd+NCA_X] = (GLfloat)wx2;
				m_pfXYZ[nnu+NCA_Y] = m_pfXYZ[nnd+NCA_Y] = (GLfloat)wy2;
				m_pfXYZ[nnu+NCA_Z] = min((float)wz2, m_rcDraw.high);	// 上面を超えない
				m_pfXYZ[nnd+NCA_Z] = m_rcDraw.low;	// 底面はm_rcDraw.low座標で敷き詰める
				// ﾃﾞﾌｫﾙﾄの法線ﾍﾞｸﾄﾙ
				m_pfNOR[nnu+NCA_X] =  0.0f;
				m_pfNOR[nnu+NCA_Y] =  0.0f;
				m_pfNOR[nnu+NCA_Z] =  1.0f;
				m_pfNOR[nnd+NCA_X] =  0.0f;
				m_pfNOR[nnd+NCA_Y] =  0.0f;
				m_pfNOR[nnd+NCA_Z] = -1.0f;
			}
			// 円を閉じる作業
			for ( j=0; j<NCXYZ; j++ ) {
				m_pfXYZ[nnu+j] = m_pfXYZ[nnu0+j];
				m_pfXYZ[nnd+j] = m_pfXYZ[nnd0+j];
				m_pfNOR[nnu+j] = m_pfNOR[nnu0+j];
				m_pfNOR[nnd+j] = m_pfNOR[nnd0+j];
			}
			nnu += NCXYZ;
			nnd += NCXYZ;
		}
	}
	else {
		for ( rx=0, ry=0, nnu=0, nnd=nSize; rx<ox; rx+=sx, ry+=sy ) {
			nnu0 = nnu;
			nnd0 = nnd;
			for ( j=0, q=0; j<CYCLECOUNT-1; j++, q+=CYCLESTEP, nnu+=NCXYZ, nnd+=NCXYZ ) {
				px = (size_t)(rx * cos(q) + ox);
				py = (size_t)(ry * sin(q) + oy);
				n  = min(py*m_icx+px, (size_t)(m_icx*m_icy));
				::gluUnProject((GLdouble)px, (GLdouble)py, m_pfDepth[n],
						mvMatrix, pjMatrix, viewPort,
						&wx2, &wy2, &wz2);
				m_pfXYZ[nnu+NCA_Z] = min((float)wz2, m_rcDraw.high);
			}
			m_pfXYZ[nnu+NCA_Z] = m_pfXYZ[nnu0+NCA_Z];
			m_pfXYZ[nnd+NCA_Z] = m_pfXYZ[nnd0+NCA_Z];
			m_pfNOR[nnu+NCA_Z] = m_pfNOR[nnu0+NCA_Z];
			m_pfNOR[nnd+NCA_Z] = m_pfNOR[nnd0+NCA_Z];
			nnu += NCXYZ;
			nnd += NCXYZ;
		}
	}
#ifdef _DEBUG
	DWORD	t4 = ::timeGetTime();
	dbg.printf( "AddMatrix=%d[ms]", t4 - t3 );
#endif

	if ( GetDocument()->GetTraceMode() == ID_NCVIEW_TRACE_STOP )
		AfxGetNCVCMainWnd()->GetProgressCtrl()->SetPos(80);	

	// 頂点配列ﾊﾞｯﾌｧｵﾌﾞｼﾞｪｸﾄ生成
	BOOL	bResult = CreateVBOMill();
#ifdef _DEBUG
	DWORD	t5 = ::timeGetTime();
	dbg.printf( "CreateVBOMill total=%d[ms]", t5 - t4 );
#endif

	if ( !bDepthKeep ) {
		delete[]	m_pfDepth;
		delete[]	m_pfXYZ;
		delete[]	m_pfNOR;
		m_pfDepth = m_pfXYZ = m_pfNOR = NULL;
	}

	return bResult;
}

/////////////////////////////////////////////////////////////////////////////
//	旋盤加工回転ﾓﾃﾞﾘﾝｸﾞ
/////////////////////////////////////////////////////////////////////////////

BOOL CNCViewGL::CreateLathe(BOOL bRange)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CreateLathe()\nStart");
#endif
	// ﾎﾞｸｾﾙ生成のための初期設定
	InitialBoxel();
	// 画面いっぱいに描画
	::glMatrixMode(GL_PROJECTION);
	::glPushMatrix();
	::glLoadIdentity();
	::glOrtho(m_rcDraw.left, m_rcDraw.right,
		m_rcView.low, m_rcView.high,	// top と bottom は使用不可
		m_rcView.low, m_rcView.high);	// m_rcDraw ではｷﾞﾘｷﾞﾘなので m_rcView を使う
	::glMatrixMode(GL_MODELVIEW);

#ifdef _DEBUG
	dbg.printf("(%f,%f)-(%f,%f)", m_rcDraw.left, m_rcView.low, m_rcDraw.right, m_rcView.high);
	dbg.printf("(%f,%f)", m_rcView.low, m_rcView.high);
#endif

	size_t	i, s, e;
	if ( bRange ) {
		s = GetDocument()->GetTraceStart();
		e = GetDocument()->GetTraceDraw();
	}
	else {
		s = 0;
		e = GetDocument()->GetNCsize();
	}

	// 旋盤用ZXﾜｲﾔｰの描画
	// --- ﾃﾞｰﾀ量からしてCreateBoxel()みたいにﾏﾙﾁｽﾚｯﾄﾞ化は必要ないでしょう
	::glPushAttrib( GL_LINE_BIT );
	::glLineWidth( 3.0 );	// 1Pixelではﾃﾞﾌﾟｽ値を拾えないかもしれないので
	for ( i=s; i<e; i++ )
		GetDocument()->GetNCdata(i)->DrawGLLatheFace();
	::glPopAttrib();
	::glFinish();

	// ﾃﾞﾌﾟｽ値の取得
	BOOL	bResult = GetClipDepthLathe(bRange);
	if ( !bResult )
		ClearVBO();

	// 終了処理
	m_bSizeChg = FALSE;
	FinalBoxel();

	return bResult;
}

BOOL CNCViewGL::GetClipDepthLathe(BOOL bDepthKeep)
{
#ifdef _DEBUG
	CMagaDbg	dbg("GetClipDepthLathe()");
#endif
	int			i, j, ii, jj, icx;
	float		q;
	GLint		viewPort[4];
	GLdouble	mvMatrix[16], pjMatrix[16],
				wx1, wy1, wz1, wx2, wy2, wz2;
	GLfloat		fz, fx, fxb;
	optional<GLfloat>	fzb;

	::glGetIntegerv(GL_VIEWPORT, viewPort);
	::glGetDoublev (GL_MODELVIEW_MATRIX,  mvMatrix);
	::glGetDoublev (GL_PROJECTION_MATRIX, pjMatrix);

	// 矩形領域(left/right)をﾋﾟｸｾﾙ座標に変換
	::gluProject(m_rcDraw.left,  0.0, 0.0, mvMatrix, pjMatrix, viewPort,
		&wx1, &wy1, &wz1);
	::gluProject(m_rcDraw.right, 0.0, 0.0, mvMatrix, pjMatrix, viewPort,
		&wx2, &wy2, &wz2);

	icx = (int)(wx2 - wx1);
	if ( icx <= 0 )
		return FALSE;
	m_wx = (GLint)wx1;
	m_wy = (GLint)wy1;

#ifdef _DEBUG
	dbg.printf("left  -> wx1 = %f -> %f", m_rcDraw.left, wx1);
	dbg.printf("right -> wx2 = %f -> %f", m_rcDraw.right, wx2);
	dbg.printf("wy1 = %f", wy1);
	dbg.printf("icx=%d", icx);
#endif

	if ( m_pfDepth &&
			(m_icx!=icx || GetDocument()->GetTraceMode()==ID_NCVIEW_TRACE_STOP) ) {
		delete[]	m_pfDepth;
		delete[]	m_pfXYZ;
		delete[]	m_pfNOR;
		m_pfDepth = m_pfXYZ = m_pfNOR = NULL;
	}
	m_icx = icx;
	m_icy = 1;

	// 領域確保
	if ( !m_pfDepth ) {
		try {
			m_pfDepth = new GLfloat[m_icx];
			m_pfXYZ   = new GLfloat[m_icx*(ARCCOUNT+1)*NCXYZ];
			m_pfNOR   = new GLfloat[m_icx*(ARCCOUNT+1)*NCXYZ];
		}
		catch (CMemoryException* e) {
			AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
			e->Delete();
			if ( m_pfDepth )
				delete[]	m_pfDepth;
			if ( m_pfXYZ )
				delete[]	m_pfXYZ;
			if ( m_pfNOR )
				delete[]	m_pfNOR;
			m_pfDepth = m_pfXYZ = m_pfNOR = NULL;
			return FALSE;
		}
	}

	// ｸﾗｲｱﾝﾄ領域のﾃﾞﾌﾟｽ値を取得（ﾋﾟｸｾﾙ単位）
#ifdef _DEBUG
	DWORD	t1 = ::timeGetTime();
#endif
	::glPixelStorei(GL_PACK_ALIGNMENT, 1);
	::glReadPixels(m_wx, m_wy, m_icx, m_icy,
					GL_DEPTH_COMPONENT, GL_FLOAT, m_pfDepth);
//	ASSERT( ::glGetError() == GL_NO_ERROR );
	GLenum glError = ::glGetError();
#ifdef _DEBUG
	DWORD	t2 = ::timeGetTime();
	dbg.printf( "glReadPixels()=%d[ms]", t2 - t1 );
#endif

	// 外径切削面の座標登録
	for ( i=0; i<m_icx; i++ ) {
		::gluUnProject(i+wx1, wy1, m_pfDepth[i],
				mvMatrix, pjMatrix, viewPort,
				&wx2, &wy2, &wz2);	// それほど遅くないので自作変換は中止
		fz = (GLfloat)fabs( m_pfDepth[i]==0.0f ?	// ﾃﾞﾌﾟｽ値が初期値(矩形範囲内で切削面でない)なら
				m_rcDraw.high :				// ﾜｰｸ半径値
				min(wz2, m_rcDraw.high) );	// 変換座標(==半径)かﾜｰｸ半径の小さい方
		ii = i * (ARCCOUNT+1);
		// ﾃｰﾊﾟｰ状の法線ﾍﾞｸﾄﾙを計算
		if ( fzb && fz != *fzb ) {
			q = fz - *fzb;
			fx = cos( atan2(q, (float)wx2-fxb) + RAD(90.0f) );
		}
		else
			fx = 0;
		// fz を半径に円筒形の座標を生成
		for ( j=0, q=0; j<=ARCCOUNT; j++, q+=ARCSTEP ) {
			jj = (ii + j) * NCXYZ;
			// ﾃﾞﾌｫﾙﾄの法線ﾍﾞｸﾄﾙ
			m_pfNOR[jj+NCA_X] = fx;
			m_pfNOR[jj+NCA_Y] = cos(q);
			m_pfNOR[jj+NCA_Z] = sin(q);
			// ﾜｰﾙﾄﾞ座標
			m_pfXYZ[jj+NCA_X] = (GLfloat)wx2;
			m_pfXYZ[jj+NCA_Y] = fz * m_pfNOR[jj+NCA_Y];
			m_pfXYZ[jj+NCA_Z] = fz * m_pfNOR[jj+NCA_Z];
		}
		// 前回値を保存
		fzb = fz;
		fxb = (GLfloat)wx2;
	}
#ifdef _DEBUG
	DWORD	t3 = ::timeGetTime();
	dbg.printf( "AddMatrix1=%d[ms]", t3 - t2 );
#endif

	// 頂点配列ﾊﾞｯﾌｧｵﾌﾞｼﾞｪｸﾄ生成
	BOOL	bResult = CreateVBOLathe();

	if ( !bDepthKeep ) {
		delete[]	m_pfDepth;
		delete[]	m_pfXYZ;
		delete[]	m_pfNOR;
		m_pfDepth = m_pfXYZ = m_pfNOR = NULL;
	}

	return bResult;
}

BOOL CNCViewGL::CreateVBOLathe(void)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CreateVBOLathe()", DBG_BLUE);
#endif
	int			i, j;
	GLsizeiptr	nVBOsize = m_icx * (ARCCOUNT+1) * NCXYZ * sizeof(GLfloat);
	GLuint		n0, n1;
	GLenum		errCode;
	UINT		errLine;
	vector<CVelement>	vvElementWrk,	// 頂点配列ｲﾝﾃﾞｯｸｽ(可変長２次元配列)
						vvElementCut;
	CVelement	vElement;

	// 頂点ｲﾝﾃﾞｯｸｽの消去
	if ( m_pSolidElement ) {
		::glDeleteBuffers((GLsizei)(m_vElementWrk.size()+m_vElementCut.size()), m_pSolidElement);
		delete[]	m_pSolidElement;
		m_pSolidElement = NULL;
	}
	m_vElementWrk.clear();
	m_vElementCut.clear();

	// 準備
	vvElementWrk.reserve(m_icx+1);
	vvElementCut.reserve(m_icx+1);
	vElement.reserve((ARCCOUNT+1)*2+1);

	// iとi+1の座標を円筒形につなげる
	for ( i=0; i<m_icx-1; i++ ) {
		vElement.clear();
		for ( j=0; j<=ARCCOUNT; j++ ) {
			n0 =  i    * (ARCCOUNT+1) + j;
			n1 = (i+1) * (ARCCOUNT+1) + j;
			vElement.push_back(n0);
			vElement.push_back(n1);
		}
		if ( m_pfDepth[i+1] == 0.0f )	// 切削面かﾜｰｸ面か
			vvElementWrk.push_back(vElement);
		else
			vvElementCut.push_back(vElement);
	}

	errCode = ::glGetError();		// ｴﾗｰをﾌﾗｯｼｭ

	// 頂点配列と法線ﾍﾞｸﾄﾙをGPUﾒﾓﾘに転送
	if ( m_nVBOsize==nVBOsize && m_nVertexID[0]>0 ) {
		::glBindBuffer(GL_ARRAY_BUFFER, m_nVertexID[0]);
		::glBufferSubData(GL_ARRAY_BUFFER, 0, nVBOsize, m_pfXYZ);
		::glBindBuffer(GL_ARRAY_BUFFER, m_nVertexID[1]);
		::glBufferSubData(GL_ARRAY_BUFFER, 0, nVBOsize, m_pfNOR);
	}
	else {
		if ( m_nVertexID[0] > 0 )
			::glDeleteBuffers(SIZEOF(m_nVertexID), m_nVertexID);
		::glGenBuffers(SIZEOF(m_nVertexID), m_nVertexID);
		::glBindBuffer(GL_ARRAY_BUFFER, m_nVertexID[0]);
		::glBufferData(GL_ARRAY_BUFFER,
				nVBOsize, m_pfXYZ,
				GL_STATIC_DRAW);
//				GL_DYNAMIC_DRAW);
		::glBindBuffer(GL_ARRAY_BUFFER, m_nVertexID[1]);
		::glBufferData(GL_ARRAY_BUFFER,
				nVBOsize, m_pfNOR,
				GL_STATIC_DRAW);
//				GL_DYNAMIC_DRAW);
		m_nVBOsize = nVBOsize;
	}
	if ( (errCode=::glGetError()) != GL_NO_ERROR ) {	// GL_OUT_OF_MEMORY
		::glBindBuffer(GL_ARRAY_BUFFER, 0);
		ClearVBO();
		OutputGLErrorMessage(errCode, __LINE__);
		return FALSE;
	}
	::glBindBuffer(GL_ARRAY_BUFFER, 0);

	// 頂点ｲﾝﾃﾞｯｸｽをGPUﾒﾓﾘに転送
	try {
		size_t	jj = 0,
				nElement,
				nWrkSize = vvElementWrk.size(),
				nCutSize = vvElementCut.size();

		m_pSolidElement = new GLuint[nWrkSize+nCutSize];
		::glGenBuffers((GLsizei)(nWrkSize+nCutSize), m_pSolidElement);
		if ( (errCode=::glGetError()) != GL_NO_ERROR ) {
			::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			ClearVBO();
			OutputGLErrorMessage(errCode, __LINE__);
			return FALSE;
		}

#ifdef _DEBUG
		int		dbgTriangleWrk = 0, dbgTriangleCut = 0;
#endif
		// ﾜｰｸ矩形用
		m_vElementWrk.reserve(nWrkSize+1);
		for ( const auto& v : vvElementWrk ) {
			nElement = v.size();
			::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_pSolidElement[jj++]);
			if ( (errCode=::glGetError()) == GL_NO_ERROR ) {
				::glBufferData(GL_ELEMENT_ARRAY_BUFFER,
					nElement*sizeof(GLuint), &(v[0]),
					GL_STATIC_DRAW);
//					GL_DYNAMIC_DRAW);
				errCode = ::glGetError();
				errLine = __LINE__;
			}
			else
				errLine = __LINE__;
			if ( errCode != GL_NO_ERROR ) {
				::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
				ClearVBO();
				OutputGLErrorMessage(errCode, errLine);
				return FALSE;
			}
			m_vElementWrk.push_back((GLuint)nElement);
#ifdef _DEBUG
			dbgTriangleWrk += nElement;
#endif
		}
		// 切削面用
		m_vElementCut.reserve(nCutSize+1);
		for ( const auto& v : vvElementCut ) {
			nElement = v.size();
			::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_pSolidElement[jj++]);
			if ( (errCode=::glGetError()) == GL_NO_ERROR ) {
				::glBufferData(GL_ELEMENT_ARRAY_BUFFER,
					nElement*sizeof(GLuint), &(v[0]),
					GL_STATIC_DRAW);
//					GL_DYNAMIC_DRAW);
				errCode = ::glGetError();
				errLine = __LINE__;
			}
			else
				errLine = __LINE__;
			if ( errCode != GL_NO_ERROR ) {
				::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
				ClearVBO();
				OutputGLErrorMessage(errCode, errLine);
				return FALSE;
			}
			m_vElementCut.push_back((GLuint)nElement);
#ifdef _DEBUG
			dbgTriangleCut += nElement;
#endif
		}
		::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

#ifdef _DEBUG
		dbg.printf("VertexCount(/3)=%d size=%d",
			m_icx*(ARCCOUNT+1), m_icx*(ARCCOUNT+1)*NCXYZ*sizeof(GLfloat));
		dbg.printf("Work IndexCount=%d Triangle=%d",
			nWrkSize, dbgTriangleWrk/3);
		dbg.printf("Cut  IndexCount=%d Triangle=%d",
			nCutSize, dbgTriangleCut/3);
#endif
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		return FALSE;
	}

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
//	ﾜｲﾔ加工表示ﾓﾃﾞﾘﾝｸﾞ
/////////////////////////////////////////////////////////////////////////////

BOOL CNCViewGL::CreateWire(void)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CreateWire()\nStart");
#endif
	const CViewOption*	pOpt = AfxGetNCVCApp()->GetViewOption();
	INT_PTR		i, nLoop = GetDocument()->GetTraceDraw();
	BOOL		bStart = TRUE;
	GLenum		errCode;
	UINT		errLine;
	float		dLength;
	CNCdata*	pData;
	CVelement	vef;	// 面生成用頂点ｲﾝﾃﾞｯｸｽ
	WIRELINE	wl;		// 線描画用

	// ｵﾌﾞｼﾞｪｸﾄの長さ情報が必要なため、切削時間計算ｽﾚｯﾄﾞ終了待ち
	GetDocument()->WaitCalcThread(TRUE);

	// 頂点ｲﾝﾃﾞｯｸｽの消去
	if ( m_pSolidElement ) {
		::glDeleteBuffers((GLsizei)(m_vElementWrk.size()+m_vElementCut.size()), m_pSolidElement);
		delete[]	m_pSolidElement;
		m_pSolidElement = NULL;
	}
	if ( m_pLocusElement ) {
		::glDeleteBuffers((GLsizei)(m_WireDraw.vwl.size()), m_pLocusElement);
		delete[]	m_pLocusElement;
		m_pLocusElement = NULL;
	}
	m_vElementWrk.clear();
	m_vElementCut.clear();

	// ﾜｲﾔ加工の座標登録
	for ( i=GetDocument()->GetTraceStart(); i<nLoop; i++ ) {
		// 面形成以外のﾙｰﾌﾟ
		wl.col = pOpt->GetNcDrawColor(NCCOL_G0);
		wl.pattern = g_penStyle[pOpt->GetNcDrawType(NCCOLLINE_G0)].nGLpattern;
		for ( ; i<nLoop; i++ ) {
			pData = GetDocument()->GetNCdata(i);
			if ( pData->IsCutCode() )
				break;
			bStart = pData->AddGLWireVertex(m_WireDraw.vpt, m_WireDraw.vnr, vef, wl, bStart);
		}
		if ( !vef.empty() ) {
			m_WireDraw.vvef.push_back(vef);
			vef.clear();
		}
		if ( !wl.vel.empty() ) {
			m_WireDraw.vwl.push_back(wl);
			wl.vel.clear();
		}
		// 面形成（切削ﾃﾞｰﾀ）
		dLength = 0.0f;
		wl.col = pOpt->GetNcDrawColor(NCCOL_G1);
		wl.pattern = g_penStyle[pOpt->GetNcDrawType(NCCOLLINE_G1)].nGLpattern;
		bStart = pData->AddGLWireVertex(m_WireDraw.vpt, m_WireDraw.vnr, vef, wl, bStart);
		dLength += pData->GetWireObj() ? 
						max(pData->GetCutLength(), pData->GetWireObj()->GetCutLength()) :
						pData->GetCutLength();
		for ( i++; i<nLoop; i++ ) {
			pData = GetDocument()->GetNCdata(i);
			if ( pData->IsCutCode() ) {
				bStart = pData->AddGLWireVertex(m_WireDraw.vpt, m_WireDraw.vnr, vef, wl, bStart);
				dLength += pData->GetWireObj() ? 
								max(pData->GetCutLength(), pData->GetWireObj()->GetCutLength()) :
								pData->GetCutLength();
				if ( pData->IsCircle() && !wl.vel.empty() ) {
					// 円弧ﾃﾞｰﾀの場合は一旦切らないとGL_LINE_STRIPで次につながらない
					m_WireDraw.vwl.push_back(wl);
					wl.vel.clear();
				}
			}
			else
				break;
		}
		if ( !vef.empty() ) {
			m_WireDraw.vvef.push_back(vef);
			vef.clear();
		}
		if ( !wl.vel.empty() ) {
			m_WireDraw.vwl.push_back(wl);
			wl.vel.clear();
		}
		if ( dLength > 0.0f )
			m_WireDraw.vLen.push_back(dLength);
		if ( i < nLoop ) 
			bStart = pData->AddGLWireVertex(m_WireDraw.vpt, m_WireDraw.vnr, vef, wl, bStart);
	}

#ifdef _DEBUG
	dbg.printf("VBO transport Start");
#endif
	if ( m_nVertexID[0] > 0 )
		::glDeleteBuffers(SIZEOF(m_nVertexID), m_nVertexID);
	if ( m_WireDraw.vpt.empty() )
		return TRUE;

	errCode = ::glGetError();	// ｴﾗｰをﾌﾗｯｼｭ
	::glGenBuffers(SIZEOF(m_nVertexID), m_nVertexID);

	// 頂点配列をGPUﾒﾓﾘに転送
	::glBindBuffer(GL_ARRAY_BUFFER, m_nVertexID[0]);
	::glBufferData(GL_ARRAY_BUFFER,
			m_WireDraw.vpt.size()*sizeof(GLfloat), &(m_WireDraw.vpt[0]),
			GL_STATIC_DRAW);
	if ( (errCode=::glGetError()) != GL_NO_ERROR ) {	// GL_OUT_OF_MEMORY
		::glBindBuffer(GL_ARRAY_BUFFER, 0);
		ClearVBO();
		OutputGLErrorMessage(errCode, __LINE__);
		return FALSE;
	}

	// 法線ﾍﾞｸﾄﾙをGPUﾒﾓﾘに転送
	::glBindBuffer(GL_ARRAY_BUFFER, m_nVertexID[1]);
	::glBufferData(GL_ARRAY_BUFFER,
			m_WireDraw.vnr.size()*sizeof(GLfloat), &(m_WireDraw.vnr[0]),
			GL_STATIC_DRAW);
	if ( (errCode=::glGetError()) != GL_NO_ERROR ) {
		::glBindBuffer(GL_ARRAY_BUFFER, 0);
		ClearVBO();
		OutputGLErrorMessage(errCode, __LINE__);
		return FALSE;
	}
	::glBindBuffer(GL_ARRAY_BUFFER, 0);

#ifdef _DEBUG
	dbg.printf("VBO(index) transport Start");
#endif
	// 頂点ｲﾝﾃﾞｯｸｽをGPUﾒﾓﾘに転送
	try {
		size_t	jj = 0;
		GLuint	nElement;
		GLsizei	nSize = (GLsizei)(m_WireDraw.vvef.size());

		m_pSolidElement = new GLuint[nSize];
		::glGenBuffers(nSize, m_pSolidElement);
		m_vElementCut.reserve(nSize+1);
		for ( const auto& v : m_WireDraw.vvef ) {
			nElement = (GLuint)(v.size());
			::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_pSolidElement[jj++]);
			if ( (errCode=::glGetError()) == GL_NO_ERROR ) {
				::glBufferData(GL_ELEMENT_ARRAY_BUFFER,
					nElement*sizeof(GLuint), &(v[0]),
					GL_STATIC_DRAW);
				errCode = ::glGetError();
				errLine = __LINE__;
			}
			else
				errLine = __LINE__;
			if ( errCode != GL_NO_ERROR ) {
				::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
				ClearVBO();
				OutputGLErrorMessage(errCode, errLine);
				return FALSE;
			}
			m_vElementCut.push_back(nElement);
		}

		jj = 0;
		nSize = (GLsizei)(m_WireDraw.vwl.size());
		m_pLocusElement = new GLuint[nSize];
		::glGenBuffers(nSize, m_pLocusElement);
		for ( const auto& v : m_WireDraw.vwl ) {
			::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_pLocusElement[jj++]);
			if ( (errCode=::glGetError()) == GL_NO_ERROR ) {
				::glBufferData(GL_ELEMENT_ARRAY_BUFFER,
					v.vel.size()*sizeof(GLuint), &(v.vel[0]),
					GL_STATIC_DRAW);
				errCode = ::glGetError();
				errLine = __LINE__;
			}
			else
				errLine = __LINE__;
			if ( errCode != GL_NO_ERROR ) {
				::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
				ClearVBO();
				OutputGLErrorMessage(errCode, errLine);
				return FALSE;
			}
		}

		::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		return FALSE;
	}

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////

BOOL CNCViewGL::ReadTexture(LPCTSTR szFileName)
{
	// ﾌｧｲﾙ名のUNICODE変換
	WCHAR	wszFileName[_MAX_PATH];
	::MultiByteToWideChar(CP_ACP, 0, szFileName, -1, wszFileName, _MAX_PATH);

	// 画像の読み込み
	Gdiplus::Bitmap	bmp(wszFileName);
	if ( bmp.GetLastStatus() != Gdiplus::Ok ) {
		ClearTexture();
		AfxMessageBox(IDS_ERR_TEXTURE, MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}
	// 画像のﾋﾟｸｾﾙを取得
	Gdiplus::BitmapData	bmpdata;
	bmp.LockBits(NULL, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &bmpdata);

	// ﾃｸｽﾁｬの生成
	::glGenTextures(1, &m_nPictureID);
	::glBindTexture(GL_TEXTURE_2D, m_nPictureID);
	::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_REPEAT);
	// ﾃｸｽﾁｬにﾋﾟｸｾﾙを書き込む(2^n サイズ以外の対応)
	::gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, bmpdata.Width, bmpdata.Height,
		GL_BGRA_EXT, GL_UNSIGNED_BYTE, bmpdata.Scan0);

	// 後始末
	::glBindTexture(GL_TEXTURE_2D, 0);
	bmp.UnlockBits(&bmpdata);

	return TRUE;
}

void CNCViewGL::CreateTextureMill(void)
{
	if ( m_icx<=0 || m_icy<=0 )
		return;

	int			i, j, n,
				icx, icy;
	GLsizeiptr	nVertex;
	GLfloat		ft;
	GLfloat*	pfTEX;

	if ( GetDocument()->IsDocFlag(NCDOC_CYLINDER) ) {
		icx = CYCLECOUNT;
		icy = (int)(max(m_icx,m_icy)/2.0+0.5);
	}
	else {
		icx = m_icx;
		icy = m_icy;
	}
	nVertex = icx * icy * 2 * 2;	// (X,Y) 上面と底面

	try {
		pfTEX = new GLfloat[nVertex];
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		ClearTexture();
		return;
	}

	// 上面用ﾃｸｽﾁｬ座標
	for ( j=0, n=0; j<icy; j++ ) {
		ft = (GLfloat)(icy-j)/icy;
		for ( i=0; i<icx; i++, n+=2 ) {
			// ﾃｸｽﾁｬ座標(0.0～1.0)
			pfTEX[n]   = (GLfloat)i/icx;
			pfTEX[n+1] = ft;
		}
	}

	// 底面用ﾃｸｽﾁｬ座標
	for ( j=0; j<icy; j++ ) {
		ft = (GLfloat)j/icy;
		for ( i=0; i<icx; i++, n+=2 ) {
			pfTEX[n]   = (GLfloat)i/icx;
			pfTEX[n+1] = ft;
		}
	}

	// ﾃｸｽﾁｬ座標をGPUﾒﾓﾘに転送
	ASSERT( n == nVertex );
	CreateTexture(nVertex, pfTEX);

	delete[]	pfTEX;
}

void CNCViewGL::CreateTextureLathe(void)
{
	if ( m_icx<=0 )
		return;

	int			i, j, n,
				icx = m_icx-1;					// 分母調整
	GLsizeiptr	nVertex = m_icx*(ARCCOUNT+1)*2;	// Xと円筒座標分
	GLfloat		ft;
	GLfloat*	pfTEX;

	try {
		pfTEX = new GLfloat[nVertex];
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		ClearTexture();
		return;
	}

	// ﾃｸｽﾁｬ座標の割り当て
	for ( i=0, n=0; i<m_icx; i++ ) {
		ft = (GLfloat)i/icx;
		for ( j=0; j<=ARCCOUNT; j++, n+=2 ) {
			// ﾃｸｽﾁｬ座標(0.0～1.0)
			pfTEX[n]   = (GLfloat)j/ARCCOUNT;
			pfTEX[n+1] = ft;
		}
	}

	// ﾃｸｽﾁｬ座標をGPUﾒﾓﾘに転送
	ASSERT( n == nVertex );
	CreateTexture(nVertex, pfTEX);

	delete[]	pfTEX;
}

void CNCViewGL::CreateTextureWire(void)
{
	INT_PTR		i, j, n, nLoop = GetDocument()->GetNCsize();
	GLsizeiptr	nVertex;		// 頂点数
	int			nResult;
	float		dAccuLength;	// 累積長さ
	CNCdata*	pData;
	GLfloat*	pfTEX;

	// 頂点数計算
	nVertex = accumulate(m_vElementCut.begin(), m_vElementCut.end(), 0);
	if ( nVertex == 0 )
		return;
	nVertex *= 2;

	try {
		pfTEX = new GLfloat[nVertex];
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		ClearTexture();
		return;
	}

	// ﾃｸｽﾁｬ座標の割り当て(CreateWireと同じﾙｰﾌﾟ)
	for ( i=0, j=0, n=0; i<nLoop; i++, j++ ) {
		// 切削開始点の検索
		for ( ; i<nLoop; i++ ) {
			pData = GetDocument()->GetNCdata(i);
			if ( pData->IsCutCode() ) {
				if ( !pData->GetWireObj() )
					continue;
				// 始点登録
				pfTEX[n++] = 0.0f;	// XY
				pfTEX[n++] = 0.0f;
				pfTEX[n++] = 0.0f;	// UV
				pfTEX[n++] = 1.0f;
				break;
			}
		}
		dAccuLength = 0.0f;
		// 各ｵﾌﾞｼﾞｪｸﾄごとのﾃｸｽﾁｬ座標を登録
		for ( ; i<nLoop; i++ ) {
			pData = GetDocument()->GetNCdata(i);
			nResult = pData->AddGLWireTexture(n, dAccuLength, m_WireDraw.vLen[j], pfTEX);
			if ( nResult < 0 )
				break;	// 次の切削開始点検索
			n += nResult;
		}
	}

	// ﾃｸｽﾁｬ座標をGPUﾒﾓﾘに転送
//	ASSERT( n == nVertex );			// 要調査！！
	CreateTexture(nVertex, pfTEX);

	delete[]	pfTEX;
}

void  CNCViewGL::CreateTexture(GLsizeiptr n, const GLfloat* pfTEX)
{
	GLenum	errCode;

	// ﾃｸｽﾁｬ座標をGPUﾒﾓﾘに転送
	if ( m_nTextureID > 0 )
		::glDeleteBuffers(1, &m_nTextureID);
	::glGenBuffers(1, &m_nTextureID);
	::glBindBuffer(GL_ARRAY_BUFFER, m_nTextureID);
	::glBufferData(GL_ARRAY_BUFFER,
			n*sizeof(GLfloat), pfTEX,
			GL_STATIC_DRAW);
	if ( (errCode=::glGetError()) != GL_NO_ERROR ) {	// GL_OUT_OF_MEMORY
		ClearTexture();
		OutputGLErrorMessage(errCode, __LINE__);
	}
	::glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void CNCViewGL::ClearVBO(void)
{
	if ( m_nVertexID[0] > 0 )
		::glDeleteBuffers(SIZEOF(m_nVertexID), m_nVertexID);
	m_nVBOsize = 0;
	m_nVertexID[0] = m_nVertexID[1] = 0;
	if ( m_pSolidElement ) {
		::glDeleteBuffers((GLsizei)(m_vElementWrk.size()+m_vElementCut.size()), m_pSolidElement);
		delete[]	m_pSolidElement;
		m_pSolidElement = NULL;
	}
	m_vElementWrk.clear();
	m_vElementCut.clear();
}

void CNCViewGL::ClearTexture(void)
{
	if ( m_nPictureID > 0 )
		::glDeleteTextures(1, &m_nPictureID);
	if ( m_nTextureID > 0 )
		::glDeleteBuffers(1, &m_nTextureID);
	m_nPictureID = m_nTextureID = 0;
}

void CNCViewGL::InitialBoxel(void)
{
	// ﾎﾞｸｾﾙ生成のための初期設定
	::glDisable(GL_NORMALIZE);
	::glClearDepth(0.0);		// 遠い方を優先させるためのﾃﾞﾌﾟｽ初期値
	::glDepthFunc(GL_GREATER);	// 遠い方を優先
	::glClear(GL_DEPTH_BUFFER_BIT);		// ﾃﾞﾌﾟｽﾊﾞｯﾌｧのみｸﾘｱ
	::glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);	// ｶﾗｰﾏｽｸOFF
	// 回転行列のﾊﾞｯｸｱｯﾌﾟと初期化
	memcpy(m_objXformBk, m_objXform, sizeof(m_objXform));
	m_ptCenterBk = m_ptCenter;
	ClearObjectForm();
	SetupViewingTransform();
}

void CNCViewGL::FinalBoxel(void)
{
	::glMatrixMode(GL_PROJECTION);
	::glPopMatrix();
	::glMatrixMode(GL_MODELVIEW);

	// 通常設定に戻す
	::glClearDepth(1.0);
	::glDepthFunc(GL_LESS);		// 近い方を優先(通常描画)
	::glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	::glEnable(GL_NORMALIZE);
	// 回転行列を元に戻す
	memcpy(m_objXform, m_objXformBk, sizeof(m_objXform));
	m_ptCenter = m_ptCenterBk;
	SetupViewingTransform();
}

void CNCViewGL::RenderBack(void)
{
	::glDisable(GL_DEPTH_TEST);	// ﾃﾞﾌﾟｽﾃｽﾄ無効で描画

	// 背景ﾎﾟﾘｺﾞﾝの描画
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	COLORREF	col1 = pOpt->GetNcDrawColor(NCCOL_BACKGROUND1),
				col2 = pOpt->GetNcDrawColor(NCCOL_BACKGROUND2);
	GLubyte		col1v[3], col2v[3];
	GLfloat		dVertex[3];
	col1v[0] = GetRValue(col1);
	col1v[1] = GetGValue(col1);
	col1v[2] = GetBValue(col1);
	col2v[0] = GetRValue(col2);
	col2v[1] = GetGValue(col2);
	col2v[2] = GetBValue(col2);

	::glPushMatrix();
	::glLoadIdentity();
	::glBegin(GL_QUADS);
	// 左上
	dVertex[0] = m_rcView.left;
	dVertex[1] = m_rcView.bottom;
//	dVertex[2] = m_rcView.low;
	dVertex[2] = m_rcView.high - NCMIN*2.0f;	// 一番奥(x2はｵﾏｹ)
	::glColor3ubv(col1v);
	::glVertex3fv(dVertex);
	// 左下
	dVertex[1] = m_rcView.top;
	::glColor3ubv(col2v);
	::glVertex3fv(dVertex);
	// 右下
	dVertex[0] = m_rcView.right;
	::glColor3ubv(col2v);
	::glVertex3fv(dVertex);
	// 右上
	dVertex[1] = m_rcView.bottom;
	::glColor3ubv(col1v);
	::glVertex3fv(dVertex);
	//
	::glEnd();
	::glPopMatrix();

	::glEnable(GL_DEPTH_TEST);	// 元に戻す
}

void CNCViewGL::RenderAxis(void)
{
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	float		dLength;
	COLORREF	col;

	::glPushAttrib( GL_LINE_BIT );	// 線情報
	::glLineWidth( 2.0 );
	::glEnable( GL_LINE_STIPPLE );
	::glBegin( GL_LINES );

	// X軸のｶﾞｲﾄﾞ
	dLength = pOpt->GetGuideLength(NCA_X);
	col = pOpt->GetNcDrawColor(NCCOL_GUIDEX);
	::glLineStipple(1, g_penStyle[pOpt->GetNcDrawType(NCA_X)].nGLpattern);
	::glColor3ub( GetRValue(col), GetGValue(col), GetBValue(col) );
	::glVertex3d(-dLength, 0.0, 0.0);
	::glVertex3d( dLength, 0.0, 0.0);
	// Y軸のｶﾞｲﾄﾞ
	dLength = pOpt->GetGuideLength(NCA_Y);
	col = pOpt->GetNcDrawColor(NCCOL_GUIDEY);
	::glLineStipple(1, g_penStyle[pOpt->GetNcDrawType(NCA_Y)].nGLpattern);
	::glColor3ub( GetRValue(col), GetGValue(col), GetBValue(col) );
	::glVertex3d(0.0, -dLength, 0.0);
	::glVertex3d(0.0,  dLength, 0.0);
	// Z軸のｶﾞｲﾄﾞ
	dLength = pOpt->GetGuideLength(NCA_Z);
	col = pOpt->GetNcDrawColor(NCCOL_GUIDEZ);
	::glLineStipple(1, g_penStyle[pOpt->GetNcDrawType(NCA_Z)].nGLpattern);
	::glColor3ub( GetRValue(col), GetGValue(col), GetBValue(col) );
	::glVertex3d(0.0, 0.0, -dLength);
	::glVertex3d(0.0, 0.0,  dLength);

	::glEnd();

	::glDisable( GL_LINE_STIPPLE );
	::glPopAttrib();
}

void CNCViewGL::RenderCode(void)
{
	if ( IsWireMode() )
		return;
	CNCdata*	pData;

	::glEnable( GL_LINE_STIPPLE );
	// NCﾃﾞｰﾀの軌跡（ﾜｲﾔｰﾌﾚｰﾑ）描画
	for ( int i=0; i<GetDocument()->GetNCsize(); i++ ) {
		pData = GetDocument()->GetNCdata(i);
		if ( pData->GetGtype() == G_TYPE ) {
			pData->DrawGLMillWire();
		}
	}
	::glDisable( GL_LINE_STIPPLE );
}

void CNCViewGL::RenderMill(const CNCdata* pData)
{
	extern	const GLuint	GLFanElement[][ARCCOUNT+2];
	extern	const GLuint	GLFanStripElement[];
	CPoint3F	ptOrg(pData->GetEndCorrectPoint());
	BOTTOMDRAW	bd;
	CVBtmDraw	vBD;
	bd.vpt.reserve( ARCCOUNT*ARCCOUNT*NCXYZ );

	// ｴﾝﾄﾞﾐﾙ長さ
	float	h = m_rcDraw.Depth() * 2.0f;

	// ｴﾝﾄﾞﾐﾙ上部
	ptOrg.z += h;
	pData->SetEndmillOrgCircle(ptOrg, bd.vpt);
	// ｴﾝﾄﾞﾐﾙ下部
	ptOrg.z -= h;
	if ( pData->GetEndmillType() == NCMIL_BALL )
		ptOrg.z += pData->GetEndmill();
	if ( pData->GetEndmillType() == NCMIL_CHAMFER )
		pData->SetChamfermillOrg(ptOrg, bd.vpt);
	else
		pData->SetEndmillOrgCircle(ptOrg, bd.vpt);
	// 上部と側面の描画
	::glVertexPointer(NCXYZ, GL_FLOAT, 0, &(bd.vpt[0]));
	::glNormalPointer(GL_FLOAT, 0, &(GLMillUpNor[0]));
	bd.vel.assign(GLFanElement[0], GLFanElement[0]+ARCCOUNT+2);
	::glDrawRangeElements(GL_TRIANGLE_FAN, 0, 64,
			(GLsizei)(bd.vel.size()), GL_UNSIGNED_INT, &(bd.vel[0]));
	::glNormalPointer(GL_FLOAT, 0, &(GLMillSdNor[0]));
	bd.vel.assign(GLFanStripElement, GLFanStripElement+(ARCCOUNT+1)*2);
	::glDrawRangeElements(GL_TRIANGLE_STRIP, 1, 129,
			(GLsizei)(bd.vel.size()), GL_UNSIGNED_INT, &(bd.vel[0]));
	// 下部
	if ( pData->GetEndmillType() == NCMIL_BALL ) {
		pData->AddEndmillSphere(pData->GetEndCorrectPoint(), bd, vBD);
		::glNormalPointer(GL_FLOAT, 0, &(GLMillPhNor[0]));
		for ( const auto& v : vBD ) {
			if ( !v.vpt.empty() )
				::glVertexPointer(NCXYZ, GL_FLOAT, 0, &(v.vpt[0]));
			if ( v.re == 0 )
				::glDrawArrays(v.mode, 0, (GLsizei)(v.vpt.size()/NCXYZ));
			else
				::glDrawRangeElements(v.mode, v.rs, v.re,
					(GLsizei)(v.vel.size()), GL_UNSIGNED_INT, &(v.vel[0]));
		}
	}
	else {
		::glNormalPointer(GL_FLOAT, 0, &(GLMillDwNor[0]));
		bd.vel.assign(GLFanElement[1], GLFanElement[1]+ARCCOUNT+2);
		::glDrawRangeElements(GL_TRIANGLE_FAN, 65, 129,
				(GLsizei)(bd.vel.size()), GL_UNSIGNED_INT, &(bd.vel[0]));
	}
}

CPoint3F CNCViewGL::PtoR(const CPoint& pt)
{
	CPoint3F	ptResult;
	// ﾓﾃﾞﾙ空間の回転
	ptResult.x = ( 2.0f * pt.x - m_cx ) / m_cx * 0.5f;
	ptResult.y = ( m_cy - 2.0f * pt.y ) / m_cy * 0.5f;
	float	 d = _hypotf( ptResult.x, ptResult.y );
	ptResult.z = cos( (PI/2.0f) * min(d, 1.0f) );

	ptResult *= 1.0f / ptResult.hypot();

	return ptResult;
}

void CNCViewGL::BeginTracking(const CPoint& pt, ENTRACKINGMODE enTrackingMode)
{
	::ShowCursor(FALSE);
	SetCapture();
	m_enTrackingMode = enTrackingMode;
	switch( m_enTrackingMode ) {
	case TM_SPIN:
		m_ptLastRound = PtoR(pt);
		break;
	case TM_PAN:
		m_ptLastMove = pt;
		break;
//	default:	// TM_NONE
//		break;
	}
}

void CNCViewGL::EndTracking(void)
{
	ReleaseCapture();
	::ShowCursor(TRUE);
	m_enTrackingMode = TM_NONE;
	Invalidate(FALSE);
}

void CNCViewGL::DoTracking( const CPoint& pt )
{
	CClientDC	dc(this);
	::wglMakeCurrent( dc.GetSafeHdc(), m_hRC );

	switch( m_enTrackingMode ) {
	case TM_SPIN:
	{
		CPoint3F	ptRound( PtoR(pt) );
		CPoint3F	ptw( ptRound - m_ptLastRound );
		m_dRoundStep = 180.0f * ptw.hypot();
		m_ptRoundBase.SetPoint(
			m_ptLastRound.y*ptRound.z - m_ptLastRound.z*ptRound.y,
			m_ptLastRound.z*ptRound.x - m_ptLastRound.x*ptRound.z,
			m_ptLastRound.x*ptRound.y - m_ptLastRound.y*ptRound.x );
		DoRotation(m_dRoundStep);
		Invalidate(FALSE);
		m_ptLastRound = ptRound;
	}
		break;
	case TM_PAN:
		m_ptCenter.x += ( pt.x - m_ptLastMove.x ) / m_dRate;
		m_ptCenter.y -= ( pt.y - m_ptLastMove.y ) / m_dRate;
		m_ptLastMove = pt;
		// ﾓﾃﾞﾘﾝｸﾞ&ﾋﾞｭｰｲﾝｸﾞ変換行列
		SetupViewingTransform();
		Invalidate(FALSE);
		break;
//	deault:		// TM_NONE
//		break;
	}

	::wglMakeCurrent( NULL, NULL );
}

void CNCViewGL::DoScale(int nRate)
{
	if ( nRate != 0 ) {
		m_rcView.InflateRect(
			copysign(m_rcView.Width(),  (float)nRate) * 0.05f,
			copysign(m_rcView.Height(), (float)nRate) * 0.05f );
		float	dW = m_rcView.Width(), dH = m_rcView.Height();
		if ( dW > dH )
			m_dRate = m_cx / dW;
		else
			m_dRate = m_cy / dH;

		CClientDC	dc(this);
		::wglMakeCurrent( dc.GetSafeHdc(), m_hRC );
		::glMatrixMode( GL_PROJECTION );
		::glLoadIdentity();
		::glOrtho(m_rcView.left, m_rcView.right, m_rcView.top, m_rcView.bottom,
			m_rcView.low, m_rcView.high);
		::glMatrixMode( GL_MODELVIEW );
		Invalidate(FALSE);
		::wglMakeCurrent( NULL, NULL );
#ifdef _DEBUG
		g_dbg.printf("DoScale() ---");
		g_dbg.printf("  (%f,%f)-(%f,%f)", m_rcView.left, m_rcView.top, m_rcView.right, m_rcView.bottom);
		g_dbg.printf("  (%f,%f)", m_rcView.low, m_rcView.high);
#endif
	}

	// MDI子ﾌﾚｰﾑのｽﾃｰﾀｽﾊﾞｰに情報表示
	static_cast<CNCChild *>(GetParentFrame())->SetFactorInfo(m_dRate/LOGPIXEL, m_strGuide);
}

void CNCViewGL::DoRotation(float dAngle)
{
#ifdef _DEBUG
//	g_dbg.printf("DoRotation() Angle=%f (%f, %f, %f)", dAngle,
//		m_ptRoundBase.x, m_ptRoundBase.y, m_ptRoundBase.z);
#endif
	// 回転ﾏﾄﾘｯｸｽを現在のｵﾌﾞｼﾞｪｸﾄﾌｫｰﾑﾏﾄﾘｯｸｽに掛け合わせる
	::glLoadIdentity();
	::glRotated( dAngle, m_ptRoundBase.x, m_ptRoundBase.y, m_ptRoundBase.z );
	::glMultMatrixd( (GLdouble *)m_objXform );
	::glGetDoublev( GL_MODELVIEW_MATRIX, (GLdouble *)m_objXform );

	SetupViewingTransform();
}

void CNCViewGL::SetupViewingTransform(void)
{
	::glLoadIdentity();
	::glTranslated( m_ptCenter.x, m_ptCenter.y, 0.0 );
	::glMultMatrixd( (GLdouble *)m_objXform );	// 表示回転（＝モデル回転）
}

/////////////////////////////////////////////////////////////////////////////
// CNCViewGL 描画

void CNCViewGL::OnDraw(CDC* pDC)
{
#ifdef _DEBUG
//	CMagaDbg	dbg("OnDraw()\nStart");
#endif
	ASSERT_VALID(GetDocument());
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();

	// ｶﾚﾝﾄｺﾝﾃｷｽﾄの割り当て
	::wglMakeCurrent( pDC->GetSafeHdc(), m_hRC );

	::glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	// 背景の描画
	RenderBack();

	// 軸の描画
	RenderAxis();

#ifdef _DEBUG_DRAWTEST_
/*
	for ( int i=0; i<GetDocument()->GetNCsize(); i++ )
		GetDocument()->GetNCdata(i)->DrawGLBottomFace();
//		GetDocument()->GetNCdata(1)->DrawGLBottomFace();
	if ( m_nPictureID > 0 ) {
//		COLORREF col = pOpt->GetNcDrawColor(NCCOL_GL_WRK);
		::glEnable(GL_LIGHTING);
		::glEnable (GL_LIGHT0);
		::glEnable (GL_LIGHT1);
		::glDisable(GL_LIGHT2);
		::glDisable(GL_LIGHT3);
		::glEnable(GL_TEXTURE_2D);
		::glBindTexture(GL_TEXTURE_2D, m_nPictureID);
		::glBegin(GL_QUADS);
//		::glColor3ub( GetRValue(col), GetGValue(col), GetBValue(col) );
//		::glColor3ub( 255, 255, 255 );
		::glTexCoord2d(0, 0);
		::glVertex3d(m_rcDraw.left,  m_rcDraw.bottom, m_rcDraw.low);
		::glTexCoord2d(0, 1);
		::glVertex3d(m_rcDraw.left,  m_rcDraw.top,    m_rcDraw.low);
		::glTexCoord2d(1, 1);
		::glVertex3d(m_rcDraw.right, m_rcDraw.top,    m_rcDraw.low);
		::glTexCoord2d(1, 0);
		::glVertex3d(m_rcDraw.right, m_rcDraw.bottom, m_rcDraw.low);
		::glEnd();
		::glBindTexture(GL_TEXTURE_2D, 0);
		::glDisable(GL_TEXTURE_2D);
		::glDisable(GL_LIGHTING);
	}
*/

	BODYList* kbl = GetDocument()->GetKodatunoBodyList();
	if ( kbl ) {
		Describe_BODY	bd;
		for ( int i=0; i<kbl->getNum(); i++ ) {
			bd.DrawBody( (BODY *)kbl->getData(i) );
		}
	}

#else
	if ( pOpt->GetNCViewFlg(NCVIEWFLG_SOLIDVIEW) && m_nVertexID[0]>0 &&
		(pOpt->GetNCViewFlg(NCVIEWFLG_DRAGRENDER) || m_enTrackingMode==TM_NONE) ) {
		size_t	j = 0;
		// 線画が正しく表示されるためにﾎﾟﾘｺﾞﾝｵﾌｾｯﾄ
		::glEnable(GL_POLYGON_OFFSET_FILL);
		::glPolygonOffset(1.0f, 1.0f);
		// 頂点ﾊﾞｯﾌｧｵﾌﾞｼﾞｪｸﾄによるﾎﾞｸｾﾙ描画
		::glEnableClientState(GL_VERTEX_ARRAY);
		::glEnableClientState(GL_NORMAL_ARRAY);
		::glBindBuffer(GL_ARRAY_BUFFER, m_nVertexID[0]);
		::glVertexPointer(NCXYZ, GL_FLOAT, 0, NULL);
		if ( IsWireMode() && pOpt->GetNCViewFlg(NCVIEWFLG_WIREPATH) ) {
			// 軌跡ﾜｲﾔｰﾌﾚｰﾑ表示
			::glEnable( GL_LINE_STIPPLE );
			for ( const auto& v : m_WireDraw.vwl ) {
				::glColor3ub(GetRValue(v.col), GetGValue(v.col), GetBValue(v.col));
				::glLineStipple(1, v.pattern);
				::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_pLocusElement[j++]);
				::glDrawElements(GL_LINE_STRIP, (GLsizei)(v.vel.size()), GL_UNSIGNED_INT, NULL);
			}
			::glDisable( GL_LINE_STIPPLE );
		}
		::glBindBuffer(GL_ARRAY_BUFFER, m_nVertexID[1]);
		::glNormalPointer(GL_FLOAT, 0, NULL);
		if ( pOpt->GetNCViewFlg(NCVIEWFLG_TEXTURE) && m_nTextureID > 0 ) {
			::glBindTexture(GL_TEXTURE_2D, m_nPictureID);
			::glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			::glBindBuffer(GL_ARRAY_BUFFER, m_nTextureID);
			::glTexCoordPointer(2, GL_FLOAT, 0, NULL);
			::glEnable(GL_TEXTURE_2D);
		}
		::glEnable(GL_LIGHTING);
		// ﾜｰｸ矩形
		::glEnable (GL_LIGHT0);
		::glEnable (GL_LIGHT1);
		::glDisable(GL_LIGHT2);
		::glDisable(GL_LIGHT3);
		::glDisable(GL_LIGHT4);
		::glDisable(GL_LIGHT5);
		j = 0;
		for ( const auto v : m_vElementWrk ) {
			::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_pSolidElement[j++]);
#ifdef _DEBUG_POLYGONLINE_
			::glDrawElements(GL_LINE_STRIP,     v, GL_UNSIGNED_INT, NULL);
#else
			::glDrawElements(GL_TRIANGLE_STRIP, v, GL_UNSIGNED_INT, NULL);
#endif
		}
		// 切削面
		::glDisable(GL_LIGHT0);
		::glDisable(GL_LIGHT1);
		::glEnable (GL_LIGHT2);
		::glEnable (GL_LIGHT3);
		for ( const auto v : m_vElementCut ) {
			::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_pSolidElement[j++]);
#ifdef _DEBUG_POLYGONLINE_
			::glDrawElements(GL_LINE_STRIP,     v, GL_UNSIGNED_INT, NULL);
#else
			::glDrawElements(GL_TRIANGLE_STRIP, v, GL_UNSIGNED_INT, NULL);
#endif
		}
		::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		::glBindBuffer(GL_ARRAY_BUFFER, 0);
		if ( pOpt->GetNCViewFlg(NCVIEWFLG_TEXTURE) && m_nTextureID > 0 ) {
			::glBindTexture(GL_TEXTURE_2D, 0);
			::glDisable(GL_TEXTURE_2D);
			::glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		}
		if ( GetDocument()->IsDocMill() && GetDocument()->GetTraceMode()!=ID_NCVIEW_TRACE_STOP ) {
			// ｴﾝﾄﾞﾐﾙ描画
			size_t	nDraw = GetDocument()->GetTraceDraw();
			if ( nDraw > 0 ) {
				::glDisable(GL_LIGHT2);
				::glDisable(GL_LIGHT3);
				::glEnable (GL_LIGHT4);
				::glEnable (GL_LIGHT5);
				RenderMill(GetDocument()->GetNCdata(nDraw-1));
			}
		}
		::glDisableClientState(GL_NORMAL_ARRAY);
		::glDisableClientState(GL_VERTEX_ARRAY);
		::glDisable(GL_POLYGON_OFFSET_FILL);
		::glDisable(GL_LIGHTING);
	}
#endif	// _DEBUG_DRAWTEST_

	if ( GetDocument()->GetTraceMode() == ID_NCVIEW_TRACE_STOP ) {
		if ( !pOpt->GetNCViewFlg(NCVIEWFLG_SOLIDVIEW) ||
				(pOpt->GetNCViewFlg(NCVIEWFLG_SOLIDVIEW) && pOpt->GetNCViewFlg(NCVIEWFLG_WIREPATH)) ||
				(!pOpt->GetNCViewFlg(NCVIEWFLG_DRAGRENDER) && m_enTrackingMode!=TM_NONE) ) {
			// 線画
			if ( m_glCode > 0 )
				::glCallList( m_glCode );
			else
				RenderCode();
		}
	}

//	::glFinish();		// SwapBuffers() に含まれる
	::SwapBuffers( pDC->GetSafeHdc() );

	::wglMakeCurrent(NULL, NULL);
}

/////////////////////////////////////////////////////////////////////////////
// CNCViewGL 診断

#ifdef _DEBUG
void CNCViewGL::AssertValid() const
{
	__super::AssertValid();
}

void CNCViewGL::Dump(CDumpContext& dc) const
{
	__super::Dump(dc);
}

CNCDoc* CNCViewGL::GetDocument() // 非デバッグ バージョンはインラインです。
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CNCDoc)));
	return static_cast<CNCDoc *>(m_pDocument);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CNCViewGL メッセージ ハンドラ

int CNCViewGL::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
#ifdef _DEBUG_FILEOPEN
	g_dbg.printf("CNCViewGL::OnCreate() Start");
#endif
	if ( __super::OnCreate(lpCreateStruct) < 0 )
		return -1;

	// ｽﾌﾟﾘｯﾀからの境界ｶｰｿﾙが移るので，IDC_ARROW を明示的に指定
	// 他の NCView.cpp のように PreCreateWindow() での AfxRegisterWndClass() ではｴﾗｰ??
	::SetClassLongPtr(m_hWnd, GCLP_HCURSOR,
		(LONG_PTR)AfxGetApp()->LoadStandardCursor(IDC_ARROW));
#ifdef _DEBUG_FILEOPEN
	g_dbg.printf("CNCViewGL::SetClassLongPtr() End");
#endif

	// OpenGL初期化処理
	CClientDC	dc(this);
	HDC	hDC = dc.GetSafeHdc();

	// OpenGL pixel format の設定
    if( !SetupPixelFormat(&dc) ) {
		TRACE0("SetupPixelFormat failed\n");
		return -1;
	}
#ifdef _DEBUG_FILEOPEN
	g_dbg.printf("CNCViewGL::SetupPixelFormat() End");
#endif

	// ﾚﾝﾀﾞﾘﾝｸﾞｺﾝﾃｷｽﾄの作成
	if( !(m_hRC = ::wglCreateContext(hDC)) ) {
		TRACE0("wglCreateContext failed\n");
		return -1;
	}
#ifdef _DEBUG_FILEOPEN
	g_dbg.printf("CNCViewGL::wglCreateContext() End");
#endif

	// ﾚﾝﾀﾞﾘﾝｸﾞｺﾝﾃｷｽﾄをｶﾚﾝﾄのﾃﾞﾊﾞｲｽｺﾝﾃｷｽﾄに設定
	if( !::wglMakeCurrent(hDC, m_hRC) ) {
		TRACE0("wglMakeCurrent failed\n");
		return -1;
	}
#ifdef _DEBUG_FILEOPEN
	g_dbg.printf("CNCViewGL::wglMakeCurrent() End");
	DWORD	t1 = ::timeGetTime();
#endif

	// OpenGL Extention 使用準備
	GLenum glewResult = ::glewInit();
	if ( glewResult != GLEW_OK ) {
		TRACE1("glewInit() failed code=%s\n", ::glewGetErrorString(glewResult));
		return -1;
	}
#ifdef _DEBUG_FILEOPEN
	DWORD	t2 = ::timeGetTime();
	g_dbg.printf("CNCViewGL::glewInit() End %d[ms]", t2 - t1);
#endif

	// OpenGL拡張ｻﾎﾟｰﾄのﾁｪｯｸ
	if ( !GLEW_ARB_vertex_buffer_object ) {
		CViewOption*	pOpt = AfxGetNCVCApp()->GetViewOption();
		CString	strErr;
		strErr.Format(IDS_ERR_OPENGLVER, ::glGetString(GL_VERSION));
		AfxMessageBox(strErr, MB_OK|MB_ICONEXCLAMATION);
		pOpt->m_bSolidView  = FALSE;	// ﾌﾗｸﾞ強制OFF
		pOpt->m_bWirePath   = FALSE;
		pOpt->m_bDragRender = FALSE;
		if ( !pOpt->SaveViewOption() )
			AfxMessageBox(IDS_ERR_REGISTRY, MB_OK|MB_ICONEXCLAMATION);
		return -1;
	}

	::glEnable(GL_DEPTH_TEST);
	::glClearColor( 0, 0, 0, 0 );

#ifdef _DEBUG
	g_dbg.printf("GetDeviceCaps([HORZSIZE|VERTSIZE])=%d, %d",
		dc.GetDeviceCaps(HORZSIZE), dc.GetDeviceCaps(VERTSIZE) );
	g_dbg.printf("GetDeviceCaps([LOGPIXELSX|LOGPIXELSY])=%d, %d",
		dc.GetDeviceCaps(LOGPIXELSX), dc.GetDeviceCaps(LOGPIXELSY) );
	GLint	nGLResult;
	g_dbg.printf("Using OpenGL version:%s", ::glGetString(GL_VERSION));
	g_dbg.printf("Using GLEW   version:%s", ::glewGetString(GLEW_VERSION));
	::glGetIntegerv(GL_STENCIL_BITS, &nGLResult);
	g_dbg.printf(" Stencil bits=%d", nGLResult);
	::glGetIntegerv(GL_DEPTH_BITS, &nGLResult);
	g_dbg.printf(" Depth   bits=%d", nGLResult);
	::glGetIntegerv(GL_MAX_ELEMENTS_VERTICES, &nGLResult);
	g_dbg.printf(" MaxElementVertices=%d", nGLResult);
	::glGetIntegerv(GL_MAX_ELEMENTS_INDICES, &nGLResult);
	g_dbg.printf(" MaxElementIndices =%d", nGLResult);
#endif

	::wglMakeCurrent( NULL, NULL );

	return 0;
}

void CNCViewGL::OnDestroy()
{
	if ( m_dRoundStep != 0.0f )
		KillTimer(IDC_OPENGL_DRAGROUND);

	// 回転行列等を保存
	if ( m_bActive ) {
		CRecentViewInfo* pInfo = GetDocument()->GetRecentViewInfo();
		if ( pInfo ) 
			pInfo->SetViewInfo(m_objXform, m_rcView, m_ptCenter);
	}

	// OpenGL 後処理
	CClientDC	dc(this);
	::wglMakeCurrent( dc.GetSafeHdc(), m_hRC );

	// ﾃﾞｨｽﾌﾟﾚｲﾘｽﾄ消去
	if ( m_glCode > 0 )
		::glDeleteLists(m_glCode, 1);
	// ﾊﾞｯﾌｧｵﾌﾞｼﾞｪｸﾄ関連の削除
	ClearVBO();
	ClearTexture();
	if ( m_pLocusElement ) {
		::glDeleteBuffers((GLsizei)(m_WireDraw.vwl.size()), m_pLocusElement);
		delete[]	m_pLocusElement;
	}

	::wglMakeCurrent(NULL, NULL);
	::wglDeleteContext( m_hRC );
	
	__super::OnDestroy();
}

void CNCViewGL::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);
	if( cx <= 0 || cy <= 0 )
		return;

	m_cx = cx;
	m_cy = cy;
	m_bSizeChg = TRUE;
	// ﾋﾞｭｰﾎﾟｰﾄ設定処理
	CClientDC	dc(this);
	::wglMakeCurrent( dc.GetSafeHdc(), m_hRC );
	::glViewport(0, 0, cx, cy);
	::wglMakeCurrent(NULL, NULL);
}

void CNCViewGL::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView) 
{
	DBGBOOL(g_dbg, "CNCViewGL::OnActivateView()", bActivate);

	if ( bActivate ) {
		DoScale(0);		// MDI子ﾌﾚｰﾑのｽﾃｰﾀｽﾊﾞｰに情報表示(だけ)
	}
	else {
		if ( GetDocument()->GetTraceMode()==ID_NCVIEW_TRACE_RUN &&
			!AfxGetNCVCApp()->GetViewOption()->GetNCViewFlg(NCVIEWFLG_NOACTIVETRACEGL) ) {
			// ﾄﾚｰｽ一時停止
			static_cast<CNCChild *>(GetParentFrame())->GetMainView()->OnUserTracePause();
		}
	}

	__super::OnActivateView(bActivate, pActivateView, pDeactiveView);
}

LRESULT CNCViewGL::OnUserActivatePage(WPARAM, LPARAM lParam)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CNCViewGL::OnUserActivatePage()\nStart");
	dbg.printf("lParam=%d m_bActive=%d", lParam, m_bActive);
#endif
	if ( !m_bActive ) {
		// m_rcView初期化
		OnUserViewFitMsg(1, 0);		// glOrtho() を実行しない
		// 描画初期設定
		CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
		pOpt->m_dwUpdateFlg = VIEWUPDATE_ALL;
		UpdateViewOption();
		// 回転行列等の読み込みと更新
		ClearObjectForm();
		CRecentViewInfo*	pInfo = GetDocument()->GetRecentViewInfo();
		if ( pInfo && !pInfo->GetViewInfo(m_objXform, m_rcView, m_ptCenter) ) {
			pInfo = AfxGetNCVCApp()->GetDefaultViewInfo();
			if ( pInfo ) {
				CRect3F	rcView;		// dummy
				CPointF	ptCenter;
				pInfo->GetViewInfo(m_objXform, rcView, ptCenter);
			}
		}
		//
		CClientDC	dc(this);
		::wglMakeCurrent( dc.GetSafeHdc(), m_hRC );
		::glMatrixMode( GL_PROJECTION );
		::glLoadIdentity();
		::glOrtho(m_rcView.left, m_rcView.right, m_rcView.top, m_rcView.bottom,
			m_rcView.low, m_rcView.high);
		::glMatrixMode( GL_MODELVIEW );
		SetupViewingTransform();
		::wglMakeCurrent( NULL, NULL );
#ifdef _DEBUG
		dbg.printf("(%f,%f)-(%f,%f)", m_rcView.left, m_rcView.top, m_rcView.right, m_rcView.bottom);
		dbg.printf("(%f,%f)", m_rcView.low, m_rcView.high);
#endif
		//
		pOpt->m_dwUpdateFlg = 0;
		m_bActive = TRUE;
	}

	// 他のﾋﾞｭｰとは違って、拡大率を更新する必要は無い
	// ｽﾃｰﾀｽﾊﾞｰへの表示も OnActivateView() で行う
	return 0;
}

LRESULT CNCViewGL::OnUserViewFitMsg(WPARAM wParam, LPARAM lParam)
{
	extern	const	float	g_dDefaultGuideLength;	// 50.0 (ViewOption.cpp)
#ifdef _DEBUG
	CMagaDbg	dbg("CNCViewGL::OnUserViewFitMsg()\nStart");
	dbg.printf("wParam=%d lParam=%d", wParam, lParam);
#endif
	float		dW, dH, dZ, dLength, d;

	if ( lParam ) {		// from CNCViewTab::OnInitialUpdate()
		// m_dRate の更新(m_cx,m_cyが正しい値のときに計算)
		dW = fabs(m_rcView.Width());
		dH = fabs(m_rcView.Height());
		if ( dW > dH )
			m_dRate = m_cx / dW;
		else
			m_dRate = m_cy / dH;
		DoScale(0);		// MDI子ﾌﾚｰﾑのｽﾃｰﾀｽﾊﾞｰに情報表示(だけ)
	}
	else {
		m_rcView  = GetDocument()->GetMaxRect();
		dW = fabs(m_rcView.Width());
		dH = fabs(m_rcView.Height());
		dZ = fabs(m_rcView.Depth());

		// 占有矩形の補正(不正表示の防止)
		const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
		if ( dW <= NCMIN ) {
			dLength = pOpt->GetGuideLength(NCA_X);
			if ( dLength == 0.0f )
				dLength = g_dDefaultGuideLength;
			m_rcView.left  = -dLength;
			m_rcView.right =  dLength;
		}
		if ( dH <= NCMIN ) {
			dLength = pOpt->GetGuideLength(NCA_Y);
			if ( dLength == 0.0f )
				dLength = g_dDefaultGuideLength;
			m_rcView.top    = -dLength;
			m_rcView.bottom =  dLength;
		}
		if ( dZ <= NCMIN ) {
			dLength = pOpt->GetGuideLength(NCA_Z);
			if ( dLength == 0.0f )
				dLength = g_dDefaultGuideLength;
			m_rcView.low  = -dLength;
			m_rcView.high =  dLength;
		}
		// ｵﾌﾞｼﾞｪｸﾄ矩形を10%(上下左右5%ずつ)大きく
		m_rcView.InflateRect(dW*0.05f, dH*0.05f);
//		m_rcView.NormalizeRect();
		dW = fabs(m_rcView.Width());
		dH = fabs(m_rcView.Height());
		dZ = fabs(m_rcView.Depth());

		// ﾃﾞｨｽﾌﾟﾚｲのｱｽﾍﾟｸﾄ比から視野直方体設定
		CPointF	pt(m_rcView.CenterPoint());
		if ( dW > dH ) {
			d = dW * m_cy / m_cx / 2.0f;
			m_rcView.top    = pt.y - d;
			m_rcView.bottom = pt.y + d;
			m_dRate = m_cx / dW;
		}
		else {
			d = dH * m_cx / m_cy / 2.0f;
			m_rcView.left   = pt.x - d;
			m_rcView.right  = pt.x + d;
			m_dRate = m_cy / dH;
		}
		d = max(max(dW, dH), dZ) * 2.0f;
		m_rcView.high =  d;		// 奥
		m_rcView.low  = -d;		// 手前
//		m_rcView.NormalizeRect();
	}

	if ( !wParam ) {	// from OnUserActivatePage()
		CClientDC	dc(this);
		::wglMakeCurrent( dc.GetSafeHdc(), m_hRC );
		::glMatrixMode( GL_PROJECTION );
		::glLoadIdentity();
		// !!! Homeキー処理で GL_INVALID_OPERATION が発生する !!!
		// !!! 当面無視で... !!!
		::glOrtho(m_rcView.left, m_rcView.right, m_rcView.top, m_rcView.bottom,
			m_rcView.low, m_rcView.high);
//		ASSERT( ::glGetError() == GL_NO_ERROR );
		GLenum glError = ::glGetError();
		::glMatrixMode( GL_MODELVIEW );
		::wglMakeCurrent(NULL, NULL);
#ifdef _DEBUG
		dbg.printf("(%f,%f)-(%f,%f)", m_rcView.left, m_rcView.top, m_rcView.right, m_rcView.bottom);
		dbg.printf("(%f,%f)", m_rcView.low, m_rcView.high);
		dbg.printf("Rate=%f", m_dRate);
#endif
	}

	return 0;
}

LRESULT CNCViewGL::OnSelectTrace(WPARAM wParam, LPARAM lParam)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CNCViewGL::OnSelectTrace()\nStart");
#endif
	CViewOption*	pOpt = AfxGetNCVCApp()->GetViewOption();
	CNCdata*		pData;
	int				i, s, e;

	if ( lParam ) {
		s = (int)wParam;
		e = (int)lParam;
		pData = NULL;
	}
	else
		pData = reinterpret_cast<CNCdata*>(wParam);

	// 初回のみ警告ﾒｯｾｰｼﾞ
	if ( !pOpt->GetNCViewFlg(NCVIEWFLG_SOLIDVIEW) ) {
		if ( !pData )
			AfxMessageBox(IDS_ERR_GLTRACE, MB_OK|MB_ICONEXCLAMATION);
		return 0;
	}

	BOOL		bResult = TRUE;
	CClientDC	dc(this);
	::wglMakeCurrent( dc.GetSafeHdc(), m_hRC );

	if ( IsWireMode() ) {
		m_WireDraw.clear();		// 毎回ｸﾘｱ
		CreateWire();
		::wglMakeCurrent(NULL, NULL);
		Invalidate(FALSE);
		UpdateWindow();
		return 0;
	}
	if ( m_bSizeChg ) {			// ｳｨﾝﾄﾞｳｻｲｽﾞ変更は最初からﾎﾞｸｾﾙ構築
		if ( IsLatheMode() )
			CreateLathe(TRUE);
		else
			CreateBoxel(TRUE);
		::wglMakeCurrent(NULL, NULL);
		m_bSizeChg = FALSE;
		Invalidate(FALSE);
		UpdateWindow();
		return 0;
	}

	InitialBoxel();
	::glMatrixMode(GL_PROJECTION);
	::glPushMatrix();
	::glLoadIdentity();
	if ( IsLatheMode() ) {
		::glOrtho(m_rcDraw.left, m_rcDraw.right,
			m_rcView.low, m_rcView.high,
			m_rcView.low, m_rcView.high);
	}
	else {
		::glOrtho(m_rcDraw.left, m_rcDraw.right, m_rcDraw.top, m_rcDraw.bottom,
			m_rcView.low, m_rcView.high);
	}
	::glMatrixMode(GL_MODELVIEW);
#ifdef _DEBUG
	dbg.printf("(%f,%f)-(%f,%f)", m_rcDraw.left, m_rcDraw.top, m_rcDraw.right, m_rcDraw.bottom);
	dbg.printf("(%f,%f) m_rcView", m_rcView.low, m_rcView.high);
	dbg.printf("(%f,%f) m_rcDraw", m_rcDraw.low, m_rcDraw.high);
#endif

	if ( m_pfDepth && (pData || lParam) ) {
		// 保存してあるﾃﾞﾌﾟｽ情報を書き込み
		::glWindowPos2i(m_wx, m_wy);
		::glDrawPixels(m_icx, m_icy, GL_DEPTH_COMPONENT, GL_FLOAT, m_pfDepth);
		// 指定ｵﾌﾞｼﾞｪｸﾄの描画
		if ( IsLatheMode() ) {
			::glPushAttrib( GL_LINE_BIT );
			::glLineWidth( 3.0 );
			if ( lParam ) {
				for ( i=s; i<e; i++ )
					GetDocument()->GetNCdata(i)->DrawGLLatheFace();
			}
			else
				pData->DrawGLLatheFace();
			::glPopAttrib();
		}
		else {
			CVBtmDraw	vBD;
			if ( lParam ) {
				BOOL	bStartDraw = TRUE;
				for ( i=s; i<e; i++ )
					bStartDraw = GetDocument()->GetNCdata(i)->AddGLBottomFaceVertex(vBD, bStartDraw);
			}
			else
				pData->AddGLBottomFaceVertex(vBD, TRUE);
			if ( vBD.empty() ) {
				bResult = FALSE;
			}
			else {
				::glEnableClientState(GL_VERTEX_ARRAY);
				for ( const auto& v : vBD ) {
					if ( !v.vpt.empty() )
						::glVertexPointer(NCXYZ, GL_FLOAT, 0, &(v.vpt[0]));
					if ( v.re == 0 )
						::glDrawArrays(v.mode, 0, (GLsizei)(v.vpt.size()/NCXYZ));
					else
						::glDrawRangeElements(v.mode, v.rs, v.re,
							(GLsizei)(v.vel.size()), GL_UNSIGNED_INT, &(v.vel[0]));
				}
				::glDisableClientState(GL_VERTEX_ARRAY);
			}
		}
		::glFinish();
	}
	else {
		// ﾄﾚｰｽ完走のあと再実行で
		// 前のﾄﾚｰｽ結果が残らないようにするための措置
		m_icx = m_icy = 0;
	}

	if ( bResult ) {
		if ( IsLatheMode() ) {
			bResult = GetClipDepthLathe(TRUE);
		}
		else {
			if ( GetDocument()->IsDocFlag(NCDOC_CYLINDER) )
				bResult = GetClipDepthCylinder(TRUE);
			// ↓トレースモードのときIGES重ねる？？
//			else if ( GetDocument()->IsDocFlag(NCDOC_WORKFILE) )
//				bResult = GetClipDepthMill(TRUE);
			else
				bResult = GetClipDepthMill(TRUE);
		}
		if ( !bResult )
			ClearVBO();
	}
	FinalBoxel();

	::wglMakeCurrent(NULL, NULL);
	Invalidate(FALSE);	// OnDraw()で描画
	UpdateWindow();		// 即再描画

	return 0;
}

void CNCViewGL::OnUpdateEditCopy(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(FALSE);	// OpenGLではcopy不可
}

void CNCViewGL::OnUpdateMoveRoundKey(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(TRUE);
}

void CNCViewGL::OnMoveKey(UINT nID)
{
	// ｸﾗｲｱﾝﾄ座標の1/6を移動範囲とする
	CSize	sz(m_cx/6, m_cy/6);
	CPoint	pt(0, 0);

	switch (nID) {
	case ID_VIEW_UP:
		pt.y += sz.cy;
		break;
	case ID_VIEW_DW:
		pt.y -= sz.cy;
		break;
	case ID_VIEW_LT:
		pt.x += sz.cx;
		break;
	case ID_VIEW_RT:
		pt.x -= sz.cx;
		break;
	}

	m_enTrackingMode = TM_PAN;
	m_ptLastMove = 0;
	DoTracking(pt);
	m_enTrackingMode = TM_NONE;
}

void CNCViewGL::OnRoundKey(UINT nID)
{
	if ( m_dRoundStep == 0.0f )
		SetTimer(IDC_OPENGL_DRAGROUND, 150, NULL);

	switch (nID) {
	case ID_VIEW_RUP:
		m_ptRoundBase.SetPoint(1.0f, 0.0f, 0.0f);
		m_dRoundAngle = m_dRoundStep = -1.0f;
		break;
	case ID_VIEW_RDW:
		m_ptRoundBase.SetPoint(1.0f, 0.0f, 0.0f);
		m_dRoundAngle = m_dRoundStep = 1.0f;
		break;
	case ID_VIEW_RLT:
		m_ptRoundBase.SetPoint(0.0, 1.0, 0.0);
		m_dRoundAngle = m_dRoundStep = -1.0f;
		break;
	case ID_VIEW_RRT:
		m_ptRoundBase.SetPoint(0.0f, 1.0f, 0.0f);
		m_dRoundAngle = m_dRoundStep = 1.0f;
		break;
	}
}

void CNCViewGL::OnLensKey(UINT nID)
{
	switch ( nID ) {
	case ID_VIEW_FIT:
		if ( m_dRoundStep != 0.0f ) {
			KillTimer(IDC_OPENGL_DRAGROUND);
			m_dRoundStep = 0.0f;
		}
		OnUserViewFitMsg(0, 0);
		{
			CClientDC	dc(this);
			::wglMakeCurrent( dc.GetSafeHdc(), m_hRC );
			ClearObjectForm();
			SetupViewingTransform();
			::wglMakeCurrent( NULL, NULL );
		}
		Invalidate(FALSE);
		DoScale(0);	// MDI子ﾌﾚｰﾑのｽﾃｰﾀｽﾊﾞｰに情報表示(だけ)
		break;
	case ID_VIEW_LENSP:
		DoScale(-1);
		break;
	case ID_VIEW_LENSN:
		DoScale(1);
		break;
	}
}

void CNCViewGL::OnDefViewInfo()
{
	AfxGetNCVCApp()->SetDefaultViewInfo(m_objXform);
	AfxMessageBox(IDS_ANA_DEFVIEWINFO, MB_OK|MB_ICONINFORMATION);
}

void CNCViewGL::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	CMenu	menu;
	menu.LoadMenu(IDR_NCPOPUP1);
	CMenu*	pMenu = menu.GetSubMenu(0);
	pMenu->TrackPopupMenu(TPM_LEFTALIGN|TPM_LEFTBUTTON|TPM_RIGHTBUTTON,
		point.x, point.y, AfxGetMainWnd());
}

void CNCViewGL::OnLButtonDown(UINT nFlags, CPoint point)
{
	if ( m_dRoundStep != 0.0f ) {
		KillTimer(IDC_OPENGL_DRAGROUND);
		m_dRoundStep = 0.0f;	// KillTimer()でもﾒｯｾｰｼﾞｷｭｰは消えない
	}
	BeginTracking( point, TM_SPIN );
}

void CNCViewGL::OnLButtonUp(UINT nFlags, CPoint point)
{
	EndTracking();
	if ( m_dRoundStep != 0.0f ) {
		KillTimer(IDC_OPENGL_DRAGROUND);	// KillTimer() 連発してもエエのかな...
		m_dRoundStep = 0.0f;
	}
}

void CNCViewGL::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	if ( pOpt->GetNCViewFlg(NCVIEWFLG_SOLIDVIEW) ) {
		pOpt->m_bWirePath = !pOpt->m_bWirePath;
		Invalidate(FALSE);
	}
}

void CNCViewGL::OnRButtonDown(UINT nFlags, CPoint point)
{
	if ( m_dRoundStep != 0.0f )
		KillTimer(IDC_OPENGL_DRAGROUND);	// 連続回転一時停止

	m_ptDownClick = point;
	BeginTracking( point, TM_PAN );
}

void CNCViewGL::OnRButtonUp(UINT nFlags, CPoint point)
{
	EndTracking();
	if ( m_dRoundStep != 0.0f )
		SetTimer(IDC_OPENGL_DRAGROUND, 150, NULL);	// 連続回転再開

	if ( m_ptDownClick == point )
		__super::OnRButtonUp(nFlags, point);	// ｺﾝﾃｷｽﾄﾒﾆｭｰの表示
}

void CNCViewGL::OnMButtonDown(UINT nFlags, CPoint point)
{
	if ( m_dRoundStep != 0.0f ) {
		KillTimer(IDC_OPENGL_DRAGROUND);
		m_dRoundStep = 0.0f;
	}
	BeginTracking( point, TM_SPIN );
}

void CNCViewGL::OnMButtonUp(UINT nFlags, CPoint point)
{
	EndTracking();
	// ﾀｲﾏｲﾍﾞﾝﾄで連続回転
#ifdef _DEBUG
	g_dbg.printf("OnMButtonUp() Angle=%f (%f, %f, %f)", m_dRoundStep,
		m_ptRoundBase.x, m_ptRoundBase.y, m_ptRoundBase.z);
#endif
	if ( m_dRoundStep != 0.0f ) {
		m_dRoundAngle = m_dRoundStep;
		SetTimer(IDC_OPENGL_DRAGROUND, 150, NULL);
	}
}

void CNCViewGL::OnMouseMove(UINT nFlags, CPoint point)
{
	if ( m_enTrackingMode != TM_NONE )
		DoTracking( point );
}

BOOL CNCViewGL::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	if ( !pOpt->IsMouseWheel() )
		return FALSE;

	// 高精度ﾏｳｽでは微細値の場合があるので、一応閾値の判定を入れる
	if ( zDelta <= -WHEEL_DELTA ) {
		if ( pOpt->GetWheelType() != 0 )
			zDelta = -zDelta;
		DoScale(zDelta);
	}
	else if ( zDelta >= WHEEL_DELTA ) {
		if ( pOpt->GetWheelType() != 0 )
			zDelta = -zDelta;
		DoScale(zDelta);
	}

	return TRUE;
}

void CNCViewGL::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if ( nChar == VK_TAB ) {
		CNCChild*	pFrame = static_cast<CNCChild *>(GetParentFrame());
		if ( ::GetKeyState(VK_SHIFT) < 0 )
			pFrame->GetListView()->SetFocus();
		else
			pFrame->GetInfoView()->SetFocus();
		return;
	}

	__super::OnKeyDown(nChar, nRepCnt, nFlags);
}

BOOL CNCViewGL::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}

void CNCViewGL::OnTimer(UINT_PTR nIDEvent) 
{
	if ( m_dRoundStep != 0.0f ) {
#ifdef _DEBUG
		g_dbg.printf("CNCViewGL::OnTimer()");
#endif
//		m_dRoundAngle += m_dRoundStep / 10.0;
		m_dRoundAngle += copysign(0.05f, m_dRoundStep);
		if ( fabs(m_dRoundAngle) > 360.0f )
			m_dRoundAngle -= copysign(360.0f, m_dRoundStep);

		CClientDC	dc(this);
		::wglMakeCurrent( dc.GetSafeHdc(), m_hRC );
		DoRotation(m_dRoundAngle);
		Invalidate(FALSE);
		::wglMakeCurrent( NULL, NULL );
	}

	__super::OnTimer(nIDEvent);
}

//////////////////////////////////////////////////////////////////////

void InitialMillNormal(void)
{
	extern	float	_TABLECOS[ARCCOUNT],
					_TABLESIN[ARCCOUNT];
	int		i, j, n1, n2;

	// 上面下面の法線ﾍﾞｸﾄﾙ
	for ( i=0, n1=0, n2=0; i<ARCCOUNT+2; i++ ) {
		GLMillUpNor[n1++] =  0.0f;
		GLMillUpNor[n1++] =  0.0f;
		GLMillUpNor[n1++] =  1.0f;	// 上向き
		GLMillDwNor[n2++] =  0.0f;
		GLMillDwNor[n2++] =  0.0f;
		GLMillDwNor[n2++] = -1.0f;	// 下向き
	}
	// 側面の法線ﾍﾞｸﾄﾙ
	for ( i=0, n1=0; i<ARCCOUNT; i++ ) {
		// 上部
		GLMillSdNor[n1++] = _TABLECOS[i];
		GLMillSdNor[n1++] = _TABLESIN[i];
		GLMillSdNor[n1++] = 0.0f;
		// 下部
		GLMillSdNor[n1++] = _TABLECOS[i];
		GLMillSdNor[n1++] = _TABLESIN[i];
		GLMillSdNor[n1++] = 0.0f;
	}
	GLMillSdNor[n1++] = _TABLECOS[0];
	GLMillSdNor[n1++] = _TABLESIN[0];
	GLMillSdNor[n1++] = 0.0f;
	// ﾎﾞｰﾙｴﾝﾄﾞﾐﾙ
	for ( j=0, n1=0; j<ARCCOUNT/4-2; j++ ) {
		n2 = j + ARCCOUNT/2;	// 180度ｽﾀｰﾄの添え字
		for ( i=0; i<ARCCOUNT; i++ ) {
			GLMillPhNor[n1++] = _TABLECOS[i];
			GLMillPhNor[n1++] = _TABLESIN[i];
			GLMillPhNor[n1++] = _TABLESIN[n2];
			GLMillPhNor[n1++] = _TABLECOS[i];
			GLMillPhNor[n1++] = _TABLESIN[i];
			GLMillPhNor[n1++] = _TABLESIN[n2+1];
		}
		GLMillPhNor[n1++] = _TABLECOS[0];
		GLMillPhNor[n1++] = _TABLESIN[0];
		GLMillPhNor[n1++] = _TABLESIN[n2];
		GLMillPhNor[n1++] = _TABLECOS[0];
		GLMillPhNor[n1++] = _TABLESIN[0];
		GLMillPhNor[n1++] = _TABLESIN[n2+1];
	}
	// ﾎﾞｰﾙｴﾝﾄﾞﾐﾙ先端
	n2++;
	GLMillPhNor[n1++] =  0.0f;
	GLMillPhNor[n1++] =  0.0f;
	GLMillPhNor[n1++] = -1.0f;
	for ( i=0; i<ARCCOUNT; i++ ) {
		GLMillPhNor[n1++] = _TABLECOS[i];
		GLMillPhNor[n1++] = _TABLESIN[i];
		GLMillPhNor[n1++] = _TABLESIN[n2];
	}
	GLMillPhNor[n1++] = _TABLECOS[0];
	GLMillPhNor[n1++] = _TABLESIN[0];
	GLMillPhNor[n1++] = _TABLESIN[n2];
}

void OutputGLErrorMessage(GLenum errCode, UINT nline)
{
	CString		strMsg;
	strMsg.Format(IDS_ERR_OUTOFVRAM, ::gluErrorString(errCode), nline);
	AfxMessageBox(strMsg, MB_OK|MB_ICONEXCLAMATION);
}
