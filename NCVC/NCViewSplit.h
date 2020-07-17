// NCViewSplit.h: CNCViewSplit �N���X�̃C���^�[�t�F�C�X
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NCdata.h"		// NCDRAWVIEW_NUM
#define		NCVIEW_OPENGL			6

//////////////////////////////////////////////////////////////////////

class CNCViewSplit : public CSplitterWnd  
{
	HDC		m_hDC[NCDRAWVIEW_NUM];	// ���د�����޳�ŊǗ�����
									// XYZ, XY, XZ, YZ �e�߲݂����޲���÷�������

	void	CalcPane(int, BOOL = FALSE);	// �e�߲ݗ̈�̌v�Z
	void	AllPane_PostMessage(int, UINT, WPARAM = 0, LPARAM = 0);

public:
	CNCViewSplit();

// �I�y���[�V����
public:
	void	DrawData(CNCdata*, BOOL, PFNNCDRAWPROC[]);	// from NCViewTab.cpp(CTraceThread)

// �I�[�o�[���C�h

// �������ꂽ���b�Z�[�W �}�b�v�֐�
protected:
	afx_msg void OnDestroy();
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	// CNCViewTab::OnInitialUpdate() ���� PostMessage()
	afx_msg LRESULT OnUserInitialUpdate(WPARAM, LPARAM);
	// CNCViewTab::OnActivatePage() ���� SendMessage()
	afx_msg LRESULT OnUserActivatePage(WPARAM, LPARAM);
	// �S�Ă��߲݂̐}�`̨��
	afx_msg	void	OnAllFitCmd();
	// �e�ޭ��ւ�̨��ү����
	afx_msg LRESULT OnUserViewFitMsg(WPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()
};
