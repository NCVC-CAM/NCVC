#pragma once

/////////////////////////////////////////////////////////////////////////////
// CShapePropDlg �_�C�A���O

class CShapePropDlg : public CDialog
{
	DXFTREETYPE	m_vSelect;
	BOOL		m_bChain;		// �֊s�W����I���ł��邩�ǂ���
	boost::optional<double>	m_dOffsetInit;	// �̾�Ēl�����͉\��

public:
	CShapePropDlg(DXFTREETYPE&, BOOL, int, boost::optional<double>&, BOOL);
	virtual ~CShapePropDlg();

// �_�C�A���O �f�[�^
	enum { IDD = IDD_SHAPE_PROP };
	int			m_nShape;
	CComboBox	m_ctShape;
	CEdit		m_ctName;
	CString		m_strShapeName;
	double		m_dOffset;	// EndDialog()���m_ctOffset
	CButton		m_ctAcuteRound;
	BOOL		m_bAcuteRound;
protected:
	CFloatEdit	m_ctOffset;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	virtual BOOL OnInitDialog();
	virtual void OnOK();

	DECLARE_MESSAGE_MAP()
};
