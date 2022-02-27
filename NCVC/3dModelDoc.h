// 3dModelDoc.h : �w�b�_�[ �t�@�C��
//

#pragma once

#include "DocBase.h"
#include "Kodatuno/BODY.h"
#undef PI	// Use NCVC (MyTemplate.h)
#include "3dScanSetupDlg.h"		// SCANSETUP

using std::vector;

/////////////////////////////////////////////////////////////////////////////
// C3dModelDoc �h�L�������g

class C3dModelDoc : public CDocBase
{
	BODY*		m_pKoBody;		// Kodatuno Body
	BODYList*	m_pKoList;		// Kodatuno Body List

	vector< vector< vector<Coord> > >	m_vPath;

protected:
	C3dModelDoc();
	DECLARE_DYNCREATE(C3dModelDoc)

public:
	virtual ~C3dModelDoc();
	virtual void Serialize(CArchive& ar);   // �h�L�������g I/O �ɑ΂��ăI�[�o�[���C�h����܂����B
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	virtual void OnCloseDocument();

	BODYList*	GetKodatunoBodyList(void) const {
		return m_pKoList;
	}
	void	MakeScanPath(NURBSS*, NURBSC*, SCANSETUP&);

protected:

	DECLARE_MESSAGE_MAP()
};
