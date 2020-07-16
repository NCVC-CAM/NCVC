// MakeNCDlgEx21.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "Layer.h"
#include "DXFDoc.h"
#include "MakeNCDlg.h"
#include "MakeNCDlgEx2.h"
#include "MakeNCDlgEx21.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

BEGIN_MESSAGE_MAP(CMakeNCDlgEx21, CDialog)
	//{{AFX_MSG_MAP(CMakeNCDlgEx21)
	ON_BN_CLICKED(IDC_MKNCEX21_STEP, OnStep)
	ON_EN_KILLFOCUS(IDC_MKNC_NCFILE, OnKillFocusNCFile)
	ON_BN_CLICKED(IDC_MKNC_NCFILEUP, OnMKNCFileUp)
	ON_BN_CLICKED(IDC_MKNCEX_COPY, OnCopy)
	ON_BN_CLICKED(IDC_MKNCEX_PARTOUT, OnPartOut)
	ON_CBN_SELCHANGE(IDC_MKNCEX_LAYER, OnSelChangeLayer)
	ON_CBN_SETFOCUS(IDC_MKNCEX_LAYER, OnSetFocusLayer)
	ON_BN_CLICKED(IDC_MKNCEX_NEW, OnNewLayerFile)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMakeNCDlgEx21 ダイアログ

CMakeNCDlgEx21::CMakeNCDlgEx21(CMakeNCDlgEx2* pParent, int nIndex, const CString& strLayerFile)
	: CDialog(CMakeNCDlgEx21::IDD, pParent)
{
	m_strLayerFile = strLayerFile;
	//{{AFX_DATA_INIT(CMakeNCDlgEx21)
	//}}AFX_DATA_INIT

	// ﾚｲﾔ情報のｾｯﾄ(上位ﾀﾞｲｱﾛｸﾞの表示順)
	CLayerData*		pData;
	try {
		const CListCtrl& Layer = pParent->m_ctLayerList;
		for ( int i=0; i<Layer.GetItemCount(); i++ ) {
			pData = new CLayerData((const CLayerData *)(Layer.GetItemData(i)), Layer.GetCheck(i));
			m_obLayer.Add(pData);
		}
	}
	catch (CMemoryException* e) {
		if ( pData )
			delete	pData;
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		m_nIndex = -1;
		return;
	}

	// 初期表示ﾃﾞｰﾀは nIndex から取得
	m_nIndex = nIndex<0 || nIndex>m_obLayer.GetUpperBound() ? 0 : nIndex;
	pData = m_obLayer[m_nIndex];
	::Path_Name_From_FullPath(pData->m_strNCFile, m_strNCPath, m_strNCFileName);
	m_bDrill	= pData->m_bDrillZ;
	m_bPartOut	= m_strNCFileName.IsEmpty() ? FALSE : pData->m_bPartOut;
	m_bCheck	= pData->m_bCutTarget;
	// ｶｽﾀﾑｺﾝﾄﾛｰﾙの初期化は CDialog::OnInitDialog() 以降でないとｱｻｰﾄｴﾗｰ
//	m_dZCut = pData->m_dZCut;
}

CMakeNCDlgEx21::~CMakeNCDlgEx21()
{
	for ( int i=0; i<m_obLayer.GetSize(); i++ )
		delete	m_obLayer[i];
}

