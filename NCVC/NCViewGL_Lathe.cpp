//	NCViewGL_Lathe.cpp : ���Չ��H��]����ݸ�
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

#ifdef _DEBUG
#define new DEBUG_NEW
#include <mmsystem.h>			// timeGetTime()
//#define	_DEBUG_FILEOUT_		// Depth File out
#endif

using std::vector;
using namespace boost;

#define	IsInside()	(GetDocument()->IsDocFlag(NCDOC_LATHE_INSIDE)||GetDocument()->IsDocFlag(NCDOC_LATHE_HOLE))

/////////////////////////////////////////////////////////////////////////////

BOOL CNCViewGL::CreateLathe(BOOL bRange)
{
#ifdef _DEBUG
	printf("CNCViewGL::CreateLathe() Start\n");
#endif

	// FBO
	CreateFBO();

	// �޸�ِ����̂��߂̏����ݒ�
	InitialBoxel();		// m_pFBO->Bind(TRUE)
	::glOrtho(m_rcDraw.left, m_rcDraw.right,
		-LATHEHEIGHT, LATHEHEIGHT,
		m_rcView.low, m_rcView.high);	// m_rcDraw �łͷ�ط�؂Ȃ̂� m_rcView ���g��
//		m_rcDraw.low, m_rcDraw.high);	// ���߽�l�̍X�V�ͷ�ط�؂͈̔͂Ő��x�悭 -> 0.0�`1.0
	::glMatrixMode(GL_MODELVIEW);

#ifdef _DEBUG
	printf("(%f,%f)-(%f,%f)\n", m_rcDraw.left, m_rcView.low, m_rcDraw.right, m_rcView.high);
	printf("(%f,%f)\n", m_rcView.low, m_rcView.high);
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

	::glPushAttrib( GL_LINE_BIT );
	::glLineWidth( LATHELINEWIDTH*2.0f );	// �[�ʐ؍퓙��YZ�p�X�Ńf�v�X�l���X�V�ł��Ȃ�

	// �������߽
	if ( GetDocument()->IsDocFlag(NCDOC_LATHE_HOLE) ) {
		CRect3F		rc(GetDocument()->GetWorkRectOrg());
		::glBegin(GL_TRIANGLE_STRIP);
			::glVertex3f(m_rcDraw.left,  -LATHEHEIGHT, -rc.low);
			::glVertex3f(m_rcDraw.left,          0.0f, -rc.low);
			::glVertex3f(m_rcDraw.right, -LATHEHEIGHT, -rc.low);
			::glVertex3f(m_rcDraw.right,         0.0f, -rc.low);
		::glEnd();
	}

	// ���՗pZXܲ԰�̕`��
	// --- �ް��ʂ��炵��CreateBoxel()�݂���������گ�މ��͕K�v�Ȃ��Ǝv����
	for ( i=s; i<e; i++ )
		GetDocument()->GetNCdata(i)->DrawGLLatheDepth();

	::glPopAttrib();
//	::glFinish();

	// ���߽�l�̎擾
	BOOL	bResult = GetClipDepthLathe();

	FinalBoxel();

	if ( bResult ) {
		// ���_�z���ޯ̧��޼ު�Đ���
		bResult = CreateVBOLathe();
	}
	else
		ClearVBO();

	return bResult;
}

