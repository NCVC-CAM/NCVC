
// TestProject.h : TestProject �A�v���P�[�V�����̃��C�� �w�b�_�[ �t�@�C��
//
#pragma once

#ifndef __AFXWIN_H__
	#error "PCH �ɑ΂��Ă��̃t�@�C�����C���N���[�h����O�� 'stdafx.h' ���C���N���[�h���Ă�������"
#endif

#include "resource.h"       // ���C�� �V���{��


// CTestProjectApp:
// ���̃N���X�̎����ɂ��ẮATestProject.cpp ���Q�Ƃ��Ă��������B
//

class CTestProjectApp : public CWinApp
{
#ifdef _DEBUG
	void	DebugCode(void);
#endif

public:
	CTestProjectApp();


// �I�[�o�[���C�h
public:
	virtual BOOL InitInstance();

// ����
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
};

extern CTestProjectApp theApp;
