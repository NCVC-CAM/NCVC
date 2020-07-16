// DXFShapeView.h : ヘッダー ファイル
//

#if !defined(AFX_DXFSHAPEVIEW_H__967EA6F2_0128_4064_A8F1_4A828C5F9D5E__INCLUDED_)
#define AFX_DXFSHAPEVIEW_H__967EA6F2_0128_4064_A8F1_4A828C5F9D5E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// ﾙｰﾄﾂﾘｰID
#define	ROOTTREE_SHAPE		0x00000001
#define	ROOTTREE_LOCUS		0x00000002
#define	ROOTTREE_EXCLUDE	0x00000003

/////////////////////////////////////////////////////////////////////////////
// CDXFShapeView ビュー

class CDXFShapeView : public CTreeView
{
	BOOL		m_bDragging;
	CImageList*	m_pImageList;
	HTREEITEM	m_hRootTree[3], m_hItemDrag, m_hItemDrop;
	CLayerData*	m_pDragLayer;	// ﾄﾞﾗｯｸﾞｱｲﾃﾑ(以下同時使用のためboost::variant使用不可)
	CDXFshape*	m_pDragShape;
	DWORD		m_dwDragRoot;

	BOOL		m_bUpdateLayerSequence;		// ﾚｲﾔ順が変更された
	CLayerMap	m_mpUpdateLayer;			// 形状順が変更されたﾚｲﾔ

	void	OnUpdateShape(LPDXFADDSHAPE);
	BOOL	IsRootTree(HTREEITEM hTree) {
		return hTree==m_hRootTree[0] || hTree==m_hRootTree[1] || hTree==m_hRootTree[2];
	}
	BOOL	IsDropItem(HTREEITEM);
	void		DragInsert(void);
	HTREEITEM	DragInsertLayer(void);
	HTREEITEM	DragInsertShape(void);
	void		DragLink(void);
	void	DragCancel(BOOL);
	HTREEITEM	SearchLayerTree(HTREEITEM, const CLayerData*);
	DWORD	GetParentAssemble(HTREEITEM);
	void	SetShapeSwitch_SubordinateTree(HTREEITEM, BOOL);

	void	SetShapeTree(void);
	void	AddWorking(CDXFworking *);
	void	AutoWorkingDel(void);
	void	AutoWorkingSet(BOOL = TRUE);
	void	UpdateSequence(void);

protected:
	CDXFShapeView();           // 動的生成に使用されるプロテクト コンストラクタ
	DECLARE_DYNCREATE(CDXFShapeView)

// アトリビュート
public:
	CDXFDoc*	GetDocument();

// オペレーション
public:

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。

	//{{AFX_VIRTUAL(CDXFShapeView)
	public:
	virtual void OnInitialUpdate();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:
	virtual ~CDXFShapeView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// 生成されたメッセージ マップ関数
protected:
	//{{AFX_MSG(CDXFShapeView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnGetDispInfo(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBeginLabelEdit(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEndLabelEdit(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnKeydown(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSelChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBeginDrag(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnNMRclick(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnSortShape();
	afx_msg void OnUpdateWorkingDel(CCmdUI* pCmdUI);
	afx_msg void OnWorkingDel();
	//}}AFX_MSG
	// 形状加工処理
	afx_msg void OnUpdateShapePattern(CCmdUI* pCmdUI);

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#ifndef _DEBUG  // DXFShapeView.cpp ファイルがデバッグ環境の時使用されます。
inline CDXFDoc* CDXFShapeView::GetDocument()
   { return (CDXFDoc*)m_pDocument; }
#endif

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ は前行の直前に追加の宣言を挿入します。

#endif // !defined(AFX_DXFSHAPEVIEW_H__967EA6F2_0128_4064_A8F1_4A828C5F9D5E__INCLUDED_)
