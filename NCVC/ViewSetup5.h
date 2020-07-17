// ViewSetup5.h : �w�b�_�[ �t�@�C��
//

#pragma once
#include "afxwin.h"

/////////////////////////////////////////////////////////////////////////////
// CViewSetup5 �_�C�A���O

class CViewSetup5 : public CPropertyPage
{
	void	EnableControl(void);

// �R���X�g���N�V����
public:
	CViewSetup5();
	virtual ~CViewSetup5();

// �_�C�A���O �f�[�^
	enum { IDD = IDD_VIEW_SETUP5 };
	BOOL	m_bSolid,
			m_bG00View,
			m_bDrag,
			m_bMillT,
			m_bMillC;
	int		m_nMillType;
	CButton m_ctG00View,
			m_ctDrag;
	CFloatEdit	m_dEndmill;

// �I�[�o�[���C�h
public:
	virtual BOOL OnKillActive();
	virtual BOOL OnApply();
protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g

// �C���v�������e�[�V����
protected:
	// �������ꂽ���b�Z�[�W �}�b�v�֐�
	afx_msg void OnSolidClick();
	afx_msg void OnChange();

	DECLARE_MESSAGE_MAP()
};
