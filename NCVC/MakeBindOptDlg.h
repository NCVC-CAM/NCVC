// CMakeBindOptDlg.h : �w�b�_�[ �t�@�C��
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMakeBindOptDlg �_�C�A���O

class CMakeBindOptDlg : public CDialog
{
public:
	CMakeBindOptDlg(CWnd* pParent = NULL);   // �W���R���X�g���N�^�[

// �_�C�A���O �f�[�^
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MAKEBINDOPT };
#endif
	int		m_nSort;
	BOOL	m_bFileComment;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	virtual void OnOK();

	DECLARE_MESSAGE_MAP()
};
