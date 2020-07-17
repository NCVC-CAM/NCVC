// NCViewGL.cpp : �����t�@�C��
//

#include "stdafx.h"
#include "NCVC.h"
#include "NCChild.h"
#include "NCdata.h"
#include "NCDoc.h"
#include "NCViewTab.h"
#include "NCInfoTab.h"
#include "NCViewGL.h"
#include "NCListView.h"
#include "ViewOption.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
//#define	_DEBUG_FILEOUT_		// Depth File out
extern	CMagaDbg	g_dbg;
#include <mmsystem.h>			// timeGetTime()
#endif

// ���_�z�񐶐��گ�ޗp
typedef	std::vector<GLuint>	CVElement;
typedef struct tagCREATEELEMENTPARAM {
	std::vector<GLfloat>*	pfXYZ;
	std::vector<GLfloat>*	pfNOR;
	std::vector<CVElement>*	pElement;
	int		cx, cy;
	GLfloat	h, l;
} CREATEELEMENTPARAM, *LPCREATEELEMENTPARAM;

static	DWORD WINAPI CreateElement_WrkThread(LPVOID);
static	DWORD WINAPI CreateElement_CutThread(LPVOID);
static	void		 CreateElement_Side(LPCREATEELEMENTPARAM);

//	�g�嗦�\���̂��߂̕ϊ��P��
//	1�����������߸�ِ� GetDeviceCaps(LOGPIXELS[X|Y]) = 96dpi
//	1��� == 25.4mm
static	const double	LOGPIXEL = 96 / 25.4;	

// CNCViewGL
IMPLEMENT_DYNCREATE(CNCViewGL, CView)

BEGIN_MESSAGE_MAP(CNCViewGL, CView)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEWHEEL()
	ON_WM_KEYDOWN()
	// �߰�ސֲؑ����
	ON_MESSAGE (WM_USERACTIVATEPAGE, OnUserActivatePage)
	// �e�ޭ��ւ�̨��ү����
	ON_MESSAGE (WM_USERVIEWFITMSG, OnUserViewFitMsg)
	// �ƭ������
	ON_COMMAND_RANGE(ID_VIEW_UP, ID_VIEW_RT, OnMoveKey)
	ON_COMMAND_RANGE(ID_VIEW_FIT, ID_VIEW_LENSN, OnLensKey)
END_MESSAGE_MAP()

//	����޳ү���ނ���M���AOpenGL���߂𑀍삷��Ƃ���
//	::wglMakeCurrent( pDC->GetSafeHdc(), m_hRC );
//	-- OpenGL���� --
//	::wglMakeCurrent( NULL, NULL );
//	��K������Ă���

/////////////////////////////////////////////////////////////////////////////
// CNCViewGL �N���X�̍\�z/����

CNCViewGL::CNCViewGL()
{
	m_bActive = FALSE;
	m_cx = m_cy = 0;
	m_dRate = 0.0;
	m_hRC = NULL;
	m_glCode = 0;

	m_nVertexID = m_nNormalID = 0;
	m_pGenBuf = NULL;

	m_enTrackingMode = TM_NONE;
	ClearObjectForm();
}

CNCViewGL::~CNCViewGL()
{
}

/////////////////////////////////////////////////////////////////////////////
// CNCViewGL �N���X�̃I�[�o���C�h�֐�

BOOL CNCViewGL::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.style |= WS_CLIPSIBLINGS|WS_CLIPCHILDREN;
	return CView::PreCreateWindow(cs);
}

void CNCViewGL::OnInitialUpdate() 
{
#ifdef _DEBUG
	CMagaDbg	dbg("CNCViewGL::OnInitialUpdate()\nStart", DBG_CYAN);
#endif
	// �܂�����޳����è�ނłȂ��\��������̂�
	// �����ł� OpenGL����� �͎g�p���Ȃ�
}

void CNCViewGL::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();

	switch ( lHint ) {
	case UAV_STARTSTOPTRACE:
	case UAV_TRACECURSOR:
	case UAV_DRAWMAXRECT:
		return;		// ����
	case UAV_FILEINSERT:	// ��L��`�̕ύX
		pOpt->m_dwUpdateFlg = VIEWUPDATE_DISPLAYLIST | VIEWUPDATE_BOXEL;
		// through
	case UAV_DRAWWORKRECT:	// ܰ���`�ύX
		if ( m_rcDraw != GetDocument()->GetWorkRect() )
			pOpt->m_dwUpdateFlg |= VIEWUPDATE_BOXEL;
		// through
	case UAV_CHANGEFONT:	// �F�̕ύX etc.
		if ( m_bActive ) {
			// �e�߰�ނ��ƂɍX�V��񂪾�Ă���Ă���
			UpdateViewOption();
			Invalidate(FALSE);
		}
		pOpt->m_dwUpdateFlg = 0;
		return;
	case UAV_ADDINREDRAW:
		Invalidate(FALSE);
		return;
	}
	CView::OnUpdate(pSender, lHint, pHint);
}

void CNCViewGL::UpdateViewOption(void)
{
	CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();

	// �������_�ɖ߂�
	if ( pOpt->m_dwUpdateFlg & VIEWUPDATE_BOXEL )
		OnLensKey(ID_VIEW_FIT);

	CClientDC	dc(this);
	::wglMakeCurrent( dc.GetSafeHdc(), m_hRC );

	// �����̐F�ݒ�
	if ( pOpt->m_dwUpdateFlg & VIEWUPDATE_LIGHT ) {
		COLORREF	col;
		GLfloat light_Wrk[] = {0.0, 0.0, 0.0, 1.0};
		GLfloat light_Cut[] = {0.0, 0.0, 0.0, 1.0};
		GLfloat light_Position0[] = {-1.0, -1.0, -1.0,  0.0};
		GLfloat light_Position1[] = { 1.0,  1.0,  1.0,  0.0};
		col = pOpt->GetNcDrawColor(NCCOL_GL_WRK);
		light_Wrk[0] = (GLfloat)GetRValue(col) / 255;	// 255 -> 1.0
		light_Wrk[1] = (GLfloat)GetGValue(col) / 255;
		light_Wrk[2] = (GLfloat)GetBValue(col) / 255;
		col = pOpt->GetNcDrawColor(NCCOL_GL_CUT);
		light_Cut[0] = (GLfloat)GetRValue(col) / 255;
		light_Cut[1] = (GLfloat)GetGValue(col) / 255;
		light_Cut[2] = (GLfloat)GetBValue(col) / 255;
		::glLightfv(GL_LIGHT0, GL_DIFFUSE,  light_Wrk);
		::glLightfv(GL_LIGHT1, GL_DIFFUSE,  light_Wrk);
		::glLightfv(GL_LIGHT2, GL_DIFFUSE,  light_Cut);
		::glLightfv(GL_LIGHT3, GL_DIFFUSE,  light_Cut);
		if ( !m_bActive ) {
			::glLightfv(GL_LIGHT0, GL_POSITION, light_Position0);
			::glLightfv(GL_LIGHT1, GL_POSITION, light_Position1);
			::glLightfv(GL_LIGHT2, GL_POSITION, light_Position0);
			::glLightfv(GL_LIGHT3, GL_POSITION, light_Position1);
		}
	}

	// ܲ԰(�ި���ڲؽ�)
	if ( pOpt->m_dwUpdateFlg & VIEWUPDATE_DISPLAYLIST ) {
		if ( m_glCode > 0 )
			::glDeleteLists(m_glCode, 1);
		CreateWire();
	}

	// �޸�ِ���
	if ( pOpt->m_dwUpdateFlg & VIEWUPDATE_BOXEL ) {
		ClearElement();
		// ܰ���`�̕`��p���W
		m_rcDraw = GetDocument()->GetWorkRect();	// �؍�̈�
		if ( pOpt->GetNCViewFlg(NCVIEWFLG_SOLIDVIEW) ) {
			if ( GLEW_ARB_vertex_buffer_object )	// OpenGL�g����߰�
				CreateBoxel();
			else {
				CString	strErr;
				strErr.Format(IDS_ERR_OPENGLVER, ::glGetString(GL_VERSION));
				AfxMessageBox(strErr, MB_OK|MB_ICONEXCLAMATION);
				pOpt->m_bSolidView  = FALSE;	// �׸ދ���OFF
				pOpt->m_bG00View    = FALSE;
				pOpt->m_bDragRender = FALSE;
				if ( !pOpt->SaveViewOption() )
					AfxMessageBox(IDS_ERR_REGISTRY, MB_OK|MB_ICONEXCLAMATION);
			}
		}
	}

	::wglMakeCurrent( NULL, NULL );
}

