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

class CNCViewGL : public CView
{
	DECLARE_DYNCREATE(CNCViewGL)

	int			m_cx,  m_cy,	// ｳｨﾝﾄﾞｳｻｲｽﾞ(ｽｸﾘｰﾝ)
				m_icx, m_icy;
	double		m_dRate;		// 基準拡大率
	CRect3D		m_rcView,		// ﾓﾃﾞﾙ空間
				m_rcDraw;		// ﾜｰｸ矩形
	CPointD		m_ptCenter,		// 描画中心
				m_ptLastMove;	// 移動前座標
	CPoint3D	m_ptLastRound;	// 回転前座標
	HGLRC		m_hRC;
	GLuint		m_glWork,		// ﾜｰｸ矩形のﾃﾞｨｽﾌﾟﾚｲﾘｽﾄ
				m_glCode;		// 切削ﾊﾟｽのﾃﾞｨｽﾌﾟﾚｲﾘｽﾄ

	std::vector<GLfloat>	m_vfXYZ;	// 変換されたﾜｰﾙﾄﾞ座標

	ENTRACKINGMODE	m_enTrackingMode;
	GLdouble		m_objectXform[4][4];

	void	ClearObjectForm(void);
	BOOL	SetupPixelFormat(CDC*);
	void	CreateBoxel(void);
	void	ClipDepth(void);

	void	RenderBack(void);
	void	RenderAxis(void);
	void	RenderWork(void);
	void	RenderCode(BOOL = FALSE);

	CPoint3D	PtoR(const CPoint& pt);
	void	BeginTracking(const CPoint&, ENTRACKINGMODE);
	void	EndTracking(void);
	void	DoTracking(const CPoint&);
	void	DoScale(int);
	void	DoRotation(double, const CPoint3D&);
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
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	// CNCViewTab::OnActivatePage() から SendMessage()
	afx_msg LRESULT OnUserActivatePage(WPARAM, LPARAM);
	// ﾒﾆｭｰｺﾏﾝﾄﾞ
	afx_msg	void OnMoveKey(UINT);
	afx_msg	void OnLensKey(UINT);
	// 各ﾋﾞｭｰへのﾌｨｯﾄﾒｯｾｰｼﾞ
	afx_msg LRESULT OnUserViewFitMsg(WPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#ifndef _DEBUG
inline CNCDoc* CNCViewGL::GetDocument()
   { return static_cast<CNCDoc *>(m_pDocument); }
#endif
