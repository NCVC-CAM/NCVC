// NCVCDoc.cpp : CNCVCDoc �N���X�̎���
//

#include "stdafx.h"
#include "NCVC.h"

#include "NCVCDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CNCVCDoc

IMPLEMENT_DYNCREATE(CNCVCDoc, CDocument)

BEGIN_MESSAGE_MAP(CNCVCDoc, CDocument)
END_MESSAGE_MAP()


// CNCVCDoc �R���X�g���N�V����/�f�X�g���N�V����

CNCVCDoc::CNCVCDoc()
{
	// TODO: ���̈ʒu�� 1 �x�����Ă΂��\�z�p�̃R�[�h��ǉ����Ă��������B

}

CNCVCDoc::~CNCVCDoc()
{
}

BOOL CNCVCDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: ���̈ʒu�ɍď�����������ǉ����Ă��������B
	// (SDI �h�L�������g�͂��̃h�L�������g���ė��p���܂��B)

	return TRUE;
}




// CNCVCDoc �V���A����

void CNCVCDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: �i�[����R�[�h�������ɒǉ����Ă��������B
	}
	else
	{
		// TODO: �ǂݍ��ރR�[�h�������ɒǉ����Ă��������B
	}
}


// CNCVCDoc �f�f

#ifdef _DEBUG
void CNCVCDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CNCVCDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


// CNCVCDoc �R�}���h
