// 3dModelDoc.h : �w�b�_�[ �t�@�C��
//

#pragma once

#include "DocBase.h"
#include "Kodatuno/BODY.h"
#undef PI	// Use NCVC (MyTemplate.h)
#include "3dOption.h"

typedef	std::vector<Coord>		VCoord;
typedef std::vector<VCoord>		VVCoord;
typedef	std::vector<VVCoord>	VVVCoord;

/////////////////////////////////////////////////////////////////////////////
// C3dModelDoc �h�L�������g

class C3dModelDoc : public CDocBase
{
	C3dOption	m_3dOpt;			// �I�v�V�����Ǘ��N���X
	CString		m_strNCFileName;	// NC����̧�ٖ�

	BODY*		m_pKoBody;			// Kodatuno Body
	BODYList*	m_pKoList;			// Kodatuno Body List

	VVVCoord	m_vvvRoughCoord,	// �r���H�p���W�i3�����z��j
				m_vvvContourCoord;	// �d�グ���������W

protected:
	C3dModelDoc();
	DECLARE_DYNCREATE(C3dModelDoc)
#ifdef _DEBUG
	void	DumpRoughCoord(const VVCoord&);
	void	DumpContourCoord(const VCoord&);
#endif

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
	void	ClearRoughCoord(void);
	void	ClearContourCoord(void);
	BOOL	MakeRoughCoord(NURBSS*, NURBSC*);
	BOOL	MakeContourCoord(NURBSS*);
	VVCoord	SetGroupCoord(VCoord&, double);
	//
	VVVCoord&	GetRoughCoord(void) {
		return m_vvvRoughCoord;
	}
	VVVCoord&	GetContourCoord(void) {
		return m_vvvContourCoord;
	}

protected:
	afx_msg void OnUpdateFile3dMake(CCmdUI* pCmdUI);
	afx_msg void OnFile3dMake();

	DECLARE_MESSAGE_MAP()
};
