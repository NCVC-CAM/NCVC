// NCViewXY.h : �w�b�_�[ �t�@�C��
//

#pragma once

#include "NCViewBase.h"

class CNCViewXY : public CNCViewBase
{
protected:
	CNCViewXY();           // ���I�����Ɏg�p�����v���e�N�g �R���X�g���N�^
	DECLARE_DYNCREATE(CNCViewXY)

	virtual	void	SetGuideData(void);

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B
	//{{AFX_VIRTUAL(CNCViewXY)
	public:
	virtual void OnInitialUpdate();
	protected:
	//}}AFX_VIRTUAL

	// �������ꂽ���b�Z�[�W �}�b�v�֐�
protected:
	//{{AFX_MSG(CNCViewXY)
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
