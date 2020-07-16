// NCVCDoc.cpp : CNCVCDoc クラスの実装
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


// CNCVCDoc コンストラクション/デストラクション

CNCVCDoc::CNCVCDoc()
{
	// TODO: この位置に 1 度だけ呼ばれる構築用のコードを追加してください。

}

CNCVCDoc::~CNCVCDoc()
{
}

BOOL CNCVCDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: この位置に再初期化処理を追加してください。
	// (SDI ドキュメントはこのドキュメントを再利用します。)

	return TRUE;
}




// CNCVCDoc シリアル化

void CNCVCDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: 格納するコードをここに追加してください。
	}
	else
	{
		// TODO: 読み込むコードをここに追加してください。
	}
}


// CNCVCDoc 診断

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


// CNCVCDoc コマンド
