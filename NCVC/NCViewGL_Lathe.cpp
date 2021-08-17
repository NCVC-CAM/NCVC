//	NCViewGL_Lathe.cpp : 旋盤加工回転ﾓﾃﾞﾘﾝｸﾞ
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

	// ﾎﾞｸｾﾙ生成のための初期設定
	InitialBoxel();		// m_pFBO->Bind(TRUE)
	::glOrtho(m_rcDraw.left, m_rcDraw.right,
		-LATHEHEIGHT, LATHEHEIGHT,
		m_rcView.low, m_rcView.high);	// m_rcDraw ではｷﾞﾘｷﾞﾘなので m_rcView を使う
//		m_rcDraw.low, m_rcDraw.high);	// ﾃﾞﾌﾟｽ値の更新はｷﾞﾘｷﾞﾘの範囲で精度よく -> 0.0～1.0
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
	::glLineWidth( LATHELINEWIDTH*2.0f );	// 端面切削等のYZパスでデプス値を更新できない

	// 中空ﾃﾞﾌﾟｽ
	if ( GetDocument()->IsDocFlag(NCDOC_LATHE_HOLE) ) {
		CRect3F		rc(GetDocument()->GetWorkRectOrg());
		::glBegin(GL_TRIANGLE_STRIP);
			::glVertex3f(m_rcDraw.left,  -LATHEHEIGHT, -rc.low);
			::glVertex3f(m_rcDraw.left,          0.0f, -rc.low);
			::glVertex3f(m_rcDraw.right, -LATHEHEIGHT, -rc.low);
			::glVertex3f(m_rcDraw.right,         0.0f, -rc.low);
		::glEnd();
	}

	// 旋盤用ZXﾜｲﾔｰの描画
	// --- ﾃﾞｰﾀ量からしてCreateBoxel()みたいにﾏﾙﾁｽﾚｯﾄﾞ化は必要ないと思われる
	for ( i=s; i<e; i++ )
		GetDocument()->GetNCdata(i)->DrawGLLatheDepth();

	::glPopAttrib();
