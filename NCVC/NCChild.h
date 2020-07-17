// NCChild.h : CNCChild �N���X�̐錾����уC���^�[�t�F�C�X�̒�`�����܂��B
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "MainStatusBar.h"
#include "ChildBase.h"
#include "NCdata.h"

class CNCViewTab;
class CNCListView;
class CNCInfoTab;

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
	int		m_nPos, m_nMaxSize;
	boost::variant<CNCblock*, CNCdata*>	m_vStatus;

protected:
	CNCChild();           // ���I�����Ɏg�p�����v���e�N�g �R���X�g���N�^�B
	DECLARE_DYNCREATE(CNCChild)

// �A�g���r���[�g
public:
	CNCViewTab*		GetMainView(void) {
		return reinterpret_cast<CNCViewTab *>(m_wndSplitter1.GetPane(0, 1));
	}
	CNCListView*	GetListView(void) {
		return reinterpret_cast<CNCListView *>(m_wndSplitter2.GetPane(1, 0));
	}
	CNCInfoTab*		GetInfoView(void) {
		return reinterpret_cast<CNCInfoTab *>(m_wndSplitter2.GetPane(0, 0));
	}
	void	SetStatusMaxLine(int nSize) {
		m_nMaxSize = nSize;
	}
	void	SetStatusInfo(int nPos, CNCblock* pBlock) {
		m_nPos = nPos;
		m_vStatus = pBlock;
	}
	void	SetStatusInfo(int nPos, CNCdata* pData) {
		m_nPos = nPos;
		m_vStatus = pData;
	}

// �I�y���[�V����
public:
	void	SetJumpList(int);					// from NCJumpDlg.cpp
	void	SetFindList(int, const CString&);	// from NCFindDlg.cpp
	void	SetFactorInfo(double, const CString&);

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
	// �ð���ް�̍X�V from NCListView.cpp
	afx_msg LRESULT OnUpdateStatusLineNo(WPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()
};
