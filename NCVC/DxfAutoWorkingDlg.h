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
	CDxfAutoWorkingDlg(AUTOWORKINGDATA*);

// �_�C�A���O �f�[�^
	//{{AFX_DATA(CDxfAutoWorkingDlg)
	enum { IDD = IDD_DXFEDIT_AUTOWORKING };
	int		m_nSelect;
	BOOL	m_bAcuteRound;
	int		m_nScan;
	BOOL	m_bCircle;
	CFloatEdit	m_ctOffset;
	CIntEdit m_ctLoop;
	CComboBox m_ctScan;
	CButton m_ctAcuteRound;
	CButton m_ctCircle;
	//}}AFX_DATA
	double	m_dOffset;	// �̾�Ēl�擾�p
	int		m_nLoopCnt;	// �J��Ԃ����擾�p

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
