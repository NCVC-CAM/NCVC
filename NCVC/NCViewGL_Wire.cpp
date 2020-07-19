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

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#include <mmsystem.h>			// timeGetTime()
#endif

using std::vector;
extern	const PENSTYLE	g_penStyle[];	// ViewOption.cpp

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
	CNCdata*	pData;
	CVelement	vef;	// �ʐ����p���_���ޯ��
	WIRELINE	wl;		// ���`��p
	float		dLen;	// ø����������蓖��

	// ��޼ު�Ă̒�����񂪕K�v�Ȃ��߁A�؍펞�Ԍv�Z�گ�ޏI���҂�
	GetDocument()->WaitCalcThread(TRUE);

	// ���_���ޯ���̏���
	if ( m_pSolidElement ) {
		::glDeleteBuffersARB(GetElementSize(), m_pSolidElement);
		delete[]	m_pSolidElement;
		m_pSolidElement = NULL;
	}
	if ( m_pLocusElement ) {
		::glDeleteBuffersARB((GLsizei)(m_WireDraw.vwl.size()), m_pLocusElement);
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
	dbg.printf("VertexCount=%d NormalCount=%d", m_WireDraw.vpt.size()/NCXYZ, m_WireDraw.vnr.size()/NCXYZ);
	dbg.printf("Area Count=%d", m_WireDraw.vvef.size());
	dbg.printf("WireLineCount=%d", m_WireDraw.vwl.size());
	int	dbgMaxIndex = 0;
	for ( const auto& v : m_WireDraw.vvef ) {
		int dbgMax = *std::max_element(v.begin(), v.end());
		if ( dbgMaxIndex < dbgMax )
			dbgMaxIndex = dbgMax;
	}
	dbg.printf("Max Face Index=%d", dbgMaxIndex);
	dbgMaxIndex = 0;
	for ( const auto&v : m_WireDraw.vwl ) {
		int dbgMax = *std::max_element(v.vel.begin(), v.vel.end());
		if ( dbgMaxIndex < dbgMax )
			dbgMaxIndex = dbgMax;
	}
	dbg.printf("Max Line Index=%d", dbgMaxIndex);
#endif
	if ( m_nVertexID[0] > 0 )
		::glDeleteBuffersARB(SIZEOF(m_nVertexID), m_nVertexID);
	if ( m_WireDraw.vpt.empty() )
		return FALSE;

	GetGLError();		// error flash
	::glGenBuffersARB(SIZEOF(m_nVertexID), m_nVertexID);

	// ���_�z���GPU��؂ɓ]��
	::glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_nVertexID[0]);
	::glBufferDataARB(GL_ARRAY_BUFFER_ARB,
			m_WireDraw.vpt.size()*sizeof(GLfloat), &(m_WireDraw.vpt[0]),
			GL_STATIC_DRAW_ARB);
	errLine = __LINE__;
	if ( (errCode=GetGLError()) != GL_NO_ERROR ) {	// GL_OUT_OF_MEMORY
		::glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
		ClearVBO();
		OutputGLErrorMessage(errCode, errLine);
		return FALSE;
	}

	// �@���޸�ق�GPU��؂ɓ]��
	::glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_nVertexID[1]);
	::glBufferDataARB(GL_ARRAY_BUFFER_ARB,
			m_WireDraw.vnr.size()*sizeof(GLfloat), &(m_WireDraw.vnr[0]),
			GL_STATIC_DRAW_ARB);
	errLine = __LINE__;
	if ( (errCode=GetGLError()) != GL_NO_ERROR ) {
		::glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
		ClearVBO();
		OutputGLErrorMessage(errCode, errLine);
		return FALSE;
	}
	::glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);

#ifdef _DEBUG
	dbg.printf("VBO(index) transport Start");
#endif
	// ���_���ޯ����GPU��؂ɓ]��
	try {
		size_t	jj = 0;
		GLuint	nElement;
		GLsizei	nSize = (GLsizei)(m_WireDraw.vvef.size());

		m_pSolidElement = new GLuint[nSize];
		::glGenBuffersARB(nSize, m_pSolidElement);
		m_vElementCut.reserve(nSize+1);
		for ( const auto& v : m_WireDraw.vvef ) {
			nElement = (GLuint)(v.size());
			::glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, m_pSolidElement[jj++]);
			if ( (errCode=GetGLError()) == GL_NO_ERROR ) {
				::glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB,
					nElement*sizeof(GLuint), &(v[0]),
					GL_STATIC_DRAW_ARB);
				errLine = __LINE__;
				errCode = GetGLError();
			}
			else
				errLine = __LINE__;
			if ( errCode != GL_NO_ERROR ) {
				::glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
				ClearVBO();
				OutputGLErrorMessage(errCode, errLine);
				return FALSE;
			}
			m_vElementCut.push_back(nElement);
		}

		jj = 0;
		nSize = (GLsizei)(m_WireDraw.vwl.size());
		m_pLocusElement = new GLuint[nSize];
		::glGenBuffersARB(nSize, m_pLocusElement);
		for ( const auto& v : m_WireDraw.vwl ) {
			::glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, m_pLocusElement[jj++]);
			if ( (errCode=GetGLError()) == GL_NO_ERROR ) {
				::glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB,
					v.vel.size()*sizeof(GLuint), &(v.vel[0]),
					GL_STATIC_DRAW_ARB);
				errLine = __LINE__;
				errCode = GetGLError();
			}
			else
				errLine = __LINE__;
			if ( errCode != GL_NO_ERROR ) {
				::glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
				ClearVBO();
				OutputGLErrorMessage(errCode, errLine);
				return FALSE;
			}
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
