// MakeNCDlgEx2.h : ヘッダー ファイル
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMakeNCDlgEx2 ダイアログ

class CMakeNCDlgEx2 : public CPropertyPage
{
	// ｽﾀﾃｨｯｸｺﾝﾄﾛｰﾙに表示する前の省略形文字列
	CString		m_strNCPath,	// 本物のﾊﾟｽ名
				m_strInitPath,
				m_strLayerToInitPath;

	BOOL	m_bNewLayer;
	void	CheckNewLayer(void);

public:
	CString	GetNCFileName(void) {
		return m_strNCPath + m_strNCFileName;
	}
	CString	GetInitFileName(void) {
		return m_strInitPath + m_strInitFileName;
	}

// コンストラクション
public:
	CMakeNCDlgEx2();
	virtual ~CMakeNCDlgEx2();

// ダイアログ データ
	//{{AFX_DATA(CMakeNCDlgEx2)
	enum { IDD = IDD_MAKENCD_EX2 };
	CStatic m_ctInit_st[4];
	CButton m_ctInit_bt[2];
	CEdit	m_ctNCFileName;
	CComboBox	m_ctInitFileName,
				m_ctLayerToInitFileName;
	BOOL	m_bNCView;
	CString	m_strNCFileName,
			m_strInitFileName,
			m_strLayerToInitFileName;
	//}}AFX_DATA

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CMakeNCDlgEx2)
	public:
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardNext();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:
	// 生成されたメッセージ マップ関数
	//{{AFX_MSG(CMakeNCDlgEx2)
	virtual BOOL OnInitDialog();
	afx_msg void OnMKNCFileUp();
	afx_msg void OnMKNCInitUp();
	afx_msg void OnMKNCInitEdit();
	afx_msg void OnKillFocusNCFile();
	afx_msg void OnKillFocusInit();
	afx_msg void OnKillFocusLayerToInit();
	afx_msg void OnSelChangeLayerToInit();
	afx_msg void OnSelChangeInit();
	afx_msg void OnMKNCLayerEdit();
	afx_msg void OnMKNCLayerUp();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
