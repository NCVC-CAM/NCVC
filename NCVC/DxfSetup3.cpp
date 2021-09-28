// DxfSetup3.cpp : 実装ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "DXFOption.h"
#include "DxfSetup.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CDxfSetup3, CPropertyPage)
	ON_BN_CLICKED(IDC_DXF_RELOAD, &CDxfSetup3::OnReload)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDxfSetup3 プロパティ ページ

CDxfSetup3::CDxfSetup3() : CPropertyPage(CDxfSetup3::IDD)
{
	m_psp.dwFlags &= ~PSP_HASHELP;
}

void CDxfSetup3::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_DXF_RELOAD, m_ctReload);
	DDX_Control(pDX, IDC_DXF_SPLINENUM, m_nSplineNum);
	DDX_Text(pDX, IDC_DXF_IGNORE, m_strIgnore);
}

/////////////////////////////////////////////////////////////////////////////
// ﾒﾝﾊﾞ関数

void CDxfSetup3::EnableReloadButton(void)
{
	m_ctReload.EnableWindow(
		AfxGetNCVCApp()->GetDXFOpenDocumentCount() <= 0 ? FALSE : TRUE );
}

/////////////////////////////////////////////////////////////////////////////
// CDxfSetup3 メッセージ ハンドラー

BOOL CDxfSetup3::OnInitDialog() 
{
	__super::OnInitDialog();

	const CDXFOption* pOpt = AfxGetNCVCApp()->GetDXFOption();
	m_strIgnore		= pOpt->GetIgnoreStr();
	m_nSplineNum	= pOpt->m_nSplineNum;
	UpdateData(FALSE);

	// DXFﾄﾞｷｭﾒﾝﾄが開かれているかどうか
	EnableReloadButton();

	return TRUE;
}

BOOL CDxfSetup3::OnSetActive()
{
	EnableReloadButton();
	return __super::OnSetActive();
}

BOOL CDxfSetup3::OnApply()
{
	CDXFOption*	pOpt = AfxGetNCVCApp()->GetDXFOption();
	pOpt->m_nSplineNum	= m_nSplineNum;
	pOpt->SetIgnoreArray(m_strIgnore);
	return TRUE;
}

BOOL CDxfSetup3::OnKillActive()
{
	if ( !__super::OnKillActive() )
		return FALSE;

	if ( m_nSplineNum <= 0 ) {
		AfxMessageBox(IDS_ERR_UNDERZERO, MB_OK|MB_ICONEXCLAMATION);
		m_nSplineNum.SetFocus();
		m_nSplineNum.SetSel(0, -1);
		return FALSE;
	}

	return TRUE;
}

void CDxfSetup3::OnReload() 
{
	if ( !OnKillActive() )	// UpdateData() & ﾃﾞｰﾀﾁｪｯｸ
		return;
	OnApply();
	if ( static_cast<CDxfSetup *>(GetParentSheet())->OnReload(this) ) {
		m_nSplineNum.SetFocus();
		m_nSplineNum.SetSel(0, -1);
		EnableReloadButton();
	}
}
