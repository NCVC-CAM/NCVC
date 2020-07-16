// MakeNCDlgEx21.h : �w�b�_�[ �t�@�C��
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMakeNCDlgEx21 �_�C�A���O

class CMakeNCDlgEx21 : public CDialog
{
	CString		m_strCaption;	// ���̳���޳����
	int			m_nIndex;	// ̫����𓾂��Ƃ���ڲԔz��ԍ�
	// ��è�����۰قɕ\������O�̏ȗ��`������
	CString		m_strNCPath;	// �{�����߽��

	void	GetNowState(void);
	void	SetNowState(int);
	void	EnablePartOut(void);

// �R���X�g���N�V����
public:
	CMakeNCDlgEx21(CMakeNCDlgEx2*, int, const CString&);
	virtual ~CMakeNCDlgEx21();

// �_�C�A���O �f�[�^
	//{{AFX_DATA(CMakeNCDlgEx21)
	enum { IDD = IDD_MAKENCD_EX2_1 };
	CStatic	m_ctPartEnable3;
	CStatic	m_ctPartEnable2;
	CButton	m_ctPartEnable1;
	CComboBox	m_ctLayer;
	CEdit	m_ctNCFileName;
	CStatic	m_ctNCPath;
	CButton	m_ctPartEnable4;
	CButton	m_ctOK;
	CFloatEdit	m_dZStep;
	CFloatEdit	m_dZCut;
	CString	m_strNCFileName;
	BOOL	m_bDrill;
	BOOL	m_bCheck;
	BOOL	m_bPartOut;
	//}}AFX_DATA
	CString		m_strLayerFile;	// ڲ�̧��
	CLayerArray	m_obLayer;	// ڲԏ��ꎞ�Ҕ�(���ؽĺ��۰ُ�)

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B
	//{{AFX_VIRTUAL(CMakeNCDlgEx21)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
protected:

	// �������ꂽ���b�Z�[�W �}�b�v�֐�
	//{{AFX_MSG(CMakeNCDlgEx21)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnStep();
	afx_msg void OnKillFocusNCFile();
	afx_msg void OnMKNCFileUp();
	afx_msg void OnCopy();
	afx_msg void OnPartOut();
	afx_msg void OnSelChangeLayer();
	afx_msg void OnSetFocusLayer();
	afx_msg void OnNewLayerFile();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
