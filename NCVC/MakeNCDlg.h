// MakeNCD.h : ヘッダー ファイル
//

#pragma once
#include "DXFOption.h"
class CDocBase;
class CDXFDoc;
class C3dModelDoc;

/////////////////////////////////////////////////////////////////////////////
// CMakeNCDlg ダイアログ

class CMakeNCDlg : public CDialog
{
	UINT		m_nTitle;
	NCMAKETYPE	m_enType;
	CDocBase*	m_pDoc;
	// ｽﾀﾃｨｯｸｺﾝﾄﾛｰﾙに表示する前の省略形文字列
	CString		m_strNCPath,	// 本物のﾊﾟｽ名
				m_strInitPath;

	void	CommonConstructor(void);

	// コンストラクション
public:
	CMakeNCDlg(UINT, NCMAKETYPE, CDXFDoc*);
	CMakeNCDlg(UINT, NCMAKETYPE, C3dModelDoc*);

// ダイアログ データ
	//{{AFX_DATA(CMakeNCDlg)
	enum { IDD = IDD_MAKENCD };
	CButton	m_ctOK;
	CButton m_ctBindOpt;
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
	afx_msg void OnBindOpt();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// 生成ﾀﾞｲｱﾛｸﾞ共通関数

BOOL	CheckMakeDlgFileExt(DOCTYPE, CString&);
//
CString	MakeDlgFileRefer(int, const CString&, CDialog*, int, CString&, CString&, BOOL);
void	MakeNCDlgInitFileEdit(CString&, CString&, CDialog*, int, CComboBox&);
int		MakeNCDlgSelChange(const CComboBox&, HWND, int, CString&, CString&);
void	MakeDlgKillFocus(CString&, CString&, CDialog*, int nID);
//
void	CreateNCFile(const CDXFDoc*, CString&, CString&);
void	CreateNCFile(const C3dModelDoc*, CString&, CString&);
void	CreateLayerFile(const CDXFDoc*, CString&, CString&);
BOOL	InitialMakeNCDlgComboBox(const CStringList*, CComboBox&);
