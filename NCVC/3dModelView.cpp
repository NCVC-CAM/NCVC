// 3dModelView.cpp : 実装ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "3dModelChild.h"
#include "3dModelDoc.h"
#include "3dModelView.h"
#include "ViewOption.h"
#include "Kodatuno/Describe_BODY.h"
#undef PI	// Use NCVC (MyTemplate.h)

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(C3dModelView, CViewBaseGL)

BEGIN_MESSAGE_MAP(C3dModelView, CViewBaseGL)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_COMMAND_RANGE(ID_VIEW_FIT, ID_VIEW_LENSN, &C3dModelView::OnLensKey)
END_MESSAGE_MAP()

#define	PICKREGION		5
#define	READBUF			(2*PICKREGION*2*PICKREGION*4)

// インデックスIDとRGBAを変換するローカルコード
static	void	IDtoRGB(int, GLubyte[]);
static	int		RGBtoID(GLubyte[]);
static	int		SearchSelectID(GLubyte[]);

/////////////////////////////////////////////////////////////////////////////
// C3dModelView

C3dModelView::C3dModelView()
{
	m_glCode = 0;
	m_nSelCurve = -1;
}

C3dModelView::~C3dModelView()
{
}

/////////////////////////////////////////////////////////////////////////////
// C3dModelView クラスのオーバライド関数

void C3dModelView::OnInitialUpdate() 
{
	m_rcView  = GetDocument()->GetMaxRect();
	CRecentViewInfo*	pInfo = GetDocument()->GetRecentViewInfo();
	if ( pInfo && !pInfo->GetViewInfo(m_objXform, m_rcView, m_ptCenter) ) {
		SetOrthoView();		// ViewBaseGL.cpp
	}

	// 設定
	CClientDC	dc(this);
	::wglMakeCurrent( dc.GetSafeHdc(), m_hRC );
	::glMatrixMode( GL_PROJECTION );
	::glOrtho(m_rcView.left, m_rcView.right, m_rcView.top, m_rcView.bottom,
		m_rcView.low, m_rcView.high);
	GetGLError();
	::glMatrixMode( GL_MODELVIEW );
	SetupViewingTransform();

	// 光源
	::glEnable(GL_AUTO_NORMAL);
	::glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
	const CViewOption*	pOpt = AfxGetNCVCApp()->GetViewOption();
	COLORREF	col = pOpt->GetDxfDrawColor(DXFCOL_CUTTER);
	GLfloat light_Model[] = {(GLfloat)GetRValue(col) / 255,
							 (GLfloat)GetGValue(col) / 255,
							 (GLfloat)GetBValue(col) / 255, 1.0f};
//	GLfloat light_Position0[] = {-1.0f, -1.0f, -1.0f,  0.0f},
//			light_Position1[] = { 1.0f,  1.0f,  1.0f,  0.0f};
	::glLightfv(GL_LIGHT0, GL_DIFFUSE,  light_Model);
//	::glLightfv(GL_LIGHT1, GL_DIFFUSE,  light_Model);
//	::glLightfv(GL_LIGHT0, GL_POSITION, light_Position0);
//	::glLightfv(GL_LIGHT1, GL_POSITION, light_Position1);
	::glEnable (GL_LIGHT0);
//	::glEnable (GL_LIGHT1);

	// ディスプレイリスト
	m_glCode = ::glGenLists(1);
	if( m_glCode > 0 ) {
		// プリミティブ描画のディスプレイリスト生成
		::glNewList( m_glCode, GL_COMPILE );
			DrawBody(RM_NORMAL);
		::glEndList();
		if ( GetGLError() != GL_NO_ERROR ) {
			::glDeleteLists(m_glCode, 1);
			m_glCode = 0;
		}
	}

	::wglMakeCurrent(NULL, NULL);
}

#ifdef _DEBUG
C3dModelDoc* C3dModelView::GetDocument() // 非デバッグ バージョンはインラインです。
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(C3dModelDoc)));
	return static_cast<C3dModelDoc *>(m_pDocument);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// C3dModelView 描画

