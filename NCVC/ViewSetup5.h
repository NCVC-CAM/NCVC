// ViewSetup5.h : �w�b�_�[ �t�@�C��
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CViewSetup5 �_�C�A���O

class CViewSetup5 : public CPropertyPage
{
	COLORREF	m_colView[3];
	CBrush		m_brColor[3];

	void	EnableSolidControl(void);
	void	EnableTextureControl(void);

// �R���X�g���N�V����
public:
	CViewSetup5();
	virtual ~CViewSetup5();

// �_�C�A���O �f�[�^
	enum { IDD = IDD_VIEW_SETUP5 };
	CStatic	m_ctColor[3];
	BOOL	m_bSolid,
			m_bWirePath,
			m_bDrag,
			m_bTexture,
			m_bLatheSlit,
			m_bNoActiveTraceGL,
			m_bToolTrace;
	int		m_nMillType;
	CButton m_ctWirePath,
			m_ctDrag,
			m_ctTexture,
			m_ctLatheSlit,
			m_ctTextureFind;
	CEdit	m_ctTextureFile;
	CFloatEdit	m_dEndmill;
	CString	m_strTexture;

// �I�[�o�[���C�h
public:
	virtual BOOL OnKillActive();
	virtual BOOL OnApply();
protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g

// �C���v�������e�[�V����
protected:
	// �������ꂽ���b�Z�[�W �}�b�v�֐�
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnChange();
	afx_msg void OnColorButton();
	afx_msg void OnSolidClick();
	afx_msg void OnTextureClick();
	afx_msg void OnTextureFind();
	afx_msg void OnDefColor();

	DECLARE_MESSAGE_MAP()
};
