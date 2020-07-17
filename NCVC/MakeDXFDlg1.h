// MakeDXFDlg1.h : �w�b�_�[ �t�@�C��
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMakeDXFDlg1 �_�C�A���O

class CMakeDXFDlg1 : public CPropertyPage
{
	// ��è�����۰قɕ\������O�̏ȗ��`������
	CString			m_strDXFPath;	// �{�����߽��

// �R���X�g���N�V����
public:
	CMakeDXFDlg1();

// �_�C�A���O �f�[�^
	//{{AFX_DATA(CMakeDXFDlg1)
	enum { IDD = IDD_MAKEDXF1 };
	CEdit	m_ctDXFFileName;
	CString	m_strDXFFileName;
	int		m_nPlane;
	//}}AFX_DATA
	BOOL			m_bOut[4];
	CString			m_strLayer[4];
	CComboBox		m_cbLineType[4];
	CColComboBox	m_ctColor[4];

	CString	GetDXFFileName(void) {
		return m_strDXFPath + m_strDXFFileName;
	}

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B

	//{{AFX_VIRTUAL(CMakeDXFDlg1)
	public:
	virtual BOOL OnApply();
	virtual BOOL OnKillActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
protected:
	// �������ꂽ���b�Z�[�W �}�b�v�֐�
	//{{AFX_MSG(CMakeDXFDlg1)
	virtual BOOL OnInitDialog();
	afx_msg void OnMKDXFileUp();
	afx_msg void OnKillFocusDXFFile();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
