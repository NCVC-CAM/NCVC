// 3dModelDoc.h : ヘッダー ファイル
//

#pragma once

#include "DocBase.h"
#include "Kodatuno/BODY.h"
#undef PI	// Use NCVC (MyTemplate.h)
#include "3dOption.h"

typedef	std::vector<Coord>		VCoord;
typedef std::vector<VCoord>		VVCoord;
typedef	std::vector<VVCoord>	VVVCoord;

enum ENCOORDMODE {
	ROUGH, CONTOUR
};

/////////////////////////////////////////////////////////////////////////////
// C3dModelDoc ドキュメント

class C3dModelDoc : public CDocBase
{
	C3dOption	m_3dOpt;			// オプション管理クラス
	CString		m_strNCFileName;	// NC生成ﾌｧｲﾙ名

	BODY*		m_pKoBody;			// Kodatuno Body
	BODYList*	m_pKoList;			// Kodatuno Body List

	ENCOORDMODE	m_enCoordMode;		// 荒化工 or 等高線
	VVVCoord	m_vvvKoCoord;		// 荒化工・等高線 兼用座標配列

protected:
	C3dModelDoc();
	DECLARE_DYNCREATE(C3dModelDoc)
#ifdef _DEBUG
	void	DumpRoughCoord(const VVCoord&);
	void	DumpContourCoord(const VCoord&);
#endif

public:
	virtual ~C3dModelDoc();
	virtual void Serialize(CArchive& ar);   // ドキュメント I/O に対してオーバーライドされました。
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	virtual void OnCloseDocument();

	C3dOption* Get3dOption(void) {
		return &m_3dOpt;
	}
	CString GetNCFileName(void) const {
		return m_strNCFileName;
	}
	BODYList*	GetKodatunoBodyList(void) const {
		return m_pKoList;
	}
	void	ClearKoCoord(void);
	ENCOORDMODE	GetKoCoordMode(void) const {
		return m_enCoordMode;
	}
	BOOL	MakeRoughCoord(NURBSS*, NURBSC*);
	BOOL	MakeContourCoord(NURBSS*);
	VVCoord	SetGroupCoord(VCoord&, double);
	//
	VVVCoord&	GetKoCoord(void) {
		return m_vvvKoCoord;
	}

protected:
	afx_msg void OnUpdateFile3dMake(CCmdUI* pCmdUI);
	afx_msg void OnFile3dMake();

	DECLARE_MESSAGE_MAP()
};
