// 3dModelView.cpp : �����t�@�C��
//

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "Kodatuno/Describe_BODY.h"
#undef PI	// Use NCVC (MyTemplate.h)
#include "3dModelChild.h"
#include "3dModelDoc.h"
#include "3dModelView.h"
#include "ViewOption.h"

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

// �C���f�b�N�XID��RGBA��ϊ����郍�[�J���R�[�h
static	void	IDtoRGB(int, GLubyte[]);
static	int		RGBtoID(GLubyte[]);
static	int		SearchSelectID(GLubyte[]);

/////////////////////////////////////////////////////////////////////////////
// C3dModelView

C3dModelView::C3dModelView()
{
	m_pSelBody = NULL;
	m_nSelCurve = -1;
}

C3dModelView::~C3dModelView()
{
}

/////////////////////////////////////////////////////////////////////////////
// C3dModelView �N���X�̃I�[�o���C�h�֐�

void C3dModelView::OnInitialUpdate() 
{
	m_rcView  = GetDocument()->GetMaxRect();
	CRecentViewInfo*	pInfo = GetDocument()->GetRecentViewInfo();
	if ( pInfo && !pInfo->GetViewInfo(m_objXform, m_rcView, m_ptCenter) ) {
		SetOrthoView();		// ViewBaseGL.cpp
	}

	// �ݒ�
	CClientDC	dc(this);
	::wglMakeCurrent( dc.GetSafeHdc(), m_hRC );
	::glMatrixMode( GL_PROJECTION );
	::glOrtho(m_rcView.left, m_rcView.right, m_rcView.top, m_rcView.bottom,
		m_rcView.low, m_rcView.high);
	GetGLError();
	::glMatrixMode( GL_MODELVIEW );
	SetupViewingTransform();

	// ����
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

	::wglMakeCurrent(NULL, NULL);
}

#ifdef _DEBUG
C3dModelDoc* C3dModelView::GetDocument() // ��f�o�b�O �o�[�W�����̓C�����C���ł��B
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(C3dModelDoc)));
	return static_cast<C3dModelDoc *>(m_pDocument);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// C3dModelView �`��

void C3dModelView::OnDraw(CDC* pDC)
{
	ASSERT_VALID(GetDocument());
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();

	// ���ĺ�÷�Ă̊��蓖��
	::wglMakeCurrent( pDC->GetSafeHdc(), m_hRC );

	::glClearColor(0.0, 0.0, 0.0, 0.0);
	::glClearDepth(1.0);
	::glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	// �w�i�̕`��
	RenderBackground(pOpt->GetDxfDrawColor(DXFCOL_BACKGROUND1), pOpt->GetDxfDrawColor(DXFCOL_BACKGROUND2));

	::glDisable(GL_LIGHTING);
	float		dLength = 50.0f;
	COLORREF	col;
	::glPushAttrib( GL_LINE_BIT );
	::glLineWidth( 2.0f );
	::glBegin( GL_LINES );
	// X���̶޲��
	col = pOpt->GetNcDrawColor(NCCOL_GUIDEX);
	::glColor3ub( GetRValue(col), GetGValue(col), GetBValue(col) );
	::glVertex3f(-dLength, 0.0f, 0.0f);
	::glVertex3f( dLength, 0.0f, 0.0f);
	// Y���̶޲��
	col = pOpt->GetNcDrawColor(NCCOL_GUIDEY);
	::glColor3ub( GetRValue(col), GetGValue(col), GetBValue(col) );
	::glVertex3f(0.0f, -dLength, 0.0f);
	::glVertex3f(0.0f,  dLength, 0.0f);
	// Z���̶޲��
	col = pOpt->GetNcDrawColor(NCCOL_GUIDEZ);
	::glColor3ub( GetRValue(col), GetGValue(col), GetBValue(col) );
	::glVertex3f(0.0f, 0.0f, -dLength);
	::glVertex3f(0.0f, 0.0f,  dLength);
	::glEnd();
	::glPopAttrib();

	// --- �e�X�g�`��
//	::glBegin(GL_QUADS);
//		::glColor3f(0.0f, 0.0f, 1.0f);
//		::glVertex3f( 0.0f,  0.0f, 0.0f);
//		::glVertex3f( 0.0f, 50.0f, 0.0f);
//		::glVertex3f(50.0f, 50.0f, 0.0f);
//		::glVertex3f(50.0f,  0.0f, 0.0f);
//	::glEnd();
	// ---

	// ���f���`�� Kodatuno
	::glEnable(GL_LIGHTING);
	::glEnable(GL_DEPTH_TEST);
	DrawBody(RM_NORMAL);
//	DrawBody(RM_PICKLINE);

	::SwapBuffers( pDC->GetSafeHdc() );
	::wglMakeCurrent(NULL, NULL);
}

