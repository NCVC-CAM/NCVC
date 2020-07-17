// MakeNCDlgEx11.h : ヘッダー ファイル
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMakeNCDlgEx11 ダイアログ

class CMakeNCDlgEx11 : public CDialog
{
	CString		m_strCaption;	// 元のｳｨﾝﾄﾞｳﾀｲﾄﾙ
	int			m_nIndex;		// ﾌｫｰｶｽを得たときのﾚｲﾔ配列番号
	// ｽﾀﾃｨｯｸｺﾝﾄﾛｰﾙに表示する前の省略形文字列
	CString		m_strNCPath,	// 本物のﾊﾟｽ名
				m_strInitPath;

	void	GetNowState(void);
	void	SetNowState(int);
	void	EnablePartOut(void);

// コンストラクション
public:
	CMakeNCDlgEx11(CMakeNCDlgEx*, int);
	virtual ~CMakeNCDlgEx11();

// ダイアログ データ
	//{{AFX_DATA(CMakeNCDlgEx11)
	enum { IDD = IDD_MAKENCD_EX1_1 };
	CStatic	m_ctNCPath,
			m_ctInitPath,
			m_ctPartEnable2,
			m_ctPartEnable3;
	CButton	m_ctOK,
			m_ctPartEnable1,
			m_ctPartEnable4;
	CComboBox	m_ctLayer,
				m_ctInitFileName;
	CEdit	m_ctNCFileName;
	BOOL	m_bCheck,
			m_bPartOut;
	CString	m_strInitFileName,
			m_strNCFileName,
			m_strLayerComment,
			m_strLayerCode;
	//}}AFX_DATA
	CLayerArray	m_obLayer;		// ﾚｲﾔ情報一時待避(上位ﾘｽﾄｺﾝﾄﾛｰﾙ順)

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
	afx_msg void OnNewLayer();
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
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
