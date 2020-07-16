// ChildBase.h: CChildBase クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once

class CChildBase  
{
	BOOL	m_bNotify;	// ﾌｧｲﾙ変更通知処理中か?

protected:
	CChildBase() {
		m_bNotify = FALSE;
	}

protected:
	// ﾒｯｾｰｼﾞﾏｯﾌﾟの共通部分
	int		ActivateFrame(int nCmdShow);
	void	OnMDIActivate(CMDIChildWnd*, BOOL bActivate);
	void	OnUserFileChangeNotify(CMDIChildWnd*);
};
