// MachineSetup2.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "MachineOption.h"
#include "MachineSetup.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CMachineSetup2, CPropertyPage)
	//{{AFX_MSG_MAP(CMachineSetup2)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMachineSetup2 プロパティ ページ

CMachineSetup2::CMachineSetup2() : CPropertyPage(CMachineSetup2::IDD)
{
	m_psp.dwFlags &= ~PSP_HASHELP;
	//{{AFX_DATA_INIT(CMachineSetup2)
		// メモ - ClassWizard はこの位置にメンバの初期化処理を追加します。
	//}}AFX_DATA_INIT
}

void CMachineSetup2::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMachineSetup2)
		// メモ - ClassWizard はこの位置にマッピング用のマクロを追加または削除します。
	//}}AFX_DATA_MAP
	int		i, j;
	for ( i=0; i<NCXYZ; i++ )
		DDX_Control(pDX, i+IDC_MCST2_X, m_dInitialXYZ[i]);
	for ( i=0; i<WORKOFFSET; i++ ) {
		for ( j=0; j<NCXYZ; j++ ) {
			DDX_Control(pDX, i*NCXYZ+j+IDC_MCST2_G54X, m_dWorkOffset[i][j]);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// CMachineSetup2 メッセージ ハンドラ

BOOL CMachineSetup2::OnInitDialog() 
{
	__super::OnInitDialog();

	int			i, j;
	const CMachineOption*	pMCopt = AfxGetNCVCApp()->GetMachineOption();
	// ｺﾝｽﾄﾗｸﾀではできないｺﾝﾄﾛｰﾙの初期化
	for ( i=0; i<NCXYZ; i++ )
		m_dInitialXYZ[i] = pMCopt->m_dInitialXYZ[i];
	for ( i=0; i<WORKOFFSET; i++ ) {
		for ( j=0; j<NCXYZ; j++ ) {
			m_dWorkOffset[i][j] = pMCopt->m_dWorkOffset[i][j];
		}
	}
	UpdateData(FALSE);

	return TRUE;  // コントロールにフォーカスを設定しないとき、戻り値は TRUE となります
	              // 例外: OCX プロパティ ページの戻り値は FALSE となります
}

BOOL CMachineSetup2::OnApply() 
{
	int		i, j;
	CMachineSetup*	pParent = static_cast<CMachineSetup *>(GetParentSheet());
	CMachineOption*	pMCopt = AfxGetNCVCApp()->GetMachineOption();

	// 再読込ﾁｪｯｸ
	for ( i=0; i<NCXYZ; i++ ) {
		if ( pMCopt->m_dInitialXYZ[i] != m_dInitialXYZ[i] ) {
			pMCopt->m_dInitialXYZ[i] = m_dInitialXYZ[i];
			pParent->m_bReload = TRUE;		// 再読込が必要
		}
	}
	for ( i=0; i<WORKOFFSET; i++ ) {
		for ( j=0; j<NCXYZ; j++ ) {
			if ( pMCopt->m_dWorkOffset[i][j] != m_dWorkOffset[i][j] ) {
				pMCopt->m_dWorkOffset[i][j] = m_dWorkOffset[i][j];
				pParent->m_bReload = TRUE;
			}
		}
	}

	return TRUE;
}