BOOL CNCViewGL::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) 
{
	// �������� CDocument::OnCmdMsg() ���Ă΂Ȃ��悤�ɂ���
//	return CView::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
	return CWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

/////////////////////////////////////////////////////////////////////////////
// CNCViewGL �N���X�̃����o�֐�

BOOL CNCViewGL::SetupPixelFormat(CDC* pDC)
{
	static PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,
		24,
		0, 0, 0, 0, 0, 0,
		0,
		0,
		0,
		0, 0, 0, 0,
		32,		// Depth   Buffer
		0,		// Stencil Buffer(32)
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
	m_ptCenter = 0.0;
	m_objectXform[0][0]	= 1.0; m_objectXform[0][1] = 0.0; m_objectXform[0][2] = 0.0; m_objectXform[0][3] = 0.0;
	m_objectXform[1][0]	= 0.0; m_objectXform[1][1] = 1.0; m_objectXform[1][2] = 0.0; m_objectXform[1][3] = 0.0;
	m_objectXform[2][0]	= 0.0; m_objectXform[2][1] = 0.0; m_objectXform[2][2] = 1.0; m_objectXform[2][3] = 0.0;
	m_objectXform[3][0]	= 0.0; m_objectXform[3][1] = 0.0; m_objectXform[3][2] = 0.0; m_objectXform[3][3] = 1.0;
}

void CNCViewGL::CreateWire(void)
{
	::glGetError();		// �װ���ׯ��

	// �؍��߽���ި���ڲؽĐ���
	// ��޼ު�Đ�����������؏���ʂ���������(�����m��Ȃ�)�̂�
	// 10,000��臒l�Ƃ���
	if ( GetDocument()->GetNCsize() <= 10000 ) {
		m_glCode = ::glGenLists(1);
		if( m_glCode > 0 ) {
			// NC�ް��`����ި���ڲؽĐ���
			::glNewList( m_glCode, GL_COMPILE );
				RenderCode();
			::glEndList();
			if ( ::glGetError() != GL_NO_ERROR ) {
				::glDeleteLists(m_glCode, 1);
				m_glCode = 0;
			}
		}
	}
}

void CNCViewGL::CreateBoxel(void)
{
	int			i;
	CPoint3D	pt(m_rcView.CenterPoint());

	// �޸�ِ����̂��߂̏����ݒ�
	::glEnable(GL_DEPTH_TEST);
	::glClearDepth(0.0);		// ���������ɗD�悳���邽�߂̒l
	::glDepthFunc(GL_GREATER);	// ��������D��
	::glClear(GL_DEPTH_BUFFER_BIT);		// ���߽�ޯ̧�̂ݸر
	::glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);	// �װϽ�OFF

	// ��ʂ����ς��ɕ`��
	::glMatrixMode(GL_PROJECTION);
	::glPushMatrix();
	::glLoadIdentity();
	::glOrtho(m_rcDraw.left, m_rcDraw.right, m_rcDraw.top, m_rcDraw.bottom,
		m_rcView.low, m_rcView.high);	// m_rcDraw �łͷ�ط�؂Ȃ̂� m_rcView ���g��

	::glMatrixMode(GL_MODELVIEW);

	// �؍��ʕ`��
	for ( i=0; i<GetDocument()->GetNCsize(); i++ )
		GetDocument()->GetNCdata(i)->DrawBottomFace();
//	ASSERT( ::glGetError() == GL_NO_ERROR );
//	GLenum glError = ::glGetError();

	::glFinish();

	// ���߽�l�̎擾
	GetClipDepth();

	::glMatrixMode(GL_PROJECTION);
	::glPopMatrix();
	::glMatrixMode(GL_MODELVIEW);

	// �ʏ�ݒ�ɖ߂�
	::glClearDepth(1.0);
	::glDepthFunc(GL_LESS);		// �߂�����D��(�ʏ�`��)
	::glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

void CNCViewGL::GetClipDepth(void)
{
#ifdef _DEBUG
	CMagaDbg	dbg("GetClipDepth()");
#endif
	int			i, j, n;
	GLint		viewPort[4];
	GLdouble	mvMatrix[16], pjMatrix[16],
				wx1, wy1, wz1, wx2, wy2, wz2;
	GLfloat		fx, fy, fz;
	GLfloat*	pfDepth;			// ���߽�l�擾�z��ꎞ�̈�
	std::vector<GLfloat>	vfXYZ,	// �ϊ����ꂽܰ��ލ��W
							vfNOR;	// �@���޸��

	::glGetIntegerv(GL_VIEWPORT, viewPort);
	::glGetDoublev (GL_MODELVIEW_MATRIX,  mvMatrix);
	::glGetDoublev (GL_PROJECTION_MATRIX, pjMatrix);

	// ��`�̈���߸�ٍ��W�ɕϊ�
	::gluProject(m_rcDraw.left,  m_rcDraw.top,    0.0, mvMatrix, pjMatrix, viewPort,
		&wx1, &wy1, &wz1);
	::gluProject(m_rcDraw.right, m_rcDraw.bottom, 0.0, mvMatrix, pjMatrix, viewPort,
		&wx2, &wy2, &wz2);
#ifdef _DEBUG
	dbg.printf("left,  top   =(%f, %f)", m_rcDraw.left,  m_rcDraw.top);
	dbg.printf("right, bottom=(%f, %f)", m_rcDraw.right, m_rcDraw.bottom);
	dbg.printf("wx1,   wy1   =(%f, %f)", wx1, wy1);
	dbg.printf("wx2,   wy2   =(%f, %f)", wx2, wy2);
#endif

	m_icx = (int)(wx2 - wx1);
	m_icy = (int)(wy2 - wy1);

	// vector�傫�����w��
	vfXYZ.reserve(m_icx*m_icy*NCXYZ*2);
	vfNOR.reserve(m_icx*m_icy*NCXYZ*2);

	// �ײ��ė̈�����߽�l���擾�i�߸�ْP�ʁj
#ifdef _DEBUG
	DWORD	t1 = ::timeGetTime();
#endif
	pfDepth = new GLfloat [m_icx*m_icy];
	::glPixelStorei(GL_PACK_ALIGNMENT, 1);
	::glReadPixels((GLint)wx1, (GLint)wy1, m_icx, m_icy,
					GL_DEPTH_COMPONENT, GL_FLOAT, pfDepth);
//	ASSERT( ::glGetError() == GL_NO_ERROR );
//	GLenum glError = ::glGetError();
#ifdef _DEBUG
	DWORD	t2 = ::timeGetTime();
	dbg.printf( "glReadPixels()=%d[ms]", t2 - t1 );
#endif

	// ܰ���ʂƐ؍�ʂ̍��W�o�^
	for ( j=0, n=0; j<m_icy; j++ ) {
		for ( i=0; i<m_icx; i++, n++ ) {
//			n = j*m_icx+i;
			::gluUnProject(i+wx1, j+wy1, pfDepth[n],
					mvMatrix, pjMatrix, viewPort,
					&wx2, &wy2, &wz2);	// ����قǒx���Ȃ��̂Ŏ���ϊ��͒��~
			fx = (GLfloat)wx2;
			fy = (GLfloat)wy2;
			fz = (GLfloat)( (pfDepth[n]==0.0) ?	// ���߽�l�������l(��`�͈͓��Ő؍�ʂłȂ�)�Ȃ�
				m_rcDraw.high :		// ܰ���`��ʍ��W
				wz2 );				// �ϊ����W
			vfXYZ.push_back(fx);
			vfXYZ.push_back(fy);
			vfXYZ.push_back(fz);
			vfNOR.push_back(0.0);
			vfNOR.push_back(0.0);
			vfNOR.push_back(1.0);	// ��̫�Ă̖@���޸��
		}
	}
#ifdef _DEBUG
	DWORD	t3 = ::timeGetTime();
	dbg.printf( "AddMatrix1=%d[ms]", t3 - t2 );
#endif

	// ��ʗp���W�̓o�^
	fz = (GLfloat)m_rcDraw.low;	// m_rcDraw.low���W�ŕ~���l�߂�
	for ( j=0; j<m_icy; j++ ) {
		for ( i=0; i<m_icx; i++ ) {
			n = (j*m_icx+i) * NCXYZ;
			fx = vfXYZ[n+NCA_X];
			fy = vfXYZ[n+NCA_Y];
			vfXYZ.push_back(fx);
			vfXYZ.push_back(fy);
			vfXYZ.push_back(fz);
			vfNOR.push_back(0.0);
			vfNOR.push_back(0.0);
			vfNOR.push_back(-1.0);
		}
	}
#ifdef _DEBUG
	DWORD	t4 = ::timeGetTime();
	dbg.printf( "AddMatrix2=%d[ms]", t4 - t3 );
#endif

#ifdef _DEBUG
#ifdef _DEBUG_FILEOUT_
	CStdioFile	dbg_f ("C:\\Users\\magara\\Documents\\tmp\\depth_f.csv", CFile::typeText|CFile::modeCreate|CFile::modeWrite);
	CStdioFile	dbg_fx("C:\\Users\\magara\\Documents\\tmp\\depth_x.csv", CFile::typeText|CFile::modeCreate|CFile::modeWrite);
	CStdioFile	dbg_fy("C:\\Users\\magara\\Documents\\tmp\\depth_y.csv", CFile::typeText|CFile::modeCreate|CFile::modeWrite);
	CStdioFile	dbg_fz("C:\\Users\\magara\\Documents\\tmp\\depth_z.csv", CFile::typeText|CFile::modeCreate|CFile::modeWrite);
	CString		r,
				s, sx, sy, sz;
	for ( j=0; j<m_icy; j++ ) {
		s.Empty();
		sx.Empty();	sy.Empty();	sz.Empty();
		for ( i=0; i<m_icx; i++ ) {
			n = j*m_icx+i;
//			if ( pfDepth[n] != 0.0 ) {
				if ( !s.IsEmpty() ) {
					s  += ", ";
//				if ( !sx.IsEmpty() ) {
					sx += ", ";	sy += ", ";	sz += ", ";
				}
				r.Format(IDS_MAKENCD_FORMAT, pfDepth[n]);
				s += r;
				r.Format(IDS_MAKENCD_FORMAT, vfXYZ[n*NCXYZ+NCA_X]);
				sx += r;
				r.Format(IDS_MAKENCD_FORMAT, vfXYZ[n*NCXYZ+NCA_Y]);
				sy += r;
				r.Format(IDS_MAKENCD_FORMAT, vfXYZ[n*NCXYZ+NCA_Z]);
				sz += r;
			}
//		}
		if ( !s.IsEmpty() ) {
			dbg_f.WriteString (s +"\n");
//		if ( !sx.IsEmpty() ) {
			dbg_fx.WriteString(sx+"\n");
			dbg_fy.WriteString(sy+"\n");
			dbg_fz.WriteString(sz+"\n");
		}
	}
#endif	// _DEBUG_FILEOUT_
//	double	znr = pjMatrix[14]/(pjMatrix[10]-1.0),
//			zfr = pjMatrix[14]/(pjMatrix[10]+1.0);
//	g_dbg.printf("near=%f far=%f", znr, zfr);
//	g_dbg.printf("depth=%f", zfr*znr/(0.515*(zfr-znr)-zfr));
//	---
//	::gluUnProject(0, 0, 0.515, mvMatrix, pjMatrix, viewPort,
//					&wx2, &wy2, &wz2);
//	g_dbg.printf("0.515 => %f", wz2);
//	::gluUnProject(0, 0, 0.482, mvMatrix, pjMatrix, viewPort,
//					&wx2, &wy2, &wz2);
//	g_dbg.printf("0.482 => %f", wz2);
#endif

	delete	pfDepth;

	// ���_�z���ޯ̧��޼ު�Đ���	
	CreateElement(vfXYZ, vfNOR);
}

void CNCViewGL::CreateElement(std::vector<GLfloat>& vfXYZ, std::vector<GLfloat>& vfNOR)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CreateElement()", DBG_BLUE);
	dbg.printf("m_icx=%d m_icy=%d", m_icx, m_icy);
#endif

	std::vector<CVElement>	vElementWrk,	// ���_�z����ޯ��(�ϒ��Q�����z��)
							vElementCut;
	vElementWrk.reserve(m_icy*2);
	vElementCut.reserve(m_icy);

	// ܰ���ʂƐ؍�ʂ��Q�̽گ�ނŏ���
	HANDLE	hThread[2];
	CREATEELEMENTPARAM	param[2];
	for ( int i=0; i<SIZEOF(hThread); i++ ) {
		param[i].pfXYZ = &vfXYZ;
		param[i].pfNOR = &vfNOR;
		param[i].cx = m_icx;
		param[i].cy = m_icy;
		param[i].h  = (GLfloat)m_rcDraw.high;
		param[i].l  = (GLfloat)m_rcDraw.low;
	}
	// �؍�ʏ����̕������ΓI�Ɏ��Ԃ������邽�ߐ�ɃX���b�h���s
	param[0].pElement = &vElementCut;
	param[1].pElement = &vElementWrk;
	hThread[0] = ::CreateThread(NULL, 0, CreateElement_CutThread, &param[0], 0, NULL);
	hThread[1] = ::CreateThread(NULL, 0, CreateElement_WrkThread, &param[1], 0, NULL);
	::WaitForMultipleObjects(SIZEOF(hThread), hThread, TRUE, INFINITE);
	::CloseHandle(hThread[0]);
	::CloseHandle(hThread[1]);

	// ܰ����ʂ͖@���޸�ق̍X�V������
	// �؍�ʏ����Ƃ̔r��������l��
	CreateElement_Side(&param[0]);

	::glGetError();		// �װ���ׯ��

	// ���_�z���GPU��؂ɓ]��
	if ( m_nVertexID > 0 )
		::glDeleteBuffersARB(1, &m_nVertexID);
	::glGenBuffersARB(1, &m_nVertexID);
	::glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_nVertexID);
	::glBufferDataARB(GL_ARRAY_BUFFER_ARB,
			vfXYZ.size()*sizeof(GLfloat), &vfXYZ[0],
			GL_STATIC_DRAW_ARB);
	if ( ::glGetError() != GL_NO_ERROR ) {	// GL_OUT_OF_MEMORY
		::glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
		ClearElement();
		AfxMessageBox(IDS_ERR_OUTOFVRAM, MB_OK|MB_ICONEXCLAMATION);
		return;
	}

	// �@���޸�ق�GPU��؂ɓ]��
	if ( m_nNormalID > 0 )
		::glDeleteBuffersARB(1, &m_nNormalID);
	::glGenBuffersARB(1, &m_nNormalID);
	::glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_nNormalID);
	::glBufferDataARB(GL_ARRAY_BUFFER_ARB,
			vfNOR.size()*sizeof(GLfloat), &vfNOR[0],
			GL_STATIC_DRAW_ARB);
	if ( ::glGetError() != GL_NO_ERROR ) {
		::glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
		ClearElement();
		AfxMessageBox(IDS_ERR_OUTOFVRAM, MB_OK|MB_ICONEXCLAMATION);
		return;
	}
	::glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);

	// ���_���ޯ����GPU��؂ɓ]��
	if ( m_pGenBuf ) {
		::glDeleteBuffersARB(m_vVertexWrk.size()+m_vVertexCut.size(), m_pGenBuf);
		delete	m_pGenBuf;
	}
	m_vVertexWrk.clear();
	m_vVertexCut.clear();
	m_pGenBuf = new GLuint[vElementWrk.size()+vElementCut.size()];
	::glGenBuffersARB(vElementWrk.size()+vElementCut.size(), m_pGenBuf);

	size_t	ii, jj;		// �x���h�~
