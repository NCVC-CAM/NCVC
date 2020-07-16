// DXFView.h : ヘッダー ファイル
//

#if !defined(AFX_DXFVIEW_H__95D33FAE_974A_11D3_B0D5_004005691B12__INCLUDED_)
#define AFX_DXFVIEW_H__95D33FAE_974A_11D3_B0D5_004005691B12__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ViewBase.h"

// ﾂﾘｰﾋﾞｭｰで選択されているﾀｲﾌﾟ
typedef	boost::variant<DWORD, CLayerData*, CDXFshape*, CDXFworking*>	DXFTREETYPE;
#define	DXFTREETYPE_MUSTER	0
#define	DXFTREETYPE_LAYER	1
#define	DXFTREETYPE_SHAPE	2
#define	DXFTREETYPE_WORKING	3
// 現在の加工指示
enum	DXFPROCESSDIRECT {
	DXFPROCESS_SELECT, DXFPROCESS_ARRAW, DXFPROCESS_OUTLINE
};

/////////////////////////////////////////////////////////////////////////////
// CDXFView ビュー

class CDXFView : public CView, public CViewBase
{
	CPointD		m_ptArraw[2][3];	// 一時的な始点終点の矢印座標
	CDXFarray	m_obOutline[2];		// 一時的な輪郭ｵﾌﾞｼﾞｪｸﾄ
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
	BOOL		CreateOutlineTempObject(const CPointD&);
	BOOL		CreateOutlineTempObject_sub(BOOL, int);
	CDXFdata*	CreateOutlineTempObject_new(const CDXFdata*, const CPointD&, const CPointD&);
	void	DeleteOutlineTempObject(void);
	BOOL	CancelForSelect(CDC* = NULL);
	void	AllChangeFactor_OutlineTempObject(void);

	void	OnLButtonUp_Sel(      CDXFshape*, const CDXFdata*,                 CDC*);
	void	OnLButtonUp_Vec(const CDXFshape*, const CDXFdata*, const CPointD&, CDC*);
	void	OnLButtonUp_Out(const CDXFshape*, const CDXFdata*, const CPointD&, CDC*);

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

#ifndef _DEBUG  // DXFView.cpp ファイルがデバッグ環境の時使用されます。
inline CDXFDoc* CDXFView::GetDocument()
   { return (CDXFDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ は前行の直前に追加の宣言を挿入します。

#endif // !defined(AFX_DXFVIEW_H__95D33FAE_974A_11D3_B0D5_004005691B12__INCLUDED_)
