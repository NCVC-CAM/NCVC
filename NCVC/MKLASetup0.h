// MKLASetup0.h : �w�b�_�[ �t�@�C��
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMKLASetup0 �_�C�A���O

class CMKLASetup0 : public CPropertyPage
{
	// ��è�����۰قɕ\������O�̏ȗ��`������
	CString		m_strHeaderPath,	// �{�����߽��
				m_strFooterPath;

public:
	CMKLASetup0();

	// �_�C�A���O �f�[�^
	enum { IDD = IDD_MKLA_SETUP0 };
	CButton	m_ctButton2;
	CButton	m_ctButton1;
	CEdit	m_ctHeader;
	CEdit	m_ctFooter;
	CString	m_strFooter;
	CString	m_strHeader;

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
