//	NCViewGL_Wire.cpp : ܲԕ��d���H�@�p������ݸ�
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
#endif

using std::vector;
extern	const PENSTYLE	g_penStyle[];	// ViewOption.cpp

/////////////////////////////////////////////////////////////////////////////

BOOL CNCViewGL::CreateWire(void)
{
#ifdef _DEBUG
	printf("CNCViewGL::CreateWire() Start\n");
#endif
	const CViewOption*	pOpt = AfxGetNCVCApp()->GetViewOption();
	INT_PTR		i, nLoop = GetDocument()->GetTraceDraw();
	BOOL		bStart = TRUE;
	GLenum		errCode;
	UINT		errLine;
	CNCdata*	pData;
	CVelement	vef;	// �ʐ����p���_���ޯ��
	WIRELINE	wl;		// ���`��p
	float		dLen;	// ø����������蓖��

	// ��޼ު�Ă̒�����񂪕K�v�Ȃ��߁A�؍펞�Ԍv�Z�گ�ޏI���҂�
	GetDocument()->WaitCalcThread(TRUE);

	// ���_���ޯ���̏���
	if ( m_pSolidElement ) {
		::glDeleteBuffers(GetElementSize(), m_pSolidElement);
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
	m_WireDraw.clear();		// ���������ر

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
		if ( !vef.empty() ) {	// �����͋���ۂ̂͂�
			m_WireDraw.vvef.push_back(vef);
			vef.clear();
		}
		if ( !wl.vel.empty() ) {
			m_WireDraw.vwl.push_back(wl);
			wl.vel.clear();
		}
		if ( i >= nLoop )
			break;
		// �ʌ`���i�؍��ް��j
		wl.col = pOpt->GetNcDrawColor(NCCOL_G1);
		wl.pattern = g_penStyle[pOpt->GetNcDrawType(NCCOLLINE_G1)].nGLpattern;
		bStart = pData->AddGLWireVertex(m_WireDraw.vpt, m_WireDraw.vnr, vef, wl, bStart);
		dLen = pData->GetWireObj() ? 
						max(pData->GetCutLength(), pData->GetWireObj()->GetCutLength()) :
						pData->GetCutLength();
		for ( i++; i<nLoop; i++ ) {
			pData = GetDocument()->GetNCdata(i);
			if ( pData->IsCutCode() ) {
				bStart = pData->AddGLWireVertex(m_WireDraw.vpt, m_WireDraw.vnr, vef, wl, bStart);
				dLen += pData->GetWireObj() ? 
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
		if ( dLen > 0.0f ) {
			m_WireDraw.vlen.push_back(dLen);
		}
		if ( i < nLoop ) 
			bStart = pData->AddGLWireVertex(m_WireDraw.vpt, m_WireDraw.vnr, vef, wl, bStart);	// ��
	}
	// �ړ��ް��Ŕ����Ă���(���̕��̓o�^)
	if ( !wl.vel.empty() ) {
		wl.col = pOpt->GetNcDrawColor(NCCOL_G0);
		wl.pattern = g_penStyle[pOpt->GetNcDrawType(NCCOLLINE_G0)].nGLpattern;
		m_WireDraw.vwl.push_back(wl);
		wl.vel.clear();
	}

#ifdef _DEBUG
	printf(" VertexCount=%d NormalCount=%d\n", m_WireDraw.vpt.size()/NCXYZ, m_WireDraw.vnr.size()/NCXYZ);
	printf(" Area Count=%d\n", m_WireDraw.vvef.size());
	printf(" WireLineCount=%d\n", m_WireDraw.vwl.size());
	int	dbgMaxIndex = 0;
	for ( const auto& v : m_WireDraw.vvef ) {
		int dbgMax = *std::max_element(v.begin(), v.end());
		if ( dbgMaxIndex < dbgMax )
			dbgMaxIndex = dbgMax;
	}
	printf(" Max Face Index=%d\n", dbgMaxIndex);
	dbgMaxIndex = 0;
	for ( const auto&v : m_WireDraw.vwl ) {
		int dbgMax = *std::max_element(v.vel.begin(), v.vel.end());
		if ( dbgMaxIndex < dbgMax )
			dbgMaxIndex = dbgMax;
	}
	printf(" Max Line Index=%d\n", dbgMaxIndex);
#endif
	if ( m_nVertexID[0] > 0 )
		::glDeleteBuffers(SIZEOF(m_nVertexID), m_nVertexID);
	if ( m_WireDraw.vpt.empty() )
		return FALSE;

	GetGLError();		// error flash
	::glGenBuffers(SIZEOF(m_nVertexID), m_nVertexID);

	// ���_�z���GPU��؂ɓ]��
	::glBindBuffer(GL_ARRAY_BUFFER, m_nVertexID[0]);
	::glBufferData(GL_ARRAY_BUFFER, m_WireDraw.vpt.size()*sizeof(GLfloat),
		&(m_WireDraw.vpt[0]), GL_STATIC_DRAW);
	errLine = __LINE__;
	if ( (errCode=GetGLError()) != GL_NO_ERROR ) {	// GL_OUT_OF_MEMORY
		::glBindBuffer(GL_ARRAY_BUFFER, 0);
		ClearVBO();
		OutputGLErrorMessage(errCode, errLine);
		return FALSE;
	}

	// �@���޸�ق�GPU��؂ɓ]��
	::glBindBuffer(GL_ARRAY_BUFFER, m_nVertexID[1]);
	::glBufferData(GL_ARRAY_BUFFER, m_WireDraw.vnr.size()*sizeof(GLfloat),
		&(m_WireDraw.vnr[0]), GL_STATIC_DRAW);
	errLine = __LINE__;
	if ( (errCode=GetGLError()) != GL_NO_ERROR ) {
		::glBindBuffer(GL_ARRAY_BUFFER, 0);
		ClearVBO();
		OutputGLErrorMessage(errCode, errLine);
		return FALSE;
	}
	::glBindBuffer(GL_ARRAY_BUFFER, 0);

#ifdef _DEBUG
	printf("VBO(index) transport Start\n");
#endif
	// ���_���ޯ����GPU��؂ɓ]��
#ifndef _WIN64
	try {	// 32bit�ł�������
#endif
		size_t	jj = 0;
		GLuint	nElement;
		GLsizei	nSize = (GLsizei)(m_WireDraw.vvef.size());

		m_pSolidElement = new GLuint[nSize];
		::glGenBuffers(nSize, m_pSolidElement);
		m_vElementCut.reserve(nSize+1);
		for ( const auto& v : m_WireDraw.vvef ) {
			nElement = (GLuint)(v.size());
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
			m_vElementCut.push_back(nElement);
		}

		jj = 0;
		nSize = (GLsizei)(m_WireDraw.vwl.size());
		m_pLocusElement = new GLuint[nSize];
		::glGenBuffers(nSize, m_pLocusElement);
		for ( const auto& v : m_WireDraw.vwl ) {
			::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_pLocusElement[jj++]);
			if ( (errCode=GetGLError()) == GL_NO_ERROR ) {
				::glBufferData(GL_ELEMENT_ARRAY_BUFFER, v.vel.size()*sizeof(GLuint), &(v.vel[0]),
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
		}

		::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
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

void CNCViewGL::CreateTextureWire(void)
{
	if ( m_WireDraw.vpt.empty() )
		return;

	INT_PTR		i, j, n = 0, nLoop = GetDocument()->GetNCsize();
	GLsizeiptr	nVertex;		// ���_����
	int			nResult;
	float		dAccuLength;	// �ݐϒ���
	CNCdata*	pData;
	GLfloat*	pfTEX;

	// ���_���ɍ��킹�Ȃ���ø������W�������
#ifdef _DEBUG
	nVertex = m_WireDraw.vpt.size() / NCXYZ * 2;
#else
	nVertex = m_WireDraw.vpt.size(); // ���Sϰ��݂����ė]���Ɋm��
#endif
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
	for ( i=j=0; i<nLoop; i++, j++ ) {
		// �؍�J�n�_�̌���
		for ( ; i<nLoop; i++ ) {
			pData = GetDocument()->GetNCdata(i);
			pfTEX[n++] = 0.0f;
			pfTEX[n++] = 1.0f;
			if  ( pData->GetWireObj() ) {
				pfTEX[n++] = 0.0f;
				pfTEX[n++] = 0.0f;
			}
			if ( pData->IsCutCode() )
				break;
		}
		dAccuLength = 0.0f;
		// �e��޼ު�Ă��Ƃ�ø������W��o�^
		for ( i++; i<nLoop; i++ ) {
			pData = GetDocument()->GetNCdata(i);
			nResult = pData->AddGLWireTexture(n, dAccuLength, m_WireDraw.vlen[j], pfTEX);
			if ( nResult < 0 ) {
				pfTEX[n++] = 0.0f;
				pfTEX[n++] = 1.0f;
				if ( pData->GetWireObj() ) {
					pfTEX[n++] = 0.0f;
					pfTEX[n++] = 0.0f;
				}
				break;	// ���̐؍�J�n�_����
			}
			n += nResult;
		}
	}

	// ø������W��GPU��؂ɓ]��
	ASSERT( n == nVertex );
	CreateTexture(n, pfTEX);

	delete[]	pfTEX;
}
