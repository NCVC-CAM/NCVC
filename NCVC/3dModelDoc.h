// 3dModelDoc.h : ヘッダー ファイル
//

#pragma once

#include "DocBase.h"

/////////////////////////////////////////////////////////////////////////////
// C3dModelDoc ドキュメント

class C3dModelDoc : public CDocBase
{

public:
	C3dModelDoc();
	virtual ~C3dModelDoc();

	virtual void Serialize(CArchive& ar);   // ドキュメント I/O に対してオーバーライドされました。
	virtual BOOL OnNewDocument();

protected:

	DECLARE_DYNCREATE(C3dModelDoc)
	DECLARE_MESSAGE_MAP()
};
