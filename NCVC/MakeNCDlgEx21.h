// MakeNCDlgEx21.h : ヘッダー ファイル
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMakeNCDlgEx21 ダイアログ

class CMakeNCDlgEx21 : public CDialog
{
	CString		m_strCaption;	// 元のｳｨﾝﾄﾞｳﾀｲﾄﾙ
	int			m_nIndex;		// ﾌｫｰｶｽを得たときのﾚｲﾔ配列番号
	// ｽﾀﾃｨｯｸｺﾝﾄﾛｰﾙに表示する前の省略形文字列
	CString		m_strNCPath;	// 本物のﾊﾟｽ名

	void	GetNowState(void);
	void	SetNowState(int);
	void	EnablePartOut(void);

// コンストラクション
public:
	CMakeNCDlgEx21(CMakeNCDlgEx*, int);
	virtual ~CMakeNCDlgEx21();

// ダイアログ データ
	//{{AFX_DATA(CMakeNCDlgEx21)
	enum { IDD = IDD_MAKENCD_EX2_1 };
	CStatic	m_ctNCPath,
			m_ctPartEnable2,
			m_ctPartEnable3;
	CButton	m_ctOK,
			m_ctPartEnable1,
			m_ctPartEnable4;
	CComboBox	m_ctLayer;
	CEdit	m_ctNCFileName;
	CFloatEdit	m_dZStep,
				m_dZCut;
	BOOL	m_bDrill,
			m_bCheck,
			m_bPartOut;
	CString	m_strNCFileName,
			m_strLayerComment,
			m_strLayerCode;
	//}}AFX_DATA
	CLayerArray	m_obLayer;		// ﾚｲﾔ情報一時待避(上位ﾘｽﾄｺﾝﾄﾛｰﾙ順)

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CMakeNCDlgEx21)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:

	// 生成されたメッセージ マップ関数
	//{{AFX_MSG(CMakeNCDlgEx21)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnNewLayer();
	afx_msg void OnStep();
	afx_msg void OnKillFocusNCFile();
	afx_msg void OnMKNCFileUp();
	afx_msg void OnCopy();
	afx_msg void OnPartOut();
	afx_msg void OnSelChangeLayer();
	afx_msg void OnSetFocusLayer();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