void C3dModelView::OnDraw(CDC* pDC)
{
	ASSERT_VALID(GetDocument());
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();

	// ｶﾚﾝﾄｺﾝﾃｷｽﾄの割り当て
	::wglMakeCurrent( pDC->GetSafeHdc(), m_hRC );

	::glClearColor(0.0, 0.0, 0.0, 0.0);
	::glClearDepth(1.0);
	::glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	// 背景の描画
	RenderBackground(pOpt->GetDxfDrawColor(DXFCOL_BACKGROUND1), pOpt->GetDxfDrawColor(DXFCOL_BACKGROUND2));

	float		dLength = 50.0f;
	COLORREF	col;
	::glPushAttrib( GL_LINE_BIT );
	::glLineWidth( 2.0f );
	::glBegin( GL_LINES );
	// X軸のｶﾞｲﾄﾞ
	col = pOpt->GetNcDrawColor(NCCOL_GUIDEX);
	::glColor3ub( GetRValue(col), GetGValue(col), GetBValue(col) );
	::glVertex3f(-dLength, 0.0f, 0.0f);
	::glVertex3f( dLength, 0.0f, 0.0f);
	// Y軸のｶﾞｲﾄﾞ
	col = pOpt->GetNcDrawColor(NCCOL_GUIDEY);
	::glColor3ub( GetRValue(col), GetGValue(col), GetBValue(col) );
	::glVertex3f(0.0f, -dLength, 0.0f);
	::glVertex3f(0.0f,  dLength, 0.0f);
	// Z軸のｶﾞｲﾄﾞ
	col = pOpt->GetNcDrawColor(NCCOL_GUIDEZ);
	::glColor3ub( GetRValue(col), GetGValue(col), GetBValue(col) );
	::glVertex3f(0.0f, 0.0f, -dLength);
	::glVertex3f(0.0f, 0.0f,  dLength);
	::glEnd();
	::glPopAttrib();

	// --- テスト描画
//	::glBegin(GL_QUADS);
//		::glColor3f(0.0f, 0.0f, 1.0f);
//		::glVertex3f( 0.0f,  0.0f, 0.0f);
//		::glVertex3f( 0.0f, 50.0f, 0.0f);
//		::glVertex3f(50.0f, 50.0f, 0.0f);
//		::glVertex3f(50.0f,  0.0f, 0.0f);
//	::glEnd();
	// ---

	// モデル描画 Kodatuno
	::glEnable(GL_LIGHTING);
	if ( m_glCode > 0 )
		::glCallList( m_glCode );
	else
		DrawBody(RM_NORMAL);

	// 選択したプリミティブを描画
	if ( m_nSelCurve > 0 ) {
		::glDisable(GL_LIGHTING);
		::glDisable(GL_DEPTH_TEST);	// ﾃﾞﾌﾟｽﾃｽﾄ無効で上書き描画
		COLORREF col = pOpt->GetDrawColor(COMCOL_SELECT);
		::glColor3ub( GetRValue(col), GetGValue(col), GetBValue(col) );
		Describe_BODY	bd;
		BODY* body = (BODY *)GetDocument()->GetKodatunoBodyList()->getData(0);
		bd.DrawNurbsCurve(body->NurbsC[m_nSelCurve]);
	}

	::SwapBuffers( pDC->GetSafeHdc() );
	::wglMakeCurrent(NULL, NULL);
}

