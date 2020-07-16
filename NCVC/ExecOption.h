// ExecOption.h: CExecOption �N���X�̃C���^�[�t�F�C�X
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

	int			m_nMenuID;	// �ƭ�ID (from CNCVCApp)
	int			m_nImage;	// �Ұ��ؽć� (from CMainFrame)

	BOOL		m_bDlgAdd,	// CExecSetupDlg��Add���ꂽ��޼ު��
				m_bDlgDel;	//      �V        Del

public:
	CExecOption(const CString&);
	CExecOption(const CString&, const CString&, const CString&,
			BOOL, BOOL, BOOL);				// �޲�۸ނ���̐���
	CExecOption(const CString&, DOCTYPE);	// ���ް�ޮ݈ڍs�p
	virtual ~CExecOption();

	CString		GetStringData(void) const;	// ڼ޽�؂֊i�[���邽�߂̕�����𐶐�

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
	int			GetMenuID(void) const {
		return m_nMenuID;
	}
	void		SetMenuID(int nID) {
		m_nMenuID = nID;
	}
	int			GetImageNo(void) const {
		return m_nImage;
	}
	void		SetImageNo(int nImage) {
		m_nImage = nImage;
	}
};

typedef CTypedPtrList<CPtrList, CExecOption*>	CExecList;
