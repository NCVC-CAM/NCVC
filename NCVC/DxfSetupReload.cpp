// DxfSetupReload.cpp : �C���v�������e�[�V���� �t�@�C��
//

#include "stdafx.h"
#include "NCVC.h"
#include "DXFDoc.h"
#include "DxfSetupReload.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

BEGIN_MESSAGE_MAP(CDxfSetupReload, CDialog)
	//{{AFX_MSG_MAP(CDxfSetupReload)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDxfSetupReload �_�C�A���O

CDxfSetupReload::CDxfSetupReload(CWnd* pParent)
	: CDialog(CDxfSetupReload::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDxfSetupReload)
		// ���� - ClassWizard �͂��̈ʒu�Ƀ}�b�s���O�p�̃}�N����ǉ��܂��͍폜���܂��B
	//}}AFX_DATA_INIT
}

void CDxfSetupReload::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDxfSetupReload)
		// ���� - ClassWizard �͂��̈ʒu�Ƀ}�b�s���O�p�̃}�N����ǉ��܂��͍폜���܂��B
	//}}AFX_DATA_MAP
}

/////////////////////////////////////////////////////////////////////////////
// CDxfSetupReload ���b�Z�[�W �n���h��

BOOL CDxfSetupReload::OnInitDialog() 
{
	CDialog::OnInitDialog();

	// �W��ؽ��ޯ�����޸׽��
	m_ctReloadList.SubclassDlgItem(IDC_DXF_RELOADLIST, this);

	CDXFDoc*	pDoc;
	CWnd*		pStatic = GetDlgItem(IDC_DXF_RELOADLIST_ST);
	CString		strPath;
	ASSERT( pStatic );

	m_ctReloadList.SendMessage(WM_SETREDRAW, FALSE);
	for ( POSITION pos=AfxGetNCVCApp()->GetDocTemplate(TYPE_DXF)->GetFirstDocPosition(); pos; ) {
		pDoc = (CDXFDoc *)(AfxGetNCVCApp()->GetDocTemplate(TYPE_DXF)->GetNextDoc(pos));
		// ��U��è�����۰قɾ�Ă����߽�\���̍œK��(shlwapi.h)
		::PathSetDlgItemPath(m_hWnd, IDC_DXF_RELOADLIST_ST, pDoc->GetPathName());
		// ����÷�Ă��擾����ؽĺ��۰قɾ��
		pStatic->GetWindowText(strPath);
		m_ctReloadList.SetCheck(m_ctReloadList.AddString(strPath), pDoc->IsReload());
	}
	m_ctReloadList.SendMessage(WM_SETREDRAW, TRUE);
	m_ctReloadList.SetCurSel(0);

	return TRUE;  // �R���g���[���Ƀt�H�[�J�X��ݒ肵�Ȃ��Ƃ��A�߂�l�� TRUE �ƂȂ�܂�
	              // ��O: OCX �v���p�e�B �y�[�W�̖߂�l�� FALSE �ƂȂ�܂�
}

void CDxfSetupReload::OnOK() 
{
	int		nCnt = 0;
	CDXFDoc*	pDoc;
	for ( POSITION pos=AfxGetNCVCApp()->GetDocTemplate(TYPE_DXF)->GetFirstDocPosition(); pos; ) {
		pDoc = (CDXFDoc *)(AfxGetNCVCApp()->GetDocTemplate(TYPE_DXF)->GetNextDoc(pos));
		pDoc->SetReload( m_ctReloadList.GetCheck(nCnt++)==1 ? TRUE : FALSE );
	}

	CDialog::OnOK();
}
