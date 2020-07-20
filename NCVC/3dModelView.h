// 3dModelView.h : ヘッダー ファイル
//

#pragma once

#include "ViewBaseGL.h"

/////////////////////////////////////////////////////////////////////////////
// C3dModelView ビュー

class C3dModelView : public CViewBaseGL
{
public:
	C3dModelDoc*	GetDocument();
	virtual void OnDraw(CDC* pDC);

protected:
	C3dModelView();
	virtual ~C3dModelView();
	DECLARE_DYNCREATE(C3dModelView)

	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#ifndef _DEBUG
inline C3dModelDoc* C3dModelView::GetDocument()
   { return static_cast<C3dModelDoc *>(m_pDocument); }
#endif
