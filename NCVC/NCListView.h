// NCListView.h : �w�b�_�[ �t�@�C��
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CNCListView �r���[

class CNCListView : public CListView
{
	CString						m_strFind;	// ���ۂ̌���������
	boost::xpressive::cregex	m_regFind;	// xpressive�I�u�W�F�N�g
	CNCdata*		m_pTraceData;	// SelectTrace()�Ăяo�������ۂ�

protected:
	CNCListView() {
		m_pTraceData = NULL;
	}
	DECLARE_DYNCREATE(CNCListView)

// �A�g���r���[�g
public:
	CNCDoc*	GetDocument();

// �I�y���[�V����
public:
	void	SetJumpList(int);					// from NCChild.cpp <- NCJumpDlg.cpp
	void	SetFindList(int, const CString&);	// from NCChild.cpp <- NCFindDlg.cpp

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
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// �������ꂽ���b�Z�[�W �}�b�v�֐�
protected:
	//{{AFX_MSG(CNCListView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnKeyDown(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnGetDispInfo(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnItemChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnUpdateTraceBreak(CCmdUI* pCmdUI);
	afx_msg void OnTraceBreak();
	afx_msg void OnTraceBreakOFF();
	afx_msg void OnViewJump();
	afx_msg void OnUpdateViewJump(CCmdUI* pCmdUI);
	afx_msg void OnViewFind();
	afx_msg void OnUpdateViewFind(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditCopy(CCmdUI* pCmdUI);
	afx_msg void OnUpdateMoveRoundKey(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFileInsert(CCmdUI* pCmdUI);
	afx_msg void OnFileInsert();
	//}}AFX_MSG
	afx_msg LRESULT OnSelectTrace(WPARAM, LPARAM);	// from NCViewTab.cpp

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#ifndef _DEBUG  // NCListView.cpp �t�@�C�����f�o�b�O���̎��g�p����܂��B
inline CNCDoc* CNCListView::GetDocument()
   { return static_cast<CNCDoc *>(m_pDocument); }
#endif
