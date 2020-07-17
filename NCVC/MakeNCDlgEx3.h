// MakeNCDlgEx3.h : �w�b�_�[ �t�@�C��
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMakeNCDlgEx3 �_�C�A���O

class CMakeNCDlgEx3 : public CPropertyPage
{
	int		m_nSortColumn;	// ��Ē��̗�ԍ�(0:�C��)
	void	SwapObject(int, int);
	void	SetFocusListCtrl(int);
	void	SetHeaderMark(int);

public:
	CMakeNCDlgEx3();
	virtual ~CMakeNCDlgEx3();

// �_�C�A���O �f�[�^
	enum { IDD = IDD_MAKENCD_EX3 };
	CListCtrl	m_ctLayerList;

public:
	virtual BOOL OnKillActive();
	virtual BOOL OnSetActive();
	virtual BOOL OnWizardFinish();
protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g

	afx_msg void OnGetDispInfoLayerList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblClkLayerList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnColumnClickLayerList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEndScrollLayerList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnUp();
	afx_msg void OnDown();
	afx_msg LRESULT OnQuerySiblings(WPARAM, LPARAM);

	static UINT	m_nParentID;
	// ��ėp����ޯ��֐�
	static int CALLBACK CompareFunc(LPARAM, LPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()
};
