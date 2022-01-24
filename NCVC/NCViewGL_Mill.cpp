//	NCViewGL_Mill.cpp : ﾌﾗｲｽ加工表示ﾓﾃﾞﾘﾝｸﾞ
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
#ifdef USE_KODATUNO
#include "Kodatuno/Describe_BODY.h"
#undef PI	// Use NCVC (MyTemplate.h)
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#include <mmsystem.h>			// timeGetTime()
//#define	_DEBUG_FILEOUT_		// Depth File out
#endif

using std::vector;
using namespace boost;
extern	DWORD			g_nProcesser;	// ﾌﾟﾛｾｯｻ数(NCVC.cpp)

// １つのｽﾚｯﾄﾞが一度に処理するNCﾌﾞﾛｯｸ数
#ifdef _WIN64
const size_t	MAXNCBLK = 400;
#else
const size_t	MAXNCBLK = 200;
#endif

//	円柱表示の円分割数
static	const int		CYCLECOUNT = ARCCOUNT*8;	// 512分割
static	const float		CYCLESTEP  = PI2/CYCLECOUNT;

// CreateBottomFaceThread() Event Name
static	LPCTSTR	g_szCBFT_S = "CBFT_S%d";
static	LPCTSTR	g_szCBFT_E = "CBFT_E%d";

static	UINT	AddBottomVertexThread(LPVOID);
static	UINT	CreateElementThread(LPVOID);
static	void	CreateElementCut(LPCREATEELEMENTPARAM);
static	void	CreateElementTop(LPCREATEELEMENTPARAM);
static	void	CreateElementBtm(LPCREATEELEMENTPARAM);
static	BOOL	CreateElementSide(LPCREATEELEMENTPARAM, BOOL);

/////////////////////////////////////////////////////////////////////////////

BOOL CNCViewGL::CreateBoxel(BOOL bRange)
{
#ifdef _DEBUG
	printf("CNCViewGL::CreateBoxel() Start\n");
#endif
	BOOL	bResult;
	CProgressCtrl*	pProgress = AfxGetNCVCMainWnd()->GetProgressCtrl();
	pProgress->SetRange32(0, 100);		// 100%表記
	pProgress->SetPos(0);

	// FBO
	CreateFBO();

	// ﾎﾞｸｾﾙ生成のための初期設定
	InitialBoxel();
	::glOrtho(m_rcDraw.left, m_rcDraw.right, m_rcDraw.top, m_rcDraw.bottom,
		m_rcView.low, m_rcView.high);	// m_rcDraw ではｷﾞﾘｷﾞﾘなので m_rcView を使う
//		m_rcDraw.low, m_rcDraw.high);	// ﾃﾞﾌﾟｽ値の更新はｷﾞﾘｷﾞﾘの範囲で精度よく -> 0.0～1.0
	::glMatrixMode(GL_MODELVIEW);
#ifdef _DEBUG
	printf("(%f,%f)-(%f,%f)\n", m_rcDraw.left, m_rcDraw.top, m_rcDraw.right, m_rcDraw.bottom);
	printf("(%f,%f) m_rcView(l,h)\n", m_rcView.low, m_rcView.high);
	printf("(%f,%f) m_rcDraw(l,h)\n", m_rcDraw.low, m_rcDraw.high);
#endif

	if ( GetDocument()->IsDocFlag(NCDOC_WORKFILE) ) {
#ifdef USE_KODATUNO
		// 図形ファイルと重ねるとき
		if ( bRange ) {
			CREATEBOXEL_IGESPARAM pParam = RANGEPARAM(GetDocument()->GetTraceStart(), GetDocument()->GetTraceDraw());
			bResult = CreateBoxel_fromIGES(&pParam);
		}
		else
			bResult = CreateBoxel_fromIGES();
#endif
	}
	else {
		// 切削底面の描画（デプス値の更新）
#ifdef _DEBUG
		DWORD t1 = ::timeGetTime();
#endif
		bResult = CreateBottomFaceThread(bRange, 80);
		if ( bResult ) {
#ifdef _DEBUG
			DWORD t2 = ::timeGetTime();
			printf( "CreateBottomFaceThread()=%d[ms]\n", t2 - t1 );
#endif
			// デプス値の取得
			bResult = GetDocument()->IsDocFlag(NCDOC_CYLINDER) ?
				GetClipDepthCylinder() : GetClipDepthMill();
#ifdef _DEBUG
			DWORD t3 = ::timeGetTime();
			printf( "GetClipDepth[Cylinder|Mill]()=%d[ms]\n", t3 - t2 );
#endif
		}
	}

	FinalBoxel();

	if ( bResult ) {
		// 頂点配列ﾊﾞｯﾌｧｵﾌﾞｼﾞｪｸﾄ生成
#ifdef _DEBUG
		DWORD	t4 = ::timeGetTime();
#endif
		bResult = CreateVBOMill();
#ifdef _DEBUG
		DWORD	t5 = ::timeGetTime();
		printf( "CreateVBOMill()=%d[ms]\n", t5 - t4 );
#endif
	}
	else
		ClearVBO();

	pProgress->SetPos(0);
	return bResult;
}

