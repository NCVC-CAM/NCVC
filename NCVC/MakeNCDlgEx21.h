// MakeNCDlgEx21.h : �w�b�_�[ �t�@�C��
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMakeNCDlgEx21 �_�C�A���O

class CMakeNCDlgEx21 : public CDialog
{
	CString		m_strCaption;	// ���̳���޳����
	int			m_nIndex;		// ̫����𓾂��Ƃ���ڲԔz��ԍ�
	// ��è�����۰قɕ\������O�̏ȗ��`������
	CString		m_strNCPath;	// �{�����߽��

	void	GetNowState(void);
	void	SetNowState(int);
	void	EnablePartOut(void);

// �R���X�g���N�V����
public:
	CMakeNCDlgEx21(CMakeNCDlgEx*, int);
	virtual ~CMakeNCDlgEx21();

// �_�C�A���O �f�[�^
	//{{AFX_DATA(CMakeNCDlgEx21)
	enum { IDD = IDD_MAKENCD_EX2_1 };
	CStatic	m_ctNCPath,
			m_ctPartEnable2,
			m_ctPartEnable3;
	CButton	m_ctOK,
			m_ctPartEnable1,
			m_ctPartEnable4;
	CComboBox	m_ctLayer;
	CEdit	m_ctNCFileName;
	CFloatEdit	m_dZStep,
				m_dZCut;
	BOOL	m_bDrill,
			m_bCheck,
			m_bPartOut;
	CString	m_strNCFileName,
			m_strLayerComment,
			m_strLayerCode;
	//}}AFX_DATA
	CLayerArray	m_obLayer;		// ڲԏ��ꎞ�Ҕ�(���ؽĺ��۰ُ�)

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
	afx_msg void OnNewLayer();
	afx_msg void OnStep();
	afx_msg void OnKillFocusNCFile();
	afx_msg void OnMKNCFileUp();
	afx_msg void OnCopy();
	afx_msg void OnPartOut();
	afx_msg void OnSelChangeLayer();
	afx_msg void OnSetFocusLayer();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
