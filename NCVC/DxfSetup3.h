// DxfSetup1.h : ヘッダー ファイル
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CDxfSetup3 ダイアログ

class CDxfSetup3 : public CPropertyPage
{
	void	EnableReloadButton(void);

public:
	CDxfSetup3();

// ダイアログ データ
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DXF_SETUP3 };
#endif
	CString		m_strIgnore;
	CIntEdit	m_nSplineNum;

	virtual BOOL OnSetActive();
	virtual BOOL OnKillActive();
	virtual BOOL OnApply();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();
	afx_msg void OnReload();

	DECLARE_MESSAGE_MAP()
};
