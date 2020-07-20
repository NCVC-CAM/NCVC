// 3dModelDoc.cpp : �����t�@�C��
//

#include "stdafx.h"
#include "NCVC.h"
#include "3dModelDoc.h"
#include "Kodatuno/IGES_Parser.h"
#include "Kodatuno/STL_Parser.h"
#undef PI	// Use NCVC (MyTemplate.h)

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(C3dModelDoc, CDocument)

BEGIN_MESSAGE_MAP(C3dModelDoc, CDocument)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// C3dModelDoc

C3dModelDoc::C3dModelDoc()
{
	m_kBody  = NULL;
	m_kbList = NULL;
}

C3dModelDoc::~C3dModelDoc()
{
	if ( m_kBody ) {
		m_kBody->DelBodyElem();
		delete	m_kBody;
	}
	if ( m_kbList ) {
		m_kbList->clear();
		delete	m_kbList;
	}
}

/////////////////////////////////////////////////////////////////////////////
// C3dModelDoc �f�f


/////////////////////////////////////////////////////////////////////////////
// C3dModelDoc �V���A����

void C3dModelDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring()) return;	// �ۑ��͍��̂Ƃ���i�V

	const CFile* fp = ar.GetFile();
	CString	strPath( fp->GetFilePath() ), strExt;
	TCHAR	szFileName[_MAX_FNAME],
			szExt[_MAX_EXT];

	_tsplitpath_s(strPath, NULL, 0, NULL, 0,
		szFileName, SIZEOF(szFileName), szExt, SIZEOF(szExt));
	if ( lstrlen(szFileName)<=0 || lstrlen(szExt)<=0 )
		return;
	strExt = szExt + 1;		// �h�b�g������

	m_kBody = new BODY;

	// �g���q�Ŕ���
	int	nResult = KOD_FALSE;
	if ( strExt.CompareNoCase("igs")==0 || strExt.CompareNoCase("iges")==0 ) {
		IGES_PARSER	iges;
		if ( (nResult=iges.IGES_Parser_Main(m_kBody, strPath)) == KOD_TRUE )
			iges.Optimize4OpenGL(m_kBody);
	}
	else if ( strExt.CompareNoCase("stl") == 0 ) {
		STL_PARSER	stl;
		nResult = stl.STL_Parser_Main(m_kBody, strPath);
	}
	if ( nResult != KOD_TRUE ) {
		delete	m_kBody;
		m_kBody = NULL;
		return;
	}

	// Kodatuno BODY �o�^
	if ( !m_kbList )
		m_kbList = new BODYList;
	m_kBody->RegistBody(m_kbList, strPath);
}

/////////////////////////////////////////////////////////////////////////////
// C3dModelDoc �R�}���h

BOOL C3dModelDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
	if (!__super::OnOpenDocument(lpszPathName))
		return FALSE;

	// �޷���ĕύX�ʒm�گ�ނ̐���
	OnOpenDocumentBase(lpszPathName);	// CDocBase

	return TRUE;
}

void C3dModelDoc::OnCloseDocument() 
{
	// �������̽گ�ނ𒆒f������
	OnCloseDocumentBase();		// ̧�ٕύX�ʒm�گ��

	__super::OnCloseDocument();
}
