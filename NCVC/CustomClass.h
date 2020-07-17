// CustomClass.h: ｶｽﾀﾑｸﾗｽ
//
//////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////
// 配列・コンテナのzeroクリア

#define	ZEROCLR(ar)		for ( auto& ref : ar ) ref = 0;

//////////////////////////////////////////////////////////////////////
// CList, CMap用のFOREACH

#define	PLIST_FOREACH(FOO, LIST) \
	for ( POSITION pos=(LIST)->GetHeadPosition(); pos; ) { \
		FOO = (LIST)->GetNext(pos);
#define	PMAP_FOREACH(KEY, VAL, MAP) \
	for ( POSITION pos=(MAP)->GetStartPosition(); pos; ) { \
		(MAP)->GetNextAssoc(pos, KEY, VAL);
#define	END_FOREACH		}

//////////////////////////////////////////////////////////////////////
//	CTypedPtrArrayEx : 拡張 CTypedPtrArray

template<class BASE_CLASS, class TYPE>
class CTypedPtrArrayEx : public CTypedPtrArray<BASE_CLASS, TYPE>
{
public:
	TYPE	GetHead(void) const {
		ASSERT( !IsEmpty() );
		return GetAt(0);
	}
	TYPE	GetTail(void) const {
		ASSERT( !IsEmpty() );
		return GetAt(GetUpperBound());
	}
	void	Reverse(void) {
		if ( GetSize() <= 1 )	// ｵﾌﾞｼﾞｪｸﾄ1つ以下なら交換の必要なし
			return;
		TYPE*	pObj = (TYPE *)GetData();
		int	ii = GetSize() >> 1;	// 1/2
		for ( int i=0, j=GetUpperBound(); i<ii; i++, j-- )
			boost::swap(pObj[i], pObj[j]);
	}
};

//////////////////////////////////////////////////////////////////////
//	CSortArray : Sort機能を持った CObArrayｺﾝﾃﾅ

template<class BASE_CLASS, class TYPE>
class CSortArray : public CTypedPtrArrayEx<BASE_CLASS, TYPE>
{
public:
	typedef	int	(*PFNCOMPARE)(TYPE pFirst, TYPE pSecond);

private:
	void	QSort(PFNCOMPARE pfnCompare, int nFirst, int nLast) {
		if ( nFirst >= nLast )
			return;

		int		i = nFirst + 1, j = nLast;
		TYPE*	pObj = (TYPE *)GetData();

		while ( i <= j ) {
			// 基準値が大きい間繰り返し
			for ( ; i <= j && (*pfnCompare)(pObj[i], pObj[nFirst]) <= 0; i++ );
			// 基準値が小さい間繰り返し
			for ( ; i <= j && (*pfnCompare)(pObj[j], pObj[nFirst]) >= 0; j-- );
			//
			if ( i < j ) {
				boost::swap(pObj[i], pObj[j]);
				i++;	j--;
			}
		}

		boost::swap(pObj[j], pObj[nFirst]);

		QSort( pfnCompare, nFirst, j-1 );
		QSort( pfnCompare, j+1, nLast );
	}

public:
	void	Sort(PFNCOMPARE pfnCompare) {
		if ( GetSize() > 1 )
			QSort( pfnCompare, 0, GetUpperBound());
	}
};

//////////////////////////////////////////////////////////////////////
//	CTypedPtrListEx : 拡張 CTypedPtrList

template<class BASE_CLASS, class TYPE>
class CTypedPtrListEx : public CTypedPtrList<BASE_CLASS, TYPE>
{
public:
	void	Reverse(void) {
		if ( GetSize() <= 1 )	// ｵﾌﾞｼﾞｪｸﾄ1つ以下なら交換の必要なし
			return;
		POSITION	posH = GetHeadPosition(), posHb,
					posT = GetTailPosition(), posTb;
		TYPE		pObjH, pObjT;
		BOOL		bLoop = TRUE;
		while ( bLoop && posH != posT ) {
			posHb = posH;
			pObjH = GetNext(posH);
			if ( posH == posT )		// 要素数が偶数のとき
				bLoop = FALSE;
			posTb = posT;
			pObjT = GetPrev(posT);
			SetAt(posHb, pObjT);
			SetAt(posTb, pObjH);
		}
	}
};

//////////////////////////////////////////////////////////////////////
//	CStringKeyIndex : CMapStringToPtrを使って文字列キーのｲﾝﾃﾞｯｸｽを検索

class CStringKeyIndex : public CMapStringToPtr
{
	BOOL	m_bCaseUpper;

public:
	CStringKeyIndex() : CMapStringToPtr() {
		m_bCaseUpper = TRUE;
	}
	CStringKeyIndex(size_t nSize, LPCTSTR pszElement[], BOOL bCaseUpper = TRUE) : CMapStringToPtr() {
		m_bCaseUpper = bCaseUpper;
		SetElement(nSize, pszElement);
	}
	CStringKeyIndex(const CStringKeyIndex& cp) {	// ｺﾋﾟｰｺﾝｽﾄﾗｸﾀないとC2248ｺﾝﾊﾟｲﾙｴﾗｰ
		m_bCaseUpper = cp.m_bCaseUpper;
		RemoveAll();
		InitHashTable(cp.GetHashTableSize());
		CString	rKey;
		LPVOID	rValue;
		PMAP_FOREACH(rKey, rValue, &cp)
			SetAt(rKey, rValue);
		END_FOREACH
	}

	void	SetElement(size_t nSize, LPCTSTR pszElement[]) {
		RemoveAll();
		InitHashTable(::GetPrimeNumber(max(17, nSize)));
		AddElement(nSize, pszElement);
	}
	void	AddElement(size_t nSize, LPCTSTR pszElement[]) {
		INT_PTR	nBase = GetSize();
		CString	strElement;
		for ( size_t i=0; i<nSize; i++ ) {
			strElement = pszElement[i];
			if ( m_bCaseUpper )
				strElement.MakeUpper();
			SetAt(strElement, reinterpret_cast<LPVOID>(i+nBase));
		}
	}

	INT_PTR	GetIndex(LPCTSTR pszKey) const {
		LPVOID	pIndex;
		CString	strKey(pszKey);
		if ( m_bCaseUpper )
			strKey.MakeUpper();
		return Lookup(strKey, pIndex) ? reinterpret_cast<INT_PTR>(pIndex) : -1;
	}
};
