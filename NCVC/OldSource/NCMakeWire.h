// NCMakeWire.h: CNCMakeWire �N���X�̃C���^�[�t�F�C�X
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NCMakeMillOpt.h"
#include "NCMakeMill.h"

class CNCMakeWire : public CNCMakeMill
{
public:
	// �؍��ް�
	CNCMakeWire(const CDXFdata*, double);
	// �㉺�ٌ`��̐؍��ް�
	CNCMakeWire(const CDXFdata*, const CDXFdata*, double);
	// �㉺�ٌ`�����א����Ő���
	CNCMakeWire(const CVPointD&, const CVPointD&, double);
	// XY��G[0|1]�ړ�
	CNCMakeWire(int, const CPointD&, double, double);
	// XY/UV��G01�ړ�
	CNCMakeWire(const CPointD&, const CPointD&, double);
	// �C�ӂ̕�������
	CNCMakeWire(const CString&);

	// ������߼�݂ɂ��ÓI�ϐ��̏�����(TH_MakeWire.cpp)
	static	void	SetStaticOption(const CNCMakeWireOpt*);	// TH_MakeWire.cpp
};
