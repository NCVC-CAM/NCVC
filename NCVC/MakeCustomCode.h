// ｶｽﾀﾑﾍｯﾀﾞｰ, ﾌｯﾀﾞｰ処理の基底ｸﾗｽ
//	from TH_MakeNCD.cpp TH_MakeLathe.cpp
//////////////////////////////////////////////////////////////////////

#pragma once

class	CDXFDoc;
class	CDXFdata;
class	CNCMakeOption;

class CMakeCustomCode
{
	const CDXFDoc*			m_pDoc;
	const CNCMakeOption*	m_pMakeOpt;

	BOOL	IsNCchar(LPCTSTR) const;

protected:
	CStringKeyIndex	m_strOrderIndex;	// ｵｰﾀﾞｰ文字列
	const CDXFdata*	m_pData;			// 派生ｸﾗｽから参照

public:
	CMakeCustomCode(const CDXFDoc*, const CDXFdata*, const CNCMakeOption*);

	boost::tuple<int, CString>	ReplaceCustomCode(const std::string&) const;
};

// '{' と '}' の token
//	boost::tokenizer<tag_separator>	tokens(str);
//	的に使用

struct tag_separator {
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

typedef boost::tokenizer<tag_separator>::iterator tokIte;
