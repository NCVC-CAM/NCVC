// NCViewGL.h : CNCViewGL クラスの宣言およびインターフェイスの定義をします。
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "ViewBaseGL.h"
//#include "GLSL.h"

// Range Parameter
struct RANGEPARAM
{
	INT_PTR		s, e;
	RANGEPARAM(INT_PTR ss, INT_PTR ee) : s(ss), e(ee) {}
};

// CreateBoxel_fromIGES() argument
class	CNCdata;
typedef	boost::variant<CNCdata*, RANGEPARAM>	CREATEBOXEL_IGESPARAM;

// GetClipDepthMill() argument
enum ENCLIPDEPTH
{
	DP_NoStencil, DP_BottomStencil, DP_TopStencil
};

// GetClipDepthXXX() argument
struct ARGVCLIPDEPTH
{
	GLdouble	wx, wy, wz;
	size_t		tp, bm;
};
typedef void (CNCViewGL::*PFNGETCLIPDEPTHMILL)(const ARGVCLIPDEPTH&);

// 底面描画座標生成ｽﾚｯﾄﾞ用
struct CREATEBOTTOMVERTEXPARAM
{
	// CEventを使うとWin7で異常終了。参照ｶｳﾝﾄが原因か？
	// 仕方ないので原始的な方法を採用
	size_t		nID;		// ｽﾚｯﾄﾞID
	BOOL		bThread,
				bResult;
	CNCDoc*		pDoc;
	size_t		s, e;
	CVBtmDraw	vBD;		// from NCdata.h
	CREATEBOTTOMVERTEXPARAM(size_t nID, CNCDoc* pDoc) : nID(nID), pDoc(pDoc), bThread(TRUE), bResult(TRUE) {}
};
typedef	CREATEBOTTOMVERTEXPARAM*	LPCREATEBOTTOMVERTEXPARAM;

// 頂点配列生成ｽﾚｯﾄﾞ用
struct CREATEELEMENTPARAM
{
#ifdef _DEBUG
	int		dbgID;
#endif
	CEvent		evStart;	// 自動
	CEvent		evEnd;		// 手動
	const GLfloat*	pfXYZ;
	GLfloat*		pfNOR;
	const GLubyte*	pbStl;
	BOOL	bThread,
			bResult;
	int		cx, cy,
			cs, ce;
	GLfloat	h, l;
	// 頂点配列ｲﾝﾃﾞｯｸｽ(可変長2次元配列)
	std::vector<CVelement>	vvElementCut,	// from NCdata.h
							vvElementWrk;
	// CEvent を手動ｲﾍﾞﾝﾄにするためのｺﾝｽﾄﾗｸﾀ
	CREATEELEMENTPARAM() : evEnd(FALSE, TRUE), bThread(TRUE), bResult(TRUE) {}
};
typedef	CREATEELEMENTPARAM*		LPCREATEELEMENTPARAM;

enum {
	NCVIEWGLFLG_ACTIVE = 0,
	NCVIEWGLFLG_SIZECHG,
	NCVIEWGLFLG_WIREVIEW,		// 線画表示
	NCVIEWGLFLG_SOLIDVIEW,		// ソリッド表示
	NCVIEWGLFLG_LATHEMODE,		// 断面表示（旋盤モード）
		NCVIEWGLFLG_NUM	// 5
};

class CNCdata;

/////////////////////////////////////////////////////////////////////////////

class CNCViewGL : public CViewBaseGL
{
	CString		m_strGuide;
	std::bitset<NCVIEWGLFLG_NUM>	m_bGLflg;
	int			m_icx, m_icy;	// glDrawPixels
	GLint		m_wx, m_wy;		// glReadPixels, glWindowPos
	CPointF		m_ptCenterBk;		// ﾎﾞｸｾﾙ処理前のﾊﾞｯｸｱｯﾌﾟ
	GLdouble	m_objXformBk[4][4];
	GLuint		m_glCode;		// 切削ﾊﾟｽのﾃﾞｨｽﾌﾟﾚｲﾘｽﾄ
	CNCdata*	m_pData;		// 選択中のNCオブジェクト
	GLfloat*	m_pfDepth;		// ﾃﾞﾌﾟｽ値取得配列
#ifndef NO_TRACE_WORKFILE		// from NCViewTab.h
	GLfloat*	m_pfDepthBottom;// WorkFile 2Pass
#endif
	GLubyte*	m_pbStencil;	// ｽﾃﾝｼﾙ
	GLfloat*	m_pfXYZ;		// -- 変換されたﾜｰﾙﾄﾞ座標(temp area)
	GLfloat*	m_pfNOR;		// -- 法線ﾍﾞｸﾄﾙ
	GLfloat*	m_pLatheX;		// -- 旋盤のX値
	GLfloat*	m_pLatheZo;		// -- 旋盤のZ値外径
	GLfloat*	m_pLatheZi;		// -- 旋盤のZ値内径
	GLsizeiptr	m_nVBOsize;		// 頂点配列ｻｲｽﾞ
	GLuint		m_nVertexID[2],	// 頂点配列と法線ﾍﾞｸﾄﾙ用
				m_nTextureID,	// ﾃｸｽﾁｬ座標用
				m_nPictureID;	// ﾃｸｽﾁｬ画像用
	GLuint*		m_pSolidElement;// 頂点ｲﾝﾃﾞｯｸｽ用
	GLuint*		m_pLocusElement;// 軌跡ｲﾝﾃﾞｯｸｽ用
	CVelement	m_vElementWrk,	// ﾜｰｸ矩形用glDrawElements頂点個数
				m_vElementCut,	// 切削面用
				m_vElementEdg,	// 旋盤端面用
				m_vElementSlt;	// 旋盤断面用
	GLsizei		GetElementSize(void) const {
		return (GLsizei)(m_vElementWrk.size()+m_vElementCut.size()+m_vElementEdg.size()+m_vElementSlt.size());
	}
	WIREDRAW	m_WireDraw;		// ﾜｲﾔ加工機用

#ifdef USE_SHADER
	CGLSL			m_glsl;		// Shader Manager
#endif

