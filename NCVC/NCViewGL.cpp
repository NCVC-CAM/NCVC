// NCViewGL.cpp : �����t�@�C��
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
extern	int				g_nProcesser;	// ��۾����(NCVC.cpp)
extern	const PENSTYLE	g_penStyle[];	// ViewOption.cpp

#define	IsWireMode()	GetDocument()->IsDocFlag(NCDOC_WIRE)
#define	IsLatheMode()	GetDocument()->IsDocFlag(NCDOC_LATHE)

// ��ʕ`����W�����گ�ޗp
struct CREATEBOTTOMVERTEXPARAM {
#ifdef _DEBUG
	int		dbgThread;		// �گ��ID
#endif
	CEvent		evStart,
				evEnd;
	BOOL		bThread,
				bResult;
	CNCDoc*		pDoc;
	size_t		s, e;
	CVBtmDraw	vBD;		// from NCdata.h
	// CEvent ���蓮����Ăɂ��邽�߂̺ݽ�׸�
	CREATEBOTTOMVERTEXPARAM() : evStart(FALSE, TRUE), evEnd(FALSE, TRUE),
		bThread(TRUE), bResult(TRUE)
	{}
};
typedef	CREATEBOTTOMVERTEXPARAM*	LPCREATEBOTTOMVERTEXPARAM ;

#ifdef _WIN64
const size_t	MAXOBJ = 400;	// �P�̽گ�ނ���x�ɏ��������޼ު�Đ�
#else
const size_t	MAXOBJ = 200;
#endif

// ���_�z�񐶐��گ�ޗp
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
	// ���_�z����ޯ��(�ϒ�2�����z��)
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

//	�g�嗦�\���̂��߂̕ϊ��P��
//	1�����������߸�ِ� GetDeviceCaps(LOGPIXELS[X|Y]) = 96dpi
//	1��� == 25.4mm
static	const float	LOGPIXEL = 96 / 25.4f;
//	�~���\���̉~������
static	const int		CYCLECOUNT = ARCCOUNT*8+1;	// 512����
static	const float		CYCLESTEP  = PI2/(CYCLECOUNT-1);

