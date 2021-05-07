
// TestProject.h : TestProject アプリケーションのメイン ヘッダー ファイル
//
#pragma once

#ifndef __AFXWIN_H__
	#error "PCH に対してこのファイルをインクルードする前に 'stdafx.h' をインクルードしてください"
#endif

#include "resource.h"       // メイン シンボル


// CTestProjectApp:
// このクラスの実装については、TestProject.cpp を参照してください。
//

class CTestProjectApp : public CWinApp
{
#ifdef _DEBUG
	void	DebugCode(void);
#endif

public:
	CTestProjectApp();


// オーバーライド
public:
	virtual BOOL InitInstance();

// 実装
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
};

extern CTestProjectApp theApp;
