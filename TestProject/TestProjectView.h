
// TestProjectView.h : CTestProjectView �N���X�̃C���^�[�t�F�C�X
//

#pragma once


class CTestProjectView : public CView
{
protected: // �V���A��������̂ݍ쐬���܂��B
	CTestProjectView();
	DECLARE_DYNCREATE(CTestProjectView)

// ����
public:
	CTestProjectDoc* GetDocument() const;

// ����
public:

// �I�[�o�[���C�h
public:
	virtual void OnDraw(CDC* pDC);  // ���̃r���[��`�悷�邽�߂ɃI�[�o�[���C�h����܂��B
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:

// ����
public:
	virtual ~CTestProjectView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// �������ꂽ�A���b�Z�[�W���蓖�Ċ֐�
protected:
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // TestProjectView.cpp �̃f�o�b�O �o�[�W����
inline CTestProjectDoc* CTestProjectView::GetDocument() const
   { return reinterpret_cast<CTestProjectDoc*>(m_pDocument); }
#endif

