// C3dRoughScanSetupDlg �_�C�A���O
//

#pragma once

class C3dModelDoc;

class C3dRoughScanSetupDlg : public CDialog
{
	C3dModelDoc*	m_pDoc;

public:
	C3dRoughScanSetupDlg(C3dModelDoc*, CWnd* pParent = nullptr);   // �W���R���X�g���N�^�[

// �_�C�A���O �f�[�^
	enum { IDD = IDD_3DROUGH };
	CFloatEdit	m_dBallEndmill;
	CFloatEdit	m_dHeight;
	CFloatEdit	m_dZCut;
	CFloatEdit	m_dOffset;
	CIntEdit	m_nLineSplit;
	BOOL		m_bZOrigin;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	virtual BOOL OnInitDialog();
	virtual void OnOK();

	DECLARE_MESSAGE_MAP()
};
