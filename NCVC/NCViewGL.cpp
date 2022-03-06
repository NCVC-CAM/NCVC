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
#include "boost/array.hpp"
#include "Kodatuno/Describe_BODY.h"
#undef PI	// Use NCVC (MyTemplate.h)

#ifdef _DEBUG
#define new DEBUG_NEW
//#define	_DEBUG_POLYGONLINE_
//#define	_DEBUG_DRAWTEST_
//#define	_DEBUG_DRAWBODY_
#ifdef USE_SHADER
//#define	_DEBUG_SHADERTEST_
#define		_DEBUG_BASICSHADERTEST_
#endif
#include <mmsystem.h>			// timeGetTime()
#endif

using std::vector;
using namespace boost;
extern	const PENSTYLE	g_penStyle[];	// ViewOption.cpp

#define	IsDocError()	GetDocument()->IsDocFlag(NCDOC_ERROR)
#define	IsWireMode()	GetDocument()->IsDocFlag(NCDOC_WIRE)
#define	IsLatheMode()	GetDocument()->IsDocFlag(NCDOC_LATHE)

//	拡大率表示のための変換単位
//	1ｲﾝﾁあたりのﾋﾟｸｾﾙ数 GetDeviceCaps(LOGPIXELS[X|Y]) = 96dpi
//	1ｲﾝﾁ == 25.4mm
static	const float	LOGPIXEL = 96 / 25.4f;

// ｴﾝﾄﾞﾐﾙ描画用の面法線(初期化は起動時CNCVCApp::CNCVCApp()から)
static	boost::array<GLfloat, (ARCCOUNT+2)*NCXYZ>	GLMillUpNor;	// 上面
static	boost::array<GLfloat, (ARCCOUNT+2)*NCXYZ>	GLMillDwNor;	// 下面
static	boost::array<GLfloat, (ARCCOUNT+1)*2*NCXYZ>	GLMillSdNor;	// 側面
static	boost::array<GLfloat, ((ARCCOUNT+1)*2*(ARCCOUNT/4-2)+(ARCCOUNT+2))*NCXYZ>
													GLMillPhNor;	// ﾎﾞｰﾙｴﾝﾄﾞﾐﾙ

// CNCViewGL
IMPLEMENT_DYNCREATE(CNCViewGL, CViewBaseGL)

BEGIN_MESSAGE_MAP(CNCViewGL, CViewBaseGL)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_CONTEXTMENU()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_KEYDOWN()
	// ﾍﾟｰｼﾞ切替ｲﾍﾞﾝﾄ
	ON_MESSAGE (WM_USERACTIVATEPAGE, &CNCViewGL::OnUserActivatePage)
	// 各ﾋﾞｭｰへのﾌｨｯﾄﾒｯｾｰｼﾞ
	ON_MESSAGE (WM_USERVIEWFITMSG, &CNCViewGL::OnUserViewFitMsg)
	// ﾒﾆｭｰｺﾏﾝﾄﾞ
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_UP,  ID_VIEW_RT,  &CNCViewGL::OnUpdateMoveRoundKey)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_RUP, ID_VIEW_RRT, &CNCViewGL::OnUpdateMoveRoundKey)
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
	printf("CNCViewGL::CNCViewGL() Start\n");
#endif
	CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	m_bActive = m_bSizeChg = FALSE;
	m_bWirePath = pOpt->GetNCViewFlg(NCVIEWFLG_WIREPATH);	// ﾃﾞﾌｫﾙﾄ値を取得
	m_bSlitView = pOpt->GetNCViewFlg(NCVIEWFLG_LATHESLIT);
	m_icx = m_icy = 0;
	m_glCode = 0;

#ifdef NO_TRACE_WORKFILE
	m_pfDepth = m_pfXYZ = m_pfNOR = m_pLatheX = m_pLatheZo = m_pLatheZi = NULL;
#else
	m_pfDepth = m_pfDepthBottom = m_pfXYZ = m_pfNOR = m_pLatheX = m_pLatheZo = m_pLatheZi = NULL;
#endif
	m_pbStencil = NULL;
	m_nVBOsize = 0;
	m_nVertexID[0] = m_nVertexID[1] = m_nPictureID = m_nTextureID = 0;
	m_pSolidElement = m_pLocusElement = NULL;

	m_nCeProc = 0;
	m_pCeHandle = NULL;
	m_pCeParam = NULL;
}

CNCViewGL::~CNCViewGL()
{
	EndOfCreateElementThread();
	DeleteDepthMemory();
	ClearVBO();
}

void CNCViewGL::DeleteDepthMemory(void)
{
	if ( m_pfDepth )
		delete[]	m_pfDepth;
	if ( m_pfXYZ )
		delete[]	m_pfXYZ;
	if ( m_pfNOR )
		delete[]	m_pfNOR;
	if ( m_pLatheX )
		delete[]	m_pLatheX;
	if ( m_pLatheZo )
		delete[]	m_pLatheZo;
	if ( m_pLatheZi )
		delete[]	m_pLatheZi;
	if ( m_pbStencil )
		delete[]	m_pbStencil;
	m_pfDepth = m_pfXYZ = m_pfNOR = m_pLatheX = m_pLatheZo = m_pLatheZi = NULL;
	m_pbStencil = NULL;
#ifndef NO_TRACE_WORKFILE
	if ( m_pfDepthBottom )
		delete[]	m_pfDepthBottom;
	m_pfDepthBottom = NULL;
#endif
}

/////////////////////////////////////////////////////////////////////////////
// CNCViewGL クラスのオーバライド関数

