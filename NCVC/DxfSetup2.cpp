// DxfSetup2.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "DXFOption.h"
#include "DxfSetup.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

BEGIN_MESSAGE_MAP(CDxfSetup2, CPropertyPage)
	//{{AFX_MSG_MAP(CDxfSetup2)
	ON_BN_CLICKED(IDC_DXF_RELOAD, &CDxfSetup2::OnReload)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDxfSetup2 プロパティ ページ

CDxfSetup2::CDxfSetup2() : CPropertyPage(CDxfSetup2::IDD)
{
	m_psp.dwFlags &= ~PSP_HASHELP;
	//{{AFX_DATA_INIT(CDxfSetup2)
	//}}AFX_DATA_INIT
	const CDXFOption* pOpt = AfxGetNCVCApp()->GetDXFOption();
	m_strStartLayer		= pOpt->m_strReadLayer[DXFSTRLAYER];
	m_strMoveLayer		= pOpt->m_strReadLayer[DXFMOVLAYER];
	m_strCommentLayer	= pOpt->m_strReadLayer[DXFCOMLAYER];
}

void CDxfSetup2::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDxfSetup2)
	DDX_Control(pDX, IDC_DXF_RELOAD, m_ctReload);
	DDX_Control(pDX, IDC_DXF_START, m_ctStartLayer);
	DDX_Text(pDX, IDC_DXF_START, m_strStartLayer);
	DDX_Text(pDX, IDC_DXF_MOVE, m_strMoveLayer);
	DDX_Text(pDX, IDC_DXF_COMMENT, m_strCommentLayer);
	//}}AFX_DATA_MAP
}

/////////////////////////////////////////////////////////////////////////////
// ﾒﾝﾊﾞ関数

void CDxfSetup2::EnableReloadButton(void)
{
	if ( AfxGetNCVCApp()->GetDXFOpenDocumentCount() <= 0 )
		m_ctReload.EnableWindow(FALSE);
	else
		m_ctReload.EnableWindow(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// CDxfSetup2 メッセージ ハンドラ

BOOL CDxfSetup2::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	// DXFﾄﾞｷｭﾒﾝﾄが開かれているかどうか
	EnableReloadButton();

	return TRUE;  // コントロールにフォーカスを設定しないとき、戻り値は TRUE となります
	              // 例外: OCX プロパティ ページの戻り値は FALSE となります
}

BOOL CDxfSetup2::OnApply() 
{
	CDXFOption*	pOpt = AfxGetNCVCApp()->GetDXFOption();
	pOpt->m_strReadLayer[DXFSTRLAYER] = m_strStartLayer;
	pOpt->m_strReadLayer[DXFMOVLAYER] = m_strMoveLayer;
	pOpt->m_strReadLayer[DXFCOMLAYER] = m_strCommentLayer;

	return TRUE;
}

void CDxfSetup2::OnReload() 
{
	UpdateData();
	OnApply();
	CDxfSetup*	pParent = static_cast<CDxfSetup *>(GetParent());
	if ( pParent->OnReload(this) ) {
		m_ctStartLayer.SetFocus();
		m_ctStartLayer.SetSel(0, -1);
		EnableReloadButton();
	}
}

BOOL CDxfSetup2::OnSetActive() 
{
	EnableReloadButton();
	return CPropertyPage::OnSetActive();
}
