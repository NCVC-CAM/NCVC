// MakeNCDlgEx11.h : �w�b�_�[ �t�@�C��
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMakeNCDlgEx11 �_�C�A���O

class CMakeNCDlgEx11 : public CDialog
{
	CString		m_strCaption;	// ���̳���޳����
	int			m_nIndex;	// ̫����𓾂��Ƃ���ڲԔz��ԍ�
	// ��è�����۰قɕ\������O�̏ȗ��`������
	CString		m_strNCPath,	// �{�����߽��
				m_strInitPath;

	void	GetNowState(void);
	void	SetNowState(int);
	void	EnablePartOut(void);

// �R���X�g���N�V����
public:
	CMakeNCDlgEx11(CMakeNCDlgEx1*, int, const CString&);
	virtual ~CMakeNCDlgEx11();

// �_�C�A���O �f�[�^
	//{{AFX_DATA(CMakeNCDlgEx11)
	enum { IDD = IDD_MAKENCD_EX1_1 };
	CButton	m_ctOK;
	CStatic	m_ctPartEnable3;
	CStatic	m_ctPartEnable2;
	CButton	m_ctPartEnable1;
	CComboBox	m_ctLayer;
	CButton	m_ctPartEnable4;
	CEdit	m_ctNCFileName;
	CStatic	m_ctNCPath;
	CStatic	m_ctInitPath;
	CComboBox	m_ctInitFileName;
	CString	m_strInitFileName;
	CString	m_strNCFileName;
	BOOL	m_bCheck;
	BOOL	m_bPartOut;
	//}}AFX_DATA
	CString		m_strLayerFile;	// ڲ�̧��
	CLayerArray	m_obLayer;	// ڲԏ��ꎞ�Ҕ�(���ؽĺ��۰ُ�)

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
	afx_msg void OnNewLayerFile();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
