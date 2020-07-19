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

// CDXFDoc �׸�
enum DXFDOCFLG {
	DXFDOC_READY = 0,	// NC�����\���ǂ���(�װ�׸�)
	DXFDOC_RELOAD,		// �ēǍ��׸�(from DXFSetup.cpp)
	DXFDOC_THREAD,		// �گ�ތp���׸�
	DXFDOC_BINDPARENT,	// ����Ӱ��
	DXFDOC_BIND,		// ����Ӱ��(�q)
	DXFDOC_SHAPE,		// �`�󏈗����s������
	DXFDOC_LATHE,		// ���՗p�̌��_(ܰ��a�ƒ[��)��ǂݍ��񂾂�
	DXFDOC_WIRE,		// ܲԉ��H�@�p�̐������\���ǂ���
		DXFDOC_FLGNUM		// �׸ނ̐�[8]
};

// CNCDoc �׸�
enum NCDOCFLG {
	NCDOC_ERROR = 0,	// �޷���ēǂݍ��ݴװ
	NCDOC_CUTCALC,		// �؍펞�Ԍv�Z�گ�ތp���׸�
	NCDOC_REVISEING,	// �␳�v�Z�s�����ǂ���
	NCDOC_COMMENTWORK,	// ���Ă�ܰ���`���w�����ꂽ
		NCDOC_COMMENTWORK_R,
		NCDOC_COMMENTWORK_Z,
		NCDOC_CYLINDER,		// �ى��H�̉~��Ӱ��
		NCDOC_WORKFILE,		// �O��̧�ق̓ǂݍ���
	NCDOC_MAXRECT,		// �ő�ړ���`�̕`��
	NCDOC_WRKRECT,		// ܰ���`�̕`��
	NCDOC_THUMBNAIL,	// ��Ȳٕ\��Ӱ��
	NCDOC_LATHE,		// NC����Ӱ��
		NCDOC_LATHE_INSIDE,	// ��������H�A��
		NCDOC_LATHE_HOLE,	// ���Ē���
	NCDOC_WIRE,			// ܲԉ��HӰ��
	NCDOC_MC_CHANGE,	// ���Ăŋ@�B���̕ύX
		NCDOC_FLGNUM		// �׸ނ̐�[16]
};
#define	DOCFLG_NUM	NCDOC_FLGNUM	// �傫����

// UAV_DXFADDSHAPE ����
class	CLayerData;
class	CDXFshape;
struct DXFADDSHAPE
{
	CLayerData*	pLayer;
	CDXFshape*	pShape;
};
typedef	DXFADDSHAPE*	LPDXFADDSHAPE;

// ���݂̕\����
struct DXFVIEWINFO
{
	CPoint		ptOrg;		// �_�����W���_
	float		dFactor;	// �g�嗦
};

// ̧�ٕύX�ʒm�̊Ď��گ��
struct FNCNGTHREADPARAM
{
	LPCTSTR		lpstrFileName;	// ���̧߽�ٖ�
	HWND		hWndFrame;		// �ύX�ʒm�̑��M��
	HANDLE		hFinish;		// �I���ʒm����������
};
typedef	FNCNGTHREADPARAM*	LPFNCNGTHREADPARAM;

//	̧�ٕύX�ʒm�̊Ď��گ��
UINT	FileChangeNotificationThread(LPVOID pParam);

/////////////////////////////////////////////////////////////////////////////
// CDocBase

class CDocBase : public CDocument
{
	// ̧�ٕύX�ʒm�گ��
	CWinThread*	m_pFileChangeThread;
	HANDLE		m_hAddinThread;			// �گ��ۯ������
	CEvent		m_evFinish;				// �I���ʒm�����

protected:
	std::bitset<DOCFLG_NUM>	m_bDocFlg;	// �h���׽�p�޷�����׸�
	CRect3F			m_rcMax;	// �޷���Ă̵�޼ު�čő��`

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
#ifdef _DEBUG
	virtual void AssertValid() const {
		__super::AssertValid();
	}
	virtual void Dump(CDumpContext& dc) const {
		__super::Dump(dc);
	}
#endif

protected:
	BOOL	OnOpenDocumentSP(LPCTSTR lpstrFileName, CFrameWnd* pWnd);
	void	OnCloseDocumentSP(void);
	BOOL	UpdateModifiedTitle(BOOL bModified, CString& strTitle);

public:
	CRect3F	GetMaxRect(void) const {
		return m_rcMax;
	}
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
	//
	virtual void ReportSaveLoadException(LPCTSTR lpszPathName, CException* e, BOOL bSaving, UINT nIDPDefault);
	// �X�Vϰ��t�^
	virtual void SetModifiedFlag(BOOL bModified = TRUE);
};
