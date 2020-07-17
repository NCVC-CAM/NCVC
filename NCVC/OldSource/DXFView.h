// DXFView.h : �w�b�_�[ �t�@�C��
//

#pragma once

#include "ViewBase.h"

// �ذ�ޭ��őI������Ă�������
typedef	boost::variant<DWORD, CLayerData*, CDXFshape*, CDXFworking*>	DXFTREETYPE;
enum {
	DXFTREETYPE_MUSTER = 0,
	DXFTREETYPE_LAYER,
	DXFTREETYPE_SHAPE,
	DXFTREETYPE_WORKING
};

/////////////////////////////////////////////////////////////////////////////
// CDXFView �r���[

class CDXFView : public CViewBase
{
	CPointD		m_ptArraw[2][3],	// �ꎞ�I�Ȏn�_�I�_�̖����W
				m_ptStart[4];		// �ꎞ�I�ȊJ�n�ʒu(�~�ōő�4�_)
	CDXFchain	m_ltOutline[2];		// �ꎞ�I�ȗ֊s��޼ު��
	CDXFdata*	m_pSelData;			// �@�V�@��޼ު��(OnLButtonUp)
	DXFTREETYPE	m_vSelect;			// ���ݑI������Ă����ذ��޼ު��
	double		m_dOffset;			// ���̵̾�Ēl
	int			m_nSelect,			// m_ptArraw[0|1] or -1
				m_nSelView;			// �q�ޭ�(bind)
	CRect		m_rcDrawWork,		// ܰ���`(bind)
				m_rcMove;			// �q�ޭ�����
	CPoint		m_ptMove;			// �q�ޭ��ړ��̍���
	CTypedPtrListEx<CPtrList, LPCADBINDINFO>	m_bindUndo;	// �ړ���UNDO�~��
	BOOL		m_bMove;

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

	void	OnLButtonUp_Separate(CDC*, CDXFdata*, const CPointD&, const CRectD&);
	void	OnLButtonUp_Vector  (CDC*, CDXFdata*, const CPointD&, const CRectD&);
	void	OnLButtonUp_Start   (CDC*, CDXFdata*, const CPointD&, const CRectD&);
	void	OnLButtonUp_Outline (CDC*, CDXFdata*, const CPointD&, const CRectD&);
	void	OnMouseMove_Separate(CDC*, const CPointD&, const CRectD&);
	void	OnMouseMove_Vector  (CDC*, const CPointD&, const CRectD&);
	void	OnMouseMove_Start   (CDC*, const CPointD&, const CRectD&);
	void	OnMouseMove_Outline (CDC*, const CPointD&, const CRectD&);

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
	virtual void OnInitialUpdate();
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
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
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
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
	// OnInitialUpdate() ���� PostMessage() or �e�ޭ��ւ�̨��ү����
	afx_msg LRESULT OnUserViewFitMsg(WPARAM, LPARAM);
	// MDI�q�ڰт̽ð���ް�\���X�V(��޼ު��ID��ClassWizard�ꗗ�ɍڂ�Ȃ��̂Ŏ蓮���ިݸ�)
	afx_msg void OnUpdateMouseCursor(CCmdUI* pCmdUI);
	// CAD�ް��̓���
	afx_msg void OnEditBindDel();
	afx_msg void OnEditBindTarget();
	afx_msg LRESULT OnBindInitMsg(WPARAM, LPARAM);
	afx_msg LRESULT OnBindMoveMsg(WPARAM, LPARAM);
	afx_msg LRESULT OnBindRoundMsg(WPARAM, LPARAM);
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