BOOL CNCViewGL::GetClipDepthLathe(void)
{
	BOOL		bBreak = FALSE;
	int			i, j, jj, icx, icy, offset;
	GLint		viewPort[4];
	GLdouble	mvMatrix[16], pjMatrix[16],
				wx1, wy1, wz1, wx2, wy2, wz2;
	ARGVCLIPDEPTH	cdm;
	GLfloat		fz, fzo, fzi, fx, fxb;
	float		q;
	optional<GLfloat>	fzb;

	::glGetIntegerv(GL_VIEWPORT, viewPort);
	::glGetDoublev (GL_MODELVIEW_MATRIX,  mvMatrix);
	::glGetDoublev (GL_PROJECTION_MATRIX, pjMatrix);

	// ��`�̈���߸�ٍ��W�ɕϊ�
	::gluProject(m_rcDraw.left, -LATHELINEWIDTH, 0.0, mvMatrix, pjMatrix, viewPort,
		&wx1, &wy1, &wz1);		// �O�aY
	::gluProject(m_rcDraw.right, LATHELINEWIDTH, 0.0, mvMatrix, pjMatrix, viewPort,
		&wx2, &wy2, &wz2);		// ���aY

	icx = (int)(wx2 - wx1);
	icy = (int)(wy2 - wy1);
	if ( icx<=0 || icy<=0 )
		return FALSE;
	m_wx = (GLint)wx1;
	m_wy = (GLint)wy1;

#ifdef _DEBUG
	printf("GetClipDepthLathe()\n");
	printf(" left  -> wx1 = %f -> %f\n", m_rcDraw.left, wx1);
	printf(" right -> wx2 = %f -> %f\n", m_rcDraw.right, wx2);
	printf(" wy1 = %f -> %f\n",  LATHELINEWIDTH, wy1);
	printf(" wy2 = %f -> %f\n", -LATHELINEWIDTH, wy2);
	printf(" icx=%d icy=%d\n", icx, icy);
#endif

	if ( m_icx!=icx ) {
		if ( m_pfDepth ) {
			DeleteDepthMemory();
		}
		m_icx = icx;
		m_icy = icy;
	}

	// �̈�m��
	if ( !m_pfDepth ) {
		size_t	nSize;
		try {
			m_pfDepth	= new GLfloat[m_icx*m_icy];
			// �f�ʕ\���ł�1�����̍��W����
			nSize  = (ARCCOUNT+1) * m_icx * 2;	// �~���~�����~[��|�O]�a
			nSize += (ARCCOUNT+1) * 2 * 2;		// �~���~[��|�O]�a�~���E�[��
			if ( m_bSlitView )
				nSize += m_icx * 4;				// �f�ʗp���W
			nSize *= NCXYZ;
			m_pfXYZ		= new GLfloat[nSize];
			m_pfNOR		= new GLfloat[nSize];
			m_pLatheX	= new GLfloat[m_icx];
			m_pLatheZo	= new GLfloat[m_icx];
			m_pLatheZi	= new GLfloat[m_icx];
		}
		catch (CMemoryException* e) {
			AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
			e->Delete();
			DeleteDepthMemory();
			return FALSE;
		}
	}

	// �ײ��ė̈�����߽�ƽ�ݼْl���擾�i�߸�ْP�ʁj
#ifdef _DEBUG
	DWORD	t1 = ::timeGetTime();
#endif
	::glPixelStorei(GL_PACK_ALIGNMENT, 1);
	::glReadPixels(m_wx, m_wy, m_icx, m_icy,
					GL_DEPTH_COMPONENT, GL_FLOAT, m_pfDepth);
#ifdef _DEBUG
	DWORD	t2 = ::timeGetTime();
	GetGLError();
	printf( "glReadPixels()=%d[ms]\n", t2 - t1);
#endif

	offset = m_icx * (m_icy-1);		// m_pfDepth�̊O�a�J�n�̾��
	// ���[���h���W�̎擾
	for ( i=0; i<m_icx; i++ ) {
		// �O�a��ZX�l���擾
		::gluUnProject(i+wx1, wy1, m_pfDepth[i+offset],
				mvMatrix, pjMatrix, viewPort,
				&cdm.wx, &cdm.wy, &cdm.wz);
		m_pLatheX[i] = (GLfloat)cdm.wx;
		m_pLatheZo[i] = m_pfDepth[i+offset] == 0.0f ?	// ���߽�l�������l�Ȃ�
				m_rcDraw.high :								// ܰ����a�l
				fabs( min((float)cdm.wz, m_rcDraw.high) );	// �ϊ����W��ܰ����a�̏�������
		// ���a��Z�l���擾
		::gluUnProject(i+wx1, wy2, m_pfDepth[i],
				mvMatrix, pjMatrix, viewPort,
				&cdm.wx, &cdm.wy, &cdm.wz);
		m_pLatheZi[i] = m_pfDepth[i] == 0.0f ?			// ���߽�l�������l�Ȃ�
				0.0f :										// ���_
				fabs( min((float)cdm.wz, m_rcDraw.high) );	// �ϊ����W��ܰ����a�̏�������
	}

#ifdef _DEBUG_FILEOUT_
	DumpLatheZ();
#endif

	// �O�a�؍�ʂ̍��W�o�^
	for ( i=jj=0; i<m_icx; i++ ) {
		fzo = m_pLatheZo[i];
		fzi = m_pLatheZi[i];
		if ( fzo < fzi ) {
			fz = bBreak ? FLT_MAX : fzi;
			bBreak = TRUE;
		}
		else {
			fz = bBreak ? fzi : fzo;
			bBreak = FALSE;
		}
		if ( fz < FLT_MAX ) {
			// ð�߰��̖@���޸�ق��v�Z
			fx = (fzb && fz != *fzb) ?
					cos( atan2(fz - *fzb, m_pLatheX[i]-fxb) + RAD(90.0f) ) : 0.0f;
			// fz �𔼌a�ɉ~���`�̍��W�𐶐��i1�������W�o�^���Ȃ���ø��������܂��\��Ȃ��j
			for ( j=0, q=RAD(-90.0f); j<=ARCCOUNT; j++, q+=ARCSTEP, jj+=NCXYZ ) {	// �f�ʕ\����߰Ă̂���-90��start
				// ��̫�Ă̖@���޸��
				m_pfNOR[jj+NCA_X] = fx;
				m_pfNOR[jj+NCA_Y] = cos(q);
				m_pfNOR[jj+NCA_Z] = sin(q);
				// ܰ��ލ��W
				m_pfXYZ[jj+NCA_X] = m_pLatheX[i];
				m_pfXYZ[jj+NCA_Y] = fz * m_pfNOR[jj+NCA_Y];
				m_pfXYZ[jj+NCA_Z] = fz * m_pfNOR[jj+NCA_Z];
			}
			// �O��l��ۑ�
			fzb = fz;
			fxb = m_pLatheX[i];
		}
		else {
			// �����l��o�^�i���ޯ���Ŗ����j
			for ( j=0; j<=ARCCOUNT; j++, jj+=NCXYZ ) {
				m_pfNOR[jj+NCA_X] = m_pfNOR[jj+NCA_Y] = m_pfNOR[jj+NCA_Z] = 0.0f;
				m_pfXYZ[jj+NCA_X] = m_pfXYZ[jj+NCA_Y] = m_pfXYZ[jj+NCA_Z] = FLT_MAX;
			}
			fzb.reset();
		}
	}

	// ���a�؍�ʂ̍��W�o�^
	fzb.reset();
	for ( i=0; i<m_icx; i++ ) {
		fzo = m_pLatheZo[i];
		fzi = m_pLatheZi[i];
		fz = fzo < fzi ? FLT_MAX : fzi;
		if ( fz < FLT_MAX ) {
			fx = (fzb && fz != *fzb) ?
					cos( atan2(fz - *fzb, m_pLatheX[i]-fxb) + RAD(90.0f) ) : 0.0f;
			for ( j=0, q=RAD(-90.0f); j<=ARCCOUNT; j++, q+=ARCSTEP, jj+=NCXYZ ) {
				m_pfNOR[jj+NCA_X] = fx;
				m_pfNOR[jj+NCA_Y] = cos(q);
				m_pfNOR[jj+NCA_Z] = sin(q);
				m_pfXYZ[jj+NCA_X] = m_pLatheX[i];
				m_pfXYZ[jj+NCA_Y] = fz * m_pfNOR[jj+NCA_Y];
				m_pfXYZ[jj+NCA_Z] = fz * m_pfNOR[jj+NCA_Z];
			}
			fzb = fz;
			fxb = m_pLatheX[i];
		}
		else {
			for ( j=0; j<=ARCCOUNT; j++, jj+=NCXYZ ) {
				m_pfNOR[jj+NCA_X] = m_pfNOR[jj+NCA_Y] = m_pfNOR[jj+NCA_Z] = 0.0f;
				m_pfXYZ[jj+NCA_X] = m_pfXYZ[jj+NCA_Y] = m_pfXYZ[jj+NCA_Z] = FLT_MAX;
			}
			fzb.reset();
		}
	}

	// �[�ʍ��W�Ɩ@���i���W�͌v�Z�ς݁B���a�ƊO�a���Ȃ��@���޸�ق�ݒ�j
	offset = (ARCCOUNT+1) * m_icx * NCXYZ;	// ���a���W�ւ̵̾��
	for ( i=j=0; i<=ARCCOUNT; i++, j+=NCXYZ, jj+=NCXYZ ) {
		// �O�a���[�_
		m_pfXYZ[jj+NCA_X] = m_pfXYZ[j+NCA_X];
		m_pfXYZ[jj+NCA_Y] = m_pfXYZ[j+NCA_Y];
		m_pfXYZ[jj+NCA_Z] = m_pfXYZ[j+NCA_Z];
		m_pfNOR[jj+NCA_X] = -1.0f;
		m_pfNOR[jj+NCA_Y] = m_pfNOR[jj+NCA_Z] = 0.0f;
		// ���a���[�_
		jj += NCXYZ;
		m_pfXYZ[jj+NCA_X] = m_pfXYZ[j+offset+NCA_X];
		m_pfXYZ[jj+NCA_Y] = m_pfXYZ[j+offset+NCA_Y];
		m_pfXYZ[jj+NCA_Z] = m_pfXYZ[j+offset+NCA_Z];
		m_pfNOR[jj+NCA_X] = -1.0f;
		m_pfNOR[jj+NCA_Y] = m_pfNOR[jj+NCA_Z] = 0.0f;
	}
	for ( i=0, j=(ARCCOUNT+1)*(m_icx-1)*NCXYZ; i<=ARCCOUNT; i++, j+=NCXYZ, jj+=NCXYZ ) {
		// �O�a�E�[�_
		m_pfXYZ[jj+NCA_X] = m_pfXYZ[j+NCA_X];
		m_pfXYZ[jj+NCA_Y] = m_pfXYZ[j+NCA_Y];
		m_pfXYZ[jj+NCA_Z] = m_pfXYZ[j+NCA_Z];
		m_pfNOR[jj+NCA_X] = 1.0f;
		m_pfNOR[jj+NCA_Y] = m_pfNOR[jj+NCA_Z] = 0.0f;
		// ���a�E�[�_
		jj += NCXYZ;
		m_pfXYZ[jj+NCA_X] = m_pfXYZ[j+offset+NCA_X];
		m_pfXYZ[jj+NCA_Y] = m_pfXYZ[j+offset+NCA_Y];
		m_pfXYZ[jj+NCA_Z] = m_pfXYZ[j+offset+NCA_Z];
		m_pfNOR[jj+NCA_X] = 1.0f;
		m_pfNOR[jj+NCA_Y] = m_pfNOR[jj+NCA_Z] = 0.0f;
	}

	if ( m_bSlitView ) {
		// �f�ʍ��W�̓o�^�i���W�͌v�Z�ς݁B���a�ƊO�a���Ȃ��@���޸�ق�ݒ�j
		for ( i=j=0; i<m_icx; i++, j+=(ARCCOUNT+1)*NCXYZ, jj+=NCXYZ ) {
			// �O�a����
			m_pfXYZ[jj+NCA_X] = m_pfXYZ[j+NCA_X];
			m_pfXYZ[jj+NCA_Y] = m_pfXYZ[j+NCA_Y];
			m_pfXYZ[jj+NCA_Z] = m_pfXYZ[j+NCA_Z];
			m_pfNOR[jj+NCA_Y] = -1.0f;
			m_pfNOR[jj+NCA_X] = m_pfNOR[jj+NCA_Z] = 0.0f;
			// ���a����
			jj += NCXYZ;
			m_pfXYZ[jj+NCA_X] = m_pfXYZ[j+offset+NCA_X];
			m_pfXYZ[jj+NCA_Y] = m_pfXYZ[j+offset+NCA_Y];
			m_pfXYZ[jj+NCA_Z] = m_pfXYZ[j+offset+NCA_Z];
			m_pfNOR[jj+NCA_Y] = -1.0f;
			m_pfNOR[jj+NCA_X] = m_pfNOR[jj+NCA_Z] = 0.0f;
		}
		for ( i=0, j=ARCCOUNT/2*NCXYZ; i<m_icx; i++, j+=(ARCCOUNT+1)*NCXYZ, jj+=NCXYZ ) {
			// �O�a�㑤
			m_pfXYZ[jj+NCA_X] = m_pfXYZ[j+NCA_X];
			m_pfXYZ[jj+NCA_Y] = m_pfXYZ[j+NCA_Y];
			m_pfXYZ[jj+NCA_Z] = m_pfXYZ[j+NCA_Z];
			m_pfNOR[jj+NCA_Y] = -1.0f;
			m_pfNOR[jj+NCA_X] = m_pfNOR[jj+NCA_Z] = 0.0f;
			// ���a�㑤
			jj += NCXYZ;
			m_pfXYZ[jj+NCA_X] = m_pfXYZ[j+offset+NCA_X];
			m_pfXYZ[jj+NCA_Y] = m_pfXYZ[j+offset+NCA_Y];
			m_pfXYZ[jj+NCA_Z] = m_pfXYZ[j+offset+NCA_Z];
			m_pfNOR[jj+NCA_Y] = -1.0f;
			m_pfNOR[jj+NCA_X] = m_pfNOR[jj+NCA_Z] = 0.0f;
		}
	}

#ifdef _DEBUG
	DWORD	t3 = ::timeGetTime();
	printf( "AddMatrix=%d[ms] jj=%d\n", t3 - t2, jj );
#endif

#ifdef _DEBUG_FILEOUT_
	DumpDepth();
#endif

	return TRUE;
}

