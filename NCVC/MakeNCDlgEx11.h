// MakeNCDlgEx11.h : ヘッダー ファイル
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMakeNCDlgEx11 ダイアログ

class CMakeNCDlgEx11 : public CDialog
{
	CString		m_strCaption;	// 元のｳｨﾝﾄﾞｳﾀｲﾄﾙ
	int			m_nIndex;	// ﾌｫｰｶｽを得たときのﾚｲﾔ配列番号
	// ｽﾀﾃｨｯｸｺﾝﾄﾛｰﾙに表示する前の省略形文字列
	CString		m_strNCPath,	// 本物のﾊﾟｽ名
				m_strInitPath;

	void	GetNowState(void);
	void	SetNowState(int);
	void	EnablePartOut(void);

// コンストラクション
public:
	CMakeNCDlgEx11(CMakeNCDlgEx1*, int, const CString&);
	virtual ~CMakeNCDlgEx11();

// ダイアログ データ
	//{{AFX_DATA(CMakeNCDlgEx11)
	enum { IDD = IDD_MAKENCD_EX1_1 };
	CButton	m_ctOK;
	CStatic	m_ctPartEnable3;
	CStatic	m_ctPartEnable2;
	CButton	m_ctPartEnable1;
	CComboBox	m_ctLayer;
	CButton	m_ctPartEnable4;
	CEdit	m_ctNCFileName;
	CStatic	m_ctNCPath;
	CStatic	m_ctInitPath;
	CComboBox	m_ctInitFileName;
	CString	m_strInitFileName;
	CString	m_strNCFileName;
	BOOL	m_bCheck;
	BOOL	m_bPartOut;
	//}}AFX_DATA
	CString		m_strLayerFile;	// ﾚｲﾔﾌｧｲﾙ
	CLayerArray	m_obLayer;	// ﾚｲﾔ情報一時待避(上位ﾘｽﾄｺﾝﾄﾛｰﾙ順)

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CMakeNCDlgEx11)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:

	// 生成されたメッセージ マップ関数
	//{{AFX_MSG(CMakeNCDlgEx11)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnMKNCInitUp();
	afx_msg void OnMKNCInitEdit();
	afx_msg void OnSelChangeInit();
	afx_msg void OnKillFocusInit();
	afx_msg void OnKillFocusNCFile();
	afx_msg void OnMKNCFileUp();
	afx_msg void OnCopy();
	afx_msg void OnPartOut();
	afx_msg void OnSelChangeLayer();
	afx_msg void OnSetFocusLayer();
	afx_msg void OnNewLayerFile();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
