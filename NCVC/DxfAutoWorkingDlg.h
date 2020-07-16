// DxfAutoWorkingDlg.h : �w�b�_�[ �t�@�C��
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CDxfAutoWorkingDlg �_�C�A���O

class CDxfAutoWorkingDlg : public CDialog
{
	void	SetDetailCtrl(void);

// �R���X�g���N�V����
public:
	CDxfAutoWorkingDlg(double);

// �_�C�A���O �f�[�^
	//{{AFX_DATA(CDxfAutoWorkingDlg)
	enum { IDD = IDD_DXFEDIT_AUTOWORKING };
	int		m_nSelect;
	int		m_nDetail;
	CFloatEdit	m_ctOffset;
	CButton m_ctDetail[4];
	//}}AFX_DATA
	double	m_dOffset;	// �̾�Ēl�擾�p

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B
	//{{AFX_VIRTUAL(CDxfAutoWorkingDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
protected:
	// �������ꂽ���b�Z�[�W �}�b�v�֐�
	//{{AFX_MSG(CDxfAutoWorkingDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnBnClickedSelect();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