#ifdef _DEBUG
	int		dbgTriangleWrk = 0, dbgTriangleCut = 0;
#endif

	// ܰ���`�p
	m_vVertexWrk.reserve(vElementWrk.size());
	for ( ii=jj=0; ii<vElementWrk.size(); ii++ ) {
		::glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, m_pGenBuf[jj++]);
		::glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB,
			vElementWrk[ii].size()*sizeof(GLuint), &vElementWrk[ii][0],
			GL_STATIC_DRAW_ARB);
		if ( ::glGetError() != GL_NO_ERROR ) {
			::glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
			ClearElement();
			AfxMessageBox(IDS_ERR_OUTOFVRAM, MB_OK|MB_ICONEXCLAMATION);
			return;
		}
		m_vVertexWrk.push_back(vElementWrk[ii].size());
#ifdef _DEBUG
		dbgTriangleWrk += vElementWrk[ii].size();
#endif
	}
	// �؍�ʗp
	m_vVertexCut.reserve(vElementCut.size());
	for ( ii=0; ii<vElementCut.size(); ii++ ) {
		::glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, m_pGenBuf[jj++]);
		::glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB,
			vElementCut[ii].size()*sizeof(GLuint), &vElementCut[ii][0],
			GL_STATIC_DRAW_ARB);
		if ( ::glGetError() != GL_NO_ERROR ) {
			::glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
			ClearElement();
			AfxMessageBox(IDS_ERR_OUTOFVRAM, MB_OK|MB_ICONEXCLAMATION);
			return;
		}
		m_vVertexCut.push_back(vElementCut[ii].size());
