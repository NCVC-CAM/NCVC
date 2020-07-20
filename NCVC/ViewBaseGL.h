// ViewBaseGL.h : ヘッダー ファイル
//

#pragma once

#include "FrameBuffer.h"

/////////////////////////////////////////////////////////////////////////////
// CViewBase

class CViewBaseGL : public CView
{
public:
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	HGLRC		m_hRC;
	int			m_cx,  m_cy;	// ｳｨﾝﾄﾞｳｻｲｽﾞ(ｽｸﾘｰﾝ)

	CViewBaseGL();
	DECLARE_DYNAMIC(CViewBaseGL)

	BOOL	SetupPixelFormat(CDC*);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//
	BOOL	_OnSize(UINT nType, int cx, int cy);
	//
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);

	DECLARE_MESSAGE_MAP()
};
