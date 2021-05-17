// DXFDoc.h : �w�b�_�[ �t�@�C��
//

#pragma once

#include "DocBase.h"
#include "DXFdata.h"
#include "DXFshape.h"
#include "Layer.h"
#include "NCVCdefine.h"

class	CDXFDoc;
class	CDXFView;

// ���_��������̫�Ă̔��a
#define	ORGRADIUS		10.0

// ���������ް��󂯓n���\����
struct	AUTOWORKINGDATA
{
	int		nSelect;		// �@�\�I��(0:�֊s, 1:�̾��)
	float	dOffset;		// �̾��
	int		nLoopCnt;		// �J��Ԃ���
	BOOL	bAcuteRound;	// �s�p�̊O�����ۂ߂�
	int		nScanLine;		// ������(0:�Ȃ�, 1:X����, 2:Y����)
	BOOL	bCircleScroll;	// �~�ް��ͽ�۰ِ؍�
	float	dGate,			// �Q�[�g�Ԋu
			dApproach;		// �A�v���[�`����
	// ������
	AUTOWORKINGDATA() {
		nSelect	= 0;
		dOffset	= 1.0f;
		nLoopCnt	= 1;
		bAcuteRound	= TRUE;
		nScanLine	= 0;
		bCircleScroll	= TRUE;
		dGate = dApproach = 0.0f;
	}
};
typedef	AUTOWORKINGDATA*	LPAUTOWORKINGDATA;
// �����`�󏈗�����
enum
{
	AUTOWORKING = 0,
	AUTORECALCWORKING,
	AUTOSTRICTOFFSET
};

// �����̂��߂̏��
struct CADBINDINFO
{
	BOOL		bTarget;
	CDXFDoc*	pDoc;
	CDXFView*	pView;
	CPointF		pt,			// �z�u�ʒu(�_�����W)
				ptOffset;	// �q�̕`�挴�_
	// ������
	CADBINDINFO(CDXFDoc* ppDoc, CDXFView* ppView) {
		bTarget = TRUE;
		pDoc = ppDoc;
		pView = ppView;
	}
	CADBINDINFO(BOOL bTgt, CDXFDoc* ppDoc, CDXFView* ppView, const CPointF& ppt, const CPointF& pptOffset) {
		bTarget = bTgt;
		pDoc = ppDoc;
		pView = ppView;
		pt = ppt;
		ptOffset = pptOffset;
	}
	CADBINDINFO(CADBINDINFO* pInfo) {
		bTarget = pInfo->bTarget;
		pDoc = pInfo->pDoc;
		pView = pInfo->pView;
		pt = pInfo->pt;
		ptOffset = pInfo->ptOffset;
	}
};
typedef	CADBINDINFO*	LPCADBINDINFO;

/////////////////////////////////////////////////////////////////////////////
// CDXFDoc �h�L�������g

class CDXFDoc : public CDocBase
{
	UINT	m_nShapeProcessID;		// �`����H�w��ID
	AUTOWORKINGDATA	m_AutoWork;		// �����֊s�����ް�
	CDXFcircleEx*	m_pCircle;		// �؍팴�_
	CDXFline*		m_pLatheLine[2];// ���՗p���_([0]:�O�a, [1]:�[��)
	CDXFDoc*		m_pParentDoc;	// �e�޷����(bind)
	boost::optional<CPointF>	m_ptOrgOrig;	// ̧�ق���ǂݍ��񂾵ؼ��ٌ��_

	CString		m_strNCFileName;	// NC����̧�ٖ�
	int			m_nDataCnt[5],		// Point,Line,Circle,Arc,Ellipse �ް�����
				m_nLayerDataCnt[DXFLAYERSIZE-1];	// ڲԕʂ��ް�����(ORIGIN����)
	CLayerData*	m_pSerializeLayer;	// ����Serialize()�ŏ�������ڲ�
	CLayerArray	m_obLayer;			// ڲԏ��z��
	CLayerMap	m_mpLayer;			// ڲԖ��𷰂ɂ���ϯ��
	CSortArray<CPtrArray, LPCADBINDINFO>	m_bindInfo;	// �������

	// DXF��޼ު�� delete
	void	RemoveData(CLayerData*, INT_PTR);

