// DxfAutoOutlineDlg.h : �w�b�_�[ �t�@�C��
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CDxfAutoOutlineDlg �_�C�A���O

class CDxfAutoOutlineDlg : public CDialogEx
{
	LPAUTOWORKINGDATA	m_pAuto;

public:
	CDxfAutoOutlineDlg(LPAUTOWORKINGDATA);

// �_�C�A���O �f�[�^
	enum { IDD = IDD_DXFEDIT_AUTOOUTLINE };
	CFloatEdit	m_dOffset;
	CIntEdit	m_nLoop;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	virtual BOOL OnInitDialog();
	virtual void OnOK();

	DECLARE_MESSAGE_MAP()
};