#ifdef USE_KODATUNO
BOOL CNCViewGL::CreateBoxel_fromIGES(CREATEBOXEL_IGESPARAM* pParam)
{
#ifdef _DEBUG
	printf("CNCViewGL::CreateBoxel_fromIGES() Start\n");
	DWORD	t1 = ::timeGetTime();
#endif
	int		i;
	CVBtmDraw		vBD;
	Describe_BODY	bd;
	BODYList*		kbl = GetDocument()->GetKodatunoBodyList();
	if ( !kbl ) {
#ifdef _DEBUG
		printf("GetKodatunoBodyList() error\n");
#endif
		return FALSE;
	}

	::glPushAttrib(GL_ALL_ATTRIB_BITS);
		
	// 図形モデルの描画（深さ優先でデプス値の更新＋ステンシルビットの更新）
	::glEnable(GL_STENCIL_TEST);
	if ( pParam ) {
		// トレース中なのでステンシルビットが１のとこだけ描画
		::glStencilFunc(GL_EQUAL, 0x01, 0xff);
		::glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	}
	else {
		// 新規レンダリングなのでIGES領域のステンシルビットを１に更新
		::glClear(GL_STENCIL_BUFFER_BIT);
		::glStencilFunc(GL_ALWAYS, 0x01, 0xff);
		::glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
	}
	for ( i=0; i<kbl->getNum(); i++ )
		bd.DrawBody( (BODY *)kbl->getData(i) );
//	::glFinish();
#ifdef _DEBUG
	DWORD	t2 = ::timeGetTime();
	printf( "DrawBody(first)=%d[ms]\n", t2 - t1 );
#endif

	// 切削底面の描画（ステンシルビットが１のとこだけデプス値を更新）
	::glStencilFunc(GL_EQUAL, 0x01, 0xff);
	::glStencilOp(GL_KEEP, GL_KEEP, GL_DECR);	// Zpassだけ"0" → 貫通
	if ( pParam ) {
		if ( pParam->which() == 0 ) {
			(*get<CNCdata*>(pParam))->AddGLBottomFaceVertex(vBD, TRUE);
		}
		else {
			BOOL bStartDraw = TRUE;
			for ( INT_PTR ii=get<RANGEPARAM>(pParam)->s; ii<get<RANGEPARAM>(pParam)->e; ii++ )
				bStartDraw = GetDocument()->GetNCdata(ii)->AddGLBottomFaceVertex(vBD, bStartDraw);
		}
		if ( !vBD.empty() ) {
			::glEnableClientState(GL_VERTEX_ARRAY);
			vBD.Draw();
			::glDisableClientState(GL_VERTEX_ARRAY);
		}
	}
	else {
		if ( !CreateBottomFaceThread(FALSE, 40) ) {
			::glDisable(GL_STENCIL_TEST);
			::glPopAttrib();
			return FALSE;
		}
	}
#ifdef _DEBUG
	DWORD	t3 = ::timeGetTime();
	printf( "CreateBottomFaceThread(first)=%d[ms]\n", t3 - t2 );
#endif

	// 底面のボクセルを構築
	if ( !GetClipDepthMill(DP_BottomStencil) ) {
		::glDisable(GL_STENCIL_TEST);
		::glPopAttrib();
		return FALSE;
	}
#ifndef NO_TRACE_WORKFILE
	// 底面用ﾃﾞﾌﾟｽ情報を保存
	memcpy(m_pfDepthBottom, m_pfDepth, m_icx*m_icy);
#endif
#ifdef _DEBUG
	DWORD	t4 = ::timeGetTime();
	printf( "GetClipDepthMill(DP_BottomStencil)=%d[ms]\n", t4 - t3 );
#endif

	// 手前優先で図形モデルを描画（ステンシルビットが１のとこだけ描画）
	::glDepthFunc(GL_LESS);
	::glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	for ( i=0; i<kbl->getNum(); i++ )
		bd.DrawBody( (BODY *)kbl->getData(i) );
//	::glFinish();
#ifdef _DEBUG
	DWORD	t5 = ::timeGetTime();
	printf( "DrawBody(second)=%d[ms]\n", t5 - t4 );
#endif

	// 切削底面の描画（深さ優先でステンシルビットが１以上のとこだけ描画）
	::glDepthFunc(GL_GREATER);
	::glStencilFunc(GL_LEQUAL, 0x01, 0xff);		// (ref&mask) <= (stencil&mask)
	::glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);	// Zpassだけ">1" → 切削面
	if ( pParam ) {
		if ( !vBD.empty() ) {
			::glEnableClientState(GL_VERTEX_ARRAY);
			vBD.Draw();
			::glDisableClientState(GL_VERTEX_ARRAY);
		}
	}
	else {
		if ( !CreateBottomFaceThread(FALSE, 80) ) {
			::glDisable(GL_STENCIL_TEST);
			::glPopAttrib();
			return FALSE;
		}
	}
#ifdef _DEBUG
	DWORD	t6 = ::timeGetTime();
	printf( "CreateBottomFaceThread(second)=%d[ms]\n", t6 - t5 );
#endif

	// 上面のボクセルを構築
	if ( !GetClipDepthMill(DP_TopStencil) ) {
		::glDisable(GL_STENCIL_TEST);
		::glPopAttrib();
		return FALSE;
	}
#ifdef _DEBUG
	DWORD	t7 = ::timeGetTime();
	printf( "GetClipDepthMill(DP_TopStencil)=%d[ms]\n", t7 - t6 );
#endif

	::glDisable(GL_STENCIL_TEST);
	::glPopAttrib();

	return TRUE;
}
#endif

