// NCViewGL.h : CNCViewGL クラスの宣言およびインターフェイスの定義をします。
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

// TrackingMode
enum ENTRACKINGMODE
{
	TM_NONE = 0,
	TM_SPIN,
	TM_PAN
};

//
class CNCViewGL : public CView
{
	DECLARE_DYNCREATE(CNCViewGL)

	CString		m_strGuide;
	BOOL		m_bActive;
	int			m_cx,  m_cy,	// ｳｨﾝﾄﾞｳｻｲｽﾞ(ｽｸﾘｰﾝ)
				m_icx, m_icy;
	double		m_dRate,		// 基準拡大率
				m_dRoundAngle,	// 中ﾎﾞﾀﾝの回転角度
				m_dRoundStep;	// 中ﾎﾞﾀﾝの１回あたりの回転角度
	CRect3D		m_rcView,		// ﾓﾃﾞﾙ空間
				m_rcDraw;		// ﾜｰｸ矩形(ｿﾘｯﾄﾞ表示用)
	CPointD		m_ptCenter,		// 描画中心
				m_ptLastMove;	// 移動前座標
	CPoint3D	m_ptLastRound,	// 回転前座標
				m_ptRoundBase;	// 中ﾎﾞﾀﾝ連続回転の基準座標
	CPoint		m_ptDownClick;	// ｺﾝﾃｷｽﾄﾒﾆｭｰ表示用他
	HGLRC		m_hRC;
	GLuint		m_glCode;		// 切削ﾊﾟｽのﾃﾞｨｽﾌﾟﾚｲﾘｽﾄ
	PFNGLDRAWPROC	m_pfnDrawProc;	// 描画関数ﾎﾟｲﾝﾀ

	GLuint		m_nVertexID,	// 頂点配列用
				m_nNormalID,	// 法線ﾍﾞｸﾄﾙ用
				m_nPictureID,	// ﾃｸｽﾁｬ画像用
				m_nTextureID;	// ﾃｸｽﾁｬ座標用
	GLuint*		m_pGenBuf;		// 頂点ｲﾝﾃﾞｯｸｽ用
	std::vector<GLsizei>	m_vVertexWrk,	// ﾜｰｸ矩形用glDrawElements頂点個数
							m_vVertexCut;	// 切削面用
	std::vector<double>		m_vWireLength;	// 移動を伴わない切削集合長さ(ﾜｲﾔ表示用)

	ENTRACKINGMODE	m_enTrackingMode;
	GLdouble		m_objectXform[4][4];

	void	ClearObjectForm(void);
	BOOL	SetupPixelFormat(CDC*);
	void	UpdateViewOption(void);
	void	CreateDisplayList(void);
	BOOL	CreateBoxel(void);
	BOOL	CreateLathe(void);
	BOOL	CreateWire(void);
	BOOL	GetClipDepthMill(void);
	BOOL	GetClipDepthLathe(void);
	BOOL	CreateVBOMill(const GLfloat*, GLfloat*);
	BOOL	CreateVBOLathe(const GLfloat*, const GLfloat*, const GLfloat*);
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

	CPoint3D	PtoR(const CPoint& pt);
	void	BeginTracking(const CPoint&, ENTRACKINGMODE);
	void	EndTracking(void);
	void	DoTracking(const CPoint&);
	void	DoScale(int);
	void	DoRotation(double);
	void	SetupViewingTransform(void);

protected:
	CNCViewGL();

public:
	CNCDoc*	GetDocument();
	virtual void OnInitialUpdate();
	virtual void OnDraw(CDC* pDC);
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	virtual void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);

public:
	virtual ~CNCViewGL();
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
	afx_msg void OnTimer(UINT nIDEvent);
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

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#ifndef _DEBUG
inline CNCDoc* CNCViewGL::GetDocument()
   { return static_cast<CNCDoc *>(m_pDocument); }
#endif