//	::glFinish();

	// ﾃﾞﾌﾟｽ値の取得
	BOOL	bResult = GetClipDepthLathe();

	FinalBoxel();

	if ( bResult ) {
		// 頂点配列ﾊﾞｯﾌｧｵﾌﾞｼﾞｪｸﾄ生成
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

	// 矩形領域をﾋﾟｸｾﾙ座標に変換
	::gluProject(m_rcDraw.left, -LATHELINEWIDTH, 0.0, mvMatrix, pjMatrix, viewPort,
		&wx1, &wy1, &wz1);		// 外径Y
	::gluProject(m_rcDraw.right, LATHELINEWIDTH, 0.0, mvMatrix, pjMatrix, viewPort,
		&wx2, &wy2, &wz2);		// 内径Y

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

	// 領域確保
	if ( !m_pfDepth ) {
		size_t	nSize;
		try {
			m_pfDepth	= new GLfloat[m_icx*m_icy];
			// 断面表示でも1周分の座標生成
			nSize  = (ARCCOUNT+1) * m_icx * 2;	// 円周×長さ×[内|外]径
			nSize += (ARCCOUNT+1) * 2 * 2;		// 円周×[内|外]径×左右端面
			if ( m_bSlitView )
				nSize += m_icx * 4;				// 断面用座標
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

	// ｸﾗｲｱﾝﾄ領域のﾃﾞﾌﾟｽとｽﾃﾝｼﾙ値を取得（ﾋﾟｸｾﾙ単位）
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

	offset = m_icx * (m_icy-1);		// m_pfDepthの外径開始ｵﾌｾｯﾄ
	// ワールド座標の取得
	for ( i=0; i<m_icx; i++ ) {
		// 外径のZX値を取得
		::gluUnProject(i+wx1, wy1, m_pfDepth[i+offset],
				mvMatrix, pjMatrix, viewPort,
				&cdm.wx, &cdm.wy, &cdm.wz);
		m_pLatheX[i] = (GLfloat)cdm.wx;
		m_pLatheZo[i] = m_pfDepth[i+offset] == 0.0f ?	// ﾃﾞﾌﾟｽ値が初期値なら
				m_rcDraw.high :								// ﾜｰｸ半径値
				fabs( min((float)cdm.wz, m_rcDraw.high) );	// 変換座標かﾜｰｸ半径の小さい方
		// 内径のZ値を取得
		::gluUnProject(i+wx1, wy2, m_pfDepth[i],
				mvMatrix, pjMatrix, viewPort,
				&cdm.wx, &cdm.wy, &cdm.wz);
		m_pLatheZi[i] = m_pfDepth[i] == 0.0f ?			// ﾃﾞﾌﾟｽ値が初期値なら
				0.0f :										// 原点
				fabs( min((float)cdm.wz, m_rcDraw.high) );	// 変換座標かﾜｰｸ半径の小さい方
	}

#ifdef _DEBUG_FILEOUT_
	DumpLatheZ();
#endif

	// 外径切削面の座標登録
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
			// ﾃｰﾊﾟｰ状の法線ﾍﾞｸﾄﾙを計算
			fx = (fzb && fz != *fzb) ?
					cos( atan2(fz - *fzb, m_pLatheX[i]-fxb) + RAD(90.0f) ) : 0.0f;
			// fz を半径に円筒形の座標を生成（1周分座標登録しないとﾃｸｽﾁｬがうまく貼れない）
			for ( j=0, q=RAD(-90.0f); j<=ARCCOUNT; j++, q+=ARCSTEP, jj+=NCXYZ ) {	// 断面表示ｻﾎﾟｰﾄのため-90°start
				// ﾃﾞﾌｫﾙﾄの法線ﾍﾞｸﾄﾙ
				m_pfNOR[jj+NCA_X] = fx;
				m_pfNOR[jj+NCA_Y] = cos(q);
				m_pfNOR[jj+NCA_Z] = sin(q);
				// ﾜｰﾙﾄﾞ座標
				m_pfXYZ[jj+NCA_X] = m_pLatheX[i];
				m_pfXYZ[jj+NCA_Y] = fz * m_pfNOR[jj+NCA_Y];
				m_pfXYZ[jj+NCA_Z] = fz * m_pfNOR[jj+NCA_Z];
			}
			// 前回値を保存
			fzb = fz;
			fxb = m_pLatheX[i];
		}
		else {
			// 無効値を登録（ｲﾝﾃﾞｯｸｽで無視）
			for ( j=0; j<=ARCCOUNT; j++, jj+=NCXYZ ) {
				m_pfNOR[jj+NCA_X] = m_pfNOR[jj+NCA_Y] = m_pfNOR[jj+NCA_Z] = 0.0f;
				m_pfXYZ[jj+NCA_X] = m_pfXYZ[jj+NCA_Y] = m_pfXYZ[jj+NCA_Z] = FLT_MAX;
			}
			fzb.reset();
		}
	}

	// 内径切削面の座標登録
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

	// 端面座標と法線（座標は計算済み。内径と外径をつなぎ法線ﾍﾞｸﾄﾙを設定）
	offset = (ARCCOUNT+1) * m_icx * NCXYZ;	// 内径座標へのｵﾌｾｯﾄ
	for ( i=j=0; i<=ARCCOUNT; i++, j+=NCXYZ, jj+=NCXYZ ) {
		// 外径左端点
		m_pfXYZ[jj+NCA_X] = m_pfXYZ[j+NCA_X];
		m_pfXYZ[jj+NCA_Y] = m_pfXYZ[j+NCA_Y];
		m_pfXYZ[jj+NCA_Z] = m_pfXYZ[j+NCA_Z];
		m_pfNOR[jj+NCA_X] = -1.0f;
		m_pfNOR[jj+NCA_Y] = m_pfNOR[jj+NCA_Z] = 0.0f;
		// 内径左端点
		jj += NCXYZ;
		m_pfXYZ[jj+NCA_X] = m_pfXYZ[j+offset+NCA_X];
		m_pfXYZ[jj+NCA_Y] = m_pfXYZ[j+offset+NCA_Y];
		m_pfXYZ[jj+NCA_Z] = m_pfXYZ[j+offset+NCA_Z];
		m_pfNOR[jj+NCA_X] = -1.0f;
		m_pfNOR[jj+NCA_Y] = m_pfNOR[jj+NCA_Z] = 0.0f;
	}
	for ( i=0, j=(ARCCOUNT+1)*(m_icx-1)*NCXYZ; i<=ARCCOUNT; i++, j+=NCXYZ, jj+=NCXYZ ) {
		// 外径右端点
		m_pfXYZ[jj+NCA_X] = m_pfXYZ[j+NCA_X];
		m_pfXYZ[jj+NCA_Y] = m_pfXYZ[j+NCA_Y];
		m_pfXYZ[jj+NCA_Z] = m_pfXYZ[j+NCA_Z];
		m_pfNOR[jj+NCA_X] = 1.0f;
		m_pfNOR[jj+NCA_Y] = m_pfNOR[jj+NCA_Z] = 0.0f;
		// 内径右端点
		jj += NCXYZ;
		m_pfXYZ[jj+NCA_X] = m_pfXYZ[j+offset+NCA_X];
		m_pfXYZ[jj+NCA_Y] = m_pfXYZ[j+offset+NCA_Y];
		m_pfXYZ[jj+NCA_Z] = m_pfXYZ[j+offset+NCA_Z];
		m_pfNOR[jj+NCA_X] = 1.0f;
		m_pfNOR[jj+NCA_Y] = m_pfNOR[jj+NCA_Z] = 0.0f;
	}

	if ( m_bSlitView ) {
		// 断面座標の登録（座標は計算済み。内径と外径をつなぎ法線ﾍﾞｸﾄﾙを設定）
		for ( i=j=0; i<m_icx; i++, j+=(ARCCOUNT+1)*NCXYZ, jj+=NCXYZ ) {
			// 外径下側
			m_pfXYZ[jj+NCA_X] = m_pfXYZ[j+NCA_X];
			m_pfXYZ[jj+NCA_Y] = m_pfXYZ[j+NCA_Y];
			m_pfXYZ[jj+NCA_Z] = m_pfXYZ[j+NCA_Z];
			m_pfNOR[jj+NCA_Y] = -1.0f;
			m_pfNOR[jj+NCA_X] = m_pfNOR[jj+NCA_Z] = 0.0f;
			// 内径下側
			jj += NCXYZ;
			m_pfXYZ[jj+NCA_X] = m_pfXYZ[j+offset+NCA_X];
			m_pfXYZ[jj+NCA_Y] = m_pfXYZ[j+offset+NCA_Y];
			m_pfXYZ[jj+NCA_Z] = m_pfXYZ[j+offset+NCA_Z];
			m_pfNOR[jj+NCA_Y] = -1.0f;
			m_pfNOR[jj+NCA_X] = m_pfNOR[jj+NCA_Z] = 0.0f;
		}
		for ( i=0, j=ARCCOUNT/2*NCXYZ; i<m_icx; i++, j+=(ARCCOUNT+1)*NCXYZ, jj+=NCXYZ ) {
			// 外径上側
			m_pfXYZ[jj+NCA_X] = m_pfXYZ[j+NCA_X];
			m_pfXYZ[jj+NCA_Y] = m_pfXYZ[j+NCA_Y];
			m_pfXYZ[jj+NCA_Z] = m_pfXYZ[j+NCA_Z];
			m_pfNOR[jj+NCA_Y] = -1.0f;
			m_pfNOR[jj+NCA_X] = m_pfNOR[jj+NCA_Z] = 0.0f;
			// 内径上側
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
		offset = m_icx * (m_icy-1),	// m_pfDepthの外径開始ｵﾌｾｯﾄ
		nSlit  = m_bSlitView ? (ARCCOUNT/2) : ARCCOUNT;	// 断面表示なら円の半分だけ
	GLsizeiptr	nVBOsize = ( (ARCCOUNT+1)*m_icx*2 + (ARCCOUNT+1)*2*2 + m_icx*4 )
									* NCXYZ * sizeof(GLfloat);
	GLuint		n0, n1;
	GLenum		errCode;
	UINT		errLine;
	vector<CVelement>	vvElementWrk,	// 頂点配列ｲﾝﾃﾞｯｸｽ(可変長２次元配列)
						vvElementCut,
						vvElementEdg,
						vvElementSlt;
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
	m_vElementSlt.clear();

	// 準備
	vvElementWrk.reserve( (m_icx+1)*2 );
	vvElementCut.reserve( (m_icx+1)*2 );
	vvElementSlt.reserve( (m_icx+1)*2 );

	// 外径ｲﾝﾃﾞｯｸｽ(iとi+1の座標を円筒形につなげる)
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
			if ( m_pfDepth[i+offset+1] == 0.0f )	// ﾃﾞﾌﾟｽが初期値->切削面かﾜｰｸ面か
				vvElementWrk.push_back(vElement);
			else
				vvElementCut.push_back(vElement);
		}
	}

	// 内径ｲﾝﾃﾞｯｸｽ
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

	// 端面ｲﾝﾃﾞｯｸｽ
	ii = (ARCCOUNT+1)*m_icx*2;
	for ( i=0; i<2; i++ ) {	// 左右端面
		n0 =  ii    * NCXYZ;
		n1 = (ii+1) * NCXYZ;
		if ( m_pfXYZ[n0]<FLT_MAX && m_pfXYZ[n1]<FLT_MAX ) {
			vElement.clear();
			for ( j=0; j<=nSlit; j++ ) {
				vElement.push_back(ii++);	// 外径
				vElement.push_back(ii++);	// 内径
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

	// 断面ｲﾝﾃﾞｯｸｽ
	if ( m_bSlitView ) {
		ii = (ARCCOUNT+1)*m_icx*2 + (ARCCOUNT+1)*4;
		for ( i=0; i<2; i++ ) {	// 上下断面
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

	// 頂点ｲﾝﾃﾞｯｸｽをGPUﾒﾓﾘに転送
#ifndef _WIN64
	try {	// 32bit版だけﾁｪｯｸ
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
		// 切削面用
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
		// ﾜｰｸ矩形用
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
		// 端面
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

		// 断面
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
				icx = m_icx-1;					// 分母調整
	GLsizeiptr	nVertex = m_icx*(ARCCOUNT+1)*2;	// Xと円筒座標分
	GLfloat		ft;
	GLfloat*	pfTEX;

#ifndef _WIN64
	try {	// 32bit版だけﾁｪｯｸ
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
