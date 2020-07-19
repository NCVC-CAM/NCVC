// ViewSetup5.cpp : 実装ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "ViewOption.h"
#include "ViewSetup.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

BEGIN_MESSAGE_MAP(CViewSetup5, CPropertyPage)
	ON_BN_CLICKED(IDC_VIEWSETUP1_DEFCOLOR, &CViewSetup5::OnDefColor)
	ON_BN_CLICKED(IDC_VIEWSETUP5_BT_WORK, &CViewSetup5::OnColorButton)
	ON_BN_CLICKED(IDC_VIEWSETUP5_BT_CUT, &CViewSetup5::OnColorButton)
	ON_BN_CLICKED(IDC_VIEWSETUP5_BT_MILL, &CViewSetup5::OnColorButton)
	ON_BN_CLICKED(IDC_VIEWSETUP5_SOLIDVIEW, &CViewSetup5::OnSolidClick)
	ON_BN_CLICKED(IDC_VIEWSETUP5_FBO, &CViewSetup5::OnChange)
	ON_BN_CLICKED(IDC_VIEWSETUP5_G00VIEW, &CViewSetup5::OnChange)
	ON_BN_CLICKED(IDC_VIEWSETUP5_LATHESLIT, &CViewSetup5::OnChange)
	ON_BN_CLICKED(IDC_VIEWSETUP5_DRAGRENDER, &CViewSetup5::OnChange)
	ON_BN_CLICKED(IDC_VIEWSETUP5_TEXTURE, &CViewSetup5::OnTextureClick)
	ON_BN_CLICKED(IDC_VIEWSETUP5_TEXTUREFIND, &CViewSetup5::OnTextureFind)
	ON_CBN_SELCHANGE(IDC_VIEWSETUP5_MILL_TYPE, &CViewSetup5::OnChange)
	ON_EN_CHANGE(IDC_VIEWSETUP5_DEFAULTENDMILL, &CViewSetup5::OnChange)
	ON_EN_CHANGE(IDC_VIEWSETUP5_TEXTUREFILE, &CViewSetup5::OnChange)
	ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CViewSetup5 プロパティ ページ

CViewSetup5::CViewSetup5() : CPropertyPage(CViewSetup5::IDD)
{
	m_psp.dwFlags &= ~PSP_HASHELP;

	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	m_bSolid			= pOpt->m_bSolidView;
	m_bUseFBO			= pOpt->m_bUseFBO;
	m_bWirePath			= pOpt->m_bWirePath;
	m_bDrag				= pOpt->m_bDragRender;
	m_bTexture			= pOpt->m_bTexture;
	m_bLatheSlit		= pOpt->m_bLatheSlit;
	m_bNoActiveTraceGL	= pOpt->m_bNoActiveTraceGL;
	m_bToolTrace		= pOpt->m_bToolTrace;
	m_strTexture		= pOpt->m_strTexture;
	m_nMillType			= pOpt->m_nMillType;
	for ( int i=0; i<SIZEOF(m_colView); i++ ) {
		m_colView[i] = pOpt->m_colNCView[i+NCCOL_GL_WRK];
		m_brColor[i].CreateSolidBrush( m_colView[i] );
	}
}

CViewSetup5::~CViewSetup5()
{
	for ( int i=0; i<SIZEOF(m_brColor); i++ )
		m_brColor[i].DeleteObject();
}

