// NCViewBase.h: CNCViewBase �N���X�̃C���^�[�t�F�C�X
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "ViewBase.h"
#include "NCdata.h"	// ENNCDRAWVIEW

class CNCViewBase : public CViewBase
{
	ENNCDRAWVIEW	m_enView;	// �h���ޭ��̎���

	virtual	CRectD	ConvertRect(const CRect3D& rc) {
		CRectD	rcResult(rc);
		return rcResult;					// CNCViewXY�p
	}
	virtual	boost::tuple<size_t, size_t>	GetPlaneAxis(void) {
		size_t	x = NCA_X, y = NCA_Y;
		return boost::make_tuple(x, y);		// CNCViewXY�p
	};
	virtual	void	SetGuideData(void) = 0;

public:
	CNCDoc*	GetDocument();
	virtual BOOL OnCmdMsg(UINT, int, void*, AFX_CMDHANDLERINFO*);
	virtual void OnInitialUpdate();
	virtual	void OnDraw(CDC*);

protected:
	CNCViewBase(ENNCDRAWVIEW enType) {
		m_enView = enType;
		m_pfnDrawProc = NULL;
	}
	CString			m_strGuide;			// ���ʈē�����
	PFNNCDRAWPROC	m_pfnDrawProc;		// �`��֐��߲��
	CRectD			m_rcDataMax;		// ��U���GetDocument()->GetMaxRect()
	CPoint			m_ptGuide[NCXYZ][2];// XYZ���̶޲�ލ��W(�n�_�E�I�_)
	// XY, XZ, YZ ���ʗp (XYZ��CNCView���Ǝ��Ɏ���)
	CRect	m_rcDrawWork;		// ܰ���`
	CRect	m_rcDrawMax;		// �ް���`
	//
	virtual	void	SetDataMaxRect(void);
	virtual	void	SetWorkRect(void) {}	// CNCView�̂�
	virtual	void	ConvertWorkRect(void);
	virtual	void	ConvertMaxRect(void);
	virtual	void	DrawWorkRect(CDC* pDC) {
		pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenNC(NCPEN_WORK));
		pDC->Rectangle(m_rcDrawWork);
	}
	virtual	void	DrawMaxRect(CDC* pDC) {
		pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenNC(NCPEN_MAXCUT));
		pDC->Rectangle(m_rcDrawMax);
	}
	void	DrawGuideDivi(CDC*, size_t, size_t);
	void	DrawInfo(CDC*);
	void	DrawNCdata(CDC*);
	void	DrawSupportRect(CDC*);
	//
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	virtual void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);
	// ����ү���ތQ
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg int OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message);
	afx_msg void OnUpdateEditCopy(CCmdUI* pCmdUI);
	afx_msg void OnUpdateMoveKey (CCmdUI* pCmdUI);
	afx_msg void OnUpdateRoundKey(CCmdUI* pCmdUI);
	afx_msg void OnEditCopy();
	afx_msg	void OnMoveKey(UINT);
	afx_msg	void OnLensKey(UINT);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg LRESULT OnUserViewFitMsg(WPARAM, LPARAM);
	afx_msg LRESULT OnUserActivatePage(WPARAM, LPARAM);
	//
	void	OnViewLensComm(void);

	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG
inline CNCDoc* CNCViewBase::GetDocument()
   { return static_cast<CNCDoc *>(m_pDocument); }
#endif
