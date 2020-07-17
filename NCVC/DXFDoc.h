// DXFDoc.h : �w�b�_�[ �t�@�C��
//

#pragma once

#include "DocBase.h"
#include "DXFdata.h"
#include "DXFshape.h"
#include "Layer.h"
#include "NCVCdefine.h"

class	CDXFBlockData;
class	CDXFDoc;
class	CDXFView;

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
		DXFDOC_FLGNUM		// �׸ނ̐�[7]
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
enum {
	AUTOWORKING = 0,
	AUTORECALCWORKING,
	AUTOSTRICTOFFSET
};

// �����̂��߂̏��
typedef	struct tagCADBINDINFO {
	CStatic*	pParent;
	CDXFDoc*	pDoc;
	CDXFView*	pView;
} CADBINDINFO, *LPCADBINDINFO;

/////////////////////////////////////////////////////////////////////////////
// CDXFDoc �h�L�������g

class CDXFDoc : public CDocBase<DXFDOC_FLGNUM>
{
	UINT	m_nShapeProcessID;		// �`����H�w��ID
	AUTOWORKINGDATA	m_AutoWork;		// �����֊s�����ް�
	CRect3D			m_rcMax;		// �޷���Ă̵�޼ު�čő��`
	CDXFcircleEx*	m_pCircle;		// �؍팴�_
	CDXFline*		m_pLatheLine[2];// ���՗p���_([0]:�O�a, [1]:�[��)
	boost::optional<CPointD>	m_ptOrgOrig;	// ̧�ق���ǂݍ��񂾵ؼ��ٌ��_

	CString		m_strNCFileName;	// NC����̧�ٖ�
	int			m_nDataCnt[5],		// Point,Line,Circle,Arc,Ellipse �ް�����
				m_nLayerDataCnt[DXFLAYERSIZE-1];	// ڲԕʂ��ް�����(ORIGIN����)
	CLayerArray	m_obLayer;			// ڲԏ��z��
	CLayerMap	m_mpLayer;			// ڲԖ��𷰂ɂ���ϯ��
	CSortArray<CPtrArray, LPCADBINDINFO>	m_bindInfo;	// �������

	// DXF��޼ު�� delete
	void	RemoveData(CLayerData*, int);

	// ��޼ު�ċ�`
	void	SetMaxRect(const CDXFdata* pData) {
		m_rcMax |= pData->GetMaxRect();
	}

	// ���_�␳�̕�����𐔒l�ɕϊ�
	BOOL	GetEditOrgPoint(LPCTSTR, CPointD&);

//	---
//	�گ������قɂ��WaitForSingleObject()�̑����
//	��è�پ���݂�p���邱�ƂŁC�گ������قɑ΂���I���ʒm(������)���K�v�Ȃ��Ȃ�
//	---
	// CDXFCircle�̌����H�Ώ��ް������ɖ߂�
	CCriticalSection	m_csRestoreCircleType;	// �گ������ق̑���
	static	UINT	RestoreCircleTypeThread(LPVOID);

protected: // �V���A���C�Y�@�\�݂̂���쐬���܂��B
	CDXFDoc();
	DECLARE_DYNCREATE(CDXFDoc)

// �A�g���r���[�g
public:
	UINT	GetShapeProcessID(void) const {
		return m_nShapeProcessID;
	}
	CDXFcircleEx*	GetCircleObject(void) const {
		return m_pCircle;
	}
	boost::optional<CPointD>	GetCutterOrigin(void) const {
		boost::optional<CPointD>	ptResult;
		if ( m_pCircle )
			ptResult = m_pCircle->GetCenter();
		return ptResult;
	}
	double	GetCutterOrgR(void) const {
		return m_pCircle ? m_pCircle->GetR() : 0.0;
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
	int	GetCutLayerCnt(void) const {
		int	i = 0, nCnt = 0;
		for ( ; i<m_obLayer.GetSize(); i++ ) {
			if ( m_obLayer[i]->IsCutType() )
				nCnt++;
		}
		return nCnt;
	}
	CLayerData*	GetLayerData(INT_PTR n) const {
		ASSERT(n>=0 && n<GetLayerCnt());
		return m_obLayer[n];
	}
	CLayerData*	GetLayerData(LPCTSTR lpszLayer) const {
		CLayerData*	pLayer = NULL;
		return m_mpLayer.Lookup(lpszLayer, pLayer) ? pLayer : NULL;
	}
	INT_PTR	GetBindInfoCnt(void) const {
		return m_bindInfo.GetSize();
	}
	LPCADBINDINFO GetBindInfoData(INT_PTR n) const {
		ASSERT(n>=0 && n<GetBindInfoCnt());
		return m_bindInfo[n];
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
	//
	void	CreateCutterOrigin(const CPointD&, double, BOOL = FALSE);
	void	CreateLatheLine(const CPointD&, const CPointD&);
	void	CreateLatheLine(const CDXFline*, LPCDXFBLOCK);
	// ϳ��د��̈ʒu�ƊY����޼ު�Ă̍ŏ�������Ԃ�
	boost::tuple<CDXFshape*, CDXFdata*, double>	GetSelectObject(const CPointD&, const CRectD&);
	//
	void	AddBindInfo(LPCADBINDINFO pInfo) {
		m_bindInfo.Add(pInfo);
	}
	void	SortBindInfo(void);

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B
	//{{AFX_VIRTUAL(CDXFDoc)
	public:
	virtual void Serialize(CArchive& ar);   // �h�L�������g I/O �ɑ΂��ăI�[�o�[���C�h����܂��B
	virtual BOOL OnNewDocument();
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
	virtual void OnCloseDocument();
	//}}AFX_VIRTUAL
	// �ύX���ꂽ�޷���Ă�������O���ڰ�ܰ����Ăяo��
	virtual BOOL SaveModified();
	// �ڰт��Q����̂ų���޳���ق́u:1�v��h��
	virtual void UpdateFrameCounts();

// �C���v�������e�[�V����
public:
	virtual ~CDXFDoc();
#ifdef _DEBUG
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

// DXF�ް��̓ǂݍ��� to ReadDXF.cpp
BOOL	ReadDXF(CDXFDoc*, LPCTSTR);
