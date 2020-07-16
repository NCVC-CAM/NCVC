// Layer.h: CLayerData, CLayerMap �N���X�̃C���^�[�t�F�C�X
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NCVCdefine.h"
#include "DXFdata.h"
#include "DXFshape.h"

// CLayerData::DataOperation() �̑�����@
enum	ENDXFOPERATION	{
	DXFADD, DXFINS, DXFMOD
};

class	CNCMakeOption;

//////////////////////////////////////////////////////////////////////
// Layer�ް�
//////////////////////////////////////////////////////////////////////
class CLayerData : public CObject
{
friend	class	CLayerDlg;
friend	class	CMakeNCDlgEx1;
friend	class	CMakeNCDlgEx2;
friend	class	CMakeNCDlgEx11;
friend	class	CMakeNCDlgEx21;

	CString	m_strLayer;		// ڲԖ�
	int		m_nType;		// ڲ�����(DXFORGLAYER�`DXFCOMLAYER)
	BOOL	m_bView;		// �\���Ώ�
	CDXFarray	m_obDXFArray;		// �؍��ް�(CDXFdata)
	CTypedPtrArrayEx<CObArray, CDXFtext*>
				m_obDXFTextArray;	// �؍��ް�(CDXFtext only)
	// �`��F��
	CShapeArray	m_obShapeArray;	// �A���W�c
	CDXFshape*	m_pActiveShape;	// CDXFworking�رײ�ގQ�Ɨp
	// �ȉ��u����ڲԂ̈ꊇ�����v�ɂĎg�p
	int		m_nListNo;		// �؍폇
	DWORD	m_dwFlags;		// ����׸�(����ڲԂ̌ʏo�͏����ł̴װ���f)
	BOOL	m_bCutTarget,	// �؍�Ώ�
			m_bDrillZ,		// �Ő[Z���W�������H�ɂ��K�p���邩
			m_bPartOut;		// �ʏo���׸�
	CString	m_strInitFile,	// �؍����̧��
			m_strNCFile;	// �ʏo��̧�ٖ�
	double	m_dInitZCut,	// �؍����̧�قɐݒ肳�ꂽ�Ő[Z���W(CMakeNCDlgEx1::�\��)
			m_dZCut;		// �Ő[Z���W(CMakeNCDlgEx2::����)

protected:
	CLayerData();
public:
	CLayerData(const CString&, int);
	CLayerData(const CLayerData*, BOOL);
	virtual	~CLayerData();

// ����ޭ��
	INT_PTR	GetDxfSize(void) const {
		return m_obDXFArray.GetSize();
	}
	INT_PTR	GetDxfTextSize(void) const {
		return m_obDXFTextArray.GetSize();
	}
	INT_PTR	GetShapeSize(void) const {
		return m_obShapeArray.GetSize();
	}
	CDXFdata* GetDxfData(int n) const {
		ASSERT(n>=0 && n<GetDxfSize());
		return m_obDXFArray[n];
	}
	CDXFtext* GetDxfTextData(int n) const {
		ASSERT(n>=0 && n<GetDxfTextSize());
		return m_obDXFTextArray[n];
	}
	CDXFshape*	GetShapeData(int n) {
		ASSERT(n>=0 && n<GetShapeSize());
		return m_obShapeArray[n];
	}
	CDXFshape*	GetActiveShape(void) {
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
	BOOL	IsViewLayer(void) const {
		return m_bView;
	}
	BOOL	IsCutTarget(void) const {
		return m_bCutTarget;
	}
	BOOL	IsMakeTarget(void) const {
		return IsCutType() && IsCutTarget();
	}
	void	SetCutTargetFlg(BOOL bCut) {
		m_bCutTarget = bCut;
	}
	void	SetCutTargetFlg_fromView(void) {
		m_bCutTarget = m_bView;
	}
	BOOL	IsDrillZ(void) const {
		return m_bDrillZ;
	}
	BOOL	IsPartOut(void) const {
		return m_bPartOut;
	}
	int		GetListNo(void) const {
		return m_nListNo;
	}
	void	SetListNo(int n) {
		m_nListNo = n;
	}
	DWORD	GetLayerFlags(void) const {
		return m_dwFlags;
	}
	void	SetLayerFlags(DWORD dwFlags = 0) {
		m_dwFlags = dwFlags;
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
	void	CopyShape(CShapeArray& obArray) {
		m_obShapeArray.Copy(obArray);
	}
	void	AscendingShapeSort(void);
	void	DescendingShapeSort(void);
	//
	void	AllChangeFactor(double) const;
	int		AllShape_OrgTuning(void);
	void	DrawWorking(CDC*);
	// ڲԖ��Ə���̧�فC�Ő[Z�l�̊֌W���
	void	SetLayerInfo(const CString&);	// from CDXFDoc::ReadLayerMap()
	CString	FormatLayerInfo(LPCTSTR);		// from CDXFDoc::SaveLayerMap()

	virtual	void	Serialize(CArchive&);
	DECLARE_SERIAL(CLayerData)
};

typedef	CSortArray<CObArray, CLayerData*>					CLayerArray;
typedef	CTypedPtrMap<CMapStringToOb, CString, CLayerData*>	CLayerMap;
