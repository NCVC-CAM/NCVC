// MCSetup5.h : �w�b�_�[ �t�@�C��
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMCSetup4 �_�C�A���O

class CMCSetup5 : public CPropertyPage
{
public:
	CMCSetup5();
	virtual ~CMCSetup5();

// �_�C�A���O �f�[�^
	enum { IDD = IDD_MC_SETUP5 };
	BOOL	m_bLathe,
			m_bL0Cycle;
	CString	m_strAutoBreak;

public:
	virtual BOOL OnApply();
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g

	DECLARE_MESSAGE_MAP()
};
