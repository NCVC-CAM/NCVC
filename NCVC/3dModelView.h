// 3dModelView.h : ヘッダー ファイル
//

#pragma once

#include "ViewBaseGL.h"

/////////////////////////////////////////////////////////////////////////////
// C3dModelView ビュー

class C3dModelView : public CViewBaseGL
{
	GLuint		m_glCode;		// BODY描画のディスプレイリスト
	CPoint		m_ptLclick;

	void	DrawBody(void);
	void	DoSelect(const CPoint&);

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
