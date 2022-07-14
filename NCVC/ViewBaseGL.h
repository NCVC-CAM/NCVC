// ViewBaseGL.h : ヘッダー ファイル
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
	int			m_cx,  m_cy;		// ｳｨﾝﾄﾞｳｻｲｽﾞ(ｽｸﾘｰﾝ)
	TRACKINGMODE	m_enTrackingMode;
	CPoint		m_ptLclick,			// Downのときのマウスポイント
				m_ptDownClick;		// ｺﾝﾃｷｽﾄﾒﾆｭｰ表示用他
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

// インデックスIDとRGBAを変換
#define	PICKREGION		5
#define	READBUF			(2*PICKREGION*2*PICKREGION*4)
void	IDtoRGB(int, GLubyte[]);
int		RGBtoID(GLubyte[]);
int		SearchSelectID(GLubyte[]);
