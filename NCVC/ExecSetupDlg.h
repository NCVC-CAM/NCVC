// ExecSetupDlgDlg.h : ヘッダー ファイル
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CExecSetupDlg ダイアログ

class CExecSetupDlg : public CDialog
{
	CImageList	m_ilExec;	// 登録・削除後のｷｬﾝｾﾙに対応するため
							// ｲﾒｰｼﾞﾘｽﾄだけはﾀﾞｲｱﾛｸﾞ独自に保持する
	void	SetDetailData(const CExecOption*);
	BOOL	CheckData(void);
	void	SwapObject(int, int);

// コンストラクション
public:
	CExecSetupDlg();
	~CExecSetupDlg();

// ダイアログ データ
	//{{AFX_DATA(CExecSetupDlg)
	enum { IDD = IDD_EXEC_SETUP };
	CEdit	m_ctFile;
	CListCtrl	m_ctList;
	CString	m_strFile;
	CString	m_strCommand;
	CString	m_strToolTip;
	BOOL	m_bNCType;
	BOOL	m_bDXFType;
	BOOL	m_bShort;
	//}}AFX_DATA

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CExecSetupDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:

	// 生成されたメッセージ マップ関数
	//{{AFX_MSG(CExecSetupDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnItemChangedExeList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnGetDispInfoExeList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnKeyDownList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnAdd();
	afx_msg void OnMod();
	afx_msg void OnDel();
	afx_msg void OnUp();
	afx_msg void OnDown();
	afx_msg void OnFileUP();
	//}}AFX_MSG
	afx_msg void OnDropFiles(HDROP hDropInfo);

	DECLARE_MESSAGE_MAP()
};
