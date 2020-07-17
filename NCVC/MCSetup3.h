// MCSetup3.h : �w�b�_�[ �t�@�C��
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMCSetup3 �_�C�A���O

class CMCSetup3 : public CPropertyPage
{
	int		m_nSortColumn;	// ��Ē��̗�ԍ�(0:�C��)
	BOOL	m_bChange;

	void	SetDetailData(const CMCTOOLINFO*);
	BOOL	CheckData(void);
	void	SetHeaderMark(int);

// �R���X�g���N�V����
public:
	CMCSetup3();

// �_�C�A���O �f�[�^
	//{{AFX_DATA(CMCSetup3)
	enum { IDD = IDD_MC_SETUP3 };
	CFloatEdit	m_dToolH;
	CFloatEdit	m_dToolD;
	CIntEdit	m_nToolNo;
	CListCtrl	m_ctToolList;
	CString	m_strToolName;
	int		m_nType;
	//}}AFX_DATA

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B

	//{{AFX_VIRTUAL(CMCSetup3)
	public:
	virtual BOOL OnApply();
	virtual void OnCancel();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
protected:
	// �������ꂽ���b�Z�[�W �}�b�v�֐�
	//{{AFX_MSG(CMCSetup3)
	virtual BOOL OnInitDialog();
	afx_msg void OnItemChangedToolList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnGetDispInfoToolList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnColumnClickToolList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnKeyDownToolList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnAdd();
	afx_msg void OnMod();
	afx_msg void OnDel();
	//}}AFX_MSG

	// ��ėp����ޯ��֐�
	static	int	CALLBACK	CompareToolListFunc(LPARAM, LPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()
};
