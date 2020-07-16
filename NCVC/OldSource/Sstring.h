/* ====================================================================== *
 *  SSTRING.HPP - Header of Class STRING_CUTTER and STRING_TESTER         *
 *                Prototype of Global Function for String Manipulation    *
 *                                                                        *
 *                                                          Ver.  2.0     *
 *                                                            by T.Soma   *
 *                                                          Ver.  2.1     *
 *                                                            by K.Magara *
 * ====================================================================== */
#ifndef ___SSTRING_H___
#define ___SSTRING_H___

#include <string.h>

/* ---------------------------------------------------------------------- *
 *  Class STRING_CUTTER
 */

class STRING_CUTTER
{
protected:
	char	*string,
			*string_save,
			*delimiter,
			*terminator,
			*blank,
			*bracket,
			*quotation,
			*start_point,
			*end_point;

	int		close_bracket,
			close_quotation;

	int		IsDelimiter(int c) {
		return ( strchr(delimiter, c) ? TRUE : FALSE );
	}
	int		IsTerminator(int c) {
		return ( strchr(terminator, c) ? TRUE : FALSE );
	}
	int		IsBlank(int c) {
		return ( strchr(blank, c) ? TRUE : FALSE );
	}
	void	Delete(char*& s) {
		if ( s ) delete[] s;
		s = NULL;
	}
	void	DeleteDelimiter(void) {
		Delete(delimiter);
	}
	void	DeleteTerminator(void) {
		Delete(terminator);
	}
	void	DeleteBlank(void) {
		Delete(blank);
	}
	void	DeleteBracket(void) {
		Delete(bracket);
	}
	void	DeleteQuotation(void) {
		Delete(quotation);
	}
	char*	CopyOf(const char* s) {
		char*	b;
		if ( s ) {
			if ( (b=new char[lstrlen(s)+1]) )
				lstrcpy(b, s);
		}
		else {
			if ( (b=new char[1]) )
				b[0] = '\0';
		}
		return b;
	}
	int		SkipBlank(void);
	int		GetCuttingPosition(void);
	int		IsOpenBracket(int character);
	int		IsOpenQuotation(int character);

public:
	STRING_CUTTER(const char* s = "",
			const char* d="/", const char* t="",
			const char* b=" \t\n", const char* r="[](){}", const char* q="''\"\"") {
		string_save = NULL;
		delimiter = terminator = blank = bracket = quotation = NULL;
		close_bracket = close_quotation = 0;
		Set(s);
		SetDelimiter(d);
		SetTerminator(t);
		SetBlank(b);
		SetBracket(r);
		SetQuotation(q);
	}
	~STRING_CUTTER(void) {
		Flush();
		DeleteDelimiter();
		DeleteTerminator();
		DeleteBlank();
		DeleteBracket();
		DeleteQuotation();
	}
	void	Set(const char* s) {
		Flush();
		string = string_save = CopyOf(s);
	}
	void	SetDelimiter(const char* d) {
		DeleteDelimiter();
		delimiter = CopyOf(d);
	}
	void	SetTerminator(const char* t) {
		DeleteTerminator();
		terminator = CopyOf(t);
	}
	void	SetBlank(const char* b) {
		DeleteBlank();
		blank = CopyOf(b);
	}
	void	SetBracket(const char* b) {
		DeleteBracket();
		bracket = CopyOf(b);
	}
	void	SetQuotation(const char* q) {
		DeleteQuotation();
		quotation = CopyOf(q);
	}
	char*	GiveAPiece(char* b,int lb) {
		int		lp;
		if ( !SkipBlank() ) {
			*b = NULL;
			return NULL;
		}
		lp = GetCuttingPosition();
		if ( lb > lp ) {
			strncpy(b, start_point, lp);
			*(b+lp)     = NULL;
			start_point = NULL;
			end_point   = NULL;
		}
		else {
			strncpy(b, start_point, lb-1);
			*(b+lb-1)    = NULL;
			start_point += (lb-1);
		}
		return b;
	}
	char*	GiveTheRest(char* b, int l) {
		int		s;
		if ( IsPieceContinued() )
			string = start_point;
		start_point = end_point = NULL;
		if ( IsEmpty() ) {
			*b = NULL;
			return NULL;
		}
		s = lstrlen(string);
		if ( l > s ) {
			lstrcpy(b, string);
			string += s;
		}
		else {
			strncpy(b, string, l-1);
			*(b+l-1) = NULL;
			string  += (l-1);
		}
		return b;
	}
	void	Flush(void) {
		Delete(string_save);
		string = NULL;
		start_point = end_point = NULL;
	}
	void	Rewind(void) {
		string = string_save;
		start_point = end_point = NULL;
	}
	int		IsPieceContinued(void) {
		if ( start_point || end_point )
			return TRUE;
		return FALSE;
	}
	int		IsEmpty(void) {
		if ( !string )
			return TRUE;
		if ( !(*string) && !start_point )
			return TRUE;
		return FALSE;
	}
};

