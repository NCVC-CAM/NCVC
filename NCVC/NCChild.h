// NCChild.h : CNCChild �N���X�̐錾����уC���^�[�t�F�C�X�̒�`�����܂��B
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "MainStatusBar.h"
#include "ChildBase.h"
#include "NCdata.h"

class CNCViewTab;
class CNCListView;

/////////////////////////////////////////////////////////////////////////////
// CNCFrameSplit �X�v���b�^�t���[��

class CNCFrameSplit : public CSplitterWnd  
{
protected:
	// ү����ϯ��
	afx_msg void OnDestroy();
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	// CNCChild::OnCreate() ���� PostMessage()
	afx_msg LRESULT OnUserInitialUpdate(WPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CNCChild �t���[��

class CNCChild : public CMDIChildWnd, public CChildBase
{
	CNCFrameSplit	m_wndSplitter1, m_wndSplitter2;
	CMainStatusBar  m_wndStatusBar;

protected:
	CNCChild();           // ���I�����Ɏg�p�����v���e�N�g �R���X�g���N�^�B
	DECLARE_DYNCREATE(CNCChild)

// �A�g���r���[�g
public:
	CNCViewTab*		GetMainView(void) {
		return (CNCViewTab *)(m_wndSplitter1.GetPane(0, 1));
	}
	CNCListView*	GetListView(void) {
		return (CNCListView *)(m_wndSplitter2.GetPane(1, 0));
	}
	CMainStatusBar*	GetStatusBar(void) {
		return &m_wndStatusBar;
	}
	CProgressCtrl*	GetProgressCtrl(void) {
		return GetStatusBar()->GetProgressCtrl();
	}

// �I�y���[�V����
public:
	void	SetWorkRect(BOOL, CRect3D&);	// from NCWorkDlg.cpp
	void	SetJumpList(int);				// from NCJumpDlg.cpp
	void	SetFactorInfo(ENNCVPLANE, double);

	// CNCListView ү���������
	void	OnUpdateStatusLineNo(int, int, const CNCblock*);

//�I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B
	//{{AFX_VIRTUAL(CNCChild)
	public:
	virtual void ActivateFrame(int nCmdShow = -1);
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	protected:
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
public:
	virtual ~CNCChild();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// �����������b�Z�[�W �}�b�v�֐�
protected:
	//{{AFX_MSG(CNCChild)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnMDIActivate(BOOL bActivate, CWnd* pActivateWnd, CWnd* pDeactivateWnd);
	afx_msg void OnClose();
	//}}AFX_MSG
	// ̧�ٕύX�ʒm from DocBase.cpp
	afx_msg LRESULT OnUserFileChangeNotify(WPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()
};
