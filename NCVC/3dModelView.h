// 3dModelView.h : �w�b�_�[ �t�@�C��
//

#pragma once

#include "ViewBase.h"

/////////////////////////////////////////////////////////////////////////////
// C3dModelView �r���[

class C3dModelView : public CViewBase
{

protected:
	C3dModelView();           // ���I�����Ŏg�p����� protected �R���X�g���N�^�[
	virtual ~C3dModelView();

public:
	C3dModelDoc*	GetDocument();
	virtual void OnDraw(CDC* pDC);      // ���̃r���[��`�悷�邽�߂ɃI�[�o�[���C�h����܂��B

protected:
	DECLARE_DYNCREATE(C3dModelView)
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#ifndef _DEBUG
inline C3dModelDoc* C3dModelView::GetDocument()
   { return static_cast<C3dModelDoc *>(m_pDocument); }
#endif
