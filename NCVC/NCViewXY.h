// NCViewXY.h : �w�b�_�[ �t�@�C��
//

#pragma once

#include "NCViewBase.h"

class CNCViewXY : public CNCViewBase
{
	// ܰ��~��
	CPointD		m_ptdWorkCylinder[ARCCOUNT];
	CPoint		m_ptDrawWorkCylinder[ARCCOUNT];

protected:
	CNCViewXY();           // ���I�����Ɏg�p�����v���e�N�g �R���X�g���N�^
	DECLARE_DYNCREATE(CNCViewXY)

	virtual	void	SetGuideData(void);
	virtual	void	SetWorkCylinder(void);
	virtual	void	ConvertWorkCylinder(void);
	virtual	void	DrawWorkCylinder(CDC*);

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
