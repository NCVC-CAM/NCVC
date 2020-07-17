// MakeNCDlgEx11.h : �w�b�_�[ �t�@�C��
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMakeNCDlgEx11 �_�C�A���O

class CMakeNCDlgEx11 : public CDialog
{
	CString		m_strCaption;	// ���̳���޳����
	int			m_nIndex;		// ̫����𓾂��Ƃ���ڲԔz��ԍ�
	// ��è�����۰قɕ\������O�̏ȗ��`������
	CString		m_strNCPath,	// �{�����߽��
				m_strInitPath;

	void	GetNowState(void);
	void	SetNowState(int);
	void	EnablePartOut(void);

// �R���X�g���N�V����
public:
	CMakeNCDlgEx11(CMakeNCDlgEx*, int);
	virtual ~CMakeNCDlgEx11();

// �_�C�A���O �f�[�^
	//{{AFX_DATA(CMakeNCDlgEx11)
	enum { IDD = IDD_MAKENCD_EX1_1 };
	CStatic	m_ctNCPath,
			m_ctInitPath,
			m_ctPartEnable2,
			m_ctPartEnable3;
	CButton	m_ctOK,
			m_ctPartEnable1,
			m_ctPartEnable4;
	CComboBox	m_ctLayer,
				m_ctInitFileName;
	CEdit	m_ctNCFileName;
	BOOL	m_bCheck,
			m_bPartOut;
	CString	m_strInitFileName,
			m_strNCFileName,
			m_strLayerComment,
			m_strLayerCode;
	//}}AFX_DATA
	CLayerArray	m_obLayer;		// ڲԏ��ꎞ�Ҕ�(���ؽĺ��۰ُ�)

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B
	//{{AFX_VIRTUAL(CMakeNCDlgEx11)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
protected:

	// �������ꂽ���b�Z�[�W �}�b�v�֐�
	//{{AFX_MSG(CMakeNCDlgEx11)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnNewLayer();
	afx_msg void OnMKNCInitUp();
	afx_msg void OnMKNCInitEdit();
	afx_msg void OnSelChangeInit();
	afx_msg void OnKillFocusInit();
	afx_msg void OnKillFocusNCFile();
	afx_msg void OnMKNCFileUp();
	afx_msg void OnCopy();
	afx_msg void OnPartOut();
	afx_msg void OnSelChangeLayer();
	afx_msg void OnSetFocusLayer();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
