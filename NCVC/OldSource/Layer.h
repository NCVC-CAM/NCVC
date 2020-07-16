// Layer.h: CLayerData, CLayerMap �N���X�̃C���^�[�t�F�C�X
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_LAYER_H__696BA943_2D27_4A34_97D7_633F16942430__INCLUDED_)
#define AFX_LAYER_H__696BA943_2D27_4A34_97D7_633F16942430__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "NCVCdefine.h"
#include "DXFdata.h"
#include "DXFshape.h"
class	CNCMakeOption;

//////////////////////////////////////////////////////////////////////
// Layer�ް�
//////////////////////////////////////////////////////////////////////
class CLayerData : public CObject
{
friend	class	CLayerDlg;
friend	class	CLayerMap;
friend	class	CMakeNCDlgEx1;
friend	class	CMakeNCDlgEx2;
friend	class	CMakeNCDlgEx11;
friend	class	CMakeNCDlgEx21;

	CString	m_strLayer;		// ڲԖ�
	int		m_nType,		// ڲ�����(DXFORGLAYER�`DXFCOMLAYER)
			m_nDataCnt;		// ڲԖ����Ƃ̶���
	BOOL	m_bView;		// �\���Ώ�
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
	// �ȉ��رײ�ނɂĎg�p
	int		m_nSerial;		// �رٔԍ�
	// �`��F��
	CDXFmap	m_mpDXFdata;	// ڲԂ��Ƃ̍��Wϯ�ߕ��(�A����޼ު�Č����p�ꎞ�̈�)
	CDXFmapArray	m_obChainMap;	// �A���W�c
	HANDLE	m_hClearMap;	// ���Wϯ�ߕ�̏����گ�ނ������

	// ڲԖ��Ə���̧�فC�Ő[Z�l�̊֌W��ۑ�(CMakeNCDlgEx[1|2])�ɂĎg�p
	CString	FormatLayerInfo(LPCTSTR);	// from CLayerMap::SaveLayerMap()

