// NCViewGL.cpp : �����t�@�C��
//
/////////////////////////////////////////////////////////////////////////////

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
#include <numeric>				// accumulate()

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
//#define	_DEBUG_FILEOUT_		// Depth File out
//#define	_DEBUG_DRAWTEST_
extern	CMagaDbg	g_dbg;
#include <mmsystem.h>			// timeGetTime()
#endif

using std::vector;
using namespace boost;
extern	int		g_nProcesser;		// ��۾����(NCVC.cpp)

// ���_�z�񐶐��گ�ޗp
typedef	vector<GLuint>	CVElement;
typedef struct tagCREATEELEMENTPARAM {
#ifdef _DEBUG
	int		dbgThread;		// �گ��ID
#endif
	const GLfloat*	pfXYZ;
	GLfloat*		pfNOR;
	BOOL	bResult;
	int		cx, cy,
			cs, ce;
	GLfloat	h, l;
	// ���_�z����ޯ��(�ϒ�2�����z��)
	vector<CVElement>	vElementCut,
						vElementWrk;
} CREATEELEMENTPARAM, *LPCREATEELEMENTPARAM;

static	DWORD WINAPI CallCreateElementThread(LPVOID);
static	void	CreateElementCut(LPCREATEELEMENTPARAM);
static	void	CreateElementTop(LPCREATEELEMENTPARAM);
static	void	CreateElementBtm(LPCREATEELEMENTPARAM);
static	BOOL	CreateElementSide(LPCREATEELEMENTPARAM);

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
	// �߰�ސֲؑ����
	ON_MESSAGE (WM_USERACTIVATEPAGE, &CNCViewGL::OnUserActivatePage)
	// �e�ޭ��ւ�̨��ү����
	ON_MESSAGE (WM_USERVIEWFITMSG, &CNCViewGL::OnUserViewFitMsg)
	// �ƭ������
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, &CNCViewGL::OnUpdateEditCopy)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_UP,  ID_VIEW_RT,  &CNCViewGL::OnUpdateMoveRoundKey)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_RUP, ID_VIEW_RRT, &CNCViewGL::OnUpdateMoveRoundKey)
	ON_COMMAND_RANGE(ID_VIEW_UP,  ID_VIEW_RT,    &CNCViewGL::OnMoveKey)
	ON_COMMAND_RANGE(ID_VIEW_RUP, ID_VIEW_RRT,   &CNCViewGL::OnRoundKey)
	ON_COMMAND_RANGE(ID_VIEW_FIT, ID_VIEW_LENSN, &CNCViewGL::OnLensKey)
	ON_COMMAND(ID_OPTION_DEFVIEWINFO, &CNCViewGL::OnDefViewInfo)
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
#ifdef _DEBUG_FILEOPEN
	g_dbg.printf("CNCViewGL::CNCViewGL() Start");
#endif
	m_bActive = FALSE;
	m_cx = m_cy = m_icx = m_icy = 0;
	m_dRate = m_dRoundAngle = m_dRoundStep = 0.0;
	m_hRC = NULL;
	m_glCode = 0;

	m_nVertexID = m_nNormalID =
		m_nPictureID = m_nTextureID = 0;
	m_pGenBuf = NULL;
	m_pfnDrawProc = NULL;

	m_enTrackingMode = TM_NONE;
	ClearObjectForm();
}

