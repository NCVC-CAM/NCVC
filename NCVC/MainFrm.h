// MainFrm.h : CMainFrame �N���X�̃C���^�[�t�F�C�X
//

#pragma once

#include "NCVCdefine.h"
#include "ExecOption.h"
#include "MainStatusBar.h"
#include "CustomMenu.h"
#include "CustomToolBar.h"

/////////////////////////////////////////////////////////////////////////////
// CMachineToolBar: �@�B���p°��ް

class CMachineToolBar : public CCustomToolBar  
{
	CComboBox		m_ctMachine;	// �@�B�������ޯ��

// ���ڰ���
public:
	void	ChangeMachine(void);

// ү����ϯ�ߊ֐�
protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSelchangeMC();

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

// Ӱ��ڽ�޲�۸ފǗ��p���ʎq
// NCView NC���ވړ�, NCView ܰ���`�޲�۸�, DXFView ڲԕ\���؂�ւ��޲�۸�
// NCView ����
enum	EN_MLD	{
	MLD_NCJUMP=0, MLD_NCWORK, MLD_DXFLAYER, MLD_NCFIND,
		MLD_NUMS	// [4]
};

// °��ް
enum	EN_TOOLBAR {
	TOOLBAR_MAIN=0, TOOLBAR_TRACE, TOOLBAR_MAKENCD, TOOLBAR_SHAPE,
	TOOLBAR_ADDIN, TOOLBAR_EXEC,
	TOOLBAR_MACHINE
};
// °��ް�Ұ��
enum	EN_TOOLBARIMAGE {
	TOOLIMAGE_ADDIN=0, TOOLIMAGE_EXEC
};

// ���
enum	EN_COMPEN	{
	COMPEN_RECT=0, COMPEN_SEL,
		COMPEN_NUMS		// [2]
};
enum	EN_NCPEN	{
	NCPEN_G0=0, NCPEN_G1, NCPEN_G1Z,
	NCPEN_CORRECT,
	NCPEN_CYCLE,
	NCPEN_WORK, NCPEN_MAXCUT,
		NCPEN_NUMS		// [7]
};
enum	EN_DXFPEN	{
	DXFPEN_ORIGIN=0, DXFPEN_CUTTER,
	DXFPEN_START, DXFPEN_MOVE,
	DXFPEN_OUTLINE,
	DXFPEN_WORK,
		DXFPEN_NUMS		// [6]
};
// ��׼
enum	EN_DXFBRUSH	{
	DXFBRUSH_CUTTER=0, DXFBRUSH_START, DXFBRUSH_MOVE, DXFBRUSH_TEXT,
		DXFBRUSH_NUMS	// [4]
};
enum	EN_NCBRUSH	{
	NCBRUSH_CYCLEXY,
		NCBRUSH_NUMS	// [1]
};

/////////////////////////////////////////////////////////////////////////////

class CMainFrame : public CMDIFrameWnd
{
	// GDI+ (NCVC.cpp����ړ�)
	ULONG_PTR	gdiplusToken;

	void	SaveWindowState(void);	// OnClose, OnEndSession ����̌Ăяo��

	// NCVC���ʑ���
	// CPen, CBrush ��[2]�͸د���ް�ނւ̕`��p�D��->���ϊ�
	CPen		m_penOrg[2][NCXYZ],	// ���S��
				m_penCom[2][COMPEN_NUMS],	// �g��k����`, �I��
				m_penNC[2][NCPEN_NUMS],		// G0, G1, G1Z, �␳, Cycle, Work, MaxCut
				m_penDXF[2][DXFPEN_NUMS];	// Origin, Cutter, Start, Move, Outline, Work
	CBrush		m_brushDXF[2][DXFBRUSH_NUMS],	// Text Point Brush
				m_brushNC[2][NCBRUSH_NUMS];		// Cycle(XY)
	CFont		m_cfText[2];	// G���ޕ\��, DXF÷��̫��
	int			m_nTextHeight,	// �@�@�V�@�̍���
				m_nTextWidth,	// �@�@�V�@�̕�
				m_nSelectGDI;	// 0:�ި���ڲ�p, 1:�د���ް�ޗp
	CImageList	m_ilList,		// G����ListCtrl��ϰ���Ұ��
				m_ilTree;		// ���H�w��TreeCtrl�̲Ұ��

	// ���۰��ް ����
	CMainStatusBar	m_wndStatusBar;
	CCustomToolBar	m_wndToolBar[6];	// Main, Trace, MakeNCD, Shape, Addin, Exec
	CMachineToolBar	m_wndToolBar_Machine;	// �@�B���°��ް

	// �����ƭ�(Office97ײ��ƭ�)
	CCustomMenuEx	m_menuMain;

	// �O�����ع���݂Ʊ�޲݂̱���(���ݲҰ��)
	HICON			m_hDefIconSmall,	// °��ް����̫�ı���
					m_hDefIconLarge;
	CImageList		m_ilAddin,				// ��޲ݏ��p
					m_ilEnableToolBar[2],	// °��ް�p���ݲҰ��
					m_ilDisableToolBar[2];
	void	CreateDisableToolBar(EN_TOOLBARIMAGE);	// ��ڲ�Ұ�ލ쐬
	CString	CommandReplace(const CExecOption*, const CDocument*);	// �O�����ع���݋N���p

