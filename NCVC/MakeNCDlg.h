// MakeNCD.h : ヘッダー ファイル
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMakeNCDlg ダイアログ

class CMakeNCDlg : public CDialog
{
	UINT	m_nTitle;
	// ｽﾀﾃｨｯｸｺﾝﾄﾛｰﾙに表示する前の省略形文字列
	CString	m_strNCPath,	// 本物のﾊﾟｽ名
			m_strInitPath;

// コンストラクション
public:
	CMakeNCDlg(UINT, CDXFDoc*);

// ダイアログ データ
	//{{AFX_DATA(CMakeNCDlg)
	enum { IDD = IDD_MAKENCD };
	CButton	m_ctOK;
	CEdit	m_ctNCFileName;
	CString	m_strNCFileName;
	CComboBox	m_ctInitFileName;
	CString	m_strInitFileName;
	BOOL	m_bNCView;
	//}}AFX_DATA

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CMakeNCDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:

	// 生成されたメッセージ マップ関数
	//{{AFX_MSG(CMakeNCDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnMKNCFileUp();
	afx_msg void OnMKNCInitUp();
	afx_msg void OnMKNCInitEdit();
	afx_msg void OnSelChangeInit();
	afx_msg void OnKillFocusNCFile();
	afx_msg void OnKillFocusInit();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// 生成ﾀﾞｲｱﾛｸﾞ共通関数

void	SetFocusListCtrl(CListCtrl&, int);
BOOL	CheckMakeDlgFileExt(DOCTYPE, CString&);
BOOL	CheckMakeNCDlgExLayerState(CString&, CEdit&, CListCtrl&, BOOL);
//
CString	MakeDlgFileRefer(int, const CString&, CDialog*, int, CString&, CString&, BOOL);
void	MakeNCDlgInitFileEdit(CString&, CString&, CDialog*, int, CComboBox&);
int		MakeNCDlgSelChange(const CComboBox&, HWND, int, CString&, CString&);
void	MakeDlgKillFocus(CString&, CString&, CDialog*, int nID);
//
int		GetMakeNCDlgExSortColumn(UINT);
void	SetMakeNCDlgExSortColumn(UINT, int);
CPoint	GetMakeNCDlgExLayerListState(const CListCtrl&);
//
void	CreateNCFile(const CDXFDoc*, CString&, CString&);
void	CreateLayerFile(const CDXFDoc*, CString&, CString&);
BOOL	InitialMakeNCDlgComboBox(const CStringList*, CComboBox&);
BOOL	InitialMakeNCDlgExLayerListCtrl(CDXFDoc*, CListCtrl&);
