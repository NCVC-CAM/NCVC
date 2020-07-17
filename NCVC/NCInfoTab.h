// NCInfoTab.h: CNCInfoTab クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "TabView.h"

class CNCInfoTab : public CTabViewBase  
{
protected:
	CNCInfoTab();		// 動的生成に使用されるプロテクト コンストラクタ
	DECLARE_DYNCREATE(CNCInfoTab)

// アトリビュート
public:
	CNCDoc*	GetDocument();

// オーバーライド
public:
	virtual void OnInitialUpdate();
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
protected:
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	virtual	void OnActivatePage(int nIndex);

// インプリメンテーション
protected:
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// 生成されたメッセージ マップ関数
protected:
	afx_msg int		OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void	OnDestroy();
//	afx_msg void	OnSetFocus(CWnd* pOldWnd);
	// ﾀﾌﾞ移動
	afx_msg	void	OnMoveTab(UINT);

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#ifndef _DEBUG
inline CNCDoc* CNCInfoTab::GetDocument()
   { return static_cast<CNCDoc *>(m_pDocument); }
#endif
