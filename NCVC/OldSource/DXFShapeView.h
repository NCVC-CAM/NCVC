// DXFShapeView.h : �w�b�_�[ �t�@�C��
//

#if !defined(AFX_DXFSHAPEVIEW_H__967EA6F2_0128_4064_A8F1_4A828C5F9D5E__INCLUDED_)
#define AFX_DXFSHAPEVIEW_H__967EA6F2_0128_4064_A8F1_4A828C5F9D5E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

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

	BOOL		m_bUpdateLayerSequence;		// ڲԏ����ύX���ꂽ
	CLayerMap	m_mpUpdateLayer;			// �`�󏇂��ύX���ꂽڲ�

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
	afx_msg void OnNMRclick(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnSortShape();
	afx_msg void OnUpdateWorkingDel(CCmdUI* pCmdUI);
	afx_msg void OnWorkingDel();
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

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ �͑O�s�̒��O�ɒǉ��̐錾��}�����܂��B

#endif // !defined(AFX_DXFSHAPEVIEW_H__967EA6F2_0128_4064_A8F1_4A828C5F9D5E__INCLUDED_)
