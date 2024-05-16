// NCListView.h : ヘッダー ファイル
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CNCListView ビュー

class CNCListView : public CListView
{
	CString						m_strFind;	// 実際の検索文字列
	boost::xpressive::cregex	m_regFind;	// xpressiveオブジェクト
	CNCdata*		m_pTraceData;	// SelectTrace()呼び出し中か否か

protected:
	CNCListView() {
		m_pTraceData = NULL;
	}
	DECLARE_DYNCREATE(CNCListView)

// アトリビュート
public:
	CNCDoc*	GetDocument();

// オペレーション
public:
	void	SetJumpList(int);					// from NCChild.cpp <- NCJumpDlg.cpp
	void	SetFindList(int, const CString&);	// from NCChild.cpp <- NCFindDlg.cpp

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。

	//{{AFX_VIRTUAL(CNCListView)
	public:
	virtual void OnInitialUpdate();
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// 生成されたメッセージ マップ関数
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

#ifndef _DEBUG  // NCListView.cpp ファイルがデバッグ環境の時使用されます。
inline CNCDoc* CNCListView::GetDocument()
   { return static_cast<CNCDoc *>(m_pDocument); }
#endif
