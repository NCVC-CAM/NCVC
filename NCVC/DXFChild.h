// DXFChild.h : ヘッダー ファイル
//

#pragma once

#include "ChildBase.h"

class	CDXFView;

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

class CDXFChild : public CMDIChildWnd, public CChildBase
{
	CDXFFrameSplit	m_wndSplitter;
	CStatusBar		m_wndStatusBar;

protected:
	CDXFChild();           // 動的生成に使用されるプロテクト コンストラクタ。
	DECLARE_DYNCREATE(CDXFChild)

// アトリビュート
public:
	CDXFView*	GetMainView(void) {
		return reinterpret_cast<CDXFView *>(m_wndSplitter.GetPane(0, 0));
	}

// オペレーション
public:
	void	SetDataInfo(int, int, int, int, int);
	void	SetFactorInfo(double);
	void	ShowShapeView(void);

	// CDXFView ﾒｯｾｰｼﾞﾊﾝﾄﾞﾗ
	void	OnUpdateMouseCursor(const CPointD* = NULL);

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します

	//{{AFX_VIRTUAL(CDXFChild)
	public:
	virtual void ActivateFrame(int nCmdShow = -1);
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	protected:
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:
	virtual ~CDXFChild();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// 生成されたメッセージ マップ関数
protected:

	//{{AFX_MSG(CDXFChild)
	afx_msg void OnMDIActivate(BOOL bActivate, CWnd* pActivateWnd, CWnd* pDeactivateWnd);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnClose();
	//}}AFX_MSG
	// ﾕｰｻﾞｲﾆｼｬﾙ処理
	afx_msg LRESULT OnUserInitialUpdate(WPARAM, LPARAM);
	// ﾌｧｲﾙ変更通知 from DocBase.cpp
	afx_msg LRESULT OnUserFileChangeNotify(WPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()
};
