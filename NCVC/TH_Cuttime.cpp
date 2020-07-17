// TH_Cuttime.cpp
// �؍펞�Ԍv�Z
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "NCVC.h"
#include "MCOption.h"
#include "NCdata.h"
#include "NCDoc.h"
#include "NCInfoView.h"
#include "ThreadDlg.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

#define	IsThread()	pDoc->IsCalcContinue()

//////////////////////////////////////////////////////////////////////
//	�؍펞�Ԃ̌v�Z�گ��
//////////////////////////////////////////////////////////////////////

UINT CNCDoc::CuttimeCalc_Thread(LPVOID pVoid)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CuttimeCalc_Thread()\nStart", DBG_BLUE);
#endif
	LPNCVCTHREADPARAM	pParam = reinterpret_cast<LPNCVCTHREADPARAM>(pVoid);
	CNCDoc*			pDoc = static_cast<CNCDoc*>(pParam->pDoc);
	CNCInfoView1*	pView = reinterpret_cast<CNCInfoView1*>(pParam->wParam);
	ASSERT(pDoc);
	ASSERT(pView);
	delete	pVoid;	// ���S�ޯ���׳��ޏ���

	const CMCOption*	pMCopt = AfxGetNCVCApp()->GetMCOption();	// G0�ʒu���߈ړ����x�擾�p
	int			i, j, nLoopCnt = pDoc->GetNCsize();
	double		dTmp, dd;
	DWORD		dwValFlags;
	CNCdata*	pData;

#ifdef _DEBUG
	dbg.printf("GetNCsize()=%d", nLoopCnt);
#endif

	// �ړ��E�؍풷�̍��v�v�Z
	pDoc->m_dMove[0] = pDoc->m_dMove[1] = 0.0;
	for ( i=0; i<nLoopCnt && IsThread(); i++ ) {
		pData = pDoc->GetNCdata(i);
		if ( pData->GetGtype() == G_TYPE ) {
			switch ( pData->GetType() ) {
			case NCDLINEDATA:	// �������
			case NCDARCDATA:	// �~�ʕ��
				// �a�␳���ŏI�_���ς��\�������邽�߁A�����Ōv�Z
				j = pData->GetGcode() > 0 ? 1 : 0;
				pDoc->m_dMove[j] += pData->SetCalcLength();
				break;
			case NCDCYCLEDATA:	// �Œ軲��
				pDoc->m_dMove[0] += static_cast<CNCcycle*>(pData)->GetCycleMove();
				pDoc->m_dMove[1] += pData->GetCutLength();
				break;
			}
		}
		// �ǂݍ��݌�ɕs�v�ɂȂ��ް��̍폜
		pData->DeleteReadData();
	}

	// �؍펞�Ԃ̌v�Z
	if ( pMCopt->IsZeroG0Speed() ) {
#ifdef _DEBUG
		dbg.printf("pMCopt->IsZeroG0Speed() Can't calc return");
#endif
		pDoc->m_dCutTime = -1.0;	// �����l�u-1�v�ŕ\������
		pView->PostMessage(WM_USERPROGRESSPOS);
		return 0;
	}
	pDoc->m_dCutTime = 0.0;
	for ( i=0; i<nLoopCnt && IsThread(); i++ ) {
		pData = pDoc->GetNCdata(i);
		if ( pData->GetGtype() != G_TYPE )
			continue;
		// ���ނ��Ƃ̎��Ԍv�Z
		switch ( pData->GetGcode() ) {
		case 0:	// ������
			dTmp = 0.0;
			for ( j=0; j<NCXYZ; j++ ) {
				dd = pData->GetMove(j) / pMCopt->GetG0Speed(j);
				dTmp += (dd * dd);
			}
			pDoc->m_dCutTime += sqrt(dTmp);
			break;
		case 1:		// �������
		case 2:		// �~�ʕ��
		case 3:
			if ( pData->GetFeed() != 0 )
				pDoc->m_dCutTime += pData->GetCutLength() / pData->GetFeed();
			break;
		case 4:		// �޳��
			dwValFlags = pData->GetValFlags();
			if ( dwValFlags & NCD_P )		// P�l��msec�P��
				pDoc->m_dCutTime += pData->GetValue(NCA_P) / 1000.0 / 60.0;
			else if ( dwValFlags & NCD_X )	// X�l��sec�P��
				pDoc->m_dCutTime += pData->GetValue(NCA_X) / 60.0;
			break;
		case 81:	// �Œ軲��
		case 82:
		case 85:
		case 86:
		case 89:
			// �����蕪
			dTmp = 0.0;
			for ( j=0; j<NCXYZ; j++ ) {
				dd = pData->GetMove(j) / pMCopt->GetG0Speed(j);
				dTmp += (dd * dd);
			}
			pDoc->m_dCutTime += sqrt(dTmp);
			// �؍핪
			if ( pData->GetFeed() != 0 )
				pDoc->m_dCutTime += pData->GetCutLength() / pData->GetFeed();
			// �޳�َ���[��]
			pDoc->m_dCutTime += static_cast<CNCcycle*>(pData)->GetDwell() / 60.0;
			break;
		}
	} // End of Loop

	// ��ۯ������̏d��
	pDoc->m_dCutTime += pMCopt->GetBlockTime() * pDoc->GetNCBlockSize() / 60.0;

	pView->PostMessage(WM_USERPROGRESSPOS);
#ifdef _DEBUG
	dbg.printf("PostMessage() Finish!");
#endif

	return 0;	// AfxEndThread(0);
}
