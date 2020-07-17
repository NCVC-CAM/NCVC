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

	WORD		m_nMenuID;	// �ƭ�ID (from CNCVCApp)
	WORD		m_nImage;	// �Ұ��ؽć� (from CMainFrame)

	BOOL		m_bDlgAdd,	// CExecSetupDlg��Add���ꂽ��޼ު��
				m_bDlgDel;	//      �V        Del

public:
	CExecOption(const CString&);
	CExecOption(const CString&, const CString&, const CString&,
			BOOL, BOOL, BOOL);				// �޲�۸ނ���̐���
	CExecOption(const CString&, DOCTYPE);	// ���ް�ޮ݈ڍs�p

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
