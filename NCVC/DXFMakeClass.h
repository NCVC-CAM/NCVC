// DXFMakeClass.h: CDXFMake �N���X�̃C���^�[�t�F�C�X
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "DXFdata.h"
#include "NCdata.h"
#include "DXFkeyword.h"
#include "DXFMakeOption.h"

class CDXFMake;
class CDXFDoc;
typedef	CString	(*PFNMAKEVALUE)(const CNCdata*);
typedef	CString	(*PFNMAKEVALUECIRCLE)(const CNCcircle*);
typedef	CString	(CDXFMake::*PFNMAKEVALUECYCLE)(const CNCcycle*, int);
// TABLES����݂Ŏg��ڲԏ��
struct TABLELAYERINFO {
	CString	strLayer;
	CString	strType;
	int		nColor;
	TABLELAYERINFO(LPCTSTR pszLayer, LPCTSTR pszType, int col) {
		strLayer = pszLayer;
		strType  = pszType;
		nColor   = col;
	}
};

class CDXFMake
{
	CStringArray	m_strDXFarray;	// �eNC��޼ު�Ă�DXF����

	// ����ݐ���
	void	MakeSection_Header(const CDocBase*);
	void	MakeSection_Tables(const CDocBase*);
	void	MakeSection_TableLayer_NCD(std::vector<TABLELAYERINFO>&);
	void	MakeSection_TableLayer_DXF(std::vector<TABLELAYERINFO>&, const CDXFDoc*);
	void	MakeSection_Blocks(void);
	void	MakeSection_Entities(void);
	void	MakeSection_EOF(void);
	// ��޼ު�Đ���
	CString	MakeDXF_Figure(int, int);
	CString	MakeValueCycle(const CNCcycle*, int, int, ENPLANE);
	void	MakeDXF_NCtoLine(const CNCline*, BOOL);
	void	MakeDXF_NCtoArc(const CNCcircle*, BOOL);
	void	MakeDXF_NCtoCycle(const CNCcycle*);
	void	MakeDXF_PolylineDismantle(const CDXFpolyline*, const CPointF&);
	CString	MakeValueCycle_XY_Circle(const CNCcycle*, int);
	CString	MakeValueCycle_XZ_Circle(const CNCcycle*, int);
	CString	MakeValueCycle_YZ_Circle(const CNCcycle*, int);
	CString	MakeValueCycle_XY_Point(const CNCcycle*, int);
	CString	MakeValueCycle_XZ_Point(const CNCcycle*, int);
	CString	MakeValueCycle_YZ_Point(const CNCcycle*, int);

	// �ÓI�ϐ�
	static	const	CDXFMakeOption*		ms_pMakeOpt;
	static	PFNMAKEVALUE		ms_pfnMakeValueLine,
								ms_pfnMakeValueCircleToLine;
	static	PFNMAKEVALUECIRCLE	ms_pfnMakeValueCircle;
	static	PFNMAKEVALUECYCLE	ms_pfnMakeValueCycle;

public:
	// ����ݏ��
	CDXFMake(enSECNAME, const CDocBase* = NULL);
	// ENTITIES�ް�
	CDXFMake(const CNCdata*,  BOOL = FALSE);
	CDXFMake(const CDXFdata*, const CPointF&);
	// ���_���Aܰ���`
	CDXFMake(const CPoint3F&, float = 0.0f);
	CDXFMake(const CRectF&);

	// �ÓI�ϐ�������
	static	void	SetStaticOption(const CDXFMakeOption*);

	// DXF�o��
	void	WriteDXF(CStdioFile& fp) {
		for ( int i=0; i<m_strDXFarray.GetSize(); i++ )
			fp.WriteString( m_strDXFarray[i] );
	}
};

typedef	CTypedPtrArrayEx<CPtrArray, CDXFMake*>	CDxfMakeArray;

/////////////////////////////////////////////////

void	InitialColorIndex(void);	// from CNCVCApp::CNCVCApp()
