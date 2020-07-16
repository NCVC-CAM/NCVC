// DXFView.h : ヘッダー ファイル
//

#pragma once

#include "ViewBase.h"

// ﾂﾘｰﾋﾞｭｰで選択されているﾀｲﾌﾟ
typedef	boost::variant<DWORD, CLayerData*, CDXFshape*, CDXFworking*>	DXFTREETYPE;
#define	DXFTREETYPE_MUSTER	0
#define	DXFTREETYPE_LAYER	1
#define	DXFTREETYPE_SHAPE	2
#define	DXFTREETYPE_WORKING	3
// 現在の加工指示
enum	DXFPROCESSDIRECT {
	DXFPROCESS_SELECT, DXFPROCESS_ARRAW, DXFPROCESS_START, DXFPROCESS_OUTLINE
};

/////////////////////////////////////////////////////////////////////////////
// CDXFView ビュー

class CDXFView : public CView, public CViewBase
{
	CPointD		m_ptArraw[2][3];	// 一時的な始点終点の矢印座標
	CDXFchain	m_ltOutline[2];		// 一時的な輪郭ｵﾌﾞｼﾞｪｸﾄ
	DXFPROCESSDIRECT	m_enProcessDirect;	// 仮加工指示に何を描画するか
	int			m_nSelect;			// m_ptArraw[0|1] or -1
	DXFTREETYPE	m_vSelect;			// 現在選択されているﾂﾘｰｵﾌﾞｼﾞｪｸﾄ
	CDXFdata*	m_pSelData;			// 　〃　ｵﾌﾞｼﾞｪｸﾄ(OnLButtonUp)

	BOOL	OnUpdateShape(DXFTREETYPE[]);
	BOOL	IsRootTree(DWORD);
	void	DrawTemporaryProcess(CDC* pDC);
	void	DrawArraw(CDC*);
	void	DrawOutline(CDC*);
	void	OnViewLensComm(void);
	CDXFworking*	CreateWorkingData(void);
	BOOL	CreateOutlineTempObject(const CDXFshape*);
	void	DeleteOutlineTempObject(void);
	BOOL	CancelForSelect(CDC* = NULL);
	void	AllChangeFactor_OutlineTempObject(void);

	BOOL	OnLButtonUp_Sel(CDC*, const CPointD&, const CRectD&);
	BOOL	OnLButtonUp_Vec(CDC*, const CPointD&, const CRectD&);
	BOOL	OnLButtonUp_Out(CDC*, const CPointD&, const CRectD&);
	BOOL	OnMouseMove_Sel(CDC*, const CPointD&, const CRectD&);
	BOOL	OnMouseMove_Vec(CDC*, const CPointD&, const CRectD&);
	BOOL	OnMouseMove_Out(CDC*, const CPointD&, const CRectD&);

protected:
	CDXFView();           // 動的生成に使用されるプロテクト コンストラクタ
	DECLARE_DYNCREATE(CDXFView)

// アトリビュート
public:
	CDXFDoc*	GetDocument();

// オペレーション
public:

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CDXFView)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void OnDraw(CDC* pDC);      // このビューを描画するためにオーバーライドしました。
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:
	virtual ~CDXFView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// 生成されたメッセージ マップ関数
protected:
	//{{AFX_MSG(CDXFView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnViewLayer();
	afx_msg void OnUpdateViewLayer(CCmdUI* pCmdUI);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnUpdateEditCopy(CCmdUI* pCmdUI);
	afx_msg void OnEditCopy();
	//}}AFX_MSG
	// OnInitialUpdate() から PostMessage() or 各ﾋﾞｭｰへのﾌｨｯﾄﾒｯｾｰｼﾞ
	afx_msg LRESULT OnUserViewFitMsg(WPARAM, LPARAM);
	// MDI子ﾌﾚｰﾑのｽﾃｰﾀｽﾊﾞｰ表示更新(ｵﾌﾞｼﾞｪｸﾄIDがClassWizard一覧に載らないので手動ｺｰﾃﾞｨﾝｸﾞ)
	afx_msg void OnUpdateMouseCursor(CCmdUI* pCmdUI);
	// 移動
	afx_msg	void OnMoveKey(UINT);
	afx_msg	void OnLensKey(UINT);

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#ifndef _DEBUG  // DXFView.cpp ファイルがデバッグ環境の時使用されます。
inline CDXFDoc* CDXFView::GetDocument()
   { return (CDXFDoc*)m_pDocument; }
#endif