	// ��޼ު�ċ�`
	void	SetMaxRect(const CDXFdata* pData) {
		m_rcMax |= pData->GetMaxRect();
	}

	void	MakeDXF(const CString&);

	// ���_�␳�̕�����𐔒l�ɕϊ�
	BOOL	GetEditOrgPoint(LPCTSTR, CPointF&);

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
	boost::optional<CPointF>	GetCutterOrigin(void) const {
		boost::optional<CPointF>	ptResult;
		if ( m_pCircle )
			ptResult = m_pCircle->GetCenter();
		return ptResult;
	}
	float	GetCutterOrgR(void) const {
		return m_pCircle ? m_pCircle->GetR() : 0.0f;
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
	CLayerData* GetSerializeLayer(void) const {
		return m_pSerializeLayer;
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
	int		GetBindInfo_fromView(const CDXFView* pView) const {
		for ( int i=0; i<m_bindInfo.GetSize(); i++ ) {
			if ( m_bindInfo[i]->pView == pView )
				return i;
		}
		return -1;
	}
	void	RemoveBindData(INT_PTR n);
	void	SetBindParentDoc(CDXFDoc* pDoc) {
		m_pParentDoc = pDoc;
	}
	CDXFDoc*	GetBindParentDoc(void) const {
		return m_pParentDoc;
	}
	CString GetNCFileName(void) const {
		return m_strNCFileName;
	}

// �I�y���[�V����
public:
	// ���Ѻ����ٰèݸ�
	BOOL	RouteCmdToAllViews(CView*, UINT, int, void*, AFX_CMDHANDLERINFO*);
	// DXF��޼ު�Ă̑���
	void	DataOperation(CDXFdata*, ENDXFOPERATION = DXFADD, int = -1);
	void	RemoveAt(LPCTSTR, INT_PTR, INT_PTR);
	void	RemoveAtText(LPCTSTR, INT_PTR, INT_PTR);
	// ڲԏ��̑���
	void	SetSerializeLayer(CLayerData* pLayer) {
		m_pSerializeLayer = pLayer;
	}
	CLayerData*	AddLayerMap(const CString&, int = DXFCAMLAYER);
	void	DelLayerMap(CLayerData*);
	CString	CheckDuplexFile(const CString&, const CLayerArray* = NULL);
	BOOL	ReadLayerMap(LPCTSTR);
	BOOL	SaveLayerMap(LPCTSTR, const CLayerArray* = NULL);
	void	UpdateLayerSequence(void);
	//
	void	AllChangeFactor(float);	// �g�嗦�̍X�V
	void	AllSetDxfFlg(DWORD, BOOL = TRUE);
	void	AllRoundObjPoint(float);
	//
	void	CreateCutterOrigin(const CPointF&, float = ORGRADIUS, BOOL = FALSE);
	void	CreateLatheLine(const CPointF&, const CPointF&);
	void	CreateLatheLine(const CDXFline*, LPCDXFBLOCK);
	// ϳ��د��̈ʒu�ƊY����޼ު�Ă̍ŏ�������Ԃ�
	boost::tuple<CDXFshape*, CDXFdata*, float>	GetSelectObject(const CPointF&, const CRectF&);
	//
	void	AddBindInfo(LPCADBINDINFO);
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
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	//}}AFX_VIRTUAL
	virtual void UpdateFrameCounts();
	// �ύX���ꂽ�޷���Ă�������O���ڰ�ܰ����Ăяo��
	virtual BOOL SaveModified();

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
	afx_msg void OnEditStrictOffset();
	afx_msg void OnUpdateEditShape(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditShaping(CCmdUI* pCmdUI);
	//}}AFX_MSG
	afx_msg void OnFileDXF2DXF();
	// NC�����ƭ�
	afx_msg void OnUpdateFileDXF2NCD(CCmdUI* pCmdUI);
	afx_msg void OnFileDXF2NCD(UINT);
	// �`����H����
	afx_msg void OnUpdateShapePattern(CCmdUI* pCmdUI);
	afx_msg	void OnShapePattern(UINT);
	// ��������
	afx_msg void OnEditAuto(UINT);

	DECLARE_MESSAGE_MAP()
};

// DXF�ް��̓ǂݍ��� to ReadDXF.cpp
BOOL	ReadDXF(CDXFDoc*, LPCTSTR);
