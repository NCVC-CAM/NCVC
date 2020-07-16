// DXFDoc.h : �w�b�_�[ �t�@�C��
//

#if !defined(AFX_DXFDOC_H__95D33FAC_974A_11D3_B0D5_004005691B12__INCLUDED_)
#define AFX_DXFDOC_H__95D33FAC_974A_11D3_B0D5_004005691B12__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "DocBase.h"
#include "DXFdata.h"
#include "DXFshape.h"
#include "Layer.h"
class	CDXFBlockData;

// CDXFDoc::DataOperation() �̑�����@
enum	ENDXFOPERATION	{
	DXFADD, DXFINS, DXFMOD
};

// m_pCircle �w��
#define	DXFCIRCLEORG	0
#define	DXFCIRCLESTA	1

/////////////////////////////////////////////////////////////////////////////
// CDXFDoc �h�L�������g

class CDXFDoc : public CDocument, public CDocBase
{
	BOOL	m_bReady,		// NC�����\���ǂ���(�װ�׸�)
			m_bThread,		// �گ�ތp���׸�
			m_bReload,		// �ēǍ��׸�(from DXFSetup.cpp)
			m_bShape;		// �`�󏈗����s������
	UINT	m_nShapePattern;	// �`�󏈗������

	CRect3D		m_rcMax;			// �޷���Ă̵�޼ު�čő��`
	CDXFcircleEx*	m_pCircle[2];	// �؍팴�_�Ɖ��H�J�n�ʒu�̏��
	CPointD		m_ptOrgOrig;		// ̧�ق���ǂݍ��񂾵ؼ��ٌ��_

	CString		m_strNCFileName;	// NC����̧�ٖ�
	int			m_nDataCnt[5];		// Point,Line,Circle,Arc,Ellipse �ް�����
	CDXFarray	m_obDXFArray,		// DXF�؍��ް�(CDXFdata)
				m_obStartArray,		// DXF���H�J�n�ʒu�w���ް�(CDXFline/CDXFtext/CDXFpolyline)
				m_obMoveArray;		// DXF�ړ��w���ް�(�V)
	CTypedPtrArray<CObArray, CDXFtext*>
				m_obDXFTextArray,	// DXF�؍��ް�(CDXFtext only)
				m_obStartTextArray,	// DXF���H�J�n�ʒu�w���ް�(�V)
				m_obMoveTextArray,	// DXF�ړ��w���ް�(�V)
				m_obCommentArray;	// ���ĕ����ް�(�V)
	CLayerMap	m_mpLayer;			// ڲԖ��Ǘ�ϯ��
	CLayerArray	m_obLayer;			// ڲԖ��Ǘ�ϯ�߂̼رײ�ޗp�ꎞ�i�[�ر
	DXFVIEWINFO	m_dxfViewInfo;		// �\����(�رײ�ޗp)

