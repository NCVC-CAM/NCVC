// DxfSetup1.h : �w�b�_�[ �t�@�C��
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CDxfSetup3 �_�C�A���O

class CDxfSetup3 : public CPropertyPage
{
	void	EnableReloadButton(void);

public:
	CDxfSetup3();

// �_�C�A���O �f�[�^
	enum { IDD = IDD_DXF_SETUP3 };
	CButton		m_ctReload;
	CString		m_strIgnore;
	CIntEdit	m_nSplineNum;

	virtual BOOL OnSetActive();
	virtual BOOL OnKillActive();
	virtual BOOL OnApply();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	virtual BOOL OnInitDialog();
	afx_msg void OnReload();

	DECLARE_MESSAGE_MAP()
};
