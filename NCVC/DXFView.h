// DXFView.h : ヘッダー ファイル
//

#pragma once

#include "ViewBase.h"

// ﾂﾘｰﾋﾞｭｰで選択されているﾀｲﾌﾟ
typedef	boost::variant<DWORD_PTR, CLayerData*, CDXFshape*, CDXFworking*>	DXFTREETYPE;
enum {
	DXFTREETYPE_MUSTER = 0,
	DXFTREETYPE_LAYER,
	DXFTREETYPE_SHAPE,
	DXFTREETYPE_WORKING
};

// 子ﾋﾞｭｰの選択情報 -> vector<> m_bindSel
struct SELECTBIND
{
	int		nSel;
	CPoint	ptDiff;
	CRect	rc;
	bool operator < (const SELECTBIND& sel) const {
		return nSel > sel.nSel;		// 降順
	}
};

// ﾏｳｽ操作の遷移(子ｳｨﾝﾄﾞｳ移動用)
enum ENBINDSTATE
{
	BD_NONE = 0,
	BD_SELECT,
	BD_MOVE,
	BD_CANCEL
};
/*
// CToolTipCtrlﾒﾓﾘﾘｰｸ対策 -> 効かず...
struct TOOLINFO_EX : public TOOLINFO
{
	void*	lpReserved;
};
*/
/////////////////////////////////////////////////////////////////////////////
// CDXFView ビュー

class CDXFView : public CViewBase
{
	CPointF		m_ptArraw[2][3],	// 一時的な始点終点の矢印座標
				m_ptStart[4];		// 一時的な開始位置(円で最大4点)
	CDXFchain	m_ltOutline[2];		// 一時的な輪郭ｵﾌﾞｼﾞｪｸﾄ
	CDXFdata*	m_pSelData;			// 　〃　ｵﾌﾞｼﾞｪｸﾄ(OnLButtonUp)
	DXFTREETYPE	m_vSelect;			// 現在選択されているﾂﾘｰｵﾌﾞｼﾞｪｸﾄ
	float		m_dOffset;			// ↑のｵﾌｾｯﾄ値
	int			m_nSelect;			// m_ptArraw[0|1] or -1
	CRect		m_rcDrawWork;		// ﾜｰｸ矩形(bind)
	std::vector<SELECTBIND>					m_bindSel;	// 選択情報
	CTypedPtrList<CPtrList, LPCADBINDINFO>	m_bindUndo;	// 移動のUNDO蓄積
	ENBINDSTATE	m_enBind;
	// ↓謎のﾒﾓﾘﾘｰｸが発生
//	CToolTipCtrl*	m_pToolTip;		// 子ﾋﾞｭｰ用ToolTip(ﾌｧｲﾙ名表示)
//	CMFCToolTipCtrl*	m_pToolTip;	// 子ﾋﾞｭｰ用ToolTip(ﾌｧｲﾙ名表示)

	BOOL	OnUpdateShape(DXFTREETYPE[]);
	BOOL	IsRootTree(DWORD);
	void	DrawTemporaryProcess(CDC* pDC);
	void	DrawTempArraw(CDC*);
	void	DrawTempStart(CDC*);
	void	DrawTempOutline(CDC*);
	CDXFworking*	CreateWorkingData(void);
	BOOL	CreateOutlineTempObject(CDXFshape*);
	void	DeleteOutlineTempObject(void);
	BOOL	CancelForSelect(CDC* = NULL);
	void	AllChangeFactor_OutlineTempObject(void);
	void	BindMove(BOOL);
	void	BindMsgPost(CDC*, LPCADBINDINFO, CPoint*);
	BOOL	IsBindSelected(int);
	void	ClearBindSelectData(void);

	void	OnLButtonUp_Separate(CDC*, CDXFdata*, const CPointF&, const CRectF&);
	void	OnLButtonUp_Vector  (CDC*, CDXFdata*, const CPointF&, const CRectF&);
	void	OnLButtonUp_Start   (CDC*, CDXFdata*, const CPointF&, const CRectF&);
	void	OnLButtonUp_Outline (CDC*, CDXFdata*, const CPointF&, const CRectF&);
	void	OnMouseMove_Separate(CDC*, const CPointF&, const CRectF&);
	void	OnMouseMove_Vector  (CDC*, const CPointF&, const CRectF&);
	void	OnMouseMove_Start   (CDC*, const CPointF&, const CRectF&);
	void	OnMouseMove_Outline (CDC*, const CPointF&, const CRectF&);

protected:
	CDXFView();           // 動的生成に使用されるプロテクト コンストラクタ
	DECLARE_DYNCREATE(CDXFView)

// アトリビュート
public:
	CDXFDoc*	GetDocument();

// オペレーション
public:
	virtual ~CDXFView();

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CDXFView)
	public:
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
//	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void OnInitialUpdate();
	virtual INT_PTR OnToolHitTest(CPoint point, TOOLINFO* pTI) const;
	protected:
	virtual void OnDraw(CDC* pDC);      // このビューを描画するためにオーバーライドしました。
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

	// 生成されたメッセージ マップ関数
protected:
	//{{AFX_MSG(CDXFView)
	afx_msg void OnDestroy();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
#ifdef _DEBUG
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
#endif
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnViewLayer();
	afx_msg void OnUpdateViewLayer(CCmdUI* pCmdUI);
	afx_msg void OnEditUndo();
	afx_msg void OnUpdateEditUndo(CCmdUI* pCmdUI);
	afx_msg void OnEditCopy();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg LRESULT OnNcHitTest(CPoint point);
	//}}AFX_MSG
	afx_msg BOOL OnToolTipNotify(UINT id, NMHDR* pNMHDR, LRESULT* pResult);
	// OnInitialUpdate() から PostMessage() or 各ﾋﾞｭｰへのﾌｨｯﾄﾒｯｾｰｼﾞ
	afx_msg LRESULT OnUserViewFitMsg(WPARAM, LPARAM);
	// MDI子ﾌﾚｰﾑのｽﾃｰﾀｽﾊﾞｰ表示更新(ｵﾌﾞｼﾞｪｸﾄIDがClassWizard一覧に載らないので手動ｺｰﾃﾞｨﾝｸﾞ)
	afx_msg void OnUpdateMouseCursor(CCmdUI* pCmdUI);
	// CADﾃﾞｰﾀの統合
	afx_msg void OnUpdateEditBind(CCmdUI* pCmdUI);
	afx_msg void OnEditBindDel();
	afx_msg void OnEditBindTarget();
	afx_msg LRESULT OnBindInitMsg(WPARAM, LPARAM);
	afx_msg LRESULT OnBindLButtonDown(WPARAM, LPARAM);
	afx_msg LRESULT OnBindRoundMsg(WPARAM, LPARAM);
	afx_msg LRESULT OnBindCancel(WPARAM, LPARAM);
	// 移動
	afx_msg	void OnMoveKey(UINT);
	afx_msg	void OnLensKey(UINT);
	//
	virtual	void	OnViewLensComm(void);

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#ifndef _DEBUG  // DXFView.cpp ファイルがデバッグ環境の時使用されます。
inline CDXFDoc* CDXFView::GetDocument()
   { return static_cast<CDXFDoc *>(m_pDocument); }
#endif
