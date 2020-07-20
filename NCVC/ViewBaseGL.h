// ViewBaseGL.h : ヘッダー ファイル
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
	int			m_cx,  m_cy;	// ｳｨﾝﾄﾞｳｻｲｽﾞ(ｽｸﾘｰﾝ)
	ENTRACKINGMODE	m_enTrackingMode;
	float		m_dRate,			// 基準拡大率
				m_dRoundAngle,		// 中ﾎﾞﾀﾝの回転角度
				m_dRoundStep;		// 中ﾎﾞﾀﾝの１回あたりの回転角度
	CRect3F		m_rcView,			// ﾓﾃﾞﾙ空間
				m_rcDraw;			// ﾜｰｸ矩形(ｿﾘｯﾄﾞ表示用)
	CPointF		m_ptCenter,			// 描画中心
				m_ptLastMove;		// 移動前座標
	CPoint3F	m_ptLastRound,		// 回転前座標
				m_ptRoundBase;		// 中ﾎﾞﾀﾝ連続回転の基準座標
	GLdouble	m_objXform[4][4];	// 回転行列

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
