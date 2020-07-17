// DocBase.h : �w�b�_�[ �t�@�C��
//

#pragma once

// UpdateAllViews() LPARAM ������
#define	UAV_STARTSTOPTRACE		-1
#define	UAV_TRACECURSOR			-2
#define	UAV_FILEINSERT			-3
#define	UAV_FILESAVE			-4
#define	UAV_DRAWWORKRECT		-5
#define	UAV_DRAWMAXRECT			-6
#define	UAV_DXFORGUPDATE		-7
#define	UAV_DXFSHAPEID			-8
#define	UAV_DXFSHAPEUPDATE		-9
#define	UAV_DXFAUTOWORKING		-10
#define	UAV_DXFAUTODELWORKING	-11
#define	UAV_DXFADDWORKING		-12
#define	UAV_DXFADDSHAPE			-13
#define	UAV_CHANGEFONT			-90
#define	UAV_ADDINREDRAW			-99

// UAV_DXFADDSHAPE ����
class	CLayerData;
class	CDXFshape;
typedef	struct	tagDXFADDSHAPE {
	CLayerData*	pLayer;
	CDXFshape*	pShape;
} DXFADDSHAPE, *LPDXFADDSHAPE;

// ���݂̕\����
typedef	struct	tagDXFVIEWINFO {
	CPoint		ptOrg;		// �_�����W���_
	double		dFactor;	// �g�嗦
} DXFVIEWINFO, *LPDXFVIEWINFO;

// ̧�ٕύX�ʒm�̊Ď��گ��
typedef	struct	tagFNCNGTHREADPARAM {
	LPCTSTR		lpstrFileName;	// ���̧߽�ٖ�
	HWND		hWndFrame;		// �ύX�ʒm�̑��M��
	HANDLE		hFinish;		// �I���ʒm����������
} FNCNGTHREADPARAM, *LPFNCNGTHREADPARAM;

//	̧�ٕύX�ʒm�̊Ď��گ��
UINT	FileChangeNotificationThread(LPVOID pParam);

/////////////////////////////////////////////////////////////////////////////
// CDocBase

template<size_t T> class CDocBase
{
	// ̧�ٕύX�ʒm�گ��
	CWinThread*	m_pFileChangeThread;
	HANDLE		m_hAddinThread;			// �گ��ۯ������
	CEvent		m_evFinish;				// �I���ʒm�����

protected:
	// �h���׽�p�޷�����׸�
	std::bitset<T>	m_bDocFlg;

	// ��޲ݼري֐��̕ێ�
	PFNNCVCSERIALIZEFUNC	m_pfnSerialFunc;
	// ��޲݌���ۯ������
	BOOL	IsLockThread(void);

protected:
	CDocBase() {
		UnlockDocument();
		m_pFileChangeThread = NULL;
		m_pfnSerialFunc = NULL;
		m_bDocFlg.reset();
	}

protected:
	BOOL	OnOpenDocumentSP(LPCTSTR lpstrFileName, CFrameWnd* pWnd);
	void	OnCloseDocumentSP(void);
	BOOL	UpdateModifiedTitle(BOOL bModified, CString& strTitle);

public:
	BOOL	IsDocFlag(size_t n) const {
		ASSERT( n < m_bDocFlg.size() );
		return m_bDocFlg[n];
	}
	void	SetDocFlag(size_t n, BOOL val = TRUE) {
		ASSERT( n < m_bDocFlg.size() );
		m_bDocFlg.set(n, val);
	}
	void	LockDocument(HANDLE hThread) {
		m_hAddinThread = hThread;
	}
	void	UnlockDocument(void) {
		m_hAddinThread = NULL;
	}
	PFNNCVCSERIALIZEFUNC	GetSerializeFunc(void) {
		return m_pfnSerialFunc;
	}
	HANDLE	GetLockHandle(void) {
		return m_hAddinThread;
	}
};

#include "DocBase.inl"
