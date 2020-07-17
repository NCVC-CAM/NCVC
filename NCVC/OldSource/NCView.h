// NCView.h : CNCView �N���X�̐錾����уC���^�[�t�F�C�X�̒�`�����܂��B
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "NCViewBase.h"

class CNCView : public CView, public CNCViewBase
{
	CPoint		m_ptGuide[NCXYZ][2];	// XYZ���̶޲�ލ��W(�n�_�E�I�_)
	// ܰ���`
	CPointD		m_ptdWorkRect[2][4];	// ܰ���`[Zmin/Zmax][��`] �S�p(from CNCDoc::m_rcWork)
	CPoint		m_ptDrawWorkRect[2][4];	// ܰ���`�̕`��p���W
	// �ް���`
	CPointD		m_ptdMaxRect[2][4];		// �ő�؍��`[Zmin/Zmax][��`]
	CPoint		m_ptDrawMaxRect[2][4];	// �ő�؍��`�̕`��p���W
	CRectD		m_rcMaxRect;			// 2D�ϊ���̋�`(�\����`)

	void	SetGuideData(void);
	void	SetMaxRect2D(void);
	void	SetWorkRect2D(void);
	void	ConvertWorkRect(void);
	void	ConvertMaxRect(void);

	void	DrawWorkRect(CDC*);
	void	DrawMaxRect(CDC*);

	void	OnViewLensComm(void);

protected: // �V���A���C�Y�@�\�݂̂���쐬���܂��B
	CNCView();
	DECLARE_DYNCREATE(CNCView)

// �A�g���r���[�g
public:
	CNCDoc*	GetDocument();

// �I�y���[�V����
public:

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B
	//{{AFX_VIRTUAL(CNCView)
	public:
	virtual void OnDraw(CDC* pDC);  // ���̃r���[��`�悷��ۂɃI�[�o�[���C�h����܂��B
	virtual void OnInitialUpdate();
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	virtual void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
public:
	virtual ~CNCView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// �������ꂽ���b�Z�[�W �}�b�v�֐�
protected:
	//{{AFX_MSG(CNCView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg int OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	//}}AFX_MSG
	// CNCViewTab::OnActivatePage() ���� SendMessage()
	afx_msg LRESULT OnUserActivatePage(WPARAM, LPARAM);
	// �ƭ������
	afx_msg void OnUpdateEditCopy(CCmdUI* pCmdUI);
	afx_msg void OnUpdateMoveKey (CCmdUI* pCmdUI);
	afx_msg void OnUpdateRoundKey(CCmdUI* pCmdUI);
	afx_msg void OnEditCopy();
	afx_msg	void OnMoveKey(UINT);
	afx_msg	void OnLensKey(UINT);
	// �e�ޭ��ւ�̨��ү����
	afx_msg LRESULT OnUserViewFitMsg(WPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#ifndef _DEBUG  // NCView.cpp �t�@�C�����f�o�b�O���̎��g�p����܂��B
inline CNCDoc* CNCView::GetDocument()
   { return static_cast<CNCDoc *>(m_pDocument); }
#endif