BOOL CNCViewGL::CreateVBOLathe(void)
{
	int	i, ii, j,
		offset = m_icx * (m_icy-1),	// m_pfDepth�̊O�a�J�n�̾��
		nSlit  = m_bSlitView ? (ARCCOUNT/2) : ARCCOUNT;	// �f�ʕ\���Ȃ�~�̔�������
	GLsizeiptr	nVBOsize = ( (ARCCOUNT+1)*m_icx*2 + (ARCCOUNT+1)*2*2 + m_icx*4 )
									* NCXYZ * sizeof(GLfloat);
	GLuint		n0, n1;
	GLenum		errCode;
	UINT		errLine;
	vector<CVelement>	vvElementWrk,	// ���_�z����ޯ��(�ϒ��Q�����z��)
						vvElementCut,
						vvElementEdg,
						vvElementSlt;
	CVelement	vElement;

	// ���_���ޯ���̏���
	if ( m_pSolidElement ) {
		::glDeleteBuffers(GetElementSize(), m_pSolidElement);
		delete[]	m_pSolidElement;
		m_pSolidElement = NULL;
	}
	m_vElementWrk.clear();
	m_vElementCut.clear();
	m_vElementEdg.clear();
	m_vElementSlt.clear();

	// ����
	vvElementWrk.reserve( (m_icx+1)*2 );
	vvElementCut.reserve( (m_icx+1)*2 );
	vvElementSlt.reserve( (m_icx+1)*2 );

	// �O�a���ޯ��(i��i+1�̍��W���~���`�ɂȂ���)
	for ( i=ii=0; i<m_icx-1; i++, ii++ ) {
		n0 =  ii    * (ARCCOUNT+1) * NCXYZ;
		n1 = (ii+1) * (ARCCOUNT+1) * NCXYZ;
		if ( m_pfXYZ[n0]<FLT_MAX && m_pfXYZ[n1]<FLT_MAX ) {
			vElement.clear();
			for ( j=0; j<=nSlit; j++ ) {
				n0 =  ii    * (ARCCOUNT+1) + j;
				n1 = (ii+1) * (ARCCOUNT+1) + j;
				vElement.push_back(n0);
				vElement.push_back(n1);
			}
			if ( m_pfDepth[i+offset+1] == 0.0f )	// ���߽�������l->�؍�ʂ�ܰ��ʂ�
				vvElementWrk.push_back(vElement);
			else
				vvElementCut.push_back(vElement);
		}
	}

	// ���a���ޯ��
	ii++;
	for ( i=0; i<m_icx-1; i++, ii++ ) {
		n0 =  ii    * (ARCCOUNT+1) * NCXYZ;
		n1 = (ii+1) * (ARCCOUNT+1) * NCXYZ;
		if ( m_pfXYZ[n0]<FLT_MAX && m_pfXYZ[n1]<FLT_MAX ) {
			vElement.clear();
			for ( j=0; j<=nSlit; j++ ) {
				n0 =  ii    * (ARCCOUNT+1) + j;
				n1 = (ii+1) * (ARCCOUNT+1) + j;
				vElement.push_back(n0);
				vElement.push_back(n1);
			}
			vvElementCut.push_back(vElement);
		}
	}

	// �[�ʲ��ޯ��
	ii = (ARCCOUNT+1)*m_icx*2;
	for ( i=0; i<2; i++ ) {	// ���E�[��
		n0 =  ii    * NCXYZ;
		n1 = (ii+1) * NCXYZ;
		if ( m_pfXYZ[n0]<FLT_MAX && m_pfXYZ[n1]<FLT_MAX ) {
			vElement.clear();
			for ( j=0; j<=nSlit; j++ ) {
				vElement.push_back(ii++);	// �O�a
				vElement.push_back(ii++);	// ���a
			}
			vvElementEdg.push_back(vElement);
		}
		else {
			for ( j=0; j<=nSlit; j++ )
				ii+=2;
		}
		if ( m_bSlitView )
			ii += ARCCOUNT;
	}

	// �f�ʲ��ޯ��
	if ( m_bSlitView ) {
		ii = (ARCCOUNT+1)*m_icx*2 + (ARCCOUNT+1)*4;
		for ( i=0; i<2; i++ ) {	// �㉺�f��
			vElement.clear();
			for ( j=0; j<m_icx; j++ ) {
				n0 =  ii    * NCXYZ;
				n1 = (ii+1) * NCXYZ;
				if ( m_pfXYZ[n0]<FLT_MAX && m_pfXYZ[n1]<FLT_MAX ) {
					vElement.push_back(ii++);
					vElement.push_back(ii++);
				}
				else {
					ii+=2;
					if ( !vElement.empty() ) {
						vvElementSlt.push_back(vElement);
						vElement.clear();
					}
				}
			}
			if ( !vElement.empty() )
				vvElementSlt.push_back(vElement);
		}
	}

	GetGLError();	// error flash

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
	if ( (errCode=GetGLError()) != GL_NO_ERROR ) {	// GL_OUT_OF_MEMORY
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
				nElement,
				nWrkSize = vvElementWrk.size(),
				nCutSize = vvElementCut.size(),
				nEdgSize = vvElementEdg.size(),
				nSltSize = vvElementSlt.size();

		m_pSolidElement = new GLuint[nWrkSize+nCutSize+nEdgSize+nSltSize];
		::glGenBuffers((GLsizei)(nWrkSize+nCutSize+nEdgSize+nSltSize), m_pSolidElement);
		errLine = __LINE__;
		if ( (errCode=GetGLError()) != GL_NO_ERROR ) {
			::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
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
		for ( const auto& v : vvElementCut ) {
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
		// ܰ���`�p
		for ( const auto& v : vvElementWrk ) {
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
		// �[��
		for ( const auto& v : vvElementEdg ) {
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
			m_vElementEdg.push_back((GLuint)nElement);
#ifdef _DEBUG
			dbgTriangleCut += nElement;
#endif
		}

		// �f��
		for ( const auto&v : vvElementSlt ) {
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
			m_vElementSlt.push_back((GLuint)nElement);
#ifdef _DEBUG
			dbgTriangleCut += nElement;
#endif
		}

		::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
#ifdef _DEBUG
		printf("CreateVBOLathe()\n");
		printf(" VertexCount(/3)=%d size=%d\n",
			m_icx*ARCCOUNT, m_icx*ARCCOUNT*NCXYZ*sizeof(GLfloat));
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
		return FALSE;
	}
#endif

	return TRUE;
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

	// ø������W�̊��蓖��
	for ( i=0, n=0; i<m_icx; i++ ) {
		ft = (GLfloat)i/icx;
		for ( j=0; j<=ARCCOUNT; j++, n+=2 ) {
			// ø������W(0.0�`1.0)
			pfTEX[n]   = ft;
			pfTEX[n+1] = (GLfloat)j/ARCCOUNT;
		}
	}

	// ø������W��GPU��؂ɓ]��
	ASSERT( n == nVertex );
	CreateTexture(nVertex, pfTEX);

	delete[]	pfTEX;
}
