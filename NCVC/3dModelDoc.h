// 3dModelDoc.h : �w�b�_�[ �t�@�C��
//

#pragma once

#include "DocBase.h"

/////////////////////////////////////////////////////////////////////////////
// C3dModelDoc �h�L�������g

class C3dModelDoc : public CDocBase
{

public:
	C3dModelDoc();
	virtual ~C3dModelDoc();

	virtual void Serialize(CArchive& ar);   // �h�L�������g I/O �ɑ΂��ăI�[�o�[���C�h����܂����B
	virtual BOOL OnNewDocument();

protected:

	DECLARE_DYNCREATE(C3dModelDoc)
	DECLARE_MESSAGE_MAP()
};
