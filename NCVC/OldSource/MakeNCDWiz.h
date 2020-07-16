#if !defined(___CMAKENCDWIZ___)
#define ___CMAKENCDWIZ___

#pragma once

#include "MakeNCDWiz1.h"
#include "MakeNCDWiz2.h"
#include "MakeNCDWiz3.h"
#include "MakeNCDWiz9.h"
#include "DXFDoc.h"

// ³¨»Ş°ÄŞµÌß¼®İ
typedef struct tagMAKENCDWIZARDPARAM {
	int		nType;		// ‰ÁHÊßÀ°İ(Œa•â³—ÖŠs, ”’l—ÖŠs, Îß¹¯Ä)
	int		nCorrect;	// G41 or G42
	int		nPref;		// —Dæ(•â³ or •ûŒü)
	int		nTool;		// H‹ï”Ô†
	int		nAcute;		// ŠO‘¤‰sŠp
	int		nOffNode;	// µÌ¾¯ÄŒğ“_–³‚¢‚Æ‚«
	BOOL	bG10;		// G10
	double	dTool;		// Œaw’è
	double	dRemain;	// c‚è‘ã
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

	CString	m_strNCFileName;	// ãˆÊ(DXFDoc)‚É“n‚·‚½‚ß

	CDXFDoc*	m_pDoc;
	LPMAKENCDWIZARDPARAM	m_pParam;

protected:

	DECLARE_MESSAGE_MAP()
};

#endif