// �����ٕ`��p�̖ʖ@��(�������͋N����CNCVCApp::CNCVCApp()����)
static	boost::array<GLfloat, (ARCCOUNT+2)*NCXYZ>	GLMillUpNor;	// ���
static	boost::array<GLfloat, (ARCCOUNT+2)*NCXYZ>	GLMillDwNor;	// ����
static	boost::array<GLfloat, (ARCCOUNT+1)*2*NCXYZ>	GLMillSdNor;	// ����
static	boost::array<GLfloat, ((ARCCOUNT+1)*2*(ARCCOUNT/4-2)+(ARCCOUNT+2))*NCXYZ>
													GLMillPhNor;	// �ްٴ�����

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
	//
	ON_MESSAGE(WM_USERTRACESELECT, &CNCViewGL::OnSelectTrace)
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
	if ( IsLatheMode() ) {
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
	case UAV_DRAWMAXRECT:
		return;		// ����
	case UAV_TRACECURSOR:
		if ( (UINT_PTR)pHint == ID_NCVIEW_TRACE_CURSOR ) {
			// ���وʒu�܂Ŏ��s�i�͈͐����j
			CClientDC	dc(this);
			::wglMakeCurrent( dc.GetSafeHdc(), m_hRC );
			if ( IsLatheMode() )
				CreateLathe(TRUE);
			else
				CreateBoxel(TRUE);
			::wglMakeCurrent( NULL, NULL );
		}
		else {
			// ���وʒu������s
			OnSelectTrace(NULL, NULL);	// �ڰ����s�̊J�n����
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
	case UAV_FILEINSERT:	// ��L��`�̕ύX
		pOpt->m_dwUpdateFlg = VIEWUPDATE_DISPLAYLIST | VIEWUPDATE_BOXEL;
		pHint = (CObject *)1;	// dummy
		// through
	case UAV_DRAWWORKRECT:	// ܰ���`�ύX
		if ( pOpt->GetNCViewFlg(NCVIEWFLG_SOLIDVIEW) )
			pOpt->m_dwUpdateFlg |= VIEWUPDATE_BOXEL;
		// through
	case UAV_CHANGEFONT:	// �F�̕ύX etc.
		if ( m_bActive ) {
			GLdouble	objXform[4][4];
			CPointF		ptCenter(m_ptCenter);
			if ( pOpt->m_dwUpdateFlg & VIEWUPDATE_BOXEL ) {
				// �s����ظ����ޯ�����
				memcpy(objXform, m_objXform, sizeof(objXform));
				// �s����ظ��̏�����
				OnLensKey(ID_VIEW_FIT);
			}
			// �\�����̍X�V
			UpdateViewOption();
			if ( pOpt->m_dwUpdateFlg & VIEWUPDATE_BOXEL ) {
				// �s����ظ���ؽı
				memcpy(m_objXform, objXform, sizeof(objXform));
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
					pOpt->m_dwUpdateFlg & VIEWUPDATE_TEXTURE ) {
		ClearTexture();
		if ( pOpt->GetNCViewFlg(NCVIEWFLG_TEXTURE) ) {
			if ( ReadTexture(pOpt->GetTextureFile()) &&	// m_nPictureID�l���
					!(pOpt->m_dwUpdateFlg & VIEWUPDATE_BOXEL) ) {
				// �摜�����ύX
				if ( IsLatheMode() )
					CreateTextureLathe();
				else if ( IsWireMode() )
					CreateTextureWire();
				else
					CreateTextureMill();
			}
		}
	}

	// �����̐F�ݒ�
	if ( pOpt->m_dwUpdateFlg & (VIEWUPDATE_LIGHT|VIEWUPDATE_TEXTURE) ) {
		COLORREF	col;
		GLfloat light_Wrk[] = {0.0f, 0.0f, 0.0f, 1.0f},
				light_Cut[] = {0.0f, 0.0f, 0.0f, 1.0f},
				light_Mil[] = {0.0f, 0.0f, 0.0f, 1.0f};
		GLfloat light_Position0[] = {-1.0f, -1.0f, -1.0f,  0.0f},
				light_Position1[] = { 1.0f,  1.0f,  1.0f,  0.0f};
		if ( m_nPictureID > 0 ) {
			// ø����F��L���ɂ��邽�ߔ��F����
			for ( int i=0; i<NCXYZ; i++ )
				light_Wrk[i] = light_Cut[i] = 1.0f;
		}
		else {
			// �\���ݒ�Ɋ�Â�����
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

	// ܲ԰(�ި���ڲؽ�)
	if ( pOpt->m_dwUpdateFlg & VIEWUPDATE_DISPLAYLIST ) {
		if ( m_glCode > 0 )
			::glDeleteLists(m_glCode, 1);
		if ( !IsWireMode() )
			CreateDisplayList();
	}

	// �\���b�h���f���̕\��
	if ( pOpt->m_dwUpdateFlg & VIEWUPDATE_BOXEL ) {
		ClearVBO();
		// ܰ���`�̕`��p���W
		if ( pOpt->GetNCViewFlg(NCVIEWFLG_SOLIDVIEW) ) {
			BOOL	bResult;
			// �؍�̈�̐ݒ�
			m_rcDraw = GetDocument()->GetWorkRect();
			if ( IsLatheMode() ) {
				// ���՗p��]���f���̐���
				bResult = CreateLathe();
				if ( bResult && m_nPictureID > 0 )
					CreateTextureLathe();	// ø������W�̐���
			}
			else if ( IsWireMode() ) {
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
	GLenum err = ::glGetError();		// �װ���ׯ��

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

BOOL CNCViewGL::CreateBoxel(BOOL bRange)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CreateBoxel()\nStart");
#endif
	BOOL	bResult;
	CProgressCtrl*	pProgress = AfxGetNCVCMainWnd()->GetProgressCtrl();
	pProgress->SetRange32(0, 100);		// 100%�\�L
	pProgress->SetPos(0);

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
	dbg.printf("(%f,%f) m_rcView(l,h)", m_rcView.low, m_rcView.high);
	dbg.printf("(%f,%f) m_rcDraw(l,h)", m_rcDraw.low, m_rcDraw.high);
#endif

	if ( GetDocument()->IsDocFlag(NCDOC_WORKFILE) ) {
		// �}�`�t�@�C���Əd�˂�Ƃ�
		bResult = CreateBoxel_fromIGES();
		// ��ݼ��ޯ̧�͍폜
		if ( m_pbStencil ) {
			delete[]	m_pbStencil;
			m_pbStencil = NULL;
		}
	}
	else {
		// �؍��ʂ̕`��i�f�v�X�l�̍X�V�j
#ifdef _DEBUG
		DWORD	t1 = ::timeGetTime();
#endif
		bResult = CreateBottomFaceThread(bRange, 80);
		if ( bResult ) {
#ifdef _DEBUG
			DWORD	t2 = ::timeGetTime();
			dbg.printf( "CreateBottomFaceThread()=%d[ms]", t2 - t1 );
#endif
			// �f�v�X�l�̎擾
			bResult = GetDocument()->IsDocFlag(NCDOC_CYLINDER) ?
				GetClipDepthCylinder(bRange) : GetClipDepthMill(bRange);
		}
	}

	if ( !bResult )
		ClearVBO();

	// �I������
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

	// �}�`���f���̕`��i�[���D��Ńf�v�X�l�̍X�V�{�X�e���V���r�b�g�̍X�V�j
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

	// �؍��ʂ̕`��i�X�e���V���r�b�g���P�̂Ƃ������f�v�X�l���X�V�j
	::glStencilFunc(GL_EQUAL, 0x01, 0xff);
	::glStencilOp(GL_KEEP, GL_KEEP, GL_DECR);	// Zpass����-1 �� �ђ�
	if ( !CreateBottomFaceThread(TRUE, 40) ) {
		::glDisable(GL_STENCIL_TEST);
		::glPopAttrib();
		return FALSE;
	}
#ifdef _DEBUG
	DWORD	t3 = ::timeGetTime();
	dbg.printf( "CreateBottomFaceThread(first)=%d[ms]", t3 - t2 );
#endif

	// ��ʂ̃{�N�Z�����\�z
	if ( !GetClipDepthMill(TRUE, DP_BottomStencil) ) {
		::glDisable(GL_STENCIL_TEST);
		::glPopAttrib();
		return FALSE;
	}
#ifdef _DEBUG
	DWORD	t4 = ::timeGetTime();
	dbg.printf( "GetClipDepthMill(DP_BottomStencil)=%d[ms]", t4 - t3 );
#endif

	// ��O�D��Ő}�`���f����`��i�X�e���V���r�b�g���P�̂Ƃ������`��j
	::glDepthFunc(GL_LESS);
	::glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	for ( i=0; i<kbl->getNum(); i++ ) {
		bd.DrawBody( (BODY *)kbl->getData(i) );
	}
#ifdef _DEBUG
	DWORD	t5 = ::timeGetTime();
	dbg.printf( "DrawBody(second)=%d[ms]", t5 - t4 );
#endif

	// �؍��ʂ̕`��i�[���D��ŃX�e���V���r�b�g���P�̂Ƃ������`��j
	::glDepthFunc(GL_GREATER);
	::glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);	// Zpass����+1 �� �؍��
	if ( !CreateBottomFaceThread(TRUE, 80) ) {
		::glDisable(GL_STENCIL_TEST);
		::glPopAttrib();
		return FALSE;
	}
#ifdef _DEBUG
	DWORD	t6 = ::timeGetTime();
	dbg.printf( "CreateBottomFaceThread(second)=%d[ms]", t6 - t5 );
#endif

	// ��ʂ̃{�N�Z�����\�z
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

	// ���_�z���ޯ̧��޼ު�Đ���
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
		n = min(nLoop/proc, MAXOBJ);	// 1�̽گ�ނ���������ő���ۯ���
		LPCREATEBOTTOMVERTEXPARAM			pParam;
		// WaitForMultipleObjects(FALSE)�ő҂ɂ�
		// CWinThread::m_bAutoDelete���ז������ĕs���������̂�
		// �I���C�x���g�I�u�W�F�N�gCREATEBOTTOMVERTEXPARAM::evEnd.m_hObject�ő҂�
		vector<HANDLE>						vThread;
		vector<LPCREATEBOTTOMVERTEXPARAM>	vParam, vParamEnd;
		// CPU�̐������گ�ސ����Ǝ��s
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
		// �؍��ʕ`��
		::glEnableClientState(GL_VERTEX_ARRAY);
		while ( !vThread.empty() ) {
			// �گ��1�I��邲�Ƃɕ`�揈��
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
				AfxGetNCVCMainWnd()->GetProgressCtrl()->SetPos(pp);	// 10%����
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
	BOOL		bStartDraw;	// �n�_�̕`�悪�K�v���ǂ���
	try {
		while ( TRUE ) {
			// ���s���҂�
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
			// �I���C�x���g
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

	// ��`�̈���߸�ٍ��W�ɕϊ�
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

	// �̈�m��
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
			nSize  *= NCXYZ;				// XYZ���W�i�[
			m_pfXYZ = new GLfloat[nSize*2];	// ��ʂƒ��
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

	// �ײ��ė̈�����߽�l���擾�i�߸�ْP�ʁj
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
		// �ײ��ė̈�̽�ݼْl���擾
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

	// ܰ���ʒ�ʂƐ؍�ʂ̍��W�o�^
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
					&wx2, &wy2, &wz2);	// ����قǒx���Ȃ��̂Ŏ���ϊ��͒��~
			// ���ꂼ��̍��W�o�^��
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
		return TRUE;	// �X�e���V�������̏ꍇ�͂����܂�

	//
	if ( GetDocument()->GetTraceMode() == ID_NCVIEW_TRACE_STOP )
		AfxGetNCVCMainWnd()->GetProgressCtrl()->SetPos(90);	

	// ���_�z���ޯ̧��޼ު�Đ���
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
	// ܰ��ލ��W
	m_pfXYZ[tu+NCA_X] = m_pfXYZ[td+NCA_X] = (GLfloat)wx;
	m_pfXYZ[tu+NCA_Y] = m_pfXYZ[td+NCA_Y] = (GLfloat)wy;
	m_pfXYZ[tu+NCA_Z] = min((GLfloat)wz, m_rcDraw.high);	// ��ʂ𒴂��Ȃ�
	m_pfXYZ[td+NCA_Z] = m_rcDraw.low;	// ��ʂ�m_rcDraw.low���W�ŕ~���l�߂�
	// ��̫�Ă̖@���޸��
	m_pfNOR[tu+NCA_X] =  0.0f;
	m_pfNOR[tu+NCA_Y] =  0.0f;
	m_pfNOR[tu+NCA_Z] =  1.0f;
	m_pfNOR[td+NCA_X] =  0.0f;
	m_pfNOR[td+NCA_Y] =  0.0f;
	m_pfNOR[td+NCA_Z] = -1.0f;
}

void CNCViewGL::GetClipDepthMill_Zonly(GLdouble wx, GLdouble wy, GLdouble wz, size_t tu, size_t td)
{
	// ���Z�l�̂�
	m_pfXYZ[tu+NCA_Z] = min((GLfloat)wz, m_rcDraw.high);
}

void CNCViewGL::GetClipDepthMill_BottomStencil(GLdouble wx, GLdouble wy, GLdouble wz, size_t tu, size_t td)
{
	// �X�e���V���l��"1"�̂Ƃ������{�N�Z���擾
	// �����łȂ��Ƃ���� -FLT_MAX ���Z�b�g���Ċђʂ�����
	m_pfXYZ[td+NCA_X] = (GLfloat)wx;
	m_pfXYZ[td+NCA_Y] = (GLfloat)wy;
	m_pfXYZ[td+NCA_Z] = m_pbStencil[tu/NCXYZ]==1 ? (GLfloat)wz : -FLT_MAX;
	m_pfNOR[td+NCA_X] =  0.0f;
	m_pfNOR[td+NCA_Y] =  0.0f;
	m_pfNOR[td+NCA_Z] = -1.0f;
}

void CNCViewGL::GetClipDepthMill_TopStencil(GLdouble wx, GLdouble wy, GLdouble wz, size_t tu, size_t td)
{
	// �X�e���V���l��"0"���傫���Ƃ������{�N�Z���擾
	// �����łȂ��Ƃ���� FLT_MAX ���Z�b�g���Ċђʂ�����
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

	// ���_���ޯ���̏���
	if ( m_pSolidElement ) {
		::glDeleteBuffers((GLsizei)(m_vElementWrk.size()+m_vElementCut.size()), m_pSolidElement);
		delete[]	m_pSolidElement;
		m_pSolidElement = NULL;
	}
	m_vElementWrk.clear();
	m_vElementCut.clear();

	// ����
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

	// CPU���گ�ދN��
	n = cy / proc;	// 1CPU������̏�����
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

	// ܰ����ʂ͖@���޸�ق̍X�V������
	// �؍�ʏ����Ƃ̔r��������l��
	if ( !n || !CreateElementSide(&pParam[0], GetDocument()->IsDocFlag(NCDOC_CYLINDER)) ) {
		delete[]	pThread;
		delete[]	pParam;
		return FALSE;
	}

	errCode = ::glGetError();		// �װ���ׯ��

	if ( GetDocument()->GetTraceMode() == ID_NCVIEW_TRACE_STOP )
		AfxGetNCVCMainWnd()->GetProgressCtrl()->SetPos(100);	

	// ���_�z��Ɩ@���޸�ق�GPU��؂ɓ]��
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

	// ���_���ޯ����GPU��؂ɓ]��
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
		// ܰ���`�p
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
		// �؍�ʗp
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
	UINT		z0, z1;
	CVelement	vElement;

	vElement.reserve(pParam->cx * 2 + 1);

	// �؍�ʈȊO�� m_rcDraw.high �������Ă���̂�
	// ���������ۂ��̔��f��OK
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
					// z0:�~, z1:�~
					if ( vElement.size() > 3 )
						pParam->vvElementWrk.push_back(vElement);
					bReverse = bSingle = FALSE;
					vElement.clear();
				}
				else {
					// z0:�~, z1:��
					if ( bSingle ) {
						// �Е��������A������Ȃ炻����break
						if ( vElement.size() > 3 )
							pParam->vvElementWrk.push_back(vElement);
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
			else if ( (pParam->pbStl&&pParam->pbStl[n1]!=1) || (!pParam->pbStl&&pParam->pfXYZ[z1]!=pParam->h) ) {
				// z0:��, z1:�~
				if ( bSingle ) {
					if ( vElement.size() > 3 )
						pParam->vvElementWrk.push_back(vElement);
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
				// z0:��, z1:��
				if ( bSingle ) {	// �O�񂪕Е�����
					// ���`��h�~
					if ( vElement.size() > 3 ) {
						pParam->vvElementWrk.push_back(vElement);
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
			pParam->vvElementWrk.push_back(vElement);
	}
}

void CreateElementBtm(LPCREATEELEMENTPARAM pParam)
{
	// ܰ���ʏ���
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
			z0 = n0*NCXYZ + NCA_Z;	// ��ʂ����߽���ŕ`�攻�f
			z1 = n1*NCXYZ + NCA_Z;
			b0 = n0 + cxcy;			// ��ʍ��W�ԍ�
			b1 = n1 + cxcy;
			if ( (pParam->pbStl&&pParam->pbStl[n0]<1) || (!pParam->pbStl&&pParam->pfXYZ[z0]<pParam->l) ) {
				if ( (pParam->pbStl&&pParam->pbStl[n1]<1) || (!pParam->pbStl&&pParam->pfXYZ[z1]<pParam->l) ) {
					// z0:�~, z1:�~
					if ( vElement.size() > 3 )
						pParam->vvElementWrk.push_back(vElement);
					bReverse = bSingle = FALSE;
					vElement.clear();
				}
				else {
					// z0:�~, z1:��
					if ( bSingle ) {
						// �Е��������A������Ȃ炻����break
						if ( vElement.size() > 3 )
							pParam->vvElementWrk.push_back(vElement);
						bReverse = bSingle = FALSE;
						vElement.clear();
					}
					else {
						vElement.push_back(b1);
						bReverse = FALSE;	// ����b0����o�^
						bSingle  = TRUE;
					}
				}
			}
			else if ( (pParam->pbStl&&pParam->pbStl[n1]<1) || (!pParam->pbStl&&pParam->pfXYZ[z1]<pParam->l) ) {
				// z0:��, z1:�~
				if ( bSingle ) {
					if ( vElement.size() > 3 )
						pParam->vvElementWrk.push_back(vElement);
					bReverse = bSingle = FALSE;
					vElement.clear();
				}
				else {
					vElement.push_back(b0);
					bReverse = TRUE;		// ����b1����o�^
					bSingle  = TRUE;
				}
			}
			else {
				// z0:��, z1:��
				if ( bSingle ) {	// �O�񂪕Е�����
					// ���`��h�~
					if ( vElement.size() > 3 ) {
						pParam->vvElementWrk.push_back(vElement);
						// �O��̍��W���ēo�^
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
		// �w������ٰ�߂��I��
		if ( vElement.size() > 3 )
			pParam->vvElementWrk.push_back(vElement);
	}
}

void CreateElementCut(LPCREATEELEMENTPARAM pParam)
{
	// �؍�ʁi��ʁE���ʌ��p�j
	int			i, j;
	BOOL		bWorkrct,	// �O��~�~���ǂ����𔻒f�����׸�
				bThrough;	// �O�񁢁��@�@�V
	GLuint		n0, n1;
	UINT		z0, z1, z0b, z1b;
	GLfloat		z0z, z1z;
	float		q;
	CVelement	vElement;

	vElement.reserve(pParam->cx * 2 + 1);

	// ���F�؍�ʁC���F�ђʁC�~�Fܰ����
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
					// z0:�~, z1:�~
					if ( bWorkrct ) {
						// �O��� z0:�~, z1:�~
						if ( vElement.size() > 3 ) {
							// break
							pParam->vvElementCut.push_back(vElement);
							// �O��̖@����������(�I�_)�ɕύX
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
					// z0:�~, z1:��
					// z0:�~, z1:��
					if ( bWorkrct ) {
						// �O�� z0:�~, z1:�~ �Ȃ�
						// �O��̖@�����E����(�n�_)�ɕύX
						pParam->pfNOR[z0b+NCA_X] = 1.0;
						pParam->pfNOR[z0b+NCA_Z] = 0;
						pParam->pfNOR[z1b+NCA_X] = 1.0;
						pParam->pfNOR[z1b+NCA_Z] = 0;
					}
					// z0��Y����������̖@����
					pParam->pfNOR[z0+NCA_Y] = 1.0;
					pParam->pfNOR[z0+NCA_Z] = 0;
					if ( z1z < pParam->l ) {
						// z1�i���F�؂�ڑ��j��Y�����������̖@��
						pParam->pfNOR[z1+NCA_Y] = -1.0;
						pParam->pfNOR[z1+NCA_Z] = 0;
					}
					bWorkrct = FALSE;
				}
				bThrough = FALSE;
			}
			else if ( (pParam->pbStl&&pParam->pbStl[n0]<1) || (!pParam->pbStl&&z0z<pParam->l) ) {
				// z0:��
				if ( bWorkrct ) {
					// �O�� z0:�~, z1:�~
					// �O��̖@�����E����(�n�_)�ɕύX
					pParam->pfNOR[z0b+NCA_X] = 1.0;
					pParam->pfNOR[z0b+NCA_Z] = 0;
					pParam->pfNOR[z1b+NCA_X] = 1.0;
					pParam->pfNOR[z1b+NCA_Z] = 0;
				}
				bWorkrct = FALSE;
				if ( (pParam->pbStl&&pParam->pbStl[n1]<1) || (!pParam->pbStl&&z1z<pParam->l) ) {
					// z0:��, z1:��
					if ( bThrough ) {
						// �O��� z0:��, z1:��
						if ( vElement.size() > 3 ) {
							// break
							pParam->vvElementCut.push_back(vElement);
							// �O��̖@����������(�؂��)�ɕύX
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
					// z0:��, z1:�~
					// z0:��, z1:��
					bThrough = FALSE;
					// z0��������, z1��������̖@��
					pParam->pfNOR[z0+NCA_Y] = -1.0;
					pParam->pfNOR[z0+NCA_Z] = 0;
					pParam->pfNOR[z1+NCA_Y] = 1.0;
					pParam->pfNOR[z1+NCA_Z] = 0;
				}
			}
			else {
				// z0:��
				if ( bWorkrct ) {
					// �O�� z0:�~, z1:�~
					// �O��̖@�����E�����ɕύX
					pParam->pfNOR[z0b+NCA_X] = 1.0;
					pParam->pfNOR[z0b+NCA_Z] = 0;
					pParam->pfNOR[z1b+NCA_X] = 1.0;
					pParam->pfNOR[z1b+NCA_Z] = 0;
				}
				if ( bThrough ) {
					// �O�� z0:��, z1:��
					// �O��̖@�����������ɕύX
					pParam->pfNOR[z0b+NCA_X] = -1.0;
					pParam->pfNOR[z0b+NCA_Z] = 0;
					pParam->pfNOR[z1b+NCA_X] = -1.0;
					pParam->pfNOR[z1b+NCA_Z] = 0;
				}
				bWorkrct = bThrough = FALSE;
				if ( (pParam->pbStl&&pParam->pbStl[n1]>=1) || (!pParam->pbStl&&z1z>=pParam->h) ) {
					// z0:��, z1:�~
					pParam->pfNOR[z1+NCA_Y] = -1.0;		// �������̖@��
					pParam->pfNOR[z1+NCA_Z] = 0;
				}
				else if ( (pParam->pbStl&&pParam->pbStl[n1]<1) || (!pParam->pbStl&&z1z<pParam->l) ) {
					// z0:��, z1:��
					pParam->pfNOR[z1+NCA_Y] = 1.0;		// ������̖@��
					pParam->pfNOR[z1+NCA_Z] = 0;
				}
				else {
					// YZ���ʂł̖@���޸�ٌv�Z
					q = atan2(z1z - z0z, 
						pParam->pfXYZ[z1+NCA_Y] - pParam->pfXYZ[z0+NCA_Y]);
					pParam->pfNOR[z1+NCA_Y] = -sin(q);	// ���̊ȗ���
					pParam->pfNOR[z1+NCA_Z] =  cos(q);
				}
				// z0�ɑ΂���@���v�Z
				if ( i>0 && pParam->pfXYZ[z0b+NCA_Z] >= pParam->l ) {
					// �O��̍��W�Ƃ̊p�x������
					// XZ���ʂł̖@���޸�ق��v�Z
					q = atan2(z0z - pParam->pfXYZ[z0b+NCA_Z],
						pParam->pfXYZ[z0+NCA_X] - pParam->pfXYZ[z0b+NCA_X]);
					pParam->pfNOR[z0+NCA_X] = -sin(q);
					pParam->pfNOR[z0+NCA_Z] =  cos(q);
				}
			}
			// �S���W���Ȃ�
			// �~�~|�������A�����鏊����clear
			vElement.push_back(n1);
			vElement.push_back(n0);
			// 
			z0b = z0;
			z1b = z1;
		}
		// �w������ٰ�߂��I��
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
				// �@���޸�ق̏C��
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
			nn = pParam->cy * pParam->cx;	// ��ʍ��W�ւ̵̾��
			// ܰ���`���ʁiX������O�Ɖ��j
			for ( j=0; j<pParam->cy; j+=pParam->cy-1 ) {	// 0��m_icy-1
				vElement.clear();
				for ( i=0; i<pParam->cx; i++ ) {
					nh = j*pParam->cx+i;	// ��ʍ��W
					nl = nh + nn;			// ��ʍ��W
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
					// �@���޸�ق̏C��
					pParam->pfNOR[kh+NCA_Y] = nor;	// ��or�������̖@��
					pParam->pfNOR[kh+NCA_Z] = 0;
					pParam->pfNOR[kl+NCA_Y] = nor;
					pParam->pfNOR[kl+NCA_Z] = 0;
				}
				if ( vElement.size() > 3 )
					pParam->vvElementWrk.push_back(vElement);
				nor *= -1.0;	// �������]
			}
			// ܰ���`���ʁiY�������ƉE�j
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
					pParam->pfNOR[kh+NCA_X] = nor;	// ��or�E�����̖@��
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
//	�ײ����H�̉~������ݸ�
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

	// ��`�̈���߸�ٍ��W�ɕϊ�
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
	// ���S==���a���i�ߋ���
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

	// �̈�m��
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

	// �ײ��ė̈�����߽�l���擾�i�߸�ْP�ʁj
	::glPixelStorei(GL_PACK_ALIGNMENT, 1);
	::glReadPixels(m_wx, m_wy, m_icx, m_icy,
					GL_DEPTH_COMPONENT, GL_FLOAT, m_pfDepth);
//	ASSERT( ::glGetError() == GL_NO_ERROR );
	GLenum glError = ::glGetError();
#ifdef _DEBUG
	DWORD	t3 = ::timeGetTime();
	dbg.printf( "glReadPixels()=%d[ms]", t3 - t2 );
#endif

	// ܰ���ʒ�ʂƐ؍�ʂ̍��W�o�^
	if ( bRecalc ) {
		for ( rx=0, ry=0, nnu=0, nnd=nSize; rx<ox; rx+=sx, ry+=sy ) {
			nnu0 = nnu;
			nnd0 = nnd;
			for ( j=0, q=0; j<CYCLECOUNT-1; j++, q+=CYCLESTEP, nnu+=NCXYZ, nnd+=NCXYZ ) {
				px = (size_t)(rx * cos(q) + ox);	// size_t�Ŏ󂯂�
				py = (size_t)(ry * sin(q) + oy);
				n  = min(py*m_icx+px, (size_t)(m_icx*m_icy));
				::gluUnProject((GLdouble)px, (GLdouble)py, m_pfDepth[n],
						mvMatrix, pjMatrix, viewPort,
						&wx2, &wy2, &wz2);
				// ܰ��ލ��W
				m_pfXYZ[nnu+NCA_X] = m_pfXYZ[nnd+NCA_X] = (GLfloat)wx2;
				m_pfXYZ[nnu+NCA_Y] = m_pfXYZ[nnd+NCA_Y] = (GLfloat)wy2;
				m_pfXYZ[nnu+NCA_Z] = min((float)wz2, m_rcDraw.high);	// ��ʂ𒴂��Ȃ�
				m_pfXYZ[nnd+NCA_Z] = m_rcDraw.low;	// ��ʂ�m_rcDraw.low���W�ŕ~���l�߂�
				// ��̫�Ă̖@���޸��
				m_pfNOR[nnu+NCA_X] =  0.0f;
				m_pfNOR[nnu+NCA_Y] =  0.0f;
				m_pfNOR[nnu+NCA_Z] =  1.0f;
				m_pfNOR[nnd+NCA_X] =  0.0f;
				m_pfNOR[nnd+NCA_Y] =  0.0f;
				m_pfNOR[nnd+NCA_Z] = -1.0f;
			}
			// �~�������
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

	// ���_�z���ޯ̧��޼ު�Đ���
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
//	���Չ��H��]����ݸ�
/////////////////////////////////////////////////////////////////////////////

BOOL CNCViewGL::CreateLathe(BOOL bRange)
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

	size_t	i, s, e;
	if ( bRange ) {
		s = GetDocument()->GetTraceStart();
		e = GetDocument()->GetTraceDraw();
	}
	else {
		s = 0;
		e = GetDocument()->GetNCsize();
	}

	// ���՗pZXܲ԰�̕`��
	// --- �ް��ʂ��炵��CreateBoxel()�݂���������گ�މ��͕K�v�Ȃ��ł��傤
	::glPushAttrib( GL_LINE_BIT );
	::glLineWidth( 3.0 );	// 1Pixel�ł����߽�l���E���Ȃ���������Ȃ��̂�
	for ( i=s; i<e; i++ )
		GetDocument()->GetNCdata(i)->DrawGLLatheFace();
	::glPopAttrib();
	::glFinish();

	// ���߽�l�̎擾
	BOOL	bResult = GetClipDepthLathe(bRange);
	if ( !bResult )
		ClearVBO();

	// �I������
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

	// ��`�̈�(left/right)���߸�ٍ��W�ɕϊ�
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

	// �̈�m��
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

	// �ײ��ė̈�����߽�l���擾�i�߸�ْP�ʁj
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

	// �O�a�؍�ʂ̍��W�o�^
	for ( i=0; i<m_icx; i++ ) {
		::gluUnProject(i+wx1, wy1, m_pfDepth[i],
				mvMatrix, pjMatrix, viewPort,
				&wx2, &wy2, &wz2);	// ����قǒx���Ȃ��̂Ŏ���ϊ��͒��~
		fz = (GLfloat)fabs( m_pfDepth[i]==0.0f ?	// ���߽�l�������l(��`�͈͓��Ő؍�ʂłȂ�)�Ȃ�
				m_rcDraw.high :				// ܰ����a�l
				min(wz2, m_rcDraw.high) );	// �ϊ����W(==���a)��ܰ����a�̏�������
		ii = i * (ARCCOUNT+1);
		// ð�߰��̖@���޸�ق��v�Z
		if ( fzb && fz != *fzb ) {
			q = fz - *fzb;
			fx = cos( atan2(q, (float)wx2-fxb) + RAD(90.0f) );
		}
		else
			fx = 0;
		// fz �𔼌a�ɉ~���`�̍��W�𐶐�
		for ( j=0, q=0; j<=ARCCOUNT; j++, q+=ARCSTEP ) {
			jj = (ii + j) * NCXYZ;
			// ��̫�Ă̖@���޸��
			m_pfNOR[jj+NCA_X] = fx;
			m_pfNOR[jj+NCA_Y] = cos(q);
			m_pfNOR[jj+NCA_Z] = sin(q);
			// ܰ��ލ��W
			m_pfXYZ[jj+NCA_X] = (GLfloat)wx2;
			m_pfXYZ[jj+NCA_Y] = fz * m_pfNOR[jj+NCA_Y];
			m_pfXYZ[jj+NCA_Z] = fz * m_pfNOR[jj+NCA_Z];
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
	vector<CVelement>	vvElementWrk,	// ���_�z����ޯ��(�ϒ��Q�����z��)
						vvElementCut;
	CVelement	vElement;

	// ���_���ޯ���̏���
	if ( m_pSolidElement ) {
		::glDeleteBuffers((GLsizei)(m_vElementWrk.size()+m_vElementCut.size()), m_pSolidElement);
		delete[]	m_pSolidElement;
		m_pSolidElement = NULL;
	}
	m_vElementWrk.clear();
	m_vElementCut.clear();

	// ����
	vvElementWrk.reserve(m_icx+1);
	vvElementCut.reserve(m_icx+1);
	vElement.reserve((ARCCOUNT+1)*2+1);

	// i��i+1�̍��W���~���`�ɂȂ���
	for ( i=0; i<m_icx-1; i++ ) {
		vElement.clear();
		for ( j=0; j<=ARCCOUNT; j++ ) {
			n0 =  i    * (ARCCOUNT+1) + j;
			n1 = (i+1) * (ARCCOUNT+1) + j;
			vElement.push_back(n0);
			vElement.push_back(n1);
		}
		if ( m_pfDepth[i+1] == 0.0f )	// �؍�ʂ�ܰ��ʂ�
			vvElementWrk.push_back(vElement);
		else
			vvElementCut.push_back(vElement);
	}

	errCode = ::glGetError();		// �װ���ׯ��

	// ���_�z��Ɩ@���޸�ق�GPU��؂ɓ]��
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

	// ���_���ޯ����GPU��؂ɓ]��
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
		// ܰ���`�p
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
		// �؍�ʗp
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
//	ܲԉ��H�\������ݸ�
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
	CVelement	vef;	// �ʐ����p���_���ޯ��
	WIRELINE	wl;		// ���`��p

	// ��޼ު�Ă̒�����񂪕K�v�Ȃ��߁A�؍펞�Ԍv�Z�گ�ޏI���҂�
	GetDocument()->WaitCalcThread(TRUE);

	// ���_���ޯ���̏���
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

	// ܲԉ��H�̍��W�o�^
	for ( i=GetDocument()->GetTraceStart(); i<nLoop; i++ ) {
		// �ʌ`���ȊO��ٰ��
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
		// �ʌ`���i�؍��ް��j
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
					// �~���ް��̏ꍇ�͈�U�؂�Ȃ���GL_LINE_STRIP�Ŏ��ɂȂ���Ȃ�
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

	errCode = ::glGetError();	// �װ���ׯ��
	::glGenBuffers(SIZEOF(m_nVertexID), m_nVertexID);

	// ���_�z���GPU��؂ɓ]��
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

	// �@���޸�ق�GPU��؂ɓ]��
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
	// ���_���ޯ����GPU��؂ɓ]��
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
	nVertex = icx * icy * 2 * 2;	// (X,Y) ��ʂƒ��

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
	for ( j=0, n=0; j<icy; j++ ) {
		ft = (GLfloat)(icy-j)/icy;
		for ( i=0; i<icx; i++, n+=2 ) {
			// ø������W(0.0�`1.0)
			pfTEX[n]   = (GLfloat)i/icx;
			pfTEX[n+1] = ft;
		}
	}

	// ��ʗpø������W
	for ( j=0; j<icy; j++ ) {
		ft = (GLfloat)j/icy;
		for ( i=0; i<icx; i++, n+=2 ) {
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
				icx = m_icx-1;					// ���꒲��
	GLsizeiptr	nVertex = m_icx*(ARCCOUNT+1)*2;	// X�Ɖ~�����W��
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
	for ( i=0, n=0; i<m_icx; i++ ) {
		ft = (GLfloat)i/icx;
		for ( j=0; j<=ARCCOUNT; j++, n+=2 ) {
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
	INT_PTR		i, j, n, nLoop = GetDocument()->GetNCsize();
	GLsizeiptr	nVertex;		// ���_��
	int			nResult;
	float		dAccuLength;	// �ݐϒ���
	CNCdata*	pData;
	GLfloat*	pfTEX;

	// ���_���v�Z
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

	// ø������W�̊��蓖��(CreateWire�Ɠ���ٰ��)
	for ( i=0, j=0, n=0; i<nLoop; i++, j++ ) {
		// �؍�J�n�_�̌���
		for ( ; i<nLoop; i++ ) {
			pData = GetDocument()->GetNCdata(i);
			if ( pData->IsCutCode() ) {
				if ( !pData->GetWireObj() )
					continue;
				// �n�_�o�^
				pfTEX[n++] = 0.0f;	// XY
				pfTEX[n++] = 0.0f;
				pfTEX[n++] = 0.0f;	// UV
				pfTEX[n++] = 1.0f;
				break;
			}
		}
		dAccuLength = 0.0f;
		// �e��޼ު�Ă��Ƃ�ø������W��o�^
		for ( ; i<nLoop; i++ ) {
			pData = GetDocument()->GetNCdata(i);
			nResult = pData->AddGLWireTexture(n, dAccuLength, m_WireDraw.vLen[j], pfTEX);
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
	GLenum	errCode;

	// ø������W��GPU��؂ɓ]��
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
	// �޸�ِ����̂��߂̏����ݒ�
	::glDisable(GL_NORMALIZE);
	::glClearDepth(0.0);		// ��������D�悳���邽�߂����߽�����l
	::glDepthFunc(GL_GREATER);	// ��������D��
	::glClear(GL_DEPTH_BUFFER_BIT);		// ���߽�ޯ̧�̂ݸر
	::glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);	// �װϽ�OFF
	// ��]�s����ޯ����߂Ə�����
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

	// �ʏ�ݒ�ɖ߂�
	::glClearDepth(1.0);
	::glDepthFunc(GL_LESS);		// �߂�����D��(�ʏ�`��)
	::glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	::glEnable(GL_NORMALIZE);
	// ��]�s������ɖ߂�
	memcpy(m_objXform, m_objXformBk, sizeof(m_objXform));
	m_ptCenter = m_ptCenterBk;
	SetupViewingTransform();
}

void CNCViewGL::RenderBack(void)
{
	::glDisable(GL_DEPTH_TEST);	// ���߽ýĖ����ŕ`��

	// �w�i��غ�݂̕`��
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
	// ����
	dVertex[0] = m_rcView.left;
	dVertex[1] = m_rcView.bottom;
//	dVertex[2] = m_rcView.low;
	dVertex[2] = m_rcView.high - NCMIN*2.0f;	// ��ԉ�(x2�͵Ϲ)
	::glColor3ubv(col1v);
	::glVertex3fv(dVertex);
	// ����
	dVertex[1] = m_rcView.top;
	::glColor3ubv(col2v);
	::glVertex3fv(dVertex);
	// �E��
	dVertex[0] = m_rcView.right;
	::glColor3ubv(col2v);
	::glVertex3fv(dVertex);
	// �E��
	dVertex[1] = m_rcView.bottom;
	::glColor3ubv(col1v);
	::glVertex3fv(dVertex);
	//
	::glEnd();
	::glPopMatrix();

	::glEnable(GL_DEPTH_TEST);	// ���ɖ߂�
}

void CNCViewGL::RenderAxis(void)
{
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	float		dLength;
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

	::glDisable( GL_LINE_STIPPLE );
	::glPopAttrib();
}

void CNCViewGL::RenderCode(void)
{
	if ( IsWireMode() )
		return;
	CNCdata*	pData;

	::glEnable( GL_LINE_STIPPLE );
	// NC�ް��̋O�Ձiܲ԰�ڰсj�`��
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

	// �����ْ���
	float	h = m_rcDraw.Depth() * 2.0f;

	// �����ُ㕔
	ptOrg.z += h;
	pData->SetEndmillOrgCircle(ptOrg, bd.vpt);
	// �����ى���
	ptOrg.z -= h;
	if ( pData->GetEndmillType() == NCMIL_BALL )
		ptOrg.z += pData->GetEndmill();
	if ( pData->GetEndmillType() == NCMIL_CHAMFER )
		pData->SetChamfermillOrg(ptOrg, bd.vpt);
	else
		pData->SetEndmillOrgCircle(ptOrg, bd.vpt);
	// �㕔�Ƒ��ʂ̕`��
	::glVertexPointer(NCXYZ, GL_FLOAT, 0, &(bd.vpt[0]));
	::glNormalPointer(GL_FLOAT, 0, &(GLMillUpNor[0]));
	bd.vel.assign(GLFanElement[0], GLFanElement[0]+ARCCOUNT+2);
	::glDrawRangeElements(GL_TRIANGLE_FAN, 0, 64,
			(GLsizei)(bd.vel.size()), GL_UNSIGNED_INT, &(bd.vel[0]));
	::glNormalPointer(GL_FLOAT, 0, &(GLMillSdNor[0]));
	bd.vel.assign(GLFanStripElement, GLFanStripElement+(ARCCOUNT+1)*2);
	::glDrawRangeElements(GL_TRIANGLE_STRIP, 1, 129,
			(GLsizei)(bd.vel.size()), GL_UNSIGNED_INT, &(bd.vel[0]));
	// ����
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
	// ���ً�Ԃ̉�]
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
		// ����ݸ�&�ޭ��ݸޕϊ��s��
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

	// MDI�q�ڰт̽ð���ް�ɏ��\��
	static_cast<CNCChild *>(GetParentFrame())->SetFactorInfo(m_dRate/LOGPIXEL, m_strGuide);
}

void CNCViewGL::DoRotation(float dAngle)
{
#ifdef _DEBUG
//	g_dbg.printf("DoRotation() Angle=%f (%f, %f, %f)", dAngle,
//		m_ptRoundBase.x, m_ptRoundBase.y, m_ptRoundBase.z);
#endif
	// ��]��د�������̵݂�޼ު��̫����د���Ɋ|�����킹��
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
	::glMultMatrixd( (GLdouble *)m_objXform );	// �\����]�i�����f����]�j
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
		// ���悪�������\������邽�߂���غ�ݵ̾��
		::glEnable(GL_POLYGON_OFFSET_FILL);
		::glPolygonOffset(1.0f, 1.0f);
		// ���_�ޯ̧��޼ު�Ăɂ���޸�ٕ`��
		::glEnableClientState(GL_VERTEX_ARRAY);
		::glEnableClientState(GL_NORMAL_ARRAY);
		::glBindBuffer(GL_ARRAY_BUFFER, m_nVertexID[0]);
		::glVertexPointer(NCXYZ, GL_FLOAT, 0, NULL);
		if ( IsWireMode() && pOpt->GetNCViewFlg(NCVIEWFLG_WIREPATH) ) {
			// �O��ܲ԰�ڰѕ\��
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
		// ܰ���`
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
		// �؍��
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
			// �����ٕ`��
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
			// ����
			if ( m_glCode > 0 )
				::glCallList( m_glCode );
			else
				RenderCode();
		}
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

	// OpenGL�g����߰Ă�����
	if ( !GLEW_ARB_vertex_buffer_object ) {
		CViewOption*	pOpt = AfxGetNCVCApp()->GetViewOption();
		CString	strErr;
		strErr.Format(IDS_ERR_OPENGLVER, ::glGetString(GL_VERSION));
		AfxMessageBox(strErr, MB_OK|MB_ICONEXCLAMATION);
		pOpt->m_bSolidView  = FALSE;	// �׸ދ���OFF
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

	// ��]�s�񓙂�ۑ�
	if ( m_bActive ) {
		CRecentViewInfo* pInfo = GetDocument()->GetRecentViewInfo();
		if ( pInfo ) 
			pInfo->SetViewInfo(m_objXform, m_rcView, m_ptCenter);
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
	// �ޭ��߰Đݒ菈��
	CClientDC	dc(this);
	::wglMakeCurrent( dc.GetSafeHdc(), m_hRC );
	::glViewport(0, 0, cx, cy);
	::wglMakeCurrent(NULL, NULL);
}

void CNCViewGL::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView) 
{
	DBGBOOL(g_dbg, "CNCViewGL::OnActivateView()", bActivate);

	if ( bActivate ) {
		DoScale(0);		// MDI�q�ڰт̽ð���ް�ɏ��\��(����)
	}
	else {
		if ( GetDocument()->GetTraceMode()==ID_NCVIEW_TRACE_RUN &&
			!AfxGetNCVCApp()->GetViewOption()->GetNCViewFlg(NCVIEWFLG_NOACTIVETRACEGL) ) {
			// �ڰ��ꎞ��~
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
		// m_rcView������
		OnUserViewFitMsg(1, 0);		// glOrtho() �����s���Ȃ�
		// �`�揉���ݒ�
		CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
		pOpt->m_dwUpdateFlg = VIEWUPDATE_ALL;
		UpdateViewOption();
		// ��]�s�񓙂̓ǂݍ��݂ƍX�V
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

	// �����ޭ��Ƃ͈���āA�g�嗦���X�V����K�v�͖���
	// �ð���ް�ւ̕\���� OnActivateView() �ōs��
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
		// ��޼ު�ċ�`��10%(�㉺���E5%����)�傫��
		m_rcView.InflateRect(dW*0.05f, dH*0.05f);
//		m_rcView.NormalizeRect();
		dW = fabs(m_rcView.Width());
		dH = fabs(m_rcView.Height());
		dZ = fabs(m_rcView.Depth());

		// �ި���ڲ�̱��߸Ĕ䂩�王�쒼���̐ݒ�
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

	// ����̂݌x��ү����
	if ( !pOpt->GetNCViewFlg(NCVIEWFLG_SOLIDVIEW) ) {
		if ( !pData )
			AfxMessageBox(IDS_ERR_GLTRACE, MB_OK|MB_ICONEXCLAMATION);
		return 0;
	}

	BOOL		bResult = TRUE;
	CClientDC	dc(this);
	::wglMakeCurrent( dc.GetSafeHdc(), m_hRC );

	if ( IsWireMode() ) {
		m_WireDraw.clear();		// ����ر
		CreateWire();
		::wglMakeCurrent(NULL, NULL);
		Invalidate(FALSE);
		UpdateWindow();
		return 0;
	}
	if ( m_bSizeChg ) {			// ����޳���ޕύX�͍ŏ������޸�ٍ\�z
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
		// �ۑ����Ă������߽������������
		::glWindowPos2i(m_wx, m_wy);
		::glDrawPixels(m_icx, m_icy, GL_DEPTH_COMPONENT, GL_FLOAT, m_pfDepth);
		// �w���޼ު�Ă̕`��
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
		// �ڰ������̂��ƍĎ��s��
		// �O���ڰ����ʂ��c��Ȃ��悤�ɂ��邽�߂̑[�u
		m_icx = m_icy = 0;
	}

	if ( bResult ) {
		if ( IsLatheMode() ) {
			bResult = GetClipDepthLathe(TRUE);
		}
		else {
			if ( GetDocument()->IsDocFlag(NCDOC_CYLINDER) )
				bResult = GetClipDepthCylinder(TRUE);
			// ���g���[�X���[�h�̂Ƃ�IGES�d�˂�H�H
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
	Invalidate(FALSE);	// OnDraw()�ŕ`��
	UpdateWindow();		// ���ĕ`��

	return 0;
}

void CNCViewGL::OnUpdateEditCopy(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(FALSE);	// OpenGL�ł�copy�s��
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
		m_dRoundStep = 0.0f;	// KillTimer()�ł�ү���޷���͏����Ȃ�
	}
	BeginTracking( point, TM_SPIN );
}

void CNCViewGL::OnLButtonUp(UINT nFlags, CPoint point)
{
	EndTracking();
	if ( m_dRoundStep != 0.0f ) {
		KillTimer(IDC_OPENGL_DRAGROUND);	// KillTimer() �A�����Ă��G�G�̂���...
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
		KillTimer(IDC_OPENGL_DRAGROUND);	// �A����]�ꎞ��~

	m_ptDownClick = point;
	BeginTracking( point, TM_PAN );
}

void CNCViewGL::OnRButtonUp(UINT nFlags, CPoint point)
{
	EndTracking();
	if ( m_dRoundStep != 0.0f )
		SetTimer(IDC_OPENGL_DRAGROUND, 150, NULL);	// �A����]�ĊJ

	if ( m_ptDownClick == point )
		__super::OnRButtonUp(nFlags, point);	// ��÷���ƭ��̕\��
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
	// ��ϲ���ĂŘA����]
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

	// ��ʉ��ʂ̖@���޸��
	for ( i=0, n1=0, n2=0; i<ARCCOUNT+2; i++ ) {
		GLMillUpNor[n1++] =  0.0f;
		GLMillUpNor[n1++] =  0.0f;
		GLMillUpNor[n1++] =  1.0f;	// �����
		GLMillDwNor[n2++] =  0.0f;
		GLMillDwNor[n2++] =  0.0f;
		GLMillDwNor[n2++] = -1.0f;	// ������
	}
	// ���ʂ̖@���޸��
	for ( i=0, n1=0; i<ARCCOUNT; i++ ) {
		// �㕔
		GLMillSdNor[n1++] = _TABLECOS[i];
		GLMillSdNor[n1++] = _TABLESIN[i];
		GLMillSdNor[n1++] = 0.0f;
		// ����
		GLMillSdNor[n1++] = _TABLECOS[i];
		GLMillSdNor[n1++] = _TABLESIN[i];
		GLMillSdNor[n1++] = 0.0f;
	}
	GLMillSdNor[n1++] = _TABLECOS[0];
	GLMillSdNor[n1++] = _TABLESIN[0];
	GLMillSdNor[n1++] = 0.0f;
	// �ްٴ�����
	for ( j=0, n1=0; j<ARCCOUNT/4-2; j++ ) {
		n2 = j + ARCCOUNT/2;	// 180�x���Ă̓Y����
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
	// �ްٴ����ِ�[
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
