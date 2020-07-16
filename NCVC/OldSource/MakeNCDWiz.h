#if !defined(___CMAKENCDWIZ___)
#define ___CMAKENCDWIZ___

#pragma once

#include "MakeNCDWiz1.h"
#include "MakeNCDWiz2.h"
#include "MakeNCDWiz3.h"
#include "MakeNCDWiz9.h"
#include "DXFDoc.h"

// ���ް�޵�߼��
typedef struct tagMAKENCDWIZARDPARAM {
	int		nType;		// ���H�����(�a�␳�֊s, ���l�֊s, �߹��)
	int		nCorrect;	// G41 or G42
	int		nPref;		// �D��(�␳ or ����)
	int		nTool;		// �H��ԍ�
	int		nAcute;		// �O���s�p
	int		nOffNode;	// �̾�Č�_�����Ƃ�
	BOOL	bG10;		// G10
	double	dTool;		// �a�w��
	double	dRemain;	// �c���
} MAKENCDWIZARDPARAM, *LPMAKENCDWIZARDPARAM;

// CMakeNCDWiz

class CMakeNCDWiz : public CPropertySheet
{
public:
	CMakeNCDWiz(CDXFDoc*, LPMAKENCDWIZARDPARAM);
	virtual ~CMakeNCDWiz();

	CMakeNCDWiz1	m_dlg1;
	CMakeNCDWiz2	m_dlg2;
	CMakeNCDWiz3	m_dlg3;
	CMakeNCDWiz9	m_dlg9;

	CString	m_strNCFileName;	// ���(DXFDoc)�ɓn������

	CDXFDoc*	m_pDoc;
	LPMAKENCDWIZARDPARAM	m_pParam;

protected:

	DECLARE_MESSAGE_MAP()
};

#endif