void C3dModelView::DrawBody(RENDERMODE enRender)
{
	Describe_BODY	bd;
	BODYList*		kbl = GetDocument()->GetKodatunoBodyList();
	BODY*			body;
	GLubyte			rgb[3];
	int				i, j;

	for ( i=0; i<kbl->getNum(); i++ ) {
		body = (BODY *)kbl->getData(i);
		if ( !body ) continue;

		switch ( enRender ) {
		case RM_PICKLINE:
			// 識別番号を色にセットして描画
			for ( j=0; j<body->TypeNum[_NURBSC]; j++ ) {
		        if ( body->NurbsC[j].EntUseFlag==GEOMTRYELEM && body->NurbsC[j].BlankStat==DISPLAY ) {
					ZEROCLR(rgb);		// CustomClass.h
					IDtoRGB(j, rgb);
					::glColor3ubv(rgb);
					bd.DrawNurbsCurve(body->NurbsC[j]);
				}
			}
			// 隠線処理のため面を白色で描画
			::glDisable(GL_LIGHTING);
			rgb[0] = rgb[1] = rgb[2] = 255;
			::glColor3ubv(rgb);
			for ( j=0; j<body->TypeNum[_NURBSS]; j++ ) {
				if ( body->NurbsS[j].TrmdSurfFlag == KOD_TRUE )
					continue;
				else
					bd.DrawNurbsSurfe(body->NurbsS[j]);
			}
			for ( j=0; j<body->TypeNum[_TRIMMED_SURFACE]; j++ ) {
				bd.DrawTrimdSurf(body->TrmS[j]);
			}
			break;
		case RM_PICKFACE:
			break;
		default:
			// ライブラリ側のループで描画
			bd.DrawBody(body);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// C3dModelView メッセージ ハンドラー

int C3dModelView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (__super::OnCreate(lpCreateStruct) == -1)
		return -1;

	return 0;
}

void C3dModelView::OnDestroy()
{
	// 回転行列等を保存
	CRecentViewInfo* pInfo = GetDocument()->GetRecentViewInfo();
	if ( pInfo ) 
		pInfo->SetViewInfo(m_objXform, m_rcView, m_ptCenter);

	// OpenGL 後処理
	CClientDC	dc(this);
	::wglMakeCurrent( dc.GetSafeHdc(), m_hRC );

	// ディスプレイリスト消去
	if ( m_glCode > 0 )
		::glDeleteLists(m_glCode, 1);

	::wglMakeCurrent(NULL, NULL);
	::wglDeleteContext( m_hRC );

	__super::OnDestroy();
}

void C3dModelView::OnLButtonDown(UINT nFlags, CPoint point)
{
	m_ptLclick = point;
	__super::OnLButtonDown(nFlags, point);
}

void C3dModelView::OnLButtonUp(UINT nFlags, CPoint point)
{
	if ( m_ptLclick == point ) {
		// ピック処理
		DoSelect(point);
	}
	__super::OnLButtonUp(nFlags, point);
}

void C3dModelView::DoSelect(const CPoint& pt)
{
	CClientDC	dc(this);
	::wglMakeCurrent( dc.GetSafeHdc(), m_hRC );

	::glClearColor(1.0, 1.0, 1.0, 1.0);	// 白色でクリア
	::glClearDepth(1.0);
	::glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	// 識別番号を色にセットしてNURBS曲線だけ描画
	DrawBody(RM_PICKLINE);
	GetGLError();		// error flash

	// マウスポイントの色情報を取得
	GLubyte	buf[READBUF];
	::glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	::glReadPixels(pt.x-PICKREGION, m_cy-pt.y-PICKREGION,	// y座標に注意!!
		2*PICKREGION, 2*PICKREGION, GL_RGBA, GL_UNSIGNED_BYTE, buf);
	GetGLError();

	// bufの中で一番多く存在するIDを検索
	m_nSelCurve = SearchSelectID(buf);

	// 再描画
	Invalidate();

	::wglMakeCurrent(NULL, NULL);
}

void C3dModelView::OnLensKey(UINT nID)
{
	switch ( nID ) {
	case ID_VIEW_FIT:
		if ( m_dRoundStep != 0.0f ) {
			KillTimer(IDC_OPENGL_DRAGROUND);
			m_dRoundStep = 0.0f;
		}
		{
			CClientDC	dc(this);
			::wglMakeCurrent( dc.GetSafeHdc(), m_hRC );
			SetOrthoView();
			::glMatrixMode( GL_PROJECTION );
			::glLoadIdentity();
			::glOrtho(m_rcView.left, m_rcView.right, m_rcView.top, m_rcView.bottom,
				m_rcView.low, m_rcView.high);
			::glMatrixMode( GL_MODELVIEW );
			IdentityMatrix();
			SetupViewingTransform();
			::wglMakeCurrent( NULL, NULL );
		}
		Invalidate(FALSE);
		break;
	case ID_VIEW_LENSP:
		DoScale(-1);
		break;
	case ID_VIEW_LENSN:
		DoScale(1);
		break;
	}
}

/////////////////////////////////////////////////////////////////////////////

void IDtoRGB(int id, GLubyte rgb[])
{
	// 0～254 の数値を RGB に変換
	// （255 は白色クリア値）
	div_t	d;
	int		n = 0;

	d.quot = id;
	do {
		d = div(d.quot, 254);
		rgb[n++] = d.rem;
	} while ( d.quot>0 && n<3 );
}

int RGBtoID(GLubyte rgb[])
{
	return rgb[0]==255 && rgb[1]==255 && rgb[2]==255 ?
		-1 : (rgb[2]*254*254 + rgb[1]*254 + rgb[0]);
}

int SearchSelectID(GLubyte buf[])
{
	GLubyte			rgb[3];
	std::map<int, int>	mp;
	int				id, maxct=0, maxid=-1;

	for ( int i=0; i<READBUF; i+=4 ) {
		rgb[0] = buf[i+0];
		rgb[1] = buf[i+1];
		rgb[2] = buf[i+2];
		id = RGBtoID(rgb);
		if ( id >= 0 ) {
			mp[id] = mp[id] + 1;	// mp[id]++; ではワーニング
		}
	}

	typedef std::map<int, int>::const_reference	T;
	BOOST_FOREACH(T x, mp) {
		if ( maxct < x.second ) {
			maxid = x.first;
			maxct = x.second;
		}
	}

#ifdef _DEBUG
	for ( int y=0; y<2*PICKREGION; y++ ) {
		CString	str, s;
		for ( int x=0; x<2*PICKREGION; x++ ) {
			rgb[0] = buf[4*(y*2*PICKREGION+x)+0];
			rgb[1] = buf[4*(y*2*PICKREGION+x)+1];
			rgb[2] = buf[4*(y*2*PICKREGION+x)+2];
			s.Format(" %d", RGBtoID(rgb));
//			s.Format(" %u", buf[4*(y*2*PICKREGION+x)+0]);
			str += s;
		}
		printf("pBuf[%d]=%s\n", y, LPCTSTR(str));
	}
	printf("size=%zd\n", mp.size());
	printf("maxid=%d, cnt=%d\n", maxid, maxct);
#endif

	return maxid;
}
