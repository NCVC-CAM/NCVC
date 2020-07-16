// DxfSetup2.h : �w�b�_�[ �t�@�C��
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CDxfSetup2 �_�C�A���O

class CDxfSetup2 : public CPropertyPage
{
	void	EnableReloadButton(void);

// �R���X�g���N�V����
public:
	CDxfSetup2();
	~CDxfSetup2();

// �_�C�A���O �f�[�^
	//{{AFX_DATA(CDxfSetup2)
	enum { IDD = IDD_DXF_SETUP2 };
	CButton	m_ctReload;
	CEdit	m_ctStartLayer;
	CString	m_strStartLayer;
	CString	m_strMoveLayer;
	CString	m_strCommentLayer;
	//}}AFX_DATA

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B

	//{{AFX_VIRTUAL(CDxfSetup2)
	public:
	virtual BOOL OnSetActive();
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
protected:
	// �������ꂽ���b�Z�[�W �}�b�v�֐�
	//{{AFX_MSG(CDxfSetup2)
	virtual BOOL OnInitDialog();
	afx_msg void OnReload();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
