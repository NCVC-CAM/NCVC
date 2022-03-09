// 3dOption.h: 3�������f���؍�̵�߼�݊Ǘ�
//
//////////////////////////////////////////////////////////////////////

#pragma once

enum {		// int�^
	D3_INT_LINESPLIT,
		D3_INT_NUMS		// [1]
};
enum {		// float�^
	D3_DBL_BALLENDMILL = 0,
	D3_DBL_HEIGHT,
	D3_DBL_ZCUT,
		D3_DBL_NUMS		// [3]
};

class C3dOption
{
friend	class	C3dScanSetupDlg;

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
			float	m_dBallEndmill,		// �{�[���G���h�~�����a
					m_dWorkHeight,		// ���[�N�̍���
					m_dZCut;			// 1��̐؂荞�ݗ�
		};
		float		m_udNums[D3_DBL_NUMS];
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
};
