// TestConsole.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"
#include "TestConsole.h"

#include "boost/foreach.hpp"
#include "boost/range/mfc.hpp"
#include "boost/range/algorithm.hpp"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 唯一のアプリケーション オブジェクトです。

CWinApp theApp;
using namespace std;

class CMyClass
{
	int		_m;
public:
	CMyClass(int m):_m(m) {}
	int		GetType(void) {
		return _m;
	}
};


int main()
{
    int nRetCode = 0;

    HMODULE hModule = ::GetModuleHandle(nullptr);

    if (hModule != nullptr)
    {
        // MFC を初期化して、エラーの場合は結果を印刷します。
        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
        {
            // TODO: 必要に応じてエラー コードを変更してください。
            wprintf(L"致命的なエラー: MFC の初期化ができませんでした。\n");
            nRetCode = 1;
        }
    }
    else
    {
        // TODO: 必要に応じてエラー コードを変更してください。
        wprintf(L"致命的なエラー: GetModuleHandle が失敗しました\n");
        nRetCode = 1;
    }

	if ( nRetCode != 0 )	return nRetCode;


    // TODO: アプリケーションの動作を記述するコードをここに挿入してください。
//	CTypedPtrArray<CPtrArray, CMyClass*>	ar;
	CPtrArray	ar;
	CMyClass*	p;
	for ( int i=0; i<3; i++ ) {
		p = new CMyClass(i);
		ar.Add(p);
	}
//	BOOST_FOREACH(p, ar) {
//		printf("%d\n", p->GetType());
	BOOST_FOREACH(auto pp, ar) {
		printf("%d\n", ((CMyClass*)pp)->GetType());
    }
	boost::reverse(ar);     // Error with std::swap()
//	BOOST_FOREACH(p, ar) {
//		printf("%d\n", p->GetType());
//		delete	p;
	BOOST_FOREACH(auto pp, ar) {
		printf("%d\n", ((CMyClass*)pp)->GetType());
		delete	pp;
	}

	return nRetCode;
}
