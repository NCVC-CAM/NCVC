// ThreadDlg.h : ヘッダー ファイル
//

#pragma once

// ｽﾚｯﾄﾞ関数
UINT NCDtoXYZ_Thread(LPVOID);			// TH_NCRead.cpp
UINT UVWire_Thread(LPVOID);				// TH_UVWire.cpp
UINT CorrectCalc_Thread(LPVOID);		// TH_Correct.cpp
UINT MakeNCD_Thread(LPVOID);			// TH_MakeNCD.cpp
UINT MakeLathe_Thread(LPVOID);			// TH_MakeLathe.cpp
UINT MakeWire_Thread(LPVOID);			// TH_MakeWire.cpp
UINT MakeNurbs_Thread(LPVOID);			// TH_MakeNurbs.cpp
UINT ShapeSearch_Thread(LPVOID);		// TH_ShapeSearch.cpp
UINT AutoWorkingSet_Thread(LPVOID);		// TH_AutoWorkingSet.cpp

class CThreadDlg;
// ｽﾚｯﾄﾞへの引数
struct NCVCTHREADPARAM
{
	CThreadDlg*	pParent;
	CDocument*	pDoc;
	WPARAM		wParam;		// 各ｽﾚｯﾄﾞ独自情報
	LPARAM		lParam;
};
typedef	NCVCTHREADPARAM*	LPNCVCTHREADPARAM;

// ﾌｧｲﾙ出力指示
#define		TH_HEADER		0x0001
#define		TH_FOOTER		0x0002
#define		TH_APPEND		0x0004

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
