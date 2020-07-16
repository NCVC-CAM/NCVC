// MakeNCDlgEx2.h : �w�b�_�[ �t�@�C��
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMakeNCDlgEx2 �_�C�A���O

class CMakeNCDlgEx2 : public CDialog
{
	CDXFDoc*	m_pDoc;			// ڲԏ��擾
	int			m_nSortLayer;	// ��Ē��̗�ԍ�
	// ��è�����۰قɕ\������O�̏ȗ��`������
	CString		m_strNCPath,	// �{�����߽��
				m_strInitPath,
				m_strLayerToInitPath;

	void	OnLayerEdit(int);

public:
	// CMakeNCDlgEx21 �ւ̏���
	CDXFDoc*	GetDocument(void) {
		return m_pDoc;
	}
	CString	GetNCFileName(void) {
		return m_strNCPath + m_strNCFileName;
	}

// �R���X�g���N�V����
public:
	CMakeNCDlgEx2(CDXFDoc*);
	virtual ~CMakeNCDlgEx2();

// �_�C�A���O �f�[�^
	//{{AFX_DATA(CMakeNCDlgEx2)
	enum { IDD = IDD_MAKENCD_EX2 };
	CButton	m_ctOK;
	CEdit	m_ctNCFileName;
	CString	m_strNCFileName;
	CComboBox	m_ctInitFileName;
	CString	m_strInitFileName;
	CComboBox	m_ctLayerToInitFileName;
	CString	m_strLayerToInitFileName;
	CListCtrl	m_ctLayerList;
	BOOL	m_bNCView;
	//}}AFX_DATA

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B
	//{{AFX_VIRTUAL(CMakeNCDlgEx2)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
protected:
	// ��ėp����ޯ��֐�
	static int CALLBACK CompareFunc(LPARAM, LPARAM, LPARAM);

	// �������ꂽ���b�Z�[�W �}�b�v�֐�
	//{{AFX_MSG(CMakeNCDlgEx2)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnMKNCFileUp();
	afx_msg void OnMKNCInitUp();
	afx_msg void OnMKNCInitEdit();
	afx_msg void OnKillFocusNCFile();
	afx_msg void OnKillFocusInit();
	afx_msg void OnKillFocusLayerToInit();
	afx_msg void OnGetDispInfoLayerList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblClkLayerList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnColumnClickLayerList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSelChangeLayerToInit();
	afx_msg void OnSelChangeInit();
	afx_msg void OnMKNCLayerEdit();
	afx_msg void OnMKNCLayerUp();
	afx_msg void OnEndScrollLayerList(NMHDR *pNMHDR, LRESULT *pResult);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
