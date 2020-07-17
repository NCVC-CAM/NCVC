// NCViewYZ.h : ヘッダー ファイル
//

#pragma once

#include "NCViewBase.h"

class CNCViewYZ : public CView, public CNCViewBase
{
	void	SetGuideData(void);
	void	ConvertWorkRect(void);
	void	ConvertMaxRect(void);
	CRectD	GetDrawMaxRect(void);
	void	OnViewLensComm(void);
	void	DrawGuideScale(CDC*);

protected:
	CNCViewYZ();           // 動的生成に使用されるプロテクト コンストラクタ
	DECLARE_DYNCREATE(CNCViewYZ)

// アトリビュート
public:
	CNCDoc*	GetDocument();

// オペレーション
public:

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。

	//{{AFX_VIRTUAL(CNCViewYZ)
	public:
	virtual void OnDraw(CDC* pDC);      // このビューを描画するためにオーバーライドしました。
	virtual void OnInitialUpdate();
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	virtual void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:
	virtual ~CNCViewYZ();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// 生成されたメッセージ マップ関数
protected:
	//{{AFX_MSG(CNCViewYZ)
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
	// CNCViewTab::OnActivatePage() から SendMessage()
	afx_msg LRESULT OnUserActivatePage(WPARAM, LPARAM);
	// ﾒﾆｭｰｺﾏﾝﾄﾞ
	afx_msg void OnUpdateEditCopy(CCmdUI* pCmdUI);
	afx_msg void OnUpdateMoveKey (CCmdUI* pCmdUI);
	afx_msg void OnUpdateRoundKey(CCmdUI* pCmdUI);
	afx_msg void OnEditCopy();
	afx_msg	void OnMoveKey(UINT);
	afx_msg	void OnLensKey(UINT);
	// 各ﾋﾞｭｰへのﾌｨｯﾄﾒｯｾｰｼﾞ
	afx_msg LRESULT OnUserViewFitMsg(WPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#ifndef _DEBUG
inline CNCDoc* CNCViewYZ::GetDocument()
   { return static_cast<CNCDoc *>(m_pDocument); }
#endif
