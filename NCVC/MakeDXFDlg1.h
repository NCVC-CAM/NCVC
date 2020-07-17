// MakeDXFDlg1.h : ヘッダー ファイル
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMakeDXFDlg1 ダイアログ

class CMakeDXFDlg1 : public CPropertyPage
{
	// ｽﾀﾃｨｯｸｺﾝﾄﾛｰﾙに表示する前の省略形文字列
	CString			m_strDXFPath;	// 本物のﾊﾟｽ名

// コンストラクション
public:
	CMakeDXFDlg1();

// ダイアログ データ
	//{{AFX_DATA(CMakeDXFDlg1)
	enum { IDD = IDD_MAKEDXF1 };
	CEdit	m_ctDXFFileName;
	CString	m_strDXFFileName;
	int		m_nPlane;
	//}}AFX_DATA
	BOOL			m_bOut[4];
	CString			m_strLayer[4];
	CComboBox		m_cbLineType[4];
	CColComboBox	m_ctColor[4];

	CString	GetDXFFileName(void) {
		return m_strDXFPath + m_strDXFFileName;
	}

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。

	//{{AFX_VIRTUAL(CMakeDXFDlg1)
	public:
	virtual BOOL OnApply();
	virtual BOOL OnKillActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:
	// 生成されたメッセージ マップ関数
	//{{AFX_MSG(CMakeDXFDlg1)
	virtual BOOL OnInitDialog();
	afx_msg void OnMKDXFileUp();
	afx_msg void OnKillFocusDXFFile();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
