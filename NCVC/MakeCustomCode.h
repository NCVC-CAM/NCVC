// ����ͯ�ް, ̯�ް�����̊��׽
//	from TH_MakeXXX.cpp
//////////////////////////////////////////////////////////////////////

#pragma once

class	CDXFDoc;
class	C3dModelDoc;
class	CDXFdata;
class	CNCMakeOption;

class CMakeCustomCode
{
	const CNCMakeOption*	m_pMakeOpt;
	// boost::variant<> �ɂ���قǂł��Ȃ�
	const CDXFDoc*			m_pDXFDoc;
	const C3dModelDoc*		m_p3DMDoc;

//	BOOL	IsNCchar(LPCTSTR) const;

protected:
	CStringKeyIndex	m_strOrderIndex;	// ���ް������
	const CDXFdata*	m_pData;			// �h���׽����Q��

public:
	CMakeCustomCode(const CDXFDoc*,     const CDXFdata*, const CNCMakeOption*);
	CMakeCustomCode(const C3dModelDoc*, const CDXFdata*, const CNCMakeOption*);

	boost::tuple<int, CString>	ReplaceCustomCode(const std::string&) const;
};

// '{' �� '}' �� token
//	boost::tokenizer<custom_separator>	tokens(str);
//	�I�Ɏg�p

struct custom_separator {
	void reset() {}		// Unused
	
	template<typename Iterator, typename Token>
	bool operator()(Iterator& i, Iterator end, Token& tok) {
		tok = Token();
		if ( i == end )
			return false;
		
		if ( *i == '{' ) {
			for ( ; i!=end && *i!='}'; ++i )
				tok += *i;
			if ( i == end )
				return false;
			
			tok += *i++;
			return true;
		}
		else {
			for ( ; i!=end && *i!='{'; ++i )
				tok += *i;
			return true;
		}
	}
};
//typedef boost::tokenizer<custom_separator>::iterator tokIte;
