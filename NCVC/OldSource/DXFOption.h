// DXFOption.h: DXF��߼�݂̊Ǘ�
//
//////////////////////////////////////////////////////////////////////

#if !defined(_DXFOPTION__INCLUDED_)
#define _DXFOPTION__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "NCVCdefine.h"

/*
	DXFOPT_REGEX	: �]���ʂ�(0)���C���K�\����(1)
	DXFOPT_MATCH	: ���S��v(0)���C������v(1)
	DXFOPT_ACCEPT	: �Ώ�ڲԂ�F������(0)���C���O���邩(1)
	DXFOPT_VIEW		: �ϊ����ޭ��N��
	DXFOPT_ORGTYPE	: ���_ڲԂ������Ƃ��̏���
							0:�װ, 1�`4:�E��,�E��,����,����, 5:����
*/
#define	DXFOPT_REGEX	0
#define	DXFOPT_MATCH	1
#define	DXFOPT_ACCEPT	2
#define	DXFOPT_VIEW		3
#define	DXFOPT_ORGTYPE	4

class	CDXFOption;
// �؍�ڲԖ��̔F������݊֐�����������
typedef BOOL (*PFNISCUTTERLAYER)(const CDXFOption*, LPCTSTR);

class CDXFOption
{
friend	class	CDxfSetup1;
friend	class	CDxfSetup2;

	CString	m_strReadLayer[DXFLAYERSIZE],	// ���_�C�؍�(���ͲҰ�ޕۑ��p)�C
											// ���H�J�n�ʒuڲԖ�, �����ړ��w��ڲԖ�,
											// ���ėp
			m_strCutterFirst;	// ����ڲԂ̂P��(CMapStringToOb��Seq�����s��)
	CMapStringToOb	m_strCutterMap;	// ����ڲԊǗ��p
	int			m_nDXF[5];		// Regex, Match, Accept, View, OrgType
	CStringList	m_strInitList;	// �؍����̧�ٖ��̗���
	CStringList	m_strLayerToInitList;	// ڲԖ��Ə���̧�ق̊֌W̧�ق̗���

	void	SetCutterList(void);
	BOOL	AddListHistory(CStringList&, LPCTSTR);
	void	DelListHistory(CStringList&, LPCTSTR);

	// �؍�ڲԖ��̔F�������
	PFNISCUTTERLAYER	m_pfnIsCutterLayer;
	void	SetCallingLayerFunction(void);
	static	BOOL	IsAllMatch(const CDXFOption*, LPCTSTR);
	static	BOOL	IsPerfectMatch(const CDXFOption*, LPCTSTR);
	static	BOOL	IsPartMatch(const CDXFOption*, LPCTSTR);
	static	BOOL	IsPerfectNotMatch(const CDXFOption*, LPCTSTR);
	static	BOOL	IsPartNotMatch(const CDXFOption*, LPCTSTR);
	static	BOOL	IsRegex(const CDXFOption*, LPCTSTR);

public:
	CDXFOption();
	BOOL	SaveDXFoption(void);	// ڼ޽�؂ւ̕ۑ�(���L�܂�)
	BOOL	SaveInitHistory(void);	// ����̧�ق̗��������ۑ�

	BOOL	IsOriginLayer(LPCTSTR lpszOrigin) const {
		return (m_strReadLayer[DXFORGLAYER] == lpszOrigin);
	}
	BOOL	IsCutterLayer(LPCTSTR lpszCutter) const {
		ASSERT( m_pfnIsCutterLayer );
		return (*m_pfnIsCutterLayer)(this, lpszCutter);
	}
	BOOL	IsStartLayer(LPCTSTR lpszStart) const {
		return m_strReadLayer[DXFSTRLAYER].IsEmpty() ?
			FALSE : (m_strReadLayer[DXFSTRLAYER] == lpszStart);
	}
	BOOL	IsMoveLayer(LPCTSTR lpszMove) const {
		return m_strReadLayer[DXFMOVLAYER].IsEmpty() ?
			FALSE : (m_strReadLayer[DXFMOVLAYER] == lpszMove);
	}
	BOOL	IsCommentLayer(LPCTSTR lpszComment) const {
		return m_strReadLayer[DXFCOMLAYER].IsEmpty() ?
			FALSE : (m_strReadLayer[DXFCOMLAYER] == lpszComment);
	}

	int		GetDxfFlag(size_t n) const {
		ASSERT( n>=0 && n<SIZEOF(m_nDXF) );
		return m_nDXF[n];
	}
	void	SetViewFlag(BOOL bView) {
		m_nDXF[DXFOPT_VIEW] = bView;
	}

	const	CStringList*	GetInitList(void) const {
		return &m_strInitList;
	}
	const	CStringList*	GetLayerToInitList(void) const {
		return &m_strLayerToInitList;
	}
	BOOL	AddInitHistory(LPCTSTR lpszSearch) {
		return AddListHistory(m_strInitList, lpszSearch);
	}
	BOOL	AddLayerHistory(LPCTSTR lpszSearch) {
		return AddListHistory(m_strLayerToInitList, lpszSearch);
	}
	void	DelInitHistory(LPCTSTR lpszSearch) {
		DelListHistory(m_strInitList, lpszSearch);
	}
	void	DelLayerHistory(LPCTSTR lpszSearch) {
		DelListHistory(m_strLayerToInitList, lpszSearch);
	}

	// from DXFMakeOption.cpp, NCVCaddinDXF.cpp
	CString	GetReadLayer(size_t n) const {
		ASSERT( n>=0 && n<DXFLAYERSIZE );
		return m_strReadLayer[n];
	}
	CString	GetCutterFirst(void) const {
		return m_strCutterFirst;
	}
};

#endif
