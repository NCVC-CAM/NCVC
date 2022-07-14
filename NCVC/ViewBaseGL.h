// ViewBaseGL.h : �w�b�_�[ �t�@�C��
//

#pragma once

#include "FrameBuffer.h"

// TrackingMode
enum TRACKINGMODE
{
	TM_NONE, TM_SPIN, TM_PAN
};

// RenderMode
enum RENDERMODE
{
	RM_NORMAL, RM_PICKLINE, RM_PICKFACE
};

/////////////////////////////////////////////////////////////////////////////
// CViewBase

class CViewBaseGL : public CView
{
protected:
	HGLRC		m_hRC;
	CFrameBuffer*	m_pFBO;			// FrameBufferObject
	int			m_cx,  m_cy;		// ����޳����(��ذ�)
	TRACKINGMODE	m_enTrackingMode;
	CPoint		m_ptLclick,			// Down�̂Ƃ��̃}�E�X�|�C���g
				m_ptDownClick;		// ��÷���ƭ��\���p��
	float		m_dRate,			// ��g�嗦
				m_dRoundAngle,		// �����݂̉�]�p�x
				m_dRoundStep;		// �����݂̂P�񂠂���̉�]�p�x
	CRect3F		m_rcView,			// ���ً��
				m_rcDraw;			// ܰ���`(�د�ޕ\���p)
	CPointF		m_ptCenter,			// �`�撆�S
				m_ptLastMove;		// �ړ��O���W
	CPoint3F	m_ptLastRound,		// ��]�O���W
				m_ptRoundBase;		// �����ݘA����]�̊���W
	GLdouble	m_objXform[4][4];	// ��]�s��

	CViewBaseGL();
	virtual ~CViewBaseGL();
	DECLARE_DYNAMIC(CViewBaseGL)

	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual	void DoScale(int);
	//
	void	CreateFBO(void);
	void	IdentityMatrix(void);
	void	SetOrthoView(void);
	BOOL	SetupPixelFormat(CDC*);
	void	SetupViewingTransform(void);
	void	BeginTracking(const CPoint&, TRACKINGMODE);
	void	EndTracking(void);
	CPoint3F	PtoR(const CPoint& pt);
	void	DoTracking(const CPoint&);
	void	DoRotation(float);
	void	RenderBackground(COLORREF, COLORREF);
	//
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg	void OnMoveKey(UINT);
	afx_msg	void OnRoundKey(UINT);
	afx_msg void OnUpdateEditCopy(CCmdUI* pCmdUI);

	DECLARE_MESSAGE_MAP()

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
};

void	OutputGLErrorMessage(GLenum, UINT);

// �C���f�b�N�XID��RGBA��ϊ�
#define	PICKREGION		5
#define	READBUF			(2*PICKREGION*2*PICKREGION*4)
void	IDtoRGB(int, GLubyte[]);
int		RGBtoID(GLubyte[]);
int		SearchSelectID(GLubyte[]);
