// NCListView.h : ヘッダー ファイル
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CNCListView ビュー

class CNCListView : public CListView
{
protected:
	CNCListView();			// 動的生成に使用されるプロテクト コンストラクタ
	DECLARE_DYNCREATE(CNCListView)

// アトリビュート
public:
	CNCDoc*	GetDocument();

// オペレーション
public:
	void	SetJumpList(int);		// from NCChild.cpp -> NCJumpDlg.cpp
	void	SelectTrace(CNCdata* pData) {	// from NCViewTab.cpp
		int	nIndex = pData->GetStrLine();
		GetListCtrl().SetItemState(nIndex, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
		GetListCtrl().EnsureVisible(nIndex, FALSE);		// 強制ｽｸﾛｰﾙの可能性もあるのでUpdate()ではNG
	}

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
	virtual ~CNCListView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// 生成されたメッセージ マップ関数
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

#ifndef _DEBUG  // NCListView.cpp ファイルがデバッグ環境の時使用されます。
inline CNCDoc* CNCListView::GetDocument()
   { return (CNCDoc*)m_pDocument; }
#endif
