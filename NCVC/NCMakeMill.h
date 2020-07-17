// NCMakeMill.h: CNCMakeMill �N���X�̃C���^�[�t�F�C�X
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NCMakeBase.h"

typedef CString (*PFNGETCYCLESTRING)(void);

// �Œ軲�ٔF������
#define	NCMAKECYCLECODE		81

class CNCMakeMill : public CNCMakeBase
{
	// �������͕ω��̂Ȃ���߼�݂ɑ΂��铮��
	static	PFNGETCYCLESTRING	ms_pfnGetCycleString;	// G����Ӱ���(�Œ軲��)
	static	int		ms_nCycleCode;	// �Œ軲�ق̐؍�w��(81,82,85,89)
	static	int		ms_nCycleReturn;// �Œ軲�ق̕��A�w��(88,89)

	// ���W�l�ݒ�
	static	CString	GetValString(int, double, BOOL);
	// G����Ӱ���(�Œ軲��)
	static	CString	GetCycleString(void);
	static	CString	GetCycleString_Clip(void);

	// ---
	void	MakePolylineMov(const CDXFpolyline*, BOOL);

public:
	// �؍��ް�
	CNCMakeMill(const CDXFdata*, double, const double* = NULL);
	// ���H�J�n�ʒu�w���ް���XY�ړ�
	CNCMakeMill(const CDXFdata*, BOOL);
	// Z���̕ω�(�㏸�E���~)
	CNCMakeMill(int, double, double);
	// XY��G[0|1]�ړ�
	CNCMakeMill(int, const CPointD& pt);
	// ���W�w���ɂ��~�ʂ̐���
	CNCMakeMill(int, const CPointD&, const CPointD&, const CPointD&, double);
	// �C�ӂ̕�������
	CNCMakeMill(const CString&);

	// ������߼�݂ɂ��ÓI�ϐ��̏�����(TH_MakeNCD.cpp)
	static	void	SetStaticOption(const CNCMakeMillOpt*);

	// TH_MakeNCD.cpp �ŏ�����
	static	double	ms_dCycleZ[2];	// �Œ軲�ق̐؂荞��Z���W
	static	double	ms_dCycleR[2];	// �Œ軲�ق�R�_
	static	double	ms_dCycleP[2];	// �Œ軲�ق��޳�َ���

	// TH_MakeNCD.cpp ����Q��
	static	CString	MakeSpindle(ENDXFTYPE enType, BOOL bDeep = FALSE);
};
