
// TestProjectView.cpp : CTestProjectView �N���X�̎���
//

#include "stdafx.h"
// SHARED_HANDLERS �́A�v���r���[�A�k���ŁA����ь����t�B���^�[ �n���h���[���������Ă��� ATL �v���W�F�N�g�Œ�`�ł��A
// ���̃v���W�F�N�g�Ƃ̃h�L�������g �R�[�h�̋��L���\�ɂ��܂��B
#ifndef SHARED_HANDLERS
#include "TestProject.h"
#endif

#include "TestProjectDoc.h"
#include "TestProjectView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CTestProjectView

IMPLEMENT_DYNCREATE(CTestProjectView, CView)

BEGIN_MESSAGE_MAP(CTestProjectView, CView)
END_MESSAGE_MAP()

// CTestProjectView �R���X�g���N�V����/�f�X�g���N�V����

CTestProjectView::CTestProjectView()
{
	// TODO: �\�z�R�[�h�������ɒǉ����܂��B

}

CTestProjectView::~CTestProjectView()
{
}

BOOL CTestProjectView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: ���̈ʒu�� CREATESTRUCT cs ���C������ Window �N���X�܂��̓X�^�C����
	//  �C�����Ă��������B

	return CView::PreCreateWindow(cs);
}

// CTestProjectView �`��

void CTestProjectView::OnDraw(CDC* /*pDC*/)
{
	CTestProjectDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	// TODO: ���̏ꏊ�Ƀl�C�e�B�u �f�[�^�p�̕`��R�[�h��ǉ����܂��B
}


// CTestProjectView �f�f

#ifdef _DEBUG
void CTestProjectView::AssertValid() const
{
	CView::AssertValid();
}

void CTestProjectView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CTestProjectDoc* CTestProjectView::GetDocument() const // �f�o�b�O�ȊO�̃o�[�W�����̓C�����C���ł��B
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CTestProjectDoc)));
	return (CTestProjectDoc*)m_pDocument;
}
#endif //_DEBUG


// CTestProjectView ���b�Z�[�W �n���h���[
