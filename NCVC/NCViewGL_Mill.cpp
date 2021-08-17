//	NCViewGL_Mill.cpp : �ײ����H�\������ݸ�
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
extern	DWORD			g_nProcesser;	// ��۾����(NCVC.cpp)

// �P�̽گ�ނ���x�ɏ�������NC��ۯ���
#ifdef _WIN64
const size_t	MAXNCBLK = 400;
#else
const size_t	MAXNCBLK = 200;
#endif

//	�~���\���̉~������
static	const int		CYCLECOUNT = ARCCOUNT*8;	// 512����
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
	pProgress->SetRange32(0, 100);		// 100%�\�L
	pProgress->SetPos(0);

	// FBO
	CreateFBO();

	// �޸�ِ����̂��߂̏����ݒ�
	InitialBoxel();
	::glOrtho(m_rcDraw.left, m_rcDraw.right, m_rcDraw.top, m_rcDraw.bottom,
		m_rcView.low, m_rcView.high);	// m_rcDraw �łͷ�ط�؂Ȃ̂� m_rcView ���g��
//		m_rcDraw.low, m_rcDraw.high);	// ���߽�l�̍X�V�ͷ�ط�؂͈̔͂Ő��x�悭 -> 0.0�`1.0
	::glMatrixMode(GL_MODELVIEW);
#ifdef _DEBUG
	printf("(%f,%f)-(%f,%f)\n", m_rcDraw.left, m_rcDraw.top, m_rcDraw.right, m_rcDraw.bottom);
	printf("(%f,%f) m_rcView(l,h)\n", m_rcView.low, m_rcView.high);
	printf("(%f,%f) m_rcDraw(l,h)\n", m_rcDraw.low, m_rcDraw.high);
#endif

	if ( GetDocument()->IsDocFlag(NCDOC_WORKFILE) ) {
#ifdef USE_KODATUNO
		// �}�`�t�@�C���Əd�˂�Ƃ�
		if ( bRange ) {
			CREATEBOXEL_IGESPARAM pParam = RANGEPARAM(GetDocument()->GetTraceStart(), GetDocument()->GetTraceDraw());
			bResult = CreateBoxel_fromIGES(&pParam);
		}
		else
			bResult = CreateBoxel_fromIGES();
#endif
	}
	else {
		// �؍��ʂ̕`��i�f�v�X�l�̍X�V�j
#ifdef _DEBUG
		DWORD t1 = ::timeGetTime();
#endif
		bResult = CreateBottomFaceThread(bRange, 80);
		if ( bResult ) {
#ifdef _DEBUG
			DWORD t2 = ::timeGetTime();
			printf( "CreateBottomFaceThread()=%d[ms]\n", t2 - t1 );
#endif
			// �f�v�X�l�̎擾
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
		// ���_�z���ޯ̧��޼ު�Đ���
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
		
	// �}�`���f���̕`��i�[���D��Ńf�v�X�l�̍X�V�{�X�e���V���r�b�g�̍X�V�j
	::glEnable(GL_STENCIL_TEST);
	if ( pParam ) {
		// �g���[�X���Ȃ̂ŃX�e���V���r�b�g���P�̂Ƃ������`��
		::glStencilFunc(GL_EQUAL, 0x01, 0xff);
		::glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	}
	else {
		// �V�K�����_�����O�Ȃ̂�IGES�̈�̃X�e���V���r�b�g���P�ɍX�V
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

	// �؍��ʂ̕`��i�X�e���V���r�b�g���P�̂Ƃ������f�v�X�l���X�V�j
	::glStencilFunc(GL_EQUAL, 0x01, 0xff);
	::glStencilOp(GL_KEEP, GL_KEEP, GL_DECR);	// Zpass����"0" �� �ђ�
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

	// ��ʂ̃{�N�Z�����\�z
	if ( !GetClipDepthMill(DP_BottomStencil) ) {
		::glDisable(GL_STENCIL_TEST);
		::glPopAttrib();
		return FALSE;
	}
#ifndef NO_TRACE_WORKFILE
	// ��ʗp���߽����ۑ�
	memcpy(m_pfDepthBottom, m_pfDepth, m_icx*m_icy);
#endif
#ifdef _DEBUG
	DWORD	t4 = ::timeGetTime();
	printf( "GetClipDepthMill(DP_BottomStencil)=%d[ms]\n", t4 - t3 );
#endif

	// ��O�D��Ő}�`���f����`��i�X�e���V���r�b�g���P�̂Ƃ������`��j
	::glDepthFunc(GL_LESS);
	::glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	for ( i=0; i<kbl->getNum(); i++ )
		bd.DrawBody( (BODY *)kbl->getData(i) );
//	::glFinish();
#ifdef _DEBUG
	DWORD	t5 = ::timeGetTime();
	printf( "DrawBody(second)=%d[ms]\n", t5 - t4 );
#endif

	// �؍��ʂ̕`��i�[���D��ŃX�e���V���r�b�g���P�ȏ�̂Ƃ������`��j
	::glDepthFunc(GL_GREATER);
	::glStencilFunc(GL_LEQUAL, 0x01, 0xff);		// (ref&mask) <= (stencil&mask)
	::glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);	// Zpass����">1" �� �؍��
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

	// ��ʂ̃{�N�Z�����\�z
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

	// MAXNCBLK/2�����Ȃ�P�گ�ނŎ��s
	if ( GetDocument()->GetNCsize() < MAXNCBLK/2 )
		proc = 1;
	else
		proc  = max(1, min(MAXIMUM_WAIT_OBJECTS, min(nLoop, g_nProcesser*2)));
	n = min(nLoop/proc, MAXNCBLK);	// 1�̽گ�ނ���������ő���ۯ���
#ifdef _DEBUG
	printf("loop=%d proc=%d OneThreadSize=%d\n", nLoop, proc, n);
#endif

	// CEvent���g����vector<>���ŎQ�ƶ��Ă��オ���Ă��܂�??
	CString		strEvent;
	LPCREATEBOTTOMVERTEXPARAM			pParam;
	vector<LPCREATEBOTTOMVERTEXPARAM>	vParam;
	vector<HANDLE>		vHandleS, vHandleE;
	// CPU�̐������گ�ސ����Ǝ��s
	// --- �S�̕`�悾���Ȃ̂Žگ�ނ͏����I��������� ---
	for ( i=0; i<proc; i++ ) {
		strEvent.Format(g_szCBFT_S, i);
		vHandleS.push_back( ::CreateEvent(NULL, FALSE, TRUE,  strEvent) );	// �J�n����Ă͏�������ُ��
		strEvent.Format(g_szCBFT_E, i);
		vHandleE.push_back( ::CreateEvent(NULL, FALSE, FALSE, strEvent) );	// �I������Ă͏�������ُ��
		pParam = new CREATEBOTTOMVERTEXPARAM(i, GetDocument());		// delete��AddBottomVertexThread()����
		pParam->s = e;
		pParam->e = e = min(e+n, nLoop);
		vParam.push_back(pParam);
		AfxBeginThread(AddBottomVertexThread, pParam);
	}
	// �؍��ʕ`��
	::glEnableClientState(GL_VERTEX_ARRAY);
	while ( !vParam.empty() ) {
		// �گ��1�I��邲�Ƃɕ`�揈��
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
		// ���ߍ��񂾍��W�l�̕`��
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
			AfxGetNCVCMainWnd()->GetProgressCtrl()->SetPos(pp);	// 10%����
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
//	�Z�ʑ�

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

	// ��`�̈���߸�ٍ��W�ɕϊ�
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

	// �̈�m��
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
			nSize  *= NCXYZ;				// XYZ���W�i�[
			m_pfXYZ = new GLfloat[nSize*2];	// ��ʂƒ��
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

	// ��`�̈�����߽�l���擾�i�߸�ْP�ʁj
	::glPixelStorei(GL_PACK_ALIGNMENT, 1);
	::glReadPixels(m_wx, m_wy, m_icx, m_icy,
					GL_DEPTH_COMPONENT, GL_FLOAT, m_pfDepth);
#ifdef _DEBUG
	DWORD	t3 = ::timeGetTime();
	GetGLError();
	printf( "glReadPixels(GL_DEPTH_COMPONENT)=%d[ms]\n", t3 - t2);
#endif
	if ( enStencil != DP_NoStencil ) {
		// ��`�̈�̽�ݼْl���擾
		::glReadPixels(m_wx, m_wy, m_icx, m_icy,
					GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, m_pbStencil);
#ifdef _DEBUG
		DWORD	t31 = ::timeGetTime();
		GetGLError();
		printf( "glReadPixels(GL_STENCIL_INDEX)=%d[ms]\n", t31 - t3);
		t3 = t31;
#endif
	}

	// ܰ���ʒ�ʂƐ؍�ʂ̍��W�o�^
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
					&cdm.wx, &cdm.wy, &cdm.wz);	// ����قǒx���Ȃ��̂Ŏ���ϊ��͒��~
			// ���ꂼ��̍��W�o�^��
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
		return TRUE;	// �X�e���V�������̏ꍇ�͂����܂�

	//
	if ( GetDocument()->GetTraceMode() == ID_NCVIEW_TRACE_STOP )
		AfxGetNCVCMainWnd()->GetProgressCtrl()->SetPos(90);	


	return TRUE;
}

void CNCViewGL::GetClipDepthMill_All(const ARGVCLIPDEPTH& a)
{
	// ܰ��ލ��W
	m_pfXYZ[a.tp+NCA_X] = m_pfXYZ[a.bm+NCA_X] = (GLfloat)a.wx;
	m_pfXYZ[a.tp+NCA_Y] = m_pfXYZ[a.bm+NCA_Y] = (GLfloat)a.wy;
	m_pfXYZ[a.tp+NCA_Z] = min((GLfloat)a.wz, m_rcDraw.high);	// ��ʂ𒴂��Ȃ�
	m_pfXYZ[a.bm+NCA_Z] = m_rcDraw.low;	// ��ʂ�m_rcDraw.low���W�ŕ~���l�߂�
	// ��̫�Ă̖@���޸��
	m_pfNOR[a.tp+NCA_X] =  0.0f;
	m_pfNOR[a.tp+NCA_Y] =  0.0f;
	m_pfNOR[a.tp+NCA_Z] =  1.0f;
	m_pfNOR[a.bm+NCA_X] =  0.0f;
	m_pfNOR[a.bm+NCA_Y] =  0.0f;
	m_pfNOR[a.bm+NCA_Z] = -1.0f;
}

void CNCViewGL::GetClipDepthMill_Zonly(const ARGVCLIPDEPTH& a)
{
	// ���Z�l�̂�
	m_pfXYZ[a.tp+NCA_Z] = min((GLfloat)a.wz, m_rcDraw.high);
}

void CNCViewGL::GetClipDepthMill_BottomStencil(const ARGVCLIPDEPTH& a)
{
	// �X�e���V���l��"1"�̂Ƃ������{�N�Z���擾
	// �����łȂ��Ƃ���� FLT_MAX(�����l) ���Z�b�g���Ċђʂ�����
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
	// �X�e���V���l��"0"���傫���Ƃ������{�N�Z���擾
	// �����łȂ��Ƃ���� FLT_MAX(�����l) ���Z�b�g���Ċђʂ�����
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
//	�~��

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
	printf("left,  top   =(%f, %f)\n", m_rcDraw.left,  m_rcDraw.top);
	printf("right, bottom=(%f, %f)\n", m_rcDraw.right, m_rcDraw.bottom);
	printf("wx1,   wy1   =(%f, %f)\n", wx1, wy1);
	printf("wx2,   wy2   =(%f, %f)\n", wx2, wy2);
	printf("icx=%d icy=%d\n", icx, icy);
	DWORD	t1 = ::timeGetTime();
#endif

	// �̈�m��
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

	// �ײ��ė̈�����߽�l���擾�i�߸�ْP�ʁj
	::glPixelStorei(GL_PACK_ALIGNMENT, 1);
	::glReadPixels(m_wx, m_wy, m_icx, m_icy,
					GL_DEPTH_COMPONENT, GL_FLOAT, m_pfDepth);
#ifdef _DEBUG
	DWORD	t3 = ::timeGetTime();
	GetGLError();
	printf( "glReadPixels()=%d[ms]\n", t3 - t2);
#endif

	// ܰ���ʒ�ʂƐ؍�ʂ̍��W�o�^
	if ( bRecalc ) {
		for ( rx=ry=0, nnu=0, nnd=nSize; rx<ox; rx+=sx, ry+=sy ) {
			nnu0 = nnu;
			nnd0 = nnd;
			for ( j=0, q=0; j<CYCLECOUNT; j++, q+=CYCLESTEP, nnu+=NCXYZ, nnd+=NCXYZ ) {
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
			// �~������Ɓi1�������W�o�^���Ȃ���ø��������܂��\��Ȃ��j
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

	// ���_���ޯ���̏���
	if ( m_pSolidElement ) {
		::glDeleteBuffers(GetElementSize(), m_pSolidElement);
		delete[]	m_pSolidElement;
		m_pSolidElement = NULL;
	}
	m_vElementWrk.clear();
	m_vElementCut.clear();

	// ����
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
		// CPU���گ�ދN��
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

	// �گ�ގ��s�w��
	n = cy / proc;	// 1CPU������̏�����
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
			// �گ�ވُ�I���̌�n��
			EndOfCreateElementThread();
		}
	}

	// ܰ����ʂ͖@���޸�ق̍X�V������
	// �؍�ʏ����Ƃ̔r��������l��
	if ( !n || !CreateElementSide(&m_pCeParam[0], GetDocument()->IsDocFlag(NCDOC_CYLINDER)) ) {
		return FALSE;
	}

	GetGLError();	// error flash

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

	// ���_���ޯ����GPU��؂ɓ]��
#ifndef _WIN64
	try {	// 32bit�ł�������
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
		// �؍�ʗp
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
		// ܰ���`�p
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
	nVertex = icx * icy * 2 * 2;	// (X,Y) ��ʂƒ��

#ifndef _WIN64
	try {	// 32bit�ł�������
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
	BOOL		bStartDraw;	// �n�_�̕`�悪�K�v���ǂ���
	CString		strEvent;

	strEvent.Format(g_szCBFT_S, pParam->nID);
	HANDLE		hStart = OpenEvent(EVENT_ALL_ACCESS, FALSE, strEvent);
	strEvent.Format(g_szCBFT_E, pParam->nID);
	HANDLE		hEnd   = OpenEvent(EVENT_ALL_ACCESS, FALSE, strEvent);

	try {
		while ( TRUE ) {
			// �گ�ގ��s���҂�
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
			// �I���C�x���g
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
			// �گ�ގ��s���҂�
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
			// �؍�ʂ̒��_���ޯ������
			CreateElementCut(pParam);
#ifdef _DEBUG
			t2 = timeGetTime();
			tt += t2 - t1;
			printf("--- ThreadID=%d CreateElementCut() End %d[ms]\n",
				pParam->dbgID, t2 - t1);
			t1 = t2;
#endif
			// ܰ���ʂ̒��_���ޯ������
			CreateElementTop(pParam);
#ifdef _DEBUG
			t2 = timeGetTime();
			tt += t2 - t1;
			printf("--- ThreadID=%d CreateElementTop() End %d[ms]\n",
				pParam->dbgID, t2 - t1);
			t1 = t2;
#endif
			// ܰ���ʂ̒��_���ޯ������
			CreateElementBtm(pParam);
#ifdef _DEBUG
			t2 = timeGetTime();
			tt += t2 - t1;
			printf("--- ThreadID=%d CreateElementBtm() End %d[ms] Total %d[ms]\n",
				pParam->dbgID, t2 - t1, tt);
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
	printf("CreateElementThread() ThreadID=%d thread end.\n",
		pParam->dbgID);
#endif
	pParam->evEnd.SetEvent();
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
						if ( pParam->pfXYZ[z1] < FLT_MAX )
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
					if ( pParam->pfXYZ[z0] < FLT_MAX )
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
						if ( pParam->pfXYZ[z1] < FLT_MAX )
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
					if ( pParam->pfXYZ[z0] < FLT_MAX )
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
			if ( (pParam->pbStl&&pParam->pbStl[n0]==1) || (!pParam->pbStl&&z0z>=pParam->h) ) {
				if ( (pParam->pbStl&&pParam->pbStl[n1]==1) || (!pParam->pbStl&&z1z>=pParam->h) ) {
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
				if ( (pParam->pbStl&&pParam->pbStl[n1]==1) || (!pParam->pbStl&&z1z>=pParam->h) ) {
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
			if ( z0z < FLT_MAX ) {
				vElement.push_back(n0);
				if ( z1z < FLT_MAX )
					vElement.push_back(n1);
			}
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
			GLfloat		nor = -1.0f;
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
				nor = -nor;	// �������]
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
