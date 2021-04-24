// DXFView.h : �w�b�_�[ �t�@�C��
//

#pragma once

#include "ViewBase.h"

// �ذ�ޭ��őI������Ă�������
typedef	boost::variant<DWORD_PTR, CLayerData*, CDXFshape*, CDXFworking*>	DXFTREETYPE;
enum {
	DXFTREETYPE_MUSTER = 0,
	DXFTREETYPE_LAYER,
	DXFTREETYPE_SHAPE,
	DXFTREETYPE_WORKING
};

// �q�ޭ��̑I����� -> vector<> m_bindSel
struct SELECTBIND
{
	int		nSel;
	CPoint	ptDiff;
	CRect	rc;
	bool operator < (const SELECTBIND& sel) const {
		return nSel > sel.nSel;		// �~��
	}
};

// ϳ�����̑J��(�q����޳�ړ��p)
enum ENBINDSTATE
{
	BD_NONE = 0,
	BD_SELECT,
	BD_MOVE,
	BD_CANCEL
};
/*
// CToolTipCtrl���ذ��΍� -> ������...
struct TOOLINFO_EX : public TOOLINFO
{
	void*	lpReserved;
};
*/
/////////////////////////////////////////////////////////////////////////////
// CDXFView �r���[

class CDXFView : public CViewBase
{
	CPointF		m_ptArraw[2][3],	// �ꎞ�I�Ȏn�_�I�_�̖����W
				m_ptStart[4];		// �ꎞ�I�ȊJ�n�ʒu(�~�ōő�4�_)
	CDXFchain	m_ltOutline[2];		// �ꎞ�I�ȗ֊s��޼ު��
	CDXFdata*	m_pSelData;			// �@�V�@��޼ު��(OnLButtonUp)
	DXFTREETYPE	m_vSelect;			// ���ݑI������Ă����ذ��޼ު��
	float		m_dOffset;			// ���̵̾�Ēl
	int			m_nSelect;			// m_ptArraw[0|1] or -1
	CRect		m_rcDrawWork;		// ܰ���`(bind)
	std::vector<SELECTBIND>					m_bindSel;	// �I�����
	CTypedPtrList<CPtrList, LPCADBINDINFO>	m_bindUndo;	// �ړ���UNDO�~��
	ENBINDSTATE	m_enBind;
	// ��������ذ�������
//	CToolTipCtrl*	m_pToolTip;		// �q�ޭ��pToolTip(̧�ٖ��\��)
//	CMFCToolTipCtrl*	m_pToolTip;	// �q�ޭ��pToolTip(̧�ٖ��\��)

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
	CDXFView();           // ���I�����Ɏg�p�����v���e�N�g �R���X�g���N�^
	DECLARE_DYNCREATE(CDXFView)

// �A�g���r���[�g
public:
	CDXFDoc*	GetDocument();

// �I�y���[�V����
public:
	virtual ~CDXFView();

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B
	//{{AFX_VIRTUAL(CDXFView)
	public:
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
//	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void OnInitialUpdate();
	virtual INT_PTR OnToolHitTest(CPoint point, TOOLINFO* pTI) const;
	protected:
	virtual void OnDraw(CDC* pDC);      // ���̃r���[��`�悷�邽�߂ɃI�[�o�[���C�h���܂����B
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

	// �������ꂽ���b�Z�[�W �}�b�v�֐�
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
	// OnInitialUpdate() ���� PostMessage() or �e�ޭ��ւ�̨��ү����
	afx_msg LRESULT OnUserViewFitMsg(WPARAM, LPARAM);
	// MDI�q�ڰт̽ð���ް�\���X�V(��޼ު��ID��ClassWizard�ꗗ�ɍڂ�Ȃ��̂Ŏ蓮���ިݸ�)
	afx_msg void OnUpdateMouseCursor(CCmdUI* pCmdUI);
	// CAD�ް��̓���
	afx_msg void OnUpdateEditBind(CCmdUI* pCmdUI);
	afx_msg void OnEditBindDel();
	afx_msg void OnEditBindTarget();
	afx_msg LRESULT OnBindInitMsg(WPARAM, LPARAM);
	afx_msg LRESULT OnBindLButtonDown(WPARAM, LPARAM);
	afx_msg LRESULT OnBindRoundMsg(WPARAM, LPARAM);
	afx_msg LRESULT OnBindCancel(WPARAM, LPARAM);
	// �ړ�
	afx_msg	void OnMoveKey(UINT);
	afx_msg	void OnLensKey(UINT);
	//
	virtual	void	OnViewLensComm(void);

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#ifndef _DEBUG  // DXFView.cpp �t�@�C�����f�o�b�O���̎��g�p����܂��B
inline CDXFDoc* CDXFView::GetDocument()
   { return static_cast<CDXFDoc *>(m_pDocument); }
#endif
