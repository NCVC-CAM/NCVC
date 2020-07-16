/* ====================================================================== *
 *  SSTRING.CPP - Definition of Class STRING_CUTTER and STRING_TESTER     *
 *                Definition of Global Function for String Manipulation   *
 *                                                                        *
 *  Major Modifications                                                   *
 *     V1.0,V1.1  Original version                                        *
 *     V2.0       Global functions added                                  *
 *                                                                        *
 *                                                          Ver.  2.0     *
 *                                                            by T.Soma   *
 *     V2.1       First Tuning                                            *
 *                                                          Ver.  2.1     *
 *                                                            by K.Magara *
 * ====================================================================== */
#include "StdAFX.h"
#include "Sstring.h"
#include <ctype.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#include "MagaDbgMac.h"
extern	CMagaDbg	g_dbg;
#endif

/* ---------------------------------------------------------------------- *
 *  Class STRING_CUTTER
 */

int STRING_CUTTER::SkipBlank(void)
{
	if ( IsEmpty() ) return FALSE;
	if ( IsPieceContinued() ) return TRUE;

	for ( ; *string; string++ )
		if( !IsBlank(*string) ) return TRUE;
	return FALSE;
}

int STRING_CUTTER::GetCuttingPosition(void)
{
	char*	p;

	if ( IsEmpty() ) return 0;
	if ( IsPieceContinued() )
		return ( (unsigned)end_point-(unsigned)start_point+1 );

	p = string;
	start_point = string;
	close_quotation = 0;
	close_bracket = IsOpenBracket(*p);
	if ( close_bracket == 0 )
		close_quotation = IsOpenQuotation(*p);

	if ( close_bracket!=0 || IsDelimiter(*p) ) {
		start_point = string;
		p = string+1;
	}
	if ( close_quotation != 0 ) {
		start_point = string + 1;
		p = string + 1;
	}
	for ( ; ; p++ ) {
		if ( !(*p) ) {
			end_point = p - 1;
			string = p;
			close_bracket = close_quotation = 0;
			break;
		}
		if ( close_bracket != 0 ) {
			if ( *p == close_bracket ) {
				end_point = p;
				string = p + 1;
				break;
			}
			else
				continue;
		}
		if ( close_quotation != 0 ) {
			if ( *p == close_quotation ) {
				end_point = p - 1;
				string = p + 1;
				break;
			}
			else
				continue;
		}
		if ( IsBlank(*p) || IsDelimiter(*p) || IsOpenBracket(*p) || IsOpenQuotation(*p) ) {
			end_point = p - 1;
			string = p;
			break;
		}
		if ( IsTerminator(*p) ) {
			end_point = p;
			string = p + 1;
			break;
		}
	}
	return ( (unsigned)end_point-(unsigned)start_point+1 );
}

int STRING_CUTTER::IsOpenBracket(int c)
{
    for ( int i=0; *(bracket+i); i++ )
		if ( !(i%2) && *(bracket+i)==c )
			return *(bracket+i+1);
	return 0;
}

int STRING_CUTTER::IsOpenQuotation(int c)
{
	for ( int i=0; *(quotation+i); i++ )
		if ( !(i%2) && *(quotation+i)==c )
			return *(quotation+i+1);
	return 0;
}

/* -------------------------------------------------------------------------- *
 *  Class STRING_TESTER
 */

void STRING_TESTER::DeleteElement(void)
{
	int		i;
	if ( element ) {
		for( i=0; i<num_element; i++ )
			delete[] *(element+i);
		delete[] element;
		element = NULL;
	}
	num_element = 0;
}

void STRING_TESTER::SetElement(const char **a)
{
	int		i;
	for ( i=0; *(a+i); i++ );
	SetElement(i, a);
}

void STRING_TESTER::SetElement(int n, const char **a)
{
	int		i;

	DeleteElement();

	element = new char*[n];
	for ( i=0; i<n; i++) {
		*(element+i) = new char[lstrlen(*(a+i))+1];
		lstrcpy(*(element+i), *(a+i));
	}
	num_element = n;
}

/* -------------------------------------------------------------------------- *
 *  Test() - Search paramter string in the table
 *
 *  IN : String to be searched
 *  RET: 0      String not found
 *       Non-0  Element number matched with the string given
 */
int STRING_TESTER::Test(char *&s)
{
	char*	e;

	for ( int i=0; i<num_element; i++) {
		if ( fg_exact_match && (lstrlen(element[i])!=lstrlen(s)) )
			continue;
		e = element[i];
		for ( int j=0; ; j++ ) {
			if ( !(*(e+j)) ) {
				if ( fg_exact_match && *(s+j) )
					break;
				if ( fg_move_pointer )
					s = s + j;
				return i+1;
			}
			if ( fg_case_sensitive ) {
				if ( *(e+j) != *(s+j) )
					break;
			}
			else {
				if ( toupper(*(e+j)) != toupper(*(s+j)) )
					break;
			}
		}
	}
	return 0;
}

/* -------------------------------------------------------------------------- *
 *  Class STRING_TESTER_EX
 */

int STRING_TESTER_EX::TestEx(CString& strBuf)
{
	CString	strElement, strTest(strBuf);
	if ( !fg_case_sensitive )
		strTest.MakeUpper();

	for ( int i=0; i<num_element; i++) {
		strElement = element[i];
		if ( !fg_case_sensitive )
			strElement.MakeUpper();
		if ( fg_exact_match ) {
			if ( strTest == strElement )
				return i+1;
		}
		else {
			if ( strTest.Find(strElement) >= 0 )
				return i+1;
		}
	}
	return 0;
}