void CNCViewGL::OnInitialUpdate() 
{
	extern	LPCTSTR	g_szNdelimiter;	// "XYZUVWIJKRPLDH" from NCDoc.cpp

#ifdef _DEBUG
	printf("CNCViewGL::OnInitialUpdate() Start\n");
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
			OnSelectTrace(NULL, NULL);
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
		// else -> if (GetTraceMode()==ID_NCVIEW_TRACE_STOP) is through
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
	const CViewOption*	pOpt = AfxGetNCVCApp()->GetViewOption();

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
#ifdef USE_SHADER
				CreateTextureMill();	// 無条件でテクスチャ座標の生成(test)
#else
				if ( bResult && m_nPictureID > 0 )
					CreateTextureMill();
#endif
			}
			m_bSizeChg = FALSE;
		}
	}

	::wglMakeCurrent( NULL, NULL );
	Invalidate(FALSE);
}

BOOL CNCViewGL::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) 
{
#ifdef _DEBUG_CMDMSG
	printf("CNCViewGL::OnCmdMsg()\n");
#endif
	// ここから CDocument::OnCmdMsg() を呼ばないようにする
//	return __super::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
	return CWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

/////////////////////////////////////////////////////////////////////////////
// CNCViewGL クラスのメンバ関数

void CNCViewGL::InitialObjectForm(void)
{
	if ( IsLatheMode() ) {
		m_ptCenter = 0.0f;
		// X軸で90°回転させた初期値
		m_objXform[0][0] = 1.0; m_objXform[0][1] = 0.0; m_objXform[0][2] = 0.0; m_objXform[0][3] = 0.0;
		m_objXform[1][0] = 0.0; m_objXform[1][1] = 0.0; m_objXform[1][2] =-1.0; m_objXform[1][3] = 0.0;
		m_objXform[2][0] = 0.0; m_objXform[2][1] = 1.0; m_objXform[2][2] = 0.0; m_objXform[2][3] = 0.0;
		m_objXform[3][0] = 0.0; m_objXform[3][1] = 0.0; m_objXform[3][2] = 0.0; m_objXform[3][3] = 1.0;
	}
	else {
		// 単位行列に初期化
		IdentityMatrix();	// ViewBaseGL.cpp
	}
}

void CNCViewGL::CreateDisplayList(void)
{
	GetGLError();		// error flash

	m_glCode = ::glGenLists(1);
	if( m_glCode > 0 ) {
		// NCﾃﾞｰﾀ描画のﾃﾞｨｽﾌﾟﾚｲﾘｽﾄ生成
		::glNewList( m_glCode, GL_COMPILE );
			RenderCode();
		::glEndList();
		if ( GetGLError() != GL_NO_ERROR ) {
			::glDeleteLists(m_glCode, 1);
			m_glCode = 0;
		}
	}
}

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

void  CNCViewGL::CreateTexture(GLsizeiptr n, const GLfloat* pfTEX)
{
	GLenum	errCode;
	UINT	errLine;

	// ﾃｸｽﾁｬ座標をGPUﾒﾓﾘに転送
	if ( m_nTextureID > 0 )
		::glDeleteBuffers(1, &m_nTextureID);
	::glGenBuffers(1, &m_nTextureID);
	::glBindBuffer(GL_ARRAY_BUFFER, m_nTextureID);
	::glBufferData(GL_ARRAY_BUFFER, n*sizeof(GLfloat), pfTEX,
			GL_STATIC_DRAW);
	errLine = __LINE__;
	if ( (errCode=GetGLError()) != GL_NO_ERROR ) {	// GL_OUT_OF_MEMORY
		ClearTexture();
		OutputGLErrorMessage(errCode, errLine);
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
		::glDeleteBuffers(GetElementSize(), m_pSolidElement);
		delete[]	m_pSolidElement;
		m_pSolidElement = NULL;
	}
	m_vElementWrk.clear();
	m_vElementCut.clear();
	m_vElementEdg.clear();
	m_vElementSlt.clear();
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
	if ( m_pFBO ) {
		m_pFBO->Bind(TRUE);
	}
	// ﾎﾞｸｾﾙ生成のための初期設定
	::glDisable(GL_NORMALIZE);
	::glDepthFunc(GL_GREATER);		// 遠い方を優先
	::glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);	// ｶﾗｰﾏｽｸOFF
	// 回転行列のﾊﾞｯｸｱｯﾌﾟと初期化
	memcpy(m_objXformBk, m_objXform, sizeof(m_objXform));
	m_ptCenterBk = m_ptCenter;
	IdentityMatrix();				// 単位行列に初期化
	SetupViewingTransform();
	// 画面いっぱいに描画
	::glMatrixMode(GL_PROJECTION);
	::glPushMatrix();
	::glLoadIdentity();
}

