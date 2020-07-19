// NCViewGL.h : CNCViewGL クラスの宣言およびインターフェイスの定義をします。
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "NCdata.h"

// TrackingMode
enum ENTRACKINGMODE
{
	TM_NONE, TM_SPIN, TM_PAN
};

// GetClipDepthMill() argument
enum ENCLIPDEPTH
{
	DP_NoStencil, DP_BottomStencil, DP_TopStencil
};

//
class CNCViewGL : public CView
{
	CString		m_strGuide;
	BOOL		m_bActive,
				m_bSizeChg;
	int			m_cx,  m_cy,	// ｳｨﾝﾄﾞｳｻｲｽﾞ(ｽｸﾘｰﾝ)
				m_icx, m_icy;
	GLint		m_wx, m_wy;		// glReadPixels, glWindowPos
	float		m_dRate,		// 基準拡大率
				m_dRoundAngle,	// 中ﾎﾞﾀﾝの回転角度
				m_dRoundStep;	// 中ﾎﾞﾀﾝの１回あたりの回転角度
	CRect3F		m_rcView,		// ﾓﾃﾞﾙ空間
				m_rcDraw;		// ﾜｰｸ矩形(ｿﾘｯﾄﾞ表示用)
	CPointF		m_ptCenter,		// 描画中心
				m_ptCenterBk,
				m_ptLastMove;	// 移動前座標
	CPoint3F	m_ptLastRound,	// 回転前座標
				m_ptRoundBase;	// 中ﾎﾞﾀﾝ連続回転の基準座標
	CPoint		m_ptDownClick;	// ｺﾝﾃｷｽﾄﾒﾆｭｰ表示用他
	HGLRC		m_hRC;
	GLuint		m_glCode;		// 切削ﾊﾟｽのﾃﾞｨｽﾌﾟﾚｲﾘｽﾄ

	GLfloat*	m_pfDepth;		// ﾃﾞﾌﾟｽ値取得配列
	GLfloat*	m_pfXYZ;		// -- 変換されたﾜｰﾙﾄﾞ座標(temp area)
	GLfloat*	m_pfNOR;		// -- 法線ﾍﾞｸﾄﾙ
	GLbyte*		m_pbStencil;	// ｽﾃﾝｼﾙ
	GLsizeiptr	m_nVBOsize;		// 頂点配列ｻｲｽﾞ
	GLuint		m_nVertexID[2],	// 頂点配列と法線ﾍﾞｸﾄﾙ用
				m_nTextureID,	// ﾃｸｽﾁｬ座標用
				m_nPictureID;	// ﾃｸｽﾁｬ画像用
	GLuint*		m_pSolidElement;// 頂点ｲﾝﾃﾞｯｸｽ用
	GLuint*		m_pLocusElement;// 軌跡ｲﾝﾃﾞｯｸｽ用
	CVelement	m_vElementWrk,	// ﾜｰｸ矩形用glDrawElements頂点個数
				m_vElementCut;	// 切削面用
	WIREDRAW	m_WireDraw;		// ﾜｲﾔ加工機用

	ENTRACKINGMODE	m_enTrackingMode;
	GLdouble		m_objXform[4][4],
					m_objXformBk[4][4];	// ﾎﾞｸｾﾙ処理時のﾊﾞｯｸｱｯﾌﾟ

	void	ClearObjectForm(void);
	BOOL	SetupPixelFormat(CDC*);
	void	UpdateViewOption(void);
	void	CreateDisplayList(void);
	BOOL	CreateBoxel(BOOL = FALSE);
	BOOL	CreateBoxel_fromIGES(void);
	BOOL	CreateLathe(BOOL = FALSE);
	BOOL	CreateWire(void);
	BOOL	CreateBottomFaceThread(BOOL, int);
	BOOL	GetClipDepthMill(BOOL, ENCLIPDEPTH = DP_NoStencil);
	void	GetClipDepthMill_All(GLdouble, GLdouble, GLdouble, size_t, size_t);
	void	GetClipDepthMill_Zonly(GLdouble, GLdouble, GLdouble, size_t, size_t);
	void	GetClipDepthMill_BottomStencil(GLdouble, GLdouble, GLdouble, size_t, size_t);
	void	GetClipDepthMill_TopStencil(GLdouble, GLdouble, GLdouble, size_t, size_t);
	BOOL	GetClipDepthCylinder(BOOL);
	BOOL	GetClipDepthLathe(BOOL);
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

	void	RenderBack(void);
	void	RenderAxis(void);
	void	RenderCode(void);
	void	RenderMill(const CNCdata*);

	CPoint3F	PtoR(const CPoint& pt);
	void	BeginTracking(const CPoint&, ENTRACKINGMODE);
	void	EndTracking(void);
	void	DoTracking(const CPoint&);
	void	DoScale(int);
	void	DoRotation(float);
	void	SetupViewingTransform(void);

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
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnUpdate(CView*, LPARAM, CObject*);
	virtual void OnActivateView(BOOL, CView*, CView*);

public:
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	// CNCViewTab::OnActivatePage() から SendMessage()
	afx_msg LRESULT OnUserActivatePage(WPARAM, LPARAM);
	// 各ﾋﾞｭｰへのﾌｨｯﾄﾒｯｾｰｼﾞ
	afx_msg LRESULT OnUserViewFitMsg(WPARAM, LPARAM);
	// ﾒﾆｭｰｺﾏﾝﾄﾞ
	afx_msg void OnUpdateEditCopy(CCmdUI* pCmdUI);
	afx_msg void OnUpdateMoveRoundKey(CCmdUI* pCmdUI);
	afx_msg	void OnMoveKey(UINT);
	afx_msg	void OnRoundKey(UINT);
	afx_msg	void OnLensKey(UINT);
	afx_msg void OnDefViewInfo();
	//
	afx_msg LRESULT OnSelectTrace(WPARAM, LPARAM);	// from NCViewTab.cpp

	DECLARE_MESSAGE_MAP()
};

typedef void (CNCViewGL::*PFN_GETCLIPDEPTHMILL)(GLdouble, GLdouble, GLdouble, size_t, size_t);

/////////////////////////////////////////////////////////////////////////////

void	InitialMillNormal(void);	// from CNCVCApp::CNCVCApp()

#ifndef _DEBUG
inline CNCDoc* CNCViewGL::GetDocument()
   { return static_cast<CNCDoc *>(m_pDocument); }
#endif
