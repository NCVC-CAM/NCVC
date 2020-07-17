// NCView.h : CNCView �N���X�̐錾����уC���^�[�t�F�C�X�̒�`�����܂��B
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "NCViewBase.h"

class CNCView : public CNCViewBase
{
	// �ް���`
	CPointD		m_ptdMaxRect[2][4];		// �ő�؍��`[Zmin/Zmax][��`]
	CPoint		m_ptDrawMaxRect[2][4];	// �ő�؍��`�̕`��p���W
	// ܰ���`
	CPointD		m_ptdWorkRect[2][4];	// ܰ���`[Zmin/Zmax][��`] �S�p(from CNCDoc::m_rcWork)
	CPoint		m_ptDrawWorkRect[2][4];	// ܰ���`�̕`��p���W
	// ܰ��~��
	CPointD		m_ptdWorkCylinder[2][ARCCOUNT];
	CPoint		m_ptDrawWorkCylinder[2][ARCCOUNT];

protected: // �V���A���C�Y�@�\�݂̂���쐬���܂��B
	CNCView();
	DECLARE_DYNCREATE(CNCView)

	virtual	void	SetGuideData(void);
	virtual	void	SetDataMaxRect(void);
	virtual	void	SetWorkRect(void);
	virtual	void	SetWorkCylinder(void);
	virtual	void	ConvertMaxRect(void);
	virtual	void	ConvertWorkRect(void);
	virtual	void	ConvertWorkCylinder(void);
	virtual	void	DrawMaxRect(CDC*);
	virtual	void	DrawWorkRect(CDC*);
	virtual	void	DrawWorkCylinder(CDC*);

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B
	//{{AFX_VIRTUAL(CNCView)
	public:
	virtual void OnInitialUpdate();
	virtual void OnDraw(CDC* pDC);  // ���̃r���[��`�悷��ۂɃI�[�o�[���C�h����܂��B
	protected:
	//}}AFX_VIRTUAL

// �������ꂽ���b�Z�[�W �}�b�v�֐�
protected:
	//{{AFX_MSG(CNCView)
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
