// ChildBase.h: CChildBase �N���X�̃C���^�[�t�F�C�X
//
//////////////////////////////////////////////////////////////////////

#pragma once

class CChildBase  
{
	BOOL	m_bNotify;	// ̧�ٕύX�ʒm��������?

protected:
	CChildBase() {
		m_bNotify = FALSE;
	}

protected:
	// ү����ϯ�߂̋��ʕ���
	int		ActivateFrame(int nCmdShow);
	void	OnMDIActivate(CMDIChildWnd*, BOOL bActivate);
	void	OnUserFileChangeNotify(CMDIChildWnd*);
};
