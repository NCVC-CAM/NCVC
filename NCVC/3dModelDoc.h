// 3dModelDoc.h : ヘッダー ファイル
//

#pragma once

#include "DocBase.h"
#include "Kodatuno/BODY.h"
#undef PI	// Use NCVC (MyTemplate.h)

/////////////////////////////////////////////////////////////////////////////
// C3dModelDoc ドキュメント

class C3dModelDoc : public CDocBase
{
	BODY*		m_pKoBody;		// Kodatuno Body
	BODYList*	m_pKoList;		// Kodatuno Body List

public:
	C3dModelDoc();
	virtual ~C3dModelDoc();
	DECLARE_DYNCREATE(C3dModelDoc)

	virtual void Serialize(CArchive& ar);   // ドキュメント I/O に対してオーバーライドされました。
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	virtual void OnCloseDocument();

protected:

	DECLARE_MESSAGE_MAP()
};
