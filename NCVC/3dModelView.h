// 3dModelView.h : ヘッダー ファイル
//

#pragma once

#include "ViewBaseGL.h"

// RenderMode
enum RENDERMODE
{
	RM_NORMAL, RM_PICKLINE, RM_PICKFACE
};

/////////////////////////////////////////////////////////////////////////////
// C3dModelView ビュー

class C3dModelView : public CViewBaseGL
{
	CPoint		m_ptLclick;			// Downのときのマウスポイント
	int			m_icx, m_icy;		// FBOを作った時のサイズ
	BODY*		m_pSelCurveBody;	// 選択したNURBS曲線が属するBODY
	BODY*		m_pSelFaceBody;		// 選択したNURBS曲面が属するBODY
	int			m_nSelCurve;		// 選択したNURBS曲線
	int			m_nSelFace;			// 選択したNURBS曲面

	void	DrawBody(RENDERMODE);
	void	DoSelect(const CPoint&);
	int		DoSelectCurve(const CPoint&);
	int		DoSelectFace (const CPoint&);

protected:
	C3dModelView();
	DECLARE_DYNCREATE(C3dModelView)

public:
	virtual ~C3dModelView();
	C3dModelDoc*	GetDocument();
	virtual void OnInitialUpdate();
	virtual void OnDraw(CDC* pDC);

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg	void OnLensKey(UINT);

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#ifndef _DEBUG
inline C3dModelDoc* C3dModelView::GetDocument()
   { return static_cast<C3dModelDoc *>(m_pDocument); }
#endif
