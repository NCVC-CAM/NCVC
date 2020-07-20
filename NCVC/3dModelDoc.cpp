// 3dModelDoc.cpp : 実装ファイル
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
// C3dModelDoc 診断


/////////////////////////////////////////////////////////////////////////////
// C3dModelDoc シリアル化

void C3dModelDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring()) return;	// 保存は今のところナシ
}

/////////////////////////////////////////////////////////////////////////////
// C3dModelDoc コマンド

BOOL C3dModelDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
	if (!__super::OnOpenDocument(lpszPathName))
		return FALSE;

	// ﾄﾞｷｭﾒﾝﾄ変更通知ｽﾚｯﾄﾞの生成
	OnOpenDocumentBase(lpszPathName);	// CDocBase

	return TRUE;
}

void C3dModelDoc::OnCloseDocument() 
{
	// 処理中のｽﾚｯﾄﾞを中断させる
	OnCloseDocumentBase();		// ﾌｧｲﾙ変更通知ｽﾚｯﾄﾞ

	__super::OnCloseDocument();
}
