// NCViewGL.h : CNCViewGL クラスの宣言およびインターフェイスの定義をします。
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

// TrackingMode
enum ENTRACKINGMODE
{
	TM_NONE = 0,
	TM_SPIN,
	TM_PAN,
	TM_ZOOM,
};

class CNCViewGL : public CView
{
	DECLARE_DYNCREATE(CNCViewGL)

	int		m_cx, m_cy;
	CRect3D	m_rcMax;
	CDC*	m_pDC;
	HGLRC	m_hRC;
	GLuint	m_uiDisplayListIndex;

	ENTRACKINGMODE	m_enTrackingMode;
	CPoint			m_ptLast;
	GLfloat			m_f3LastPos[3];
	GLfloat			m_f3RenderingCenter[3];		// 描画中心
	GLfloat			m_fRenderingRate;			// 拡大率
	GLfloat			m_objectXform[4][4];

	BOOL	SetupPixelFormat(void);
	void	RenderAxis(void);
	void	RenderCode(void);

	void	ptov(const CPoint& pt, GLfloat v[3]);
	void	BeginTracking(const CPoint&, ENTRACKINGMODE);
	void	EndTracking(void);
	void	DoTracking(const CPoint&);
	void	DoRotation(float, float, float, float);
	void	SetupViewingTransform(void);
	void	SetupViewingFrustum(void);

protected:
	CNCViewGL();

public:
	CNCDoc*	GetDocument();
	virtual void OnInitialUpdate();
	virtual void OnDraw(CDC* pDC);
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

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
	// 各ﾋﾞｭｰへのﾌｨｯﾄﾒｯｾｰｼﾞ
	afx_msg LRESULT OnUserViewFitMsg(WPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#ifndef _DEBUG
inline CNCDoc* CNCViewGL::GetDocument()
   { return static_cast<CNCDoc *>(m_pDocument); }
#endif
