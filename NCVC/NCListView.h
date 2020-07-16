// NCListView.h : �w�b�_�[ �t�@�C��
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CNCListView �r���[

class CNCListView : public CListView
{
protected:
	CNCListView();			// ���I�����Ɏg�p�����v���e�N�g �R���X�g���N�^
	DECLARE_DYNCREATE(CNCListView)

// �A�g���r���[�g
public:
	CNCDoc*	GetDocument();

// �I�y���[�V����
public:
	void	SetJumpList(int);		// from NCChild.cpp -> NCJumpDlg.cpp
	void	SelectTrace(CNCdata* pData) {	// from NCViewTab.cpp
		int	nIndex = pData->GetStrLine();
		GetListCtrl().SetItemState(nIndex, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
		GetListCtrl().EnsureVisible(nIndex, FALSE);		// ������۰ق̉\��������̂�Update()�ł�NG
	}

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B

	//{{AFX_VIRTUAL(CNCListView)
	public:
	virtual void OnInitialUpdate();
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
protected:
	virtual ~CNCListView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// �������ꂽ���b�Z�[�W �}�b�v�֐�
protected:
	//{{AFX_MSG(CNCListView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnGetDispInfo(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnItemChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnUpdateTraceBreak(CCmdUI* pCmdUI);
	afx_msg void OnTraceBreak();
	afx_msg void OnTraceBreakOFF();
	afx_msg void OnViewJump();
	afx_msg void OnUpdateViewJump(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditCopy(CCmdUI* pCmdUI);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#ifndef _DEBUG  // NCListView.cpp �t�@�C�����f�o�b�O���̎��g�p����܂��B
inline CNCDoc* CNCListView::GetDocument()
   { return (CNCDoc*)m_pDocument; }
#endif