BOOL CNCViewGL::CreateBottomFaceThread(BOOL bRange, int nProgress)
{
#ifdef _DEBUG
	printf("CNCViewGL::CreateBottomFaceThread() Start\n");
#endif
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
	if ( nLoop <= 0 )
		return bResult;

	// MAXNCBLK/2未満なら１ｽﾚｯﾄﾞで実行
	if ( GetDocument()->GetNCsize() < MAXNCBLK/2 )
		proc = 1;
	else
		proc  = max(1, min(MAXIMUM_WAIT_OBJECTS, min(nLoop, g_nProcesser*2)));
	n = min(nLoop/proc, MAXNCBLK);	// 1つのｽﾚｯﾄﾞが処理する最大ﾌﾞﾛｯｸ数
#ifdef _DEBUG
	printf("loop=%d proc=%d OneThreadSize=%d\n", nLoop, proc, n);
#endif

	// CEventを使うとvector<>内で参照ｶｳﾝﾄが上がってしまう??
	CString		strEvent;
	LPCREATEBOTTOMVERTEXPARAM			pParam;
	vector<LPCREATEBOTTOMVERTEXPARAM>	vParam;
	vector<HANDLE>		vHandleS, vHandleE;
	// CPUの数だけｽﾚｯﾄﾞ生成と実行
	// --- 全体描画だけなのでｽﾚｯﾄﾞは処理終了次第消滅 ---
	for ( i=0; i<proc; i++ ) {
		strEvent.Format(g_szCBFT_S, i);
		vHandleS.push_back( ::CreateEvent(NULL, FALSE, TRUE,  strEvent) );	// 開始ｲﾍﾞﾝﾄは初期ｼｸﾞﾅﾙ状態
		strEvent.Format(g_szCBFT_E, i);
		vHandleE.push_back( ::CreateEvent(NULL, FALSE, FALSE, strEvent) );	// 終了ｲﾍﾞﾝﾄは初期非ｼｸﾞﾅﾙ状態
		pParam = new CREATEBOTTOMVERTEXPARAM(i, GetDocument());		// deleteはAddBottomVertexThread()内で
		pParam->s = e;
		pParam->e = e = min(e+n, nLoop);
		vParam.push_back(pParam);
		AfxBeginThread(AddBottomVertexThread, pParam);
	}
	// 切削底面描画
	::glEnableClientState(GL_VERTEX_ARRAY);
	while ( !vParam.empty() ) {
		// ｽﾚｯﾄﾞ1つ終わるごとに描画処理
		dwResult = ::WaitForMultipleObjects((DWORD)vHandleE.size(), &vHandleE[0], FALSE, INFINITE);
		id = dwResult - WAIT_OBJECT_0;
		if ( id<0 || id>=vParam.size() || !vParam[id]->bResult ) {
			bResult = FALSE;
#ifdef _DEBUG
			printf("WaitForMultipleObjects() error id=%d\n", id);
#endif
			break;
		}
#ifdef _DEBUG
		printf("WaitForMultipleObjects() id=%d Thread=%d return\n", id, vParam[id]->nID);
#endif
		// ため込んだ座標値の描画
		vParam[id]->vBD.Draw();
//		::glFinish();
		//
		if ( e < nLoop ) {
			vParam[id]->vBD.clear();
			vParam[id]->s = e;
			vParam[id]->e = e = min(e+n, nLoop);
			::SetEvent(vHandleS[id]);
		}
		else {
			vParam[id]->bThread = FALSE;	// end of thread
			::SetEvent(vHandleS[id]);
			::CloseHandle(vHandleS[id]);
			::CloseHandle(vHandleE[id]);
			vParam.erase(vParam.begin()+id);
			vHandleS.erase(vHandleS.begin()+id);
			vHandleE.erase(vHandleE.begin()+id);
		}
		p = e*nProgress / nLoop;	// MAX 70% or 35%
		if ( p >= pp ) {
			while ( p >= pp )
				pp += 10;
			AfxGetNCVCMainWnd()->GetProgressCtrl()->SetPos(pp);	// 10%ずつ
		}
	}
	::glDisableClientState(GL_VERTEX_ARRAY);

	GetGLError();		// error flash
//	::glFinish();

	for ( i=0; i<vParam.size(); i++ ) {
		vParam[i]->bThread = FALSE;
		::SetEvent(vHandleS[i]);
		::CloseHandle(vHandleS[i]);
		::CloseHandle(vHandleE[i]);
	}

#ifdef _DEBUG
	printf("AddBottomVertexThread() WaitForMultipleObjects() ");
	if ( bResult )
		printf("ok\n");
	else
		printf("error??n");
#endif

	return bResult;
}

/////////////////////////////////////////////////////////////////////////////
//	六面体

