// MachineSetup5.h : �w�b�_�[ �t�@�C��
//

#pragma once
#include "afxwin.h"

/////////////////////////////////////////////////////////////////////////////
// CMachineSetup5 �_�C�A���O

class CMachineSetup5 : public CPropertyPage
{
	void	EnableControl_ViewMode(void);

public:
	CMachineSetup5();

// �_�C�A���O �f�[�^
	enum { IDD = IDD_MC_SETUP5 };
	int			m_nForceViewMode;
	CFloatEdit	m_dDefWireDepth;
	BOOL		m_bOBS[10],
				m_bL0Cycle;
	CString		m_strAutoBreak;
	CStatic		m_ctDepthLabel1,
				m_ctDepthLabel2;
	CButton		m_ctL0Cycle;

public:
	virtual BOOL OnApply();
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	virtual BOOL OnInitDialog();

	afx_msg void OnSelchangeViewMode();
	afx_msg void OnAllON();
	afx_msg void OnAllOFF();

	DECLARE_MESSAGE_MAP()
};
