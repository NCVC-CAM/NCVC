// NCVCView.h : CNCVCView �N���X�̃C���^�[�t�F�C�X
//


#pragma once


class CNCVCView : public CView
{
protected: // �V���A��������̂ݍ쐬���܂��B
	CNCVCView();
	DECLARE_DYNCREATE(CNCVCView)

// ����
public:
	CNCVCDoc* GetDocument() const;

// ����
public:

// �I�[�o�[���C�h
public:
	virtual void OnDraw(CDC* pDC);  // ���̃r���[��`�悷�邽�߂ɃI�[�o�[���C�h����܂��B
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:

// ����
public:
	virtual ~CNCVCView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// �������ꂽ�A���b�Z�[�W���蓖�Ċ֐�
protected:
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // NCVCView.cpp �̃f�o�b�O �o�[�W����
inline CNCVCDoc* CNCVCView::GetDocument() const
   { return reinterpret_cast<CNCVCDoc*>(m_pDocument); }
#endif

