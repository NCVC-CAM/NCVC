// 3dModelDoc.h : ヘッダー ファイル
//

#pragma once

#include "DocBase.h"
#include "Kodatuno/BODY.h"
#undef PI	// Use NCVC (MyTemplate.h)
#include "3dOption.h"

typedef	std::vector<Coord>	VCoord;
typedef std::vector< std::vector<Coord> >	VVCoord;

/////////////////////////////////////////////////////////////////////////////
// C3dModelDoc ドキュメント

class C3dModelDoc : public CDocBase
{
	C3dOption	m_3dOpt;			// オプション管理クラス
	CString		m_strNCFileName;	// NC生成ﾌｧｲﾙ名

	BODY*		m_pKoBody;			// Kodatuno Body
	BODYList*	m_pKoList;			// Kodatuno Body List

	std::vector<VVCoord>	m_vvvRoughCoord,	// 荒加工用座標（3次元配列）
							m_vvvContourCoord;	// 仕上げ等高線座標

protected:
	C3dModelDoc();
	DECLARE_DYNCREATE(C3dModelDoc)
#ifdef _DEBUG
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
	void	ClearRoughCoord(void);
	void	ClearContourCoord(void);
	BOOL	MakeRoughCoord(NURBSS*, NURBSC*);
	BOOL	MakeContourCoord(NURBSS*);
	void	SetContourGroup(VCoord&);
	std::vector<VVCoord>&	GetRoughCoord(void) {
		return m_vvvRoughCoord;
	}
	std::vector<VVCoord>&	GetContourCoord(void) {
		return m_vvvContourCoord;
	}

protected:
	afx_msg void OnUpdateFile3dMake(CCmdUI* pCmdUI);
	afx_msg void OnFile3dMake();

	DECLARE_MESSAGE_MAP()
};
