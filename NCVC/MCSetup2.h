// MCSetup2.h : �w�b�_�[ �t�@�C��
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMCSetup2 �_�C�A���O

class CMCSetup2 : public CPropertyPage
{
// �R���X�g���N�V����
public:
	CMCSetup2();

// �_�C�A���O �f�[�^
	//{{AFX_DATA(CMCSetup2)
	enum { IDD = IDD_MC_SETUP2 };
		// ���� - ClassWizard �͂��̈ʒu�Ƀf�[�^ �����o��ǉ����܂��B
		//    ���̈ʒu�ɐ��������R�[�h��ҏW���Ȃ��ł��������B
	//}}AFX_DATA
	CFloatEdit	m_dInitialXYZ[NCXYZ];
	CFloatEdit	m_dWorkOffset[WORKOFFSET][NCXYZ];

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B

	//{{AFX_VIRTUAL(CMCSetup2)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
protected:
	// �������ꂽ���b�Z�[�W �}�b�v�֐�
	//{{AFX_MSG(CMCSetup2)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
