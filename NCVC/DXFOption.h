// DXFOption.h: DXF��߼�݂̊Ǘ�
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NCVCdefine.h"

/*
	DXFOPT_VIEW		: �ϊ����ޭ��N��
	DXFOPT_ORGTYPE	: ���_ڲԂ������Ƃ��̏���
							0:�װ, 1�`4:�E��,�E��,����,����, 5:����
*/
#define	DXFOPT_VIEW		0
#define	DXFOPT_ORGTYPE	1
enum	eMAKETYPE	{NCMAKEMILL, NCMAKELATHE};

class CDXFOption
{
friend	class	CDxfSetup1;
friend	class	CDxfSetup2;

	CString	m_strReadLayer[DXFLAYERSIZE];	// ���_�C�؍�(���ͲҰ�ޕۑ��p)�C
											// ���H�J�n�ʒuڲԖ�, �����ړ��w��ڲԖ�, ���ėp
	boost::regex	m_regCutter;			// �؍�ڲԐ��K�\��
	int			m_nDXF[2];			// View, OrgType
	CStringList	m_strMillList,			// �؍����̧�ٖ��̗���
				m_strLatheList,			// ���՗p�؍����̧�ٖ��̗���
				m_strLayerToInitList;	// ڲԖ��Ə���̧�ق̊֌W̧�ق̗���
	eMAKETYPE	m_enMakeType;		// ���O��NC��������

	BOOL	AddListHistory(CStringList&, LPCTSTR);
	void	DelListHistory(CStringList&, LPCTSTR);
	BOOL	ReadInitHistory(eMAKETYPE);
	BOOL	SaveInitHistory(eMAKETYPE);
	BOOL	SaveLayerHistory(void);

public:
	CDXFOption();
	BOOL	SaveDXFoption(void);		// ڼ޽�؂ւ̕ۑ�(���L�܂�)

	BOOL	IsOriginLayer(LPCTSTR lpszLayer) const {
		return (m_strReadLayer[DXFORGLAYER] == lpszLayer);
	}
	BOOL	IsCutterLayer(LPCTSTR lpszLayer) const {
		return boost::regex_search(lpszLayer, m_regCutter);
	}
	BOOL	IsStartLayer(LPCTSTR lpszLayer) const {
		return m_strReadLayer[DXFSTRLAYER].IsEmpty() ?
			FALSE : (m_strReadLayer[DXFSTRLAYER] == lpszLayer);
	}
	BOOL	IsMoveLayer(LPCTSTR lpszLayer) const {
		return m_strReadLayer[DXFMOVLAYER].IsEmpty() ?
			FALSE : (m_strReadLayer[DXFMOVLAYER] == lpszLayer);
	}
	BOOL	IsCommentLayer(LPCTSTR lpszLayer) const {
		return m_strReadLayer[DXFCOMLAYER].IsEmpty() ?
			FALSE : (m_strReadLayer[DXFCOMLAYER] == lpszLayer);
	}

	int		GetDxfFlag(size_t n) const {
		ASSERT( n>=0 && n<SIZEOF(m_nDXF) );
		return m_nDXF[n];
	}
	void	SetViewFlag(BOOL bView) {
		m_nDXF[DXFOPT_VIEW] = bView;
	}
	eMAKETYPE	GetNCMakeType(void) const {
		return m_enMakeType;
	}

	const	CStringList*	GetMillInitList(void) const {
		return &m_strMillList;
	}
	const	CStringList*	GetLatheInitList(void) const {
		return &m_strLatheList;
	}
	const	CStringList*	GetLayerToInitList(void) const {
		return &m_strLayerToInitList;
	}
	BOOL	AddMillInitHistory(LPCTSTR lpszSearch) {
		if ( AddListHistory(m_strMillList, lpszSearch) )
			return SaveInitHistory(NCMAKEMILL);
		return FALSE;
	}
	BOOL	AddLatheInitHistory(LPCTSTR lpszSearch) {
		if ( AddListHistory(m_strLatheList, lpszSearch) )
			return SaveInitHistory(NCMAKELATHE);
		return FALSE;
	}
	BOOL	AddLayerHistory(LPCTSTR lpszSearch) {
		if ( AddListHistory(m_strLayerToInitList, lpszSearch) )
			return SaveLayerHistory();
		return FALSE;
	}
	void	DelMillInitHistory(LPCTSTR lpszSearch) {
		DelListHistory(m_strMillList, lpszSearch);
	}
	void	DelLatheInitHistory(LPCTSTR lpszSearch) {
		DelListHistory(m_strLatheList, lpszSearch);
	}
	void	DelLayerHistory(LPCTSTR lpszSearch) {
		DelListHistory(m_strLayerToInitList, lpszSearch);
	}

	// from DXFMakeOption.cpp, NCVCaddinDXF.cpp
	CString	GetReadLayer(size_t n) const {
		ASSERT( n>=0 && n<DXFLAYERSIZE );
		return m_strReadLayer[n];
	}
};
