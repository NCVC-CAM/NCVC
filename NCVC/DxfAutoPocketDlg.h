// DxfAutoPocketDlg.h : �w�b�_�[ �t�@�C��
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CDxfAutoPocketgDlg �_�C�A���O

class CDxfAutoPocketgDlg : public CDialogEx
{
	LPAUTOWORKINGDATA	m_pAuto;

// �R���X�g���N�V����
public:
	CDxfAutoPocketgDlg(LPAUTOWORKINGDATA);

// �_�C�A���O �f�[�^
	//{{AFX_DATA(CDxfAutoPocketgDlg)
	enum { IDD = IDD_DXFEDIT_AUTOPOCKET };
	CFloatEdit	m_dOffset;
	CIntEdit	m_nLoop;
	BOOL	m_bAcuteRound;
	int		m_nScan;
	BOOL	m_bCircle;
	//}}AFX_DATA

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B
	//{{AFX_VIRTUAL(CDxfAutoPocketgDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
protected:
	// �������ꂽ���b�Z�[�W �}�b�v�֐�
	//{{AFX_MSG(CDxfAutoPocketgDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
