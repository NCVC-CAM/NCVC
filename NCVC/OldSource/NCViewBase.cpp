// NCViewBase.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "NCDoc.h"
#include "NCViewBase.h"
#include "NCViewTab.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNCViewBase クラスのメッセージ ハンドラ

void CNCViewBase::OnInitialUpdate(int nTab)
{
	ASSERT(m_pView);	// ViewBase.h
	CNCDoc*	pDoc = static_cast<CNCDoc *>(m_pView->GetDocument());

	if ( pDoc->IsDocFlag(NCDOC_LATHE) ) {
		// 親ﾀﾌﾞの名称を変更
		CWnd*	pParent = m_pView->GetParent();
		if ( pParent->IsKindOf(RUNTIME_CLASS(CNCViewTab)) )
			static_cast<CNCViewTab *>(pParent)->SetPageTitle(nTab, m_strGuide);
		// (Lathe Mode)文字列を追加
		CString	strLathe;
		VERIFY(strLathe.LoadString(IDCV_LATHE));
		m_strGuide += strLathe;
	}
	else if ( pDoc->IsDocFlag(NCDOC_WIRE) ) {
		// (Wire Mode)文字列を追加
		CString	strLathe;
		VERIFY(strLathe.LoadString(IDCV_WIRE));
		m_strGuide += strLathe;
	}
}
