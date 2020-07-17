// ChildBase.h: CChildBase �N���X�̃C���^�[�t�F�C�X
//
//////////////////////////////////////////////////////////////////////

#pragma once

class CChildBase : public CMDIChildWnd
{
protected:
	BOOL	m_bNotify;	// ̧�ٕύX�ʒm��������?

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
	// ̧�ٕύX�ʒm from DocBase.cpp
	afx_msg LRESULT OnUserFileChangeNotify(WPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()
};
