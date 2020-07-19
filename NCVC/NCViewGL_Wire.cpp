//	NCViewGL_Wire.cpp : ﾜｲﾔ放電加工機用のﾓﾃﾞﾘﾝｸﾞ
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
	CVelement	vef;	// 面生成用頂点ｲﾝﾃﾞｯｸｽ
	WIRELINE	wl;		// 線描画用
	float		dLen;	// ﾃｸｽﾁｬ長さ割り当て

	// ｵﾌﾞｼﾞｪｸﾄの長さ情報が必要なため、切削時間計算ｽﾚｯﾄﾞ終了待ち
	GetDocument()->WaitCalcThread(TRUE);

	// 頂点ｲﾝﾃﾞｯｸｽの消去
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
	m_WireDraw.clear();		// これも毎回ｸﾘｱ

	// ﾜｲﾔ加工の座標登録
	for ( i=GetDocument()->GetTraceStart(); i<nLoop; i++ ) {
		// 面形成以外のﾙｰﾌﾟ
		wl.col = pOpt->GetNcDrawColor(NCCOL_G0);
		wl.pattern = g_penStyle[pOpt->GetNcDrawType(NCCOLLINE_G0)].nGLpattern;
		for ( ; i<nLoop; i++ ) {
			pData = GetDocument()->GetNCdata(i);
			if ( pData->IsCutCode() )
				break;
			bStart = pData->AddGLWireVertex(m_WireDraw.vpt, m_WireDraw.vnr, vef, wl, bStart);
		}
		if ( !vef.empty() ) {	// ここは空っぽのはず
			m_WireDraw.vvef.push_back(vef);
			vef.clear();
		}
		if ( !wl.vel.empty() ) {
			m_WireDraw.vwl.push_back(wl);
			wl.vel.clear();
		}
		if ( i >= nLoop )
			break;
		// 面形成（切削ﾃﾞｰﾀ）
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
					// 円弧ﾃﾞｰﾀの場合は一旦切らないとGL_LINE_STRIPで次につながらない
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
			bStart = pData->AddGLWireVertex(m_WireDraw.vpt, m_WireDraw.vnr, vef, wl, bStart);	// ※
	}
	// 移動ﾃﾞｰﾀで抜けてくる(※の分の登録)
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

	// 頂点配列をGPUﾒﾓﾘに転送
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

	// 法線ﾍﾞｸﾄﾙをGPUﾒﾓﾘに転送
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
	// 頂点ｲﾝﾃﾞｯｸｽをGPUﾒﾓﾘに転送
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
	GLsizeiptr	nVertex;		// 頂点総数
	int			nResult;
	float		dAccuLength;	// 累積長さ
	CNCdata*	pData;
	GLfloat*	pfTEX;

	// 頂点数に合わせないとﾃｸｽﾁｬ座標がずれる
#ifdef _DEBUG
	nVertex = m_WireDraw.vpt.size() / NCXYZ * 2;
#else
	nVertex = m_WireDraw.vpt.size(); // 安全ﾏｰｼﾞﾝを見て余分に確保
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

	// ﾃｸｽﾁｬ座標の割り当て
	for ( i=j=0; i<nLoop; i++, j++ ) {
		// 切削開始点の検索
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
		// 各ｵﾌﾞｼﾞｪｸﾄごとのﾃｸｽﾁｬ座標を登録
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
				break;	// 次の切削開始点検索
			}
			n += nResult;
		}
	}

	// ﾃｸｽﾁｬ座標をGPUﾒﾓﾘに転送
	ASSERT( n == nVertex );
	CreateTexture(n, pfTEX);

	delete[]	pfTEX;
}
