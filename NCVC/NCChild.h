// NCChild.h : CNCChild クラスの宣言およびインターフェイスの定義をします。
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "MainStatusBar.h"
#include "ChildBase.h"
#include "NCdata.h"

class CNCViewTab;
class CNCListView;

/////////////////////////////////////////////////////////////////////////////
// CNCFrameSplit スプリッタフレーム

class CNCFrameSplit : public CSplitterWnd  
{
protected:
	// ﾒｯｾｰｼﾞﾏｯﾌﾟ
	afx_msg void OnDestroy();
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	// CNCChild::OnCreate() から PostMessage()
	afx_msg LRESULT OnUserInitialUpdate(WPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CNCChild フレーム

class CNCChild : public CMDIChildWnd, public CChildBase
{
	CNCFrameSplit	m_wndSplitter1, m_wndSplitter2;
	CMainStatusBar  m_wndStatusBar;

protected:
	CNCChild();           // 動的生成に使用されるプロテクト コンストラクタ。
	DECLARE_DYNCREATE(CNCChild)

// アトリビュート
public:
	CNCViewTab*		GetMainView(void) {
		return (CNCViewTab *)(m_wndSplitter1.GetPane(0, 1));
	}
	CNCListView*	GetListView(void) {
		return (CNCListView *)(m_wndSplitter2.GetPane(1, 0));
	}
	CMainStatusBar*	GetStatusBar(void) {
		return &m_wndStatusBar;
	}
	CProgressCtrl*	GetProgressCtrl(void) {
		return GetStatusBar()->GetProgressCtrl();
	}

// オペレーション
public:
	void	SetWorkRect(BOOL, CRect3D&);	// from NCWorkDlg.cpp
	void	SetJumpList(int);				// from NCJumpDlg.cpp
	void	SetFactorInfo(ENNCVPLANE, double);

	// CNCListView ﾒｯｾｰｼﾞﾊﾝﾄﾞﾗ
	void	OnUpdateStatusLineNo(int, int, const CNCblock*);

//オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CNCChild)
	public:
	virtual void ActivateFrame(int nCmdShow = -1);
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	protected:
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	//}}AFX_VIRTUAL

// インプリメンテーション
public:
	virtual ~CNCChild();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// 生成したメッセージ マップ関数
protected:
	//{{AFX_MSG(CNCChild)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnMDIActivate(BOOL bActivate, CWnd* pActivateWnd, CWnd* pDeactivateWnd);
	afx_msg void OnClose();
	//}}AFX_MSG
	// ﾌｧｲﾙ変更通知 from DocBase.cpp
	afx_msg LRESULT OnUserFileChangeNotify(WPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()
};
