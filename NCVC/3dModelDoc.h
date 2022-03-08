// 3dModelDoc.h : �w�b�_�[ �t�@�C��
//

#pragma once

#include "DocBase.h"
#include "Kodatuno/BODY.h"
#undef PI	// Use NCVC (MyTemplate.h)
#include "3dOption.h"

/////////////////////////////////////////////////////////////////////////////
// C3dModelDoc �h�L�������g

class C3dModelDoc : public CDocBase
{
	C3dOption	m_3dOpt;			// �I�v�V�����Ǘ��N���X
	CString		m_strNCFileName;	// NC����̧�ٖ�

	BODY*		m_pKoBody;		// Kodatuno Body
	BODYList*	m_pKoList;		// Kodatuno Body List

	Coord***	m_pScanCoord;	// �������ꂽ�p�X���i�[
	int			m_pScanX,
				m_pScanY;
	int*		m_pScanNum;		// �X�L�������C��1�{���Ƃ̉��H�_�����i�[

protected:
	C3dModelDoc();
	DECLARE_DYNCREATE(C3dModelDoc)

public:
	virtual ~C3dModelDoc();
	virtual void Serialize(CArchive& ar);   // �h�L�������g I/O �ɑ΂��ăI�[�o�[���C�h����܂����B
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
	void	ClearScanPath(void);
	BOOL	MakeScanPath(NURBSS*, NURBSC*);
	Coord***	GetScanPathCoord(void) const {
		return m_pScanCoord;
	}
	boost::tuple<int, int>	GetScanNumXY(void) const {
		return boost::make_tuple(m_pScanX, m_pScanY);
	}
	int			GetScanNumZ(int y) const {
		return m_pScanNum[y];
	}

protected:
	afx_msg void OnUpdateFile3dMake(CCmdUI* pCmdUI);
	afx_msg void OnFile3dMake();

	DECLARE_MESSAGE_MAP()
};
