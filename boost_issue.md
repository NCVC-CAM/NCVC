Algorithm functions cannot be used with CTypedPtrArray of MFC.


If you use an algorithm function with CTypedPtrArray, you will get a compile error where the template arguments do not match with std::swap().  
I used reverse() as an example, but the same was true for sort() and so on.  
The following sample code is a console application + MFC, and unnecessary initialization is omitted.  
Boost 1.76, Tried with Visual Studio 2015 and 2019

```C++
#include "boost/foreach.hpp"
#include "boost/range/mfc.hpp"
#include "boost/range/algorithm.hpp"

class CMyClass
{
	int	_m;
public:
	CMyClass(int m):_m(m) {}
	int	GetType(void) {
		return _m;
	}
};

int main()
{
	CTypedPtrArray<CPtrArray, CMyClass*>	ar;
	CMyClass*	p;
	for ( int i=0; i<3; i++ ) {
		p = new CMyClass(i);
		ar.Add(p);
	}
	BOOST_FOREACH(p, ar) {
		printf("%d\n", p->GetType());
	}
	boost::reverse(ar);     // Error with std::swap()
	BOOST_FOREACH(p, ar) {
		printf("%d\n", p->GetType());
		delete	p;
	}
}
```
It works fine with CPtrArray instead of CTypedPtrArray.

```C++
	CPtrArray	ar;
	CMyClass*	p;
	for ( int i=0; i<3; i++ ) {
		p = new CMyClass(i);
		ar.Add(p);
	}
	BOOST_FOREACH(auto pp, ar) {
		printf("%d\n", ((CMyClass*)pp)->GetType());
	}
	boost::reverse(ar);     // ok !!
	BOOST_FOREACH(auto pp, ar) {
		printf("%d\n", ((CMyClass*)pp)->GetType());
		delete	pp;
	}
```
I suspect something is missing in the CTypedPtrArray definition in mfc.hpp, but I didn't know what it was.