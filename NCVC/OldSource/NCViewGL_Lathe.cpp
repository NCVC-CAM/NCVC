//	NCViewGL_Lathe.cpp : 旋盤加工回転ﾓﾃﾞﾘﾝｸﾞ
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NCVC.h"
#include "NCdata.h"
#include "NCDoc.h"
#include "NCViewGL.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#include <mmsystem.h>			// timeGetTime()
#endif

using std::vector;
using namespace boost;

/////////////////////////////////////////////////////////////////////////////

BOOL CNCViewGL::CreateLathe(BOOL bRange)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CreateLathe()\nStart");
#endif
	// FBO
	if ( !m_pFBO && GLEW_EXT_framebuffer_object ) {
		m_pFBO = new CFrameBuffer(m_cx, m_cy, TRUE);
		if ( m_pFBO->IsBind() ) {
			::glClearDepth(0.0);
			::glClear(GL_DEPTH_BUFFER_BIT);
		}
		else {
			// FBO使用中止
			delete	m_pFBO;
			m_pFBO = NULL;
		}
	}

	// ﾎﾞｸｾﾙ生成のための初期設定
	InitialBoxel();		// m_pFBO->Bind(TRUE)
	::glOrtho(m_rcDraw.left, m_rcDraw.right,
		m_rcView.low, m_rcView.high,	// top と bottom は使用不可
		m_rcView.low, m_rcView.high);	// m_rcDraw ではｷﾞﾘｷﾞﾘなので m_rcView を使う
//		m_rcDraw.low, m_rcDraw.high);	// ﾃﾞﾌﾟｽ値の更新はｷﾞﾘｷﾞﾘの範囲で精度よく -> 0.0～1.0
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

	// 旋盤用ZXﾜｲﾔｰの描画
	// --- ﾃﾞｰﾀ量からしてCreateBoxel()みたいにﾏﾙﾁｽﾚｯﾄﾞ化は必要ないでしょう
	CNCdata*	pData;
	::glPushAttrib( GL_LINE_BIT );
	::glLineWidth( 3.0 );	// 1Pixelではﾃﾞﾌﾟｽ値を拾えないかもしれないので
	for ( i=s; i<e; i++ ) {
		pData = GetDocument()->GetNCdata(i);
		// 外径データだけ描画
		if ( !(pData->GetValFlags() & NCFLG_LATHE_INSIDE) )
			pData->DrawGLLatheFace();
	}
/*
	if ( m_pFBO &&
		( GetDocument()->IsDocFlag(NCDOC_LATHE_INSIDE) || GetDocument()->IsDocFlag(NCDOC_LATHE_HOLE) ) ) {
		if ( m_pFBO->Bind(TRUE) ) {
			for ( i=s; i<e; i++ ) {
				pData = GetDocument()->GetNCdata(i);
				// 内径データだけ描画
				if ( pData->GetValFlags() & NCFLG_LATHE_INSIDE )
					pData->DrawGLLatheFace();
			}
		}
	}
*/
	::glPopAttrib();
	::glFinish();

	// ﾃﾞﾌﾟｽ値の取得
	BOOL	bResult = GetClipDepthLathe();

	FinalBoxel();

	if ( !bResult )
		ClearVBO();
	else {
		// 頂点配列ﾊﾞｯﾌｧｵﾌﾞｼﾞｪｸﾄ生成
		bResult = CreateVBOLathe();
	}

	return bResult;
}

