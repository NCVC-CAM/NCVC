// Layer.h: CLayerData, CLayerMap �N���X�̃C���^�[�t�F�C�X
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NCVCdefine.h"
#include "DXFdata.h"
#include "DXFshape.h"

// CLayerData �׸�
enum LAYERFLG {
	LAYER_VIEW = 0,		// �\���Ώ�
	LAYER_CUT_TARGET,	// �؍�Ώ�
	LAYER_DRILL_Z,		// �Ő[Z���W�������H�ɂ��K�p���邩
	LAYER_PART_OUT,		// �ʏo���׸�
	LAYER_PART_ERROR,	// �ʏo�͂ł̴װ
		LAYER_FLGNUM		// �׸ނ̐�[5]
};

// CLayerData::DataOperation() �̑�����@
enum	ENDXFOPERATION	{
	DXFADD, DXFINS, DXFMOD
};

class	CNCMakeMillOpt;

//////////////////////////////////////////////////////////////////////
// Layer�ް�
//////////////////////////////////////////////////////////////////////
class CLayerData : public CObject
{
friend	class	CLayerDlg;
friend	class	CMakeNCDlgEx2;
friend	class	CMakeNCDlgEx3;
friend	class	CMakeNCDlgEx11;
friend	class	CMakeNCDlgEx21;

	std::bitset<LAYER_FLGNUM>	m_bLayerFlg;	// CLayerData�׸�
	CString	m_strLayer;		// ڲԖ�
	int		m_nType;		// ڲ�����(DXFORGLAYER�`DXFCOMLAYER)
	CDXFarray	m_obDXFArray;		// �؍��ް�(CDXFdata)
	CTypedPtrArrayEx<CObArray, CDXFtext*>
				m_obDXFTextArray;	// �؍��ް�(CDXFtext only)
	// �`��F��
	CShapeArray	m_obShapeArray;	// �A���W�c
	CDXFshape*	m_pActiveShape;	// CDXFworking�رײ�ގQ�Ɨp
	// �ȉ��u����ڲԂ̈ꊇ�����v�ɂĎg�p
	int		m_nListNo;		// �؍폇
	CString	m_strInitFile,	// �؍����̧��
			m_strNCFile,	// �ʏo��̧�ٖ�
			m_strLayerComment,	// ڲԂ��Ƃ̺���
			m_strLayerCode;		// ڲԂ��Ƃ̓��꺰��
	double	m_dInitZCut,	// �؍����̧�قɐݒ肳�ꂽ�Ő[Z���W(CMakeNCDlgEx::�\��)
			m_dZCut;		// �Ő[Z���W(CMakeNCDlgEx2::����)

protected:
	CLayerData();
public:
	CLayerData(const CString&, int);
	CLayerData(const CLayerData*, BOOL);
	virtual	~CLayerData();

// ����ޭ��
	BOOL	IsLayerFlag(LAYERFLG n) const {
		return m_bLayerFlg[n];
	}
	INT_PTR	GetDxfSize(void) const {
		return m_obDXFArray.GetSize();
	}
	INT_PTR	GetDxfTextSize(void) const {
		return m_obDXFTextArray.GetSize();
	}
	INT_PTR	GetShapeSize(void) const {
		return m_obShapeArray.GetSize();
	}
	CDXFdata* GetDxfData(INT_PTR n) const {
		ASSERT(n>=0 && n<GetDxfSize());
		return m_obDXFArray[n];
	}
	CDXFtext* GetDxfTextData(INT_PTR n) const {
		ASSERT(n>=0 && n<GetDxfTextSize());
		return m_obDXFTextArray[n];
	}
	CDXFshape*	GetShapeData(INT_PTR n) const {
		ASSERT(n>=0 && n<GetShapeSize());
		return m_obShapeArray[n];
	}
	CDXFshape*	GetActiveShape(void) const {
		return m_pActiveShape;
	}
	void	SetActiveShape(CDXFshape* pShape) {
		m_pActiveShape = pShape;
	}
	//
	const	CString		GetStrLayer(void) const {
		return m_strLayer;
	}
	int		GetLayerType(void) const {
		ASSERT(m_nType>=DXFORGLAYER && m_nType<=DXFCOMLAYER);
		return m_nType;
	}
	BOOL	IsCutType(void) const {
		return m_nType == DXFCAMLAYER;
	}
	BOOL	IsMakeTarget(void) const {
		return IsCutType() && IsLayerFlag(LAYER_CUT_TARGET);
	}
	void	SetCutTargetFlg(BOOL bCut) {
		m_bLayerFlg.set(LAYER_CUT_TARGET, bCut);
	}
	void	SetCutTargetFlg_fromView(void) {
		m_bLayerFlg.set(LAYER_CUT_TARGET, m_bLayerFlg[LAYER_VIEW]);
	}
	int		GetLayerListNo(void) const {
		return m_nListNo;
	}
	void	SetLayerListNo(int n) {
		m_nListNo = n;
	}
	void	SetLayerPartFlag(BOOL bError = FALSE) {
		m_bLayerFlg.set(LAYER_PART_ERROR, bError);
	}
	CString	GetInitFile(void) const {
		return m_strInitFile;
	}
	void	SetInitFile(LPCTSTR);
	CString	GetNCFile(void) const {
		return m_strNCFile;
	}
	void	SetNCFile(CString& strFile) {
		m_strNCFile = strFile;
	}
	double	GetZCut(void) const {
		return m_dZCut;
	}
	CString	GetLayerComment(void) const {
		return m_strLayerComment;
	}
	CString	GetLayerOutputCode(void) const {
		return m_strLayerCode;
	}

// ���ڰ���
	CDXFdata*	DataOperation(CDXFdata*, ENDXFOPERATION, int);
	CDXFdata*	RemoveData(int nIndex) {
		CDXFdata*	pData = m_obDXFArray[nIndex];
		m_obDXFArray.RemoveAt(nIndex);
		return pData;
	}
	CDXFtext*	RemoveDataText(int nIndex) {
		CDXFtext*	pData = m_obDXFTextArray[nIndex];
		m_obDXFTextArray.RemoveAt(nIndex);
		return pData;
	}
	void	AddShape(CDXFshape* pShape) {
		m_obShapeArray.Add(pShape);
	}
	void	RemoveShape(CDXFshape* pShape) {
		for ( int i=0; i<m_obShapeArray.GetSize(); i++ ) {
			if ( pShape == m_obShapeArray[i] ) {
				delete	m_obShapeArray[i];
				m_obShapeArray.RemoveAt(i);
				break;
			}
		}
	}
	void	RemoveAllShape(void) {
		for ( int i=0; i<m_obShapeArray.GetSize(); i++ )
			delete	m_obShapeArray[i];
		m_obShapeArray.RemoveAll();
	}
	void	AscendingShapeSort(void);
	void	DescendingShapeSort(void);
	void	SerializeShapeSort(void);
	//
	void	AllChangeFactor(double) const;
	void	AllShapeClearSideFlg(void) const;
	void	DrawWorking(CDC*);
	// ڲԖ��Ə���̧�فC�Ő[Z�l�̊֌W���
	void	SetLayerInfo(const CString&);	// from CDXFDoc::ReadLayerMap()
	CString	FormatLayerInfo(LPCTSTR);		// from CDXFDoc::SaveLayerMap()

	virtual	void	Serialize(CArchive&);
	DECLARE_SERIAL(CLayerData)
};

typedef	CSortArray<CObArray, CLayerData*>					CLayerArray;
typedef	CTypedPtrMap<CMapStringToOb, CString, CLayerData*>	CLayerMap;
