// NCMakeLathe.h: CNCMakeLathe �N���X�̃C���^�[�t�F�C�X
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NCMakeBase.h"

enum	TWOMOVEMODE {
	ZXMOVE, XZMOVE
};

class CNCMakeLathe : public CNCMakeBase
{
	// ���W�l�ݒ�
//	static	CString	GetValString(int, double, BOOL);
	virtual	CString	GetValString(int, double, BOOL);

public:
	// �؍��ް�
	CNCMakeLathe(const CDXFdata*);
	// �w��ʒu�ɒ����ړ�
	CNCMakeLathe(const CPointD&);
	// �w��ʒu�ɂQ���ړ�
	CNCMakeLathe(TWOMOVEMODE, const CPointD&);
	// X|Z���̕ω�
	CNCMakeLathe(int, int, double);
	// �C�ӂ̕�������
	CNCMakeLathe(const CString&);

	// ������߼�݂ɂ��ÓI�ϐ��̏�����(TH_MakeLathe.cpp)
	static	void	SetStaticOption(const CNCMakeLatheOpt*);

	// TH_MakeLathe.cpp ����Q��
	static	CString	MakeSpindle(void);
};