void CNCViewGL::FinalBoxel(void)
{
	if ( m_pFBO ) {
		m_pFBO->Bind(FALSE);
		::glViewport(0, 0, m_cx, m_cy);
	}
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

void CNCViewGL::RenderAxis(void)
{
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	float		dLength;
	COLORREF	col;

	::glPushAttrib( GL_LINE_BIT );	// 線情報
	::glLineWidth( 2.0f );
	::glEnable( GL_LINE_STIPPLE );
	::glBegin( GL_LINES );

	// X軸のｶﾞｲﾄﾞ
	dLength = pOpt->GetGuideLength(NCA_X);
	col = pOpt->GetNcDrawColor(NCCOL_GUIDEX);
	::glLineStipple(1, g_penStyle[pOpt->GetNcDrawType(NCA_X)].nGLpattern);
	::glColor3ub( GetRValue(col), GetGValue(col), GetBValue(col) );
	::glVertex3f(-dLength, 0.0f, 0.0f);
	::glVertex3f( dLength, 0.0f, 0.0f);
	// Y軸のｶﾞｲﾄﾞ
	dLength = pOpt->GetGuideLength(NCA_Y);
	col = pOpt->GetNcDrawColor(NCCOL_GUIDEY);
	::glLineStipple(1, g_penStyle[pOpt->GetNcDrawType(NCA_Y)].nGLpattern);
	::glColor3ub( GetRValue(col), GetGValue(col), GetBValue(col) );
	::glVertex3f(0.0f, -dLength, 0.0f);
	::glVertex3f(0.0f,  dLength, 0.0f);
	// Z軸のｶﾞｲﾄﾞ
	dLength = pOpt->GetGuideLength(NCA_Z);
	col = pOpt->GetNcDrawColor(NCCOL_GUIDEZ);
	::glLineStipple(1, g_penStyle[pOpt->GetNcDrawType(NCA_Z)].nGLpattern);
	::glColor3ub( GetRValue(col), GetGValue(col), GetBValue(col) );
	::glVertex3f(0.0f, 0.0f, -dLength);
	::glVertex3f(0.0f, 0.0f,  dLength);

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
			pData->DrawGLWirePass();
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
	switch ( pData->GetEndmillType() ) {
	case NCMIL_CHAMFER:
		pData->SetChamfermillOrg(ptOrg, bd.vpt);
		break;
	case NCMIL_DRILL:
		pData->SetDrillOrg(ptOrg, bd.vpt);
		break;
	case NCMIL_BALL:
		ptOrg.z += pData->GetEndmill();
		// through
	default:
		pData->SetEndmillOrgCircle(ptOrg, bd.vpt);
		break;
	}
	// 上部と側面の描画
	::glVertexPointer(NCXYZ, GL_FLOAT, 0, &(bd.vpt[0]));
	::glNormalPointer(GL_FLOAT, 0, &(GLMillUpNor[0]));
	bd.vel.assign(GLFanElement[0], GLFanElement[0]+ARCCOUNT+2);
//	::glDrawElements(GL_TRIANGLE_FAN,
	::glDrawRangeElements(GL_TRIANGLE_FAN, 0, 64,
			(GLsizei)(bd.vel.size()), GL_UNSIGNED_INT, &(bd.vel[0]));
	::glNormalPointer(GL_FLOAT, 0, &(GLMillSdNor[0]));
	bd.vel.assign(GLFanStripElement, GLFanStripElement+(ARCCOUNT+1)*2);
//	::glDrawElements(GL_TRIANGLE_STRIP,
	::glDrawRangeElements(GL_TRIANGLE_STRIP, 1, 129,
			(GLsizei)(bd.vel.size()), GL_UNSIGNED_INT, &(bd.vel[0]));
	// 下部
	if ( pData->GetEndmillType() == NCMIL_BALL ) {
		pData->AddEndmillSphere(pData->GetEndCorrectPoint(), bd, vBD);
		::glNormalPointer(GL_FLOAT, 0, &(GLMillPhNor[0]));
		vBD.Draw();
	}
	else {
		::glNormalPointer(GL_FLOAT, 0, &(GLMillDwNor[0]));
		bd.vel.assign(GLFanElement[1], GLFanElement[1]+ARCCOUNT+2);
//		::glDrawElements(GL_TRIANGLE_FAN,
		::glDrawRangeElements(GL_TRIANGLE_FAN, 65, 129,
				(GLsizei)(bd.vel.size()), GL_UNSIGNED_INT, &(bd.vel[0]));
	}
}

void CNCViewGL::DoScale(int nRate)
{
	CViewBaseGL::DoScale(nRate);
	// MDI子ﾌﾚｰﾑのｽﾃｰﾀｽﾊﾞｰに情報表示
	static_cast<CNCChild *>(GetParentFrame())->SetFactorInfo(m_dRate/LOGPIXEL, m_strGuide,
		m_pFBO ? TRUE : FALSE);
}

/////////////////////////////////////////////////////////////////////////////
// CNCViewGL 描画

void CNCViewGL::OnDraw(CDC* pDC)
{
#ifdef _DEBUG
//	printf("CNCViewGL::OnDraw() Start\n");
#endif
	ASSERT_VALID(GetDocument());
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();

	// ｶﾚﾝﾄｺﾝﾃｷｽﾄの割り当て
	::wglMakeCurrent( pDC->GetSafeHdc(), m_hRC );

	::glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	// 背景の描画
	RenderBackground(pOpt->GetNcDrawColor(NCCOL_BACKGROUND1), pOpt->GetNcDrawColor(NCCOL_BACKGROUND2));

	// 軸の描画
	RenderAxis();

	if ( IsDocError() ) {
		::SwapBuffers( pDC->GetSafeHdc() );
		::wglMakeCurrent(NULL, NULL);
		return;
	}

	::glEnable(GL_DEPTH_TEST);
	::glEnable(GL_LIGHTING);

#if defined(_DEBUG_DRAWTEST_)
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
		::glVertex3f(m_rcDraw.left,  m_rcDraw.bottom, m_rcDraw.low);
		::glTexCoord2d(0, 1);
		::glVertex3f(m_rcDraw.left,  m_rcDraw.top,    m_rcDraw.low);
		::glTexCoord2d(1, 1);
		::glVertex3f(m_rcDraw.right, m_rcDraw.top,    m_rcDraw.low);
		::glTexCoord2d(1, 0);
		::glVertex3f(m_rcDraw.right, m_rcDraw.bottom, m_rcDraw.low);
		::glEnd();
		::glBindTexture(GL_TEXTURE_2D, 0);
		::glDisable(GL_TEXTURE_2D);
		::glDisable(GL_LIGHTING);
	}
#elif defined(_DEBUG_DRAWBODY_)
	BODYList* kbl = GetDocument()->GetKodatunoBodyList();
	if ( kbl ) {
		Describe_BODY	bd;
		for ( int i=0; i<kbl->getNum(); i++ ) {
			bd.DrawBody( (BODY *)kbl->getData(i) );
		}
	}
#elif defined(_DEBUG_SHADERTEST_)
//	::glDisable(GL_DEPTH_TEST);
//	::glPushMatrix();
//	::glLoadIdentity();
//	::glEnable(GL_TEXTURE_2D);
	::glBindTexture(GL_TEXTURE_2D, m_pFBO->GetBufferID(TEXTUREBUFFER));
	m_glsl.Use();
	m_glsl.SetUniform("depth", 0);
	m_glsl.SetUniform("L", m_rcDraw.left);
	m_glsl.SetUniform("R", m_rcDraw.right);
	m_glsl.SetUniform("T", m_rcDraw.bottom);
	m_glsl.SetUniform("B", m_rcDraw.top);
//	::glBegin(GL_QUADS);
//		::glVertex2f(m_rcView.left,  m_rcView.bottom);
//		::glVertex2f(m_rcView.left,  m_rcView.top);
//		::glVertex2f(m_rcView.right, m_rcView.top);
//		::glVertex2f(m_rcView.right, m_rcView.bottom);
//	::glEnd();
	::glBegin(GL_TRIANGLE_STRIP);
		::glVertex2f(m_rcDraw.left,  m_rcDraw.bottom);
		::glVertex2f(m_rcDraw.left,  m_rcDraw.top);
		::glVertex2f(m_rcDraw.right, m_rcDraw.bottom);
		::glVertex2f(m_rcDraw.right, m_rcDraw.top);
	::glEnd();
	m_glsl.Use(FALSE);
	::glBindTexture(GL_TEXTURE_2D, 0);
//	::glDisable(GL_TEXTURE_2D);
//	::glPopMatrix();
//	::glEnable(GL_DEPTH_TEST);
#else
	if ( pOpt->GetNCViewFlg(NCVIEWFLG_SOLIDVIEW) && m_nVertexID[0]>0 &&
		(pOpt->GetNCViewFlg(NCVIEWFLG_DRAGRENDER) || m_enTrackingMode==TM_NONE) ) {
		size_t	j = 0;
		// 線画が正しく表示されるためにﾎﾟﾘｺﾞﾝｵﾌｾｯﾄ
		::glEnable(GL_POLYGON_OFFSET_FILL);
		::glPolygonOffset(1.0f, 1.0f);
		// 頂点ﾊﾞｯﾌｧｵﾌﾞｼﾞｪｸﾄによるﾎﾞｸｾﾙ描画
		::glEnableClientState(GL_VERTEX_ARRAY);
		::glBindBuffer(GL_ARRAY_BUFFER, m_nVertexID[0]);
		::glVertexPointer(NCXYZ, GL_FLOAT, 0, NULL);
		if ( IsWireMode() && m_bWirePath ) {
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
		::glEnableClientState(GL_NORMAL_ARRAY);
		::glBindBuffer(GL_ARRAY_BUFFER, m_nVertexID[1]);
		::glNormalPointer(GL_FLOAT, 0, NULL);
		// ﾃｸｽﾁｬ
		if ( pOpt->GetNCViewFlg(NCVIEWFLG_TEXTURE) && m_nTextureID > 0 ) {
			::glActiveTexture(GL_TEXTURE0);
			::glEnable(GL_TEXTURE_2D);
			::glClientActiveTexture(GL_TEXTURE0);
			::glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			::glBindTexture(GL_TEXTURE_2D, m_nPictureID);
			::glBindBuffer(GL_ARRAY_BUFFER, m_nTextureID);
			::glTexCoordPointer(2, GL_FLOAT, 0, NULL);
		}
		::glEnable(GL_LIGHTING);
#ifdef _DEBUG_BASICSHADERTEST_
		::glActiveTexture(GL_TEXTURE1);
//		::glEnable(GL_TEXTURE_2D);
		::glClientActiveTexture(GL_TEXTURE1);
//		::glEnableClientState(GL_TEXTURE_COORD_ARRAY);
//			GetGLError();	// error flash ここでエラー...
		::glBindTexture(GL_TEXTURE_2D, m_pFBO->GetBufferID(TEXTUREBUFFER));
//			GetGLError();	// bind には問題なし
//		::glBindTexture(GL_TEXTURE_RECTANGLE, m_pFBO->GetBufferID(TEXTUREBUFFER));
//		::glBindBuffer(GL_ARRAY_BUFFER, m_nTextureID);
//		::glTexCoordPointer(2, GL_FLOAT, 0, NULL);
		m_glsl.Use();
		m_glsl.SetUniform("depth", 1);			// GL_TEXTURE1 を適用
		m_glsl.SetUniform("N", m_rcView.low);	// デプス値構築のときのglOrtho
		m_glsl.SetUniform("F", m_rcView.high);
		m_glsl.SetUniform("L", m_rcDraw.left);
		m_glsl.SetUniform("R", m_rcDraw.right);
		m_glsl.SetUniform("T", m_rcDraw.bottom);
		m_glsl.SetUniform("B", m_rcDraw.top);
#endif
		// 切削面
		::glDisable(GL_LIGHT0);
		::glDisable(GL_LIGHT1);
		::glDisable(GL_LIGHT4);
		::glDisable(GL_LIGHT5);
		::glEnable (GL_LIGHT2);
		::glEnable (GL_LIGHT3);
		j = 0;
		for ( const auto& v : m_vElementCut ) {
			::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_pSolidElement[j++]);
#ifdef _DEBUG_POLYGONLINE_
			::glDrawElements(GL_LINE_STRIP,     v, GL_UNSIGNED_INT, NULL);
#else
			::glDrawElements(GL_TRIANGLE_STRIP, v, GL_UNSIGNED_INT, NULL);
#endif
		}
		// ﾜｰｸ矩形
		::glDisable(GL_LIGHT2);
		::glDisable(GL_LIGHT3);
		::glEnable (GL_LIGHT0);
		::glEnable (GL_LIGHT1);
		for ( const auto& v : m_vElementWrk ) {
			::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_pSolidElement[j++]);
#ifdef _DEBUG_POLYGONLINE_
			::glDrawElements(GL_LINE_STRIP,     v, GL_UNSIGNED_INT, NULL);
#else
			::glDrawElements(GL_TRIANGLE_STRIP, v, GL_UNSIGNED_INT, NULL);
#endif
		}
		// 旋盤端面
		for ( const auto& v : m_vElementEdg ) {
			::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_pSolidElement[j++]);
#ifdef _DEBUG_POLYGONLINE_
			::glDrawElements(GL_LINE_STRIP,     v, GL_UNSIGNED_INT, NULL);
#else
			::glDrawElements(GL_TRIANGLE_STRIP, v, GL_UNSIGNED_INT, NULL);
#endif
		}
		// ﾃｸｽﾁｬ解除
		if ( pOpt->GetNCViewFlg(NCVIEWFLG_TEXTURE) && m_nTextureID > 0 ) {
			::glActiveTexture(GL_TEXTURE0);
			::glBindTexture(GL_TEXTURE_2D, 0);
			::glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			::glDisable(GL_TEXTURE_2D);
		}
		// 旋盤断面（ﾜｰｸ矩形色で表示...色が違う？？）
		for ( const auto& v : m_vElementSlt ) {
			::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_pSolidElement[j++]);
