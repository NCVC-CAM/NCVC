// NCVCView.cpp : CNCVCView �N���X�̎���
//

#include "stdafx.h"
#include "NCVC.h"

#include "NCVCDoc.h"
#include "NCVCView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CNCVCView

IMPLEMENT_DYNCREATE(CNCVCView, CView)

BEGIN_MESSAGE_MAP(CNCVCView, CView)
END_MESSAGE_MAP()

// CNCVCView �R���X�g���N�V����/�f�X�g���N�V����

CNCVCView::CNCVCView()
{
	// TODO: �\�z�R�[�h�������ɒǉ����܂��B

}

CNCVCView::~CNCVCView()
{
}

BOOL CNCVCView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: ���̈ʒu�� CREATESTRUCT cs ���C������ Window �N���X�܂��̓X�^�C����
	//  �C�����Ă��������B

	return CView::PreCreateWindow(cs);
}

// CNCVCView �`��

void CNCVCView::OnDraw(CDC* /*pDC*/)
{
	CNCVCDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	// TODO: ���̏ꏊ�Ƀl�C�e�B�u �f�[�^�p�̕`��R�[�h��ǉ����܂��B
}


// CNCVCView �f�f

#ifdef _DEBUG
void CNCVCView::AssertValid() const
{
	CView::AssertValid();
}

void CNCVCView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CNCVCDoc* CNCVCView::GetDocument() const // �f�o�b�O�ȊO�̃o�[�W�����̓C�����C���ł��B
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CNCVCDoc)));
	return (CNCVCDoc*)m_pDocument;
}
#endif //_DEBUG


// CNCVCView ���b�Z�[�W �n���h��
