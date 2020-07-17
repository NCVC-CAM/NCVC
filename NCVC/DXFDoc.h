// DXFDoc.h : �w�b�_�[ �t�@�C��
//

#pragma once

#include "DocBase.h"
#include "DXFdata.h"
#include "DXFshape.h"
#include "Layer.h"
#include "NCVCdefine.h"

class	CDXFBlockData;

// CDXFDoc �׸�
enum DXFDOCFLG {
	DXFDOC_READY = 0,		// NC�����\���ǂ���(�װ�׸�)
	DXFDOC_RELOAD,			// �ēǍ��׸�(from DXFSetup.cpp)
	DXFDOC_THREAD,			// �گ�ތp���׸�
	DXFDOC_SHAPE,			// �`�󏈗����s������
	DXFDOC_LATHE,			// ���՗p�̌��_(ܰ��a�ƒ[��)��ǂݍ��񂾂�
		DXFDOC_FLGNUM			// �׸ނ̐�[5]
};

// ���������ް��󂯓n���\����
struct	AUTOWORKINGDATA
{
	int		nSelect;		// �@�\�I��(0:�֊s, 1:�̾��)
	double	dOffset;		// �̾��
	BOOL	bAcuteRound;	// �s�p�̊O�����ۂ߂�
	int		nLoopCnt;		// �J��Ԃ���
	int		nScanLine;		// ������(0:�Ȃ�, 1:X����, 2:Y����)
	BOOL	bCircleScroll;	// �~�ް��ͽ�۰ِ؍�
	// ������
	AUTOWORKINGDATA() {
		nSelect	= 0;
		dOffset	= 1.0;
		bAcuteRound	= TRUE;
		nLoopCnt	= 1;
		nScanLine	= 0;
		bCircleScroll	= TRUE;
	}
};
// �����`�󏈗�����
#define	AUTOWORKING			0
#define	AUTORECALCWORKING	1
#define	AUTOSTRICTOFFSET	2

/////////////////////////////////////////////////////////////////////////////
// CDXFDoc �h�L�������g

class CDXFDoc : public CDocument, public CDocBase
{
	std::bitset<DXFDOC_FLGNUM>	m_bDxfDocFlg;	// CDXFDoc�׸�
	UINT	m_nShapeProcessID;	// �`����H�w��ID
	AUTOWORKINGDATA	m_AutoWork;	// �����֊s�����ް�
	CRect3D		m_rcMax;			// �޷���Ă̵�޼ު�čő��`
	CDXFcircleEx*	m_pCircle;		// �؍팴�_
	CDXFline*		m_pLatheLine[2];// ���՗p���_([0]:�O�a, [1]:�[��)
	boost::optional<CPointD>	m_ptOrgOrig;	// ̧�ق���ǂݍ��񂾵ؼ��ٌ��_

	CString		m_strNCFileName;	// NC����̧�ٖ�
	int			m_nDataCnt[5],		// Point,Line,Circle,Arc,Ellipse �ް�����
				m_nLayerDataCnt[DXFLAYERSIZE-1];	// ڲԕʂ��ް�����(ORIGIN����)
	CLayerArray	m_obLayer;			// ڲԏ��z��
	CLayerMap	m_mpLayer;			// ڲԖ��𷰂ɂ���ϯ��

	// DXF��޼ު�� delete
	void	RemoveData(CLayerData*, int);

	// ��޼ު�ċ�`
	void	SetMaxRect(const CDXFdata* pData) {
		m_rcMax |= pData->GetMaxRect();
	}