	struct {	// CreateElementThread() from CreateVBOMill()
		DWORD	m_nCeProc;
		HANDLE*	m_pCeHandle;
		LPCREATEELEMENTPARAM	m_pCeParam;
	};

	void	InitialObjectForm(void);
	void	UpdateViewOption(void);
	void	CreateDisplayList(void);
	BOOL	CreateBoxel(BOOL = FALSE);
	BOOL	CreateBoxel_fromIGES(CREATEBOXEL_IGESPARAM* = NULL);
	BOOL	CreateLathe(BOOL = FALSE);
	BOOL	CreateWire(void);
	BOOL	CreateBottomFaceThread(BOOL, int);
	BOOL	GetClipDepthMill(ENCLIPDEPTH = DP_NoStencil);
	void	GetClipDepthMill_All(const ARGVCLIPDEPTH&);
	void	GetClipDepthMill_Zonly(const ARGVCLIPDEPTH&);
	void	GetClipDepthMill_BottomStencil(const ARGVCLIPDEPTH&);
	void	GetClipDepthMill_TopStencil(const ARGVCLIPDEPTH&);
	BOOL	GetClipDepthCylinder(void);
	BOOL	GetClipDepthLathe(void);
	BOOL	CreateVBOMill(void);
	BOOL	CreateVBOLathe(void);
	BOOL	ReadTexture(LPCTSTR);
	void	CreateTextureMill(void);
	void	CreateTextureLathe(void);
	void	CreateTextureWire(void);
	void	CreateTexture(GLsizeiptr, const GLfloat*);
	void	ClearVBO(void);
	void	ClearTexture(void);
	void	InitialBoxel(void);
	void	FinalBoxel(void);
	void	DeleteDepthMemory(void);
	void	EndOfCreateElementThread(void);

	void	RenderAxis(void);
	void	RenderCode(RENDERMODE);
	void	RenderMill(const CNCdata*);
	void	DoSelect(const CPoint&);

#ifdef _DEBUG
	void	DumpDepth(void) const;
	void	DumpStencil(void) const;
	void	DumpLatheZ(void) const;
#endif

protected:
	CNCViewGL();
	DECLARE_DYNCREATE(CNCViewGL)

public:
	virtual ~CNCViewGL();
	CNCDoc*	GetDocument();
	virtual void OnInitialUpdate();
	virtual void OnDraw(CDC*);
	virtual BOOL OnCmdMsg(UINT, int, void*, AFX_CMDHANDLERINFO*);

protected:
	virtual void OnUpdate(CView*, LPARAM, CObject*);
	virtual void OnActivateView(BOOL, CView*, CView*);
	virtual void DoScale(int);	// ﾌﾚｰﾑの拡大率を更新

	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	// CNCViewTab::OnActivatePage() から SendMessage()
	afx_msg LRESULT OnUserActivatePage(WPARAM, LPARAM);
	// 各ﾋﾞｭｰへのﾌｨｯﾄﾒｯｾｰｼﾞ
	afx_msg LRESULT OnUserViewFitMsg(WPARAM, LPARAM);
	// ﾒﾆｭｰｺﾏﾝﾄﾞ
	afx_msg void OnUpdateMoveRoundKey(CCmdUI* pCmdUI);
	afx_msg	void OnLensKey(UINT);
	afx_msg void OnDefViewInfo();
	afx_msg void OnViewMode(UINT);
	afx_msg void OnUpdateViewMode(CCmdUI* pCmdUI);
	//
	afx_msg LRESULT OnSelectTrace(WPARAM, LPARAM);	// from NCViewTab.cpp

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

void	InitialMillNormal(void);	// from CNCVCApp::CNCVCApp()

#ifndef _DEBUG
inline CNCDoc* CNCViewGL::GetDocument()
   { return static_cast<CNCDoc *>(m_pDocument); }
#endif