#ifdef _DEBUG
		dbgTriangleCut += vElementCut[ii].size();
#endif
	}
	::glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);

#ifdef _DEBUG
	dbg.printf("VertexCount/3=%d size=%d(*2)", vfXYZ.size()/3, vfXYZ.size()*sizeof(GLfloat));
	dbg.printf("Work IndexCount=%d Triangle=%d", vElementWrk.size(), dbgTriangleWrk/3);
	dbg.printf("Cut  IndexCount=%d Triangle=%d", vElementCut.size(), dbgTriangleCut/3);
#endif
}

DWORD WINAPI CreateElement_WrkThread(LPVOID pVoid)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CreateElement_WrkThread()\nStart", DBG_BLUE);
#endif

	LPCREATEELEMENTPARAM pParam = reinterpret_cast<LPCREATEELEMENTPARAM>(pVoid);
	int			i, j, cxcy = pParam->cx * pParam->cy;
	BOOL		bReverse, bSingle;
	GLuint		n0, n1,	nn;
	UINT		k0, k1;
	CVElement	vElement;

	vElement.reserve(pParam->cx * 2);

	// ܰ����
	// �؍�ʈȊO�� m_rcDraw.high �������Ă���̂�
	// ���������ۂ��̔��f��OK
	for ( j=0; j<pParam->cy-1; j++ ) {
		bReverse = bSingle = FALSE;
		vElement.clear();
		for ( i=0; i<pParam->cx; i++ ) {
			n0 =  j   *pParam->cx+i;
			n1 = (j+1)*pParam->cx+i;
			k0 = n0*NCXYZ + NCA_Z;
			k1 = n1*NCXYZ + NCA_Z;
			if ( pParam->pfXYZ->operator[](k0) != pParam->h ) {
				if ( pParam->pfXYZ->operator[](k1) != pParam->h ) {
					// k0:�~, k1:�~
					if ( vElement.size() > 3 )
						pParam->pElement->push_back(vElement);
					bReverse = bSingle = FALSE;
					vElement.clear();
				}
				else {
					// k0:�~, k1:��
					if ( bSingle ) {
						// �Е��������A������Ȃ炻����break
						if ( vElement.size() > 3 )
							pParam->pElement->push_back(vElement);
						bReverse = bSingle = FALSE;
						vElement.clear();
					}
					else {
						vElement.push_back(n1);
						bReverse = FALSE;	// ����n0����o�^
						bSingle  = TRUE;
					}
				}
			}
			else if ( pParam->pfXYZ->operator[](k1) != pParam->h ) {
				// k0:��, k1:�~
				if ( bSingle ) {
					if ( vElement.size() > 3 )
						pParam->pElement->push_back(vElement);
					bReverse = bSingle = FALSE;
					vElement.clear();
				}
				else {
					vElement.push_back(n0);
					bReverse = TRUE;		// ����n1����o�^
					bSingle  = TRUE;
				}
			}
			else {
				// k0:��, k1:��
				if ( bSingle ) {	// �O�񂪕Е�����
					// ���`��h�~
					if ( vElement.size() > 3 ) {
						pParam->pElement->push_back(vElement);
						// �O��̍��W���ēo�^
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
		// �w������ٰ�߂��I��
		if ( vElement.size() > 3 )
			pParam->pElement->push_back(vElement);
	}

	// ܰ����
	for ( j=0; j<pParam->cy-1; j++ ) {
		bReverse = bSingle = FALSE;
		vElement.clear();
		for ( i=0; i<pParam->cx; i++ ) {
			n0 =  j   *pParam->cx+i;
			n1 = (j+1)*pParam->cx+i;
			k0 = n0*NCXYZ + NCA_Z;	// ܰ���ʂ�Z�l�ŕ`�攻�f
			k1 = n1*NCXYZ + NCA_Z;
			n0 += cxcy;				// ��ʍ��W�ԍ�
			n1 += cxcy;
			if ( pParam->pfXYZ->operator[](k0) < pParam->l ) {
				if ( pParam->pfXYZ->operator[](k1) < pParam->l ) {
					// k0:�~, k1:�~
					if ( vElement.size() > 3 )
						pParam->pElement->push_back(vElement);
					bReverse = bSingle = FALSE;
					vElement.clear();
				}
				else {
					// k0:�~, k1:��
					if ( bSingle ) {
						// �Е��������A������Ȃ炻����break
						if ( vElement.size() > 3 )
							pParam->pElement->push_back(vElement);
						bReverse = bSingle = FALSE;
						vElement.clear();
					}
					else {
						vElement.push_back(n1);
						bReverse = FALSE;	// ����n0����o�^
						bSingle  = TRUE;
					}
				}
			}
			else if ( pParam->pfXYZ->operator[](k1) < pParam->l ) {
				// k0:��, k1:�~
				if ( bSingle ) {
					if ( vElement.size() > 3 )
						pParam->pElement->push_back(vElement);
					bReverse = bSingle = FALSE;
					vElement.clear();
				}
				else {
					vElement.push_back(n0);
					bReverse = TRUE;		// ����n1����o�^
					bSingle  = TRUE;
				}
			}
			else {
				// k0:��, k1:��
				if ( bSingle ) {	// �O�񂪕Е�����
					// ���`��h�~
					if ( vElement.size() > 3 ) {
						pParam->pElement->push_back(vElement);
						// �O��̍��W���ēo�^
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
		// �w������ٰ�߂��I��
		if ( vElement.size() > 3 )
			pParam->pElement->push_back(vElement);
	}

#ifdef _DEBUG
	dbg.printf("end");
#endif

	return 0;
}

DWORD WINAPI CreateElement_CutThread(LPVOID pVoid)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CreateElement_CutThread()\nStart", DBG_BLUE);
#endif

	LPCREATEELEMENTPARAM pParam = reinterpret_cast<LPCREATEELEMENTPARAM>(pVoid);
	int			i, j;
	BOOL		bWorkrct,	// �O��~�~���ǂ����𔻒f�����׸�
				bThrough;	// �O�񁢁��@�@�V
	GLuint		n0, n1;
	UINT		k0, k1, kk0, kk1;
	CVElement	vElement;

	vElement.reserve(pParam->cx * 2);

	// �؍�ʁi��ʁE���ʌ��p�j
	// ���F�؍�ʁC���F�ђʁC�~�Fܰ����
	for ( j=0; j<pParam->cy-1; j++ ) {
		bWorkrct = bThrough = FALSE;
		vElement.clear();
		for ( i=0; i<pParam->cx; i++ ) {
			n0 =  j   *pParam->cx+i;
			n1 = (j+1)*pParam->cx+i;
			k0 = n0*NCXYZ;
			k1 = n1*NCXYZ;
			if ( pParam->pfXYZ->operator[](k0+NCA_Z) >= pParam->h ) {
				if ( pParam->pfXYZ->operator[](k1+NCA_Z) >= pParam->h ) {
					// k0:�~, k1:�~
					if ( bWorkrct ) {
						// �O��� k0:�~, k1:�~
						if ( vElement.size() > 3 ) {
							// break
							pParam->pElement->push_back(vElement);
							// �O��̖@����������(�I�_)�ɕύX
							pParam->pfNOR->operator[](kk0+NCA_X) = -1.0;
							pParam->pfNOR->operator[](kk0+NCA_Z) = 0;
							pParam->pfNOR->operator[](kk1+NCA_X) = -1.0;
							pParam->pfNOR->operator[](kk1+NCA_Z) = 0;
						}
						vElement.clear();
					}
					bWorkrct = TRUE;
				}
				else {
					// k0:�~, k1:��
					// k0:�~, k1:��
					if ( bWorkrct ) {
						// �O�� k0:�~, k1:�~ �Ȃ�
						// �O��̖@�����E����(�n�_)�ɕύX
						pParam->pfNOR->operator[](kk0+NCA_X) = 1.0;
						pParam->pfNOR->operator[](kk0+NCA_Z) = 0;
						pParam->pfNOR->operator[](kk1+NCA_X) = 1.0;
						pParam->pfNOR->operator[](kk1+NCA_Z) = 0;
					}
					bWorkrct = FALSE;
					// �����������̖@��
					pParam->pfNOR->operator[](k0+NCA_Y) = 1.0;
					pParam->pfNOR->operator[](k0+NCA_Z) = 0;
					if ( pParam->pfXYZ->operator[](k1+NCA_Z) < pParam->l ) {
						// k1�i�؂�ڑ��j���������̖@��
						pParam->pfNOR->operator[](k1+NCA_Y) = -1.0;
						pParam->pfNOR->operator[](k1+NCA_Z) = 0;
					}
				}
				bThrough = FALSE;
			}
			else if ( pParam->pfXYZ->operator[](k0+NCA_Z) < pParam->l ) {
				// k0:��
				if ( bWorkrct ) {
					// �O�� k0:�~, k1:�~
					// �O��̖@�����E����(�n�_)�ɕύX
					pParam->pfNOR->operator[](kk0+NCA_X) = 1.0;
					pParam->pfNOR->operator[](kk0+NCA_Z) = 0;
					pParam->pfNOR->operator[](kk1+NCA_X) = 1.0;
					pParam->pfNOR->operator[](kk1+NCA_Z) = 0;
				}
				bWorkrct = FALSE;
				if ( pParam->pfXYZ->operator[](k1+NCA_Z) < pParam->l ) {
					// k0:��, k1:��
					if ( bThrough ) {
						// �O��� k0:��, k1:��
						if ( vElement.size() > 3 ) {
							// break
							pParam->pElement->push_back(vElement);
							// �O��̖@����������(�؂��)�ɕύX
							pParam->pfNOR->operator[](kk0+NCA_X) = -1.0;
							pParam->pfNOR->operator[](kk0+NCA_Z) = 0;
							pParam->pfNOR->operator[](kk1+NCA_X) = -1.0;
							pParam->pfNOR->operator[](kk1+NCA_Z) = 0;
						}
						vElement.clear();
					}
					bThrough = TRUE;
				}
				else {
					// k0:��, k1:�~
					// k0:��, k1:��
					bThrough = FALSE;
					// k0��������, k1��������̖@��
					pParam->pfNOR->operator[](k0+NCA_Y) = -1.0;
					pParam->pfNOR->operator[](k0+NCA_Z) = 0;
					pParam->pfNOR->operator[](k1+NCA_Y) = 1.0;
					pParam->pfNOR->operator[](k1+NCA_Z) = 0;
				}
			}
			else {
				// k0:��
				if ( bWorkrct ) {
					// �O�� k0:�~, k1:�~
					// �O��̖@�����E�����ɕύX
					pParam->pfNOR->operator[](kk0+NCA_X) = 1.0;
					pParam->pfNOR->operator[](kk0+NCA_Z) = 0;
					pParam->pfNOR->operator[](kk1+NCA_X) = 1.0;
					pParam->pfNOR->operator[](kk1+NCA_Z) = 0;
				}
				if ( bThrough ) {
					// �O�� k0:��, k1:��
					// �O��̖@�����������ɕύX
					pParam->pfNOR->operator[](kk0+NCA_X) = -1.0;
					pParam->pfNOR->operator[](kk0+NCA_Z) = 0;
					pParam->pfNOR->operator[](kk1+NCA_X) = -1.0;
					pParam->pfNOR->operator[](kk1+NCA_Z) = 0;
				}
				bWorkrct = bThrough = FALSE;
				if ( pParam->pfXYZ->operator[](k1+NCA_Z) >= pParam->h ) {
					// k0:��, k1:�~
					pParam->pfNOR->operator[](k1+NCA_Y) = -1.0;		// �������̖@��
					pParam->pfNOR->operator[](k1+NCA_Z) = 0;
				}
				else if ( pParam->pfXYZ->operator[](k1+NCA_Z) < pParam->l ) {
					// k0:��, k1:��
					pParam->pfNOR->operator[](k1+NCA_Y) = 1.0;		// ������̖@��
					pParam->pfNOR->operator[](k1+NCA_Z) = 0;
				}
				// k0:��, k1:�� �͂��̂܂�
			}
			// �S���W���Ȃ�
			// �~�~|�������A�����鏊����clear
			vElement.push_back(n1);
			vElement.push_back(n0);
			// 
			kk0 = k0;
			kk1 = k1;
		}
		// �w������ٰ�߂��I��
		if ( vElement.size() > 3 )
			pParam->pElement->push_back(vElement);
	}

#ifdef _DEBUG
	dbg.printf("end");
#endif

	return 0;
}

void CreateElement_Side(LPCREATEELEMENTPARAM pParam)
{
	int			i, j;
	GLuint		n0, n1,
				nn = pParam->cy * pParam->cx;	// ��ʍ��W�ւ̵̾��
	UINT		k0, k1;
	GLfloat		nor = -1.0;
	CVElement	vElement;

	vElement.reserve(pParam->cx * 2);

	// ܰ���`���ʁiX������O�Ɖ��j
	for ( j=0; j<pParam->cy; j+=pParam->cy-1 ) {	// 0��m_icy-1
		vElement.clear();
		for ( i=0; i<pParam->cx; i++ ) {
			n0 = j*pParam->cx+i;	// ��ʍ��W
			n1 = nn + n0;			// ��ʍ��W
			k0 = n0*NCXYZ;
			k1 = n1*NCXYZ;
			if ( pParam->l >= pParam->pfXYZ->operator[](k0+NCA_Z) ) {
				if ( vElement.size() > 3 )
					pParam->pElement->push_back(vElement);
				vElement.clear();
			}
			else {
				vElement.push_back(n0);
				vElement.push_back(n1);
			}
			// �@���޸�ق̏C��
			pParam->pfNOR->operator[](k0+NCA_Y) = nor;	// ��or�������̖@��
			pParam->pfNOR->operator[](k0+NCA_Z) = 0;
			pParam->pfNOR->operator[](k1+NCA_Y) = nor;
			pParam->pfNOR->operator[](k1+NCA_Z) = 0;
		}
		if ( vElement.size() > 3 )
			pParam->pElement->push_back(vElement);
		nor *= -1.0;	// �������]
	}
	// ܰ���`���ʁiY�������ƉE�j
	for ( i=0; i<pParam->cx; i+=pParam->cx-1 ) {
		vElement.clear();
		for ( j=0; j<pParam->cy; j++ ) {
			n0 = j*pParam->cx+i;
			n1 = nn + n0;
			k0 = n0*NCXYZ;
			k1 = n1*NCXYZ;
			if ( pParam->l >= pParam->pfXYZ->operator[](k0+NCA_Z) ) {
				if ( vElement.size() > 3 )
					pParam->pElement->push_back(vElement);
				vElement.clear();
			}
			else {
				vElement.push_back(n0);
				vElement.push_back(n1);
			}
			pParam->pfNOR->operator[](k0+NCA_X) = nor;	// ��or�E�����̖@��
			pParam->pfNOR->operator[](k0+NCA_Z) = 0;
			pParam->pfNOR->operator[](k1+NCA_X) = nor;
			pParam->pfNOR->operator[](k1+NCA_Z) = 0;
		}
		if ( vElement.size() > 3 )
			pParam->pElement->push_back(vElement);
		nor *= -1.0;
	}
}

void CNCViewGL::ClearElement(void)
{
	if ( m_nVertexID > 0 )
		::glDeleteBuffersARB(1, &m_nVertexID);
	if ( m_nNormalID > 0 )
		::glDeleteBuffersARB(1, &m_nNormalID);
	m_nVertexID = m_nNormalID = 0;
	if ( m_pGenBuf ) {
		::glDeleteBuffersARB(m_vVertexWrk.size()+m_vVertexCut.size(), m_pGenBuf);
		delete	m_pGenBuf;
		m_pGenBuf = NULL;
	}
	m_vVertexWrk.clear();
	m_vVertexCut.clear();
}

void CNCViewGL::RenderBack(void)
{
	// �w�i��غ�݂̕`��
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	COLORREF	col1 = pOpt->GetNcDrawColor(NCCOL_BACKGROUND1),
				col2 = pOpt->GetNcDrawColor(NCCOL_BACKGROUND2);
	GLubyte		col1v[3], col2v[3];
	GLdouble	dVertex[3];
	col1v[0] = GetRValue(col1);
	col1v[1] = GetGValue(col1);
	col1v[2] = GetBValue(col1);
	col2v[0] = GetRValue(col2);
	col2v[1] = GetGValue(col2);
	col2v[2] = GetBValue(col2);

	::glPushMatrix();
	::glLoadIdentity();
	::glBegin(GL_QUADS);
	// ����
	dVertex[0] = m_rcView.left;
	dVertex[1] = m_rcView.bottom;
	dVertex[2] = m_rcView.low;		// ���߽ýĖ����ŕ`��
	::glColor3ubv(col1v);
	::glVertex3dv(dVertex);
	// ����
	dVertex[1] = m_rcView.top;
	::glColor3ubv(col2v);
	::glVertex3dv(dVertex);
	// �E��
	dVertex[0] = m_rcView.right;
	::glColor3ubv(col2v);
	::glVertex3dv(dVertex);
	// �E��
	dVertex[1] = m_rcView.bottom;
	::glColor3ubv(col1v);
	::glVertex3dv(dVertex);
	//
	::glEnd();
	::glPopMatrix();
}

void CNCViewGL::RenderAxis(void)
{
	extern	const	PENSTYLE	g_penStyle[];	// ViewOption.cpp
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	double		dLength;
	COLORREF	col;

	::glPushAttrib( GL_CURRENT_BIT | GL_LINE_BIT );	// �F | �����
	::glLineWidth( 2.0 );
	::glEnable( GL_LINE_STIPPLE );
	::glBegin( GL_LINES );

	// X���̶޲��
	dLength = pOpt->GetGuideLength(NCA_X);
	col = pOpt->GetNcDrawColor(NCCOL_GUIDEX);
	::glLineStipple(1, g_penStyle[pOpt->GetNcDrawType(NCA_X)].nGLpattern);
	::glColor3ub( GetRValue(col), GetGValue(col), GetBValue(col) );
	::glVertex3d(-dLength, 0.0, 0.0);
	::glVertex3d( dLength, 0.0, 0.0);
	// Y���̶޲��
	dLength = pOpt->GetGuideLength(NCA_Y);
	col = pOpt->GetNcDrawColor(NCCOL_GUIDEY);
	::glLineStipple(1, g_penStyle[pOpt->GetNcDrawType(NCA_Y)].nGLpattern);
	::glColor3ub( GetRValue(col), GetGValue(col), GetBValue(col) );
	::glVertex3d(0.0, -dLength, 0.0);
	::glVertex3d(0.0,  dLength, 0.0);
	// Z���̶޲��
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
	::glEnable( GL_LINE_STIPPLE );
	// NC�ް��`��
	for ( int i=0; i<GetDocument()->GetNCsize(); i++ )
		GetDocument()->GetNCdata(i)->DrawGL();
	::glDisable( GL_LINE_STIPPLE );
}

CPoint3D CNCViewGL::PtoR(const CPoint& pt)
{
	CPoint3D	ptResult;
	// ���ً�Ԃ̉�]
	ptResult.x = ( 2.0 * pt.x - m_cx ) / m_cx * 0.5;
	ptResult.y = ( m_cy - 2.0 * pt.y ) / m_cy * 0.5;
	double	d  = _hypot( ptResult.x, ptResult.y );
	ptResult.z = cos( (PI/2.0) * ( ( d < 1.0 ) ? d : 1.0 ) );

	ptResult *= 1.0 / ptResult.hypot();

	return ptResult;
}

void CNCViewGL::BeginTracking(const CPoint& pt, ENTRACKINGMODE enTrackingMode)
{
	::ShowCursor(FALSE);
	SetCapture();
	m_enTrackingMode = enTrackingMode;
	switch( m_enTrackingMode ) {
	case TM_NONE:
		break;
	case TM_SPIN:
		m_ptLastRound = PtoR(pt);
		break;
	case TM_PAN:
		m_ptLastMove = pt;
		break;
	}
}

void CNCViewGL::EndTracking(void)
{
	ReleaseCapture();
	::ShowCursor(TRUE);
	m_enTrackingMode = TM_NONE;
	Invalidate(FALSE);		// ���悩�������ݸޕ`���
}

void CNCViewGL::DoTracking( const CPoint& pt )
{
	CClientDC	dc(this);
	::wglMakeCurrent( dc.GetSafeHdc(), m_hRC );

	switch( m_enTrackingMode ) {
	case TM_NONE:
		break;
	case TM_SPIN:
	{
		CPoint3D	ptRound( PtoR(pt) );
		CPoint3D	ptw( ptRound - m_ptLastRound );
		CPoint3D	ptr(
			m_ptLastRound.y*ptRound.z - m_ptLastRound.z*ptRound.y,
			m_ptLastRound.z*ptRound.x - m_ptLastRound.x*ptRound.z,
			m_ptLastRound.x*ptRound.y - m_ptLastRound.y*ptRound.x );
		DoRotation( 180.0 * ptw.hypot(), ptr );
		Invalidate(FALSE);
		m_ptLastRound = ptRound;
	}
		break;
	case TM_PAN:
		m_ptCenter.x += ( pt.x - m_ptLastMove.x ) / m_dRate;
		m_ptCenter.y -= ( pt.y - m_ptLastMove.y ) / m_dRate;
		m_ptLastMove = pt;
		// ����ݸ�&�ޭ��ݸޕϊ��s��
		SetupViewingTransform();
		Invalidate(FALSE);
		break;
	}

	::wglMakeCurrent( NULL, NULL );
}

void CNCViewGL::DoScale(int nRate)
{
	m_rcView.InflateRect(
		_copysign(m_rcView.Width(),  nRate) * 0.05,
		_copysign(m_rcView.Height(), nRate) * 0.05 );
	double	dW = m_rcView.Width(), dH = m_rcView.Height();
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

	// MDI�q�ڰт̽ð���ް�ɏ��\��
	static_cast<CNCChild *>(GetParentFrame())->SetFactorInfo(NC_XYZ_PLANE, m_dRate/LOGPIXEL);
}

void CNCViewGL::DoRotation(double dAngle, const CPoint3D& pt)
{
	// ��]��د�������̵݂�޼ު��̫����د���Ɋ|�����킹��
	::glLoadIdentity();
	::glRotated( dAngle, pt.x, pt.y, pt.z );
	::glMultMatrixd( (GLdouble *)m_objectXform );
	::glGetDoublev( GL_MODELVIEW_MATRIX, (GLdouble *)m_objectXform );

	// ����ݸ�&�ޭ��ݸޕϊ��s��
	SetupViewingTransform();
}

void CNCViewGL::SetupViewingTransform(void)
{
	::glLoadIdentity();
	::glTranslated( m_ptCenter.x, m_ptCenter.y, 0.0 );
	::glMultMatrixd( (GLdouble *)m_objectXform );	// �\����]�i�����f����]�j
}

/////////////////////////////////////////////////////////////////////////////
// CNCViewGL �`��

void CNCViewGL::OnDraw(CDC* pDC)
{
#ifdef _DEBUG
	CMagaDbg	dbg("OnDraw()");
#endif
	ASSERT_VALID(GetDocument());
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();

	// ���ĺ�÷�Ă̊��蓖��
	::wglMakeCurrent( pDC->GetSafeHdc(), m_hRC );

	::glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	::glDisable(GL_DEPTH_TEST);

	// �w�i�̕`��
	RenderBack();
	// ���̕`��
	::glEnable(GL_DEPTH_TEST);
	RenderAxis();

	if ( pOpt->GetNCViewFlg(NCVIEWFLG_SOLIDVIEW) && m_nVertexID > 0 &&
		(pOpt->GetNCViewFlg(NCVIEWFLG_DRAGRENDER) || m_enTrackingMode==TM_NONE) ) {
		::glEnable(GL_LIGHTING);
		// ���悪�������\������邽�߂���غ�ݵ̾��
		::glEnable(GL_POLYGON_OFFSET_FILL);
		::glPolygonOffset(1.0f, 1.0f);
		// ���_�ޯ̧��޼ު�Ăɂ���޸�ٕ`��
		::glEnableClientState(GL_VERTEX_ARRAY);
		::glEnableClientState(GL_NORMAL_ARRAY);
		::glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_nVertexID);
		if ( ::glIsBufferARB(m_nVertexID) == GL_TRUE ) {
			::glVertexPointer(NCXYZ, GL_FLOAT, 0, NULL);
			::glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_nNormalID);
			::glNormalPointer(GL_FLOAT, 0, NULL);
			// ܰ���`
			::glEnable (GL_LIGHT0);
			::glEnable (GL_LIGHT1);
			::glDisable(GL_LIGHT2);  
			::glDisable(GL_LIGHT3);
			size_t	i, j;
			for ( i=j=0; i<m_vVertexWrk.size(); i++, j++ ) {
				::glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, m_pGenBuf[j]);
				if ( ::glIsBufferARB(m_pGenBuf[j]) == GL_TRUE )
					::glDrawElements(GL_TRIANGLE_STRIP, m_vVertexWrk[i], GL_UNSIGNED_INT, NULL);
			}
			// �؍��
			::glDisable(GL_LIGHT0);
			::glDisable(GL_LIGHT1);
			::glEnable (GL_LIGHT2);
			::glEnable (GL_LIGHT3);
			for ( i=0; i<m_vVertexCut.size(); i++, j++ ) {
				::glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, m_pGenBuf[j]);
				if ( ::glIsBufferARB(m_pGenBuf[j]) == GL_TRUE )
					::glDrawElements(GL_TRIANGLE_STRIP, m_vVertexCut[i], GL_UNSIGNED_INT, NULL);
			}
		}
		::glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
		::glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
		::glDisableClientState(GL_NORMAL_ARRAY);
		::glDisableClientState(GL_VERTEX_ARRAY);
		::glDisable(GL_POLYGON_OFFSET_FILL);
		::glDisable(GL_LIGHTING);
	}

#ifdef _DEBUG_PATH
	for ( int i=0; i<GetDocument()->GetNCsize(); i++ )
		GetDocument()->GetNCdata(i)->DrawBottomFace();
//	COLORREF col = pOpt->GetNcDrawColor(NCCOL_GL_WRK);
//	::glBegin(GL_QUADS);
//	::glColor3ub( GetRValue(col), GetGValue(col), GetBValue(col) );
//	::glVertex3d(m_rcDraw.left,  m_rcDraw.bottom, m_rcDraw.low);
//	::glVertex3d(m_rcDraw.left,  m_rcDraw.top,    m_rcDraw.low);
//	::glVertex3d(m_rcDraw.right, m_rcDraw.top,    m_rcDraw.low);
//	::glVertex3d(m_rcDraw.right, m_rcDraw.bottom, m_rcDraw.low);
//	::glEnd();
#endif

	if ( !pOpt->GetNCViewFlg(NCVIEWFLG_SOLIDVIEW) ||
			(pOpt->GetNCViewFlg(NCVIEWFLG_SOLIDVIEW) && pOpt->GetNCViewFlg(NCVIEWFLG_G00VIEW)) ||
			(!pOpt->GetNCViewFlg(NCVIEWFLG_DRAGRENDER) && m_enTrackingMode!=TM_NONE) ) {
		// ����
		if ( m_glCode > 0 )
			::glCallList( m_glCode );
		else
			RenderCode();
	}

//	::glFinish();		// SwapBuffers() �Ɋ܂܂��
	::SwapBuffers( pDC->GetSafeHdc() );

	::wglMakeCurrent(NULL, NULL);
}

/////////////////////////////////////////////////////////////////////////////
// CNCViewGL �f�f

#ifdef _DEBUG
void CNCViewGL::AssertValid() const
{
	CView::AssertValid();
}

void CNCViewGL::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CNCDoc* CNCViewGL::GetDocument() // ��f�o�b�O �o�[�W�����̓C�����C���ł��B
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CNCDoc)));
	return static_cast<CNCDoc *>(m_pDocument);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CNCViewGL ���b�Z�[�W �n���h��

int CNCViewGL::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if ( CView::OnCreate(lpCreateStruct) < 0 )
		return -1;

	// ���د�����̋��E���ق��ڂ�̂ŁCIDC_ARROW �𖾎��I�Ɏw��
	// ���� NCView.cpp �̂悤�� PreCreateWindow() �ł� AfxRegisterWndClass() �łʹװ??
	::SetClassLongPtr(m_hWnd, GCLP_HCURSOR,
		(LONG_PTR)AfxGetApp()->LoadStandardCursor(IDC_ARROW));

	// OpenGL����������
	CClientDC	dc(this);
	HDC	hDC = dc.GetSafeHdc();

	// OpenGL pixel format �̐ݒ�
    if( !SetupPixelFormat(&dc) ) {
		TRACE0("SetupPixelFormat failed\n");
		return -1;
	}

	// �����ݸ޺�÷�Ă̍쐬
	if( !(m_hRC = ::wglCreateContext(hDC)) ) {
		TRACE0("wglCreateContext failed\n");
		return -1;
	}

	// �����ݸ޺�÷�Ă���Ă����޲���÷�Ăɐݒ�
	if( !::wglMakeCurrent(hDC, m_hRC) ) {
		TRACE0("wglMakeCurrent failed\n");
		return -1;
	}

	// OpenGL Extention �g�p����
	GLenum glewResult = ::glewInit();
	if ( glewResult != GLEW_OK ) {
		TRACE1("glewInit() failed code=%s\n", ::glewGetErrorString(glewResult));
		return -1;
	}

	::glEnable(GL_NORMALIZE);
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
#endif

	::wglMakeCurrent( NULL, NULL );

	return 0;
}

