// DXFOption.h: DXF��߼�݂̊Ǘ�
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NCVCdefine.h"

enum {
	DXFOPT_VIEW = 0,
	DXFOPT_FILECOMMENT,
		DXFOPT_FLGS			// [2]
};
enum {
	DXFOPT_ORGTYPE = 0,
	DXFOPT_BINDORG,
	DXFOPT_BINDSORT,
		DXFOPT_NUMS			// [3]
};
enum {
	DXFOPT_BINDWORKX = 0,
	DXFOPT_BINDWORKY,
	DXFOPT_BINDMARGIN,
		DXFOPT_DBL_NUMS		// [3]
};
enum	enMAKETYPE	{
	NCMAKEMILL = 0,		// MC
	NCMAKELATHE,		// ����
	NCMAKEWIRE,			// ܲԕ��d���H�@
	NCMAKELAYER,		// ڲԖ��Ə���̧�ق̊֌W̧�ق̗���
		NCMAKENUM			// [4]
};

class CDXFOption
{
friend	class	CDxfSetup1;
friend	class	CDxfSetup2;
friend	class	CCADbindDlg;
friend	class	CMakeBindOptDlg;

	union {
		struct {
			BOOL	m_bView,		// �ϊ����ޭ��N��
					m_bFileComment;	// ������̧�ق��Ƃɺ��Ă𖄂ߍ���
		};
		BOOL		m_ubNums[DXFOPT_FLGS];
	};
	union {
		struct {
			int		m_nOrgType,		// ���_ڲԂ������Ƃ��̏���
									//    0:�װ,  1�`4:�E��,�E��,����,����, 5:����
					m_nBindOrg,		// CAD�ް��������̉��H���_
									//    0�`3:�E��,�E��,����,����, 4:����
					m_nBindSort;	// �������̕��בւ�
		};
		int			m_unNums[DXFOPT_NUMS];
	};
	union {
		struct {
			float	m_dBindWork[2],	// CAD�ް��̓���ܰ�����
					m_dBindMargin;	// �z�uϰ���
		};
		float		m_udNums[DXFOPT_DBL_NUMS];
	};
	CString	m_strReadLayer[DXFLAYERSIZE];	// ���_�C�؍�(���ͲҰ�ޕۑ��p)�C
											// ���H�J�n�ʒuڲԖ�, �����ړ��w��ڲԖ�, ���ėp
	CStringList	m_strInitList[NCMAKENUM];	// �؍����̧�ٖ��̗���
	enMAKETYPE	m_enMakeType;				// ���O��NC��������
	boost::regex	m_regCutter;			// �؍�ڲԐ��K�\��

	BOOL	AddListHistory(enMAKETYPE, LPCTSTR);
	BOOL	ReadInitHistory(enMAKETYPE);
	BOOL	SaveInitHistory(enMAKETYPE);

public:
	CDXFOption();
	BOOL	SaveDXFoption(void);		// ڼ޽�؂ւ̕ۑ�
	BOOL	SaveBindOption(void);

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
	//
	BOOL	GetDxfOptFlg(size_t n) const {
		ASSERT( n>=0 && n<SIZEOF(m_ubNums) );
		return m_ubNums[n];
	}
	int		GetDxfOptNum(size_t n) const {
		ASSERT( n>=0 && n<SIZEOF(m_unNums) );
		return m_unNums[n];
	}
	void	SetViewFlag(BOOL bView) {
		m_bView = bView;
	}
	enMAKETYPE	GetNCMakeType(void) const {
		return m_enMakeType;
	}
	const	CStringList*	GetInitList(enMAKETYPE enType) const {
		return &m_strInitList[enType];
	}

	BOOL	AddInitHistory(enMAKETYPE enType, LPCTSTR lpszSearch) {
		if ( AddListHistory(enType, lpszSearch) )
			return SaveInitHistory(enType);
		return FALSE;
	}
	void	DelInitHistory(enMAKETYPE, LPCTSTR);
	//
	float	GetBindSize(size_t n) const {
		return m_dBindWork[n];
	}
	float	GetBindMargin(void) const {
		return m_dBindMargin;
	}
	//
	// from DXFMakeOption.cpp, NCVCaddinDXF.cpp
	CString	GetReadLayer(size_t n) const {
		ASSERT( n>=0 && n<DXFLAYERSIZE );
		return m_strReadLayer[n];
	}
};
