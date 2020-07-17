// ChildBase.h: CChildBase クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once

class CChildBase : public CMDIChildWnd
{
protected:
	BOOL	m_bNotify;	// ﾌｧｲﾙ変更通知処理中か?

	CChildBase() {
		m_bNotify = FALSE;
	}
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

public:
	virtual void ActivateFrame(int nCmdShow = -1);

protected:
	afx_msg void OnClose();
	// ﾌｧｲﾙ変更通知 from DocBase.cpp
	afx_msg LRESULT OnUserFileChangeNotify(WPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()
};