BOOL CNCViewGL::GetClipDepthMill(ENCLIPDEPTH enStencil)
{
#ifdef _DEBUG
	printf("GetClipDepthMill() Start\n");
#endif
	BOOL		bRecalc;
	size_t		i, j, n, nSize;
	int			icx, icy;
	GLint		viewPort[4];
	GLdouble	mvMatrix[16], pjMatrix[16],
				wx, wy, wz;
	ARGVCLIPDEPTH	cdm;

	::glGetIntegerv(GL_VIEWPORT, viewPort);
	::glGetDoublev (GL_MODELVIEW_MATRIX,  mvMatrix);
	::glGetDoublev (GL_PROJECTION_MATRIX, pjMatrix);

	// 矩形領域をﾋﾟｸｾﾙ座標に変換
	::gluProject(m_rcDraw.left,  m_rcDraw.top,    0.0, mvMatrix, pjMatrix, viewPort,
		&wx, &wy, &wz);
	::gluProject(m_rcDraw.right, m_rcDraw.bottom, 0.0, mvMatrix, pjMatrix, viewPort,
		&cdm.wx, &cdm.wy, &cdm.wz);
	icx = (int)(cdm.wx - wx);
	icy = (int)(cdm.wy - wy);
	if ( icx<=0 || icy<=0 )
		return FALSE;
	m_wx = (GLint)wx;
	m_wy = (GLint)wy;
#ifdef _DEBUG
	printf("left,  top   =(%f, %f)\n", m_rcDraw.left,  m_rcDraw.top);
	printf("right, bottom=(%f, %f)\n", m_rcDraw.right, m_rcDraw.bottom);
	printf("wx1,   wy1   =(%f, %f)\n", wx, wy);
	printf("wx2,   wy2   =(%f, %f)\n", cdm.wx, cdm.wy);
	printf("icx=%d icy=%d\n", icx, icy);
	DWORD	t1 = ::timeGetTime();
#endif

	// 領域確保
	if ( m_icx!=icx || m_icy!=icy ) {
		if ( m_pfDepth ) {
			DeleteDepthMemory();
		}
		m_icx = icx;
		m_icy = icy;
	}
	nSize = m_icx * m_icy;
	if ( !m_pfDepth ) {
		try {
			m_pfDepth		= new GLfloat[nSize];
#ifndef NO_TRACE_WORKFILE
			m_pfDepthBottom	= new GLfloat[nSize];
#endif
			if ( enStencil != DP_NoStencil ) {
				m_pbStencil = new GLubyte[nSize];
			}
			nSize  *= NCXYZ;				// XYZ座標格納
			m_pfXYZ = new GLfloat[nSize*2];	// 上面と底面
			m_pfNOR = new GLfloat[nSize*2];
			bRecalc = TRUE;
		}
		catch (CMemoryException* e) {
			AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
			e->Delete();
			DeleteDepthMemory();
			return FALSE;
		}
	}
	else {
		nSize  *= NCXYZ;
		bRecalc = FALSE;
	}
#ifdef _DEBUG
	DWORD	t2 = ::timeGetTime();
	printf( "Memory alloc=%d[ms]\n", t2 - t1 );
#endif

	// 矩形領域のﾃﾞﾌﾟｽ値を取得（ﾋﾟｸｾﾙ単位）
	::glPixelStorei(GL_PACK_ALIGNMENT, 1);
	::glReadPixels(m_wx, m_wy, m_icx, m_icy,
					GL_DEPTH_COMPONENT, GL_FLOAT, m_pfDepth);
#ifdef _DEBUG
	DWORD	t3 = ::timeGetTime();
	GetGLError();
	printf( "glReadPixels(GL_DEPTH_COMPONENT)=%d[ms]\n", t3 - t2);
#endif
	if ( enStencil != DP_NoStencil ) {
		// 矩形領域のｽﾃﾝｼﾙ値を取得
		::glReadPixels(m_wx, m_wy, m_icx, m_icy,
					GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, m_pbStencil);
#ifdef _DEBUG
		DWORD	t31 = ::timeGetTime();
		GetGLError();
		printf( "glReadPixels(GL_STENCIL_INDEX)=%d[ms]\n", t31 - t3);
		t3 = t31;
#endif
	}

	// ﾜｰｸ上面底面と切削面の座標登録
	PFNGETCLIPDEPTHMILL	pfnGetClipDepthMill;
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
	for ( j=n=0, cdm.tp=0, cdm.bm=nSize; j<(size_t)m_icy; j++ ) {
		for ( i=0; i<(size_t)m_icx; i++, n++, cdm.tp+=NCXYZ, cdm.bm+=NCXYZ ) {
			::gluUnProject(i+wx, j+wy, m_pfDepth[n],
					mvMatrix, pjMatrix, viewPort,
					&cdm.wx, &cdm.wy, &cdm.wz);	// それほど遅くないので自作変換は中止
			// それぞれの座標登録へ
			(this->*pfnGetClipDepthMill)(cdm);
		}
	}
#ifdef _DEBUG
	DWORD	t4 = ::timeGetTime();
	printf( "AddMatrix=%d[ms]\n", t4 - t3 );
#endif

#ifdef _DEBUG_FILEOUT_
	DumpDepth();
	if ( m_pbStencil )
		DumpStencil();
#endif

	if ( enStencil != DP_NoStencil )
		return TRUE;	// ステンシル処理の場合はここまで

	//
	if ( GetDocument()->GetTraceMode() == ID_NCVIEW_TRACE_STOP )
		AfxGetNCVCMainWnd()->GetProgressCtrl()->SetPos(90);	


	return TRUE;
}

void CNCViewGL::GetClipDepthMill_All(const ARGVCLIPDEPTH& a)
{
	// ﾜｰﾙﾄﾞ座標
	m_pfXYZ[a.tp+NCA_X] = m_pfXYZ[a.bm+NCA_X] = (GLfloat)a.wx;
	m_pfXYZ[a.tp+NCA_Y] = m_pfXYZ[a.bm+NCA_Y] = (GLfloat)a.wy;
	m_pfXYZ[a.tp+NCA_Z] = min((GLfloat)a.wz, m_rcDraw.high);	// 上面を超えない
	m_pfXYZ[a.bm+NCA_Z] = m_rcDraw.low;	// 底面はm_rcDraw.low座標で敷き詰める
	// ﾃﾞﾌｫﾙﾄの法線ﾍﾞｸﾄﾙ
	m_pfNOR[a.tp+NCA_X] =  0.0f;
	m_pfNOR[a.tp+NCA_Y] =  0.0f;
	m_pfNOR[a.tp+NCA_Z] =  1.0f;
	m_pfNOR[a.bm+NCA_X] =  0.0f;
	m_pfNOR[a.bm+NCA_Y] =  0.0f;
	m_pfNOR[a.bm+NCA_Z] = -1.0f;
}

void CNCViewGL::GetClipDepthMill_Zonly(const ARGVCLIPDEPTH& a)
{
	// 上面Z値のみ
	m_pfXYZ[a.tp+NCA_Z] = min((GLfloat)a.wz, m_rcDraw.high);
}