	// ���_�␳�̕�����𐔒l�ɕϊ�
	BOOL	GetEditOrgPoint(LPCTSTR, CPointD&);

/*
	�گ������قɂ��WaitForSingleObject()�̑����
	��è�پ���݂�p���邱�ƂŁC�گ������قɑ΂���I���ʒm(������)���K�v�Ȃ��Ȃ�
*/
	// CDXFCircle�̌����H�Ώ��ް������ɖ߂�
	CCriticalSection	m_csRestoreCircleType;	// �گ������ق̑���
	static	UINT	RestoreCircleTypeThread(LPVOID);

protected: // �V���A���C�Y�@�\�݂̂���쐬���܂��B
	CDXFDoc();
	DECLARE_DYNCREATE(CDXFDoc)

// �A�g���r���[�g
public:
	BOOL	IsDXFDocFlag(DXFDOCFLG n) const {
		return m_bDxfDocFlg[n];
	}
	UINT	GetShapeProcessID(void) const {
		return m_nShapeProcessID;
	}
	CDXFcircleEx*	GetCircleObject(void) const {
		return m_pCircle;
	}
	CDXFline*		GetLatheLine(size_t n) const {
		ASSERT(0<=n && n<SIZEOF(m_pLatheLine));
		return m_pLatheLine[n];
	}
	int	GetDxfDataCnt(ENDXFTYPE enType) const {
		ASSERT(enType>=DXFPOINTDATA && enType<=DXFELLIPSEDATA);
		return m_nDataCnt[enType];
	}
	int	GetDxfLayerDataCnt(size_t n) const {
		ASSERT(n>=DXFCAMLAYER && n<=DXFCOMLAYER);
		return m_nLayerDataCnt[n-1];
	}
	INT_PTR	GetLayerCnt(void) const {
		return m_obLayer.GetSize();
	}
	CLayerData*	GetLayerData(INT_PTR n) const {
		ASSERT(n>=0 && n<GetLayerCnt());
		return m_obLayer[n];
	}
	CLayerData*	GetLayerData(LPCTSTR lpszLayer) const {
		CLayerData*	pLayer = NULL;
		return m_mpLayer.Lookup(lpszLayer, pLayer) ? pLayer : NULL;
	}
	CString GetNCFileName(void) const {
		return m_strNCFileName;
	}
	CRect3D	GetMaxRect(void) const {
		return m_rcMax;
	}

// �I�y���[�V����
public:
	// ���Ѻ����ٰèݸ�
	BOOL	RouteCmdToAllViews(CView*, UINT, int, void*, AFX_CMDHANDLERINFO*);
	void	SetReadyFlg(BOOL bReady) {
		m_bDxfDocFlg.set(DXFDOC_READY, bReady);
	}
	void	SetReload(BOOL bReload) {
		m_bDxfDocFlg.set(DXFDOC_RELOAD, bReload);
	}
	// DXF��޼ު�Ă̑���
	void	DataOperation(CDXFdata*, ENDXFOPERATION = DXFADD, int = -1);
	void	RemoveAt(LPCTSTR, int, int);
	void	RemoveAtText(LPCTSTR, int, int);
	// ڲԏ��̑���
	CLayerData*	AddLayerMap(const CString&, int = DXFCAMLAYER);
	void	DelLayerMap(CLayerData*);
	CString	CheckDuplexFile(const CString&, const CLayerArray* = NULL);
	BOOL	ReadLayerMap(LPCTSTR);
	BOOL	SaveLayerMap(LPCTSTR, const CLayerArray* = NULL);
	void	UpdateLayerSequence(void);
	//
	void	AllChangeFactor(double) const;	// �g�嗦�̍X�V

	boost::optional<CPointD>	GetCutterOrigin(void) {
		boost::optional<CPointD>	ptResult;
		if ( m_pCircle )
			ptResult = m_pCircle->GetCenter();
		return ptResult;
	}
	void	CreateCutterOrigin(const CPointD&, double, BOOL = FALSE);
	void	CreateLatheLine(const CPointD&, const CPointD&);
	void	CreateLatheLine(const CDXFline*, LPCDXFBLOCK);
	double	GetCutterOrgR(void) {
		return m_pCircle ? m_pCircle->GetR() : 0.0;
	}

	// ϳ��د��̈ʒu�ƊY����޼ު�Ă̍ŏ�������Ԃ�
	boost::tuple<CDXFshape*, CDXFdata*, double>	GetSelectObject(const CPointD&, const CRectD&);

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B
	//{{AFX_VIRTUAL(CDXFDoc)
	public:
	virtual void Serialize(CArchive& ar);   // �h�L�������g I/O �ɑ΂��ăI�[�o�[���C�h����܂��B
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
	virtual void OnCloseDocument();
	virtual void ReportSaveLoadException(LPCTSTR lpszPathName, CException* e, BOOL bSaving, UINT nIDPDefault);
	//}}AFX_VIRTUAL
	// �X�Vϰ��t�^
	virtual void SetModifiedFlag(BOOL bModified = TRUE);
	// �ύX���ꂽ�޷���Ă�������O���ڰ�ܰ����Ăяo��
	virtual BOOL SaveModified();
	// �ڰт��Q����̂ų���޳���ق́u:1�v��h��
	virtual void UpdateFrameCounts();

// �C���v�������e�[�V����
public:
	virtual ~CDXFDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
	// Serialize()����ް��ڍ�
	void	DbgSerializeInfo(void);
#endif

	// ���b�Z�[�W �}�b�v�֐��̐���
protected:
	//{{AFX_MSG(CDXFDoc)
	afx_msg void OnFileSave();
	afx_msg void OnFileSaveAs();
	afx_msg void OnEditOrigin();
	afx_msg void OnEditShape();
	afx_msg void OnEditAutoShape();
	afx_msg void OnEditStrictOffset();
	afx_msg void OnUpdateEditShape(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditShaping(CCmdUI* pCmdUI);
	//}}AFX_MSG
	// NC�����ƭ�
	afx_msg void OnUpdateFileDXF2NCD(CCmdUI* pCmdUI);
	afx_msg void OnFileDXF2NCD(UINT);
	// �`����H����
	afx_msg void OnUpdateShapePattern(CCmdUI* pCmdUI);
	afx_msg	void OnShapePattern(UINT);

	DECLARE_MESSAGE_MAP()
};

// DXF�ް��̓ǂݍ��� from DXFDoc2.cpp
BOOL	ReadDXF(CDXFDoc*, LPCTSTR);
