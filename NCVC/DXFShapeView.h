// DXFShapeView.h : �w�b�_�[ �t�@�C��
//

#pragma once

// ٰ��ذID
#define	ROOTTREE_SHAPE		0x00000001
#define	ROOTTREE_LOCUS		0x00000002
#define	ROOTTREE_EXCLUDE	0x00000003

/////////////////////////////////////////////////////////////////////////////
// CDXFShapeView �r���[

class CDXFShapeView : public CTreeView
{
	BOOL		m_bDragging;
	CImageList*	m_pImageList;
	HTREEITEM	m_hRootTree[3], m_hItemDrag, m_hItemDrop;
	CLayerData*	m_pDragLayer;	// ��ׯ�ޱ���(�ȉ������g�p�̂���boost::variant�g�p�s��)
	CDXFshape*	m_pDragShape;
	DWORD		m_dwDragRoot;

	void	OnUpdateShape(LPDXFADDSHAPE);
	BOOL	IsRootTree(HTREEITEM hTree) {
		return hTree==m_hRootTree[0] || hTree==m_hRootTree[1] || hTree==m_hRootTree[2];
	}
	BOOL	IsDropItem(HTREEITEM);
	void		DragInsert(void);
	HTREEITEM	DragInsertLayer(void);
	HTREEITEM	DragInsertShape(void);
	void		DragLink(void);
	void	DragCancel(BOOL, BOOL = TRUE);
	HTREEITEM	SearchLayerTree(HTREEITEM, const CLayerData*);
	DWORD	GetParentAssemble(HTREEITEM);
	void	SetShapeSwitch_SubordinateTree(HTREEITEM, BOOL);

	void	SetShapeTree(void);
	void	AddWorking(CDXFworking *);
	void	AutoWorkingDel(const CLayerData* = NULL, const CDXFshape* = NULL);
	void	AutoWorkingSet(BOOL, const CLayerData* = NULL, const CDXFshape* = NULL);
	void	UpdateSequence(void);

protected:
	CDXFShapeView();           // ���I�����Ɏg�p�����v���e�N�g �R���X�g���N�^
	DECLARE_DYNCREATE(CDXFShapeView)

// �A�g���r���[�g
public:
	CDXFDoc*	GetDocument();

// �I�y���[�V����
public:

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B

	//{{AFX_VIRTUAL(CDXFShapeView)
	public:
	virtual void OnInitialUpdate();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
protected:
	virtual ~CDXFShapeView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// �������ꂽ���b�Z�[�W �}�b�v�֐�
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
	afx_msg void OnSortShape();
	afx_msg void OnUpdateWorkingDel(CCmdUI* pCmdUI);
	afx_msg void OnWorkingDel();
	afx_msg void OnUpdateEditShapeProp(CCmdUI *pCmdUI);
	afx_msg void OnEditShapeProp();
	afx_msg void OnUpdateEditShapeName(CCmdUI *pCmdUI);
	afx_msg void OnEditShapeName();
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	//}}AFX_MSG
	// �`����H����
	afx_msg void OnUpdateShapePattern(CCmdUI* pCmdUI);

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#ifndef _DEBUG  // DXFShapeView.cpp �t�@�C�����f�o�b�O���̎��g�p����܂��B
inline CDXFDoc* CDXFShapeView::GetDocument()
   { return (CDXFDoc*)m_pDocument; }
#endif
