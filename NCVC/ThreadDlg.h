// ThreadDlg.h : ヘッダー ファイル
//

#pragma once

// ｽﾚｯﾄﾞ関数
UINT NCDtoXYZ_Thread(LPVOID);
UINT CorrectCalc_Thread(LPVOID);
UINT MakeNCD_Thread(LPVOID);
UINT ShapeSearch_Thread(LPVOID);
UINT AutoWorkingSet_Thread(LPVOID);

class CThreadDlg;
// ｽﾚｯﾄﾞへの引数
typedef struct tagNCVCTHREADPARAM {
	CThreadDlg*	pParent;
	CDocument*	pDoc;
	WPARAM		wParam;		// 各ｽﾚｯﾄﾞ独自情報
	LPARAM		lParam;
} NCVCTHREADPARAM, *LPNCVCTHREADPARAM;

/////////////////////////////////////////////////////////////////////////////
// CThreadDlg ダイアログ

class CThreadDlg : public CDialog
{
	int		m_nID;
	NCVCTHREADPARAM		m_paramThread;		// ｽﾚｯﾄﾞへの引数

	CWinThread*	m_pThread;
	BOOL		m_bThread;		// ｽﾚｯﾄﾞ継続ﾌﾗｸﾞ

// コンストラクション
public:
	CThreadDlg(int, CDocument*, WPARAM = NULL, LPARAM = NULL);

// オペレーション
	BOOL	IsThreadContinue(void) {
		return m_bThread;
	}
	void	SetFaseMessage(LPCTSTR lpszMsg1, LPCTSTR lpszMsg2 = NULL) {
		// CString型でUpdataData(FALSE)を呼び出すと，
		// Thread関数からのｱｸｾｽでｱｻｰﾄｴﾗｰになる
		if ( lpszMsg1 )
			m_ctMsgText1.SetWindowText(lpszMsg1);
		if ( lpszMsg2 )
			m_ctMsgText2.SetWindowText(lpszMsg2);
		m_ctReadProgress.SetPos(0);
	}

// ダイアログ データ
	//{{AFX_DATA(CThreadDlg)
	enum { IDD = IDD_THREADDLG };
	CStatic	m_ctMsgText2;
	CStatic	m_ctMsgText1;
	CProgressCtrl	m_ctReadProgress;
	//}}AFX_DATA

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CThreadDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:

	// 生成されたメッセージ マップ関数
	//{{AFX_MSG(CThreadDlg)
	virtual BOOL OnInitDialog();
	virtual void OnCancel();
	afx_msg LRESULT OnNcHitTest(CPoint point);
	//}}AFX_MSG
	afx_msg LRESULT OnUserFinish(WPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()
};
