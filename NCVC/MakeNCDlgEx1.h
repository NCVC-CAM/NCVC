// MakeNCDlgEx1.h : �w�b�_�[ �t�@�C��
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMakeNCDlgEx1 �_�C�A���O

class CMakeNCDlgEx1 : public CDialog
{
	CDXFDoc*	m_pDoc;			// ڲԏ��擾
	int			m_nSortLayer;	// ��Ē��̗�ԍ�
	// ��è�����۰قɕ\������O�̏ȗ��`������
	CString		m_strNCPath,	// �{�����߽��
				m_strLayerToInitPath;

	void	OnLayerEdit(int);

public:
	// CMakeNCDlgEx11 �ւ̏���
	CDXFDoc*	GetDocument(void) {
		return m_pDoc;
	}
	CString	GetNCFileName(void) {
		return m_strNCPath + m_strNCFileName;
	}

// �R���X�g���N�V����
public:
	CMakeNCDlgEx1(CDXFDoc*);
	virtual ~CMakeNCDlgEx1();

// �_�C�A���O �f�[�^
	//{{AFX_DATA(CMakeNCDlgEx1)
	enum { IDD = IDD_MAKENCD_EX1 };
	CButton	m_ctOK;
	CEdit	m_ctNCFileName;
	CString	m_strNCFileName;
	CComboBox	m_ctLayerToInitFileName;
	CString	m_strLayerToInitFileName;
	CListCtrl	m_ctLayerList;
	BOOL	m_bNCView;
	//}}AFX_DATA

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B
	//{{AFX_VIRTUAL(CMakeNCDlgEx1)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
protected:
	// ��ėp����ޯ��֐�
	static int CALLBACK CompareFunc(LPARAM, LPARAM, LPARAM);

	// �������ꂽ���b�Z�[�W �}�b�v�֐�
	//{{AFX_MSG(CMakeNCDlgEx1)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnGetDispInfoLayerList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblClkLayerList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnColumnClickLayerList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnKillFocusNCFile();
	afx_msg void OnKillFocusLayerToInit();
	afx_msg void OnSelChangeLayerToInit();
	afx_msg void OnMKNCFileUp();
	afx_msg void OnMKNCLayerEdit();
	afx_msg void OnMKNCLayerUp();
	afx_msg void OnEndScrollLayerList(NMHDR *pNMHDR, LRESULT *pResult);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
