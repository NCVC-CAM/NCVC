// 3dModelView.cpp : �����t�@�C��
//

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "3dModelChild.h"
#include "3dModelDoc.h"
#include "3dModelView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// C3dModelView

IMPLEMENT_DYNCREATE(C3dModelView, CViewBase)

BEGIN_MESSAGE_MAP(C3dModelView, CViewBase)
END_MESSAGE_MAP()

C3dModelView::C3dModelView()
{
}

C3dModelView::~C3dModelView()
{
}

/////////////////////////////////////////////////////////////////////////////
// C3dModelView �N���X�̃I�[�o���C�h�֐�

#ifdef _DEBUG
C3dModelDoc* C3dModelView::GetDocument() // ��f�o�b�O �o�[�W�����̓C�����C���ł��B
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(C3dModelDoc)));
	return static_cast<C3dModelDoc *>(m_pDocument);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// C3dModelView �`��

void C3dModelView::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: �`��R�[�h�������ɒǉ����Ă��������B
}

/////////////////////////////////////////////////////////////////////////////
// C3dModelView ���b�Z�[�W �n���h���[
