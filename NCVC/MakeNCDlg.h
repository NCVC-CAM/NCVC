// MakeNCD.h : �w�b�_�[ �t�@�C��
//

#pragma once
#include "DXFOption.h"
class CDocBase;
class CDXFDoc;
class C3dModelDoc;

/////////////////////////////////////////////////////////////////////////////
// CMakeNCDlg �_�C�A���O

class CMakeNCDlg : public CDialog
{
	UINT		m_nTitle;
	NCMAKETYPE	m_enType;
	CDocBase*	m_pDoc;
	// ��è�����۰قɕ\������O�̏ȗ��`������
	CString		m_strNCPath,	// �{�����߽��
				m_strInitPath;

	void	CommonConstructor(void);

	// �R���X�g���N�V����
public:
	CMakeNCDlg(UINT, NCMAKETYPE, CDXFDoc*);
	CMakeNCDlg(UINT, NCMAKETYPE, C3dModelDoc*);

// �_�C�A���O �f�[�^
	//{{AFX_DATA(CMakeNCDlg)
	enum { IDD = IDD_MAKENCD };
	CButton	m_ctOK;
	CButton m_ctBindOpt;
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
	afx_msg void OnBindOpt();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// �����޲�۸ދ��ʊ֐�

BOOL	CheckMakeDlgFileExt(DOCTYPE, CString&);
//
CString	MakeDlgFileRefer(int, const CString&, CDialog*, int, CString&, CString&, BOOL);
void	MakeNCDlgInitFileEdit(CString&, CString&, CDialog*, int, CComboBox&);
int		MakeNCDlgSelChange(const CComboBox&, HWND, int, CString&, CString&);
void	MakeDlgKillFocus(CString&, CString&, CDialog*, int nID);
//
void	CreateNCFile(const CDXFDoc*, CString&, CString&);
void	CreateNCFile(const C3dModelDoc*, CString&, CString&);
void	CreateLayerFile(const CDXFDoc*, CString&, CString&);
BOOL	InitialMakeNCDlgComboBox(const CStringList*, CComboBox&);
