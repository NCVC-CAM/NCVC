// MakeNCDlgEx2.h : ヘッダー ファイル
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMakeNCDlgEx2 ダイアログ

class CMakeNCDlgEx2 : public CDialog
{
	CDXFDoc*	m_pDoc;			// ﾚｲﾔ情報取得
	int			m_nSortLayer;	// ｿｰﾄ中の列番号
	// ｽﾀﾃｨｯｸｺﾝﾄﾛｰﾙに表示する前の省略形文字列
	CString		m_strNCPath,	// 本物のﾊﾟｽ名
				m_strInitPath,
				m_strLayerToInitPath;

	void	OnLayerEdit(int);

public:
	// CMakeNCDlgEx21 への情報提供
	CDXFDoc*	GetDocument(void) {
		return m_pDoc;
	}
	CString	GetNCFileName(void) {
		return m_strNCPath + m_strNCFileName;
	}

// コンストラクション
public:
	CMakeNCDlgEx2(CDXFDoc*);
	virtual ~CMakeNCDlgEx2();

// ダイアログ データ
	//{{AFX_DATA(CMakeNCDlgEx2)
	enum { IDD = IDD_MAKENCD_EX2 };
	CButton	m_ctOK;
	CEdit	m_ctNCFileName;
	CString	m_strNCFileName;
	CComboBox	m_ctInitFileName;
	CString	m_strInitFileName;
	CComboBox	m_ctLayerToInitFileName;
	CString	m_strLayerToInitFileName;
	CListCtrl	m_ctLayerList;
	BOOL	m_bNCView;
	//}}AFX_DATA

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CMakeNCDlgEx2)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:
	// ｿｰﾄ用ｺｰﾙﾊﾞｯｸ関数
	static int CALLBACK CompareFunc(LPARAM, LPARAM, LPARAM);

	// 生成されたメッセージ マップ関数
	//{{AFX_MSG(CMakeNCDlgEx2)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnMKNCFileUp();
	afx_msg void OnMKNCInitUp();
	afx_msg void OnMKNCInitEdit();
	afx_msg void OnKillFocusNCFile();
	afx_msg void OnKillFocusInit();
	afx_msg void OnKillFocusLayerToInit();
	afx_msg void OnGetDispInfoLayerList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblClkLayerList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnColumnClickLayerList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSelChangeLayerToInit();
	afx_msg void OnSelChangeInit();
	afx_msg void OnMKNCLayerEdit();
	afx_msg void OnMKNCLayerUp();
	afx_msg void OnEndScrollLayerList(NMHDR *pNMHDR, LRESULT *pResult);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
