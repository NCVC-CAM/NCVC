// NCMakeLathe.h: CNCMakeLathe �N���X�̃C���^�[�t�F�C�X
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NCMakeBase.h"
#include "NCMakeLatheOpt.h"

enum	TWOMOVEMODE {
	ZXMOVE, XZMOVE
};

class CNCMakeLathe : public CNCMakeBase
{
	// ���W�l�ݒ�(PFNGETVALSTRING�^�ɍ��킹�邽�߂̑�3����)
	static	CString	GetValString(int, float, BOOL = FALSE);

public:
	// ���޼ު��
	CNCMakeLathe(void);
	// �؍��ް�
	CNCMakeLathe(const CDXFdata*, float);
	// �w��ʒu�ɒ����ړ�
	CNCMakeLathe(const CPointF&);
	// �w��ʒu�ɂQ���ړ�
	CNCMakeLathe(TWOMOVEMODE, const CPointF&, float);
	// X|Z���̕ω�
	CNCMakeLathe(int, int, float, float);
	// �C�ӂ̕�������
	CNCMakeLathe(const CString&);

	// �[�ʉ��H����
	void	CreateEndFace(const CPointF&);
	// �������H����
	void	CreatePilotHole(void);

	// ������߼�݂ɂ��ÓI�ϐ��̏�����(TH_MakeLathe.cpp)
	static	void	SetStaticOption(const CNCMakeLatheOpt*);

	// TH_MakeLathe.cpp ����Q��
	static	CString	MakeSpindle(int);
};
