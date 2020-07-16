// MakeNCDlgEx1.h : ヘッダー ファイル
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMakeNCDlgEx1 ダイアログ

class CMakeNCDlgEx1 : public CDialog
{
	CDXFDoc*	m_pDoc;			// ﾚｲﾔ情報取得
	int			m_nSortLayer;	// ｿｰﾄ中の列番号
	// ｽﾀﾃｨｯｸｺﾝﾄﾛｰﾙに表示する前の省略形文字列
	CString		m_strNCPath,	// 本物のﾊﾟｽ名
				m_strLayerToInitPath;

	void	OnLayerEdit(int);

public:
	// CMakeNCDlgEx11 への情報提供
	CDXFDoc*	GetDocument(void) {
		return m_pDoc;
	}
	CString	GetNCFileName(void) {
		return m_strNCPath + m_strNCFileName;
	}

// コンストラクション
public:
	CMakeNCDlgEx1(CDXFDoc*);
	virtual ~CMakeNCDlgEx1();

// ダイアログ データ
	//{{AFX_DATA(CMakeNCDlgEx1)
	enum { IDD = IDD_MAKENCD_EX1 };
	CButton	m_ctOK;
	CEdit	m_ctNCFileName;
	CString	m_strNCFileName;
	CComboBox	m_ctLayerToInitFileName;
	CString	m_strLayerToInitFileName;
	CListCtrl	m_ctLayerList;
	BOOL	m_bNCView;
	//}}AFX_DATA

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CMakeNCDlgEx1)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:
	// ｿｰﾄ用ｺｰﾙﾊﾞｯｸ関数
	static int CALLBACK CompareFunc(LPARAM, LPARAM, LPARAM);

	// 生成されたメッセージ マップ関数
	//{{AFX_MSG(CMakeNCDlgEx1)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnGetDispInfoLayerList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblClkLayerList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnColumnClickLayerList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnKillFocusNCFile();
	afx_msg void OnKillFocusLayerToInit();
	afx_msg void OnSelChangeLayerToInit();
	afx_msg void OnMKNCFileUp();
	afx_msg void OnMKNCLayerEdit();
	afx_msg void OnMKNCLayerUp();
	afx_msg void OnEndScrollLayerList(NMHDR *pNMHDR, LRESULT *pResult);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
