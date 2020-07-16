// MainStatusBar.h : �w�b�_�[ �t�@�C��
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMainStatusBar �E�B���h�E

class CMainStatusBar : public CStatusBar
{
	CProgressCtrl	m_ctProgress;

// �R���X�g���N�V����
public:
	CMainStatusBar();

// �A�g���r���[�g
public:
	CProgressCtrl*	GetProgressCtrl(void) {
		return &m_ctProgress;
	}

// �I�y���[�V����
public:
	void	EnableProgress(BOOL f = TRUE) {
		m_ctProgress.ShowWindow(f ? SW_SHOWNA : SW_HIDE);
	}
	void	ChangeProgressSize(int, int);	// �e�ڰт��Ƃ���۸�ڽ���۰ق̻��ޕύX
											// OnSize() �̑���

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B
	//{{AFX_VIRTUAL(CMainStatusBar)
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
public:
	virtual ~CMainStatusBar();

	// �������ꂽ���b�Z�[�W �}�b�v�֐�
protected:
	//{{AFX_MSG(CMainStatusBar)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	afx_msg LRESULT OnUserProgressPos(WPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()
};
