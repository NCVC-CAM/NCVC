// DXFView.h : �w�b�_�[ �t�@�C��
//

#pragma once

#include "ViewBase.h"

// �ذ�ޭ��őI������Ă�������
typedef	boost::variant<DWORD, CLayerData*, CDXFshape*, CDXFworking*>	DXFTREETYPE;
#define	DXFTREETYPE_MUSTER	0
#define	DXFTREETYPE_LAYER	1
#define	DXFTREETYPE_SHAPE	2
#define	DXFTREETYPE_WORKING	3
// ���݂̉��H�w��
enum	DXFPROCESSDIRECT {
	DXFPROCESS_SELECT, DXFPROCESS_ARRAW, DXFPROCESS_START, DXFPROCESS_OUTLINE
};

/////////////////////////////////////////////////////////////////////////////
// CDXFView �r���[

class CDXFView : public CView, public CViewBase
{
	CPointD		m_ptArraw[2][3];	// �ꎞ�I�Ȏn�_�I�_�̖����W
	CDXFchain	m_ltOutline[2];		// �ꎞ�I�ȗ֊s��޼ު��
	DXFPROCESSDIRECT	m_enProcessDirect;	// �����H�w���ɉ���`�悷�邩
	int			m_nSelect;			// m_ptArraw[0|1] or -1
	DXFTREETYPE	m_vSelect;			// ���ݑI������Ă����ذ��޼ު��
	CDXFdata*	m_pSelData;			// �@�V�@��޼ު��(OnLButtonUp)

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
	CDXFView();           // ���I�����Ɏg�p�����v���e�N�g �R���X�g���N�^
	DECLARE_DYNCREATE(CDXFView)

// �A�g���r���[�g
public:
	CDXFDoc*	GetDocument();

// �I�y���[�V����
public:

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B
	//{{AFX_VIRTUAL(CDXFView)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void OnDraw(CDC* pDC);      // ���̃r���[��`�悷�邽�߂ɃI�[�o�[���C�h���܂����B
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
protected:
	virtual ~CDXFView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// �������ꂽ���b�Z�[�W �}�b�v�֐�
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
	// OnInitialUpdate() ���� PostMessage() or �e�ޭ��ւ�̨��ү����
	afx_msg LRESULT OnUserViewFitMsg(WPARAM, LPARAM);
	// MDI�q�ڰт̽ð���ް�\���X�V(��޼ު��ID��ClassWizard�ꗗ�ɍڂ�Ȃ��̂Ŏ蓮���ިݸ�)
	afx_msg void OnUpdateMouseCursor(CCmdUI* pCmdUI);
	// �ړ�
	afx_msg	void OnMoveKey(UINT);
	afx_msg	void OnLensKey(UINT);

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#ifndef _DEBUG  // DXFView.cpp �t�@�C�����f�o�b�O���̎��g�p����܂��B
inline CDXFDoc* CDXFView::GetDocument()
   { return (CDXFDoc*)m_pDocument; }
#endif
