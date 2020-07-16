// CLayerDlg.h : �w�b�_�[ �t�@�C��
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CLayerDlg �_�C�A���O

class CLayerDlg : public CDialog
{
	HTREEITEM	m_hTree[4];	// �eڲԂ��ذ�����

	void	EnableButton(BOOL bEnable) {
		m_ctLayerTree.EnableWindow(bEnable);
		m_ctOK.EnableWindow(bEnable);
	}
	void	SetChildCheck(HTREEITEM);
	void	SetParentCheck(HTREEITEM);

// �R���X�g���N�V����
public:
	CLayerDlg();

// �_�C�A���O �f�[�^
	//{{AFX_DATA(CLayerDlg)
	enum { IDD = IDD_DXFVIEW_LAYER };
	CTreeCtrl	m_ctLayerTree;
	CButton	m_ctOK;
	//}}AFX_DATA

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B
	//{{AFX_VIRTUAL(CLayerDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
protected:

	// �������ꂽ���b�Z�[�W �}�b�v�֐�
	//{{AFX_MSG(CLayerDlg)
	virtual BOOL OnInitDialog();
	virtual void OnCancel();
	virtual void OnOK();
	afx_msg void OnLayerTreeKeydown(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnLayerTreeClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnLayerTreeGetdispinfo(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	afx_msg LRESULT OnUserSwitchDocument(WPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()
};
