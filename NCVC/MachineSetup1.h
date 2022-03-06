// MachineSetup1.h : ヘッダー ファイル
//

#pragma once

#include "NCVCdefine.h"

/////////////////////////////////////////////////////////////////////////////
// CMachineSetup1 ダイアログ

class CMachineSetup1 : public CPropertyPage
{
// コンストラクション
public:
	CMachineSetup1();

// ダイアログ データ
	//{{AFX_DATA(CMachineSetup1)
	enum { IDD = IDD_MC_SETUP1 };
	CFloatEdit	m_dFspeed;
	CFloatEdit	m_dBlock;
	CString	m_strName;
	int		m_nFselect;
	//}}AFX_DATA
	int			m_nModalGroup[MODALGROUP];
	CIntEdit	m_nMoveSpeed[NCXYZ];

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。

	//{{AFX_VIRTUAL(CMachineSetup1)
	public:
	virtual BOOL OnApply();
	virtual BOOL OnKillActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL

// インプリメンテーション
protected:
	// 生成されたメッセージ マップ関数
	//{{AFX_MSG(CMachineSetup1)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
