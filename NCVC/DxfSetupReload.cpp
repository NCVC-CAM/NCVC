// DxfSetupReload.cpp : �C���v�������e�[�V���� �t�@�C��
//

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "DXFDoc.h"
#include "DxfSetupReload.h"

#ifdef _DEBUG
#define new DEBUG_NEW
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
	__super::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDxfSetupReload)
		// ���� - ClassWizard �͂��̈ʒu�Ƀ}�b�s���O�p�̃}�N����ǉ��܂��͍폜���܂��B
	//}}AFX_DATA_MAP
}

/////////////////////////////////////////////////////////////////////////////
// CDxfSetupReload ���b�Z�[�W �n���h��

BOOL CDxfSetupReload::OnInitDialog() 
{
	__super::OnInitDialog();

	// �W��ؽ��ޯ�����޸׽��
	m_ctReloadList.SubclassDlgItem(IDC_DXF_RELOADLIST, this);

	int			n;
	CDXFDoc*	pDoc;
	CWnd*		pStatic = GetDlgItem(IDC_DXF_RELOADLIST_ST);
	CString		strPath;
	ASSERT( pStatic );

	m_ctReloadList.SendMessage(WM_SETREDRAW, FALSE);
	for ( POSITION pos=AfxGetNCVCApp()->GetDocTemplate(TYPE_DXF)->GetFirstDocPosition(); pos; ) {
		pDoc = static_cast<CDXFDoc*>(AfxGetNCVCApp()->GetDocTemplate(TYPE_DXF)->GetNextDoc(pos));
		// ��U��è�����۰قɾ�Ă����߽�\���̍œK��(shlwapi.h)
		strPath = pDoc->GetPathName();
		if ( !strPath.IsEmpty() ) {
			::PathSetDlgItemPath(m_hWnd, IDC_DXF_RELOADLIST_ST, strPath);
			// ����÷�Ă��擾����ؽĺ��۰قɾ��
			pStatic->GetWindowText(strPath);
			n = m_ctReloadList.AddString(strPath);
			m_ctReloadList.SetCheck(n, pDoc->IsDocFlag(DXFDOC_RELOAD));
			m_ctReloadList.SetItemDataPtr(n, pDoc);
		}
	}
	m_ctReloadList.SendMessage(WM_SETREDRAW, TRUE);
	m_ctReloadList.SetCurSel(0);

	return TRUE;  // �R���g���[���Ƀt�H�[�J�X��ݒ肵�Ȃ��Ƃ��A�߂�l�� TRUE �ƂȂ�܂�
	              // ��O: OCX �v���p�e�B �y�[�W�̖߂�l�� FALSE �ƂȂ�܂�
}

void CDxfSetupReload::OnOK() 
{
	CDXFDoc*	pDoc;

	for ( int i=0; i<m_ctReloadList.GetCount(); i++ ) {
		pDoc = reinterpret_cast<CDXFDoc*>(m_ctReloadList.GetItemDataPtr(i));
		pDoc->SetDocFlag(DXFDOC_RELOAD, m_ctReloadList.GetCheck(i)==BST_CHECKED ? TRUE : FALSE);
	}

//	__super::OnOK();
	EndDialog(IDOK);
}