void CMakeNCDlgEx21::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMakeNCDlgEx21)
	DDX_Control(pDX, IDC_MKNCEX_STATIC3, m_ctPartEnable3);
	DDX_Control(pDX, IDC_MKNCEX_STATIC2, m_ctPartEnable2);
	DDX_Control(pDX, IDC_MKNCEX_STATIC1, m_ctPartEnable1);
	DDX_Control(pDX, IDC_MKNCEX_LAYER, m_ctLayer);
	DDX_Control(pDX, IDC_MKNC_NCFILE, m_ctNCFileName);
	DDX_Control(pDX, IDC_MKNC_NCPATH, m_ctNCPath);
	DDX_Control(pDX, IDC_MKNC_NCFILEUP, m_ctPartEnable4);
	DDX_Control(pDX, IDOK, m_ctOK);
	DDX_Control(pDX, IDC_MKNC3_ZSTEP, m_dZStep);
	DDX_Control(pDX, IDC_MKNC1_ZCUT, m_dZCut);
	DDX_Text(pDX, IDC_MKNC_NCFILE, m_strNCFileName);
	DDX_Check(pDX, IDC_MKNCEX21_DRILL, m_bDrill);
	DDX_Check(pDX, IDC_MKNCEX_CHECK, m_bCheck);
	DDX_Check(pDX, IDC_MKNCEX_PARTOUT, m_bPartOut);
	//}}AFX_DATA_MAP
}

void CMakeNCDlgEx21::GetNowState(void)
{
	CLayerData* pData = m_obLayer[m_nIndex];
	UpdateData();
	pData->m_bCutTarget	= m_bCheck;
	pData->m_bDrillZ	= m_bDrill;
	pData->m_bPartOut	= m_bPartOut;
	pData->m_dZCut		= m_dZCut;
	pData->m_strNCFile	= m_strNCPath + m_strNCFileName;
}

void CMakeNCDlgEx21::SetNowState(int nIndex)
{
	m_nIndex = nIndex;
	CLayerData* pData = m_obLayer[m_nIndex];
	m_bCheck	= pData->m_bCutTarget;
	m_bDrill	= pData->m_bDrillZ;
	m_bPartOut	= pData->m_bPartOut;
	m_dZCut		= pData->m_dZCut;
	::Path_Name_From_FullPath(pData->m_strNCFile, m_strNCPath, m_strNCFileName);
	::PathSetDlgItemPath(m_hWnd, IDC_MKNC_NCPATH, m_strNCPath);
	UpdateData(FALSE);
	EnablePartOut();
}

