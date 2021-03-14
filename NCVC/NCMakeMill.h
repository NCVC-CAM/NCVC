// NCMakeMill.h: CNCMakeMill �N���X�̃C���^�[�t�F�C�X
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NCMakeBase.h"
#include "NCMakeMillOpt.h"

typedef CString (*PFNGETCYCLESTRING)(void);

// �Œ軲�ٔF������
#define	NCMAKECYCLECODE		81

class CNCMakeMill : public CNCMakeBase
{
	// �������͕ω��̂Ȃ���߼�݂ɑ΂��铮��
	static	PFNGETCYCLESTRING	ms_pfnGetCycleString;	// G����Ӱ���(�Œ軲��)
	static	int		ms_nCycleCode;	// �Œ軲�ق̐؍�w��(81,82,83,85,89)
	static	int		ms_nCycleReturn;// �Œ軲�ق̕��A�w��(98,99)
	// G����Ӱ���(�Œ軲��)
	static	CString	GetCycleString(void);
	static	CString	GetCycleString_Clip(void);

protected:
	CNCMakeMill();		// �h���׽�p
	// ���W�l�ݒ�
	static	CString	GetValString(int, float, BOOL = FALSE);
	//
	void	MakePolylineMov(const CDXFpolyline*, BOOL);

public:
	// �؍��ް�
	CNCMakeMill(const CDXFdata*, float, const float* = NULL);
	// ���H�J�n�ʒu�w���ް���XY�ړ�
	CNCMakeMill(const CDXFdata*, BOOL);
	// Z���̕ω�(�㏸�E���~)
	CNCMakeMill(int, float, float);
	// XY��G[0|1]�ړ�
	CNCMakeMill(int, const CPointF&, float);
	// XYZ��G01
	CNCMakeMill(const CPoint3F&, float);
	// ���W�w���ɂ��~�ʂ̐���
	CNCMakeMill(int, const CPointF&, const CPointF&, const CPointF&, float);
	// �h�E�F�����ԁiG04�j
	CNCMakeMill(float);
	// �C�ӂ̕�������
	CNCMakeMill(const CString&);

	// ������߼�݂ɂ��ÓI�ϐ��̏�����(TH_MakeNCD.cpp)
	static	void	SetStaticOption(const CNCMakeMillOpt*);

	// TH_MakeNCD.cpp �ŏ�����
	static	float	ms_dCycleZ[2],	// �Œ軲�ق̐؂荞��Z���W
					ms_dCycleR[2],	// �Œ軲�ق�R�_
					ms_dCycleP[2],	// �Œ軲�ق��޳�َ���
					ms_dCycleQ[2];	// G83�[��Q�l

	// TH_MakeNCD.cpp ����Q��
	static	CString	MakeSpindle(ENDXFTYPE enType, BOOL bDeep = FALSE);
};
