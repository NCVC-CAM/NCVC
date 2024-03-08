// TH_Cuttime.cpp
// �؍펞�Ԍv�Z
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "NCVC.h"
#include "MachineOption.h"
#include "NCdata.h"
#include "NCDoc.h"
#include "NCInfoView.h"
#include "ThreadDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define	IsThread()	pDoc->IsDocFlag(NCDOC_CUTCALC)

static	float	GetCutTime_Milling(const CNCdata*);
static	float	GetCutTime_Lathe(const CNCdata*);

//////////////////////////////////////////////////////////////////////
//	�؍펞�Ԃ̌v�Z�گ��
//////////////////////////////////////////////////////////////////////

UINT CNCDoc::CuttimeCalc_Thread(LPVOID pVoid)
{
#ifdef _DEBUG
	printf("CuttimeCalc_Thread() Start\n");
#endif
	LPNCVCTHREADPARAM	pParam = reinterpret_cast<LPNCVCTHREADPARAM>(pVoid);
	CNCDoc*			pDoc = static_cast<CNCDoc*>(pParam->pDoc);
	CNCInfoView1*	pView = reinterpret_cast<CNCInfoView1*>(pParam->wParam);
	ASSERT(pDoc);
	ASSERT(pView);
	delete	pVoid;	// ���S�ޯ���׳��ޏ���

	const CMachineOption* pMCopt = AfxGetNCVCApp()->GetMachineOption();	// G00�ʒu���߈ړ����x�擾�p
	INT_PTR		i, j, nLoopCnt = pDoc->GetNCsize();
	float		dTmp, dd;
	DWORD		dwValFlags;
	CNCdata*	pData;

#ifdef _DEBUG
	printf("GetNCsize()=%Id\n", nLoopCnt);
#endif

	// �ړ��E�؍풷�̍��v�v�Z
	ZEROCLR(pDoc->m_dMove);
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
		printf("pMCopt->IsZeroG0Speed() Can't calc return\n");
#endif
		pDoc->m_dCutTime = -1.0f;	// �����l�u-1�v�ŕ\������
		pView->PostMessage(WM_USERPROGRESSPOS);
		return 0;
	}

	pDoc->m_dCutTime = 0.0f;
	boost::function<float (const CNCdata*)>	pfnGetCutTime =
		pDoc->IsDocFlag(NCDOC_LATHE) ?
		GetCutTime_Lathe : GetCutTime_Milling;

	for ( i=0; i<nLoopCnt && IsThread(); i++ ) {
		pData = pDoc->GetNCdata(i);
		if ( pData->GetGtype() != G_TYPE )
			continue;
		// ���ނ��Ƃ̎��Ԍv�Z
		switch ( pData->GetGcode() ) {
		case 0:	// ������
			dTmp = 0.0f;
			for ( j=0; j<NCXYZ; j++ ) {
				dd = pData->GetMove(j) / (pMCopt->GetG0Speed(j) / 60.0f);
				dTmp += (dd * dd);
			}
			pDoc->m_dCutTime += sqrt(dTmp);
			break;
		case 1:		// �������
		case 2:		// �~�ʕ��
		case 3:
			if ( pData->GetFeed() != 0 )
				pDoc->m_dCutTime += pfnGetCutTime(pData);
			break;
		case 4:		// �޳��
			dwValFlags = pData->GetValFlags();
			if ( dwValFlags & NCD_P )		// P�l��msec�P��
				pDoc->m_dCutTime += pData->GetValue(NCA_P) / 1000.0f;
			else if ( dwValFlags & NCD_X )	// X�l��sec�P��
				pDoc->m_dCutTime += pData->GetValue(NCA_X);
			break;
		case 73:	case 74:	case 76:	// �Œ軲��
		case 81:	case 82:	case 83:	case 84:	case 85:
		case 86:	case 87:	case 88:	case 89:
			// �����蕪
			dTmp = 0.0f;
			for ( j=0; j<NCXYZ; j++ ) {
				dd = pData->GetMove(j) / (pMCopt->GetG0Speed(j) / 60.0f);
				dTmp += (dd * dd);
			}
			pDoc->m_dCutTime += sqrt(dTmp);
			// �؍핪
			if ( pData->GetFeed() != 0 )
				pDoc->m_dCutTime += pData->GetCutLength() / (pData->GetFeed() / 60.0f);
			// �޳�َ���[��]
			pDoc->m_dCutTime += static_cast<CNCcycle*>(pData)->GetDwell();
			break;
		}
	} // End of Loop

	// ��ۯ������̏d��
	pDoc->m_dCutTime += pMCopt->GetDbl(MC_DBL_BLOCKWAIT) * pDoc->GetNCBlockSize();

	pView->PostMessage(WM_USERPROGRESSPOS);
#ifdef _DEBUG
	printf("PostMessage() Finish!\n");
#endif

	return 0;	// AfxEndThread(0);
}

//////////////////////////////////////////////////////////////////////

float GetCutTime_Milling(const CNCdata* pData)
{
	return pData->GetCutLength() / (pData->GetFeed() / 60.0f);
}

float GetCutTime_Lathe(const CNCdata* pData)
{
	// ��������v�Z
	float	dResult = GetCutTime_Milling(pData);

	// G99����]����Ȃ�
	if ( !pData->GetG98() ) {	
		if ( pData->GetSpindle() == 0 )	// �厲��]���͕K�{
			dResult = 0.0f;
		else
			dResult /= (pData->GetSpindle()/60.0f);	// �P��]������̎���[sec]���|����
													// *= (1.0/pData->GetSpindle());
	}

	return dResult;
}