void CNCViewGL::GetClipDepthMill_BottomStencil(const ARGVCLIPDEPTH& a)
{
	// ステンシル値が"1"のとこだけボクセル取得
	// そうでないところは FLT_MAX(無効値) をセットして貫通を示す
	m_pfXYZ[a.bm+NCA_X] = (GLfloat)a.wx;
	m_pfXYZ[a.bm+NCA_Y] = (GLfloat)a.wy;
#ifdef _DEBUG
	if ( m_pbStencil[a.tp/NCXYZ]==1 )
		m_pfXYZ[a.bm+NCA_Z] = (GLfloat)a.wz;
	else
		m_pfXYZ[a.bm+NCA_Z] = FLT_MAX;
#else
	m_pfXYZ[a.bm+NCA_Z] = m_pbStencil[a.tp/NCXYZ]==1 ? (GLfloat)a.wz : FLT_MAX;
#endif
	m_pfNOR[a.bm+NCA_X] =  0.0f;
	m_pfNOR[a.bm+NCA_Y] =  0.0f;
	m_pfNOR[a.bm+NCA_Z] = -1.0f;
}

void CNCViewGL::GetClipDepthMill_TopStencil(const ARGVCLIPDEPTH& a)
{
	// ステンシル値が"0"より大きいとこだけボクセル取得
	// そうでないところは FLT_MAX(無効値) をセットして貫通を示す
	m_pfXYZ[a.tp+NCA_X] = (GLfloat)a.wx;
	m_pfXYZ[a.tp+NCA_Y] = (GLfloat)a.wy;
#ifdef _DEBUG
	if ( m_pbStencil[a.tp/NCXYZ]>0 )
		m_pfXYZ[a.tp+NCA_Z] = (GLfloat)a.wz;
	else
		m_pfXYZ[a.tp+NCA_Z] = FLT_MAX;
#else
	m_pfXYZ[a.tp+NCA_Z] = m_pbStencil[a.tp/NCXYZ]>0 ? (GLfloat)a.wz : FLT_MAX;
#endif
	m_pfNOR[a.tp+NCA_X] =  0.0f;
	m_pfNOR[a.tp+NCA_Y] =  0.0f;
	m_pfNOR[a.tp+NCA_Z] =  1.0f;
}

/////////////////////////////////////////////////////////////////////////////
//	円柱