/////////////////////////////////////////////////////////////////////////////
// CNCViewGL �N���X�̃I�[�o���C�h�֐�

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

	// �޲�ޕ\��
	if ( GetDocument()->IsDocFlag(NCDOC_LATHE) ) {
		m_strGuide  = g_szNdelimiter[NCA_Z];	// [ZYX]
		m_strGuide += g_szNdelimiter[NCA_Y];
		m_strGuide += g_szNdelimiter[NCA_X];
	}
	else {
		m_strGuide = g_szNdelimiter;
		m_strGuide.Delete(NCXYZ, 999);			// [XYZ]
	}

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
		if ( pOpt->GetNCViewFlg(NCVIEWFLG_SOLIDVIEW) &&
				GLEW_ARB_vertex_buffer_object &&	// OpenGL�g����߰�
				m_rcDraw != GetDocument()->GetWorkRect() )
			pOpt->m_dwUpdateFlg |= VIEWUPDATE_BOXEL;
		// through
	case UAV_CHANGEFONT:	// �F�̕ύX etc.
		if ( m_bActive ) {
			GLdouble	objectXform[4][4];
			CPointD		ptCenter(m_ptCenter);
			if ( pOpt->m_dwUpdateFlg & VIEWUPDATE_BOXEL ) {
				// �s����ظ����ޯ�����
				memcpy(objectXform, m_objectXform, sizeof(objectXform));
				// �s����ظ��̏�����
				OnLensKey(ID_VIEW_FIT);
			}
			// �\�����̍X�V
			UpdateViewOption();
			if ( pOpt->m_dwUpdateFlg & VIEWUPDATE_BOXEL ) {
				// �s����ظ���ؽı
				memcpy(m_objectXform, objectXform, sizeof(objectXform));
				m_ptCenter = ptCenter;
				// ��]�̕���
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

	// ø����摜�̓ǂݍ���
	if ( pOpt->GetNCViewFlg(NCVIEWFLG_SOLIDVIEW) &&
			pOpt->m_dwUpdateFlg & VIEWUPDATE_TEXTURE &&
			GLEW_ARB_vertex_buffer_object ) {	// OpenGL�g����߰�
		ClearTexture();
		if ( pOpt->GetNCViewFlg(NCVIEWFLG_TEXTURE) ) {
			if ( ReadTexture(pOpt->GetTextureFile()) &&	// m_nPictureID�l���
					!(pOpt->m_dwUpdateFlg & VIEWUPDATE_BOXEL) ) {
				// �摜�����ύX
				if ( GetDocument()->IsDocFlag(NCDOC_LATHE) )
					CreateTextureLathe();
				else if ( GetDocument()->IsDocFlag(NCDOC_WIRE) )
					CreateTextureWire();
				else
					CreateTextureMill();
			}
		}
	}

	// �����̐F�ݒ�
	if ( pOpt->m_dwUpdateFlg & (VIEWUPDATE_LIGHT|VIEWUPDATE_TEXTURE) ) {
		GLfloat light_Wrk[] = {0.0, 0.0, 0.0, 1.0};
		GLfloat light_Cut[] = {0.0, 0.0, 0.0, 1.0};
		GLfloat light_Position0[] = {-1.0, -1.0, -1.0,  0.0};
		GLfloat light_Position1[] = { 1.0,  1.0,  1.0,  0.0};
		if ( m_nPictureID > 0 ) {
			// ø����F��L���ɂ��邽�ߔ��F����
			for ( int i=0; i<NCXYZ; i++ )
				light_Wrk[i] = light_Cut[i] = 1.0;
		}
		else {
			// �\���ݒ�Ɋ�Â�����
			COLORREF	col;
			col = pOpt->GetNcDrawColor(NCCOL_GL_WRK);
			light_Wrk[0] = (GLfloat)GetRValue(col) / 255;	// 255 -> 1.0
			light_Wrk[1] = (GLfloat)GetGValue(col) / 255;
			light_Wrk[2] = (GLfloat)GetBValue(col) / 255;
			col = pOpt->GetNcDrawColor(NCCOL_GL_CUT);
			light_Cut[0] = (GLfloat)GetRValue(col) / 255;
			light_Cut[1] = (GLfloat)GetGValue(col) / 255;
			light_Cut[2] = (GLfloat)GetBValue(col) / 255;
		}
		::glLightfv(GL_LIGHT0, GL_DIFFUSE,  light_Wrk);
		::glLightfv(GL_LIGHT1, GL_DIFFUSE,  light_Wrk);
		::glLightfv(GL_LIGHT2, GL_DIFFUSE,  light_Cut);
		::glLightfv(GL_LIGHT3, GL_DIFFUSE,  light_Cut);
		::glLightfv(GL_LIGHT0, GL_POSITION, light_Position0);
		::glLightfv(GL_LIGHT1, GL_POSITION, light_Position1);
		::glLightfv(GL_LIGHT2, GL_POSITION, light_Position0);
		::glLightfv(GL_LIGHT3, GL_POSITION, light_Position1);
	}

	// ܲ԰(�ި���ڲؽ�)
	if ( pOpt->m_dwUpdateFlg & VIEWUPDATE_DISPLAYLIST ) {
		if ( m_glCode > 0 )
			::glDeleteLists(m_glCode, 1);
		if ( m_pfnDrawProc )
			CreateDisplayList();
	}

	// �\���b�h���f���̕\��
	if ( pOpt->m_dwUpdateFlg & VIEWUPDATE_BOXEL ) {
		ClearVBO();
		// ܰ���`�̕`��p���W
		if ( pOpt->GetNCViewFlg(NCVIEWFLG_SOLIDVIEW) ) {
			if ( GLEW_ARB_vertex_buffer_object ) {	// OpenGL�g����߰�
				BOOL	bResult;
				// �؍�̈�̐ݒ�
				m_rcDraw = GetDocument()->GetWorkRect();
				if ( GetDocument()->IsDocFlag(NCDOC_LATHE) ) {
					// ���՗p��]���f���̐���
					bResult = CreateLathe();
					if ( bResult && m_nPictureID > 0 )
						CreateTextureLathe();	// ø������W�̐���
				}
				else if ( GetDocument()->IsDocFlag(NCDOC_WIRE) ) {
					// ܲԉ��H
					bResult = CreateWire();
					if ( bResult && m_nPictureID > 0 )
						CreateTextureWire();
				}
				else {
					// �ײ��p�޸�ق̐���
					bResult = CreateBoxel();
					if ( bResult && m_nPictureID > 0 )
						CreateTextureMill();
				}
			}
			else {
				ClearTexture();		// �ꉞ�ر���Ă���
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
	Invalidate(FALSE);
}

BOOL CNCViewGL::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) 
{
#ifdef _DEBUG_CMDMSG
	g_dbg.printf("CNCViewGL::OnCmdMsg()");
#endif
	// �������� CDocument::OnCmdMsg() ���Ă΂Ȃ��悤�ɂ���
//	return __super::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
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

void CNCViewGL::CreateDisplayList(void)
{
	::glGetError();		// �װ���ׯ��

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

/////////////////////////////////////////////////////////////////////////////
//	�ײ����H�\������ݸ�
/////////////////////////////////////////////////////////////////////////////

BOOL CNCViewGL::CreateBoxel(void)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CreateBoxel()\nStart");
#endif
	// �޸�ِ����̂��߂̏����ݒ�
	InitialBoxel();
	// ��ʂ����ς��ɕ`��
	::glMatrixMode(GL_PROJECTION);
	::glPushMatrix();
	::glLoadIdentity();
	::glOrtho(m_rcDraw.left, m_rcDraw.right, m_rcDraw.top, m_rcDraw.bottom,
		m_rcView.low, m_rcView.high);	// m_rcDraw �łͷ�ط�؂Ȃ̂� m_rcView ���g��
	::glMatrixMode(GL_MODELVIEW);
	
#ifdef _DEBUG
	dbg.printf("(%f,%f)-(%f,%f)", m_rcDraw.left, m_rcDraw.top, m_rcDraw.right, m_rcDraw.bottom);
	dbg.printf("(%f,%f)", m_rcView.low, m_rcView.high);
	DWORD	t1 = ::timeGetTime();
#endif

	// �؍��ʕ`��
	::glEnableClientState(GL_VERTEX_ARRAY);
	int		i, nLoop = GetDocument()->GetNCsize();
	for ( i=0; i<nLoop; ++i )
		GetDocument()->GetNCdata(i)->DrawGLBottomFace();
	::glDisableClientState(GL_VERTEX_ARRAY);
//	ASSERT( ::glGetError() == GL_NO_ERROR );
//	GLenum glError = ::glGetError();
	::glFinish();
#ifdef _DEBUG
	DWORD	t2 = ::timeGetTime();
	dbg.printf( "DrawGLBottomFace()=%d[ms]", t2 - t1 );
#endif

	// ���߽�l�̎擾
	BOOL	bResult = GetDocument()->IsDocFlag(NCDOC_CYLINDER) ?
						GetClipDepthCylinder() : GetClipDepthMill();
	if ( !bResult )
		ClearVBO();

	// �I������
	FinalBoxel();

	return bResult;
}

BOOL CNCViewGL::GetClipDepthMill(void)
{
#ifdef _DEBUG
	CMagaDbg	dbg("GetClipDepthMill()\nStart");
#endif
	int			i, j, n, nn, nSize;
	GLint		viewPort[4];
	GLdouble	mvMatrix[16], pjMatrix[16],
				wx1, wy1, wz1, wx2, wy2, wz2;
	GLfloat		fz;
	GLfloat*	pfDepth = NULL;		// ���߽�l�擾�z��ꎞ�̈�
	GLfloat*	pfXYZ = NULL;		// �ϊ����ꂽܰ��ލ��W
	GLfloat*	pfNOR = NULL;		// �@���޸��

	::glGetIntegerv(GL_VIEWPORT, viewPort);
	::glGetDoublev (GL_MODELVIEW_MATRIX,  mvMatrix);
	::glGetDoublev (GL_PROJECTION_MATRIX, pjMatrix);

	// ��`�̈���߸�ٍ��W�ɕϊ�
	::gluProject(m_rcDraw.left,  m_rcDraw.top,    0.0, mvMatrix, pjMatrix, viewPort,
		&wx1, &wy1, &wz1);
	::gluProject(m_rcDraw.right, m_rcDraw.bottom, 0.0, mvMatrix, pjMatrix, viewPort,
		&wx2, &wy2, &wz2);

	m_icx = (int)(wx2 - wx1);
	m_icy = (int)(wy2 - wy1);
	if ( m_icx<=0 || m_icy<=0 )
		return FALSE;

#ifdef _DEBUG
	dbg.printf("left,  top   =(%f, %f)", m_rcDraw.left,  m_rcDraw.top);
	dbg.printf("right, bottom=(%f, %f)", m_rcDraw.right, m_rcDraw.bottom);
	dbg.printf("wx1,   wy1   =(%f, %f)", wx1, wy1);
	dbg.printf("wx2,   wy2   =(%f, %f)", wx2, wy2);
	dbg.printf("m_icx=%d m_icy=%d", m_icx, m_icy);
#endif

	try {
		// �̈�m��
		nSize = m_icx * m_icy;
		pfDepth = new GLfloat[nSize];
		nSize *= NCXYZ;					// ����ȍ~�R�{����
		pfXYZ   = new GLfloat[nSize*2];
		pfNOR   = new GLfloat[nSize*2];
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		if ( pfDepth )
			delete[]	pfDepth;
		if ( pfXYZ )
			delete[]	pfXYZ;
		if ( pfNOR )
			delete[]	pfNOR;
		return FALSE;
	}

	// �ײ��ė̈�����߽�l���擾�i�߸�ْP�ʁj
#ifdef _DEBUG
	DWORD	t1 = ::timeGetTime();
#endif
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
	for ( j=0, n=0, nn=0; j<m_icy; ++j ) {
		for ( i=0; i<m_icx; ++i, ++n, nn+=NCXYZ ) {
			::gluUnProject(i+wx1, j+wy1, pfDepth[n],
					mvMatrix, pjMatrix, viewPort,
					&wx2, &wy2, &wz2);	// ����قǒx���Ȃ��̂Ŏ���ϊ��͒��~
			fz = (GLfloat)( (pfDepth[n]==0.0) ?	// ���߽�l�������l(��`�͈͓��Ő؍�ʂłȂ�)�Ȃ�
				m_rcDraw.high :		// ܰ���`��ʍ��W
				wz2 );				// �ϊ����W
			// ܰ��ލ��W
			pfXYZ[nn+NCA_X] = (GLfloat)wx2;
			pfXYZ[nn+NCA_Y] = (GLfloat)wy2;
			pfXYZ[nn+NCA_Z] = fz;
			// ��̫�Ă̖@���޸��
			pfNOR[nn+NCA_X] = 0.0;
			pfNOR[nn+NCA_Y] = 0.0;
			pfNOR[nn+NCA_Z] = 1.0;
		}
	}
#ifdef _DEBUG
	DWORD	t3 = ::timeGetTime();
	dbg.printf( "AddMatrix1=%d[ms]", t3 - t2 );
#endif

	// ��ʗp���W�̓o�^
	fz = (GLfloat)m_rcDraw.low;	// m_rcDraw.low���W�ŕ~���l�߂�
	for ( j=0, n=0; j<m_icy; ++j ) {
		for ( i=0; i<m_icx; ++i, n+=NCXYZ, nn+=NCXYZ ) {
			pfXYZ[nn+NCA_X] = pfXYZ[n+NCA_X];	// ��ʂŏ�������
			pfXYZ[nn+NCA_Y] = pfXYZ[n+NCA_Y];	// �ϊ���̍��W���ė��p
			pfXYZ[nn+NCA_Z] = fz;
			pfNOR[nn+NCA_X] = 0.0;
			pfNOR[nn+NCA_Y] = 0.0;
			pfNOR[nn+NCA_Z] = -1.0;
		}
	}
#ifdef _DEBUG
	DWORD	t4 = ::timeGetTime();
	dbg.printf( "AddMatrix2=%d[ms]", t4 - t3 );
#endif

#ifdef _DEBUG_FILEOUT_
	CStdioFile	dbg_f ("C:\\Users\\magara\\Documents\\tmp\\depth_f.csv", CFile::typeText|CFile::modeCreate|CFile::modeWrite);
	CStdioFile	dbg_fx("C:\\Users\\magara\\Documents\\tmp\\depth_x.csv", CFile::typeText|CFile::modeCreate|CFile::modeWrite);
	CStdioFile	dbg_fy("C:\\Users\\magara\\Documents\\tmp\\depth_y.csv", CFile::typeText|CFile::modeCreate|CFile::modeWrite);
	CStdioFile	dbg_fz("C:\\Users\\magara\\Documents\\tmp\\depth_z.csv", CFile::typeText|CFile::modeCreate|CFile::modeWrite);
	CString		r,
				s, sx, sy, sz;
	for ( j=0, n=0; j<m_icy; j++ ) {
		s.Empty();
		sx.Empty();	sy.Empty();	sz.Empty();
		for ( i=0; i<m_icx; i++, n++ ) {
//			if ( pfDepth[n] != 0.0 ) {
				if ( !s.IsEmpty() ) {
					s  += ", ";
//				if ( !sx.IsEmpty() ) {
					sx += ", ";	sy += ", ";	sz += ", ";
				}
				r.Format(IDS_MAKENCD_FORMAT, pfDepth[n]);
				s += r;
				r.Format(IDS_MAKENCD_FORMAT, pfXYZ[n*NCXYZ+NCA_X]);
				sx += r;
				r.Format(IDS_MAKENCD_FORMAT, pfXYZ[n*NCXYZ+NCA_Y]);
				sy += r;
				r.Format(IDS_MAKENCD_FORMAT, pfXYZ[n*NCXYZ+NCA_Z]);
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

	delete[]	pfDepth;

	// ���_�z���ޯ̧��޼ު�Đ���
	BOOL	bResult = CreateVBOMill(pfXYZ, pfNOR);

	delete[]	pfXYZ;
	delete[]	pfNOR;

	return bResult;
}

BOOL CNCViewGL::CreateVBOMill(const GLfloat* pfXYZ, GLfloat* pfNOR)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CreateVBOMill()", DBG_BLUE);
#endif

	HANDLE*					pThread;
	LPCREATEELEMENTPARAM	pParam;

	pThread	= new HANDLE[g_nProcesser];
	pParam	= new CREATEELEMENTPARAM[g_nProcesser];
	int		i, n = m_icy / g_nProcesser;	// 1CPU������̏�����
	BOOL	bResult = TRUE;

	// CPU���گ�ދN��
	for ( i=0; i<g_nProcesser-1; ++i ) {
#ifdef _DEBUG
		pParam[i].dbgThread = i;
#endif
		pParam[i].pfXYZ = pfXYZ;		// const
		pParam[i].pfNOR = pfNOR;
		pParam[i].bResult = TRUE;
		pParam[i].cx = m_icx;
		pParam[i].cy = m_icy;
		pParam[i].cs = n * i;
		pParam[i].ce = min(n*i+n, m_icy-1);
		pParam[i].h  = (GLfloat)m_rcDraw.high;
		pParam[i].l  = (GLfloat)m_rcDraw.low;
		pParam[i].vElementCut.reserve(n+1);
		pParam[i].vElementWrk.reserve((n+1)*2);
		pThread[i] = ::CreateThread(NULL, 0, CallCreateElementThread, &pParam[i], 0, NULL);
	}
#ifdef _DEBUG
	pParam[i].dbgThread = i;
#endif
	pParam[i].pfXYZ = pfXYZ;
	pParam[i].pfNOR = pfNOR;
	pParam[i].bResult = TRUE;
	pParam[i].cx = m_icx;
	pParam[i].cy = m_icy;
	pParam[i].cs = n * i;
	pParam[i].ce = m_icy - 1;	// �[�����c��Ȃ��悤��
	pParam[i].h  = (GLfloat)m_rcDraw.high;
	pParam[i].l  = (GLfloat)m_rcDraw.low;
	pParam[i].vElementCut.reserve(n+1);
	pParam[i].vElementWrk.reserve((n+1)*2);
	pThread[i] = ::CreateThread(NULL, 0, CallCreateElementThread, &pParam[i], 0, NULL);

	::WaitForMultipleObjects(g_nProcesser, pThread, TRUE, INFINITE);
	for ( i=0, n=1; i<g_nProcesser; ++i ) {
		::CloseHandle(pThread[i]);
		if ( !pParam[i].bResult )
			n = 0;
	}

	// ܰ����ʂ͖@���޸�ق̍X�V������
	// �؍�ʏ����Ƃ̔r��������l��
	if ( !n || !CreateElementSide(&pParam[0]) ) {
		delete[]	pThread;
		delete[]	pParam;
		return FALSE;
	}

	::glGetError();		// �װ���ׯ��

	// ���_�z���GPU��؂ɓ]��
	if ( m_nVertexID > 0 )
		::glDeleteBuffersARB(1, &m_nVertexID);
	::glGenBuffersARB(1, &m_nVertexID);
	::glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_nVertexID);
	::glBufferDataARB(GL_ARRAY_BUFFER_ARB,
			m_icx*m_icy*NCXYZ*2*sizeof(GLfloat), pfXYZ,
			GL_STATIC_DRAW_ARB);
	if ( ::glGetError() != GL_NO_ERROR ) {	// GL_OUT_OF_MEMORY
		::glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
		ClearVBO();
		delete[]	pThread;
		delete[]	pParam;
		AfxMessageBox(IDS_ERR_OUTOFVRAM, MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}

	// �@���޸�ق�GPU��؂ɓ]��
	if ( m_nNormalID > 0 )
		::glDeleteBuffersARB(1, &m_nNormalID);
	::glGenBuffersARB(1, &m_nNormalID);
	::glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_nNormalID);
	::glBufferDataARB(GL_ARRAY_BUFFER_ARB,
			m_icx*m_icy*NCXYZ*2*sizeof(GLfloat), pfNOR,
			GL_STATIC_DRAW_ARB);
	if ( ::glGetError() != GL_NO_ERROR ) {
		::glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
		ClearVBO();
		delete[]	pThread;
		delete[]	pParam;
		AfxMessageBox(IDS_ERR_OUTOFVRAM, MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}
	::glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);

	// ���_���ޯ����GPU��؂ɓ]��
	if ( m_pGenBuf ) {
		::glDeleteBuffersARB(m_vVertexWrk.size()+m_vVertexCut.size(), m_pGenBuf);
		delete[]	m_pGenBuf;
		m_pGenBuf = NULL;
	}

	m_vVertexWrk.clear();
	m_vVertexCut.clear();

	try {
		size_t	ii, jj = 0,		// �x���h�~
				nCutSize = 0, nWrkSize = 0,
				nElement, nSize;
		for ( i=0; i<g_nProcesser; i++ ) {
			nCutSize += pParam[i].vElementCut.size();
			nWrkSize += pParam[i].vElementWrk.size();
		}

		m_pGenBuf = new GLuint[nWrkSize+nCutSize];
		::glGenBuffersARB(nWrkSize+nCutSize, m_pGenBuf);

		m_vVertexWrk.reserve(nWrkSize+1);
		m_vVertexCut.reserve(nCutSize+1);

#ifdef _DEBUG
		int		dbgTriangleWrk = 0, dbgTriangleCut = 0;
#endif
		// ܰ���`�p
		for ( i=0; i<g_nProcesser; ++i ) {
			nSize = pParam[i].vElementWrk.size();
			for ( ii=0; ii<nSize; ++ii, ++jj ) {
				nElement = pParam[i].vElementWrk[ii].size();
				::glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, m_pGenBuf[jj]);
				::glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB,
					nElement*sizeof(GLuint), &(pParam[i].vElementWrk[ii][0]),
					GL_STATIC_DRAW_ARB);
				if ( ::glGetError() != GL_NO_ERROR ) {
					::glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
					ClearVBO();
					delete[]	pThread;
					delete[]	pParam;
					AfxMessageBox(IDS_ERR_OUTOFVRAM, MB_OK|MB_ICONEXCLAMATION);
					return FALSE;
				}
				m_vVertexWrk.push_back(nElement);
#ifdef _DEBUG
				dbgTriangleWrk += nElement;
#endif
			}
		}
		// �؍�ʗp
		for ( i=0; i<g_nProcesser; ++i ) {
			nSize = pParam[i].vElementCut.size();
			for ( ii=0; ii<nSize; ++ii, ++jj ) {
				nElement = pParam[i].vElementCut[ii].size();
				::glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, m_pGenBuf[jj]);
				::glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB,
					nElement*sizeof(GLuint), &(pParam[i].vElementCut[ii][0]),
					GL_STATIC_DRAW_ARB);
				if ( ::glGetError() != GL_NO_ERROR ) {
					::glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
					ClearVBO();
					delete[]	pThread;
					delete[]	pParam;
					AfxMessageBox(IDS_ERR_OUTOFVRAM, MB_OK|MB_ICONEXCLAMATION);
					return FALSE;
				}
				m_vVertexCut.push_back(nElement);
#ifdef _DEBUG
				dbgTriangleCut += nElement;
#endif
			}
		}

		::glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);

#ifdef _DEBUG
		dbg.printf("VertexCount=%d size=%d",
			m_icx*m_icy*2, m_icx*m_icy*NCXYZ*2*sizeof(GLfloat));
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

	return bResult;
}