void CViewSetup5::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_VIEWSETUP5_DEFAULTENDMILL, m_dEndmill);
	DDX_Control(pDX, IDC_VIEWSETUP5_FBO, m_ctUseFBO);
	DDX_Control(pDX, IDC_VIEWSETUP5_G00VIEW, m_ctWirePath);
	DDX_Control(pDX, IDC_VIEWSETUP5_DRAGRENDER, m_ctDrag);
	DDX_Control(pDX, IDC_VIEWSETUP5_TEXTURE, m_ctTexture);
	DDX_Control(pDX, IDC_VIEWSETUP5_LATHESLIT, m_ctLatheSlit);
	DDX_Control(pDX, IDC_VIEWSETUP5_TEXTUREFILE, m_ctTextureFile);
	DDX_Control(pDX, IDC_VIEWSETUP5_TEXTUREFIND, m_ctTextureFind);
	DDX_Check(pDX, IDC_VIEWSETUP5_SOLIDVIEW, m_bSolid);
	DDX_Check(pDX, IDC_VIEWSETUP5_FBO, m_bUseFBO);
	DDX_Check(pDX, IDC_VIEWSETUP5_G00VIEW, m_bWirePath);
	DDX_Check(pDX, IDC_VIEWSETUP5_DRAGRENDER, m_bDrag);
	DDX_Check(pDX, IDC_VIEWSETUP5_TEXTURE, m_bTexture);
	DDX_Check(pDX, IDC_VIEWSETUP5_LATHESLIT, m_bLatheSlit);
	DDX_Check(pDX, IDC_VIEWSETUP5_NOACTIVETRACEGL, m_bNoActiveTraceGL);
	DDX_Check(pDX, IDC_VIEWSETUP5_TOOLTRACE, m_bToolTrace);
	DDX_Text(pDX, IDC_VIEWSETUP5_TEXTUREFILE, m_strTexture);
	DDX_CBIndex(pDX, IDC_VIEWSETUP5_MILL_TYPE, m_nMillType);
	for ( int i=0; i<SIZEOF(m_ctColor); i++ )
		DDX_Control(pDX, i + IDC_VIEWSETUP5_ST_WORK, m_ctColor[i]);
}

void CViewSetup5::EnableSolidControl(void)
{
	m_ctUseFBO.EnableWindow(m_bSolid);
	m_ctWirePath.EnableWindow(m_bSolid);
	m_ctDrag.EnableWindow(m_bSolid);
	m_ctLatheSlit.EnableWindow(m_bSolid);
	m_ctTexture.EnableWindow(m_bSolid);
	m_ctTextureFile.EnableWindow(m_bSolid);
	m_ctTextureFind.EnableWindow(m_bSolid);
}

void CViewSetup5::EnableTextureControl(void)
{
	m_ctTextureFile.EnableWindow(m_bTexture);
	m_ctTextureFind.EnableWindow(m_bTexture);
}

/////////////////////////////////////////////////////////////////////////////
// CViewSetup5 メッセージ ハンドラ

BOOL CViewSetup5::OnInitDialog()
{
	__super::OnInitDialog();

	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	m_dEndmill = pOpt->m_dDefaultEndmill * 2.0f;
	EnableSolidControl();
	EnableTextureControl();

	return TRUE;  // return TRUE unless you set the focus to a control
	// 例外 : OCX プロパティ ページは必ず FALSE を返します。
}

BOOL CViewSetup5::OnApply()
{
	CViewOption*	pOpt = AfxGetNCVCApp()->GetViewOption();

	// 更新情報を pOpt->m_dwUpdateFlg にｾｯﾄ
	if ( pOpt->m_bSolidView != m_bSolid ||
			pOpt->m_bUseFBO != m_bUseFBO ||
			pOpt->m_nMillType != m_nMillType ||
			pOpt->m_dDefaultEndmill != (m_dEndmill/2.0f) )
		pOpt->m_dwUpdateFlg |= VIEWUPDATE_BOXEL;
	if ( pOpt->m_bWirePath != m_bWirePath )
		pOpt->m_dwUpdateFlg |= VIEWUPDATE_REDRAW;
	if ( pOpt->m_bTexture != m_bTexture ||
			pOpt->m_strTexture != m_strTexture )
		pOpt->m_dwUpdateFlg |= VIEWUPDATE_TEXTURE;
	pOpt->m_bSolidView		= m_bSolid;
	pOpt->m_bUseFBO			= m_bUseFBO;
	pOpt->m_bWirePath		= m_bWirePath;
	pOpt->m_bDragRender		= m_bDrag;
	pOpt->m_bTexture		= m_bTexture;
	pOpt->m_bLatheSlit		= m_bLatheSlit;
	pOpt->m_bNoActiveTraceGL= m_bNoActiveTraceGL;
	pOpt->m_bToolTrace		= m_bToolTrace;
	pOpt->m_strTexture		= m_strTexture;
	pOpt->m_nMillType		= m_nMillType;
	pOpt->m_dDefaultEndmill = m_dEndmill / 2.0f;

	for ( int i=0; i<SIZEOF(m_colView); i++ ) {
		if ( pOpt->m_colNCView[i+NCCOL_GL_WRK] != m_colView[i] )
			pOpt->m_dwUpdateFlg |= VIEWUPDATE_LIGHT;
		pOpt->m_colNCView[i+NCCOL_GL_WRK] = m_colView[i];
	}

	SetModified(FALSE);

	return TRUE;
}