void CMakeNCDlgEx21::EnablePartOut(void)
{
	if ( m_bPartOut ) {
		m_ctPartEnable1.EnableWindow(TRUE);
		m_ctPartEnable2.EnableWindow(TRUE);
		m_ctPartEnable3.EnableWindow(TRUE);
		m_ctPartEnable4.EnableWindow(TRUE);
		m_ctNCPath.EnableWindow(TRUE);
		m_ctNCFileName.EnableWindow(TRUE);
		if ( m_strNCFileName.IsEmpty() ) {
			// 親ﾀﾞｲｱﾛｸﾞからNCﾌｧｲﾙ名取得
			::Path_Name_From_FullPath(((CMakeNCDlgEx2 *)m_pParentWnd)->GetNCFileName(),
					m_strNCPath, m_strNCFileName, FALSE);
			m_strNCFileName += "_" + m_obLayer[m_nIndex]->m_strLayer +
				AfxGetNCVCApp()->GetDocExtString(TYPE_NCD);
			::PathSetDlgItemPath(m_hWnd, IDC_MKNC_NCPATH, m_strNCPath);
			UpdateData(FALSE);
		}
	}
	else {
		m_ctPartEnable1.EnableWindow(FALSE);
		m_ctPartEnable2.EnableWindow(FALSE);
		m_ctPartEnable3.EnableWindow(FALSE);
		m_ctPartEnable4.EnableWindow(FALSE);
		m_ctNCPath.EnableWindow(FALSE);
		m_ctNCFileName.EnableWindow(FALSE);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CMakeNCDlgEx21 メッセージ ハンドラ

BOOL CMakeNCDlgEx21::OnInitDialog() 
{
	CDialog::OnInitDialog();

	if ( m_nIndex < 0 ) {
		EndDialog(IDCANCEL);
		return TRUE;
	}

	// ﾀｲﾄﾙ設定
	GetWindowText(m_strCaption);	// 元のｳｨﾝﾄﾞｳﾀｲﾄﾙを取得
	if ( !m_strLayerFile.IsEmpty() )
		SetWindowText(::AddDialogTitle2File(m_strCaption, m_strLayerFile));

	m_dZCut = m_obLayer[m_nIndex]->m_dZCut;
	// ﾚｲﾔﾘｽﾄの初期化
	for ( int i=0; i<m_obLayer.GetSize(); i++ )
		m_ctLayer.AddString( m_obLayer[i]->m_strLayer );
	// ﾊﾟｽ表示の最適化
	::PathSetDlgItemPath(m_hWnd, IDC_MKNC_NCPATH, m_strNCPath);
	// 個別出力ﾌｧｲﾙの有効化
	EnablePartOut();
	// ﾀﾞﾌﾞﾙｸﾘｯｸされたｱｲﾃﾑを選択
	m_ctLayer.SetCurSel(m_nIndex);
	m_ctLayer.SetFocus();

	return FALSE;
}

void CMakeNCDlgEx21::OnOK() 
{
	CString	strNCFile;

	// Enterｷｰが押されたとき他のｺﾝﾄﾛｰﾙにKillFocusせずにここに来る
	m_ctOK.SetFocus();

	// 最後の情報を取得
	GetNowState();

	// 各ﾌｧｲﾙのﾁｪｯｸ
	CLayerData* pData;
	for ( int i=0; i<m_obLayer.GetSize(); i++ ) {
		pData = m_obLayer[i];
		if ( !pData->m_bCutTarget )
			continue;
		// 個別ﾌｧｲﾙ名のﾁｪｯｸ
		if ( pData->m_bPartOut ) {
			strNCFile = pData->m_strNCFile;
			if ( !CheckMakeDlgFileExt(TYPE_NCD, strNCFile) ) {
				m_ctNCFileName.SetFocus();
				m_ctNCFileName.SetSel(0, -1);
				SetNowState(i);
				return;
			}
			if ( strNCFile.CompareNoCase(pData->m_strNCFile) )
				pData->m_strNCFile = strNCFile;
		}
	}

	// 個別ﾌｧｲﾙの重複ﾁｪｯｸ(上位ﾌｧｲﾙとの重複ﾁｪｯｸはなし)
	CMakeNCDlgEx2*	pParent = (CMakeNCDlgEx2 *)m_pParentWnd;
	CString	strResult( pParent->GetDocument()->CheckDuplexFile(CString(), &m_obLayer) );
	if ( !strResult.IsEmpty() ) {
		int	nResult = m_ctLayer.FindString(-1, strResult);
		m_ctLayer.SetCurSel(nResult);
		SetNowState(nResult);
		return;
	}

	CDialog::OnOK();
}

void CMakeNCDlgEx21::OnSetFocusLayer() 
{
	m_nIndex = m_ctLayer.GetCurSel();
}

void CMakeNCDlgEx21::OnSelChangeLayer() 
{
	int	nIndex = m_ctLayer.GetCurSel();
	if ( nIndex<0 || nIndex>m_obLayer.GetUpperBound() ) {
		m_ctNCPath.SetWindowText("");
		m_strNCFileName.Empty();
		m_bCheck = m_bDrill = m_bPartOut = FALSE;
		m_dZCut = 0.0;
		UpdateData(FALSE);
		EnablePartOut();
	}
	else {
		// 現在の情報を取得
		GetNowState();
		// 切替後の情報をｾｯﾄ
		SetNowState(nIndex);
	}
}

void CMakeNCDlgEx21::OnMKNCFileUp() 
{
	UpdateData();
	MakeDlgFileRefer(IDS_DXF2NCD_FILE, AfxGetNCVCApp()->GetFilterString(TYPE_NCD),
			this, IDC_MKNC_NCPATH, m_strNCPath, m_strNCFileName, FALSE);
	m_ctLayer.SetFocus();
	// 文字選択状態
	m_ctNCFileName.SetFocus();
	m_ctNCFileName.SetSel(0, -1);
}

void CMakeNCDlgEx21::OnKillFocusNCFile() 
{
	UpdateData();
	MakeDlgKillFocus(m_strNCPath, m_strNCFileName, this, IDC_MKNC_NCPATH);
}

void CMakeNCDlgEx21::OnCopy() 
{
	// 現在の情報を取得
	GetNowState();
	// 現在位置以降にﾃﾞｰﾀをｺﾋﾟｰ
	CLayerData* pData = m_obLayer[m_nIndex];
	BOOL	bCheck		= pData->m_bCutTarget;
	double	dZCut		= pData->m_dZCut;
	BOOL	bDrill		= pData->m_bDrillZ;
	BOOL	bPartOut	= pData->m_bPartOut;
	CString	strPath, strFile, strNCFile;
	::Path_Name_From_FullPath(((CMakeNCDlgEx2 *)m_pParentWnd)->GetNCFileName(),
			strPath, strFile, FALSE);
	strNCFile = strPath + strFile + "_";
	for ( int i=m_nIndex+1; i<m_obLayer.GetSize(); i++ ) {
		pData = m_obLayer[i];
		pData->m_bCutTarget	= bCheck;
		pData->m_dZCut		= dZCut;
		pData->m_bDrillZ	= bDrill;
		pData->m_bPartOut	= bPartOut;
		if ( bPartOut && pData->m_strNCFile.IsEmpty() )
			pData->m_strNCFile = strNCFile + pData->m_strLayer + AfxGetNCVCApp()->GetDocExtString(TYPE_NCD);
	}
	m_ctLayer.SetFocus();
}

void CMakeNCDlgEx21::OnStep() 
{
	// 現在の情報を取得
	GetNowState();
	double	dZStep = m_dZStep;	// 変換ｺﾝｽﾄﾗｸﾀなのでﾙｰﾌﾟ外で
	// 現在位置以降にｽﾃｯﾌﾟﾃﾞｰﾀを加算
	double	dZCut = m_obLayer[m_nIndex]->m_dZCut;
	for ( int i=m_nIndex+1, nCnt=1; i<m_obLayer.GetSize(); i++, nCnt++ )
		m_obLayer[i]->m_dZCut = dZCut+dZStep*nCnt;
	m_ctOK.SetFocus();
}

void CMakeNCDlgEx21::OnPartOut() 
{
	int	nIndex = m_ctLayer.GetCurSel();
	if ( nIndex<0 || nIndex>m_obLayer.GetUpperBound() ) {
		m_ctPartEnable1.EnableWindow(FALSE);
		m_ctPartEnable2.EnableWindow(FALSE);
		m_ctPartEnable3.EnableWindow(FALSE);
		m_ctPartEnable4.EnableWindow(FALSE);
		m_ctNCPath.EnableWindow(FALSE);
		m_ctNCFileName.EnableWindow(FALSE);
	}
	else {
		UpdateData();
		EnablePartOut();
	}
}

void CMakeNCDlgEx21::OnNewLayerFile() 
{
	CDXFDoc*	pDoc = ((CMakeNCDlgEx2 *)m_pParentWnd)->GetDocument();
	CString		strPath, strFile;
	if ( m_strLayerFile.IsEmpty() )
		CreateLayerFile(pDoc, strPath, strFile);
	else
		::Path_Name_From_FullPath(m_strLayerFile, strPath, strFile);

	if ( ::NCVC_FileDlgCommon(IDS_OPTION_LAYER2INITSAVE, IDS_NCL_FILTER,
				strFile, strPath, FALSE, OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT) == IDOK ) {
		pDoc->SaveLayerMap(strFile);
		if ( m_strLayerFile.CompareNoCase(strFile) != 0 ) {
			m_strLayerFile = strFile;
			SetWindowText(::AddDialogTitle2File(m_strCaption, m_strLayerFile));
		}
	}
}
