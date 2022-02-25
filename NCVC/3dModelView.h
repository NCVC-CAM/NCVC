// 3dModelView.h : �w�b�_�[ �t�@�C��
//

#pragma once

#include "ViewBaseGL.h"

// RenderMode
enum RENDERMODE
{
	RM_NORMAL, RM_PICKLINE, RM_PICKFACE
};

/////////////////////////////////////////////////////////////////////////////
// C3dModelView �r���[

class C3dModelView : public CViewBaseGL
{
	GLuint		m_glCode;		// BODY�`��̃f�B�X�v���C���X�g
	CPoint		m_ptLclick;		// Down�̂Ƃ��̃}�E�X�|�C���g
	int			m_icx, m_icy;	// FBO����������̃T�C�Y
	int			m_nSelCurve;	// �I������NURBS�Ȑ�

	void	DrawBody(RENDERMODE);
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
