// NCViewXY.h : �w�b�_�[ �t�@�C��
//

#pragma once

#include "NCViewBase.h"

class CNCViewXY : public CView, public CNCViewBase
{
	void	SetGuideData(void);
	void	DrawConvertWorkRect(void);
	void	DrawConvertMaxRect(void);

	void	OnViewLensComm(void);

protected:
	CNCViewXY();           // ���I�����Ɏg�p�����v���e�N�g �R���X�g���N�^
	DECLARE_DYNCREATE(CNCViewXY)

// �A�g���r���[�g
public:
	CNCDoc*	GetDocument();

// �I�y���[�V����
public:

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B
	//{{AFX_VIRTUAL(CNCViewXY)
	public:
	virtual void OnDraw(CDC* pDC);      // ���̃r���[��`�悷�邽�߂ɃI�[�o�[���C�h���܂����B
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	virtual void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
protected:
	virtual ~CNCViewXY();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// �������ꂽ���b�Z�[�W �}�b�v�֐�
protected:
	//{{AFX_MSG(CNCViewXY)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnUpdateEditCopy(CCmdUI* pCmdUI);
	afx_msg void OnEditCopy();
	//}}AFX_MSG
	// CNCViewTab::OnActivatePage() ���� SendMessage()
	afx_msg LRESULT OnUserActivatePage(WPARAM, LPARAM);
	// �ƭ������
	afx_msg	void OnMoveKey(UINT);
	afx_msg	void OnLensKey(UINT);
	// �e�ޭ��ւ�̨��ү����
	afx_msg LRESULT OnUserViewFitMsg(WPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#ifndef _DEBUG
inline CNCDoc* CNCViewXY::GetDocument()
   { return (CNCDoc*)m_pDocument; }
#endif