	// RemoveAt() �� DataOperation()::DXFMOD �� inline���
	void	RemoveSub(CDXFdata* pData) {
		ENDXFTYPE	enType = pData->GetType();
		const CLayerData* pLayer = pData->GetLayerData();
		if ( pLayer )
			m_mpLayer.DelLayerMap(pLayer->GetStrLayer());
		delete	pData;
		if ( enType>=DXFPOINTDATA || enType<=DXFELLIPSEDATA )
			m_nDataCnt[enType]--;
	}

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
	void	SetShapePattern(UINT nShape) {
		m_nShapePattern = nShape;
	}
	CDXFcircleEx*	GetCircleObject(size_t a) {
		ASSERT(a>=0 && a<SIZEOF(m_pCircle));
		return m_pCircle[a];
	}
	int	GetDxfSize(void) const {
		return m_obDXFArray.GetSize();
	}
	int	GetDxfTextSize(void) const {
		return m_obDXFTextArray.GetSize();
	}
	int	GetDxfStartSize(void) const {
		return m_obStartArray.GetSize();
	}
	int	GetDxfStartTextSize(void) const {
		return m_obStartTextArray.GetSize();
	}
	int	GetDxfMoveSize(void) const {
		return m_obMoveArray.GetSize();
	}
	int	GetDxfMoveTextSize(void) const {
		return m_obMoveTextArray.GetSize();
	}
	int	GetDxfCommentSize(void) const {
		return m_obCommentArray.GetSize();
	}
	CDXFdata* GetDxfData(int n) {
		ASSERT(n>=0 && n<GetDxfSize());
		return m_obDXFArray[n];
	}
	CDXFtext* GetDxfTextData(int n) {
		ASSERT(n>=0 && n<GetDxfTextSize());
		return m_obDXFTextArray[n];
	}
	CDXFdata* GetDxfStartData(int n) {
		ASSERT(n>=0 && n<GetDxfStartSize());
		return m_obStartArray[n];
	}
	CDXFtext* GetDxfStartTextData(int n) {
		ASSERT(n>=0 && n<GetDxfStartTextSize());
		return m_obStartTextArray[n];
	}
	CDXFdata* GetDxfMoveData(int n) {
		ASSERT(n>=0 && n<GetDxfMoveSize());
		return m_obMoveArray[n];
	}
	CDXFtext* GetDxfMoveTextData(int n) {
		ASSERT(n>=0 && n<GetDxfMoveTextSize());
		return m_obMoveTextArray[n];
	}
	CDXFtext* GetDxfCommentData(int n) {
		ASSERT(n>=0 && n<GetDxfCommentSize());
		return m_obCommentArray[n];
	}
	int	GetDxfDataCnt(ENDXFTYPE enType) const {
		ASSERT(enType>=DXFPOINTDATA && enType<=DXFELLIPSEDATA);
		return m_nDataCnt[enType];
	}
	CString GetNCFileName(void) const {
		return m_strNCFileName;
	}
	CLayerMap*	GetLayerMap(void) {
		return &m_mpLayer;
	}
	CLayerData*	GetLayerData_FromArray(int a) {
		return a>=0 && a<m_obLayer.GetSize() ? m_obLayer[a] : NULL;
	}
	CRect3D	GetMaxRect(void) const {
		return m_rcMax;
	}
	const	LPDXFVIEWINFO	GetDxfViewInfo(void) const {
		return (const LPDXFVIEWINFO)&m_dxfViewInfo;
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
	// DXF��޼ު�Ă̑��� -> m_obDXFArray
	void	DataOperation(LPDXFPARGV, int = -1, ENDXFOPERATION = DXFADD);
	void	DataOperation(LPDXFLARGV, int = -1, ENDXFOPERATION = DXFADD);
	void	DataOperation(LPDXFCARGV, int = -1, ENDXFOPERATION = DXFADD);
	void	DataOperation(LPDXFAARGV, int = -1, ENDXFOPERATION = DXFADD);
	void	DataOperation(LPDXFEARGV, int = -1, ENDXFOPERATION = DXFADD);
	void	DataOperation(LPDXFTARGV, int = -1, ENDXFOPERATION = DXFADD);
	void	DataOperation(CDXFpoint*, int = -1, ENDXFOPERATION = DXFADD);
	void	DataOperation(CDXFline*, int = -1, ENDXFOPERATION = DXFADD);
	void	DataOperation(CDXFcircle*, int = -1, ENDXFOPERATION = DXFADD);
	void	DataOperation(CDXFarc*, int = -1, ENDXFOPERATION = DXFADD);
	void	DataOperation(CDXFellipse*, int = -1, ENDXFOPERATION = DXFADD);
	void	DataOperation(CDXFtext*, int = -1, ENDXFOPERATION = DXFADD);
	void	DataOperation(CDXFpolyline*, int = -1, ENDXFOPERATION = DXFADD);
	void	RemoveAt(int, int);
	void	RemoveAtText(int, int);
	// DXF��޼ު�Ă̑��� -> m_obStartArray
	void	DataOperation_STR(LPDXFLARGV, int = -1, ENDXFOPERATION = DXFADD);
	void	DataOperation_STR(LPDXFTARGV, int = -1, ENDXFOPERATION = DXFADD);
	void	DataOperation_STR(CDXFline*, int = -1, ENDXFOPERATION = DXFADD);
	void	DataOperation_STR(CDXFtext*, int = -1, ENDXFOPERATION = DXFADD);
	void	DataOperation_STR(CDXFpolyline*, int = -1, ENDXFOPERATION = DXFADD);
	void	RemoveAt_STR(int, int);
	void	RemoveAtText_STR(int, int);
	// DXF��޼ު�Ă̑��� -> m_obMoveArray
	void	DataOperation_MOV(LPDXFLARGV, int = -1, ENDXFOPERATION = DXFADD);
	void	DataOperation_MOV(LPDXFTARGV, int = -1, ENDXFOPERATION = DXFADD);
	void	DataOperation_MOV(CDXFline*, int = -1, ENDXFOPERATION = DXFADD);
	void	DataOperation_MOV(CDXFtext*, int = -1, ENDXFOPERATION = DXFADD);
	void	DataOperation_MOV(CDXFpolyline*, int = -1, ENDXFOPERATION = DXFADD);
	void	RemoveAt_MOV(int, int);
	void	RemoveAtText_MOV(int, int);
	// DXF��޼ު�Ă̑��� -> m_obCommentArray
	void	DataOperation_COM(LPDXFTARGV, int = -1, ENDXFOPERATION = DXFADD);
	void	DataOperation_COM(CDXFtext*, int = -1, ENDXFOPERATION = DXFADD);
	void	RemoveAt_COM(int, int);

	void	AllChangeFactor(double);	// �g�嗦�̍X�V

	CPointD	GetCutterOrigin(void) {
		if ( m_pCircle[DXFCIRCLEORG] )
			return m_pCircle[DXFCIRCLEORG]->GetCenter();
		else
			return CPointD(HUGE_VAL);
	}
	void	SetCutterOrigin(const CPointD&, double, BOOL = FALSE);
	double	GetCutterOrgR(void) {
		if ( m_pCircle[DXFCIRCLEORG] )
			return m_pCircle[DXFCIRCLEORG]->GetR();
		else
			return 0.0;
	}
	CPointD	GetStartOrigin(void) {
		if ( m_pCircle[DXFCIRCLESTA] )
			return m_pCircle[DXFCIRCLESTA]->GetCenter();
		else
			return CPointD(HUGE_VAL);
	}
	void	SetStartOrigin(const CPointD&, double, BOOL = FALSE);
	double	GetStartOrgR(void) {
		if ( m_pCircle[DXFCIRCLESTA] )
			return m_pCircle[DXFCIRCLESTA]->GetR();
		else
			return 0.0;
	}

	// ϳ��د��̈ʒu�ƊY����޼ު�Ă̍ŏ�������Ԃ�
	double	GetSelectViewPointGap(const CPointD&, const CRectD&, CDXFdata**);

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
	afx_msg void OnFileSaveAs();
	afx_msg void OnFileSave();
	afx_msg void OnEditOrigin();
	afx_msg void OnEditShape();
	afx_msg void OnEditAutoShape();
	afx_msg void OnUpdateEditShape(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditShaping(CCmdUI* pCmdUI);
	//}}AFX_MSG
	// NC�����ƭ�
	afx_msg void OnUpdateFileDXF2NCD(CCmdUI* pCmdUI);
	afx_msg void OnFileDXF2NCD(UINT);

	DECLARE_MESSAGE_MAP()
};

// DXF�ް��̓ǂݍ��� from DXFDoc2.cpp
BOOL	ReadDXF(CDXFDoc*, LPCTSTR);

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ �͑O�s�̒��O�ɒǉ��̐錾��}�����܂��B

#endif // !defined(AFX_DXFDOC_H__95D33FAC_974A_11D3_B0D5_004005691B12__INCLUDED_)
