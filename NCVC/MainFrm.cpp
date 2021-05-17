// MainFrm.cpp : CMainFrame �N���X�̎���
//

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "NCChild.h"
#include "DXFChild.h"
#include "NCDoc.h"
#include "NCListView.h"
#include "DXFDoc.h"
#include "NCWorkDlg.h"
#include "ExecOption.h"
#include "ViewOption.h"
#include "ToolBarSetupDlg.h"
#include "SplashWnd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
//#define	_DEBUGOLD
#undef	_DEBUGOLD
#endif

// ��ݺ��۰�BITMAP_SIZE
static	const	int		COMMONCTRL_LISTBITMAPSIZE = 12;
static	const	int		COMMONCTRL_TREEBITMAPSIZE = 16;

// �����\���p°��ް���ݒ�`
static	CUSTTBBUTTON	tbMainButtons[] = {
	//�R�}���h,				���,				�X�^�C��,		�\��
	{ ID_FILE_OPEN,				TBSTATE_ENABLED,	TBSTYLE_BUTTON,	TRUE  },
	{ ID_FILE_SAVE,				TBSTATE_ENABLED,	TBSTYLE_BUTTON,	TRUE  },
	{ ID_SEPARATOR,				TBSTATE_ENABLED,	TBSTYLE_SEP,	TRUE  },
	{ ID_EDIT_CUT,				TBSTATE_ENABLED,	TBSTYLE_BUTTON,	TRUE  },
	{ ID_EDIT_COPY,				TBSTATE_ENABLED,	TBSTYLE_BUTTON,	TRUE  },
	{ ID_EDIT_PASTE,			TBSTATE_ENABLED,	TBSTYLE_BUTTON,	TRUE  },
	{ ID_EDIT_FIND,				TBSTATE_ENABLED,	TBSTYLE_BUTTON,	FALSE },
	{ ID_SEPARATOR,				TBSTATE_ENABLED,	TBSTYLE_SEP,	TRUE  },
	{ ID_DXFVIEW_LAYER,			TBSTATE_ENABLED,	TBSTYLE_BUTTON,	TRUE  },
	{ ID_VIEW_FIT,				TBSTATE_ENABLED,	TBSTYLE_BUTTON,	TRUE  },
	{ ID_NCVIEW_ALLFIT,			TBSTATE_ENABLED,	TBSTYLE_BUTTON,	FALSE },
	{ ID_VIEW_LENSP,			TBSTATE_ENABLED,	TBSTYLE_BUTTON,	TRUE  },
	{ ID_VIEW_LENSN,			TBSTATE_ENABLED,	TBSTYLE_BUTTON,	TRUE  },
	{ ID_SEPARATOR,				TBSTATE_ENABLED,	TBSTYLE_SEP,	TRUE  },
	{ ID_HELP_USING,			TBSTATE_ENABLED,	TBSTYLE_BUTTON,	TRUE  },
	{ ID_CONTEXT_HELP,			TBSTATE_ENABLED,	TBSTYLE_BUTTON,	FALSE },
	{ 0,						0,					0,				FALSE }	// ��`�I��
};
static	CUSTTBBUTTON	tbTraceButtons[] = {
	{ ID_NCVIEW_TRACE_RUN,		TBSTATE_ENABLED,	TBSTYLE_BUTTON,	TRUE  },
	{ ID_NCVIEW_TRACE_PAUSE,	TBSTATE_ENABLED,	TBSTYLE_BUTTON,	TRUE  },
	{ ID_NCVIEW_TRACE_STOP,		TBSTATE_ENABLED,	TBSTYLE_BUTTON,	TRUE  },
	{ ID_NCVIEW_TRACE_CURSOR,	TBSTATE_ENABLED,	TBSTYLE_BUTTON,	TRUE  },
	{ ID_NCVIEW_TRACE_CURSOR2,	TBSTATE_ENABLED,	TBSTYLE_BUTTON,	TRUE  },
	{ ID_NCVIEW_TRACE_BREAK,	TBSTATE_ENABLED,	TBSTYLE_BUTTON,	TRUE  },
	{ ID_NCVIEW_TRACE_BREAKOFF,	TBSTATE_ENABLED,	TBSTYLE_BUTTON,	FALSE },
	{ ID_NCVIEW_TRACE_FAST,		TBSTATE_ENABLED,	TBSTYLE_BUTTON,	TRUE  },
	{ ID_NCVIEW_TRACE_MIDDLE,	TBSTATE_ENABLED,	TBSTYLE_BUTTON,	TRUE  },
	{ ID_NCVIEW_TRACE_LOW,		TBSTATE_ENABLED,	TBSTYLE_BUTTON,	TRUE  },
	{ 0,						0,					0,				FALSE }
};
static	CUSTTBBUTTON	tbMakeButtons[] = {
	{ ID_OPTION_MAKENC,			TBSTATE_ENABLED,	TBSTYLE_BUTTON,	TRUE  },
	{ ID_SEPARATOR,				TBSTATE_ENABLED,	TBSTYLE_SEP,	TRUE  },
	{ ID_FILE_DXF2NCD,			TBSTATE_ENABLED,	TBSTYLE_BUTTON,	TRUE  },
	{ ID_FILE_DXF2NCD_EX1,		TBSTATE_ENABLED,	TBSTYLE_BUTTON,	TRUE  },
	{ ID_FILE_DXF2NCD_EX2,		TBSTATE_ENABLED,	TBSTYLE_BUTTON,	TRUE  },
	{ ID_FILE_DXF2NCD_SHAPE,	TBSTATE_ENABLED,	TBSTYLE_BUTTON,	TRUE  },
	{ ID_FILE_DXF2NCD_LATHE,	TBSTATE_ENABLED,	TBSTYLE_BUTTON,	TRUE  },
	{ ID_FILE_DXF2NCD_WIRE,		TBSTATE_ENABLED,	TBSTYLE_BUTTON,	TRUE  },
	{ ID_SEPARATOR,				TBSTATE_ENABLED,	TBSTYLE_SEP,	TRUE  },
	{ ID_FILE_NCD2DXF,			TBSTATE_ENABLED,	TBSTYLE_BUTTON,	TRUE  },
	{ 0,						0,					0,				FALSE }
};
static	CUSTTBBUTTON	tbShapeButtons[] = {
	{ ID_EDIT_SHAPE_VEC,		TBSTATE_ENABLED,	TBSTYLE_BUTTON,	TRUE  },
	{ ID_EDIT_SHAPE_START,		TBSTATE_ENABLED,	TBSTYLE_BUTTON,	TRUE  },
	{ ID_EDIT_SHAPE_OUT,		TBSTATE_ENABLED,	TBSTYLE_BUTTON,	TRUE  },
	{ ID_EDIT_SHAPE_POC,		TBSTATE_ENABLED,	TBSTYLE_BUTTON,	TRUE  },
	{ ID_EDIT_SHAPE_SEL,		TBSTATE_ENABLED,	TBSTYLE_BUTTON,	TRUE  },
	{ ID_EDIT_SHAPE_DEL,		TBSTATE_ENABLED,	TBSTYLE_BUTTON,	TRUE  },
	{ ID_SEPARATOR,				TBSTATE_ENABLED,	TBSTYLE_SEP,	TRUE  },
	{ ID_EDIT_SHAPE_POCKET,		TBSTATE_ENABLED,	TBSTYLE_BUTTON,	TRUE  },
	{ ID_EDIT_SHAPE_OUTLINE,	TBSTATE_ENABLED,	TBSTYLE_BUTTON,	TRUE  },
	{ ID_EDIT_SHAPE_PROP,		TBSTATE_ENABLED,	TBSTYLE_BUTTON,	TRUE  },
	{ 0,						0,					0,				FALSE }
};
static	CUSTTBBUTTON	tbMachineButtons[] = {
	{ ID_OPTION_MC,				TBSTATE_ENABLED,	TBSTYLE_BUTTON,	TRUE  },
	{ ID_OPTION_MCCOMBO,		TBSTATE_ENABLED,	TBSTYLE_SEP,	TRUE  },
	{ 0,						0,					0,				FALSE }
};