void CNCViewGL::OnDestroy()
{
	CClientDC	dc(this);
	::wglMakeCurrent( dc.GetSafeHdc(), m_hRC );

	// �S�̂̐؍�p�X�������ި���ڲؽď���
	if ( m_glCode > 0 )
		::glDeleteLists(m_glCode, 1);

	// ���_�ޯ̧��޼ު�Ă̍폜
	ClearElement();

	::wglMakeCurrent(NULL, NULL);
	::wglDeleteContext( m_hRC );
	
	CView::OnDestroy();
}

void CNCViewGL::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);
	if( cx <= 0 || cy <= 0 )
		return;

	m_cx = cx;
	m_cy = cy;

	// �ޭ��߰Đݒ菈��
	CClientDC	dc(this);
	::wglMakeCurrent( dc.GetSafeHdc(), m_hRC );
	::glViewport(0, 0, cx, cy);
	::wglMakeCurrent(NULL, NULL);
}

void CNCViewGL::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView) 
{
	if ( bActivate ) {
#ifdef _DEBUG
		g_dbg.printf("CNCViewGL::OnActivateView()");
#endif
		// MDI�q�ڰт̽ð���ް�ɏ��\��
		static_cast<CNCChild *>(GetParentFrame())->SetFactorInfo(NC_XYZ_PLANE, m_dRate/LOGPIXEL);
	}
	CView::OnActivateView(bActivate, pActivateView, pDeactiveView);
}

