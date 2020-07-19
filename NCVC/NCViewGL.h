// NCViewGL.h : CNCViewGL �N���X�̐錾����уC���^�[�t�F�C�X�̒�`�����܂��B
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
	int			m_cx,  m_cy,	// ����޳����(��ذ�)
				m_icx, m_icy;
	GLint		m_wx, m_wy;		// glReadPixels, glWindowPos
	float		m_dRate,		// ��g�嗦
				m_dRoundAngle,	// �����݂̉�]�p�x
				m_dRoundStep;	// �����݂̂P�񂠂���̉�]�p�x
	CRect3F		m_rcView,		// ���ً��
				m_rcDraw;		// ܰ���`(�د�ޕ\���p)
	CPointF		m_ptCenter,		// �`�撆�S
				m_ptCenterBk,
				m_ptLastMove;	// �ړ��O���W
	CPoint3F	m_ptLastRound,	// ��]�O���W
				m_ptRoundBase;	// �����ݘA����]�̊���W
	CPoint		m_ptDownClick;	// ��÷���ƭ��\���p��
	HGLRC		m_hRC;
	GLuint		m_glCode;		// �؍��߽���ި���ڲؽ�

	GLfloat*	m_pfDepth;		// ���߽�l�擾�z��
	GLfloat*	m_pfXYZ;		// -- �ϊ����ꂽܰ��ލ��W(temp area)
	GLfloat*	m_pfNOR;		// -- �@���޸��
	GLbyte*		m_pbStencil;	// ��ݼ�
	GLsizeiptr	m_nVBOsize;		// ���_�z�񻲽�
	GLuint		m_nVertexID[2],	// ���_�z��Ɩ@���޸�ٗp
				m_nTextureID,	// ø������W�p
				m_nPictureID;	// ø����摜�p
	GLuint*		m_pSolidElement;// ���_���ޯ���p
	GLuint*		m_pLocusElement;// �O�ղ��ޯ���p
	CVelement	m_vElementWrk,	// ܰ���`�pglDrawElements���_��
				m_vElementCut;	// �؍�ʗp
	WIREDRAW	m_WireDraw;		// ܲԉ��H�@�p

	ENTRACKINGMODE	m_enTrackingMode;
	GLdouble		m_objXform[4][4],
					m_objXformBk[4][4];	// �޸�ُ��������ޯ�����

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
	// CNCViewTab::OnActivatePage() ���� SendMessage()
	afx_msg LRESULT OnUserActivatePage(WPARAM, LPARAM);
	// �e�ޭ��ւ�̨��ү����
	afx_msg LRESULT OnUserViewFitMsg(WPARAM, LPARAM);
	// �ƭ������
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
