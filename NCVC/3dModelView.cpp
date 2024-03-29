// 3dModelView.cpp : 実装ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "3dModelChild.h"
#include "3dModelDoc.h"
#include "3dModelView.h"
#include "ViewOption.h"
#include "3dRoughScanSetupDlg.h"
#include "3dContourScanSetupDlg.h"

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
	ON_UPDATE_COMMAND_UI(ID_FILE_3DROUGH, &C3dModelView::OnUpdateFile3dRough)
	ON_COMMAND(ID_FILE_3DROUGH, &C3dModelView::OnFile3dRough)
	ON_UPDATE_COMMAND_UI(ID_FILE_3DCONTOUR, &C3dModelView::OnUpdateFile3dSmooth)
	ON_COMMAND(ID_FILE_3DCONTOUR, &C3dModelView::OnFile3dSmooth)
	ON_UPDATE_COMMAND_UI(ID_FILE_3DDEL, &C3dModelView::OnUpdateFile3dDel)
	ON_COMMAND(ID_FILE_3DDEL, &C3dModelView::OnFile3dDel)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// C3dModelView

C3dModelView::C3dModelView()
{
	m_pSelCurve = NULL;
	m_pSelFace  = NULL;
	m_nCoord = 0;
	m_nDrawSize = 0;
}

C3dModelView::~C3dModelView()
{
}

void C3dModelView::ClearVBO(void)
{
	if ( m_nCoord > 0 ) {
		::glDeleteBuffers(1, &m_nCoord);
		m_nCoord = 0;
		m_nDrawSize = 0;
	}
}

