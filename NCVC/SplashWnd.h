// SplashWnd.h : �w�b�_�[ �t�@�C��
//
#pragma once

/////////////////////////////////////////////////////////////////////////////
// CSplashWnd �E�B���h�E

class CSplashWnd : public CWnd
{
	CBitmap	m_bitmap;
	BITMAP	m_bm;

// �R���X�g���N�V����
public:
	CSplashWnd();

// �A�g���r���[�g
public:
	static	BOOL	ms_bShowSplashWnd;

// �I�y���[�V����
public:

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B

	//{{AFX_VIRTUAL(CSplashWnd)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void PostNcDestroy();
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
public:

	// �������ꂽ���b�Z�[�W �}�b�v�֐�
protected:
	//{{AFX_MSG(CSplashWnd)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPaint();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
