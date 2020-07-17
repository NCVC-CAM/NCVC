// NCMakeWire.h: CNCMakeWire �N���X�̃C���^�[�t�F�C�X
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NCMakeMillOpt.h"
#include "NCMakeMill.h"

class CNCMakeWire : public CNCMakeMill
{
public:
	// �؍��ް�
	CNCMakeWire(const CDXFdata*, float);
	// �㉺�ٌ`��̐؍��ް�
	CNCMakeWire(const CDXFdata*, const CDXFdata*, float);
	// �㉺�ٌ`�����א����Ő���
	CNCMakeWire(const CVPointF&, const CVPointF&, float);
	// XY��G[0|1]�ړ�
	CNCMakeWire(int, const CPointF&, float, float);
	// XY/UV��G01�ړ�
	CNCMakeWire(const CPointF&, const CPointF&, float);
	// �C�ӂ̕�������
	CNCMakeWire(const CString&);

	// ������߼�݂ɂ��ÓI�ϐ��̏�����(TH_MakeWire.cpp)
	static	void	SetStaticOption(const CNCMakeWireOpt*);	// TH_MakeWire.cpp
};
