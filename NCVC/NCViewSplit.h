// NCViewSplit.h: CNCViewSplit クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NCdata.h"		// NCDRAWVIEW_NUM
#define		NCVIEW_OPENGL			6

//////////////////////////////////////////////////////////////////////

class CNCViewSplit : public CSplitterWnd  
{
	HDC		m_hDC[NCDRAWVIEW_NUM];	// ｽﾌﾟﾘｯﾀｳｨﾝﾄﾞｳで管理する
									// XYZ, XY, XZ, YZ 各ﾍﾟｲﾝのﾃﾞﾊﾞｲｽｺﾝﾃｷｽﾄﾊﾝﾄﾞﾙ

	void	CalcPane(int, BOOL = FALSE);	// 各ﾍﾟｲﾝ領域の計算
	void	AllPane_PostMessage(int, UINT, WPARAM = 0, LPARAM = 0);

public:
	CNCViewSplit();

// オペレーション
public:
	void	DrawData(CNCdata*, BOOL, PFNNCDRAWPROC[]);	// from NCViewTab.cpp(CTraceThread)

// オーバーライド

// 生成されたメッセージ マップ関数
protected:
	afx_msg void OnDestroy();
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	// CNCViewTab::OnInitialUpdate() から PostMessage()
	afx_msg LRESULT OnUserInitialUpdate(WPARAM, LPARAM);
	// CNCViewTab::OnActivatePage() から SendMessage()
	afx_msg LRESULT OnUserActivatePage(WPARAM, LPARAM);
	// 全てのﾍﾟｲﾝの図形ﾌｨｯﾄ
	afx_msg	void	OnAllFitCmd();
	// 各ﾋﾞｭｰへのﾌｨｯﾄﾒｯｾｰｼﾞ
	afx_msg LRESULT OnUserViewFitMsg(WPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()
};
