// ExecOption.h: CExecOption クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once

class CExecOption
{
friend	class	CExecSetupDlg;

	CString		m_strFileName,
				m_strCommand,
				m_strToolTip;
	BOOL		m_bNCType;
	BOOL		m_bDXFType;
	BOOL		m_bShort;

	WORD		m_nMenuID;	// ﾒﾆｭｰID (from CNCVCApp)
	WORD		m_nImage;	// ｲﾒｰｼﾞﾘｽﾄ№ (from CMainFrame)

	BOOL		m_bDlgAdd,	// CExecSetupDlgでAddされたｵﾌﾞｼﾞｪｸﾄ
				m_bDlgDel;	//      〃        Del

public:
	CExecOption(const CString&);
	CExecOption(const CString&, const CString&, const CString&,
			BOOL, BOOL, BOOL);				// ﾀﾞｲｱﾛｸﾞからの生成
	CExecOption(const CString&, DOCTYPE);	// 旧ﾊﾞｰｼﾞｮﾝ移行用

	CString		GetStringData(void) const;	// ﾚｼﾞｽﾄﾘへ格納するための文字列を生成

	CString		GetFileName(void) const {
		return m_strFileName;
	}
	CString		GetCommand(void) const {
		return m_strCommand;
	}
	CString		GetToolTip(void) const {
		return m_strToolTip;
	}
	BOOL		IsNCType(void) const {
		return m_bNCType;
	}
	BOOL		IsDXFType(void) const {
		return m_bDXFType;
	}
	BOOL		IsShort(void) const {
		return m_bShort;
	}
	WORD		GetMenuID(void) const {
		return m_nMenuID;
	}
	void		SetMenuID(WORD nID) {
		m_nMenuID = nID;
	}
	WORD		GetImageNo(void) const {
		return m_nImage;
	}
	void		SetImageNo(WORD nImage) {
		m_nImage = nImage;
	}
};

typedef CTypedPtrList<CPtrList, CExecOption*>	CExecList;
