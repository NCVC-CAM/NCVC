// DxfSetupReload.h : �w�b�_�[ �t�@�C��
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CDxfSetupReload �_�C�A���O

class CDxfSetupReload : public CDialog
{
// �R���X�g���N�V����
public:
	CDxfSetupReload(CWnd* pParent);

// �_�C�A���O �f�[�^
	//{{AFX_DATA(CDxfSetupReload)
	enum { IDD = IDD_DXF_SETUP_RELOAD };
		// ����: ClassWizard �͂��̈ʒu�Ƀf�[�^ �����o��ǉ����܂��B
	//}}AFX_DATA
	CCheckListBox	m_ctReloadList;	// �W��ؽ��ޯ����u��(��޸׽��)

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B
	//{{AFX_VIRTUAL(CDxfSetupReload)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
protected:

	// �������ꂽ���b�Z�[�W �}�b�v�֐�
	//{{AFX_MSG(CDxfSetupReload)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
