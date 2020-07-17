// DXFChild.h : �w�b�_�[ �t�@�C��
//

#pragma once

#include "ChildBase.h"

class	CDXFDoc;
class	CDXFView;
class	CDXFShapeView;

/////////////////////////////////////////////////////////////////////////////
// CDXFFrameSplit �X�v���b�^�t���[��

class CDXFFrameSplit : public CSplitterWnd
{
protected:
	// ү����ϯ��
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CDXFChild �t���[��

class CDXFChild : public CChildBase
{
	CDXFFrameSplit	m_wndSplitter;
	CStatusBar		m_wndStatusBar;

protected:
	CDXFChild();           // ���I�����Ɏg�p�����v���e�N�g �R���X�g���N�^�B
	DECLARE_DYNCREATE(CDXFChild)

// �A�g���r���[�g
public:
	CDXFView*		GetMainView(void) {
		return reinterpret_cast<CDXFView *>(m_wndSplitter.GetPane(0, 0));
	}
	CDXFShapeView*	GetTreeView(void) {
		return reinterpret_cast<CDXFShapeView *>(m_wndSplitter.GetPane(0, 1));
	}

// �I�y���[�V����
public:
	void	SetDataInfo(const CDXFDoc*);
	void	SetFactorInfo(float);
	void	ShowShapeView(void);

	// CDXFView ү���������
	void	OnUpdateMouseCursor(const CPointF* = NULL);

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂�

	//{{AFX_VIRTUAL(CDXFChild)
	public:
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	protected:
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	//}}AFX_VIRTUAL

// �������ꂽ���b�Z�[�W �}�b�v�֐�
protected:

	//{{AFX_MSG(CDXFChild)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnMDIActivate(BOOL bActivate, CWnd* pActivateWnd, CWnd* pDeactivateWnd);
	//}}AFX_MSG
	// հ�޲Ƽ�ُ���
	afx_msg LRESULT OnUserInitialUpdate(WPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()
};
