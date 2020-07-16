// CustomClass.h: ���Ѹ׽
//
//////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////
//	CTypedPtrArrayEx : �g�� CTypedPtrArray

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
		if ( GetSize() <= 1 )	// ��޼ު��1�ȉ��Ȃ�����̕K�v�Ȃ�
			return;
		TYPE*	pObj = (TYPE *)GetData();
		int	ii = GetSize() >> 1;	// 1/2
		for ( int i=0, j=GetUpperBound(); i<ii; i++, j-- )
			swap(pObj[i], pObj[j]);
	}
};

//////////////////////////////////////////////////////////////////////
//	CSortArray : Sort�@�\�������� CObArray����

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
			// ��l���傫���ԌJ��Ԃ�
			for ( ; i <= j && (*pfnCompare)(pObj[i], pObj[nFirst]) <= 0; i++ );
			// ��l���������ԌJ��Ԃ�
			for ( ; i <= j && (*pfnCompare)(pObj[j], pObj[nFirst]) >= 0; j-- );
			//
			if ( i < j ) {
				swap(pObj[i], pObj[j]);
				i++;	j--;
			}
		}

		swap(pObj[j], pObj[nFirst]);

		QSort( pfnCompare, nFirst, j-1 );
		QSort( pfnCompare, j+1, nLast );
	}

public:
	void	Sort(PFNCOMPARE pfnCompare) {
		if ( GetSize() > 1 )
			QSort( pfnCompare, 0, GetUpperBound());
	}
};
/*
//////////////////////////////////////////////////////////////////////
//	CTypedPtrListEx : �g�� CTypedPtrList

template<class BASE_CLASS, class TYPE>
class CTypedPtrListEx : public CTypedPtrList<BASE_CLASS, TYPE>
{
public:
	void	Reverse(void) {
		if ( GetSize() <= 1 )	// ��޼ު��1�ȉ��Ȃ�����̕K�v�Ȃ�
			return;
		POSITION	posH = GetHeadPosition(), posHb,
					posT = GetTailPosition(), posTb;
		TYPE		pObjH, pObjT;
		BOOL		bLoop = TRUE;
		while ( bLoop && posH != posT ) {
			posHb = posH;
			pObjH = GetNext(posH);
			if ( posH == posT )		// �v�f���������̂Ƃ�
				bLoop = FALSE;
			posTb = posT;
			pObjT = GetPrev(posT);
			SetAt(posHb, pObjT);
			SetAt(posTb, pObjH);
		}
	}
	// �S�Ă̗v�f�����ފ֐����Ăяo��
	template<class TYPE>
	void	AllElementCall( void (TYPE::*pfnFunc)(void) ) {	// ���z�֐����w��ł��Ȃ��̂ŋp��
		for ( POSITION pos=GetHeadPosition(); pos; )
			(GetNext(pos)->*pfnFunc)();
	}
};
*/
//////////////////////////////////////////////////////////////////////
//	CStringKeyIndex : CMapStringToPtr���g���ĕ�����L�[�̲��ޯ��������

class CStringKeyIndex : public CMapStringToPtr
{
	BOOL	m_bCaseUpper;

public:
	CStringKeyIndex(size_t nSize, LPCTSTR pszElement[], BOOL bCaseUpper = TRUE) {
		InitHashTable(::GetPrimeNumber(max(17, nSize)));
		CString	strElement;
		for ( size_t i=0; i<nSize; i++ ) {
			strElement = pszElement[i];
			if ( bCaseUpper )
				strElement.MakeUpper();
			SetAt(strElement, reinterpret_cast<LPVOID>(i));
		}
		m_bCaseUpper = bCaseUpper;
	}

	INT_PTR	GetIndex(LPCTSTR pszKey) {
		LPVOID	pIndex;
		CString	strKey(pszKey);
		if ( m_bCaseUpper )
			strKey.MakeUpper();
		return Lookup(strKey, pIndex) ? reinterpret_cast<INT_PTR>(pIndex) : -1;
	}
};

//////////////////////////////////////////////////////////////////////
//	std::string �⏕�֐�

//�O��̋󔒕������폜
inline std::string Trim(const std::string &str)
{
	std::string	strResult;
    size_t sPos = str.find_first_not_of(' ');
    size_t ePos = str.find_last_not_of(' ');
    if ( sPos != -1 )
		strResult = str.substr(sPos, ePos - sPos + 1);
    return strResult;
}
