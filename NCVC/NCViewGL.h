// NCViewGL.h : CNCViewGL �N���X�̐錾����уC���^�[�t�F�C�X�̒�`�����܂��B
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

	BOOL		m_bActive;
	int			m_cx,  m_cy,	// ����޳����(��ذ�)
				m_icx, m_icy;
	double		m_dRate;		// ��g�嗦
	CRect3D		m_rcView,		// ���ً��
				m_rcDraw;		// ܰ���`(�د�ޕ\���p)
	CPointD		m_ptCenter,		// �`�撆�S
				m_ptLastMove;	// �ړ��O���W
	CPoint3D	m_ptLastRound;	// ��]�O���W
	CPoint		m_ptDownClick;	// ��÷���ƭ��\���p��
	HGLRC		m_hRC;
	GLuint		m_glCode;		// �؍��߽���ި���ڲؽ�

	GLuint		m_nVertexID,	// ���_�z��p
				m_nNormalID,	// �@���޸�ٗp
				m_nPictureID,	// ø����摜�p
				m_nTextureID;	// ø������W�p
	GLuint*		m_pGenBuf;		// ���_���ޯ���p
	std::vector<GLsizei>	m_vVertexWrk,	// ܰ���`�pglDrawElements�p���_��
							m_vVertexCut;	// �؍�ʗp

	ENTRACKINGMODE	m_enTrackingMode;
	GLdouble		m_objectXform[4][4];

	void	ClearObjectForm(void);
	BOOL	SetupPixelFormat(CDC*);
	void	UpdateViewOption(void);
	void	CreateWire(void);
	BOOL	CreateBoxel(void);
	BOOL	GetClipDepth(void);
	BOOL	CreateElement(GLfloat*, GLfloat*);
	BOOL	ReadTexture(LPCTSTR);
	void	CreateTexture(void);
	void	ClearElement(void);
	void	ClearTexture(void);

	void	RenderBack(void);
	void	RenderAxis(void);
	void	RenderCode(void);

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
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	// CNCViewTab::OnActivatePage() ���� SendMessage()
	afx_msg LRESULT OnUserActivatePage(WPARAM, LPARAM);
	// �ƭ������
	afx_msg	void OnMoveKey(UINT);
	afx_msg	void OnLensKey(UINT);
	// �e�ޭ��ւ�̨��ү����
	afx_msg LRESULT OnUserViewFitMsg(WPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#ifndef _DEBUG
inline CNCDoc* CNCViewGL::GetDocument()
   { return static_cast<CNCDoc *>(m_pDocument); }
#endif
