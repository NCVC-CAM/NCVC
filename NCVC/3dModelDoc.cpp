// 3dModelDoc.cpp : �����t�@�C��
//

#include "stdafx.h"
#include "NCVC.h"
#include "3dModelDoc.h"

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
	m_pKoBody  = NULL;
	m_pKoList = NULL;
	m_rcMax.SetRectMinimum();
}

C3dModelDoc::~C3dModelDoc()
{
	if ( m_pKoBody ) {
		m_pKoBody->DelBodyElem();
		delete	m_pKoBody;
	}
	if ( m_pKoList ) {
		m_pKoList->clear();
		delete	m_pKoList;
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
	CString	strPath( fp->GetFilePath() );

	// �R�c���f���̓ǂݍ���
	m_pKoBody = Read3dModel(strPath);
	if ( !m_pKoBody ) {
		return;
	}
	// Kodatuno BODY �o�^
	m_pKoList = new BODYList;
	m_pKoBody->RegistBody(m_pKoList, strPath);
}

/////////////////////////////////////////////////////////////////////////////
// C3dModelDoc �R�}���h

BOOL C3dModelDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
	if (!__super::OnOpenDocument(lpszPathName))
		return FALSE;

	// �޷���ĕύX�ʒm�گ�ނ̐���
	OnOpenDocumentBase(lpszPathName);	// CDocBase

	// ��L��`�̎擾
	BODY*		pBody;
	CPoint3D	pt;
	int		i, nLoop = m_pKoList->getNum();

	for ( i=0; i<nLoop; i++ ) {
		pBody = (BODY *)m_pKoList->getData(i);
		// ���C�u����������������
		pt = pBody->minmaxCoord[0];
		m_rcMax |= pt;
		pt = pBody->minmaxCoord[1];
		m_rcMax |= pt;
	}

	return TRUE;
}

void C3dModelDoc::OnCloseDocument() 
{
	// �������̽گ�ނ𒆒f������
	OnCloseDocumentBase();		// ̧�ٕύX�ʒm�گ��

	__super::OnCloseDocument();
}
