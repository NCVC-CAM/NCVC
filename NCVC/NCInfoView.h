// NCInfoView.h : �w�b�_�[ �t�@�C��
//

#pragma once

#include "ViewBase.h"

/////////////////////////////////////////////////////////////////////////////
// CNCInfoView[1|2] ����

class CNCInfoBase : public CViewBase
{
	void	CopyNCInfoForClipboard(void);	// �N���b�v�{�[�h�ւ̃R�s�[

protected:
	CString	CreateCutTimeStr(void);

	DECLARE_DYNAMIC(CNCInfoBase)

	virtual BOOL PreCreateWindow(CREATESTRUCT&);
	virtual void OnUpdate(CView*, LPARAM, CObject*);
	//
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnEditCopy();
	afx_msg void OnUpdateEditCopy(CCmdUI* pCmdUI);
	afx_msg void OnUpdateMoveRoundKey(CCmdUI* pCmdUI);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);

public:
	CNCDoc*	GetDocument();
	virtual BOOL OnCmdMsg(UINT, int, void*, AFX_CMDHANDLERINFO*);

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CNCInfoView1 �r���[

class CNCInfoView1 : public CNCInfoBase
{
protected:
	CNCInfoView1();           // ���I�����Ɏg�p�����v���e�N�g �R���X�g���N�^
	DECLARE_DYNCREATE(CNCInfoView1)

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B

	//{{AFX_VIRTUAL(CNCInfoView1)
protected:
	virtual void OnDraw(CDC* pDC);      // ���̃r���[��`�悷�邽�߂ɃI�[�o�[���C�h���܂����B
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
protected:
	afx_msg LRESULT OnUserCalcMsg(WPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CNCInfoView2 �r���[

class CNCInfoView2 : public CNCInfoBase
{
protected:
	CNCInfoView2();           // ���I�����Ɏg�p�����v���e�N�g �R���X�g���N�^
	DECLARE_DYNCREATE(CNCInfoView2)

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B

	//{{AFX_VIRTUAL(CNCInfoView2)
protected:
	virtual void OnDraw(CDC* pDC);      // ���̃r���[��`�悷�邽�߂ɃI�[�o�[���C�h���܂����B
	//}}AFX_VIRTUAL

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#ifndef _DEBUG
inline CNCDoc* CNCInfoBase::GetDocument()
   { return static_cast<CNCDoc *>(m_pDocument); }
#endif
