// ViewSetup6.h : �w�b�_�[ �t�@�C��
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CViewSetup6 �_�C�A���O

class CViewSetup6 : public CPropertyPage
{

// �R���X�g���N�V����
public:
	CViewSetup6();

// �_�C�A���O �f�[�^
	enum { IDD = IDD_VIEW_SETUP6 };
	int		m_nForceView01[4],
			m_nForceView02[4];

// �I�[�o�[���C�h
public:
	virtual BOOL OnApply();
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g

// �C���v�������e�[�V����
protected:
	// �������ꂽ���b�Z�[�W �}�b�v�֐�
	afx_msg void OnChange();
	afx_msg void OnDefColor();

	DECLARE_MESSAGE_MAP()
};
