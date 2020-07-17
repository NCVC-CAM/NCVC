// NCViewYZ.h : �w�b�_�[ �t�@�C��
//

#pragma once

#include "NCViewBase.h"

class CNCViewYZ : public CNCViewBase
{
protected:
	CNCViewYZ();           // ���I�����Ɏg�p�����v���e�N�g �R���X�g���N�^
	DECLARE_DYNCREATE(CNCViewYZ)

	virtual	CRectD	ConvertRect(const CRect3D& rc) {
		CRectD	rcResult(rc.top, rc.low, rc.bottom, rc.high);
		return rcResult;
	}
	virtual	boost::tuple<size_t, size_t>	GetPlaneAxis(void) {
		size_t	x = NCA_Y, y = NCA_Z;
		return boost::make_tuple(x, y);
	}
	virtual	void	SetGuideData(void);

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B

	//{{AFX_VIRTUAL(CNCViewYZ)
	public:
	virtual void OnInitialUpdate();
	protected:
	//}}AFX_VIRTUAL

	// �������ꂽ���b�Z�[�W �}�b�v�֐�
protected:
	//{{AFX_MSG(CNCViewYZ)
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