#ifdef _DEBUG_POLYGONLINE_
			::glDrawElements(GL_LINE_STRIP,     v, GL_UNSIGNED_INT, NULL);
#else
			::glDrawElements(GL_TRIANGLE_STRIP, v, GL_UNSIGNED_INT, NULL);
#endif
		}
		::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		::glBindBuffer(GL_ARRAY_BUFFER, 0);
		if ( pOpt->GetNCViewFlg(NCVIEWFLG_TOOLTRACE) &&
					GetDocument()->IsDocMill() && GetDocument()->GetTraceMode()!=ID_NCVIEW_TRACE_STOP ) {
			// ｴﾝﾄﾞﾐﾙ描画
			size_t	nDraw = GetDocument()->GetTraceDraw();
			if ( nDraw > 0 ) {
				::glDisable(GL_LIGHT0);
				::glDisable(GL_LIGHT1);
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
#ifdef _DEBUG_BASICSHADERTEST_
	m_glsl.Use(FALSE);
	::glActiveTexture(GL_TEXTURE1);
	::glBindTexture(GL_TEXTURE_2D, 0);
//	::glDisableClientState(GL_TEXTURE_COORD_ARRAY);
//	::glDisable(GL_TEXTURE_2D);
#endif
#endif	// _DEBUG_DRAWTEST_

	if ( GetDocument()->GetTraceMode() == ID_NCVIEW_TRACE_STOP ) {
		if ( !pOpt->GetNCViewFlg(NCVIEWFLG_SOLIDVIEW) || m_bWirePath ||
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
// CNCViewGL メッセージ ハンドラ

int CNCViewGL::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if ( __super::OnCreate(lpCreateStruct) < 0 )
		return -1;

	// OpenGL拡張ｻﾎﾟｰﾄのﾁｪｯｸ
	CString		strVer( ::glGetString(GL_VERSION) );
	if ( !GLEW_ARB_vertex_buffer_object ) {
		CViewOption*	pOpt = AfxGetNCVCApp()->GetViewOption();
		CString	strErrMsg;
		strErrMsg.Format(IDS_ERR_OPENGLVER, strVer);
		AfxMessageBox(strErrMsg, MB_OK|MB_ICONEXCLAMATION);
		pOpt->m_bSolidView  = FALSE;	// ﾌﾗｸﾞ強制OFF
		pOpt->m_bWirePath   = TRUE;
		pOpt->m_bDragRender = FALSE;
		if ( !pOpt->SaveViewOption() )
			AfxMessageBox(IDS_ERR_REGISTRY, MB_OK|MB_ICONEXCLAMATION);
		return -1;
	}

	// ｽﾌﾟﾘｯﾀからの境界ｶｰｿﾙが移るので，IDC_ARROW を明示的に指定
	// 他の NCView.cpp のように PreCreateWindow() での AfxRegisterWndClass() ではｴﾗｰ??
	::SetClassLongPtr(m_hWnd, GCLP_HCURSOR,
		(LONG_PTR)AfxGetApp()->LoadStandardCursor(IDC_ARROW));
#ifdef _DEBUG_FILEOPEN
	printf("CNCViewGL::SetClassLongPtr() End\n");
#endif

#if defined(_DEBUG_SHADERTEST_)
	if ( !m_glsl.CompileShaderFromFile("C:\\Users\\magara\\Documents\\Visual Studio 2015\\Projects\\NCVC\\NCVC\\showdepth.vert", VERTEX) ) {
		strErrMsg.Format(IDS_ERR_GLSL, __LINE__, m_glsl.GetErrorStr());
		AfxMessageBox(strErrMsg, MB_OK|MB_ICONEXCLAMATION);
	}
	else if ( !m_glsl.CompileShaderFromFile("C:\\Users\\magara\\Documents\\Visual Studio 2015\\Projects\\NCVC\\NCVC\\showdepth.frag", FRAGMENT) ) {
		strErrMsg.Format(IDS_ERR_GLSL, __LINE__, m_glsl.GetErrorStr());
		AfxMessageBox(strErrMsg, MB_OK|MB_ICONEXCLAMATION);
	}
	else if ( !m_glsl.Link() ) {
		strErrMsg.Format(IDS_ERR_GLSL, __LINE__, m_glsl.GetErrorStr());
		AfxMessageBox(strErrMsg, MB_OK|MB_ICONEXCLAMATION);
	}
	m_glsl.SetUniform("depth", 0);
#elif defined(_DEBUG_BASICSHADERTEST_)
	if ( !m_glsl.CompileShaderFromFile("C:\\Users\\magara\\Documents\\Visual Studio 2015\\Projects\\NCVC\\NCVC\\basic.vert", VERTEX) ) {
		strErrMsg.Format(IDS_ERR_GLSL, __LINE__, m_glsl.GetErrorStr());
		AfxMessageBox(strErrMsg, MB_OK|MB_ICONEXCLAMATION);
	}
	else if ( !m_glsl.CompileShaderFromFile("C:\\Users\\magara\\Documents\\Visual Studio 2015\\Projects\\NCVC\\NCVC\\basic.frag", FRAGMENT) ) {
		strErrMsg.Format(IDS_ERR_GLSL, __LINE__, m_glsl.GetErrorStr());
		AfxMessageBox(strErrMsg, MB_OK|MB_ICONEXCLAMATION);
	}
	else if ( !m_glsl.Link() ) {
		strErrMsg.Format(IDS_ERR_GLSL, __LINE__, m_glsl.GetErrorStr());
		AfxMessageBox(strErrMsg, MB_OK|MB_ICONEXCLAMATION);
	}
#endif

	return 0;
}

void CNCViewGL::OnDestroy()
{
	// 回転行列等を保存
	if ( m_bActive && !IsDocError() ) {
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

	if ( cx > 0 && cy > 0 )
		m_bSizeChg = TRUE;
}

void CNCViewGL::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView) 
{
#ifdef _DEBUG
	printf("CNCViewGL::OnActivateView()=%d\n", bActivate);
#endif

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
	printf("CNCViewGL::OnUserActivatePage() lParam=%Id m_bActive=%d\n", lParam, m_bActive);
#endif
	if ( !m_bActive ) {
		// m_rcView初期化
		OnUserViewFitMsg(1, 0);		// glOrtho() を実行しない
		CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
		if ( !IsDocError() ) {
			// 描画初期設定
			pOpt->m_dwUpdateFlg = VIEWUPDATE_ALL;
			UpdateViewOption();
			// 回転行列等の読み込みと更新
			InitialObjectForm();
			CRecentViewInfo*	pInfo = GetDocument()->GetRecentViewInfo();
			if ( pInfo && !pInfo->GetViewInfo(m_objXform, m_rcView, m_ptCenter) ) {
				pInfo = AfxGetNCVCApp()->GetDefaultViewInfo();
				if ( pInfo ) {
					CRect3F	rcView;		// dummy
					CPointF	ptCenter;
					pInfo->GetViewInfo(m_objXform, rcView, ptCenter);
				}
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
		printf("(%f,%f)-(%f,%f)\n", m_rcView.left, m_rcView.top, m_rcView.right, m_rcView.bottom);
		printf("(%f,%f)\n", m_rcView.low, m_rcView.high);
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
#ifdef _DEBUG
	printf("CNCViewGL::OnUserViewFitMsg() wParam=%Id lParam=%Id\n", wParam, lParam);
#endif
	if ( lParam ) {		// from CNCViewTab::OnInitialUpdate()
		// m_dRate の更新(m_cx,m_cyが正しい値のときに計算)
		float	dW = fabs(m_rcView.Width()),
				dH = fabs(m_rcView.Height());
		if ( dW > dH )
			m_dRate = m_cx / dW;
		else
			m_dRate = m_cy / dH;
		DoScale(0);		// MDI子ﾌﾚｰﾑのｽﾃｰﾀｽﾊﾞｰに情報表示(だけ)
	}
	else {
		m_rcView  = GetDocument()->GetMaxRect();
		SetOrthoView();	// ViewBaseGL.cpp
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
		GetGLError();
		::glMatrixMode( GL_MODELVIEW );
		::wglMakeCurrent(NULL, NULL);
#ifdef _DEBUG
		printf("(%f,%f)-(%f,%f)\n", m_rcView.left, m_rcView.top, m_rcView.right, m_rcView.bottom);
		printf("(%f,%f)\n", m_rcView.low, m_rcView.high);
		printf("Rate=%f\n", m_dRate);
#endif
	}

	return 0;
}

LRESULT CNCViewGL::OnSelectTrace(WPARAM wParam, LPARAM lParam)
{
#ifdef _DEBUG
	printf("CNCViewGL::OnSelectTrace() Start\n");
#endif
	CNCdata*	pData;
	INT_PTR		i, s, e;
	CREATEBOXEL_IGESPARAM	pParam;

	if ( lParam ) {
		s = (INT_PTR)wParam;
		e = (INT_PTR)lParam;
		pData = NULL;
		pParam = RANGEPARAM(s, e);
	}
	else {
		pData = reinterpret_cast<CNCdata*>(wParam);
		pParam = pData;
	}

	if ( !pData && !AfxGetNCVCApp()->GetViewOption()->GetNCViewFlg(NCVIEWFLG_SOLIDVIEW) ) {
		// 初回のみ警告ﾒｯｾｰｼﾞ
		AfxMessageBox(IDS_ERR_GLTRACE, MB_OK|MB_ICONEXCLAMATION);
		return 0;
	}

	BOOL		bResult = TRUE;
	CClientDC	dc(this);
	::wglMakeCurrent( dc.GetSafeHdc(), m_hRC );

	if ( IsWireMode() ) {
		CreateWire();
		::wglMakeCurrent(NULL, NULL);
		Invalidate(FALSE);
		UpdateWindow();
		return 0;
	}
	if ( m_bSizeChg || (!wParam && !lParam) ) {
		// ｳｨﾝﾄﾞｳｻｲｽﾞ変更、またはﾄﾚｰｽ開始時は、最初からﾎﾞｸｾﾙ構築
		if ( m_pFBO ) {
			delete	m_pFBO;
			m_pFBO = NULL;
		}
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
	if ( IsLatheMode() ) {
		::glOrtho(m_rcDraw.left, m_rcDraw.right,
			-LATHEHEIGHT, LATHEHEIGHT,
			m_rcView.low, m_rcView.high);
//			m_rcDraw.low, m_rcDraw.high);
	}
	else {
		::glOrtho(m_rcDraw.left, m_rcDraw.right, m_rcDraw.top, m_rcDraw.bottom,
			m_rcView.low, m_rcView.high);
//			m_rcDraw.low, m_rcDraw.high);
	}
	::glMatrixMode(GL_MODELVIEW);
	GetGLError();	// error flash

	if ( GetDocument()->IsDocFlag(NCDOC_WORKFILE) ) {
		if ( !pData ) {
			// 初回のみここでBODY描画＆ｽﾃﾝｼﾙ初期化
			Describe_BODY	bd;
			BODYList*	kbl = GetDocument()->GetKodatunoBodyList();
			if ( kbl ) {
				::glPushAttrib(GL_ALL_ATTRIB_BITS);
				::glEnable(GL_STENCIL_TEST);
				::glClear(GL_STENCIL_BUFFER_BIT);
				::glStencilFunc(GL_ALWAYS, 0x01, 0xff);
				::glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
				for ( int ii=0; ii<kbl->getNum(); ii++ )
					bd.DrawBody( (BODY *)kbl->getData(ii) );
				::glDisable(GL_STENCIL_TEST);
				::glPopAttrib();
			}
		}
#ifndef NO_TRACE_WORKFILE
		else {
			ASSERT( m_pfDepthBottom );
			// 保存してあるﾃﾞﾌﾟｽ情報を書き込み
			// ｽﾃﾝｼﾙで更新されないｴﾘｱのﾃﾞﾌﾟｽ情報
			::glWindowPos2i(m_wx, m_wy);
			::glDrawPixels(m_icx, m_icy, GL_DEPTH_COMPONENT, GL_FLOAT, m_pfDepthBottom);
#ifdef _DEBUG
			GetGLError();
#endif
		}
#endif
		if ( pData || lParam ) {
			bResult = CreateBoxel_fromIGES(&pParam);
			if ( !bResult )
				ClearVBO();
		}
	}
	else {
		if ( !m_pFBO ) {
#ifdef _DEBUG
			ASSERT( m_pfDepth );
			DWORD	t1 = ::timeGetTime();
#endif
			// 保存してあるﾃﾞﾌﾟｽ情報を書き込み
			::glWindowPos2i(m_wx, m_wy);
			::glDrawPixels(m_icx, m_icy, GL_DEPTH_COMPONENT, GL_FLOAT, m_pfDepth);
#ifdef _DEBUG
			DWORD	t2 = ::timeGetTime();
			GetGLError();
			printf("glDrawPixels(GL_DEPTH_COMPONENT)=%d[ms]\n", t2 - t1);
#endif
		}
		if ( pData || lParam ) {
			// 指定ｵﾌﾞｼﾞｪｸﾄの描画
			if ( IsLatheMode() ) {
				::glPushAttrib( GL_LINE_BIT );
				::glLineWidth( LATHELINEWIDTH );
				if ( lParam ) {
					for ( i=s; i<e; i++ )
						GetDocument()->GetNCdata(i)->DrawGLLatheDepth();
				}
				else
					pData->DrawGLLatheDepth();
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
				if ( !vBD.empty() ) {
					::glEnableClientState(GL_VERTEX_ARRAY);
					vBD.Draw();
					::glDisableClientState(GL_VERTEX_ARRAY);
				}
			}
			GetGLError();	// error flash
//			::glFinish();
		}
	}

	if ( bResult ) {
		if ( IsLatheMode() ) {
			bResult = GetClipDepthLathe();
		}
		else {
			if ( GetDocument()->IsDocFlag(NCDOC_CYLINDER) )
				bResult = GetClipDepthCylinder();
			else if ( !GetDocument()->IsDocFlag(NCDOC_WORKFILE) )
				bResult = GetClipDepthMill();
		}
	}

	FinalBoxel();
	if ( bResult ) {
		if ( IsLatheMode() )
			CreateVBOLathe();
		else
			CreateVBOMill();
	}
	else
		ClearVBO();

	::wglMakeCurrent(NULL, NULL);
	Invalidate(FALSE);	// OnDraw()で描画
	UpdateWindow();		// 即再描画

	return 0;
}

void CNCViewGL::OnUpdateMoveRoundKey(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(TRUE);
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
			InitialObjectForm();
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

void CNCViewGL::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	if ( pOpt->GetNCViewFlg(NCVIEWFLG_SOLIDVIEW) ) {
		// ﾛｰｶﾙ設定だけを切替
		if ( nFlags&MK_CONTROL && IsLatheMode() ) {
			m_bSlitView = !m_bSlitView;	// 断面表示切り替え（旋盤）
			CClientDC	dc(this);
			::wglMakeCurrent( dc.GetSafeHdc(), m_hRC );
				CreateVBOLathe();	// 頂点ｲﾝﾃﾞｯｸｽ再生成
			::wglMakeCurrent( NULL, NULL );
		}
		else
			m_bWirePath = !m_bWirePath;	// パス表示切り替え
		Invalidate(FALSE);
	}
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

/////////////////////////////////////////////////////////////////////////////
// CNCViewGL 診断

#ifdef _DEBUG
#define _DEPTH_ONLY_
void CNCViewGL::DumpDepth(void) const
{
	extern	LPCTSTR	gg_szCat;		// ", "
	extern	LPCTSTR	gg_szReturn;	// "\n"
	CStdioFile	dbg_fd("C:\\Users\\magara\\Documents\\tmp\\depth_f.csv", CFile::typeText|CFile::modeCreate|CFile::modeWrite);
#ifndef _DEPTH_ONLY_
	CStdioFile	dbg_fx("C:\\Users\\magara\\Documents\\tmp\\depth_x.csv", CFile::typeText|CFile::modeCreate|CFile::modeWrite);
	CStdioFile	dbg_fy("C:\\Users\\magara\\Documents\\tmp\\depth_y.csv", CFile::typeText|CFile::modeCreate|CFile::modeWrite);
	CStdioFile	dbg_fz("C:\\Users\\magara\\Documents\\tmp\\depth_z.csv", CFile::typeText|CFile::modeCreate|CFile::modeWrite);
#endif
	CString		r,
				sd, sx, sy, sz;
	for ( int j=0, n=0; j<m_icy; j++ ) {
		sd.Empty();
#ifndef _DEPTH_ONLY_
		sx.Empty();	sy.Empty();	sz.Empty();
#endif
		for ( int i=0; i<m_icx; i++, n++ ) {
			if ( !sd.IsEmpty() ) {
				sd += gg_szCat;
#ifndef _DEPTH_ONLY_
				sx += gg_szCat;
				sy += gg_szCat;
				sz += gg_szCat;
#endif
			}
			r.Format("%f", m_pfDepth[n]);
			sd += r;
#ifndef _DEPTH_ONLY_
			r.Format("%f", m_pfXYZ[n*NCXYZ+NCA_X]);
			sx += r;
			r.Format("%f", m_pfXYZ[n*NCXYZ+NCA_Y]);
			sy += r;
			r.Format("%f", m_pfXYZ[n*NCXYZ+NCA_Z]);
			sz += r;
#endif
		}
		dbg_fd.WriteString(sd + gg_szReturn);
#ifndef _DEPTH_ONLY_
		dbg_fx.WriteString(sx + gg_szReturn);
		dbg_fy.WriteString(sy + gg_szReturn);
		dbg_fz.WriteString(sz + gg_szReturn);
#endif
	}
}

void CNCViewGL::DumpStencil(void) const
{
	extern	LPCTSTR	gg_szCat;		// ", "
	extern	LPCTSTR	gg_szReturn;	// "\n"
	CStdioFile	dbg_fs("C:\\Users\\magara\\Documents\\tmp\\stencil.csv", CFile::typeText|CFile::modeCreate|CFile::modeWrite);
	CString		r, ss;
	for ( int j=0, n=0; j<m_icy; j++ ) {
		ss.Empty();
		for ( int i=0; i<m_icx; i++, n++ ) {
			if ( !ss.IsEmpty() ) {
				ss += gg_szCat;
			}
			r.Format("%d", m_pbStencil[n]);
			ss += r;
		}
		dbg_fs.WriteString(ss + gg_szReturn);
	}
}

void CNCViewGL::DumpLatheZ(void) const
{
	CStdioFile	dbg_fs("C:\\Users\\magara\\Documents\\tmp\\latheZ.csv", CFile::typeText|CFile::modeCreate|CFile::modeWrite);
	CString		ss;
	for ( int i=0; i<m_icx; i++ ) {
		ss.Format("o:%f, i:%f\n", m_pLatheZo[i], m_pLatheZi[i]);
		dbg_fs.WriteString(ss);
	}
}

CNCDoc* CNCViewGL::GetDocument() // 非デバッグ バージョンはインラインです。
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CNCDoc)));
	return static_cast<CNCDoc *>(m_pDocument);
}
#endif //_DEBUG

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