BOOL CViewSetup5::OnKillActive()
{
	if ( !__super::OnKillActive() )
		return FALSE;

	if ( m_dEndmill < 0 ) {
		AfxMessageBox(IDS_ERR_UNDERZERO, MB_OK|MB_ICONEXCLAMATION);
		m_dEndmill.SetFocus();
		m_dEndmill.SetSel(0, -1);
		return FALSE;
	}

	return TRUE;
}

HBRUSH CViewSetup5::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	if ( nCtlColor == CTLCOLOR_STATIC ) {
		int	nID = pWnd->GetDlgCtrlID();
		if ( nID>=IDC_VIEWSETUP5_ST_WORK && nID<=IDC_VIEWSETUP5_ST_MILL )
			return m_brColor[nID-IDC_VIEWSETUP5_ST_WORK];
	}
	return __super::OnCtlColor(pDC, pWnd, nCtlColor);
}

void CViewSetup5::OnColorButton() 
{
	int	nIndex = GetFocus()->GetDlgCtrlID() - IDC_VIEWSETUP5_BT_WORK;
	if ( 0<=nIndex && nIndex<SIZEOF(m_colView) ) {
		CColorDialog	dlg(m_colView[nIndex]);
		dlg.m_cc.lpCustColors = (COLORREF *)&(AfxGetNCVCApp()->GetViewOption()->m_colCustom);
		if ( dlg.DoModal() == IDOK ) {
			m_colView[nIndex] = dlg.GetColor();
			m_brColor[nIndex].DeleteObject();
			m_brColor[nIndex].CreateSolidBrush( m_colView[nIndex] );
			m_ctColor[nIndex].Invalidate();
			SetModified();
		}
	}
}

void CViewSetup5::OnSolidClick()
{
	UpdateData();
	EnableSolidControl();
	SetModified();
}

void CViewSetup5::OnTextureClick()
{
	UpdateData();
	EnableTextureControl();
	SetModified();
}

void CViewSetup5::OnTextureFind()
{
	extern	LPTSTR	g_pszExecDir;
	extern	LPCTSTR	g_szViewOptFlag[];	// from ViewOption.cpp

	UpdateData();
	CString		strPath, strFile;
	::Path_Name_From_FullPath(m_strTexture, strPath, strFile);
	if ( strFile.IsEmpty() ) {
		// ﾌｧｲﾙ指定がなければ、ｲﾝｽﾄｰﾙﾌｫﾙﾀﾞを参照
		strPath  = g_pszExecDir;
		strPath += g_szViewOptFlag[NCVIEWFLG_TEXTURE];
	}
	else
		strFile = m_strTexture;
	if ( ::NCVC_FileDlgCommon(IDS_VIEW_TEXTURE, IDS_TEXTURE_FILTER, FALSE, strFile, strPath) == IDOK ) {
		SetModified();
		// ﾃﾞｰﾀの反映
		m_strTexture = strFile;
		UpdateData(FALSE);
		// 文字選択状態
		m_ctTextureFile.SetFocus();
		m_ctTextureFile.SetSel(0, -1);
	}
}

void CViewSetup5::OnChange()
{
	SetModified();
}

void CViewSetup5::OnDefColor()
{
	extern	LPCTSTR		g_szNcViewColDef[];
	int			i;
	COLORREF	clr;

	for ( i=0; i<SIZEOF(m_colView); i++ ) {
		clr = ConvertSTRtoRGB(g_szNcViewColDef[i+NCCOL_GL_WRK]);
		if ( m_colView[i] != clr ) {
			m_colView[i] = clr;
			m_brColor[i].DeleteObject();
			m_brColor[i].CreateSolidBrush( m_colView[i] );
			m_ctColor[i].Invalidate();
			SetModified();
		}
	}

}
