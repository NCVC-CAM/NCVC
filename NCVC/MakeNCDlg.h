// MakeNCD.h : �w�b�_�[ �t�@�C��
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMakeNCDlg �_�C�A���O

class CMakeNCDlg : public CDialog
{
	UINT	m_nTitle;
	// ��è�����۰قɕ\������O�̏ȗ��`������
	CString	m_strNCPath,	// �{�����߽��
			m_strInitPath;

// �R���X�g���N�V����
public:
	CMakeNCDlg(UINT, CDXFDoc*);

// �_�C�A���O �f�[�^
	//{{AFX_DATA(CMakeNCDlg)
	enum { IDD = IDD_MAKENCD };
	CButton	m_ctOK;
	CEdit	m_ctNCFileName;
	CString	m_strNCFileName;
	CComboBox	m_ctInitFileName;
	CString	m_strInitFileName;
	BOOL	m_bNCView;
	//}}AFX_DATA

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B
	//{{AFX_VIRTUAL(CMakeNCDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
protected:

	// �������ꂽ���b�Z�[�W �}�b�v�֐�
	//{{AFX_MSG(CMakeNCDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnMKNCFileUp();
	afx_msg void OnMKNCInitUp();
	afx_msg void OnMKNCInitEdit();
	afx_msg void OnSelChangeInit();
	afx_msg void OnKillFocusNCFile();
	afx_msg void OnKillFocusInit();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// �����޲�۸ދ��ʊ֐�

void	SetFocusListCtrl(CListCtrl&, int);
BOOL	CheckMakeDlgFileExt(DOCTYPE, CString&);
BOOL	CheckMakeNCDlgExLayerState(CString&, CEdit&, CListCtrl&, BOOL);
//
CString	MakeDlgFileRefer(int, const CString&, CDialog*, int, CString&, CString&, BOOL);
void	MakeNCDlgInitFileEdit(CString&, CString&, CDialog*, int, CComboBox&);
int		MakeNCDlgSelChange(const CComboBox&, HWND, int, CString&, CString&);
void	MakeDlgKillFocus(CString&, CString&, CDialog*, int nID);
//
int		GetMakeNCDlgExSortColumn(UINT);
void	SetMakeNCDlgExSortColumn(UINT, int);
CPoint	GetMakeNCDlgExLayerListState(const CListCtrl&);
//
void	CreateNCFile(const CDXFDoc*, CString&, CString&);
void	CreateLayerFile(const CDXFDoc*, CString&, CString&);
BOOL	InitialMakeNCDlgComboBox(const CStringList*, CComboBox&);
BOOL	InitialMakeNCDlgExLayerListCtrl(CDXFDoc*, CListCtrl&);