	// CG: �u�X�e�[�^�X �o�[�v�R���|�[�l���g�ɂ��ǉ�����Ă��܂��B
	UINT		m_nStatusPane1ID;
	UINT		m_nStatusPane1Style;
	INT			m_nStatusPane1Width;
	BOOL		m_bMenuSelect;

	// Ӱ��ڽ�޲�۸ފǗ��p
	CDialog*	m_pModelessDlg[MLD_NUMS];

	DECLARE_DYNAMIC(CMainFrame)
public:
	CMainFrame();

// ����
public:
	CPen*		GetPenOrg(size_t a) {
		ASSERT(NCA_X<=a && a<=NCA_Z);
		return &m_penOrg[m_nSelectGDI][a];
	}
	CPen*		GetPenCom(EN_COMPEN a) {
		return &m_penCom[m_nSelectGDI][a];
	}
	CPen*		GetPenNC(EN_NCPEN a) {
		return &m_penNC[m_nSelectGDI][a];
	}
	CPen*		GetPenDXF(EN_DXFPEN a) {
		return &m_penDXF[m_nSelectGDI][a];
	}
	CBrush*		GetBrushNC(EN_NCBRUSH a) {
		return &m_brushNC[m_nSelectGDI][a];
	}
	CBrush*		GetBrushDXF(EN_DXFBRUSH a) {
		return &m_brushDXF[m_nSelectGDI][a];
	}
	CFont*	GetTextFont(DOCTYPE enType) {		// ���ݑI����̫��
		return &m_cfText[enType];
	}
	int		GetNCTextHeight(void) {		// �����̍���(tmHeight+tmExternalLeading)
		return m_nTextHeight;
	}
	int		GetNCTextWidth(void) {		// �����̕�(tmMaxCharWidth)
		return m_nTextWidth;
	}

	HICON		GetIconHandle(BOOL, LPCTSTR);	// �O��̧�ٖ��̱��ݎ擾
	CImageList*	GetListImage(void) {
		return &m_ilList;
	}
	CImageList*	GetTreeImage(void) {
		return &m_ilTree;
	}
	CImageList*	GetAddinImage(void) {
		return &m_ilAddin;
	}
	CImageList*	GetEnableToolBarImage(EN_TOOLBARIMAGE enImage) {
		return &(m_ilEnableToolBar[enImage]);
	}
	CImageList*	GetDisableToolBarImage(EN_TOOLBARIMAGE enImage) {
		return &(m_ilDisableToolBar[enImage]);
	}
	CProgressCtrl*	GetProgressCtrl(void) {
		return m_wndStatusBar.GetProgressCtrl();
	}
	CDialog*	GetModelessDlg(EN_MLD n) {
		return m_pModelessDlg[n];
	}
	void		SetModelessDlg(EN_MLD n, CDialog* pDlg) {
		m_pModelessDlg[n] = pDlg;
	}

// ����
public:
	BOOL	RestoreWindowState(void);	// CNCVCApp::InitInstance()����̌Ăяo��
	void	ChangeViewOption(void);
	void	ChangeMachine(void) {
		m_wndToolBar_Machine.ChangeMachine();
	}
	void	AllModelessDlg_PostSwitchMessage(void);
	CString	MakeCommand(int);	// g_szCommandReplace[] �� ${hogehoge} �ɕϊ�(ExecOption.cpp)
	void	SetExecButtons(BOOL bRestore = TRUE);
	void	SetAddinButtons(void);
	void	RemoveCustomMenu(const CStringArray&, LPWORD);	// from CNCVCApp::OnOptionExec()
	BOOL	CreateOutsideProcess(LPCTSTR, LPCTSTR, BOOL = TRUE, BOOL = FALSE);	// �O����۾��̋N��
	void	CustomizedToolBar(int);

	// Ӱ����޲�۸��ޯ���\�����ł�
	// ����ُ������������s�����邽�߂�
	// WM_TIMER CALLBACK�֐�
	static void CALLBACK StatusBarEventTimerProc(HWND, UINT, UINT, DWORD);

	// GDI��޼ު�Ă̐ؑ�
	void	SelectGDI(BOOL bSelect = TRUE) {
		m_nSelectGDI = bSelect ? 0 : 1;
	}

// �I�[�o�[���C�h
protected:
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);

// ����
public:
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// �������ꂽ�A���b�Z�[�W���蓖�Ċ֐�
protected:
	afx_msg void OnUpdateDate(CCmdUI* pCmdUI);
	afx_msg void OnUpdateTime(CCmdUI* pCmdUI);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnClose();
	afx_msg void OnEndSession(BOOL bEnding);
	afx_msg void OnDestroy();
	afx_msg void OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu);
	afx_msg void OnMenuSelect(UINT nItemID, UINT nFlags, HMENU hSysMenu);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg void OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct);
	afx_msg void OnSysColorChange();
	afx_msg void OnNextPane();
	afx_msg void OnUpdateEditCut(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditPaste(CCmdUI* pCmdUI);
	afx_msg void OnViewToolbarCustom();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	// ̧�ٍēǍ��ʒm(from ChildBase.cpp)
	afx_msg LRESULT	OnUserFileChange(WPARAM, LPARAM);
	// ��޲݁C�O�����ع���݋N��°��ް�p
	afx_msg BOOL	OnToolTipText(UINT nID, NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void	OnUpdateExecButtonCheck(CCmdUI* pCmdUI);
	afx_msg void	OnUpdateAddinButtonCheck(CCmdUI* pCmdUI);

	DECLARE_MESSAGE_MAP()
};