// �ƭ��̐�
static	const	UINT	MAINMENUCOUNT = 4;
static	const	UINT	DOCMENUCOUNT = 6;

// °��ްID��
struct	NCVCTOOLINFO
{
	int		nID;		// ToolBar ID
	int		nBitmap;	// Bitmap ID
	LPCTSTR	pszTitle;	// ����
	LPCUSTTBBUTTON	lpCustTb;	// ��̫��°��ް���
};
static	NCVCTOOLINFO	g_tbInfo[] = {
	{AFX_IDW_TOOLBAR,	IDR_MAINFRAME,		"Main",		tbMainButtons},
	{IDR_TRACEBAR,		IDR_TRACEBAR,		"Trace",	tbTraceButtons},
	{IDR_MAKENCD,		IDR_MAKENCD,		"Make",		tbMakeButtons},
	{IDR_SHAPE,			IDR_SHAPE,			"Shape",	tbShapeButtons},
	{IDR_ADDINBAR,		IDR_ADDINBAR,		"Addin",	NULL},
	{IDR_EXECBAR,		IDR_EXECBAR,		"Exec",		NULL},
	{IDR_MACHINE,		IDR_MACHINE,		"Machine",	tbMachineButtons}
};

// �ð��ײ� �ݼ޹��
static UINT g_nIndicators[] =
{
	ID_SEPARATOR,
	ID_SEPARATOR,	// ��۸�ڽ�ް�̈�
	ID_INDICATOR_DATE,
	ID_INDICATOR_TIME
};
static	const	int		PROGRESS_INDEX = 1;

extern	const	int		gg_nIconX;
extern	const	int		gg_nIconY;

// �O�����؋N���p��ܰ��
static	LPCTSTR	g_szCommandReplace[] = {
	"FileFullPath", "FilePath", "FileNameNoExt", "FileName",
	"SelectNcLine"
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CMDIFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWnd)
	ON_WM_CREATE()
	ON_WM_CLOSE()
	ON_WM_ENDSESSION()
	ON_WM_DESTROY()
	ON_WM_INITMENUPOPUP()
	ON_WM_MENUSELECT()
	ON_WM_SIZE()
	ON_WM_DRAWITEM()
	ON_WM_MEASUREITEM()
	ON_WM_SYSCOLORCHANGE()
	ON_WM_CONTEXTMENU()
	ON_WM_SYSCOMMAND()
	ON_COMMAND(ID_WINDOW_NEXT, &CMainFrame::OnNextPane)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_DATE, &CMainFrame::OnUpdateDate)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_TIME, &CMainFrame::OnUpdateTime)
	ON_COMMAND(ID_VIEW_TOOLBARCUSTOM, &CMainFrame::OnViewToolbarCustom)
	ON_UPDATE_COMMAND_UI(ID_EDIT_CUT, &CMainFrame::OnUpdateEditCut)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, &CMainFrame::OnUpdateEditPaste)
	// ̧�ٍēǍ��ʒm
	ON_MESSAGE(WM_USERFILECHANGENOTIFY, &CMainFrame::OnUserFileChange)
	// ��޲݁C�O�����ع���݋N��°��ް�p°�����
	ON_NOTIFY_EX(TTN_NEEDTEXT, 0, OnToolTipText)
	ON_UPDATE_COMMAND_UI_RANGE(EXECSTARTID,  ADDINSTARTID-1,    &CMainFrame::OnUpdateExecButtonCheck)
	ON_UPDATE_COMMAND_UI_RANGE(ADDINSTARTID, EXECADDIN_ENDID-1, &CMainFrame::OnUpdateAddinButtonCheck)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMainFrame �R���X�g���N�V����/�f�X�g���N�V����

CMainFrame::CMainFrame()
{
	m_nSelectGDI = 0;

	// Ӱ��ڽ�޲�۸��߲���̏�����
	ZEROCLR(m_pModelessDlg);	// m_pModelessDlg[i++]=NULL
	// ��޲��ƭ��̂��� ON_COMMAND ү����ϯ�߂������Ȃ��ƭ����L���ɂ���
	m_bAutoMenuEnable = FALSE;
	m_nStatusPane1Width = -1;
	m_bMenuSelect = FALSE;

	// ��̫�ı��݂̓ǂݍ���
	m_hDefIconLarge = (HICON)::LoadImage(AfxGetResourceHandle(),
			MAKEINTRESOURCE(IDI_DEFICONLARGE), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
	m_hDefIconSmall = (HICON)::LoadImage(AfxGetResourceHandle(),
			MAKEINTRESOURCE(IDI_DEFICONSMALL), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
	ASSERT(m_hDefIconLarge);
	ASSERT(m_hDefIconSmall);
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame �N���X�̃I�[�o���C�h�֐�

BOOL CMainFrame::OnCommand(WPARAM wParam, LPARAM lParam) 
{
	if ( lParam==0 && HIWORD(wParam)==1 )
		return __super::OnCommand(wParam, lParam);

#ifdef _DEBUG
	printf("CMainFrame::OnCommand() Start\n");
#endif

	WORD	wID = LOWORD(wParam);

	// �o�^���ꂽ�O�����ع���݂̌Ăяo��
	if ( wID>=EXECSTARTID && wID<AfxGetNCVCApp()->GetMaxExecID() ) {
#ifdef _DEBUG
		printf("Exec CommandID=%d\n", wID);
#endif
		const CExecOption* pExec = AfxGetNCVCApp()->GetLookupExecID(wID);
		if ( pExec ) {
			CString	strCommand;
			const CDocument* pDoc = GetActiveFrame()->GetActiveDocument();
			// �����̒u��
			strCommand = pDoc ? CommandReplace(pExec, pDoc) : pExec->GetCommand();
#ifdef _DEBUG
			printf("ProgName=%s\n", LPCTSTR(pExec->GetFileName()));
			printf("Command =%s\n", LPCTSTR(strCommand));
#endif
			CreateOutsideProcess(pExec->GetFileName(), strCommand);
			return TRUE;
		}
	}

	// �o�^���ꂽ��޲݊֐��̌Ăяo��
	if ( wID>=ADDINSTARTID && wID<AfxGetNCVCApp()->GetMaxAddinID() ) {
#ifdef _DEBUG
		printf("Addin CommandID=%d\n", wID);
#endif
		AfxGetNCVCApp()->CallAddinFunc(wID);
		return TRUE;
	}

	return __super::OnCommand(wParam, lParam);
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame ���ފ֐�

void CMainFrame::SaveWindowState(void)
{
	WINDOWPLACEMENT		wp;
	wp.length = sizeof(WINDOWPLACEMENT);
	GetWindowPlacement(&wp);	// ����޳�̏��Get
	CString	strRegKey;
	VERIFY(strRegKey.LoadString(IDS_REGKEY_WINDOW));
	AfxGetNCVCApp()->SaveWindowState(strRegKey, wp);
}

BOOL CMainFrame::RestoreWindowState(void)
{
	WINDOWPLACEMENT		wp;
	wp.length = sizeof(WINDOWPLACEMENT);
	GetWindowPlacement(&wp);
	CString	strRegKey;
	VERIFY(strRegKey.LoadString(IDS_REGKEY_WINDOW));

	if ( AfxGetNCVCApp()->GetWindowState(strRegKey, &wp) ) {
		SetWindowPlacement(&wp);
		return TRUE;
	}

	return FALSE;
}

void CMainFrame::AllModelessDlg_PostSwitchMessage(void)
{
	for ( int i=0; i<SIZEOF(m_pModelessDlg); i++ ) {
		if ( m_pModelessDlg[i] )
			m_pModelessDlg[i]->PostMessage(WM_USERSWITCHDOCUMENT);
	}
}

void CMainFrame::ChangeViewOption(void)
{
	extern	const	PENSTYLE	g_penStyle[];	// ViewOption.cpp
	static	int		colNC[] = {
		NCCOL_G0, NCCOL_G1, NCCOL_G1Z,
		NCCOL_CORRECT,
		NCCOL_CYCLE, NCCOL_WORK, NCCOL_MAXCUT
	};
	static	int		typeNCLine[] = {
		NCCOLLINE_G0, NCCOLLINE_G1, NCCOLLINE_G1Z,
		NCCOLLINE_G1,		// �␳�\���͓�������
		NCCOLLINE_CYCLE, NCCOLLINE_WORK, NCCOLLINE_MAXCUT
	};
	static	int		colBrushNC[] = {
		NCCOL_CYCLE
	};
	static	int		colDXF[] = {
		DXFCOL_ORIGIN,
		DXFCOL_CUTTER, DXFCOL_START, DXFCOL_MOVE,
		DXFCOL_OUTLINE,
		DXFCOL_WORK
	};
	static	int		colBrushDXF[] = {
		DXFCOL_CUTTER, DXFCOL_START, DXFCOL_MOVE, DXFCOL_TEXT
	};
	int			i, pen;
	COLORREF	col;
	CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();	// Can't const(GetLogFont)

	// ������݂̍쐬
	for ( i=0; i<SIZEOF(m_penCom[0]); i++ ) {
		if ( (HPEN)m_penCom[0][i] )
			m_penCom[0][i].DeleteObject();
		if ( (HPEN)m_penCom[1][i] )
			m_penCom[1][i].DeleteObject();
		col = pOpt->GetDrawColor(i);
		pen = g_penStyle[pOpt->GetDrawType(i)].nPenType;
		m_penCom[0][i].CreatePen(pen, 0, col);
		m_penCom[1][i].CreatePen(pen, 0, col!=RGB(255,255,255) ? col : RGB(0,0,0));
	}

	// ���S���̐F�ݒ�(�޲�ޕ����F)����ݍ쐬
	for ( i=0; i<SIZEOF(m_penOrg[0]); i++ ) {
		col = pOpt->GetNcDrawColor(i+NCCOL_GUIDEX);
		if ( (HPEN)m_penOrg[0][i] )
			m_penOrg[0][i].DeleteObject();
		if ( (HPEN)m_penOrg[1][i] )
			m_penOrg[1][i].DeleteObject();
		pen = g_penStyle[pOpt->GetNCViewFlg(NCVIEWFLG_GUIDESCALE) ? 0 : pOpt->GetNcDrawType(i)].nPenType;
		m_penOrg[0][i].CreatePen(pen, 0, col);
		m_penOrg[1][i].CreatePen(pen, 0, col!=RGB(255,255,255) ? col : RGB(0,0,0));
	}
	// �߽�`����ݍ쐬
	ASSERT( SIZEOF(m_penNC[0]) == SIZEOF(colNC) );
	ASSERT( SIZEOF(m_penNC[0]) == SIZEOF(typeNCLine) );
	for ( i=0; i<SIZEOF(m_penNC[0]); i++ ) {
		if ( (HPEN)m_penNC[0][i] )
			m_penNC[0][i].DeleteObject();
		if ( (HPEN)m_penNC[1][i] )
			m_penNC[1][i].DeleteObject();
		col = pOpt->GetNcDrawColor(colNC[i]);
		pen = g_penStyle[pOpt->GetNcDrawType(typeNCLine[i])].nPenType;
		m_penNC[0][i].CreatePen(pen, 0, col);
		m_penNC[1][i].CreatePen(pen, 0, col!=RGB(255,255,255) ? col : RGB(0,0,0));
	}
	ASSERT( SIZEOF(m_penDXF[0]) == SIZEOF(colDXF) );
	for ( i=0; i<SIZEOF(m_penDXF[0]); i++ ) {
		if ( (HPEN)m_penDXF[0][i] )
			m_penDXF[0][i].DeleteObject();
		if ( (HPEN)m_penDXF[1][i] )
			m_penDXF[1][i].DeleteObject();
		col = pOpt->GetDxfDrawColor(colDXF[i]);
		pen = g_penStyle[pOpt->GetDxfDrawType(i)].nPenType;
		m_penDXF[0][i].CreatePen(pen, 0, col);
		m_penDXF[1][i].CreatePen(pen, 0, col!=RGB(255,255,255) ? col : RGB(0,0,0));
	}
	// ��׼�쐬
	ASSERT( SIZEOF(m_brushDXF[0]) == SIZEOF(colBrushDXF) );
	for ( i=0; i<SIZEOF(m_brushDXF[0]); i++ ) {
		if ( (HBRUSH)m_brushDXF[0][i] )
			m_brushDXF[0][i].DeleteObject();
		if ( (HBRUSH)m_brushDXF[1][i] )
			m_brushDXF[1][i].DeleteObject();
		col = pOpt->GetDxfDrawColor(colBrushDXF[i]);
		m_brushDXF[0][i].CreateSolidBrush(col);
		m_brushDXF[1][i].CreateSolidBrush(col!=RGB(255,255,255) ? col : RGB(0,0,0));
	}
	ASSERT( SIZEOF(m_brushNC[0]) == SIZEOF(colBrushNC) );
	for ( i=0; i<SIZEOF(m_brushNC[0]); i++ ) {
		if ( (HBRUSH)m_brushNC[0][i] )
			m_brushNC[0][i].DeleteObject();
		if ( (HBRUSH)m_brushNC[1][i] )
			m_brushNC[1][i].DeleteObject();
		col = pOpt->GetNcDrawColor(colBrushNC[i]);
		m_brushNC[0][i].CreateSolidBrush(col);
		m_brushNC[1][i].CreateSolidBrush(col!=RGB(255,255,255) ? col : RGB(0,0,0));
	}

	// ÷��̫��
	for ( i=0; i<SIZEOF(m_cfText); i++ ) {
		if ( (HFONT)m_cfText[i] )
			m_cfText[i].DeleteObject();
		m_cfText[i].CreateFontIndirect(pOpt->GetLogFont((DOCTYPE)i));
	}

	// �����̍����ƕ������߂�
	CClientDC	dc(this);
	CFont* pOldFont = dc.SelectObject(&m_cfText[TYPE_NCD]);
	TEXTMETRIC	tm;
	dc.GetTextMetrics(&tm);
	m_nTextHeight = tm.tmHeight + tm.tmExternalLeading;
	m_nTextWidth  = tm.tmAveCharWidth;
	dc.SelectObject(pOldFont);
}

HICON CMainFrame::GetIconHandle(BOOL bLarge, LPCTSTR lpszFile)
{
	HICON	hIcon, hIconLarge, hIconSmall;

	// �܂� ExtractIconEx() �ɂĎ擾
	if ( ::ExtractIconEx(lpszFile, 0, &hIconLarge, &hIconSmall, 1) >= 1 ) {
		hIcon = bLarge ? hIconLarge : hIconSmall;
		if ( !hIcon )
			hIcon = bLarge ? m_hDefIconLarge : m_hDefIconSmall;
	}
	else {
		// ExtractIconEx() �Ŗ�����΁Ç�قɊ֘A�t�����ꂽ���݂��擾
		SHFILEINFO	shFileInfo;
		if ( ::SHGetFileInfo(lpszFile, 0,
				&shFileInfo, sizeof(shFileInfo),
				SHGFI_ICON | (bLarge ? SHGFI_LARGEICON : SHGFI_SMALLICON)) )
			hIcon = shFileInfo.hIcon;
		else
			hIcon = bLarge ? m_hDefIconLarge : m_hDefIconSmall;
	}

	return hIcon;
}

void CMainFrame::CreateDisableToolBar(EN_TOOLBARIMAGE enImage)
{
#ifdef _DEBUG
	printf("CMainFrame::CreateDisableToolBar() Start\n");
#endif
	int		i, nBtnCnt;

	nBtnCnt = m_ilDisableToolBar[enImage].GetImageCount();
	for ( i=0; i<nBtnCnt; i++ )
		m_ilDisableToolBar[enImage].Remove(0);

	// �o�^���ع���̸݂�ڲ���ݗp��	
	nBtnCnt = m_ilEnableToolBar[enImage].GetImageCount();
	CClientDC	dcScreen(this);
	CBitmap		bm;
	bm.CreateCompatibleBitmap(&dcScreen, gg_nIconX*nBtnCnt, gg_nIconY/*+1*/);
	CDC			dcMem;
	dcMem.CreateCompatibleDC(&dcScreen);
	CBitmap*	pOldBitmap = dcMem.SelectObject(&bm);
	CPoint		pt(0,0);
	for ( i=0; i<nBtnCnt; i++ ) {
		m_ilEnableToolBar[enImage].Draw(&dcMem, i, pt, ILD_NORMAL);
		pt.x += gg_nIconX;
	}
	for ( int y=0; y<gg_nIconY/*+1*/; y++ ) {
		for ( int x=0; x<gg_nIconX*nBtnCnt; x++ ) {
			COLORREF col = dcMem.GetPixel(x, y);
			float	dColor = GetRValue(col)*0.30f +
				             GetGValue(col)*0.59f +
							 GetBValue(col)*0.11f;
			dcMem.SetPixelV(x, y, RGB(dColor, dColor, dColor));
		}
	}
	dcMem.SelectObject(pOldBitmap);
	m_ilDisableToolBar[enImage].Add(&bm, (CBitmap*)NULL);
}

CString	CMainFrame::MakeCommand(int a)
{
	CString	strResult;
	ASSERT( a>=0 && a<SIZEOF(g_szCommandReplace) );
	if ( a>=0 && a<SIZEOF(g_szCommandReplace) ) {
		strResult  = "${";
		strResult += g_szCommandReplace[a];
		strResult += "}";
	}
	return strResult;
}

// �{���ł���� TH_MakeNCD.cpp::AddCustomCode() �̂悤�ɉ�͂��邪
// �P�s�����ײ݂����Ȃ̂ŁCCString::Replace() �̎蔲������(^^;)
CString	CMainFrame::CommandReplace(const CExecOption* pExec, const CDocument* pDoc)
{
	CString	strResult(pExec->GetCommand()), strKey, strTmp;
	TCHAR	szDrive[_MAX_DRIVE],
			szDir[_MAX_DIR],
			szFileName[_MAX_FNAME],
			szExt[_MAX_EXT];
	LPTSTR	lpszShortName = NULL;
	int		nReplace = 0;

	if ( pExec->IsShort() ) {
		lpszShortName = new TCHAR[lstrlen(pDoc->GetPathName())+1];
		::GetShortPathName(pDoc->GetPathName(), lpszShortName, lstrlen(pDoc->GetPathName()));
		_tsplitpath_s(lpszShortName,
			szDrive, SIZEOF(szDrive), szDir, SIZEOF(szDir),
			szFileName, SIZEOF(szFileName), szExt, SIZEOF(szExt));
	}
	else
		_tsplitpath_s(pDoc->GetPathName(),
			szDrive, SIZEOF(szDrive), szDir, SIZEOF(szDir),
			szFileName, SIZEOF(szFileName), szExt, SIZEOF(szExt));

	for ( int i=0; i<SIZEOF(g_szCommandReplace); i++ ) {
		strKey = MakeCommand(i);
		switch ( i ) {
		case 0:		// FileFullPath
			if ( pExec->IsShort() )
				nReplace += strResult.Replace(strKey, lpszShortName);
			else
				nReplace += strResult.Replace(strKey, pDoc->GetPathName());
			break;
		case 1:		// FilePath
			strTmp  = szDrive;
			strTmp += szDir;
			nReplace += strResult.Replace(strKey, strTmp);
			break;
		case 2:		// FileNameNoExt
			nReplace += strResult.Replace(strKey, szFileName);
			break;
		case 3:		// FileName
			strTmp  = szFileName;
			strTmp += szExt;
			nReplace += strResult.Replace(strKey, strTmp);
			break;
		case 4:		// SelectNcLine
			CMDIChildWnd* pFrame = MDIGetActive();
			if ( pFrame->IsKindOf(RUNTIME_CLASS(CNCChild)) ) {
				CNCChild* pChild = static_cast<CNCChild *>(pFrame);
				CNCListView* pList = pChild->GetListView();
				POSITION pos = pList->GetListCtrl().GetFirstSelectedItemPosition();
				if ( pos ) {
					strTmp = boost::lexical_cast<std::string>(pList->GetListCtrl().GetNextSelectedItem(pos)+1).c_str();
					nReplace += strResult.Replace(strKey, strTmp);
					break;
				}
			}
			// else
			strTmp.Empty();
			nReplace += strResult.Replace(strKey, strTmp);
			break;
		}
	}
	if ( lpszShortName )
		delete[] lpszShortName;

#ifdef _DEBUG
	printf("CMainFrame::CommandReplace()\n");
	printf("ActiveDoc=%s\n", LPCTSTR(pDoc->GetPathName()));
	printf("ReplaceCnt=%d\n", nReplace);
#endif

	return strResult;
}

BOOL CMainFrame::CreateOutsideProcess
	(LPCTSTR lpszProcess, LPCTSTR lpszArgv, BOOL bMsg/*=TRUE*/, BOOL bWait/*=FALSE*/)
{
	CString		strProcess;
	// �N����۾�������ٸ��ð��݂ł�����
	strProcess  = "\"";
	strProcess += lpszProcess;
	strProcess += "\"";

	// �����͵�߼�ݓ�̧�ٖ��łȂ��ꍇ������̂�
	// ����������ٸ��ð��݂ł����邱�Ƃ͂ł��Ȃ�
	PROCESS_INFORMATION	pi;
	STARTUPINFO			si;
	::ZeroMemory(&si, sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	if ( ::CreateProcess(NULL, const_cast<LPTSTR>(LPCTSTR(strProcess+" "+lpszArgv)), NULL, NULL, FALSE, 
			NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi) ) {
		if ( bWait )
			::WaitForSingleObject(pi.hProcess, INFINITE);
		::CloseHandle(pi.hThread);
		::CloseHandle(pi.hProcess);
	}
	else {
#ifdef _DEBUG
		::NC_FormatMessage();
#endif
		SHELLEXECUTEINFO	shi;
		::ZeroMemory(&shi, sizeof(SHELLEXECUTEINFO));
		shi.cbSize	= sizeof(SHELLEXECUTEINFO);
		shi.fMask	= SEE_MASK_NOCLOSEPROCESS|SEE_MASK_FLAG_DDEWAIT|SEE_MASK_FLAG_NO_UI;
		shi.lpFile	= strProcess;
		shi.lpParameters	= lpszArgv;
		shi.nShow	= SW_SHOWNORMAL;
		if ( !::ShellExecuteEx(&shi) ) {
			if ( bMsg )
				AfxMessageBox(IDS_ERR_PROCESS, MB_OK|MB_ICONEXCLAMATION);
			return FALSE;
		}
		if ( bWait && shi.hProcess ) {
			::WaitForSingleObject(shi.hProcess, INFINITE);
			::CloseHandle(pi.hThread);
		}
	}

	return TRUE;
}

void CMainFrame::SetExecButtons(BOOL bRestore)
{
#ifdef _DEBUG
	printf("CMainFrame::SetExecButtons() Start\n");
#endif
	static	LPCTSTR	szMenu = "�O������(&X)";
	UINT		nMenuNCD, nMenuDXF;
	int			i, nCount, nIndex, nBtnCnt;
	CDocument*	pDoc = GetActiveFrame()->GetActiveDocument();
	CExecList*	pExeList = AfxGetNCVCApp()->GetExecList();
	CExecOption* pExec;

	nBtnCnt = m_ilEnableToolBar[TOOLIMAGE_EXEC].GetImageCount();
	for ( i=0; i<nBtnCnt; i++ )
		m_ilEnableToolBar[TOOLIMAGE_EXEC].Remove(0);

	nBtnCnt = (int)(pExeList->GetCount());
	LPCUSTTBINFO lpInfo = NULL;
	try {
		i = 0;
		lpInfo = new CUSTTBINFO[pExeList->GetCount()];
		PLIST_FOREACH(pExec, pExeList)
			nIndex = m_ilEnableToolBar[TOOLIMAGE_EXEC].Add(
							GetIconHandle(FALSE, pExec->GetFileName()) );
			pExec->SetImageNo(nIndex);
			lpInfo[i].tb.idCommand = pExec->GetMenuID();
			lpInfo[i].strInfo = pExec->GetToolTip();
			i++;
			// �����ƭ����ƭ�ID�ƲҰ�އ��̓o�^
			m_menuMain.SetMapImageID(pExec->GetMenuID(), TOOLIMAGE_EXEC, nIndex);
		END_FOREACH
		CreateDisableToolBar(TOOLIMAGE_EXEC);	// ��ڲ���݂̍쐬
		m_wndToolBar[TOOLBAR_EXEC].SetCustomButtons(
			MAKEINTRESOURCE(g_tbInfo[TOOLBAR_EXEC].nBitmap),
			&m_ilEnableToolBar[TOOLIMAGE_EXEC],
			&m_ilDisableToolBar[TOOLIMAGE_EXEC], lpInfo, bRestore );
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
	}
	if ( lpInfo )
		delete[]	lpInfo;

	// --- �ƭ��̍X�V ---
	// ���ɂ���u�O�����؁v�ƭ��̍폜(Main)
	CMenu* pMenuMain = CMenu::FromHandle(m_hMenuDefault);
	if ( pMenuMain->GetMenuItemCount() > MAINMENUCOUNT )
		pMenuMain->DeleteMenu(MAINMENUCOUNT-1, MF_BYPOSITION);

	// �V�@(NCD)
	if ( pDoc && pDoc->IsKindOf(RUNTIME_CLASS(CNCDoc)) ) {	// ��è���޷���ĂȂ�
		nCount = DOCMENUCOUNT + 4;	// �޷�������߱��� + �ŏ���,�ő剻,��������
		nMenuNCD = DOCMENUCOUNT;
	}
	else {
		nCount = DOCMENUCOUNT;
		nMenuNCD = DOCMENUCOUNT - 1;
	}
	CMenu* pMenuNCD = CMenu::FromHandle(AfxGetNCVCApp()->GetDocTemplate(TYPE_NCD)->m_hMenuShared);
	if ( pMenuNCD->GetMenuItemCount() > nCount )
		pMenuNCD->DeleteMenu(nMenuNCD, MF_BYPOSITION);

	// �V�@(DXF)
	if ( pDoc && pDoc->IsKindOf(RUNTIME_CLASS(CDXFDoc)) ) {
		nCount = DOCMENUCOUNT + 4;
		nMenuDXF = DOCMENUCOUNT;
	}
	else {
		nCount = DOCMENUCOUNT;
		nMenuDXF = DOCMENUCOUNT - 1;
	}
	CMenu* pMenuDXF = CMenu::FromHandle(AfxGetNCVCApp()->GetDocTemplate(TYPE_DXF)->m_hMenuShared);
	if ( pMenuDXF->GetMenuItemCount() > nCount )
		pMenuDXF->DeleteMenu(nMenuDXF, MF_BYPOSITION);

	// �O�����ع���ݓo�^���Ȃ����
	if ( nBtnCnt <= 0 )
		return;

	CMenu	menuMain, menuNCD, menuDXF;
	WORD	nID;
	CString	strTip;
	menuMain.CreatePopupMenu();
	menuNCD.CreatePopupMenu();
	menuDXF.CreatePopupMenu();
	PLIST_FOREACH(pExec, pExeList)
		nID = pExec->GetMenuID();
		strTip = pExec->GetToolTip();
		menuMain.AppendMenu(MF_STRING, nID, strTip);
		menuNCD.AppendMenu (MF_STRING, nID, strTip);
		menuDXF.AppendMenu (MF_STRING, nID, strTip);
	END_FOREACH
	pMenuMain->InsertMenu(MAINMENUCOUNT-1, MF_BYPOSITION|MF_POPUP, (UINT_PTR)menuMain.Detach(), szMenu);
	pMenuNCD->InsertMenu(nMenuNCD, MF_BYPOSITION|MF_POPUP, (UINT_PTR)menuNCD.Detach(), szMenu);
	pMenuDXF->InsertMenu(nMenuDXF, MF_BYPOSITION|MF_POPUP, (UINT_PTR)menuDXF.Detach(), szMenu);
	DrawMenuBar();
}

void CMainFrame::SetAddinButtons(void)
{
#ifdef _DEBUG
	printf("CMainFrame::SetAddinButtons() Start\n");
#endif
	extern	const	DWORD	g_dwAddinType[];	// NCVCaddinIF.cpp
	int		i, nIndex;
	const	CNCVCaddinArray*	pAddinArray = AfxGetNCVCApp()->GetAddinArray();
	const	CNCVCaddinWordMap*	pAddinMap = AfxGetNCVCApp()->GetAddinMap();
	CNCVCaddinIF*	pAddin;
	CNCVCaddinMap*	pMap;
	WORD			wKey;

	nIndex = m_ilAddin.GetImageCount();
	for ( i=0; i<nIndex; i++ )
		m_ilAddin.Remove(0);
	nIndex = m_ilEnableToolBar[TOOLIMAGE_ADDIN].GetImageCount();
	for ( i=0; i<nIndex; i++ )
		m_ilEnableToolBar[TOOLIMAGE_ADDIN].Remove(0);

	LPCUSTTBINFO lpInfo = NULL;
	try {
		lpInfo = new CUSTTBINFO[pAddinArray->GetSize()];
		for ( i=0; i<pAddinArray->GetSize(); i++ ) {
			pAddin = pAddinArray->GetAt(i);
			nIndex = m_ilEnableToolBar[TOOLIMAGE_ADDIN].Add(
							GetIconHandle(FALSE, pAddin->GetFileName()) );
			pAddin->SetImageNo(nIndex);
			lpInfo[i].tb.idCommand = pAddin->GetToolBarID();
			lpInfo[i].strInfo = pAddin->GetDLLName();
			//
			m_ilAddin.Add( GetIconHandle(TRUE, pAddin->GetFileName()) );
		}
		CreateDisableToolBar(TOOLIMAGE_ADDIN);	// ��ڲ���݂̍쐬
		m_wndToolBar[TOOLBAR_ADDIN].SetCustomButtons(
			MAKEINTRESOURCE(g_tbInfo[TOOLBAR_ADDIN].nBitmap),
			&m_ilEnableToolBar[TOOLIMAGE_ADDIN],
			&m_ilDisableToolBar[TOOLIMAGE_ADDIN], lpInfo );
		// ��޲݂̏ꍇ�C�P�̱�޲݂ŕ�����ID�����ꍇ������̂�
		// IDϯ�߂���Ұ�ޓo�^
		PMAP_FOREACH(wKey, pMap, pAddinMap)
			// �����ƭ����ƭ�ID�ƲҰ�އ��̓o�^
			m_menuMain.SetMapImageID(wKey, TOOLIMAGE_ADDIN,
					pMap->GetAddinIF()->GetImageNo());
		END_FOREACH
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
	}
	if ( lpInfo )
		delete[]	lpInfo;
}

void CMainFrame::RemoveCustomMenu(const CStringArray& arMenu, LPWORD pMenuID)
{
	m_menuMain.RemoveCustomImageMap(arMenu.GetSize(), pMenuID);
	m_menuMain.RemoveMenuString(arMenu);
}

void CMainFrame::CustomizedToolBar(int nTool)
{
	ASSERT( nTool>=0 && nTool<SIZEOF(m_wndToolBar) );
	m_wndToolBar[nTool].GetToolBarCtrl().Customize();
}

void CALLBACK CMainFrame::StatusBarEventTimerProc(HWND, UINT, UINT_PTR, DWORD)
{
	AfxGetApp()->OnIdle(0);
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame �f�f

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	__super::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	__super::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame ���b�Z�[�W �n���h��

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
#ifdef _DEBUG
	printf("CMainFrame::OnCreate() Start\n");
#endif
	if ( __super::OnCreate(lpCreateStruct) < 0 )
		return -1;

	CString	strRegKey, strEntry;
	int		i;

	// ���ׯ������޳
	if ( CSplashWnd::ms_bShowSplashWnd ) {
		CSplashWnd*	pSplash = new CSplashWnd;
		pSplash->CreateEx(WS_EX_WINDOWEDGE, AfxRegisterWndClass(0), NULL,
			WS_POPUP|WS_VISIBLE, 0, 0, 0, 0, m_hWnd, NULL);
		pSplash->UpdateWindow();
	}

	// G����ListCtrl��ϰ���Ұ��
	if ( !m_ilList.Create(IDB_LISTVIEW_MARK, COMMONCTRL_LISTBITMAPSIZE, 1, RGB(255, 0, 255)) ) {
		TRACE0("Failed to create m_ilList\n");
		return -1;
	}
	// ���H�w��TreeCtrl�̲Ұ��
	if ( !m_ilTree.Create(IDB_TREEVIEW_MARK, COMMONCTRL_TREEBITMAPSIZE, 1, RGB(255, 0, 255)) ) {
		TRACE0("Failed to create m_ilTree\n");
		return -1;
	}

	// ��޲݁C�O�����ع���݂̲Ұ��ؽ�
	if ( !m_ilAddin.Create(32, 32, ILC_COLOR32|ILC_MASK, 1, 10) ) {
		TRACE0("Failed to create Addin image\n");
		return -1;
	}
	for ( i=0; i<SIZEOF(m_ilEnableToolBar); i++ ) {
		if ( !m_ilEnableToolBar[i].Create(gg_nIconX, gg_nIconY/*+1*/,
					ILC_COLOR32|ILC_MASK, 1, 10) ) {
			TRACE1("Failed to create Enable image No.%d\n", i);
			return -1;
		}
		if ( !m_ilDisableToolBar[i].Create(gg_nIconX, gg_nIconY/*+1*/,
					ILC_COLOR32, 1, 10) ) {
			TRACE1("Failed to create Disable image No.%d\n", i);
			return -1;
		}
		// �����ƭ��ֲҰ��ؽ��߲����o�^
		m_menuMain.AddCustomEnableImageList(&m_ilEnableToolBar[i]);
		m_menuMain.AddCustomDisableImageList(&m_ilDisableToolBar[i]);
	}

	// Main, Trace, MakeNCD, Shape °��ް
	EnableDocking(CBRS_ALIGN_ANY);
	ASSERT(	SIZEOF(m_wndToolBar)+1 == SIZEOF(g_tbInfo) );
	for ( i=0; i<TOOLBAR_ADDIN; i++ ) {
		if ( !m_wndToolBar[i].CreateExEx(this, g_tbInfo[i].pszTitle, g_tbInfo[i].nID) ||
				!m_wndToolBar[i].LoadToolBarEx(g_tbInfo[i].nBitmap, g_tbInfo[i].lpCustTb) ) {
			TRACE1("Failed to create toolbar No.%d\n", i);
			return -1;      // �쐬�Ɏ��s
		}
		DockControlBar(&m_wndToolBar[i]);
	}
	// ��޲݁C�O�����؋N���p°��ް
	// TBSTYLE_TOOLTIPS::���I�ɕω����邽��°����߂�OnToolTipText()�ŏ���
	for ( ; i<SIZEOF(m_wndToolBar); i++ ) {
		if ( !m_wndToolBar[i].CreateExEx(this, g_tbInfo[i].pszTitle, g_tbInfo[i].nID,
					TBSTYLE_FLAT|TBSTYLE_TRANSPARENT|TBSTYLE_WRAPABLE|
						CCS_ADJUSTABLE|TBSTYLE_TOOLTIPS,
					WS_CHILD|WS_VISIBLE|
					CBRS_TOP|CBRS_GRIPPER|CBRS_SIZE_DYNAMIC) ) {
			TRACE1("Failed to create toolbar No.%d\n", i);
			return -1;      // �쐬�Ɏ��s
		}
		DockControlBar(&m_wndToolBar[i]);
	}
	// �@�B���°��ް(CCS_ADJUSTABLE:���ϲ�޽��ق͂Ȃ�)
	if ( !m_wndToolBar_Machine.CreateExEx(this, g_tbInfo[i].pszTitle, g_tbInfo[i].nID,
				TBSTYLE_FLAT|TBSTYLE_TRANSPARENT|TBSTYLE_WRAPABLE) ) {
		TRACE0("Failed to create toolbar 'Machine'\n");
		return -1;      // �쐬�Ɏ��s
	}
	DockControlBar(&m_wndToolBar_Machine);

	// Ҳ�°��ް�̉��ɕ��Ԃ悤�ɒ���
	RecalcLayout();
	CRect	rcBase, rc;
	m_wndToolBar[TOOLBAR_MAIN].GetWindowRect(&rcBase);
	for ( i=1; i<SIZEOF(m_wndToolBar); i++ ) {
		m_wndToolBar[i].GetWindowRect(&rc);
		rc.top = rcBase.top;	rc.bottom = rcBase.bottom;
		rc.OffsetRect(i, 0);
		DockControlBar(&m_wndToolBar[i], AFX_IDW_DOCKBAR_TOP, &rc);
	}
	m_wndToolBar_Machine.GetWindowRect(&rc);
	rc.top = rcBase.top;	rc.bottom = rcBase.bottom;
	rc.OffsetRect(i, 0);
	DockControlBar(&m_wndToolBar_Machine, AFX_IDW_DOCKBAR_TOP, &rc);

	// �ð���ް
	if ( !m_wndStatusBar.Create(this) ||
			!m_wndStatusBar.SetIndicators(g_nIndicators, SIZEOF(g_nIndicators)) ) {
		TRACE0("Failed to create status bar\n");
		return -1;      // �쐬�Ɏ��s
	}
	// �ð���ް:����ْ��ɽð���ް���X�V����ۂ̓K�؂Ȳ�����ْl
	m_wndStatusBar.SetTimer(IDC_STATUSBAR_EVENT, 15 * 1000,
		StatusBarEventTimerProc);

	// °��ް���C���۰��ް�̐ݒ�𕜌�
	VERIFY(strRegKey.LoadString(IDS_REGKEY_WINDOW));
	LoadBarState(strRegKey);

	// �����ƭ��̐ݒ�
	CMenu*	pOldMenu = GetMenu();
	if ( !m_menuMain.LoadToolBar(IDR_MAINFRAME) ||
		 !m_menuMain.LoadToolBar(IDR_TRACEBAR) ||
		 !m_menuMain.LoadToolBar(IDR_MAKENCD) ||
		 !m_menuMain.LoadToolBar(IDR_MACHINE) ||
		 !m_menuMain.LoadToolBar(IDR_SHAPE) ||
		 !m_menuMain.LoadMenu(IDR_MAINFRAME) ||
		 !SetMenu(&m_menuMain) ) {
		TRACE0("Failed to create custom menu\n");
		return -1;      // �쐬�Ɏ��s
	}
	DrawMenuBar();
	pOldMenu->DestroyMenu();

	// G���ޕ\��÷��̫�đ�
	ChangeViewOption();

	// GDI+ ������
	Gdiplus::GdiplusStartupInput	gdiplusInput;
	Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusInput, NULL);

	return 0;
}


void CMainFrame::OnSize(UINT nType, int cx, int cy) 
{
	__super::OnSize(nType, cx, cy);

	if ( cx<=0 || cy<=0 || !::IsWindow(m_wndStatusBar.m_hWnd) )
		return;
	m_wndStatusBar.ChangeProgressSize(PROGRESS_INDEX, cx / 3);
}

void CMainFrame::OnClose() 
{
#ifdef _DEBUG
	printf("CMainFrame::OnClose()\n");
#endif
	KillTimer(IDC_STATUSBAR_EVENT);

	SaveWindowState();
	if ( !IsIconic() ) {
		// °��ް�̐ݒ�
		CString	strRegKey;
		VERIFY(strRegKey.LoadString(IDS_REGKEY_WINDOW));
		SaveBarState(strRegKey);
	}

	__super::OnClose();
}

void CMainFrame::OnDestroy() 
{
#ifdef _DEBUG
	printf("CMainFrame::OnDestroy()\n");
#endif
	int		i;

	// �\������Ӱ��ڽ�޲�۸ނ��I��
	for ( i=0; i<SIZEOF(m_pModelessDlg); i++ ) {
		if ( m_pModelessDlg[i] )
			m_pModelessDlg[i]->DestroyWindow();
	}
	// ��׼��޼ު�Ă̍폜
	for ( i=0; i<SIZEOF(m_brushDXF[0]); i++ ) {
		if ( (HBRUSH)m_brushDXF[0][i] )
			m_brushDXF[0][i].DeleteObject();
		if ( (HBRUSH)m_brushDXF[1][i] )
			m_brushDXF[1][i].DeleteObject();
	}
	for ( i=0; i<SIZEOF(m_brushNC[0]); i++ ) {
		if ( (HBRUSH)m_brushNC[0][i] )
			m_brushNC[0][i].DeleteObject();
		if ( (HBRUSH)m_brushNC[1][i] )
			m_brushNC[1][i].DeleteObject();
	}

	// GDI+ �j��
	Gdiplus::GdiplusShutdown(gdiplusToken);

	__super::OnDestroy();
}

void CMainFrame::OnEndSession(BOOL bEnding) 
{
	if ( bEnding ) {
		SaveWindowState();
		if ( !IsIconic() ) {
			// °��ް�̐ݒ�
			CString	strRegKey;
			VERIFY(strRegKey.LoadString(IDS_REGKEY_WINDOW));
			SaveBarState(strRegKey);
		}
	}
	__super::OnEndSession(bEnding);
}

void CMainFrame::OnMenuSelect(UINT nItemID, UINT nFlags, HMENU hSysMenu)
{
  	__super::OnMenuSelect(nItemID, nFlags, hSysMenu);
  
	// �X�e�[�^�X �o�[�̍ŏ��̃y�C�������ɖ߂����ǂ����𔻒f���܂��B
	if (nFlags==0xFFFF && hSysMenu==0 && m_nStatusPane1Width!=-1) {
		m_bMenuSelect = FALSE;
		m_wndStatusBar.SetPaneInfo(0, 
				m_nStatusPane1ID, m_nStatusPane1Style, m_nStatusPane1Width);
		m_nStatusPane1Width = -1;   // �s���Ȓl�ɐݒ肵�Ă����܂��B
		// �ð���ް����۸�ڽ���۰ق��\��
		m_wndStatusBar.EnableProgress();
	}
	else
		m_bMenuSelect = TRUE;
}

void CMainFrame::OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu)
{
  	__super::OnInitMenuPopup(pPopupMenu, nIndex, bSysMenu);
  
	// �ŏ��̃y�C���̕��ƃX�^�C����ۑ����Ă����܂��B
	if (m_nStatusPane1Width == -1 && m_bMenuSelect) {
		m_wndStatusBar.GetPaneInfo(0, m_nStatusPane1ID, m_nStatusPane1Style, m_nStatusPane1Width);
		m_wndStatusBar.SetPaneInfo(0, m_nStatusPane1ID, SBPS_NOBORDERS|SBPS_STRETCH, 16384);
		// �ð���ް����۸�ڽ���۰ق��\��
		m_wndStatusBar.EnableProgress(FALSE);
	}

	// �����ƭ�
	if ( !bSysMenu )
		m_menuMain.OnInitMenuPopup(pPopupMenu);
}

LRESULT CMainFrame::OnUserFileChange(WPARAM, LPARAM)
{
	CDocument*	pDoc = GetActiveFrame()->GetActiveDocument();
	if ( !pDoc )
		return 0;

	if ( IsIconic() )
		ShowWindow(SW_RESTORE);

	// �۰�ނ���O�ɕK�v�ȏ����擾
	CNCVCApp*	pApp = AfxGetNCVCApp();
	CString		strFileName = pDoc->GetPathName();	// ̧�ٖ�
	PFNNCVCSERIALIZEFUNC pfnSerialFunc;			// �ري֐�
	BOOL	bDoc;	// TRUE:NCD, FALSE:DXF
	if ( pDoc->IsKindOf(RUNTIME_CLASS(CNCDoc)) ) {
		pfnSerialFunc = static_cast<CNCDoc *>(pDoc)->GetSerializeFunc();
		bDoc = TRUE;
	}
	else {
		pfnSerialFunc = static_cast<CDXFDoc *>(pDoc)->GetSerializeFunc();
		bDoc = FALSE;
	}
	// ���ĊJ��
	pDoc->OnCloseDocument();
	pApp->SetSerializeFunc(pfnSerialFunc);
	if ( bDoc )
		pApp->GetDocTemplate(TYPE_NCD)->OpenDocumentFile(strFileName);
	else
		pApp->GetDocTemplate(TYPE_DXF)->OpenDocumentFile(strFileName);
	pApp->SetSerializeFunc((PFNNCVCSERIALIZEFUNC)NULL);

	return 0;
}

// �����ƭ��p
void CMainFrame::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
	if ( lpDrawItemStruct->CtlType == ODT_MENU)
		m_menuMain.OnDrawItem(lpDrawItemStruct);
	else
		__super::OnDrawItem(nIDCtl, lpDrawItemStruct);
}

void CMainFrame::OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct) 
{
	if ( lpMeasureItemStruct->CtlType == ODT_MENU )
		m_menuMain.OnMeasureItem(lpMeasureItemStruct);
	else
		__super::OnMeasureItem(nIDCtl, lpMeasureItemStruct);
}

void CMainFrame::OnSysColorChange() 
{
	static	UINT	nID[] = {
		IDR_MAINFRAME, IDR_TRACEBAR, IDR_MAKENCD
	};
	__super::OnSysColorChange();
	// °��ް�Ұ�ނ̍ēǍ�
	m_menuMain.OnSysColorChange(SIZEOF(nID), nID);
}

// ��޲݁C�O�����ع���݋N��°��ް��°�����
BOOL CMainFrame::OnToolTipText(UINT nID, NMHDR* pNMHDR, LRESULT* pResult)
{
	LPTOOLTIPTEXT pTTT = reinterpret_cast<LPTOOLTIPTEXT>(pNMHDR);
	WORD	nForm = (WORD)(pNMHDR->idFrom);
#ifdef _DEBUG
	printf("OnToolTipText() Call ID=%d Flag=%d\n", nForm, pTTT->uFlags);
#endif
	if ( nForm>=EXECSTARTID && nForm<AfxGetNCVCApp()->GetMaxExecID() ) {
		const CExecOption* pOption = AfxGetNCVCApp()->GetLookupExecID(nForm);
		if ( pOption ) {
			lstrcpy(pTTT->szText, pOption->GetToolTip());
			return TRUE;
		}
	}
	if ( nForm>=ADDINSTARTID && nForm<AfxGetNCVCApp()->GetMaxAddinID() ) {
		const CNCVCaddinIF* pAddin = AfxGetNCVCApp()->GetLookupAddinID(nForm);
		if ( pAddin ) {
			lstrcpy(pTTT->szText, pAddin->GetDLLName());
			return TRUE;
		}
	}

	return __super::OnToolTipText(nID, pNMHDR, pResult);
}

// �O�����ع���݋N��°��ް�̗L�������؂�ւ�(ү����ϯ�߂͔����Z)
void CMainFrame::OnUpdateExecButtonCheck(CCmdUI* pCmdUI)
{
	if ( !(pCmdUI->m_nID>=EXECSTARTID && pCmdUI->m_nID<AfxGetNCVCApp()->GetMaxExecID()) ) {
		pCmdUI->ContinueRouting();
		return;
	}
	const CExecOption* pOption = AfxGetNCVCApp()->GetLookupExecID(pCmdUI->m_nID);
	if ( !pOption )
		return;

	BOOL	bEnable = FALSE;
	BOOL	bNC  = pOption->IsNCType();
	BOOL	bDXF = pOption->IsDXFType();
	CDocument*	pDoc = GetActiveFrame()->GetActiveDocument();
	if ( pDoc ) {
		if ( bNC  && pDoc->IsKindOf(RUNTIME_CLASS(CNCDoc)) )
			bEnable = TRUE;
		else if ( bDXF && pDoc->IsKindOf(RUNTIME_CLASS(CDXFDoc)) )
			bEnable = TRUE;
	}
	else {
		if ( !bNC && !bDXF )
			bEnable = TRUE;
	}
	pCmdUI->Enable(bEnable);
}

void CMainFrame::OnUpdateAddinButtonCheck(CCmdUI* pCmdUI)
{
	if ( !(pCmdUI->m_nID>=ADDINSTARTID && pCmdUI->m_nID<AfxGetNCVCApp()->GetMaxAddinID()) ) {
		pCmdUI->ContinueRouting();
		return;
	}
	const CNCVCaddinIF* pAddin = AfxGetNCVCApp()->GetLookupAddinID(pCmdUI->m_nID);
	if ( !pAddin )
		return;

	BOOL	bEnable = FALSE;
	DWORD	dwType = pAddin->GetAddinType();
	CDocument*	pDoc = GetActiveFrame()->GetActiveDocument();
	if ( pDoc ) {
		if ( dwType & 0x00f0 && pDoc->IsKindOf(RUNTIME_CLASS(CNCDoc)) )
			bEnable = TRUE;
		else if ( dwType & 0x0f00 && pDoc->IsKindOf(RUNTIME_CLASS(CDXFDoc)) )
			bEnable = TRUE;
	}
	else {
		if ( dwType & 0x000f )
			bEnable = TRUE;
	}
	pCmdUI->Enable(bEnable);
}

void CMainFrame::OnUpdateDate(CCmdUI* pCmdUI)
{
	// ���݂̓��t���擾���A�����ɂ��������ĕϊ����܂��B
	CTime time = CTime::GetCurrentTime();
	CString	strFormat;
	VERIFY(strFormat.LoadString(ID_INDICATOR_DATE_F));	// %m/%d
	CString strDate = time.Format(strFormat);
	// ���݂̓��t�𔽉f����悤�Ƀy�C�����X�V���܂��B
	m_wndStatusBar.SetPaneText(
		m_wndStatusBar.CommandToIndex(ID_INDICATOR_DATE), strDate);

	pCmdUI->Enable(TRUE);
}

void CMainFrame::OnUpdateTime(CCmdUI* pCmdUI)
{
	// ���݂̎������擾���A�����ɂ��������ĕϊ����܂��B
	CTime time = CTime::GetCurrentTime();
	CString	strFormat;
	VERIFY(strFormat.LoadString(ID_INDICATOR_TIME_F));	// %H:%M
	CString strTime = time.Format(strFormat);
	// ���݂̎����𔽉f����悤�Ƀy�C�����X�V���܂��B
	m_wndStatusBar.SetPaneText(
		m_wndStatusBar.CommandToIndex(ID_INDICATOR_TIME), strTime);

	pCmdUI->Enable(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame ���b�Z�[�W �n���h���i���j���[�ҁj

void CMainFrame::OnNextPane() 
{
	MDINext();
}

void CMainFrame::OnUpdateEditCut(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(FALSE);
}

void CMainFrame::OnUpdateEditPaste(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(FALSE);
}

void CMainFrame::OnViewToolbarCustom() 
{
	int				i;

	// ���݂̕\���󋵂��擾
	DWORD	dwViewFlg = 0;
	for ( i=0; i<SIZEOF(g_tbInfo); i++ )
		dwViewFlg |= ((GetControlBar(g_tbInfo[i].nID)->GetStyle() & WS_VISIBLE) ? 1 : 0) << i;

	// °��ް�ݒ��޲�۸�
	CToolBarSetupDlg	dlg(dwViewFlg);
	if ( dlg.DoModal() == IDOK ) {
		for ( i=0; i<SIZEOF(g_tbInfo); i++ )
			ShowControlBar(GetControlBar(g_tbInfo[i].nID), dlg.m_bToolBar[i], FALSE);
	}
}

void CMainFrame::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	// TODO: ���̈ʒu�Ƀ��b�Z�[�W �n���h���p�̃R�[�h��ǉ����Ă�������
	ASSERT( pWnd );
	if ( pWnd->IsKindOf(RUNTIME_CLASS(CControlBar)) )
		OnViewToolbarCustom();
}

//////////////////////////////////////////////////////////////////////
// CMachineToolBar �N���X
//////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CMachineToolBar, CCustomToolBar)
	ON_WM_CREATE()
	ON_CBN_SELCHANGE(ID_OPTION_MCCOMBO, &CMachineToolBar::OnSelchangeMC)
END_MESSAGE_MAP()

int CMachineToolBar::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
#ifdef _DEBUG
	printf("CMachineToolBar::OnCreate() Start\n");
#endif

	if ( CCustomToolBar::OnCreate(lpCreateStruct) < 0 ) {
		TRACE0("Failed to CCustomToolBar::OnCreate (CMachineToolBar)\n");
		return -1;
	}
	if ( !LoadToolBarEx(g_tbInfo[TOOLBAR_MACHINE].nBitmap, g_tbInfo[TOOLBAR_MACHINE].lpCustTb) ) {
		TRACE0("Failed to LoadToolBarEx() (CMachineToolBar)\n");
		return -1;
	}

	// �����ޯ���̑傫��������
	CFont*	pFont = CFont::FromHandle((HFONT)::GetStockObject(DEFAULT_GUI_FONT));
	CClientDC dc(this);
	CFont*	pOldFont = dc.SelectObject(pFont);
	CSize	size( dc.GetTextExtent( CString('W', 20) ) );	// 20������
	dc.SelectObject(pOldFont);

	SetButtonInfo(1, ID_OPTION_MCCOMBO, TBBS_SEPARATOR, size.cx);
	CRect	rc;
	GetItemRect(1, &rc);
	rc.bottom = rc.top + size.cy * 20;	// 10�s��

	// �����ޯ���̍쐬
	if ( !m_ctMachine.Create(WS_CHILD|WS_VISIBLE|WS_VSCROLL|CBS_DROPDOWNLIST,
			rc, this, ID_OPTION_MCCOMBO) )
		return -1;
	m_ctMachine.SetFont( pFont );

	// �@�B��񗚗���ǉ�
	ChangeMachine();

	return 0;
}

void CMachineToolBar::OnSelchangeMC()
{
#ifdef _DEBUG
	printf("CMachineToolBar::OnSelchangeMC() Start\n");
#endif
	int	nIndex = m_ctMachine.GetCurSel();
	if ( nIndex >= 0 ) {
		// �@�B���̧�ق̓ǂݍ���
		if ( AfxGetNCVCApp()->ChangeMachine(nIndex) ) {
			// ��߼�����ۯ�����ߍX�V
			if ( AfxGetNCVCMainWnd()->GetModelessDlg(MLD_NCOBS) )
				AfxGetNCVCMainWnd()->GetModelessDlg(MLD_NCOBS)->PostMessage(WM_USERFILECHANGENOTIFY);
		}
		else
			m_ctMachine.SetCurSel( m_ctMachine.GetCount() > 1 ? 0 : -1 );
	}

	// �I����`���c��̂�h��
	GetParentFrame()->SetFocus();
}

void CMachineToolBar::ChangeMachine(void)
{
#ifdef _DEBUG
	printf("CMachineToolBar::ChangeMachine() Start\n");
#endif
	// �����ޯ���ɋ@�B���̗�����ǉ�
	AfxGetNCVCApp()->GetMCOption()->AddMCHistory_ComboBox(m_ctMachine);
}