/* ---------------------------------------------------------------------- *
 *  Class STRING_CUTTER_EX
 *		by K.Magara
 */

class STRING_CUTTER_EX : public STRING_CUTTER
{
	int		m_nLen;
	char*	m_lpPiece;	// GiveAPieceEx()—p

public:
	STRING_CUTTER_EX(const char* d, const char* r="[](){}") :
			STRING_CUTTER(NULL, d, NULL, NULL, r/*, q*/) {
		m_lpPiece = NULL;
		m_nLen = -1;
	}
	~STRING_CUTTER_EX() {
		if ( m_lpPiece )
			delete[]	m_lpPiece;
	}

	void	Set(const char* s) {
		int nLen = lstrlen(s);
		if ( m_nLen < nLen ) {
			STRING_CUTTER::Set(s);
			m_nLen = nLen;
			if ( m_lpPiece )
				delete[]	m_lpPiece;
			m_lpPiece = new char[m_nLen+1];
		}
		else {
			lstrcpy(string_save, s);
			Rewind();
		}
	}
	BOOL	GiveAPieceEx(CString& strPiece) {
		BOOL	bResult;
		if ( GiveAPiece(m_lpPiece, m_nLen+1) ) {
			strPiece = m_lpPiece;
			bResult = TRUE;
		}
		else {
			strPiece.Empty();
			bResult = FALSE;
		}
		return bResult;
	}
};

/* ---------------------------------------------------------------------- *
 *  class STRING_TESTER
 */

class STRING_TESTER
{
protected:
	char	**element;
	int		num_element;
	BOOL	fg_exact_match,
			fg_case_sensitive,
			fg_move_pointer;

	void	DeleteElement(void);

public:
	STRING_TESTER(int n, const char **a,
			BOOL e=TRUE, BOOL c=FALSE, BOOL m=FALSE) {
		element = NULL;
		num_element = 0;
		SetExactMatch(e);
		SetCaseSensitive(c);
		SetMovePointer(m);
		SetElement(n, a);
	}
	STRING_TESTER(const char **a,
			BOOL e=TRUE, BOOL c=FALSE, BOOL m=FALSE) {
		element = NULL;
		num_element = 0;
		SetExactMatch(e);
		SetCaseSensitive(c);
		SetMovePointer(m);
		SetElement(a);
	}

	~STRING_TESTER(void) {
		DeleteElement();
	}

	void	SetElement(const char **element);
	void	SetElement(int num_element, const char **element);

	void	SetExactMatch(BOOL f) {
		fg_exact_match = f;
	}
	void	SetCaseSensitive(BOOL f) {
		fg_case_sensitive = f;
	}
	void	SetMovePointer(BOOL f) {
		fg_move_pointer = f;
	}

	int		Test(char *&string);
};

/* ---------------------------------------------------------------------- *
 *  Class STRING_TESTER_EX
 *			Non move pointer version
 *		by K.Magara
 */

class STRING_TESTER_EX : public STRING_TESTER
{
public:
	STRING_TESTER_EX(int n, const char **a,
			BOOL e=TRUE, BOOL c=FALSE) :
			STRING_TESTER(n, a, e, c, FALSE) {
	}

	void	SetMovePointer(BOOL) {
		return;
	}
	int		TestEx(CString&);
};

#endif
