// DxfSetup1.h : �w�b�_�[ �t�@�C��
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CDxfSetup1 �_�C�A���O

class CDxfSetup1 : public CPropertyPage
{
	void	EnableReloadButton(void);

// �R���X�g���N�V����
public:
	CDxfSetup1();

// �_�C�A���O �f�[�^
	//{{AFX_DATA(CDxfSetup1)
	enum { IDD = IDD_DXF_SETUP1 };
	CEdit		m_ctCamLayer;
	CEdit		m_ctOrgLayer;
	CButton		m_ctReload;
	CString		m_strCamLayer;
	CString		m_strOrgLayer;
	int			m_nOrgType;
	//}}AFX_DATA

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B

	//{{AFX_VIRTUAL(CDxfSetup1)
	public:
	virtual BOOL OnSetActive();
	virtual BOOL OnKillActive();
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
protected:
	// �������ꂽ���b�Z�[�W �}�b�v�֐�
	//{{AFX_MSG(CDxfSetup1)
	virtual BOOL OnInitDialog();
	afx_msg void OnReload();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
