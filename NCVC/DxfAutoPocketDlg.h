// DxfAutoPocketDlg.h : �w�b�_�[ �t�@�C��
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CDxfAutoPocketgDlg �_�C�A���O

class CDxfAutoPocketgDlg : public CDialog
{
	void	SetDetailCtrl(void);

// �R���X�g���N�V����
public:
	CDxfAutoPocketgDlg(LPAUTOWORKINGDATA);

// �_�C�A���O �f�[�^
	//{{AFX_DATA(CDxfAutoPocketgDlg)
	enum { IDD = IDD_DXFEDIT_AUTOPOCKET };
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
	float	m_dOffset;	// �̾�Ēl�擾�p
	int		m_nLoopCnt;	// �J��Ԃ����擾�p

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B
	//{{AFX_VIRTUAL(CDxfAutoPocketgDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
protected:
	// �������ꂽ���b�Z�[�W �}�b�v�֐�
	//{{AFX_MSG(CDxfAutoPocketgDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnBnClickedSelect();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