	// ���Wϯ�ߕ�̏����گ��
	static	UINT	RemoveMasterMapThread(LPVOID);

protected:
	// m_mpDXFdata ������Ԃ� DXFMAPFLG_LAYER �׸ނ𗧂ĂĂ���
	CLayerData() : m_mpDXFdata(DXFMAPFLG_LAYER) {
		m_nType		= -1;
		m_nDataCnt	= 0;
		m_nListNo	= -1;
		m_dwFlags	= 0;
		m_bView = m_bCutTarget = m_bDrillZ = m_bPartOut = FALSE;
		m_dInitZCut	= m_dZCut = 0.0;
		m_hClearMap	= NULL;
	};
public:
	CLayerData(const CString& strLayer, int nType) : m_mpDXFdata(DXFMAPFLG_LAYER) {	// �W���ݽ�׸�
		m_strLayer	= strLayer;
		m_nType		= nType;
		m_nDataCnt	= 1;
		m_bView = m_bCutTarget = TRUE;
		m_nListNo	= -1;
		m_dwFlags	= 0;
		m_bDrillZ	= TRUE;
		m_bPartOut	= FALSE;
		m_dInitZCut	= m_dZCut = 0.0;
		m_hClearMap	= NULL;
	}
	CLayerData(const CLayerData* pLayer, BOOL bCut) {	// CMakeNCDlgEx[1|2][1]�p��߰�ݽ�׸�
		// �K�v���ް�������߰
		m_strLayer		= pLayer->m_strLayer;
		m_nType			= pLayer->m_nType;
		m_bCutTarget	= bCut;
		m_bDrillZ		= pLayer->m_bDrillZ;
		m_bPartOut		= pLayer->m_bPartOut;
		m_strInitFile	= pLayer->m_strInitFile;
		m_strNCFile		= pLayer->m_strNCFile;
		m_dInitZCut		= pLayer->m_dInitZCut;
		m_dZCut			= pLayer->m_dZCut;
		m_hClearMap	= NULL;
	}
	virtual	~CLayerData() {
		for ( int i=0; i<m_obChainMap.GetSize(); i++ )
			delete	m_obChainMap[i];
		if ( m_hClearMap ) {
			::WaitForSingleObject(m_hClearMap, INFINITE);
			::CloseHandle(m_hClearMap);
		}
	}

// ����ޭ��
	int		GetCount(void) const {
		return m_nDataCnt;
	}
	const	CString		GetStrLayer(void) const {
		return m_strLayer;
	}
	int		GetLayerType(void) const {
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
	BOOL	IsDrillZ(void) const {
		return m_bDrillZ;
	}
	BOOL	IsPartOut(void) const {
		return m_bPartOut;
	}

// ���ڰ���
	int		Increment(void) {
		m_nDataCnt++;
		return GetCount();
	}
	int		Decrement(void) {
		m_nDataCnt--;
		return GetCount();
	}
	//
	int		GetListNo(void) const {
		return m_nListNo;
	}
	void	SetListNo(int a) {
		m_nListNo = a;
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
	void	SetInitFile(LPCTSTR, CNCMakeOption* = NULL, CLayerData* = NULL);
	CString	GetNCFile(void) const {
		return m_strNCFile;
	}
	void	SetNCFile(LPCTSTR lpNCFile) {
		m_strNCFile = lpNCFile;
	}
	double	GetZCut(void) const {
		return m_dZCut;
	}
	void	SetCutTargetFlg(BOOL bCut) {
		m_bCutTarget = bCut;
	}
	//
	void	SetSerial(int a) {
		m_nSerial = a;
	}
	int		GetSerial(void) const {
		return m_nSerial;
	}
	//
	CDXFmap*	GetMasterMap(void) {
		return &m_mpDXFdata;
	}
	void	SetPointMasterMap(CDXFdata* pData) {
		m_mpDXFdata.SetPointMap(pData);
	}
	CDXFmapArray*	GetChainMap(void) {
		return &m_obChainMap;
	}
	void	SetShapeSwitch_AllChainMap(BOOL);
	void	RemoveMasterMap(void);

	virtual	void	Serialize(CArchive&);
	DECLARE_SERIAL(CLayerData)
};

typedef	CTypedPtrArray<CObArray, CLayerData*>	CLayerArray;

//////////////////////////////////////////////////////////////////////
// LayerMap(Layer�ް��Ǘ��p)
//////////////////////////////////////////////////////////////////////
typedef	CTypedPtrMap<CMapStringToOb, CString, CLayerData*>	CLayerStringMap;
class CLayerMap : public CLayerStringMap
{
public:
	virtual ~CLayerMap();

// ���ڰ���
	CLayerData*	AddLayerMap(const CString& strLayer, int nType = DXFCAMLAYER) {
		ASSERT( !strLayer.IsEmpty() );
		CLayerData* pLayer;
		if ( Lookup(strLayer, pLayer) )
			pLayer->Increment();
		else {
			pLayer = new CLayerData(strLayer, nType);
			SetAt(strLayer, pLayer);
		}
		ASSERT( pLayer );
		return pLayer;
	}
	void	DelLayerMap(const CString& strLayer) {
		CLayerData* pLayer;
		// ���x�D��̂��߶��Ă���ۂɂȂ��Ă��ް��͏����Ȃ��D���قǕϓ��̂����ް��ł͂Ȃ�ʽ�
		if ( Lookup(strLayer, pLayer) )
			pLayer->Decrement();
	}

	void	InitialCutFlg(BOOL);
	CString	CheckDuplexFile(const CString&);

	BOOL	ReadLayerMap(LPCTSTR, CNCMakeOption* = NULL);
	BOOL	SaveLayerMap(LPCTSTR) const;
};

#endif // !defined(AFX_LAYER_H__696BA943_2D27_4A34_97D7_633F16942430__INCLUDED_)
