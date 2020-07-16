// DXFBlock.h: CDXFBlockData クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once

//
// DXF Blockｾｸｼｮﾝﾃﾞｰﾀ
class CDXFBlockData
{
	CString		m_strBlock;		// ﾌﾞﾛｯｸ名
	CPointD		m_ptOrg;		// ﾌﾞﾛｯｸ基点
	CDXFarray	m_obDXFArray;	// 要素ﾃﾞｰﾀ

public:
	CDXFBlockData(const CString& strBlock) {
		m_strBlock = strBlock;
		m_obDXFArray.SetSize(0, 1024);
	}
	virtual ~CDXFBlockData() {
		for ( int i=0; i<m_obDXFArray.GetSize(); i++ )
			delete	m_obDXFArray[i];
	}

// ｱﾄﾘﾋﾞｭｰﾄ
	const	CString		GetBlockName(void) {
		return m_strBlock;
	}
	int		GetSize(void) const {
		return m_obDXFArray.GetSize();
	}
	CDXFdata*	GetBlockData(int n) const {
		ASSERT(n>=0 && n<m_obDXFArray.GetSize());
		return m_obDXFArray[n];
	}
	CPointD		GetBlockOrigin(void) const {
		return m_ptOrg;
	}

// ｵﾍﾟﾚｰｼｮﾝ
	void	SetBlockOrigin(CPointD& pt) {
		m_ptOrg = pt;
	}
	// Blockﾃﾞｰﾀ登録(例外ｽﾛｰは上位でｷｬｯﾁ)
	void	AddData(LPDXFPARGV lpArgv) {
		lpArgv->c -= m_ptOrg;
		CDXFpoint*	pPoint = new CDXFpoint(lpArgv);
		ASSERT(pPoint);
		m_obDXFArray.Add(pPoint);
	}
	void	AddData(LPDXFLARGV lpArgv) {
		lpArgv->s -= m_ptOrg;
		lpArgv->e -= m_ptOrg;
		CDXFline*	pLine = new CDXFline(lpArgv);
		ASSERT(pLine);
		m_obDXFArray.Add(pLine);
	}
	void	AddData(LPDXFCARGV lpArgv) {
		lpArgv->c -= m_ptOrg;
		CDXFcircle*	pCircle = new CDXFcircle(lpArgv);
		ASSERT(pCircle);
		m_obDXFArray.Add(pCircle);
	}
	void	AddData(LPDXFAARGV lpArgv) {
		lpArgv->c -= m_ptOrg;
		CDXFarc*	pArc = new CDXFarc(lpArgv);
		ASSERT(pArc);
		m_obDXFArray.Add(pArc);
	}
	void	AddData(LPDXFEARGV lpArgv) {
		lpArgv->c -= m_ptOrg;
		CDXFellipse*	pEllipse = new CDXFellipse(lpArgv);
		ASSERT(pEllipse);
		m_obDXFArray.Add(pEllipse);
	}
	void	AddData(CDXFpolyline* pPolyline) {
		// CDXFpolyline の原点補正は SetVertex() にて
		ASSERT(pPolyline);
		m_obDXFArray.Add(pPolyline);
	}
	void	AddData(LPDXFTARGV lpArgv) {
		lpArgv->c -= m_ptOrg;
		CDXFtext*	pText = new CDXFtext(lpArgv);
		ASSERT(pText);
		m_obDXFArray.Add(pText);
	}
	//
	void	CopyBlock(const CDXFBlockData *, LPDXFBLOCK);
private:
	void	AddData(const CDXFpoint* pData, LPDXFBLOCK pBlock) {
		CDXFpoint*	pPoint = new CDXFpoint(NULL, pData, pBlock);
		ASSERT(pPoint);
		m_obDXFArray.Add(pPoint);
	}
	void	AddData(const CDXFline* pData, LPDXFBLOCK pBlock) {
		CDXFline*	pLine = new CDXFline(NULL, pData, pBlock);
		ASSERT(pLine);
		m_obDXFArray.Add(pLine);
	}
	void	AddData(const CDXFcircle* pData, LPDXFBLOCK pBlock) {
		CDXFcircle*	pCircle = new CDXFcircle(NULL, pData, pBlock);
		ASSERT(pCircle);
		m_obDXFArray.Add(pCircle);
	}
	void	AddData(const CDXFarc* pData, LPDXFBLOCK pBlock) {
		CDXFarc*	pArc = new CDXFarc(NULL, pData, pBlock);
		ASSERT(pArc);
		m_obDXFArray.Add(pArc);
	}
	void	AddData(const CDXFellipse* pData, LPDXFBLOCK pBlock) {
		CDXFellipse*	pEllipse = new CDXFellipse(NULL, pData, pBlock);
		ASSERT(pEllipse);
		m_obDXFArray.Add(pEllipse);
	}
	void	AddData(const CDXFpolyline* pData, LPDXFBLOCK pBlock) {
		CDXFpolyline*	pPolyline = new CDXFpolyline(NULL, pData, pBlock);
		ASSERT(pPolyline);
		m_obDXFArray.Add(pPolyline);
	}
	void	AddData(const CDXFtext* pData, LPDXFBLOCK pBlock) {
		CDXFtext*	pText = new CDXFtext(NULL, pData, pBlock);
		ASSERT(pText);
		m_obDXFArray.Add(pText);
	}
};