LRESULT CNCViewGL::OnUserActivatePage(WPARAM, LPARAM lParam)
{
	if ( !m_bActive ) {
		CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
		pOpt->m_dwUpdateFlg = VIEWUPDATE_ALL;
		// �`�揉���ݒ�
		UpdateViewOption();
		//
		pOpt->m_dwUpdateFlg = 0;
	}

	// OpenGL����ނ𔭍s����ɂ�����A
	// ��x�ł౸è�ނɂȂ��Ă��Ȃ��Ɗ댯�Ȃ���
	m_bActive = TRUE;

	// �����ޭ��Ƃ͈���āA�g�嗦���X�V����K�v�͖���
	// �ð���ް�ւ̕\���� OnActivateView() �ōs��
	return 0;
}

LRESULT CNCViewGL::OnUserViewFitMsg(WPARAM, LPARAM)
{
	extern	const	double	g_dDefaultGuideLength;	// 50.0 (ViewOption.cpp)
#ifdef _DEBUG
	CMagaDbg	dbg("CNCViewGL::OnUserViewFitMsg()\nStart");
#endif

	m_rcView  = GetDocument()->GetMaxRect();
	m_rcView |= GetDocument()->GetWorkRect();

	// ��L��`�̕␳(�s���\���̖h�~)
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	double	dLength;
	if ( m_rcView.Width() <= NCMIN ) {
		dLength = pOpt->GetGuideLength(NCA_X);
		if ( dLength == 0.0 )
			dLength = g_dDefaultGuideLength;
		m_rcView.left  = -dLength;
		m_rcView.right =  dLength;
	}
	if ( m_rcView.Height() <= NCMIN ) {
		dLength = pOpt->GetGuideLength(NCA_Y);
		if ( dLength == 0.0 )
			dLength = g_dDefaultGuideLength;
		m_rcView.top    = -dLength;
		m_rcView.bottom =  dLength;
	}
	if ( m_rcView.Depth() <= NCMIN ) {
		dLength = pOpt->GetGuideLength(NCA_Z);
		if ( dLength == 0.0 )
			dLength = g_dDefaultGuideLength;
		m_rcView.low  = -dLength;
		m_rcView.high =  dLength;
	}
	// ��޼ު�ċ�`��10%(�㉺���E5%����)�傫��
	m_rcView.InflateRect(m_rcView.Width()*0.05, m_rcView.Height()*0.05);
	m_rcView.NormalizeRect();

	// �ި���ڲ�̱��߸Ĕ䂩�王�쒼���̐ݒ�
	CPointD	pt(m_rcView.CenterPoint());
	double	dW = m_rcView.Width(), dH = m_rcView.Height(), dZ;
	if ( dW > dH ) {
		dH = dW * m_cy / m_cx / 2.0;
		m_rcView.top    = pt.y - dH;
		m_rcView.bottom = pt.y + dH;
		m_dRate = m_cx / dW;
	}
	else {
		dW = dH * m_cx / m_cy / 2.0;
		m_rcView.left   = pt.x - dW;
		m_rcView.right  = pt.x + dW;
		m_dRate = m_cy / dH;
	}
	dZ = max(max(m_rcView.Width(), m_rcView.Height()), m_rcView.Depth()) * 2.0;
	m_rcView.high =  dZ;	// ��
	m_rcView.low  = -dZ;	// ��O
	m_rcView.NormalizeRect();

	CClientDC	dc(this);
	::wglMakeCurrent( dc.GetSafeHdc(), m_hRC );
	::glMatrixMode( GL_PROJECTION );
	::glLoadIdentity();
	// !!! Home�L�[������ GL_INVALID_OPERATION ���������� !!!
	// !!! ���ʖ�����... !!!
	::glOrtho(m_rcView.left, m_rcView.right, m_rcView.top, m_rcView.bottom,
		m_rcView.low, m_rcView.high);
//	ASSERT( ::glGetError() == GL_NO_ERROR );
//	GLenum glError = ::glGetError();
	::glMatrixMode( GL_MODELVIEW );
	::wglMakeCurrent(NULL, NULL);

#ifdef _DEBUG
	dbg.printf("(%f,%f)-(%f,%f)", m_rcView.left, m_rcView.top, m_rcView.right, m_rcView.bottom);
	dbg.printf("(%f,%f)", m_rcView.low, m_rcView.high);
	dbg.printf("Rate=%f", m_dRate);
#endif

	return 0;
}

