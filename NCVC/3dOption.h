// 3dOption.h: 3�������f���؍�̵�߼�݊Ǘ�
//
//////////////////////////////////////////////////////////////////////

#pragma once

enum {		// int�^
	D3_INT_LINESPLIT,
		D3_INT_NUMS		// [1]
};
enum {		// float�^
	D3_DBL_ROUGH_BALLENDMILL = 0,
	D3_DBL_WORKHEIGHT,
	D3_DBL_ROUGH_ZCUT,
	D3_DBL_CONTOUR_BALLENDMILL,
	D3_DBL_CONTOUR_SPACE,
	D3_DBL_CONTOUR_ZMIN,
	D3_DBL_CONTOUR_ZMAX,
	D3_DBL_CONTOUR_SHIFT,
		D3_DBL_NUMS		// [8]
};
enum {		// BOOL�^
	D3_FLG_ROUGH_ZORIGIN = 0,
	D3_FLG_CONTOUR_ZORIGIN,
		D3_FLG_NUMS		// [2]
};

class C3dOption
{
friend	class	C3dRoughScanSetupDlg;
friend	class	C3dContourScanSetupDlg;

	CString		m_str3dOptionFile;

	// int�^��߼��
	union {
		struct {
			int		m_nLineSplit;			// �X�L���j���O���C��������
		};
		int			m_unNums[D3_INT_NUMS];
	};
	// float�^��߼��
	union {
		struct {
			float	m_dRoughBallEndmill,	// �r���H�p�{�[���G���h�~�����a
					m_dWorkHeight,			// ���[�N�̍���
					m_dRoughZCut,			// �r���H�̐؂荞�ݗ�
					m_dContourBallEndmill,	// �������p�{�[���G���h�~�����a
					m_dContourSpace,		// �������̓_�Ԋu
					m_dContourZmin,			// ���������X�L��������Z�l
					m_dContourZmax,
					m_dContourShift;		// �������̕��ʃV�t�g��
		};
		float		m_udNums[D3_DBL_NUMS];
	};
	// BOOL�^��߼��
	union {
		struct {
			int		m_bRoughZOrigin,		// ���[�N��ʂ�Z�����_�ɂ���
					m_bContourZOrigin;
		};
		int			m_ubFlgs[D3_FLG_NUMS];
	};

public:
	C3dOption();
	~C3dOption();
	BOOL	Read3dOption(LPCTSTR);
	BOOL	Save3dOption(void);

	// CNCMakeOption �Ɗ֐��������� TH_MakeXXX�n�� #define �Ńo�b�e�B���O����
	int		Get3dInt(size_t n) const {
		ASSERT( n>=0 && n<SIZEOF(m_unNums) );
		return m_unNums[n];
	}
	float	Get3dDbl(size_t n) const {
		ASSERT( n>=0 && n<SIZEOF(m_udNums) );
		return m_udNums[n];
	}
	BOOL	Get3dFlg(size_t n) const {
		ASSERT( n>=0 && n<SIZEOF(m_ubFlgs) );
		return m_ubFlgs[n];
	}
};
