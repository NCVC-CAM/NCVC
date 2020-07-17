// ViewSetup2.h : �w�b�_�[ �t�@�C��
//

#pragma once

#include "NCVCdefine.h"

/////////////////////////////////////////////////////////////////////////////
// CViewSetup2 �_�C�A���O

class CViewSetup2 : public CPropertyPage
{
	COLORREF	m_colView[14];
	CBrush		m_brColor[14];
	
	void	EnableControl(void);

// �R���X�g���N�V����
public:
	CViewSetup2();
	~CViewSetup2();

// �_�C�A���O �f�[�^
	//{{AFX_DATA(CViewSetup2)
	enum { IDD = IDD_VIEW_SETUP2 };
	BOOL	m_bDrawCircleCenter;
	CButton	m_ctGuide;
	//}}AFX_DATA
	BOOL		m_bGuide[2];
	CStatic		m_ctColor[14];
	CComboBox	m_cbLineType[9];
	CFloatEdit	m_dGuide[NCXYZ];

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B

	//{{AFX_VIRTUAL(CViewSetup2)
	public:
	virtual BOOL OnApply();
	virtual BOOL OnKillActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
protected:
	// �������ꂽ���b�Z�[�W �}�b�v�֐�
	//{{AFX_MSG(CViewSetup2)
	virtual BOOL OnInitDialog();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnChange();
	afx_msg void OnColorButton();
	afx_msg void OnScale();
	afx_msg void OnDefColor();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
