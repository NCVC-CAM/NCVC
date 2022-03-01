// 3dModelDoc.h : ヘッダー ファイル
//

#pragma once

#include "DocBase.h"
#include "Kodatuno/BODY.h"
#undef PI	// Use NCVC (MyTemplate.h)
#include "3dScanSetupDlg.h"		// SCANSETUP

/////////////////////////////////////////////////////////////////////////////
// C3dModelDoc ドキュメント

class C3dModelDoc : public CDocBase
{
	CString		m_strNCFileName;	// NC生成ﾌｧｲﾙ名

	BODY*		m_pKoBody;		// Kodatuno Body
	BODYList*	m_pKoList;		// Kodatuno Body List

	Coord***	m_pScanPath;	// 生成されたパスを格納
	int			m_pScanX,
				m_pScanY;
	int*		m_pScanNum;		// スキャンライン1本ごとの加工点数を格納

protected:
	C3dModelDoc();
	DECLARE_DYNCREATE(C3dModelDoc)

public:
	virtual ~C3dModelDoc();
	virtual void Serialize(CArchive& ar);   // ドキュメント I/O に対してオーバーライドされました。
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	virtual void OnCloseDocument();

	CString GetNCFileName(void) const {
		return m_strNCFileName;
	}
	BODYList*	GetKodatunoBodyList(void) const {
		return m_pKoList;
	}
	void	ClearScanPath(void);
	BOOL	MakeScanPath(NURBSS*, NURBSC*, SCANSETUP&);
	Coord***	GetScanPathCoord(void) const {
		return m_pScanPath;
	}
	boost::tuple<int, int>	GetScanPathXY(void) const {
		return boost::make_tuple(m_pScanX, m_pScanY);
	}
	int*		GetScanPathZ(void) const {
		return m_pScanNum;
	}

protected:
	afx_msg void OnUpdateFile3dMake(CCmdUI* pCmdUI);
	afx_msg void OnFile3dMake();

	DECLARE_MESSAGE_MAP()
};
