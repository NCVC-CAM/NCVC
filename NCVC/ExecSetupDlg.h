// ExecSetupDlgDlg.h : �w�b�_�[ �t�@�C��
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CExecSetupDlg �_�C�A���O

class CExecSetupDlg : public CDialog
{
	CImageList	m_ilExec;	// �o�^�E�폜��̷�ݾقɑΉ����邽��
							// �Ұ��ؽĂ������޲�۸ޓƎ��ɕێ�����
	void	SetDetailData(const CExecOption*);
	BOOL	CheckData(void);
	void	SwapObject(int, int);

// �R���X�g���N�V����
public:
	CExecSetupDlg();
	~CExecSetupDlg();

// �_�C�A���O �f�[�^
	//{{AFX_DATA(CExecSetupDlg)
	enum { IDD = IDD_EXEC_SETUP };
	CEdit	m_ctFile;
	CListCtrl	m_ctList;
	CString	m_strFile;
	CString	m_strCommand;
	CString	m_strToolTip;
	BOOL	m_bNCType;
	BOOL	m_bDXFType;
	BOOL	m_bShort;
	//}}AFX_DATA

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B
	//{{AFX_VIRTUAL(CExecSetupDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
protected:

	// �������ꂽ���b�Z�[�W �}�b�v�֐�
	//{{AFX_MSG(CExecSetupDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnItemChangedExeList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnGetDispInfoExeList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnKeyDownList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnAdd();
	afx_msg void OnMod();
	afx_msg void OnDel();
	afx_msg void OnUp();
	afx_msg void OnDown();
	afx_msg void OnFileUP();
	//}}AFX_MSG
	afx_msg void OnDropFiles(HDROP hDropInfo);

	DECLARE_MESSAGE_MAP()
};
