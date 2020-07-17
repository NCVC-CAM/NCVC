// DXFChild.h : ヘッダー ファイル
//

#pragma once

#include "ChildBase.h"

class	CDXFDoc;
class	CDXFView;
class	CDXFShapeView;

/////////////////////////////////////////////////////////////////////////////
// CDXFFrameSplit スプリッタフレーム

class CDXFFrameSplit : public CSplitterWnd
{
protected:
	// ﾒｯｾｰｼﾞﾏｯﾌﾟ
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CDXFChild フレーム

class CDXFChild : public CChildBase
{
	CDXFFrameSplit	m_wndSplitter;
	CStatusBar		m_wndStatusBar;

protected:
	CDXFChild();           // 動的生成に使用されるプロテクト コンストラクタ。
	DECLARE_DYNCREATE(CDXFChild)

// アトリビュート
public:
	CDXFView*		GetMainView(void) {
		return reinterpret_cast<CDXFView *>(m_wndSplitter.GetPane(0, 0));
	}
	CDXFShapeView*	GetTreeView(void) {
		return reinterpret_cast<CDXFShapeView *>(m_wndSplitter.GetPane(0, 1));
	}

// オペレーション
public:
	void	SetDataInfo(const CDXFDoc*);
	void	SetFactorInfo(float);
	void	ShowShapeView(void);

	// CDXFView ﾒｯｾｰｼﾞﾊﾝﾄﾞﾗ
	void	OnUpdateMouseCursor(const CPointF* = NULL);

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します

	//{{AFX_VIRTUAL(CDXFChild)
	public:
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	protected:
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	//}}AFX_VIRTUAL

// 生成されたメッセージ マップ関数
protected:

	//{{AFX_MSG(CDXFChild)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnMDIActivate(BOOL bActivate, CWnd* pActivateWnd, CWnd* pDeactivateWnd);
	//}}AFX_MSG
	// ﾕｰｻﾞｲﾆｼｬﾙ処理
	afx_msg LRESULT OnUserInitialUpdate(WPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()
};
