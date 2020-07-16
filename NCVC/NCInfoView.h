// NCInfoView.h : ヘッダー ファイル
//

#pragma once

#include "ViewBase.h"

/////////////////////////////////////////////////////////////////////////////
// CNCInfoView1 ビュー

class CNCInfoView1 : public CView, public CViewBase
{
protected:
	CNCInfoView1();           // 動的生成に使用されるプロテクト コンストラクタ
	DECLARE_DYNCREATE(CNCInfoView1)

// アトリビュート
public:
	CNCDoc*	GetDocument();

// オペレーション
public:

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。

	//{{AFX_VIRTUAL(CNCInfoView1)
	public:
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	protected:
	virtual void OnDraw(CDC* pDC);      // このビューを描画するためにオーバーライドしました。
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:
	virtual ~CNCInfoView1();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// 生成されたメッセージ マップ関数
protected:
	//{{AFX_MSG(CNCInfoView1)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnEditCopy();
	afx_msg void OnUpdateEditCopy(CCmdUI* pCmdUI);
	//}}AFX_MSG
	afx_msg LRESULT OnUserCalcMsg(WPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CNCInfoView2 ビュー

class CNCInfoView2 : public CView, public CViewBase
{
protected:
	CNCInfoView2();           // 動的生成に使用されるプロテクト コンストラクタ
	DECLARE_DYNCREATE(CNCInfoView2)

// アトリビュート
public:
	CNCDoc*	GetDocument();

// オペレーション
public:

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。

	//{{AFX_VIRTUAL(CNCInfoView2)
	public:
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	protected:
	virtual void OnDraw(CDC* pDC);      // このビューを描画するためにオーバーライドしました。
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:
	virtual ~CNCInfoView2();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// 生成されたメッセージ マップ関数
protected:
	//{{AFX_MSG(CNCInfoView2)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnEditCopy();
	afx_msg void OnUpdateEditCopy(CCmdUI* pCmdUI);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#ifndef _DEBUG
inline CNCDoc* CNCInfoView1::GetDocument()
   { return (CNCDoc*)m_pDocument; }
inline CNCDoc* CNCInfoView2::GetDocument()
   { return (CNCDoc*)m_pDocument; }
#endif
