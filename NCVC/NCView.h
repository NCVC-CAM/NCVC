// NCView.h : CNCView クラスの宣言およびインターフェイスの定義をします。
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "NCViewBase.h"

class CNCView : public CView, public CNCViewBase
{
	CPoint		m_ptGuide[NCXYZ][2];	// XYZ軸のｶﾞｲﾄﾞ座標(始点・終点)
	// ﾜｰｸ矩形
	CPointD		m_ptdWorkRect[2][4];	// ﾜｰｸ矩形[Zmin/Zmax][矩形] ４角(from CNCDoc::m_rcWork)
	CPoint		m_ptDrawWorkRect[2][4];	// ﾜｰｸ矩形の描画用座標
	// ﾃﾞｰﾀ矩形
	CPointD		m_ptdMaxRect[2][4];		// 最大切削矩形[Zmin/Zmax][矩形]
	CPoint		m_ptDrawMaxRect[2][4];	// 最大切削矩形の描画用座標
	CRectD		m_rcMaxRect;			// 2D変換後の矩形(表示矩形)

	void	SetGuideData(void);
	void	SetMaxRect2D(void);
	void	SetWorkRect2D(void);
	void	ConvertWorkRect(void);
	void	ConvertMaxRect(void);

	void	DrawWorkRect(CDC*);
	void	DrawMaxRect(CDC*);

	void	OnViewLensComm(void);

protected: // シリアライズ機能のみから作成します。
	CNCView();
	DECLARE_DYNCREATE(CNCView)

// アトリビュート
public:
	CNCDoc*	GetDocument();

// オペレーション
public:

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CNCView)
	public:
	virtual void OnDraw(CDC* pDC);  // このビューを描画する際にオーバーライドされます。
	virtual void OnInitialUpdate();
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	virtual void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);
	//}}AFX_VIRTUAL

// インプリメンテーション
public:
	virtual ~CNCView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// 生成されたメッセージ マップ関数
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

#ifndef _DEBUG  // NCView.cpp ファイルがデバッグ環境の時使用されます。
inline CNCDoc* CNCView::GetDocument()
   { return static_cast<CNCDoc *>(m_pDocument); }
#endif