void CNCViewGL::OnMoveKey(UINT nID)
{
	// �ײ��č��W��1/6���ړ��͈͂Ƃ���
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

void CNCViewGL::OnLensKey(UINT nID)
{
	switch ( nID ) {
	case ID_VIEW_FIT:
		ClearObjectForm();
		OnUserViewFitMsg(0, 0);
		{
			CClientDC	dc(this);
			::wglMakeCurrent( dc.GetSafeHdc(), m_hRC );
			SetupViewingTransform();
			::wglMakeCurrent( NULL, NULL );
		}
		Invalidate(FALSE);
		// MDI�q�ڰт̽ð���ް�ɏ��\��
		static_cast<CNCChild *>(GetParentFrame())->SetFactorInfo(NC_XYZ_PLANE, m_dRate/LOGPIXEL);
		break;
	case ID_VIEW_LENSP:
		DoScale(-1);
		break;
	case ID_VIEW_LENSN:
		DoScale(1);
		break;
	}
}

void CNCViewGL::OnLButtonDown(UINT nFlags, CPoint point)
{
	BeginTracking( point, TM_SPIN );
}

void CNCViewGL::OnLButtonUp(UINT nFlags, CPoint point)
{
	EndTracking();
}

void CNCViewGL::OnRButtonDown(UINT nFlags, CPoint point)
{
	BeginTracking( point, TM_PAN );
}

void CNCViewGL::OnRButtonUp(UINT nFlags, CPoint point)
{
	EndTracking();
}

void CNCViewGL::OnMouseMove(UINT nFlags, CPoint point)
{
	DoTracking( point );
}

BOOL CNCViewGL::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	if ( !pOpt->IsMouseWheel() )
		return FALSE;

	// �����xϳ��ł͔��גl�̏ꍇ������̂ŁA�ꉞ臒l�̔��������
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
	}

	CView::OnKeyDown(nChar, nRepCnt, nFlags);
}

BOOL CNCViewGL::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}
