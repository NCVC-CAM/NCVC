#pragma once

/////////////////////////////////////////////////////////////////////////////
// CShapePropDlg ダイアログ

class CShapePropDlg : public CDialog
{
	DXFTREETYPE	m_vSelect;
	BOOL		m_bChain;		// 輪郭集合を選択できるかどうか
	boost::optional<double>	m_dOffsetInit;	// ｵﾌｾｯﾄ値が入力可能か

public:
	CShapePropDlg(DXFTREETYPE&, BOOL, int, boost::optional<double>&, BOOL);
	virtual ~CShapePropDlg();

// ダイアログ データ
	enum { IDD = IDD_SHAPE_PROP };
	int			m_nShape;
	CComboBox	m_ctShape;
	CEdit		m_ctName;
	CString		m_strShapeName;
	double		m_dOffset;	// EndDialog()後のm_ctOffset
	CButton		m_ctAcuteRound;
	BOOL		m_bAcuteRound;
protected:
	CFloatEdit	m_ctOffset;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();
	virtual void OnOK();

	DECLARE_MESSAGE_MAP()
};
