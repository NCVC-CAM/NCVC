// NCInfoView.h : ヘッダー ファイル
//

#pragma once

#include "ViewBase.h"

/////////////////////////////////////////////////////////////////////////////
// CNCInfoView[1|2] 共通

class CNCInfoBase : public CViewBase
{
	void	CopyNCInfoForClipboard(void);	// クリップボードへのコピー

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
// CNCInfoView1 ビュー

class CNCInfoView1 : public CNCInfoBase
{
protected:
	CNCInfoView1();           // 動的生成に使用されるプロテクト コンストラクタ
	DECLARE_DYNCREATE(CNCInfoView1)

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。

	//{{AFX_VIRTUAL(CNCInfoView1)
protected:
	virtual void OnDraw(CDC* pDC);      // このビューを描画するためにオーバーライドしました。
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:
	afx_msg LRESULT OnUserCalcMsg(WPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CNCInfoView2 ビュー

class CNCInfoView2 : public CNCInfoBase
{
protected:
	CNCInfoView2();           // 動的生成に使用されるプロテクト コンストラクタ
	DECLARE_DYNCREATE(CNCInfoView2)

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。

	//{{AFX_VIRTUAL(CNCInfoView2)
protected:
	virtual void OnDraw(CDC* pDC);      // このビューを描画するためにオーバーライドしました。
	//}}AFX_VIRTUAL

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#ifndef _DEBUG
inline CNCDoc* CNCInfoBase::GetDocument()
   { return static_cast<CNCDoc *>(m_pDocument); }
#endif
