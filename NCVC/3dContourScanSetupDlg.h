// C3dContourScanSetupDlg �_�C�A���O
//

#pragma once

class C3dModelDoc;

class C3dContourScanSetupDlg : public CDialog
{
	C3dModelDoc*	m_pDoc;

public:
	C3dContourScanSetupDlg(C3dModelDoc*, CWnd* pParent = nullptr);   // �W���R���X�g���N�^�[

// �_�C�A���O �f�[�^
	enum { IDD = IDD_3DCONTOUR };
	CFloatEdit	m_dBallEndmill;
	CFloatEdit	m_dSpace;
	CFloatEdit	m_dZmin;
	CFloatEdit	m_dZmax;
	CFloatEdit	m_dShift;
	BOOL		m_bZOrigin;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	virtual BOOL OnInitDialog();
	virtual void OnOK();

	DECLARE_MESSAGE_MAP()
};
