// ����ͯ�ް, ̯�ް�����̊��׽
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
	CStringKeyIndex	m_strOrderIndex;	// ���ް������
	const CDXFdata*	m_pData;			// �h���׽����Q��
	CString&		m_strResult;		// �u������(�Q�ƌ^)

public:
	CMakeCustomCode(CString&,
		const CDXFDoc*, const CDXFdata*, const CNCMakeOption*);

	int		ReplaceCustomCode(const char*, const char*) const;
};
