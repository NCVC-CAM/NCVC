// DXFChild.h : �w�b�_�[ �t�@�C��
//

#pragma once

#include "ChildBase.h"

class	CDXFView;

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

class CDXFChild : public CMDIChildWnd, public CChildBase
{
	CDXFFrameSplit	m_wndSplitter;
	CStatusBar		m_wndStatusBar;

protected:
	CDXFChild();           // ���I�����Ɏg�p�����v���e�N�g �R���X�g���N�^�B
	DECLARE_DYNCREATE(CDXFChild)

// �A�g���r���[�g
public:
	CDXFView*	GetMainView(void) {
		return reinterpret_cast<CDXFView *>(m_wndSplitter.GetPane(0, 0));
	}

// �I�y���[�V����
public:
	void	SetDataInfo(int, int, int, int, int);
	void	SetFactorInfo(double);
	void	ShowShapeView(void);

	// CDXFView ү���������
	void	OnUpdateMouseCursor(const CPointD* = NULL);

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂�

	//{{AFX_VIRTUAL(CDXFChild)
	public:
	virtual void ActivateFrame(int nCmdShow = -1);
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	protected:
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
protected:
	virtual ~CDXFChild();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// �������ꂽ���b�Z�[�W �}�b�v�֐�
protected:

	//{{AFX_MSG(CDXFChild)
	afx_msg void OnMDIActivate(BOOL bActivate, CWnd* pActivateWnd, CWnd* pDeactivateWnd);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnClose();
	//}}AFX_MSG
	// հ�޲Ƽ�ُ���
	afx_msg LRESULT OnUserInitialUpdate(WPARAM, LPARAM);
	// ̧�ٕύX�ʒm from DocBase.cpp
	afx_msg LRESULT OnUserFileChangeNotify(WPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()
};