void C3dModelView::DrawBody(RENDERMODE enRender)
{
	Describe_BODY	bd;
	BODYList*		kbl = GetDocument()->GetKodatunoBodyList();
	BODY*			body;
	GLubyte			rgb[3];
	int				i, j,
					id = 0;		// �����{�f�B�ւ̑Ή�

	for ( i=0; i<kbl->getNum(); i++ ) {
		body = (BODY *)kbl->getData(i);
		if ( !body ) continue;
		switch ( enRender ) {
		case RM_PICKLINE:
			::glEnable(GL_DEPTH_TEST);
			// ���ʔԍ���F�ɃZ�b�g���ĕ`��
			for ( j=0; j<body->TypeNum[_NURBSC]; j++, id++ ) {
		        if ( body->NurbsC[j].EntUseFlag==GEOMTRYELEM && body->NurbsC[j].BlankStat==DISPLAY ) {
					::glDisable(GL_LIGHTING);	// DrawNurbsCurve()�̒���Enable�����
					ZEROCLR(rgb);			// CustomClass.h
					IDtoRGB(id, rgb);
					::glColor3ubv(rgb);
					bd.DrawNurbsCurve(body->NurbsC[j]);
				}
			}
			// �B�������̂��ߖʂ𔒐F�ŕ`��
			::glDisable(GL_LIGHTING);
			rgb[0] = rgb[1] = rgb[2] = 255;
			::glColor3ubv(rgb);
			for ( j=0; j<body->TypeNum[_NURBSS]; j++ ) {
				if ( body->NurbsS[j].TrmdSurfFlag != KOD_TRUE )
					bd.DrawNurbsSurfe(body->NurbsS[j]);
			}
			for ( j=0; j<body->TypeNum[_TRIMMED_SURFACE]; j++ ) {
				bd.DrawTrimdSurf(body->TrmS[j]);
			}
			break;
		case RM_PICKFACE:
			break;
		default:
			::glEnable(GL_POLYGON_OFFSET_FILL);
			::glPolygonOffset(1.0f, 1.0f);		// �ʂ��������ɂ��炵�ĕ`��
			// ���C�u�������̃��[�v�ŕ`��
			bd.DrawBody(body);
			::glDisable(GL_POLYGON_OFFSET_FILL);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// C3dModelView ���b�Z�[�W �n���h���[

int C3dModelView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (__super::OnCreate(lpCreateStruct) == -1)
		return -1;

	return 0;
}

void C3dModelView::OnDestroy()
{
	// ��]�s�񓙂�ۑ�
	CRecentViewInfo* pInfo = GetDocument()->GetRecentViewInfo();
	if ( pInfo ) 
		pInfo->SetViewInfo(m_objXform, m_rcView, m_ptCenter);

	// OpenGL �㏈��
	CClientDC	dc(this);
	::wglMakeCurrent( dc.GetSafeHdc(), m_hRC );

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
		// �s�b�N����
		DoSelect(point);
	}
	__super::OnLButtonUp(nFlags, point);
}

void C3dModelView::DoSelect(const CPoint& pt)
{
	CClientDC	dc(this);
	::wglMakeCurrent( dc.GetSafeHdc(), m_hRC );

	::glClearColor(1.0, 1.0, 1.0, 1.0);	// ���F�ŃN���A
	::glClearDepth(1.0);
	::glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	// ���ʔԍ���F�ɃZ�b�g����NURBS�Ȑ������`��
	DrawBody(RM_PICKLINE);
	GetGLError();		// error flash

	// �}�E�X�|�C���g�̐F�����擾
	GLubyte	buf[READBUF];
	::glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	::glReadPixels(pt.x-PICKREGION, m_cy-pt.y-PICKREGION,	// y���W�ɒ���!!
		2*PICKREGION, 2*PICKREGION, GL_RGBA, GL_UNSIGNED_BYTE, buf);
	GetGLError();

	// buf�̒��ň�ԑ������݂���ID������
	int nResult = SearchSelectID(buf);
	if ( m_pSelBody && m_nSelCurve>=0 && m_nSelCurve!=nResult ) {
		// �I���ς݂̐F�����ɖ߂�
		m_pSelBody->NurbsC[m_nSelCurve].Dstat.Color[0] = 1.0f;
		m_pSelBody->NurbsC[m_nSelCurve].Dstat.Color[1] = 1.0f;
		m_pSelBody->NurbsC[m_nSelCurve].Dstat.Color[2] = 1.0f;
	}
	if ( nResult >= 0 ) {
		int nSel = nResult, nNum;
		// �����̃{�f�B����I���I�u�W�F�N�g���������F��ݒ�
		BODYList*	kbl = GetDocument()->GetKodatunoBodyList();
		BODY*		body;
		for ( int i=0; i<kbl->getNum(); i++ ) {
			body = (BODY *)kbl->getData(i);
			if ( !body ) continue;
			nNum = body->TypeNum[_NURBSC];
			if ( nNum < nSel ) {
				nSel -= nNum;
				continue;
			}
			COLORREF col = AfxGetNCVCApp()->GetViewOption()->GetDrawColor(COMCOL_SELECT);
			body->NurbsC[nSel].Dstat.Color[0] = GetRValue(col) / 255.0f;
			body->NurbsC[nSel].Dstat.Color[1] = GetGValue(col) / 255.0f;
			body->NurbsC[nSel].Dstat.Color[2] = GetBValue(col) / 255.0f;
		}
		m_nSelCurve = nResult;
		m_pSelBody = body;
	}
	else {
		m_nSelCurve = -1;
		m_pSelBody = NULL;
	}

	// �ĕ`��
	Invalidate(FALSE);

	::wglMakeCurrent(NULL, NULL);
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

/////////////////////////////////////////////////////////////////////////////

void IDtoRGB(int id, GLubyte rgb[])
{
	// 0�`254 �̐��l�� RGB �ɕϊ�
	// �i255 �͔��F�N���A�l�j
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
			mp[id] = mp[id] + 1;	// mp[id]++; �ł̓��[�j���O
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
