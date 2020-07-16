// DxfEditOrgDlg.h : �w�b�_�[ �t�@�C��
//

#pragma once

//	���۰ق̔�è���׸�
#define	EDITORG_NUMERIC		0x0001
#define	EDITORG_ORIGINAL	0x0002

/////////////////////////////////////////////////////////////////////////////
// CDxfEditOrgDlg �_�C�A���O

class CDxfEditOrgDlg : public CDialog
{
	DWORD	m_dwControl;	// ���۰ق̔�è���׸�
	void	SelectControl(int);

// �R���X�g���N�V����
public:
	CDxfEditOrgDlg(DWORD dwControl);

// �_�C�A���O �f�[�^
	//{{AFX_DATA(CDxfEditOrgDlg)
	enum { IDD = IDD_DXFEDIT_ORG };
	CButton	m_ctSelNumeric;
	CButton	m_ctSelOriginal;
	CComboBox	m_ctRectType;
	CEdit	m_ctNumeric;
	int		m_nSelect;
	CString	m_strNumeric;
	int		m_nRectType;
	//}}AFX_DATA

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B
	//{{AFX_VIRTUAL(CDxfEditOrgDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
protected:

	// �������ꂽ���b�Z�[�W �}�b�v�֐�
	//{{AFX_MSG(CDxfEditOrgDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelect();
	virtual void OnOK();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
