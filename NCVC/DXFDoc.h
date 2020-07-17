// DXFDoc.h : �w�b�_�[ �t�@�C��
//

#pragma once

#include "DocBase.h"
#include "DXFdata.h"
#include "DXFshape.h"
#include "Layer.h"
#include "NCVCdefine.h"
class	CDXFBlockData;

/////////////////////////////////////////////////////////////////////////////
// CDXFDoc �h�L�������g

class CDXFDoc : public CDocument, public CDocBase
{
	BOOL	m_bReady,		// NC�����\���ǂ���(�װ�׸�)
			m_bThread,		// �گ�ތp���׸�
			m_bReload,		// �ēǍ��׸�(from DXFSetup.cpp)
			m_bShape;		// �`�󏈗����s������
	UINT	m_nShapePattern;	// �`�󏈗������
	double	m_dOffset;		// ��̫�ė֊s�̾��
	BOOL	m_bAcute;		// ��̫�ĉs�p�ۂߐݒ�

	CRect3D		m_rcMax;			// �޷���Ă̵�޼ު�čő��`
	CDXFcircleEx*	m_pCircle;		// �؍팴�_
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
	BOOL	IsReload(void) const {
		return m_bReload;
	}
	BOOL	IsShape(void) const {
		return m_bShape;
	}
	UINT	GetShapePattern(void) const {
		return m_nShapePattern;
	}
	CDXFcircleEx*	GetCircleObject(void) {
		return m_pCircle;
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
		m_bReady = bReady;
	}
	void	SetReload(BOOL bReload) {
		m_bReload = bReload;
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
	BOOL	SaveLayerMap(LPCTSTR);
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
	double	GetCutterOrgR(void) {
		return m_pCircle ? m_pCircle->GetR() : 0.0;
	}

	// ϳ��د��̈ʒu�ƊY����޼ު�Ă̍ŏ�������Ԃ�
	boost::tuple<CDXFshape*, double>	GetSelectObject(const CPointD&, const CRectD&);

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
	void	SerializeInfo(void);
#endif

	// ���b�Z�[�W �}�b�v�֐��̐���
protected:
	//{{AFX_MSG(CDXFDoc)
	afx_msg void OnFileSave();
	afx_msg void OnFileSaveAs();
	afx_msg void OnEditOrigin();
	afx_msg void OnEditShape();
	afx_msg void OnEditAutoShape();
	afx_msg void OnUpdateEditShape(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditShaping(CCmdUI* pCmdUI);
	//}}AFX_MSG
	// NC�����ƭ�
	afx_msg void OnUpdateFileDXF2NCD(CCmdUI* pCmdUI);
	afx_msg void OnFileDXF2NCD(UINT);
	// �`����H����
	afx_msg	void OnShapePattern(UINT);

	DECLARE_MESSAGE_MAP()
};

// DXF�ް��̓ǂݍ��� from DXFDoc2.cpp
BOOL	ReadDXF(CDXFDoc*, LPCTSTR);