BOOL CNCViewGL::GetClipDepthLathe(void)
{
#ifdef _DEBUG
	CMagaDbg	dbg("GetClipDepthLathe()");
#endif
	int			i, j, jj, icx;
	float		q;
	GLint		viewPort[4];
	GLdouble	mvMatrix[16], pjMatrix[16],
				wx1, wy1, wz1, wx2, wy2, wz2;
	GLfloat		fz, fx, fxb;
	optional<GLfloat>	fzb;

	::glGetIntegerv(GL_VIEWPORT, viewPort);
	::glGetDoublev (GL_MODELVIEW_MATRIX,  mvMatrix);
	::glGetDoublev (GL_PROJECTION_MATRIX, pjMatrix);

	// 矩形領域(left/right)をﾋﾟｸｾﾙ座標に変換
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

	if ( m_icx!=icx ) {
		if ( m_pfDepth ) {
			delete[]	m_pfDepth;
			delete[]	m_pfXYZ;
			delete[]	m_pfNOR;
			m_pfDepth = m_pfXYZ = m_pfNOR = NULL;
		}
		m_icx = icx;
		m_icy = 1;
	}

	// 領域確保
	if ( !m_pfDepth ) {
		try {
			m_pfDepth = new GLfloat[m_icx];
			m_pfXYZ   = new GLfloat[(m_icx*(ARCCOUNT+1)+(ARCCOUNT+1)*2+2)*NCXYZ];	// 端面含む
			m_pfNOR   = new GLfloat[(m_icx*(ARCCOUNT+1)+(ARCCOUNT+1)*2+2)*NCXYZ];
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

	// ｸﾗｲｱﾝﾄ領域のﾃﾞﾌﾟｽ値を取得（ﾋﾟｸｾﾙ単位）
#ifdef _DEBUG
	DWORD	t1 = ::timeGetTime();
#endif
	::glPixelStorei(GL_PACK_ALIGNMENT, 1);
	::glReadPixels(m_wx, m_wy, m_icx, 1,
					GL_DEPTH_COMPONENT, GL_FLOAT, m_pfDepth);
#ifdef _DEBUG
	DWORD	t2 = ::timeGetTime();
	GetGLError(__LINE__);
	dbg.printf( "glReadPixels()=%d[ms]", t2 - t1);
#endif

	// 外径切削面の座標登録
	for ( i=0, jj=0; i<m_icx; i++ ) {
		::gluUnProject(i+wx1, wy1, m_pfDepth[i],
				mvMatrix, pjMatrix, viewPort,
				&wx2, &wy2, &wz2);	// それほど遅くないので自作変換は中止
		fz = (GLfloat)fabs( m_pfDepth[i]==0.0f ?	// ﾃﾞﾌﾟｽ値が初期値(矩形範囲内で切削面でない)なら
				m_rcDraw.high :				// ﾜｰｸ半径値
				min(wz2, m_rcDraw.high) );	// 変換座標(==半径)かﾜｰｸ半径の小さい方
		// ﾃｰﾊﾟｰ状の法線ﾍﾞｸﾄﾙを計算
		if ( fzb && fz != *fzb ) {
			q = fz - *fzb;
			fx = cos( atan2(q, (float)wx2-fxb) + RAD(90.0f) );
		}
		else
			fx = 0;
		// fz を半径に円筒形の座標を生成（1周分座標登録しないとﾃｸｽﾁｬがうまく貼れない）
		for ( j=0, q=0; j<=ARCCOUNT; j++, q+=ARCSTEP, jj+=NCXYZ ) {
			// ﾃﾞﾌｫﾙﾄの法線ﾍﾞｸﾄﾙ
			m_pfNOR[jj+NCA_X] = fx;
			m_pfNOR[jj+NCA_Y] = cos(q);
			m_pfNOR[jj+NCA_Z] = sin(q);
			// ﾜｰﾙﾄﾞ座標
			m_pfXYZ[jj+NCA_X] = (GLfloat)wx2;
			m_pfXYZ[jj+NCA_Y] = fz * m_pfNOR[jj+NCA_Y];
			m_pfXYZ[jj+NCA_Z] = fz * m_pfNOR[jj+NCA_Z];
		}
		// 前回値を保存
		fzb = fz;
		fxb = (GLfloat)wx2;
	}
	// 端面座標と法線
	m_pfNOR[jj+NCA_X] = -1.0f;
	m_pfXYZ[jj+NCA_X] = m_pfXYZ[0];
	m_pfXYZ[jj+NCA_Y] = m_pfXYZ[jj+NCA_Z] = m_pfNOR[jj+NCA_Y] = m_pfNOR[jj+NCA_Z] = 0.0f;
	for ( i=0, j=0, jj+=NCXYZ; i<=ARCCOUNT; i++, j+=NCXYZ, jj+=NCXYZ ) {
		m_pfNOR[jj+NCA_X] = -1.0f;
		m_pfNOR[jj+NCA_Y] = m_pfNOR[jj+NCA_Z] = 0.0f;
		m_pfXYZ[jj+NCA_X] = m_pfXYZ[0];
		m_pfXYZ[jj+NCA_Y] = m_pfXYZ[j+NCA_Y];
		m_pfXYZ[jj+NCA_Z] = m_pfXYZ[j+NCA_Z];
	}
	m_pfNOR[jj+NCA_X] = 1.0f;
	m_pfXYZ[jj+NCA_X] = fxb;
	m_pfXYZ[jj+NCA_Y] = m_pfXYZ[jj+NCA_Z] = m_pfNOR[jj+NCA_Y] = m_pfNOR[jj+NCA_Z] = 0.0f;
	for ( i=0, j=(m_icx-1)*(ARCCOUNT+1)*NCXYZ, jj+=NCXYZ; i<=ARCCOUNT; i++, j+=NCXYZ, jj+=NCXYZ ) {
		m_pfNOR[jj+NCA_X] = 1.0f;
		m_pfNOR[jj+NCA_Y] = m_pfNOR[jj+NCA_Z] = 0.0f;
		m_pfXYZ[jj+NCA_X] = fxb;
		m_pfXYZ[jj+NCA_Y] = m_pfXYZ[j+NCA_Y];
		m_pfXYZ[jj+NCA_Z] = m_pfXYZ[j+NCA_Z];
	}

#ifdef _DEBUG
	DWORD	t3 = ::timeGetTime();
	dbg.printf( "AddMatrix=%d[ms]", t3 - t2 );
#endif

	return TRUE;
}

BOOL CNCViewGL::CreateVBOLathe(void)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CreateVBOLathe()", DBG_BLUE);
#endif
	int			i, j, jj;
	GLsizeiptr	nVBOsize = (m_icx*(ARCCOUNT+1)+(ARCCOUNT+1)*2+2)*NCXYZ*sizeof(GLfloat);
	GLuint		n0, n1;
	GLenum		errCode;
	UINT		errLine;
	vector<CVelement>	vvElementWrk,	// 頂点配列ｲﾝﾃﾞｯｸｽ(可変長２次元配列)
						vvElementCut,
						vvElementEdg;
	CVelement	vElement;

	// 頂点ｲﾝﾃﾞｯｸｽの消去
	if ( m_pSolidElement ) {
		::glDeleteBuffers(GetElementSize(), m_pSolidElement);
		delete[]	m_pSolidElement;
		m_pSolidElement = NULL;
	}
	m_vElementWrk.clear();
	m_vElementCut.clear();
	m_vElementEdg.clear();

	// 準備
	vvElementWrk.reserve(m_icx+1);
	vvElementCut.reserve(m_icx+1);
	vElement.reserve( (ARCCOUNT+1)*2 );

	// iとi+1の座標を円筒形につなげる
	for ( i=0, jj=0; i<m_icx-1; i++ ) {
		vElement.clear();
		for ( j=0; j<=ARCCOUNT; j++, jj++ ) {
			n0 =  i    * (ARCCOUNT+1) + j;
			n1 = (i+1) * (ARCCOUNT+1) + j;
			vElement.push_back(n0);
			vElement.push_back(n1);
		}
		if ( m_pfDepth[i+1] == 0.0f )	// 切削面かﾜｰｸ面か
			vvElementWrk.push_back(vElement);
		else
			vvElementCut.push_back(vElement);
	}
	// 端面のｲﾝﾃﾞｯｸｽ
	for ( i=0, jj+=ARCCOUNT+1; i<2; i++ ) {
		vElement.clear();
		vElement.push_back(jj);
		for ( j=0, jj++; j<=ARCCOUNT; j++, jj++ ) {
			vElement.push_back(jj);
		}
		vvElementEdg.push_back(vElement);
	}

	GetGLError(__LINE__);	// error flash

	// 頂点配列と法線ﾍﾞｸﾄﾙをGPUﾒﾓﾘに転送
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
	errLine = __LINE__;
	if ( (errCode=GetGLError(errLine)) != GL_NO_ERROR ) {	// GL_OUT_OF_MEMORY
		::glBindBuffer(GL_ARRAY_BUFFER, 0);
		ClearVBO();
		OutputGLErrorMessage(errCode, errLine);
		return FALSE;
	}
	::glBindBuffer(GL_ARRAY_BUFFER, 0);

	// 頂点ｲﾝﾃﾞｯｸｽをGPUﾒﾓﾘに転送
	try {
		size_t	jj = 0,
				nElement,
				nWrkSize = vvElementWrk.size(),
				nCutSize = vvElementCut.size(),
				nEdgSize = 2;

		m_pSolidElement = new GLuint[nWrkSize+nCutSize+nEdgSize];
		::glGenBuffers((GLsizei)(nWrkSize+nCutSize+nEdgSize), m_pSolidElement);
		errLine = __LINE__;
		if ( (errCode=GetGLError(errLine)) != GL_NO_ERROR ) {
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

		// 切削面用
		for ( const auto& v : vvElementCut ) {
			nElement = v.size();
			::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_pSolidElement[jj++]);
			if ( (errCode=GetGLError(__LINE__)) == GL_NO_ERROR ) {
				::glBufferData(GL_ELEMENT_ARRAY_BUFFER,
					nElement*sizeof(GLuint), &(v[0]),
					GL_STATIC_DRAW);
//					GL_DYNAMIC_DRAW);
				errLine = __LINE__;
				errCode = GetGLError(errLine);
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
		// ﾜｰｸ矩形用
		for ( const auto& v : vvElementWrk ) {
			nElement = v.size();
			::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_pSolidElement[jj++]);
			if ( (errCode=GetGLError(__LINE__)) == GL_NO_ERROR ) {
				::glBufferData(GL_ELEMENT_ARRAY_BUFFER,
					nElement*sizeof(GLuint), &(v[0]),
					GL_STATIC_DRAW);
//					GL_DYNAMIC_DRAW);
				errLine = __LINE__;
				errCode = GetGLError(errLine);
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
		// 端面
		for ( const auto& v : vvElementEdg ) {
			nElement = v.size();
			::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_pSolidElement[jj++]);
			if ( (errCode=GetGLError(__LINE__)) == GL_NO_ERROR ) {
				::glBufferData(GL_ELEMENT_ARRAY_BUFFER,
					nElement*sizeof(GLuint), &(v[0]),
					GL_STATIC_DRAW);
//					GL_DYNAMIC_DRAW);
				errLine = __LINE__;
				errCode = GetGLError(errLine);
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
		::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

#ifdef _DEBUG
		dbg.printf("VertexCount(/3)=%d size=%d",
			m_icx*ARCCOUNT, m_icx*ARCCOUNT*NCXYZ*sizeof(GLfloat));
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

void CNCViewGL::CreateTextureLathe(void)
{
	if ( m_icx<=0 )
		return;

	int			i, j, n,
				icx = m_icx-1;					// 分母調整
	GLsizeiptr	nVertex = m_icx*(ARCCOUNT+1)*2;	// Xと円筒座標分
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

	// ﾃｸｽﾁｬ座標の割り当て
	for ( i=0, n=0; i<m_icx; i++ ) {
		ft = (GLfloat)i/icx;
		for ( j=0; j<=ARCCOUNT; j++, n+=2 ) {
			// ﾃｸｽﾁｬ座標(0.0～1.0)
			pfTEX[n]   = ft;
			pfTEX[n+1] = (GLfloat)j/ARCCOUNT;
		}
	}

	// ﾃｸｽﾁｬ座標をGPUﾒﾓﾘに転送
	ASSERT( n == nVertex );
	CreateTexture(nVertex, pfTEX);

	delete[]	pfTEX;
}
