// MKWISetup2.h : �w�b�_�[ �t�@�C��
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMKWISetup2 �_�C�A���O

class CMKWISetup2 : public CPropertyPage
{
public:
	CMKWISetup2();

// �_�C�A���O �f�[�^
	enum { IDD = IDD_MKWI_SETUP2 };
	CString	m_strAwfCnt;
	CString	m_strAwfCut;
	CFloatEdit m_dAWFcircleLo;
	CFloatEdit m_dAWFcircleHi;
	BOOL	m_bAWFstart;
	BOOL	m_bAWFend;

protected:
	virtual BOOL OnApply();
	virtual BOOL OnKillActive();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
};