DWORD WINAPI CallCreateElementThread(LPVOID pVoid)
{
	LPCREATEELEMENTPARAM pParam = reinterpret_cast<LPCREATEELEMENTPARAM>(pVoid);
#ifdef _DEBUG
	CMagaDbg	dbg(DBG_BLUE);
	dbg.printf("CallCreateElementThread() ThreadID=%d s=%d e=%d Start!",
		pParam->dbgThread, pParam->cs, pParam->ce);
	DWORD	t1 = ::timeGetTime(), t2, tt = 0;
#endif

	try {
		// �؍�ʂ̒��_���ޯ������
		CreateElementCut(pParam);
#ifdef _DEBUG
		t2 = ::timeGetTime();
		tt += t2 - t1;
		dbg.printf("--- ThreadID=%d CreateElementCut() End %d[ms]",
			pParam->dbgThread, t2 - t1);
		t1 = t2;
#endif
		// ܰ���ʂ̒��_���ޯ������
		CreateElementTop(pParam);
#ifdef _DEBUG
		t2 = ::timeGetTime();
		tt += t2 - t1;
		dbg.printf("--- ThreadID=%d CreateElementTop() End %d[ms]",
			pParam->dbgThread, t2 - t1);
		t1 = t2;
#endif
		// ܰ���ʂ̒��_���ޯ������
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
	// ܰ���ʏ���
	int			i, j;
	BOOL		bReverse, bSingle;
	GLuint		n0, n1,	nn;
	UINT		k0, k1;
	CVElement	vElement;

	vElement.reserve(pParam->cx * 2 + 1);

	// �؍�ʈȊO�� m_rcDraw.high �������Ă���̂�
	// ���������ۂ��̔��f��OK
	for ( j=pParam->cs; j<pParam->ce; ++j ) {
		bReverse = bSingle = FALSE;
		vElement.clear();
		for ( i=0; i<pParam->cx; ++i ) {
			n0 =  j   *pParam->cx+i;
			n1 = (j+1)*pParam->cx+i;
			k0 = n0*NCXYZ + NCA_Z;
			k1 = n1*NCXYZ + NCA_Z;
			if ( pParam->pfXYZ[k0] != pParam->h ) {
				if ( pParam->pfXYZ[k1] != pParam->h ) {
					// k0:�~, k1:�~
					if ( vElement.size() > 3 )
						pParam->vElementWrk.push_back(vElement);
					bReverse = bSingle = FALSE;
					vElement.clear();
				}
				else {
					// k0:�~, k1:��
					if ( bSingle ) {
						// �Е��������A������Ȃ炻����break
						if ( vElement.size() > 3 )
							pParam->vElementWrk.push_back(vElement);
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
			else if ( pParam->pfXYZ[k1] != pParam->h ) {
				// k0:��, k1:�~
				if ( bSingle ) {
					if ( vElement.size() > 3 )
						pParam->vElementWrk.push_back(vElement);
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
						pParam->vElementWrk.push_back(vElement);
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
			pParam->vElementWrk.push_back(vElement);
	}
}

void CreateElementBtm(LPCREATEELEMENTPARAM pParam)
{
	// ܰ���ʏ���
	int			i, j, cxcy = pParam->cx * pParam->cy;
	BOOL		bReverse, bSingle;
	GLuint		n0, n1,	nn;
	UINT		k0, k1;
	CVElement	vElement;

	vElement.reserve(pParam->cx * 2 + 1);

	for ( j=pParam->cs; j<pParam->ce; ++j ) {
		bReverse = bSingle = FALSE;
		vElement.clear();
		for ( i=0; i<pParam->cx; ++i ) {
			n0 =  j   *pParam->cx+i;
			n1 = (j+1)*pParam->cx+i;
			k0 = n0*NCXYZ + NCA_Z;	// ��ʂ����߽���ŕ`�攻�f
			k1 = n1*NCXYZ + NCA_Z;
			n0 += cxcy;				// ��ʍ��W�ԍ�
			n1 += cxcy;
			if ( pParam->pfXYZ[k0] < pParam->l ) {
				if ( pParam->pfXYZ[k1] < pParam->l ) {
					// k0:�~, k1:�~
					if ( vElement.size() > 3 )
						pParam->vElementWrk.push_back(vElement);
					bReverse = bSingle = FALSE;
					vElement.clear();
				}
				else {
					// k0:�~, k1:��
					if ( bSingle ) {
						// �Е��������A������Ȃ炻����break
						if ( vElement.size() > 3 )
							pParam->vElementWrk.push_back(vElement);
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
			else if ( pParam->pfXYZ[k1] < pParam->l ) {
				// k0:��, k1:�~
				if ( bSingle ) {
					if ( vElement.size() > 3 )
						pParam->vElementWrk.push_back(vElement);
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
						pParam->vElementWrk.push_back(vElement);
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
			pParam->vElementWrk.push_back(vElement);
	}
}

void CreateElementCut(LPCREATEELEMENTPARAM pParam)
{
	// �؍�ʁi��ʁE���ʌ��p�j
	int			i, j;
	BOOL		bWorkrct,	// �O��~�~���ǂ����𔻒f�����׸�
				bThrough;	// �O�񁢁��@�@�V
	GLuint		n0, n1;
	UINT		k0, k1, kk0, kk1;
	GLfloat		k0z, k1z;
	float		q;
	CVElement	vElement;

	vElement.reserve(pParam->cx * 2 + 1);

	// ���F�؍�ʁC���F�ђʁC�~�Fܰ����
	for ( j=pParam->cs; j<pParam->ce; ++j ) {
		bWorkrct = bThrough = FALSE;
		vElement.clear();
		for ( i=0; i<pParam->cx; ++i ) {
			n0  =  j   *pParam->cx+i;
			n1  = (j+1)*pParam->cx+i;
			k0  = n0*NCXYZ;
			k1  = n1*NCXYZ;
			k0z = pParam->pfXYZ[k0+NCA_Z];
			k1z = pParam->pfXYZ[k1+NCA_Z];
			if ( k0z >= pParam->h ) {
				if ( k1z >= pParam->h ) {
					// k0:�~, k1:�~
					if ( bWorkrct ) {
						// �O��� k0:�~, k1:�~
						if ( vElement.size() > 3 ) {
							// break
							pParam->vElementCut.push_back(vElement);
							// �O��̖@����������(�I�_)�ɕύX
							pParam->pfNOR[kk0+NCA_X] = -1.0;
							pParam->pfNOR[kk0+NCA_Z] = 0;
							pParam->pfNOR[kk1+NCA_X] = -1.0;
							pParam->pfNOR[kk1+NCA_Z] = 0;
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
						pParam->pfNOR[kk0+NCA_X] = 1.0;
						pParam->pfNOR[kk0+NCA_Z] = 0;
						pParam->pfNOR[kk1+NCA_X] = 1.0;
						pParam->pfNOR[kk1+NCA_Z] = 0;
					}
					// k0��Y����������̖@����
					pParam->pfNOR[k0+NCA_Y] = 1.0;
					pParam->pfNOR[k0+NCA_Z] = 0;
					if ( k1z < pParam->l ) {
						// k1�i���F�؂�ڑ��j��Y�����������̖@��
						pParam->pfNOR[k1+NCA_Y] = -1.0;
						pParam->pfNOR[k1+NCA_Z] = 0;
					}
					bWorkrct = FALSE;
				}
				bThrough = FALSE;
			}
			else if ( k0z < pParam->l ) {
				// k0:��
				if ( bWorkrct ) {
					// �O�� k0:�~, k1:�~
					// �O��̖@�����E����(�n�_)�ɕύX
					pParam->pfNOR[kk0+NCA_X] = 1.0;
					pParam->pfNOR[kk0+NCA_Z] = 0;
					pParam->pfNOR[kk1+NCA_X] = 1.0;
					pParam->pfNOR[kk1+NCA_Z] = 0;
				}
				bWorkrct = FALSE;
				if ( k1z < pParam->l ) {
					// k0:��, k1:��
					if ( bThrough ) {
						// �O��� k0:��, k1:��
						if ( vElement.size() > 3 ) {
							// break
							pParam->vElementCut.push_back(vElement);
							// �O��̖@����������(�؂��)�ɕύX
							pParam->pfNOR[kk0+NCA_X] = -1.0;
							pParam->pfNOR[kk0+NCA_Z] = 0;
							pParam->pfNOR[kk1+NCA_X] = -1.0;
							pParam->pfNOR[kk1+NCA_Z] = 0;
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
					pParam->pfNOR[k0+NCA_Y] = -1.0;
					pParam->pfNOR[k0+NCA_Z] = 0;
					pParam->pfNOR[k1+NCA_Y] = 1.0;
					pParam->pfNOR[k1+NCA_Z] = 0;
				}
			}
			else {
				// k0:��
				if ( bWorkrct ) {
					// �O�� k0:�~, k1:�~
					// �O��̖@�����E�����ɕύX
					pParam->pfNOR[kk0+NCA_X] = 1.0;
					pParam->pfNOR[kk0+NCA_Z] = 0;
					pParam->pfNOR[kk1+NCA_X] = 1.0;
					pParam->pfNOR[kk1+NCA_Z] = 0;
				}
				if ( bThrough ) {
					// �O�� k0:��, k1:��
					// �O��̖@�����������ɕύX
					pParam->pfNOR[kk0+NCA_X] = -1.0;
					pParam->pfNOR[kk0+NCA_Z] = 0;
					pParam->pfNOR[kk1+NCA_X] = -1.0;
					pParam->pfNOR[kk1+NCA_Z] = 0;
				}
				bWorkrct = bThrough = FALSE;
				if ( k1z >= pParam->h ) {
					// k0:��, k1:�~
					pParam->pfNOR[k1+NCA_Y] = -1.0;		// �������̖@��
					pParam->pfNOR[k1+NCA_Z] = 0;
				}
				else if ( k1z < pParam->l ) {
					// k0:��, k1:��
					pParam->pfNOR[k1+NCA_Y] = 1.0;		// ������̖@��
					pParam->pfNOR[k1+NCA_Z] = 0;
				}
				else {
					// YZ���ʂł̖@���޸�ٌv�Z
					q = atan2f(k1z - k0z, 
						pParam->pfXYZ[k1+NCA_Y] - pParam->pfXYZ[k0+NCA_Y]);
				//	CPointD	pt(0, 1);
				//	pt.RoundPoint(q);
				//	pParam->pfNOR[k0+NCA_X] = pt.x;
				//	pParam->pfNOR[k0+NCA_Z] = pt.y;
					pParam->pfNOR[k1+NCA_Y] = -sinf(q);	// ���̊ȗ���
					pParam->pfNOR[k1+NCA_Z] =  cosf(q);
				}
				// k0�ɑ΂���@���v�Z
				if ( i>0 && pParam->pfXYZ[kk0+NCA_Z] >= pParam->l ) {
					// �O��̍��W�Ƃ̊p�x������
					// XZ���ʂł̖@���޸�ق��v�Z
					q = atan2f(k0z - pParam->pfXYZ[kk0+NCA_Z],
						pParam->pfXYZ[k0+NCA_X] - pParam->pfXYZ[kk0+NCA_X]);
					pParam->pfNOR[k0+NCA_X] = -sinf(q);
					pParam->pfNOR[k0+NCA_Z] =  cosf(q);
				}
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
			pParam->vElementCut.push_back(vElement);
	}
}

BOOL CreateElementSide(LPCREATEELEMENTPARAM pParam)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CreateElementSide()\nStart", DBG_BLUE);
	DWORD	t1 = ::timeGetTime();
#endif

	int			i, j;
	GLuint		n0, n1,
				nn = pParam->cy * pParam->cx;	// ��ʍ��W�ւ̵̾��
	UINT		k0, k1;
	GLfloat		nor = -1.0;
	CVElement	vElement;

	vElement.reserve(pParam->cx * 2 + 1);

	try {
		// ܰ���`���ʁiX������O�Ɖ��j
		for ( j=0; j<pParam->cy; j+=pParam->cy-1 ) {	// 0��m_icy-1
			vElement.clear();
			for ( i=0; i<pParam->cx; ++i ) {
				n0 = j*pParam->cx+i;	// ��ʍ��W
				n1 = nn + n0;			// ��ʍ��W
				k0 = n0*NCXYZ;
				k1 = n1*NCXYZ;
				if ( pParam->l >= pParam->pfXYZ[k0+NCA_Z] ) {
					if ( vElement.size() > 3 )
						pParam->vElementWrk.push_back(vElement);
					vElement.clear();
				}
				else {
					vElement.push_back(n0);
					vElement.push_back(n1);
				}
				// �@���޸�ق̏C��
				pParam->pfNOR[k0+NCA_Y] = nor;	// ��or�������̖@��
				pParam->pfNOR[k0+NCA_Z] = 0;
				pParam->pfNOR[k1+NCA_Y] = nor;
				pParam->pfNOR[k1+NCA_Z] = 0;
			}
			if ( vElement.size() > 3 )
				pParam->vElementWrk.push_back(vElement);
			nor *= -1.0;	// �������]
		}
		// ܰ���`���ʁiY�������ƉE�j
		for ( i=0; i<pParam->cx; i+=pParam->cx-1 ) {
			vElement.clear();
			for ( j=0; j<pParam->cy; ++j ) {
				n0 = j*pParam->cx+i;
				n1 = nn + n0;
				k0 = n0*NCXYZ;
				k1 = n1*NCXYZ;
				if ( pParam->l >= pParam->pfXYZ[k0+NCA_Z] ) {
					if ( vElement.size() > 3 )
						pParam->vElementWrk.push_back(vElement);
					vElement.clear();
				}
				else {
					vElement.push_back(n0);
					vElement.push_back(n1);
				}
				pParam->pfNOR[k0+NCA_X] = nor;	// ��or�E�����̖@��
				pParam->pfNOR[k0+NCA_Z] = 0;
				pParam->pfNOR[k1+NCA_X] = nor;
				pParam->pfNOR[k1+NCA_Z] = 0;
			}
			if ( vElement.size() > 3 )
				pParam->vElementWrk.push_back(vElement);
			nor *= -1.0;
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
//	�ײ����H�̉~������ݸ�
/////////////////////////////////////////////////////////////////////////////

BOOL CNCViewGL::GetClipDepthCylinder(void)
{
#ifdef _DEBUG
	CMagaDbg	dbg("GetClipDepthCylinder()\nStart");
#endif
	int			i, j, n, nn, pr, px, py, nSize;
	double		q, dCylinderD, dCylinderH;
	CPoint3D	ptCylinderOffset;
	GLint		viewPort[4];
	GLdouble	mvMatrix[16], pjMatrix[16],
				wx1, wy1, wz1, wx2, wy2, wz2;
	GLfloat		fz;
	GLfloat*	pfDepth = NULL;		// ���߽�l�擾�z��ꎞ�̈�
	GLfloat*	pfXYZ = NULL;		// �ϊ����ꂽܰ��ލ��W
	GLfloat*	pfNOR = NULL;		// �@���޸��

	::glGetIntegerv(GL_VIEWPORT, viewPort);
	::glGetDoublev (GL_MODELVIEW_MATRIX,  mvMatrix);
	::glGetDoublev (GL_PROJECTION_MATRIX, pjMatrix);

	// ��`�̈���߸�ٍ��W�ɕϊ�
	::gluProject(m_rcDraw.left,  m_rcDraw.top,    0.0, mvMatrix, pjMatrix, viewPort,
		&wx1, &wy1, &wz1);
	::gluProject(m_rcDraw.right, m_rcDraw.bottom, 0.0, mvMatrix, pjMatrix, viewPort,
		&wx2, &wy2, &wz2);

	m_icx = (int)(wx2 - wx1);
	m_icy = (int)(wy2 - wy1);
	if ( m_icx<=0 || m_icy<=0 )
		return FALSE;

#ifdef _DEBUG
	dbg.printf("left,  top   =(%f, %f)", m_rcDraw.left,  m_rcDraw.top);
	dbg.printf("right, bottom=(%f, %f)", m_rcDraw.right, m_rcDraw.bottom);
	dbg.printf("wx1,   wy1   =(%f, %f)", wx1, wy1);
	dbg.printf("wx2,   wy2   =(%f, %f)", wx2, wy2);
	dbg.printf("m_icx=%d m_icy=%d", m_icx, m_icy);
#endif

	// �~�����擾
	tie(dCylinderD, dCylinderH, ptCylinderOffset) = GetDocument()->GetCylinderData();
	pr = m_icx / 2;		// ���a(�߸�ْP��[����])

	try {
		// �̈�m��
		pfDepth = new GLfloat[m_icx * m_icy];
		nSize	= ARCCOUNT * pr * NCXYZ * 2;
		pfXYZ   = new GLfloat[nSize];
		pfNOR   = new GLfloat[nSize];
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		if ( pfDepth )
			delete[]	pfDepth;
		if ( pfXYZ )
			delete[]	pfXYZ;
		if ( pfNOR )
			delete[]	pfNOR;
		return FALSE;
	}

	// �ײ��ė̈�����߽�l���擾�i�߸�ْP�ʁj
#ifdef _DEBUG
	DWORD	t1 = ::timeGetTime();
#endif
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
	for ( i=0, nn=0; i<pr; ++i ) {
		for ( j=0, q=0; j<ARCCOUNT; ++j, q+=ARCSTEP, nn+=NCXYZ ) {
			px = (int)(i*cos(q))+pr;
			py = (int)(i*sin(q))+pr;
			n = py*m_icx + px;
			::gluUnProject(px, py, pfDepth[n],
					mvMatrix, pjMatrix, viewPort,
					&wx2, &wy2, &wz2);
			fz = (GLfloat)( (pfDepth[n]==0.0) ?	// ���߽�l�������l(��`�͈͓��Ő؍�ʂłȂ�)�Ȃ�
				m_rcDraw.high :		// ܰ���`��ʍ��W
				wz2 );				// �ϊ����W
			// ܰ��ލ��W
			pfXYZ[nn+NCA_X] = (GLfloat)wx2;
			pfXYZ[nn+NCA_Y] = (GLfloat)wy2;
			pfXYZ[nn+NCA_Z] = fz;
			// ��̫�Ă̖@���޸��
			pfNOR[nn+NCA_X] = 0.0;
			pfNOR[nn+NCA_Y] = 0.0;
			pfNOR[nn+NCA_Z] = 1.0;
		}
	}
#ifdef _DEBUG
	DWORD	t3 = ::timeGetTime();
	dbg.printf( "AddMatrix1=%d[ms]", t3 - t2 );
#endif

	// ��ʗp���W�̓o�^
	fz = (GLfloat)m_rcDraw.low;	// m_rcDraw.low���W�ŕ~���l�߂�
	for ( i=0, n=0; i<pr; ++i ) {
		for ( j=0; j<ARCCOUNT; ++j, n+=NCXYZ, nn+=NCXYZ ) {
			pfXYZ[nn+NCA_X] = pfXYZ[n+NCA_X];	// ��ʂŏ�������
			pfXYZ[nn+NCA_Y] = pfXYZ[n+NCA_Y];	// �ϊ���̍��W���ė��p
			pfXYZ[nn+NCA_Z] = fz;
			pfNOR[nn+NCA_X] = 0.0;
			pfNOR[nn+NCA_Y] = 0.0;
			pfNOR[nn+NCA_Z] = -1.0;
		}
	}
#ifdef _DEBUG
	DWORD	t4 = ::timeGetTime();
	dbg.printf( "AddMatrix2=%d[ms]", t4 - t3 );
#endif

	delete[]	pfDepth;

	// ���_�z���ޯ̧��޼ު�Đ���
//	BOOL	bResult = CreateVBOMill(pfXYZ, pfNOR);

	delete[]	pfXYZ;
	delete[]	pfNOR;

//	return bResult;
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
//	���Չ��H��]����ݸ�
/////////////////////////////////////////////////////////////////////////////

BOOL CNCViewGL::CreateLathe(void)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CreateLathe()\nStart");
#endif
	// �޸�ِ����̂��߂̏����ݒ�
	InitialBoxel();
	// ��ʂ����ς��ɕ`��
	::glMatrixMode(GL_PROJECTION);
	::glPushMatrix();
	::glLoadIdentity();
	::glOrtho(m_rcDraw.left, m_rcDraw.right,
		m_rcView.low, m_rcView.high,	// top �� bottom �͎g�p�s��
		m_rcView.low, m_rcView.high);	// m_rcDraw �łͷ�ط�؂Ȃ̂� m_rcView ���g��
	::glMatrixMode(GL_MODELVIEW);

#ifdef _DEBUG
	dbg.printf("(%f,%f)-(%f,%f)", m_rcDraw.left, m_rcView.low, m_rcDraw.right, m_rcView.high);
	dbg.printf("(%f,%f)", m_rcView.low, m_rcView.high);
#endif

	// ���՗pZXܲ԰�̕`��
	::glPushAttrib( GL_LINE_BIT );
	::glLineWidth( 3.0 );	// 1Pixel�ł����߽�l���E���Ȃ���������Ȃ��̂�
	for ( int i=0; i<GetDocument()->GetNCsize(); i++ )
		GetDocument()->GetNCdata(i)->DrawGLLatheFace();
	::glPopAttrib();
	::glFinish();

	// ���߽�l�̎擾
	BOOL	bResult = GetClipDepthLathe();
	if ( !bResult )
		ClearVBO();

	// �I������
	FinalBoxel();

	return bResult;
}

BOOL CNCViewGL::GetClipDepthLathe(void)
{
#ifdef _DEBUG
	CMagaDbg	dbg("GetClipDepthLathe()");
#endif
	int			i, j, ii, jj;
	double		q;
	GLint		viewPort[4];
	GLdouble	mvMatrix[16], pjMatrix[16],
				wx1, wy1, wz1, wx2, wy2, wz2;
	GLfloat		fz, fx, fxb;
	optional<GLfloat>	fzb;
	GLfloat*	pfDepth = NULL;		// ���߽�l�擾�z��ꎞ�̈�
	GLfloat*	pfCylinder = NULL;	// �~���`���W
	GLfloat*	pfNOR = NULL;		// �@���޸��

	::glGetIntegerv(GL_VIEWPORT, viewPort);
	::glGetDoublev (GL_MODELVIEW_MATRIX,  mvMatrix);
	::glGetDoublev (GL_PROJECTION_MATRIX, pjMatrix);

	// ��`�̈�(left/right)���߸�ٍ��W�ɕϊ�
	::gluProject(m_rcDraw.left,  0.0, 0.0, mvMatrix, pjMatrix, viewPort,
		&wx1, &wy1, &wz1);
	::gluProject(m_rcDraw.right, 0.0, 0.0, mvMatrix, pjMatrix, viewPort,
		&wx2, &wy2, &wz2);

	m_icx = (int)(wx2 - wx1);
	m_icy = 0;
	if ( m_icx <= 0 )
		return FALSE;

#ifdef _DEBUG
	dbg.printf("left  -> wx1 = %f -> %f", m_rcDraw.left, wx1);
	dbg.printf("right -> wx2 = %f -> %f", m_rcDraw.right, wx2);
	dbg.printf("wy1 = %f", wy1);
	dbg.printf("m_icx=%d", m_icx);
#endif

	try {
		// �̈�m��
		pfDepth    = new GLfloat[m_icx];
		pfCylinder = new GLfloat[m_icx*(ARCCOUNT+1)*NCXYZ];
		pfNOR      = new GLfloat[m_icx*(ARCCOUNT+1)*NCXYZ];
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		if ( pfDepth )
			delete[]	pfDepth;
		if ( pfCylinder )
			delete[]	pfCylinder;
		if ( pfNOR )
			delete[]	pfNOR;
		return FALSE;
	}

	// �ײ��ė̈�����߽�l���擾�i�߸�ْP�ʁj
#ifdef _DEBUG
	DWORD	t1 = ::timeGetTime();
#endif
	::glPixelStorei(GL_PACK_ALIGNMENT, 1);
	::glReadPixels((GLint)wx1, (GLint)wy1, m_icx, 1,
					GL_DEPTH_COMPONENT, GL_FLOAT, pfDepth);
//	ASSERT( ::glGetError() == GL_NO_ERROR );
//	GLenum glError = ::glGetError();
#ifdef _DEBUG
	DWORD	t2 = ::timeGetTime();
	dbg.printf( "glReadPixels()=%d[ms]", t2 - t1 );
#endif

	// �O�a�؍�ʂ̍��W�o�^
	for ( i=0; i<m_icx; i++ ) {
		::gluUnProject(i+wx1, wy1, pfDepth[i],
				mvMatrix, pjMatrix, viewPort,
				&wx2, &wy2, &wz2);	// ����قǒx���Ȃ��̂Ŏ���ϊ��͒��~
		fz = (GLfloat)fabs( (pfDepth[i]==0.0) ?	// ���߽�l�������l(��`�͈͓��Ő؍�ʂłȂ�)�Ȃ�
				m_rcDraw.high :				// ܰ����a�l
				min(wz2, m_rcDraw.high) );	// �ϊ����W(==���a)��ܰ����a�̏�������
		ii = i * (ARCCOUNT+1);
		// ð�߰��̖@���޸�ق��v�Z
		if ( fzb && fz != *fzb ) {
			q = fz - *fzb;
			fx = (GLfloat)cos( atan2(q, wx2 - fxb) + RAD(90.0) );
		}
		else
			fx = 0;
		// fz �𔼌a�ɉ~���`�̍��W�𐶐�
		for ( j=0, q=0; j<=ARCCOUNT; j++, q+=ARCSTEP ) {
			jj = (ii + j) * NCXYZ;
			// ��̫�Ă̖@���޸��
			pfNOR[jj+NCA_X] = fx;
			pfNOR[jj+NCA_Y] = (GLfloat)cos(q);
			pfNOR[jj+NCA_Z] = (GLfloat)sin(q);
			// ܰ��ލ��W
			pfCylinder[jj+NCA_X] = (GLfloat)wx2;
			pfCylinder[jj+NCA_Y] = fz * pfNOR[jj+NCA_Y];
			pfCylinder[jj+NCA_Z] = fz * pfNOR[jj+NCA_Z];
		}
		// �O��l��ۑ�
		fzb = fz;
		fxb = (GLfloat)wx2;
	}
#ifdef _DEBUG
	DWORD	t3 = ::timeGetTime();
	dbg.printf( "AddMatrix1=%d[ms]", t3 - t2 );
#endif

	// ���_�z���ޯ̧��޼ު�Đ���
	BOOL	bResult = CreateVBOLathe(pfDepth, pfCylinder, pfNOR);

	delete[]	pfDepth;
	delete[]	pfCylinder;
	delete[]	pfNOR;

	return bResult;
}

BOOL CNCViewGL::CreateVBOLathe
	(const GLfloat* pfDepth, const GLfloat* pfCylinder, const GLfloat* pfNOR)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CreateVBOLathe()", DBG_BLUE);
#endif
	int		i, j;
	GLuint	n0, n1;
	vector<CVElement>	vElementWrk,	// ���_�z����ޯ��(�ϒ��Q�����z��)
						vElementCut;
	CVElement	vElement;

	vElementWrk.reserve(m_icx+1);
	vElementCut.reserve(m_icx+1);
	vElement.reserve((ARCCOUNT+1)*2+1);

	// i��i+1�̍��W���~���`�ɂȂ���
	for ( i=0; i<m_icx-1; ++i ) {
		vElement.clear();
		for ( j=0; j<=ARCCOUNT; ++j ) {
			n0 =  i    * (ARCCOUNT+1) + j;
			n1 = (i+1) * (ARCCOUNT+1) + j;
			vElement.push_back(n0);
			vElement.push_back(n1);
		}
		if ( pfDepth[i+1] == 0.0 )	// �؍�ʂ�ܰ��ʂ�
			vElementWrk.push_back(vElement);
		else
			vElementCut.push_back(vElement);
	}

	::glGetError();		// �װ���ׯ��

	// ���_�z���GPU��؂ɓ]��
	if ( m_nVertexID > 0 )
		::glDeleteBuffersARB(1, &m_nVertexID);
	::glGenBuffersARB(1, &m_nVertexID);
	::glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_nVertexID);
	::glBufferDataARB(GL_ARRAY_BUFFER_ARB,
			m_icx*(ARCCOUNT+1)*NCXYZ*sizeof(GLfloat), pfCylinder,
			GL_STATIC_DRAW_ARB);
	if ( ::glGetError() != GL_NO_ERROR ) {	// GL_OUT_OF_MEMORY
		::glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
		ClearVBO();
		AfxMessageBox(IDS_ERR_OUTOFVRAM, MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}

	// �@���޸�ق�GPU��؂ɓ]��
	if ( m_nNormalID > 0 )
		::glDeleteBuffersARB(1, &m_nNormalID);
	::glGenBuffersARB(1, &m_nNormalID);
	::glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_nNormalID);
	::glBufferDataARB(GL_ARRAY_BUFFER_ARB,
			m_icx*(ARCCOUNT+1)*NCXYZ*sizeof(GLfloat), pfNOR,
			GL_STATIC_DRAW_ARB);
	if ( ::glGetError() != GL_NO_ERROR ) {
		::glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
		ClearVBO();
		AfxMessageBox(IDS_ERR_OUTOFVRAM, MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}
	::glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);

	// ���_���ޯ����GPU��؂ɓ]��
	if ( m_pGenBuf ) {
		::glDeleteBuffersARB(m_vVertexWrk.size()+m_vVertexCut.size(), m_pGenBuf);
		delete[]	m_pGenBuf;
		m_pGenBuf = NULL;
	}

	m_vVertexWrk.clear();
	m_vVertexCut.clear();

	try {
		size_t	ii, jj,		// �x���h�~
				nElement,
				nWrkSize = vElementWrk.size(),
				nCutSize = vElementCut.size();

		m_pGenBuf = new GLuint[nWrkSize+nCutSize];
		::glGenBuffersARB(nWrkSize+nCutSize, m_pGenBuf);

#ifdef _DEBUG
		int		dbgTriangleWrk = 0, dbgTriangleCut = 0;
#endif
		// ܰ���`�p
		m_vVertexWrk.reserve(nWrkSize+1);
		for ( ii=jj=0; ii<nWrkSize; ++ii, ++jj ) {
			nElement = vElementWrk[ii].size();
			::glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, m_pGenBuf[jj]);
			::glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB,
				nElement*sizeof(GLuint), &vElementWrk[ii][0],
				GL_STATIC_DRAW_ARB);
			if ( ::glGetError() != GL_NO_ERROR ) {
				::glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
				ClearVBO();
				AfxMessageBox(IDS_ERR_OUTOFVRAM, MB_OK|MB_ICONEXCLAMATION);
				return FALSE;
			}
			m_vVertexWrk.push_back(nElement);
#ifdef _DEBUG
			dbgTriangleWrk += nElement;
#endif
		}
		// �؍�ʗp
		m_vVertexCut.reserve(nCutSize+1);
		for ( ii=0; ii<nCutSize; ++ii, ++jj ) {
			nElement = vElementCut[ii].size();
			::glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, m_pGenBuf[jj]);
			::glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB,
				nElement*sizeof(GLuint), &vElementCut[ii][0],
				GL_STATIC_DRAW_ARB);
			if ( ::glGetError() != GL_NO_ERROR ) {
				::glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
				ClearVBO();
				AfxMessageBox(IDS_ERR_OUTOFVRAM, MB_OK|MB_ICONEXCLAMATION);
				return FALSE;
			}
			m_vVertexCut.push_back(nElement);
#ifdef _DEBUG
			dbgTriangleCut += nElement;
#endif
		}
		::glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);

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
//	ܲԉ��H�\������ݸ�
/////////////////////////////////////////////////////////////////////////////

BOOL CNCViewGL::CreateWire(void)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CreateWire()\nStart");
#endif
	int					i, j, nResult, nVertex = 0;
	double				dLength;
	CNCdata*			pData;
	vector<GLfloat>		vVertex, vNormal;
	vector<CVElement>	vElementCut;
	CVElement			vElement;

	// ��޼ު�Ă̒�����񂪕K�v�Ȃ��߁A�؍펞�Ԍv�Z�گ�ޏI���҂�
	GetDocument()->WaitCalcThread(TRUE);
	m_vWireLength.clear();

	// ܲԉ��H�̍��W�o�^
	for ( i=0; i<GetDocument()->GetNCsize(); i++ ) {
		// �؍�J�n�_�̌���
		for ( ; i<GetDocument()->GetNCsize(); i++ ) {
			pData = GetDocument()->GetNCdata(i);
			if ( pData->GetGtype()==G_TYPE && pData->IsCutCode() ) {
				// �n�_�o�^
				nResult = pData->AddGLWireFirstVertex(vVertex, vNormal);
				if ( nResult < 0 )
					continue;
				for ( j=0; j<nResult; j++ )
					vElement.push_back(nVertex++);	// XY, UV
				break;
			}
		}
		dLength = 0.0;
		// �؍��ް��̒��_�o�^
		for ( ; i<GetDocument()->GetNCsize(); i++ ) {
			pData = GetDocument()->GetNCdata(i);
			nResult = pData->AddGLWireVertex(vVertex, vNormal);
			if ( nResult < 0 ) {
				// glDrawElements break
				vElementCut.push_back(vElement);
				vElement.clear();
				// �؍�W������
				m_vWireLength.push_back(dLength);
				break;	// ���̐؍�J�n�_����
			}
			for ( j=0; j<nResult; j++ )
				vElement.push_back(nVertex++);
			dLength += pData->GetWireObj() ? 
				max(pData->GetCutLength(), pData->GetWireObj()->GetCutLength()) :
				pData->GetCutLength();
		}
	}
	if ( !vElement.empty() ) {
		vElementCut.push_back(vElement);
		vElement.clear();
		m_vWireLength.push_back(dLength);
	}

#ifdef _DEBUG
	dbg.printf("VBO transport Start");
#endif
	// ���_�z���GPU��؂ɓ]��
	if ( m_nVertexID > 0 )
		::glDeleteBuffersARB(1, &m_nVertexID);
	::glGenBuffersARB(1, &m_nVertexID);
	::glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_nVertexID);
	::glBufferDataARB(GL_ARRAY_BUFFER_ARB,
			vVertex.size()*sizeof(GLfloat), &vVertex[0],
			GL_STATIC_DRAW_ARB);
	if ( ::glGetError() != GL_NO_ERROR ) {	// GL_OUT_OF_MEMORY
		::glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
		ClearVBO();
		AfxMessageBox(IDS_ERR_OUTOFVRAM, MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}

	// �@���޸�ق�GPU��؂ɓ]��
	if ( m_nNormalID > 0 )
		::glDeleteBuffersARB(1, &m_nNormalID);
	::glGenBuffersARB(1, &m_nNormalID);
	::glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_nNormalID);
	::glBufferDataARB(GL_ARRAY_BUFFER_ARB,
			vNormal.size()*sizeof(GLfloat), &vNormal[0],
			GL_STATIC_DRAW_ARB);
	if ( ::glGetError() != GL_NO_ERROR ) {
		::glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
		ClearVBO();
		AfxMessageBox(IDS_ERR_OUTOFVRAM, MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}
	::glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);

#ifdef _DEBUG
	dbg.printf("VBO(index) transport Start");
#endif
	// ���_���ޯ����GPU��؂ɓ]��
	if ( m_pGenBuf ) {
		::glDeleteBuffersARB(m_vVertexWrk.size()+m_vVertexCut.size(), m_pGenBuf);
		delete[]	m_pGenBuf;
		m_pGenBuf = NULL;
	}

	m_vVertexWrk.clear();
	m_vVertexCut.clear();

	try {
		size_t	ii,		// �x���h�~
				nElement,
				nCutSize = vElementCut.size();

		m_pGenBuf = new GLuint[nCutSize];
		::glGenBuffersARB(nCutSize, m_pGenBuf);

		m_vVertexCut.reserve(nCutSize+1);
		for ( ii=0; ii<nCutSize; ++ii ) {
			nElement = vElementCut[ii].size();
			::glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, m_pGenBuf[ii]);
			::glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB,
				nElement*sizeof(GLuint), &vElementCut[ii][0],
				GL_STATIC_DRAW_ARB);
			if ( ::glGetError() != GL_NO_ERROR ) {
				::glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
				ClearVBO();
				AfxMessageBox(IDS_ERR_OUTOFVRAM, MB_OK|MB_ICONEXCLAMATION);
				return FALSE;
			}
			m_vVertexCut.push_back(nElement);
		}
		::glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
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
	// ̧�ٖ���UNICODE�ϊ�
	WCHAR	wszFileName[_MAX_PATH];
	::MultiByteToWideChar(CP_ACP, 0, szFileName, -1, wszFileName, _MAX_PATH);

	// �摜�̓ǂݍ���
	Gdiplus::Bitmap	bmp(wszFileName);
	if ( bmp.GetLastStatus() != Gdiplus::Ok ) {
		ClearTexture();
		AfxMessageBox(IDS_ERR_TEXTURE, MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}
	// �摜���߸�ق��擾
	Gdiplus::BitmapData	bmpdata;
	bmp.LockBits(NULL, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &bmpdata);

	// ø����̐���
	::glGenTextures(1, &m_nPictureID);
	::glBindTexture(GL_TEXTURE_2D, m_nPictureID);
	::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_REPEAT);
	// ø������߸�ق���������(2^n �T�C�Y�ȊO�̑Ή�)
	::gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, bmpdata.Width, bmpdata.Height,
		GL_BGRA_EXT, GL_UNSIGNED_BYTE, bmpdata.Scan0);

	// ��n��
	::glBindTexture(GL_TEXTURE_2D, 0);
	bmp.UnlockBits(&bmpdata);

	return TRUE;
}

void CNCViewGL::CreateTextureMill(void)
{
	if ( m_icx<=0 || m_icy<=0 )
		return;

	int			i, j, n,
				nVertex = m_icx*m_icy*2*2,		// (X,Y) ��ʂƒ��
				icx = m_icx-1, icy = m_icy-1;	// ���꒲��
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

	// ��ʗpø������W
	for ( j=0, n=0; j<m_icy; ++j ) {
		ft = (GLfloat)(m_icy-j)/icy;
		for ( i=0; i<m_icx; ++i, n+=2 ) {
			// ø������W(0.0�`1.0)
			pfTEX[n]   = (GLfloat)i/icx;
			pfTEX[n+1] = ft;
		}
	}

	// ��ʗpø������W
	for ( j=0; j<m_icy; ++j ) {
		ft = (GLfloat)j/icy;
		for ( i=0; i<m_icx; ++i, n+=2 ) {
			pfTEX[n]   = (GLfloat)i/icx;
			pfTEX[n+1] = ft;
		}
	}

	// ø������W��GPU��؂ɓ]��
	ASSERT( n == nVertex );
	CreateTexture(nVertex, pfTEX);

	delete[]	pfTEX;
}

void CNCViewGL::CreateTextureLathe(void)
{
	if ( m_icx<=0 )
		return;

	int			i, j, n,
				nVertex = m_icx*(ARCCOUNT+1)*2,	// X�Ɖ~�����W��
				icx = m_icx-1;					// ���꒲��
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

	// ø������W�̊��蓖��
	for ( i=0, n=0; i<m_icx; ++i ) {
		ft = (GLfloat)i/icx;
		for ( j=0; j<=ARCCOUNT; ++j, n+=2 ) {
			// ø������W(0.0�`1.0)
			pfTEX[n]   = (GLfloat)j/ARCCOUNT;
			pfTEX[n+1] = ft;
		}
	}

	// ø������W��GPU��؂ɓ]��
	ASSERT( n == nVertex );
	CreateTexture(nVertex, pfTEX);

	delete[]	pfTEX;
}

void CNCViewGL::CreateTextureWire(void)
{
	int			i, j, n,
				nVertex,		// ���_��
				nResult;
	double		dAccuLength;	// �ݐϒ���
	CNCdata*	pData;
	GLfloat*	pfTEX;

	// ���_���v�Z
	nVertex = accumulate(m_vVertexCut.begin(), m_vVertexCut.end(), 0);
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

	// ø������W�̊��蓖��(CreateWire�Ɠ���ٰ��)
	for ( i=0, j=0, n=0; i<GetDocument()->GetNCsize(); ++i, ++j ) {
		// �؍�J�n�_�̌���
		for ( ; i<GetDocument()->GetNCsize(); ++i ) {
			pData = GetDocument()->GetNCdata(i);
			if ( pData->GetGtype()==G_TYPE && pData->IsCutCode() ) {
				if ( !pData->GetWireObj() )
					continue;
				// �n�_�o�^
				pfTEX[n++] = 0.0;	// XY
				pfTEX[n++] = 0.0;
				pfTEX[n++] = 0.0;	// UV
				pfTEX[n++] = 1.0;
				break;
			}
		}
		dAccuLength = 0.0;
		// �e��޼ު�Ă��Ƃ�ø������W��o�^
		for ( ; i<GetDocument()->GetNCsize(); ++i ) {
			pData = GetDocument()->GetNCdata(i);
			nResult = pData->AddGLWireTexture(n, dAccuLength, m_vWireLength[j], pfTEX);
			if ( nResult < 0 )
				break;	// ���̐؍�J�n�_����
			n += nResult;
		}
	}

	// ø������W��GPU��؂ɓ]��
//	ASSERT( n == nVertex );			// �v�����I�I
	CreateTexture(nVertex, pfTEX);

	delete[]	pfTEX;
}

void  CNCViewGL::CreateTexture(GLsizeiptr n, const GLfloat* pfTEX)
{
	// ø������W��GPU��؂ɓ]��
	if ( m_nTextureID > 0 )
		::glDeleteBuffersARB(1, &m_nTextureID);
	::glGenBuffersARB(1, &m_nTextureID);
	::glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_nTextureID);
	::glBufferDataARB(GL_ARRAY_BUFFER_ARB,
			n*sizeof(GLfloat), pfTEX,
			GL_STATIC_DRAW_ARB);
	if ( ::glGetError() != GL_NO_ERROR ) {	// GL_OUT_OF_MEMORY
		ClearTexture();
		AfxMessageBox(IDS_ERR_OUTOFVRAM, MB_OK|MB_ICONEXCLAMATION);
	}
	::glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
}

void CNCViewGL::ClearVBO(void)
{
	if ( m_nVertexID > 0 )
		::glDeleteBuffersARB(1, &m_nVertexID);
	if ( m_nNormalID > 0 )
		::glDeleteBuffersARB(1, &m_nNormalID);
	m_nVertexID = m_nNormalID = 0;
	if ( m_pGenBuf ) {
		::glDeleteBuffersARB(m_vVertexWrk.size()+m_vVertexCut.size(), m_pGenBuf);
		delete[]	m_pGenBuf;
		m_pGenBuf = NULL;
	}
	m_vVertexWrk.clear();
	m_vVertexCut.clear();
}

void CNCViewGL::ClearTexture(void)
{
	if ( m_nPictureID > 0 )
		::glDeleteTextures(1, &m_nPictureID);
	if ( m_nTextureID > 0 )
		::glDeleteBuffersARB(1, &m_nTextureID);
	m_nPictureID = m_nTextureID = 0;
}

void CNCViewGL::InitialBoxel(void)
{
	// �޸�ِ����̂��߂̏����ݒ�
	::glEnable(GL_DEPTH_TEST);
	::glClearDepth(0.0);		// ��������D�悳���邽�߂����߽�����l
	::glDepthFunc(GL_GREATER);	// ��������D��
	::glClear(GL_DEPTH_BUFFER_BIT);		// ���߽�ޯ̧�̂ݸر
	::glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);	// �װϽ�OFF
}

void CNCViewGL::FinalBoxel(void)
{
	::glMatrixMode(GL_PROJECTION);
	::glPopMatrix();
	::glMatrixMode(GL_MODELVIEW);

	// �ʏ�ݒ�ɖ߂�
	::glClearDepth(1.0);
	::glDepthFunc(GL_LESS);		// �߂�����D��(�ʏ�`��)
	::glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

void CNCViewGL::RenderBack(void)
{
	::glDisable(GL_DEPTH_TEST);	// ���߽ýĖ����ŕ`��

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
//	dVertex[2] = m_rcView.low;
	dVertex[2] = m_rcView.high - NCMIN *2.0;	// ��ԉ�(x2�͵Ϲ)
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

	::glEnable(GL_DEPTH_TEST);
}

void CNCViewGL::RenderAxis(void)
{
	extern	const	PENSTYLE	g_penStyle[];	// ViewOption.cpp
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	double		dLength;
	COLORREF	col;

	::glPushAttrib( GL_LINE_BIT );	// �����
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

	::glPopAttrib();	//	::glDisable(GL_LINE_STIPPLE)�� pop�ɂ��s�v
}

void CNCViewGL::RenderCode(void)
{
	ASSERT( m_pfnDrawProc );
	CNCdata*	pData;

	::glEnable( GL_LINE_STIPPLE );
	// NC�ް��`��
	for ( int i=0; i<GetDocument()->GetNCsize(); i++ ) {
		pData = GetDocument()->GetNCdata(i);
		if ( pData->GetGtype() == G_TYPE ) {
//			pData->DrawGLMillWire();
			(pData->*m_pfnDrawProc)();
		}
	}
	::glDisable( GL_LINE_STIPPLE );
}

CPoint3D CNCViewGL::PtoR(const CPoint& pt)
{
	CPoint3D	ptResult;
	// ���ً�Ԃ̉�]
	ptResult.x = ( 2.0 * pt.x - m_cx ) / m_cx * 0.5;
	ptResult.y = ( m_cy - 2.0 * pt.y ) / m_cy * 0.5;
	double	 d = _hypot( ptResult.x, ptResult.y );
	ptResult.z = cos( (PI/2.0) * min(d, 1.0) );

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
	Invalidate(FALSE);
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
		m_dRoundStep = 180.0 * ptw.hypot();
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
		// ����ݸ�&�ޭ��ݸޕϊ��s��
		SetupViewingTransform();
		Invalidate(FALSE);
		break;
	}

	::wglMakeCurrent( NULL, NULL );
}

void CNCViewGL::DoScale(int nRate)
{
	if ( nRate != 0 ) {
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
#ifdef _DEBUG
		g_dbg.printf("DoScale() ---");
		g_dbg.printf("  (%f,%f)-(%f,%f)", m_rcView.left, m_rcView.top, m_rcView.right, m_rcView.bottom);
		g_dbg.printf("  (%f,%f)", m_rcView.low, m_rcView.high);
#endif
	}

	// MDI�q�ڰт̽ð���ް�ɏ��\��
	static_cast<CNCChild *>(GetParentFrame())->SetFactorInfo(m_dRate/LOGPIXEL, m_strGuide);
}

void CNCViewGL::DoRotation(double dAngle)
{
#ifdef _DEBUG
//	g_dbg.printf("DoRotation() Angle=%f (%f, %f, %f)", dAngle,
//		m_ptRoundBase.x, m_ptRoundBase.y, m_ptRoundBase.z);
#endif
	// ��]��د�������̵݂�޼ު��̫����د���Ɋ|�����킹��
	::glLoadIdentity();
	::glRotated( dAngle, m_ptRoundBase.x, m_ptRoundBase.y, m_ptRoundBase.z );
	::glMultMatrixd( (GLdouble *)m_objectXform );
	::glGetDoublev( GL_MODELVIEW_MATRIX, (GLdouble *)m_objectXform );

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
//	CMagaDbg	dbg("OnDraw()\nStart");
#endif
	ASSERT_VALID(GetDocument());
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();

	// ���ĺ�÷�Ă̊��蓖��
	::wglMakeCurrent( pDC->GetSafeHdc(), m_hRC );

	::glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	// �w�i�̕`��
	RenderBack();

	// ���̕`��
	RenderAxis();

#ifdef _DEBUG_DRAWTEST_
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
#else
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
		::glVertexPointer(NCXYZ, GL_FLOAT, 0, NULL);
		::glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_nNormalID);
		::glNormalPointer(GL_FLOAT, 0, NULL);
		if ( pOpt->GetNCViewFlg(NCVIEWFLG_TEXTURE) && m_nTextureID > 0 ) {
			::glBindTexture(GL_TEXTURE_2D, m_nPictureID);
			::glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			::glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_nTextureID);
			::glTexCoordPointer(2, GL_FLOAT, 0, NULL);
			::glEnable(GL_TEXTURE_2D);
		}
		// ܰ���`
		::glEnable (GL_LIGHT0);
		::glEnable (GL_LIGHT1);
		::glDisable(GL_LIGHT2);
		::glDisable(GL_LIGHT3);
		size_t	i, j;
		for ( i=j=0; i<m_vVertexWrk.size(); i++, j++ ) {
			::glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, m_pGenBuf[j]);
			::glDrawElements(GL_TRIANGLE_STRIP, m_vVertexWrk[i], GL_UNSIGNED_INT, NULL);
		}
		// �؍��
		::glDisable(GL_LIGHT0);
		::glDisable(GL_LIGHT1);
		::glEnable (GL_LIGHT2);
		::glEnable (GL_LIGHT3);
		for ( i=0; i<m_vVertexCut.size(); i++, j++ ) {
			::glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, m_pGenBuf[j]);
			::glDrawElements(GL_TRIANGLE_STRIP, m_vVertexCut[i], GL_UNSIGNED_INT, NULL);
		}
		::glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
		::glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
		if ( pOpt->GetNCViewFlg(NCVIEWFLG_TEXTURE) && m_nTextureID > 0 ) {
			::glBindTexture(GL_TEXTURE_2D, 0);
			::glDisable(GL_TEXTURE_2D);
			::glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		}
		::glDisableClientState(GL_NORMAL_ARRAY);
		::glDisableClientState(GL_VERTEX_ARRAY);
		::glDisable(GL_POLYGON_OFFSET_FILL);
		::glDisable(GL_LIGHTING);
	}
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
	__super::AssertValid();
}

void CNCViewGL::Dump(CDumpContext& dc) const
{
	__super::Dump(dc);
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
#ifdef _DEBUG_FILEOPEN
	g_dbg.printf("CNCViewGL::OnCreate() Start");
#endif
	if ( __super::OnCreate(lpCreateStruct) < 0 )
		return -1;

	// ���د�����̋��E���ق��ڂ�̂ŁCIDC_ARROW �𖾎��I�Ɏw��
	// ���� NCView.cpp �̂悤�� PreCreateWindow() �ł� AfxRegisterWndClass() �łʹװ??
	::SetClassLongPtr(m_hWnd, GCLP_HCURSOR,
		(LONG_PTR)AfxGetApp()->LoadStandardCursor(IDC_ARROW));
#ifdef _DEBUG_FILEOPEN
	g_dbg.printf("CNCViewGL::SetClassLongPtr() End");
#endif

	// OpenGL����������
	CClientDC	dc(this);
	HDC	hDC = dc.GetSafeHdc();

	// OpenGL pixel format �̐ݒ�
    if( !SetupPixelFormat(&dc) ) {
		TRACE0("SetupPixelFormat failed\n");
		return -1;
	}
#ifdef _DEBUG_FILEOPEN
	g_dbg.printf("CNCViewGL::SetupPixelFormat() End");
#endif

	// �����ݸ޺�÷�Ă̍쐬
	if( !(m_hRC = ::wglCreateContext(hDC)) ) {
		TRACE0("wglCreateContext failed\n");
		return -1;
	}
#ifdef _DEBUG_FILEOPEN
	g_dbg.printf("CNCViewGL::wglCreateContext() End");
#endif

	// �����ݸ޺�÷�Ă���Ă����޲���÷�Ăɐݒ�
	if( !::wglMakeCurrent(hDC, m_hRC) ) {
		TRACE0("wglMakeCurrent failed\n");
		return -1;
	}
#ifdef _DEBUG_FILEOPEN
	g_dbg.printf("CNCViewGL::wglMakeCurrent() End");
	DWORD	t1 = ::timeGetTime();
#endif

	// OpenGL Extention �g�p����
	GLenum glewResult = ::glewInit();
	if ( glewResult != GLEW_OK ) {
		TRACE1("glewInit() failed code=%s\n", ::glewGetErrorString(glewResult));
		return -1;
	}
#ifdef _DEBUG_FILEOPEN
	DWORD	t2 = ::timeGetTime();
	g_dbg.printf("CNCViewGL::glewInit() End %d[ms]", t2 - t1);
#endif

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
	if ( m_dRoundStep != 0.0 )
		KillTimer(IDC_OPENGL_DRAGROUND);

	// ��]�s�񓙂�ۑ�
	if ( m_bActive ) {
		CRecentViewInfo* pInfo = GetDocument()->GetRecentViewInfo();
		if ( pInfo ) 
			pInfo->SetViewInfo(m_objectXform, m_rcView, m_ptCenter);
	}

	// OpenGL �㏈��
	CClientDC	dc(this);
	::wglMakeCurrent( dc.GetSafeHdc(), m_hRC );

	// �ި���ڲؽď���
	if ( m_glCode > 0 )
		::glDeleteLists(m_glCode, 1);
	// �ޯ̧��޼ު�Ċ֘A�̍폜
	ClearVBO();
	ClearTexture();

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
		g_dbg.printf("CNCViewGL::OnActivateView() Active");
#endif
		DoScale(0);		// MDI�q�ڰт̽ð���ް�ɏ��\��(����)
	}
#ifdef _DEBUG
	else {
		g_dbg.printf("CNCViewGL::OnActivateView() KillActive");
	}
#endif

	__super::OnActivateView(bActivate, pActivateView, pDeactiveView);
}

LRESULT CNCViewGL::OnUserActivatePage(WPARAM, LPARAM lParam)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CNCViewGL::OnUserActivatePage()\nStart");
	dbg.printf("lParam=%d m_bActive=%d", lParam, m_bActive);
#endif
	if ( !m_bActive ) {
		// m_rcView������
		OnUserViewFitMsg(1, 0);		// glOrtho() �����s���Ȃ�
		// �`��֐��̌���(OnInitialUpdate�����������s�����ǂ�)
		m_pfnDrawProc = GetDocument()->IsDocFlag(NCDOC_WIRE) ?
			&(CNCdata::DrawGLWireWire) : &(CNCdata::DrawGLMillWire);
		// �`�揉���ݒ�
		CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
		pOpt->m_dwUpdateFlg = VIEWUPDATE_ALL;
		UpdateViewOption();
		// ��]�s�񓙂̓ǂݍ��݂ƍX�V
		ClearObjectForm();
		CRecentViewInfo*	pInfo = GetDocument()->GetRecentViewInfo();
		if ( pInfo && !pInfo->GetViewInfo(m_objectXform, m_rcView, m_ptCenter) ) {
			pInfo = AfxGetNCVCApp()->GetDefaultViewInfo();
			if ( pInfo ) {
				CRect3D	rcView;		// dummy
				CPointD	ptCenter;
				pInfo->GetViewInfo(m_objectXform, rcView, ptCenter);
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

	// �����ޭ��Ƃ͈���āA�g�嗦���X�V����K�v�͖���
	// �ð���ް�ւ̕\���� OnActivateView() �ōs��
	return 0;
}

LRESULT CNCViewGL::OnUserViewFitMsg(WPARAM wParam, LPARAM lParam)
{
	extern	const	double	g_dDefaultGuideLength;	// 50.0 (ViewOption.cpp)
#ifdef _DEBUG
	CMagaDbg	dbg("CNCViewGL::OnUserViewFitMsg()\nStart");
	dbg.printf("wParam=%d lParam=%d", wParam, lParam);
#endif
	double		dW, dH, dZ, dLength, d;

	if ( lParam ) {		// from CNCViewTab::OnInitialUpdate()
		// m_dRate �̍X�V(m_cx,m_cy���������l�̂Ƃ��Ɍv�Z)
		dW = fabs(m_rcView.Width());
		dH = fabs(m_rcView.Height());
		if ( dW > dH )
			m_dRate = m_cx / dW;
		else
			m_dRate = m_cy / dH;
		DoScale(0);		// MDI�q�ڰт̽ð���ް�ɏ��\��(����)
	}
	else {
		m_rcView  = GetDocument()->GetMaxRect();
		dW = fabs(m_rcView.Width());
		dH = fabs(m_rcView.Height());
		dZ = fabs(m_rcView.Depth());

		// ��L��`�̕␳(�s���\���̖h�~)
		const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
		if ( dW <= NCMIN ) {
			dLength = pOpt->GetGuideLength(NCA_X);
			if ( dLength == 0.0 )
				dLength = g_dDefaultGuideLength;
			m_rcView.left  = -dLength;
			m_rcView.right =  dLength;
		}
		if ( dH <= NCMIN ) {
			dLength = pOpt->GetGuideLength(NCA_Y);
			if ( dLength == 0.0 )
				dLength = g_dDefaultGuideLength;
			m_rcView.top    = -dLength;
			m_rcView.bottom =  dLength;
		}
		if ( dZ <= NCMIN ) {
			dLength = pOpt->GetGuideLength(NCA_Z);
			if ( dLength == 0.0 )
				dLength = g_dDefaultGuideLength;
			m_rcView.low  = -dLength;
			m_rcView.high =  dLength;
		}
		// ��޼ު�ċ�`��10%(�㉺���E5%����)�傫��
		m_rcView.InflateRect(dW*0.05, dH*0.05);
//		m_rcView.NormalizeRect();
		dW = fabs(m_rcView.Width());
		dH = fabs(m_rcView.Height());
		dZ = fabs(m_rcView.Depth());

		// �ި���ڲ�̱��߸Ĕ䂩�王�쒼���̐ݒ�
		CPointD	pt(m_rcView.CenterPoint());
		if ( dW > dH ) {
			d = dW * m_cy / m_cx / 2.0;
			m_rcView.top    = pt.y - d;
			m_rcView.bottom = pt.y + d;
			m_dRate = m_cx / dW;
		}
		else {
			d = dH * m_cx / m_cy / 2.0;
			m_rcView.left   = pt.x - d;
			m_rcView.right  = pt.x + d;
			m_dRate = m_cy / dH;
		}
		d = max(max(dW, dH), dZ) * 2.0;
		m_rcView.high =  d;		// ��
		m_rcView.low  = -d;		// ��O
//		m_rcView.NormalizeRect();
	}

	if ( !wParam ) {	// from OnUserActivatePage()
		CClientDC	dc(this);
		::wglMakeCurrent( dc.GetSafeHdc(), m_hRC );
		::glMatrixMode( GL_PROJECTION );
		::glLoadIdentity();
		// !!! Home�L�[������ GL_INVALID_OPERATION ���������� !!!
		// !!! ���ʖ�����... !!!
		::glOrtho(m_rcView.left, m_rcView.right, m_rcView.top, m_rcView.bottom,
			m_rcView.low, m_rcView.high);
//		ASSERT( ::glGetError() == GL_NO_ERROR );
//		GLenum glError = ::glGetError();
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

void CNCViewGL::OnUpdateEditCopy(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(FALSE);
}

void CNCViewGL::OnUpdateMoveRoundKey(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(TRUE);
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

void CNCViewGL::OnRoundKey(UINT nID)
{
	if ( m_dRoundStep == 0.0 )
		SetTimer(IDC_OPENGL_DRAGROUND, 150, NULL);

	switch (nID) {
	case ID_VIEW_RUP:
		m_ptRoundBase.SetPoint(1.0, 0.0, 0.0);
		m_dRoundAngle = m_dRoundStep = -1.0;
		break;
	case ID_VIEW_RDW:
		m_ptRoundBase.SetPoint(1.0, 0.0, 0.0);
		m_dRoundAngle = m_dRoundStep = 1.0;
		break;
	case ID_VIEW_RLT:
		m_ptRoundBase.SetPoint(0.0, 1.0, 0.0);
		m_dRoundAngle = m_dRoundStep = -1.0;
		break;
	case ID_VIEW_RRT:
		m_ptRoundBase.SetPoint(0.0, 1.0, 0.0);
		m_dRoundAngle = m_dRoundStep = 1.0;
		break;
	}
}

void CNCViewGL::OnLensKey(UINT nID)
{
	switch ( nID ) {
	case ID_VIEW_FIT:
		if ( m_dRoundStep != 0.0 ) {
			KillTimer(IDC_OPENGL_DRAGROUND);
			m_dRoundStep = 0.0;
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
		DoScale(0);	// MDI�q�ڰт̽ð���ް�ɏ��\��(����)
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
	AfxGetNCVCApp()->SetDefaultViewInfo(m_objectXform);
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
	if ( m_dRoundStep != 0.0 ) {
		KillTimer(IDC_OPENGL_DRAGROUND);
		m_dRoundStep = 0.0;		// KillTimer()�ł�ү���޷���͏����Ȃ�
	}
	BeginTracking( point, TM_SPIN );
}

void CNCViewGL::OnLButtonUp(UINT nFlags, CPoint point)
{
	EndTracking();
	if ( m_dRoundStep != 0.0 ) {
		KillTimer(IDC_OPENGL_DRAGROUND);	// KillTimer() �A�����Ă��G�G�̂���...
		m_dRoundStep = 0.0;
	}
}

void CNCViewGL::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	if ( pOpt->GetNCViewFlg(NCVIEWFLG_SOLIDVIEW) ) {
		pOpt->m_bG00View = !pOpt->m_bG00View;
		Invalidate(FALSE);
	}
}

void CNCViewGL::OnRButtonDown(UINT nFlags, CPoint point)
{
	if ( m_dRoundStep != 0.0 )
		KillTimer(IDC_OPENGL_DRAGROUND);	// �A����]�ꎞ��~

	m_ptDownClick = point;
	BeginTracking( point, TM_PAN );
}

void CNCViewGL::OnRButtonUp(UINT nFlags, CPoint point)
{
	EndTracking();
	if ( m_dRoundStep != 0.0 )
		SetTimer(IDC_OPENGL_DRAGROUND, 150, NULL);	// �A����]�ĊJ

	if ( m_ptDownClick == point )
		__super::OnRButtonUp(nFlags, point);	// ��÷���ƭ��̕\��
}

void CNCViewGL::OnMButtonDown(UINT nFlags, CPoint point)
{
	if ( m_dRoundStep != 0.0 ) {
		KillTimer(IDC_OPENGL_DRAGROUND);
		m_dRoundStep = 0.0;
	}
	BeginTracking( point, TM_SPIN );
}

void CNCViewGL::OnMButtonUp(UINT nFlags, CPoint point)
{
	EndTracking();
	// ��ϲ���ĂŘA����]
#ifdef _DEBUG
	g_dbg.printf("OnMButtonUp() Angle=%f (%f, %f, %f)", m_dRoundStep,
		m_ptRoundBase.x, m_ptRoundBase.y, m_ptRoundBase.z);
#endif
	if ( m_dRoundStep != 0.0 ) {
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
		return;
	}

	__super::OnKeyDown(nChar, nRepCnt, nFlags);
}

BOOL CNCViewGL::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}

void CNCViewGL::OnTimer(UINT nIDEvent) 
{
	if ( m_dRoundStep != 0.0 ) {
#ifdef _DEBUG
		g_dbg.printf("CNCViewGL::OnTimer()");
#endif
//		m_dRoundAngle += m_dRoundStep / 10.0;
		m_dRoundAngle += _copysign(0.05, m_dRoundStep);
		if ( fabs(m_dRoundAngle) > 360.0 )
			m_dRoundAngle -= _copysign(360.0, m_dRoundStep);

		CClientDC	dc(this);
		::wglMakeCurrent( dc.GetSafeHdc(), m_hRC );
		DoRotation(m_dRoundAngle);
		Invalidate(FALSE);
		::wglMakeCurrent( NULL, NULL );
	}

	__super::OnTimer(nIDEvent);
}