void C3dModelView::CreateVBO(void)
{
	GLenum		errCode;
	CVdouble	vpt;

	CClientDC	dc(this);
	::wglMakeCurrent( dc.GetSafeHdc(), m_hRC );

	ClearVBO();
	GetGLError();	// reset GL error
	::glGenBuffers(1, &m_nCoord);
	::glBindBuffer(GL_ARRAY_BUFFER, m_nCoord);
	const VVVCoord& vvv = GetDocument()->GetKoCoord();
	for ( auto it1=vvv.begin(); it1!=vvv.end(); ++it1 ) {
		for ( auto it2=it1->begin(); it2!=it1->end(); ++it2 ) {
			for ( auto it3=it2->begin(); it3!=it2->end(); ++it3 ) {
				vpt.push_back(it3->x);
				vpt.push_back(it3->y);
				vpt.push_back(it3->z);
			}
		}
	}
	m_nDrawSize = (GLsizei)(vpt.size()/NCXYZ);
	::glBufferData(GL_ARRAY_BUFFER, vpt.size()*sizeof(GLdouble), &vpt[0], GL_STATIC_DRAW);
	if ( (errCode=GetGLError()) != GL_NO_ERROR ) {
		ClearVBO();
		OutputGLErrorMessage(errCode, __FILE__, __LINE__);
	}
	::glBindBuffer(GL_ARRAY_BUFFER, 0);

	::wglMakeCurrent(NULL, NULL);
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
	else {
		// m_dRate の更新
		float	dW = fabs(m_rcView.Width()),
				dH = fabs(m_rcView.Height());
		if ( dW > dH )
			m_dRate = m_cx / dW;
		else
			m_dRate = m_cy / dH;
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
	COLORREF	col = AfxGetNCVCApp()->GetViewOption()->GetDxfDrawColor(DXFCOL_CUTTER);
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
	::glEnableClientState(GL_COLOR_ARRAY);
	::glEnableClientState(GL_VERTEX_ARRAY);
	::glDisable(GL_DEPTH_TEST);	// デプステスト無効で描画
	::glDisable(GL_LIGHTING);	// ライティングも無効
	RenderBackground(pOpt->GetDxfDrawColor(DXFCOL_BACKGROUND1), pOpt->GetDxfDrawColor(DXFCOL_BACKGROUND2));

	// 軸の描画
	::glEnable(GL_DEPTH_TEST);
	RenderAxis();
	::glDisableClientState(GL_COLOR_ARRAY);

	// --- テスト描画
//	::glBegin(GL_QUADS);
//		::glColor3f(0.0f, 0.0f, 1.0f);
//		::glVertex3f( 0.0f,  0.0f, 0.0f);
//		::glVertex3f( 0.0f, 50.0f, 0.0f);
//		::glVertex3f(50.0f, 50.0f, 0.0f);
//		::glVertex3f(50.0f,  0.0f, 0.0f);
//	::glEnd();
	// ---

	// モデル描画
	::glEnable(GL_LIGHTING);
//	try {
		// Kodatuno側での描画のため例外処理対応
		// !!! tryブロック設置してもntdll.dllの例外がcatchできない？？？ !!!
		// こんなところでtryブロックを入れるべきではない．別の方法を考える
		DrawKodatunoBody(RM_NORMAL);
//		DrawKodatunoBody(RM_PICKLINE);
//		DrawKodatunoBody(RM_PICKFACE);
//	}
//	catch(...) {
//		AfxMessageBox(IDS_ERR_KODATUNO, MB_OK|MB_ICONSTOP);
//		PostMessage(WM_CLOSE);	// ウィンドウ終了
//	}

	// 荒加工または等高線スキャンパスの描画
	if ( GetDocument()->GetKoCoordMode() != CM_NOCOORD ) {
		::glDisable(GL_LIGHTING);
		DrawKodatunoCoordPath();
	}

	::SwapBuffers( pDC->GetSafeHdc() );
	::wglMakeCurrent(NULL, NULL);
}

void C3dModelView::DrawKodatunoBody(RENDERMODE enRender)
{
	Describe_BODY	bd;
	BODYList*		kbl = GetDocument()->GetKodatunoBodyList();
	BODY*			body;
	GLubyte			rgb[3];
	int				i, j,
					id = 0;		// 複数ボディへの対応

	for ( i=0; i<kbl->getNum(); i++ ) {
		body = (BODY *)kbl->getData(i);
		if ( !body ) continue;
		switch ( enRender ) {
		case RM_PICKLINE:
			::glEnable(GL_DEPTH_TEST);
			// 識別番号を色にセットして描画
			for ( j=0; j<body->TypeNum[_NURBSC]; j++, id++ ) {
		        if ( body->NurbsC[j].EntUseFlag==GEOMTRYELEM && body->NurbsC[j].BlankStat==DISPLAY ) {
					::glDisable(GL_LIGHTING);	// DrawNurbsCurve()の中でEnableされる
					IDtoRGB(id, rgb);		// ViewBaseGL.cpp
					::glColor3ubv(rgb);
					bd.DrawNurbsCurve(body->NurbsC[j]);
				}
			}
			// 隠線処理のため面を白色で描画
			::glDisable(GL_LIGHTING);
			rgb[0] = rgb[1] = rgb[2] = 255;
			::glColor3ubv(rgb);
			for ( j=0; j<body->TypeNum[_NURBSS]; j++ ) {
				if ( body->NurbsS[j].TrmdSurfFlag != KOD_TRUE ) {
					bd.DrawNurbsSurfe(body->NurbsS[j]);
				}
			}
			for ( j=0; j<body->TypeNum[_TRIMMED_SURFACE]; j++ ) {
				bd.DrawTrimdSurf(body->TrmS[j]);
			}
			break;
		case RM_PICKFACE:
			::glDisable(GL_LIGHTING);
			// 識別番号を色にセットして描画
			for ( j=0; j<body->TypeNum[_NURBSS]; j++, id++ ) {
				if ( body->NurbsS[j].TrmdSurfFlag != KOD_TRUE ) {
					IDtoRGB(id, rgb);
					::glColor3ubv(rgb);
					bd.DrawNurbsSurfe(body->NurbsS[j]);
				}
			}
			for ( j=0; j<body->TypeNum[_TRIMMED_SURFACE]; j++, id++ ) {
				IDtoRGB(id, rgb);
				::glColor3ubv(rgb);
				bd.DrawTrimdSurf(body->TrmS[j]);
			}
			break;
		default:
			::glEnable(GL_POLYGON_OFFSET_FILL);
			::glPolygonOffset(1.0f, 1.0f);		// 面を少し後ろにずらして描画
			// ライブラリ側のループで描画
			bd.DrawBody(body);
			::glDisable(GL_POLYGON_OFFSET_FILL);
		}
	}
}

void C3dModelView::DrawKodatunoCoordPath(void)
{
	const COLORREF	col = AfxGetNCVCApp()->GetViewOption()->GetDxfDrawColor(DXFCOL_MOVE);

	::glColor3ub( GetRValue(col), GetGValue(col), GetBValue(col) );
	if ( m_nCoord > 0 ) {
		::glBindBuffer(GL_ARRAY_BUFFER, m_nCoord);
		::glVertexPointer(NCXYZ, GL_DOUBLE, 0, NULL);
		::glDrawArrays(GL_POINTS, 0, m_nDrawSize);
		::glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	else {
		// こちらの描画方法は保険
		const VVVCoord& vvv = GetDocument()->GetKoCoord();
		CVdouble	vpt;
		for ( auto it1=vvv.begin(); it1!=vvv.end(); ++it1 ) {
			for ( auto it2=it1->begin(); it2!=it1->end(); ++it2 ) {
//-				::glVertexPointer(NCXYZ, GL_DOUBLE, sizeof(Coord), &(it2[0]));
//-				::glDrawArrays(GL_POINTS, 0, (GLsizei)(it2->size()));
				for ( auto it3=it2->begin(); it3!=it2->end(); ++it3 ) {
					vpt.push_back(it3->x);
					vpt.push_back(it3->y);
					vpt.push_back(it3->z);
				}
			}
		}
		::glBindBuffer(GL_ARRAY_BUFFER, 0);
		::glVertexPointer(NCXYZ, GL_DOUBLE, 0, &vpt[0]);
		::glDrawArrays(GL_POINTS, 0, (GLsizei)(vpt.size()/NCXYZ));
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

	ClearVBO();

	::wglMakeCurrent(NULL, NULL);
	::wglDeleteContext( m_hRC );

	__super::OnDestroy();
}

void C3dModelView::OnLButtonDown(UINT nFlags, CPoint point)
{
	m_ptDownClick = point;		// ViewBaseGL.h
	__super::OnLButtonDown(nFlags, point);
}

void C3dModelView::OnLButtonUp(UINT nFlags, CPoint point)
{
	if ( m_ptDownClick == point ) {
		// ピック処理
		DoSelect(point);
	}
	__super::OnLButtonUp(nFlags, point);
}

void C3dModelView::SetKodatunoColor(DispStat& Dstat, const COLORREF col)
{
	Dstat.Color[0] = GetRValue(col) / 255.0f;
	Dstat.Color[1] = GetGValue(col) / 255.0f;
	Dstat.Color[2] = GetBValue(col) / 255.0f;
}

void C3dModelView::DoSelect(const CPoint& pt)
{
	CClientDC	dc(this);
	::wglMakeCurrent( dc.GetSafeHdc(), m_hRC );

	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	const COLORREF	col = pOpt->GetDrawColor(COMCOL_SELECT),
					clr = pOpt->GetDxfDrawColor(DXFCOL_CUTTER);

	// NURBS曲線の判定
	::glClearColor(1.0, 1.0, 1.0, 1.0);
	::glClearDepth(1.0);
	::glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	DrawKodatunoBody(RM_PICKLINE);
	NURBSC* pNurbsC = DoSelectCurve(pt);
	if ( pNurbsC ) {
		if ( m_pSelCurve != pNurbsC ) {
			if ( m_pSelCurve ) {
				// 選択済みの色を元に戻す
				SetKodatunoColor(m_pSelCurve->Dstat, clr);
			}
			// 選択オブジェクトに色の設定
			SetKodatunoColor(pNurbsC->Dstat, col);
			m_pSelCurve = pNurbsC;
		}
	}
	else {
		// NURBS曲面の判定
		::glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
		DrawKodatunoBody(RM_PICKFACE);
		NURBSS* pNurbsS = DoSelectFace(pt);
		if ( pNurbsS ) {
			if ( m_pSelFace != pNurbsS ) {
				if ( m_pSelFace ) {
					// 選択済みの色を元に戻す
					SetKodatunoColor(m_pSelFace->Dstat, clr);
				}
				// 選択オブジェクトに色の設定
				SetKodatunoColor(pNurbsS->Dstat, col);
				m_pSelFace = pNurbsS;
			}
		}
		else {
			// 線と面，両方の選択を解除
			if ( m_pSelCurve ) {
				SetKodatunoColor(m_pSelCurve->Dstat, clr);
			}
			if ( m_pSelFace ) {
				SetKodatunoColor(m_pSelFace->Dstat, clr);
			}
			m_pSelCurve = NULL;
			m_pSelFace  = NULL;
		}
	}

	// 再描画
	Invalidate(FALSE);

	::wglMakeCurrent(NULL, NULL);
}

NURBSC* C3dModelView::DoSelectCurve(const CPoint& pt)
{
	// マウスポイントの色情報を取得
	GLubyte	buf[READBUF];
	::glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	::glReadPixels(pt.x-PICKREGION, m_cy-pt.y-PICKREGION,	// y座標に注意!!
		2*PICKREGION, 2*PICKREGION, GL_RGBA, GL_UNSIGNED_BYTE, buf);
	GetGLError();

	// bufの中で一番多く存在するIDを検索
	BODY*	body;
	NURBSC*	pNurbsC = NULL;
	int		nResult = SearchSelectID(buf), nIndex, nNum;

	if ( nResult >= 0 ) {
		nIndex = nResult;
		// 複数のボディから選択オブジェクトを検索
		BODYList*	kbl = GetDocument()->GetKodatunoBodyList();
		for ( int i=0; i<kbl->getNum(); i++ ) {
			body = (BODY *)kbl->getData(i);
			if ( !body ) continue;
			nNum = body->TypeNum[_NURBSC];
			if ( nNum < nIndex ) {
				nIndex -= nNum;
				continue;
			}
			pNurbsC = &body->NurbsC[nIndex];
		}
	}

	return pNurbsC;
}

NURBSS* C3dModelView::DoSelectFace(const CPoint& pt)
{
	// マウスポイントの色情報を取得
	GLubyte	buf[READBUF];
	::glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	::glReadPixels(pt.x-PICKREGION, m_cy-pt.y-PICKREGION,	// y座標に注意!!
		2*PICKREGION, 2*PICKREGION, GL_RGBA, GL_UNSIGNED_BYTE, buf);
	GetGLError();

	// bufの中で一番多く存在するIDを検索
	BODY*	body;
	NURBSS*	pNurbsS = NULL;
	int		nResult = SearchSelectID(buf), nIndex, nNum;

	if ( nResult >= 0 ) {
		nIndex = nResult;
		// 複数のボディから選択オブジェクトを検索
		BODYList*	kbl = GetDocument()->GetKodatunoBodyList();
		for ( int i=0; i<kbl->getNum(); i++ ) {
			body = (BODY *)kbl->getData(i);
			if ( !body ) continue;
			nNum = body->TypeNum[_NURBSS] + body->TypeNum[_TRIMMED_SURFACE];
			if ( nNum < nIndex ) {
				nIndex -= nNum;
				continue;
			}
			nNum = body->TypeNum[_NURBSS];
			if ( nNum <= nIndex ) {
				nIndex -= nNum;
				pNurbsS = body->TrmS[nIndex].pts;
			}
			else {
				pNurbsS = &body->NurbsS[nIndex];
			}
		}
	}

	return pNurbsS;
}

void C3dModelView::OnLensKey(UINT nID)
{
	switch ( nID ) {
	case ID_VIEW_FIT:
		m_rcView  = GetDocument()->GetMaxRect();
		SetOrthoView();
		{
			CClientDC	dc(this);
			::wglMakeCurrent( dc.GetSafeHdc(), m_hRC );
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

void C3dModelView::OnUpdateFile3dRough(CCmdUI* pCmdUI)
{
	// 荒加工スキャンが有効になる条件
	pCmdUI->Enable(m_pSelCurve && m_pSelFace);
}

void C3dModelView::OnFile3dRough()
{
	// 荒加工スキャン設定
	C3dRoughScanSetupDlg	dlg(GetDocument());
	if ( dlg.DoModal() != IDOK )
		return;

	// ウエイトカーソル
	CWaitCursor	wait;
	// 荒加工スキャンパスの生成
	if ( GetDocument()->MakeRoughCoord(m_pSelFace, m_pSelCurve) ) {
		// VBO
		CreateVBO();
		// 荒加工スキャンパス描画
		Invalidate(FALSE);
	}
}

void C3dModelView::OnUpdateFile3dSmooth(CCmdUI* pCmdUI)
{
	// 等高線加工スキャンが有効になる条件
	pCmdUI->Enable(m_pSelFace!=NULL);
}

void C3dModelView::OnFile3dSmooth()
{
	// 等高線加工スキャン設定
	C3dContourScanSetupDlg	dlg(GetDocument());
	if ( dlg.DoModal() != IDOK )
		return;

	// ウエイトカーソル
	CWaitCursor	wait;
	// 等高線加工スキャンパスの生成
	if ( GetDocument()->MakeContourCoord(m_pSelFace) ) {
		// VBO
		CreateVBO();
		// 等高線加工スキャンパス描画
		Invalidate(FALSE);
	}
}

void C3dModelView::OnUpdateFile3dDel(CCmdUI* pCmdUI)
{
	pCmdUI->Enable( !GetDocument()->GetKoCoord().empty() );
}

void C3dModelView::OnFile3dDel()
{
	GetDocument()->ClearKoCoord();
	ClearVBO();
	Invalidate(FALSE);
}
