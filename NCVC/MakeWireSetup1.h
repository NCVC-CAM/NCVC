// MakeWireSetup1.h : �w�b�_�[ �t�@�C��
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMakeWireSetup1 �_�C�A���O

class CMakeWireSetup1 : public CPropertyPage
{
	// ��è�����۰قɕ\������O�̏ȗ��`������
	CString		m_strHeaderPath,	// �{�����߽��
				m_strFooterPath;

public:
	CMakeWireSetup1();

// �_�C�A���O �f�[�^
	enum { IDD = IDD_MKWI_SETUP1 };
	CButton	m_ctButton2;
	CButton	m_ctButton1;
	CEdit	m_ctHeader;
	CEdit	m_ctFooter;
	CString	m_strTaperMode;
	CString	m_strFooter;
	CString	m_strHeader;
	CFloatEdit m_dDepth;
	CFloatEdit m_dTaper;
	CFloatEdit m_dFeed;
	CFloatEdit m_dG92X;
	CFloatEdit m_dG92Y;

protected:
	virtual BOOL OnApply();
	virtual BOOL OnKillActive();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	virtual BOOL OnInitDialog();
	afx_msg void OnHeaderLoopup();
	afx_msg void OnFooterLoopup();
	afx_msg void OnHeaderEdit();
	afx_msg void OnFooterEdit();

	DECLARE_MESSAGE_MAP()
};
