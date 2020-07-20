// ViewBaseGL.h : �w�b�_�[ �t�@�C��
//

#pragma once

#include "FrameBuffer.h"

// TrackingMode
enum ENTRACKINGMODE
{
	TM_NONE, TM_SPIN, TM_PAN
};

/////////////////////////////////////////////////////////////////////////////
// CViewBase

class CViewBaseGL : public CView
{
protected:
	HGLRC		m_hRC;
	int			m_cx,  m_cy;	// ����޳����(��ذ�)
	ENTRACKINGMODE	m_enTrackingMode;
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
	DECLARE_DYNAMIC(CViewBaseGL)

	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//
	void	IdentityMatrix(void);
	BOOL	SetupPixelFormat(CDC*);
	void	SetupViewingTransform(void);
	void	BeginTracking(const CPoint&, ENTRACKINGMODE);
	void	EndTracking(void);
	CPoint3F	PtoR(const CPoint& pt);
	void	DoTracking(const CPoint&);
	void	DoScale(int);
	void	DoRotation(float);
	void	RenderBackground(COLORREF, COLORREF);
	//
	BOOL	_OnSize(UINT nType, int cx, int cy);
	//
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);

	DECLARE_MESSAGE_MAP()

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
};

void	OutputGLErrorMessage(GLenum, UINT);