BOOL CNCViewGL::GetClipDepthCylinder(void)
{
#ifdef _DEBUG
	printf("GetClipDepthCylinder() Start\n");
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
	printf("left,  top   =(%f, %f)\n", m_rcDraw.left,  m_rcDraw.top);
	printf("right, bottom=(%f, %f)\n", m_rcDraw.right, m_rcDraw.bottom);
	printf("wx1,   wy1   =(%f, %f)\n", wx1, wy1);
	printf("wx2,   wy2   =(%f, %f)\n", wx2, wy2);
	printf("icx=%d icy=%d\n", icx, icy);
	DWORD	t1 = ::timeGetTime();
#endif

	// 領域確保
	if ( m_icx!=icx || m_icy!=icy ) {
		if ( m_pfDepth ) {
			DeleteDepthMemory();
		}
		m_icx = icx;
		m_icy = icy;
	}
	nSize = (CYCLECOUNT+1) * (int)(max(ox,oy)+0.5) * NCXYZ;
	if ( !m_pfDepth ) {
		try {
			m_pfDepth = new GLfloat[m_icx*m_icy];
			m_pfXYZ   = new GLfloat[nSize*2];
			m_pfNOR   = new GLfloat[nSize*2];
		}
		catch (CMemoryException* e) {
			AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
			e->Delete();
			DeleteDepthMemory();
			return FALSE;
		}
		bRecalc = TRUE;
	}
	else {
		bRecalc = FALSE;
	}
#ifdef _DEBUG
	DWORD	t2 = ::timeGetTime();
	printf( "Memory alloc=%d[ms]\n", t2 - t1 );
#endif

	// ｸﾗｲｱﾝﾄ領域のﾃﾞﾌﾟｽ値を取得（ﾋﾟｸｾﾙ単位）
	::glPixelStorei(GL_PACK_ALIGNMENT, 1);
	::glReadPixels(m_wx, m_wy, m_icx, m_icy,
					GL_DEPTH_COMPONENT, GL_FLOAT, m_pfDepth);
#ifdef _DEBUG
	DWORD	t3 = ::timeGetTime();
	GetGLError();
	printf( "glReadPixels()=%d[ms]\n", t3 - t2);
#endif

	// ﾜｰｸ上面底面と切削面の座標登録
	if ( bRecalc ) {
		for ( rx=ry=0, nnu=0, nnd=nSize; rx<ox; rx+=sx, ry+=sy ) {
			nnu0 = nnu;
			nnd0 = nnd;
			for ( j=0, q=0; j<CYCLECOUNT; j++, q+=CYCLESTEP, nnu+=NCXYZ, nnd+=NCXYZ ) {
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
			// 円を閉じる作業（1周分座標登録しないとﾃｸｽﾁｬがうまく貼れない）
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
		for ( rx=ry=0, nnu=0, nnd=nSize; rx<ox; rx+=sx, ry+=sy ) {
			nnu0 = nnu;
			nnd0 = nnd;
			for ( j=0, q=0; j<CYCLECOUNT; j++, q+=CYCLESTEP, nnu+=NCXYZ, nnd+=NCXYZ ) {
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
	printf( "AddMatrix=%d[ms]\n", t4 - t3 );
#endif

	if ( GetDocument()->GetTraceMode() == ID_NCVIEW_TRACE_STOP )
		AfxGetNCVCMainWnd()->GetProgressCtrl()->SetPos(80);	

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////

BOOL CNCViewGL::CreateVBOMill(void)
{
#ifdef _DEBUG
	printf("CNCViewGL::CreateVBOMill() start\n");
#endif
	int		i, cx, cy, n, proc;
	GLsizeiptr	nVBOsize;
	BOOL	bResult = TRUE;
	GLenum	errCode;
	UINT	errLine;

	// 頂点ｲﾝﾃﾞｯｸｽの消去
	if ( m_pSolidElement ) {
		::glDeleteBuffers(GetElementSize(), m_pSolidElement);
		delete[]	m_pSolidElement;
		m_pSolidElement = NULL;
	}
	m_vElementWrk.clear();
	m_vElementCut.clear();

	// 準備
	if ( GetDocument()->IsDocFlag(NCDOC_CYLINDER) ) {
		cx = CYCLECOUNT+1;
		cy = (int)(max(m_icx, m_icy) / 2.0 + 0.5);
	}
	else {
		cx = m_icx;
		cy = m_icy;
	}
	nVBOsize = cx * cy * NCXYZ * 2 * sizeof(GLfloat);
	proc = max(1, min(MAXIMUM_WAIT_OBJECTS, min(cy, (int)g_nProcesser*2)));		// cy==0 ??
	if ( m_nCeProc != proc ) {
		EndOfCreateElementThread();
		m_nCeProc = proc;
		m_pCeHandle	= new HANDLE[proc];
		m_pCeParam	= new CREATEELEMENTPARAM[proc];
		// CPU数ｽﾚｯﾄﾞ起動
		for ( i=0; i<proc; i++ ) {
			m_pCeHandle[i] = m_pCeParam[i].evEnd.m_hObject;
#ifdef _DEBUG
			m_pCeParam[i].dbgID = i;
			CWinThread* hThread = AfxBeginThread(CreateElementThread, &m_pCeParam[i]);
			ASSERT( hThread );
#else
			AfxBeginThread(CreateElementThread, &m_pCeParam[i]);
#endif
		}
	}

	// ｽﾚｯﾄﾞ実行指示
	n = cy / proc;	// 1CPU当たりの処理数
	for ( i=0; i<proc; i++ ) {
		m_pCeParam[i].pfXYZ = m_pfXYZ;
		m_pCeParam[i].pfNOR = m_pfNOR;
		m_pCeParam[i].pbStl = m_pbStencil ? m_pbStencil : NULL;
		m_pCeParam[i].cx = cx;
		m_pCeParam[i].cy = cy;
		m_pCeParam[i].cs = n * i;
		m_pCeParam[i].ce = i==proc-1 ? (cy-1) : min(n*i+n, cy-1);
		m_pCeParam[i].h  = m_rcDraw.high;
		m_pCeParam[i].l  = m_rcDraw.low;
		m_pCeParam[i].evStart.SetEvent();
	}
	::WaitForMultipleObjects(proc, m_pCeHandle, TRUE, INFINITE);
	for ( i=0, n=1; i<proc; i++ ) {
		if ( !m_pCeParam[i].bResult ) {
			n = 0;
			// ｽﾚｯﾄﾞ異常終了の後始末
			EndOfCreateElementThread();
		}
	}

	// ﾜｰｸ側面は法線ﾍﾞｸﾄﾙの更新があり
	// 切削面処理との排他制御を考慮
	if ( !n || !CreateElementSide(&m_pCeParam[0], GetDocument()->IsDocFlag(NCDOC_CYLINDER)) ) {
		return FALSE;
	}

	GetGLError();	// error flash

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
		::glBufferData(GL_ARRAY_BUFFER, nVBOsize, m_pfXYZ,
				GL_STATIC_DRAW);
		::glBindBuffer(GL_ARRAY_BUFFER, m_nVertexID[1]);
		::glBufferData(GL_ARRAY_BUFFER, nVBOsize, m_pfNOR,
				GL_STATIC_DRAW);
		m_nVBOsize = nVBOsize;
	}
	errLine = __LINE__;
	if ( (errCode=GetGLError()) != GL_NO_ERROR ) {	// GL_OUT_OF_MEMORY?
		::glBindBuffer(GL_ARRAY_BUFFER, 0);
		ClearVBO();
		OutputGLErrorMessage(errCode, errLine);
		return FALSE;
	}
	::glBindBuffer(GL_ARRAY_BUFFER, 0);

	// 頂点ｲﾝﾃﾞｯｸｽをGPUﾒﾓﾘに転送
#ifndef _WIN64
	try {	// 32bit版だけﾁｪｯｸ
#endif
		size_t	jj = 0,
				nCutSize = 0, nWrkSize = 0,
				nElement;
		for ( i=0; i<proc; i++ ) {
			nCutSize += m_pCeParam[i].vvElementCut.size();
			nWrkSize += m_pCeParam[i].vvElementWrk.size();
		}

		m_pSolidElement = new GLuint[nWrkSize+nCutSize];
		::glGenBuffers((GLsizei)(nWrkSize+nCutSize), m_pSolidElement);
		errLine = __LINE__;
		if ( (errCode=GetGLError()) != GL_NO_ERROR ) {
			::glBindBuffer(GL_ARRAY_BUFFER, 0);
			ClearVBO();
			OutputGLErrorMessage(errCode, errLine);
			return FALSE;
		}

		m_vElementWrk.reserve(nWrkSize+1);
		m_vElementCut.reserve(nCutSize+1);

#ifdef _DEBUG
		int		dbgTriangleWrk = 0, dbgTriangleCut = 0;
#endif
		// 切削面用
		for ( i=0; i<proc; i++ ) {
			for ( const auto& v : m_pCeParam[i].vvElementCut ) {
				nElement = v.size();
				::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_pSolidElement[jj++]);
				if ( (errCode=GetGLError()) == GL_NO_ERROR ) {
					::glBufferData(GL_ELEMENT_ARRAY_BUFFER, nElement*sizeof(GLuint), &(v[0]),
						GL_STATIC_DRAW);
					errLine = __LINE__;
					errCode = GetGLError();
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
		}
		// ﾜｰｸ矩形用
		for ( i=0; i<proc; i++ ) {
			for ( const auto& v : m_pCeParam[i].vvElementWrk ) {
				nElement = v.size();
				::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_pSolidElement[jj++]);
				if ( (errCode=GetGLError()) == GL_NO_ERROR ) {
					::glBufferData(GL_ELEMENT_ARRAY_BUFFER, nElement*sizeof(GLuint), &(v[0]),
						GL_STATIC_DRAW);
					errLine = __LINE__;
					errCode = GetGLError();
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
		}

		::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

#ifdef _DEBUG
		printf("CreateVBOMill()\n");
		printf(" VertexCount=%d size=%d\n",
			cx*cy*2, cx*cy*NCXYZ*2*sizeof(GLfloat));
		printf(" Work IndexCount=%d Triangle=%d\n",
			nWrkSize, dbgTriangleWrk/3);
		printf(" Cut  IndexCount=%d Triangle=%d\n",
			nCutSize, dbgTriangleCut/3);
#endif
#ifndef _WIN64
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		ClearVBO();
		bResult = FALSE;
	}
#endif
//	if ( GetDocument()->GetTraceMode() == ID_NCVIEW_TRACE_STOP )
//		AfxGetNCVCMainWnd()->GetProgressCtrl()->SetPos(100);	

	return bResult;
}

void CNCViewGL::EndOfCreateElementThread(void)
{
#ifdef _DEBUG
	printf("CNCViewGL::EndOfCreateElementThread() start\n");
#endif
	if ( m_nCeProc>0 && m_pCeParam ) {
		for ( DWORD i=0; i<m_nCeProc; i++ ) {
			m_pCeParam[i].bThread = FALSE;		// end of thread
			m_pCeParam[i].evStart.SetEvent();
		}
		::WaitForMultipleObjects(m_nCeProc, m_pCeHandle, TRUE, INFINITE);
		delete[]	m_pCeParam;
		m_pCeParam = NULL;
		m_nCeProc  = 0;
	}
	if ( m_pCeHandle ) {
		delete[]	m_pCeHandle;
		m_pCeHandle = NULL;
	}
}

void CNCViewGL::CreateTextureMill(void)
{
#ifdef _DEBUG
	printf("CNCViewGL::CreateTextureMill() start\n");
#endif
	if ( m_icx<=0 || m_icy<=0 ) {
#ifdef _DEBUG
		printf(" [m_icx|m_icy]<=0 return\n");
#endif
		return;
	}

	int			i, j, n,
				icx, icy;
	GLsizeiptr	nVertex;
	GLfloat		ft;
	GLfloat*	pfTEX;

	if ( GetDocument()->IsDocFlag(NCDOC_CYLINDER) ) {
		icx = CYCLECOUNT+1;
		icy = (int)(max(m_icx,m_icy)/2.0+0.5);
	}
	else {
		icx = m_icx;
		icy = m_icy;
	}
	nVertex = icx * icy * 2 * 2;	// (X,Y) 上面と底面

#ifndef _WIN64
	try {	// 32bit版だけﾁｪｯｸ
#endif
		pfTEX = new GLfloat[nVertex];
#ifndef _WIN64
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		ClearTexture();
		return;
	}
#endif

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
#ifdef _DEBUG
	printf("CNCViewGL::CreateTextureMill() end\n");
#endif
}

/////////////////////////////////////////////////////////////////////////////

UINT AddBottomVertexThread(LPVOID pVoid)
{
	LPCREATEBOTTOMVERTEXPARAM pParam = reinterpret_cast<LPCREATEBOTTOMVERTEXPARAM>(pVoid);
#ifdef _DEBUG
	DWORD		t1, t2;
	size_t		s;
#endif
	size_t		i, nLoopMax = (size_t)(pParam->pDoc->GetNCsize());
	BOOL		bStartDraw;	// 始点の描画が必要かどうか
	CString		strEvent;

	strEvent.Format(g_szCBFT_S, pParam->nID);
	HANDLE		hStart = OpenEvent(EVENT_ALL_ACCESS, FALSE, strEvent);
	strEvent.Format(g_szCBFT_E, pParam->nID);
	HANDLE		hEnd   = OpenEvent(EVENT_ALL_ACCESS, FALSE, strEvent);

	try {
		while ( TRUE ) {
			// ｽﾚｯﾄﾞ実行許可待ち
			WaitForSingleObject(hStart, INFINITE);
			if ( !pParam->bThread )
				break;
			for ( auto& v : pParam->vBD ) {
				v.vpt.clear();
				v.vel.clear();
			}
			bStartDraw = TRUE;
#ifdef _DEBUG
			printf("AddBottomVertexThread() ThreadID=%d s=%d e=%d Start!\n",
				pParam->nID, pParam->s, pParam->e);
			t1 = timeGetTime();
#endif
			for ( i=pParam->s; i<pParam->e && i<nLoopMax; i++ )
				bStartDraw = pParam->pDoc->GetNCdata(i)->AddGLBottomFaceVertex(pParam->vBD, bStartDraw);
#ifdef _DEBUG
			t2 = timeGetTime();
			s = 0;
			for ( const auto& v : pParam->vBD )
				s += v.vpt.size();
			printf("                        ThreadID=%d End %d[ms] DrawCount=%d VertexSize=%d(/3)\n",
				pParam->nID, t2 - t1, pParam->vBD.size(), s);
#endif
			// 終了イベント
			SetEvent(hEnd);
		}
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		pParam->bResult = FALSE;
	}

#ifdef _DEBUG
	printf("AddBottomVertexThread() ThreadID=%d ThreadEnd\n", pParam->nID);
#endif
	CloseHandle(hStart);
	CloseHandle(hEnd);
	delete	pParam;

	return 0;
}

UINT CreateElementThread(LPVOID pVoid)
{
	LPCREATEELEMENTPARAM pParam = reinterpret_cast<LPCREATEELEMENTPARAM>(pVoid);
#ifdef _DEBUG
	DWORD		t1, t2, tt;
#endif

	try {
		while ( TRUE ) {
			// ｽﾚｯﾄﾞ実行許可待ち
			pParam->evStart.Lock();
			pParam->evEnd.ResetEvent();
			if ( !pParam->bThread )
				break;
			//
			pParam->vvElementCut.clear();
			pParam->vvElementWrk.clear();
#ifdef _DEBUG
			printf("CreateElementThread() ThreadID=%d s=%d e=%d Start!\n",
				pParam->dbgID, pParam->cs, pParam->ce-1);
			tt = 0;
			t1 = timeGetTime();
#endif
			// 切削面の頂点ｲﾝﾃﾞｯｸｽ処理
			CreateElementCut(pParam);
#ifdef _DEBUG
			t2 = timeGetTime();
			tt += t2 - t1;
			printf("--- ThreadID=%d CreateElementCut() End %d[ms]\n",
				pParam->dbgID, t2 - t1);
			t1 = t2;
#endif
			// ﾜｰｸ上面の頂点ｲﾝﾃﾞｯｸｽ処理
			CreateElementTop(pParam);
#ifdef _DEBUG
			t2 = timeGetTime();
			tt += t2 - t1;
			printf("--- ThreadID=%d CreateElementTop() End %d[ms]\n",
				pParam->dbgID, t2 - t1);
			t1 = t2;
#endif
			// ﾜｰｸ底面の頂点ｲﾝﾃﾞｯｸｽ処理
			CreateElementBtm(pParam);
#ifdef _DEBUG
			t2 = timeGetTime();
			tt += t2 - t1;
			printf("--- ThreadID=%d CreateElementBtm() End %d[ms] Total %d[ms]\n",
				pParam->dbgID, t2 - t1, tt);
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
	printf("CreateElementThread() ThreadID=%d thread end.\n",
		pParam->dbgID);
#endif
	pParam->evEnd.SetEvent();
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
						if ( pParam->pfXYZ[z1] < FLT_MAX )
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
					if ( pParam->pfXYZ[z0] < FLT_MAX )
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
					if ( pParam->pfXYZ[z1] < FLT_MAX ) {
						vElement.push_back(n1);
						if ( pParam->pfXYZ[z0] < FLT_MAX )
							vElement.push_back(n0);
					}
				}
				else {
					if ( pParam->pfXYZ[z0] < FLT_MAX ) {
						vElement.push_back(n0);
						if ( pParam->pfXYZ[z1] < FLT_MAX )
							vElement.push_back(n1);
					}
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
						if ( pParam->pfXYZ[z1] < FLT_MAX )
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
					if ( pParam->pfXYZ[z0] < FLT_MAX )
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
					if ( pParam->pfXYZ[z1] < FLT_MAX ) {
						vElement.push_back(b1);
						if ( pParam->pfXYZ[z0] < FLT_MAX )
							vElement.push_back(b0);
					}
				}
				else {
					if ( pParam->pfXYZ[z0] < FLT_MAX ) {
						vElement.push_back(b0);
						if ( pParam->pfXYZ[z1] < FLT_MAX )
							vElement.push_back(b1);
					}
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
			if ( (pParam->pbStl&&pParam->pbStl[n0]==1) || (!pParam->pbStl&&z0z>=pParam->h) ) {
				if ( (pParam->pbStl&&pParam->pbStl[n1]==1) || (!pParam->pbStl&&z1z>=pParam->h) ) {
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
				if ( (pParam->pbStl&&pParam->pbStl[n1]==1) || (!pParam->pbStl&&z1z>=pParam->h) ) {
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
			if ( z0z < FLT_MAX ) {
				vElement.push_back(n0);
				if ( z1z < FLT_MAX )
					vElement.push_back(n1);
			}
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
	printf("CreateElementSide() Start\n");
	DWORD	t1 = ::timeGetTime();
#endif

	int			i, j;
	UINT		kh, kl;
	GLuint		nh, nl, nn;
	CVelement	vElement;

	try {
		if ( bCylinder ) {
			float	q;
			vElement.reserve( (CYCLECOUNT+1)*2 );
			nh = (CYCLECOUNT+1) * (pParam->cy -1);
			nl = nh + (CYCLECOUNT+1) * pParam->cy;
			for ( i=0, q=0; i<=CYCLECOUNT; i++, nh++, nl++, q+=CYCLESTEP ) {
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
			GLfloat		nor = -1.0f;
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
				nor = -nor;	// 符号反転
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
				nor = -nor;
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
	printf("end %d[ms]\n", t2 - t1);
#endif

	return TRUE;
}
