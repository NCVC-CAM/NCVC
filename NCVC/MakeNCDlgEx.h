// MakeNCDlgEx.h : ヘッダー ファイル
//

#pragma once

#include "MakeNCDlgEx2.h"
#include "MakeNCDlgEx3.h"

/////////////////////////////////////////////////////////////////////////////
// CMakeNCDlgEx

class CMakeNCDlgEx : public CPropertySheet
{
	UINT		m_nID;		// 生成ID
	CDXFDoc*	m_pDoc;		// ﾚｲﾔ情報取得
	
// コンストラクション
public:
	CMakeNCDlgEx(UINT, CDXFDoc*);
	~CMakeNCDlgEx();

// アトリビュート
public:
	UINT		GetNCMakeID(void) {
		return m_nID;
	}
	CDXFDoc*	GetDocument(void) {
		return m_pDoc;
	}

	// 内部ﾍﾟｰｼﾞﾀﾞｲｱﾛｸﾞ
	CMakeNCDlgEx2	m_dlg1;
	CMakeNCDlgEx3	m_dlg2;

	// ﾀﾞｲｱﾛｸﾞ共通項
	CString		m_strNCFileName,
				m_strInitFileName,
				m_strLayerToInitFileName;

	DECLARE_MESSAGE_MAP()
};
