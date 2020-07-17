// NCViewXZ.h : �w�b�_�[ �t�@�C��
//

#pragma once

#include "NCViewBase.h"

class CNCViewXZ : public CNCViewBase
{
protected:
	CNCViewXZ();           // ���I�����Ɏg�p�����v���e�N�g �R���X�g���N�^
	DECLARE_DYNCREATE(CNCViewXZ)

	virtual	CRectF	ConvertRect(const CRect3F& rc) {
		CRectF	rcResult(rc.left, rc.low, rc.right, rc.high);
		return rcResult;
	}
	virtual	boost::tuple<size_t, size_t>	GetPlaneAxis(void) {
		size_t	x = NCA_X, y = NCA_Z;
		return boost::make_tuple(x, y);
	}
	virtual	void	SetGuideData(void);

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B

	//{{AFX_VIRTUAL(CNCViewXZ)
	public:
	virtual void OnInitialUpdate();
	protected:
	//}}AFX_VIRTUAL

	// �������ꂽ���b�Z�[�W �}�b�v�֐�
protected:
	//{{AFX_MSG(CNCViewXZ)
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
