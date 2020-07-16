// MakeNCDWiz2.cpp : 実装ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "MakeNCDWiz.h"
#include "MCOption.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
extern	CMagaDbg	g_dbg;
#endif

// CMakeNCDWiz2 ダイアログ

BEGIN_MESSAGE_MAP(CMakeNCDWiz2, CPropertyPage)
	ON_BN_CLICKED(IDC_MAKENCD_G10, OnBnClickedG10)
	ON_EN_KILLFOCUS(IDC_MAKENCD_TOOL, OnEnKillfocusTool)
	ON_CBN_SELCHANGE(IDC_MAKENCD_MC, OnSelchangeMC)
END_MESSAGE_MAP()

CMakeNCDWiz2::CMakeNCDWiz2() : CPropertyPage(CMakeNCDWiz2::IDD)
{
	m_psp.dwFlags &= ~PSP_HASHELP;

	m_nCorrect = m_nPreference = 0;
	m_bG10 = FALSE;
}

CMakeNCDWiz2::~CMakeNCDWiz2()
{
}

void CMakeNCDWiz2::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MAKENCD_TOOL, m_nTool);
	DDX_Control(pDX, IDC_MAKENCD_MC, m_ctMachine);
	DDX_Control(pDX, IDC_MAKENCD_G10R, m_dTool);
	DDX_Radio(pDX, IDC_MAKENCD_G41, m_nCorrect);
	DDX_Radio(pDX, IDC_MAKENCD_CORRECT, m_nPreference);
	DDX_Text(pDX, IDC_MAKENCD_MCTOOL, m_strTool);
	DDX_Check(pDX, IDC_MAKENCD_G10, m_bG10);
	DDX_CBString(pDX, IDC_MAKENCD_MC, m_strMachineRef);
}

// CMakeNCDWiz2 メッセージ ハンドラ

BOOL CMakeNCDWiz2::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	// ｺﾝﾄﾛｰﾙの初期化
	m_dTool.EnableWindow(m_bG10);
	// ｺﾝﾎﾞﾎﾞｯｸｽに機械情報の履歴を追加
	m_optMC.AddMCHistory_ComboBox(m_ctMachine);
	m_strMachine = m_optMC.GetMCHeadFileName();
	m_nCurSelMC = m_ctMachine.GetCount() > 1 ? 0 : -1;
	m_ctMachine.SetCurSel( m_nCurSelMC );

	return TRUE;  // return TRUE unless you set the focus to a control
	// 例外 : OCX プロパティ ページは必ず FALSE を返します。
}

BOOL CMakeNCDWiz2::OnSetActive()
{
	((CPropertySheet *)GetParent())->SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);
	return CPropertyPage::OnSetActive();
}

LRESULT CMakeNCDWiz2::OnWizardNext()
{
	UpdateData();
	// ﾃﾞｰﾀﾁｪｯｸ
	if ( m_nTool <= 0 ) {
		AfxMessageBox(IDS_ERR_UNDERZERO, MB_OK|MB_ICONEXCLAMATION);
		m_nTool.SetFocus();
		m_nTool.SetSel(0, -1);
		return -1;
	}
	if ( m_dMachine==0.0 || (m_bG10 && m_dTool==0.0) ) {
		AfxMessageBox(IDS_ERR_SETTING, MB_OK|MB_ICONEXCLAMATION);
		m_dTool.SetFocus();
		m_dTool.SetSel(0, -1);
		return -1;
	}
	SetRefTool();

	LPMAKENCDWIZARDPARAM pParam = ((CMakeNCDWiz *)GetParent())->m_pParam;
	pParam->nCorrect= m_nCorrect;
	pParam->nPref	= m_nPreference;
	pParam->nTool	= m_nTool;
	pParam->bG10	= m_bG10;
	pParam->dTool	= m_bG10 ? (double)m_dTool : m_dMachine;

	return IDD_MAKENCD_WIZ9;	// 次に表示するﾀﾞｲｱﾛｸﾞID
}

LRESULT CMakeNCDWiz2::OnWizardBack()
{
	return IDD_MAKENCD_WIZ1;
}

void CMakeNCDWiz2::OnBnClickedG10()
{
	UpdateData();
	m_dTool.EnableWindow(m_bG10);
}

void CMakeNCDWiz2::OnEnKillfocusTool()
{
	UpdateData();
	SetRefTool();
}

void CMakeNCDWiz2::OnSelchangeMC()
{
	int	nIndex = m_ctMachine.GetCurSel();
	if ( nIndex < 0 )
		return;

	CString	strFileName, strPath;

	// 「参照...」を選択？
	if ( m_ctMachine.GetCount() == nIndex+1 ) {
		strFileName = GetMCListFile(m_nCurSelMC);
		if ( ::NCVC_FileDlgCommon(IDS_OPTION_MC, IDS_MC_FILTER, strFileName) != IDOK ) {
			m_ctMachine.SetCurSel( m_nCurSelMC );
			return;
		}
	}
	else {
		m_nCurSelMC = nIndex;
		strFileName = GetMCListFile(nIndex);
		if ( strFileName.IsEmpty() )
			return;
	}

	// 機械情報読み込み
	m_optMC.ReadMCoption(strFileName, FALSE);
	m_strMachine = strFileName;

	// ｺﾝﾎﾞﾎﾞｯｸｽ表示更新
	::Path_Name_From_FullPath(strFileName, strPath, m_strMachineRef);

	// 工具情報取得
	SetRefTool();
}

CString CMakeNCDWiz2::GetMCListFile(int nIndex)
{
	CString	strFileName;
	const	CStringList*	pList = m_optMC.GetMCList();
	POSITION pos = pList->GetHeadPosition();
	// ｺﾝﾎﾞﾎﾞｯｸｽの順番からﾌﾙﾊﾟｽ情報を取得
	while ( nIndex-- > 0 && pos )
		pList->GetNext(pos);
	if ( pos )
		strFileName = pList->GetAt(pos);
	return strFileName;
}

void CMakeNCDWiz2::SetRefTool(void)
{
	boost::optional<double>	dMachine = m_optMC.GetToolD(m_nTool);
	if ( dMachine ) {
		m_dMachine = *dMachine;
		m_strTool.Format(IDS_MAKENCD_FORMAT, m_dMachine);
	}
	else
		m_strTool.Empty();
	UpdateData(FALSE);
}
