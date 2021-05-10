// TestConsole.cpp : �R���\�[�� �A�v���P�[�V�����̃G���g�� �|�C���g���`���܂��B
//

#include "stdafx.h"
#include "TestConsole.h"

#include "boost/foreach.hpp"
#include "boost/range/mfc.hpp"
#include "boost/range/algorithm.hpp"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// �B��̃A�v���P�[�V���� �I�u�W�F�N�g�ł��B

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
        // MFC �����������āA�G���[�̏ꍇ�͌��ʂ�������܂��B
        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
        {
            // TODO: �K�v�ɉ����ăG���[ �R�[�h��ύX���Ă��������B
            wprintf(L"�v���I�ȃG���[: MFC �̏��������ł��܂���ł����B\n");
            nRetCode = 1;
        }
    }
    else
    {
        // TODO: �K�v�ɉ����ăG���[ �R�[�h��ύX���Ă��������B
        wprintf(L"�v���I�ȃG���[: GetModuleHandle �����s���܂���\n");
        nRetCode = 1;
    }

	if ( nRetCode != 0 )	return nRetCode;


    // TODO: �A�v���P�[�V�����̓�����L�q����R�[�h�������ɑ}�����Ă��������B
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
