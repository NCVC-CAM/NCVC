// NCMakeBase.h: CNCMake �N���X�̃C���^�[�t�F�C�X
//////////////////////////////////////////////////////////////////////

#pragma once

typedef	CString	(*PFNGETARGINT)(int);
typedef	CString	(*PFNGETARGDOUBLE)(double);
typedef CString (*PFNGETARGVOID)(void);
typedef	CString	(*PFNGETVALSTRING)(int, double, BOOL);
typedef	CString	(*PFNMAKECIRCLESUB)(int, const CPointD&, const CPointD&, double);
typedef	CString	(*PFNMAKECIRCLE)(const CDXFcircle*, double);
typedef	CString	(*PFNMAKEHELICAL)(const CDXFcircle*, double, double);
typedef	CString	(*PFNMAKEARC)(const CDXFarc*, double);

class CNCMakeBase
{
protected:
	CNCMakeBase();		// �h���׽�p
	// �ݽ�׸� -- �C�ӂ̕�������
	CNCMakeBase(const CString& strGcode);

	static	const	CNCMakeOption*	ms_pMakeOpt;

	CString			m_strGcode;		// �������ꂽG����(1��ۯ�)
	CStringArray	m_strGarray;	// POLYLINE, Ellipse��������G���ސ���

	// �������͕ω��̂Ȃ���߼�݂ɑ΂��铮��
	static	int		NCAX, NCAY,		// �ײ��Ɛ��Ղ̎��ϊ�
					NCAI, NCAJ;
	static	int		ms_nGcode;		// �O���G����
	static	int		ms_nSpindle;	// �O��̉�]��
	static	double	ms_dFeed;		// �O��̐؍푗�葬�x
	static	int		ms_nCnt;		// ̧�ُo�͎��̍s���ް
	static	int		ms_nMagni;		// �s�ԍ��{��
	static	int		ms_nCircleCode;	// �~�ް��̐؍�w��(2 or 3)
	static	BOOL	ms_bIJValue;	// [I|J]0�𐶐����邩�ǂ���
	static	double	ms_dEllipse;	// �ȉ~����
	static	CString	ms_strEOB;		// EOB

	static	PFNGETARGINT		ms_pfnGetSpindle;		// S���ނ̐���
	static	PFNGETARGDOUBLE		ms_pfnGetFeed;			// F���ނ̐���
	static	PFNGETARGVOID		ms_pfnGetLineNo;		// �s�ԍ��t��
	static	PFNGETARGINT		ms_pfnGetGString;		// G����Ӱ���
	static	PFNGETVALSTRING		ms_pfnGetValString;		// ���W�l�ݒ�
	static	PFNGETARGDOUBLE		ms_pfnGetValDetail;		// �@�̏ڍ�
	static	PFNMAKECIRCLESUB	ms_pfnMakeCircleSub;	// �~�E�~���ް��̐����⏕
	static	PFNMAKECIRCLE		ms_pfnMakeCircle;		// �~�ް��̐���
	static	PFNMAKEHELICAL		ms_pfnMakeHelical;		// �~�ް����ضِ؍�
	static	PFNMAKEARC			ms_pfnMakeArc;			// �~���ް��̐���

	// ��]�w��
	static	CString	GetSpindleString(int);
	static	CString	GetSpindleString_Clip(int);
	// ���葬�x
	static	CString	GetFeedString(double);
	static	CString	GetFeedString_Integer(double);
	// �s�ԍ��t��
	static	CString	GetLineNoString(void);
	static	CString	GetLineNoString_Clip(void);
	// G����Ӱ���
	static	CString	GetGString(int);
	static	CString	GetGString_Clip(int);
	// ���W�l�ݒ�
	static	CString	GetValString_Normal(double);
	static	CString	GetValString_UZeroCut(double);
	static	CString	GetValString_Multi1000(double);
	// �~�E�~�ʂ̐����⏕
	static	CString	MakeCircleSub_R(int, const CPointD&, const CPointD&, double);
	static	CString	MakeCircleSub_IJ(int, const CPointD&, const CPointD&, double);
	static	CString	MakeCircleSub_Helical(int, const CPoint3D&);
	// �~�ް��̐���
	static	CString	MakeCircle_R(const CDXFcircle*, double);
	static	CString	MakeCircle_IJ(const CDXFcircle*, double);
	static	CString	MakeCircle_IJHALF(const CDXFcircle*, double);
	static	CString	MakeCircle_R_Helical(const CDXFcircle*, double, double);
	static	CString	MakeCircle_IJ_Helical(const CDXFcircle*, double, double);
	static	CString	MakeCircle_IJHALF_Helical(const CDXFcircle*, double, double);
	// �~���ް��̐���
	static	CString	MakeArc_R(const CDXFarc*, double);
	static	CString	MakeArc_IJ(const CDXFarc*, double);
	// ������߼�݂ɂ��ÓI�ϐ��̏�����
	static	void	SetStaticOption(const CNCMakeOption*);

	// ---

	// �ȉ~�ް��̐���(�������)
	void	MakeEllipse(const CDXFellipse *, double);
	CString	MakeEllipse_Tolerance(const CDXFellipse*, double);
	// Polyline �̐���
	void	MakePolylineCut(const CDXFpolyline*, double);

public:
	// TH_MakeXXX.cpp �ŏ�����
	static	double	ms_xyz[NCXYZ];	// �O��̈ʒu
	static	void	InitialVariable(void);
	// TH_MakeXXX.cpp �ł��g�p
	static	CString	MakeCustomString(int, DWORD = 0, double* = NULL, BOOL = TRUE);
	static	CString	MakeCustomString(int, int[], double[]);

	// G���ޏo��
	void	WriteGcode(CStdioFile&);
};
